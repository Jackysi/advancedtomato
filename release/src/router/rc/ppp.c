
/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

 This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 the contents of this file may not be disclosed to third parties,
 copied or duplicated in any form without the prior written
 permission of CyberTAN Inc.

 This software should be used as a reference only, and it not
 intended for production use!


 THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

/*
 * ppp scripts
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: ppp.c,v 1.27.18.1 2005/08/08 12:06:41 honor Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <net/route.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>

#include <code_pattern.h>
#include <cy_conf.h>
#include <utils.h>

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)


/*
 * Called when link comes up
 */
int
ipup_main(int argc, char **argv)
{
	FILE *fp;
	char *wan_ifname = safe_getenv("IFNAME");
	char *wan_proto = nvram_safe_get("wan_proto");
	char *value;
	char buf[256];

	dprintf("%s\n", argv[0]);

	eval("killall", "-9", "listen");

	nvram_set("wan_iface", wan_ifname);

	if(check_action() != ACT_IDLE)
                return -1;

	/* ipup will receive bellow six arguments */
	/* interface-name  tty-device  speed  local-IP-address   remote-IP-address ipparam */
	nvram_set("wan_iface", wan_ifname);
	/* Touch connection file */
	if (!(fp = fopen("/tmp/ppp/link", "a"))) {
		perror("/tmp/ppp/link");
		return errno;
	}
	fprintf(fp, "%s", argv[1]);
	fclose(fp);

	if ((value = getenv("IPLOCAL"))) {
		ifconfig(wan_ifname, IFUP,
			 value, "255.255.255.255");
		if(!strcmp(wan_proto, "pppoe")
		){
                        nvram_set("wan_ipaddr_buf", nvram_safe_get("wan_ipaddr"));	//Store last ip address
                        nvram_set("wan_ipaddr", value);
                        nvram_set("wan_netmask", "255.255.255.255");
                }
                else if(!strcmp(wan_proto, "pptp")){
                        nvram_set("wan_ipaddr_buf", nvram_safe_get("pptp_get_ip"));	// Store last ip address
                        nvram_set("pptp_get_ip", value);
                }
                else if(!strcmp(wan_proto, "l2tp")){
                        nvram_set("wan_ipaddr_buf", nvram_safe_get("l2tp_get_ip"));     // Store last ip address
                        nvram_set("l2tp_get_ip", value);
                }
	}

        if ((value = getenv("IPREMOTE")))
		nvram_set("wan_gateway", value);
	strcpy(buf, "");
	if (getenv("DNS1"))
		sprintf(buf, "%s", getenv("DNS1"));
	if (getenv("DNS2"))
		sprintf(buf + strlen(buf), "%s%s", strlen(buf) ? " " : "", getenv("DNS2"));
	nvram_set("wan_get_dns", buf);
	
        if ((value = getenv("AC_NAME")))
		nvram_set("ppp_get_ac", value);
        if ((value = getenv("SRV_NAME")))
		nvram_set("ppp_get_srv", value);
        if ((value = getenv("MTU")))
		nvram_set("wan_run_mtu", value);

	start_wan_done(wan_ifname);

	dprintf("done\n");
	return 0;
}

/*
 * Called when link goes down
 */
int
ipdown_main(int argc, char **argv)
{
	if(check_action() != ACT_IDLE)
                return -1;
	
	stop_ddns();    // Aviod to trigger DOD
        stop_ntp();

		unlink("/tmp/ppp/link");

		if(nvram_match("wan_proto", "l2tp")) {
			/* clear dns from the resolv.conf */
			nvram_set("wan_get_dns","");
			dns_to_resolv();

			route_del(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"), nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); //fixed routing problem in Israel by kanki

			/* Restore the default gateway for WAN interface */
			nvram_set("wan_gateway", nvram_safe_get("wan_gateway_buf"));

			/* Set default route to gateway if specified */
			route_add(nvram_safe_get("wan_ifname"), 0, "0.0.0.0", nvram_safe_get("wan_gateway"), "0.0.0.0");
		}

		if(nvram_match("ppp_demand", "1") && (nvram_match("wan_proto", "pptp") || nvram_match("wan_proto", "l2tp"))){
			eval("killall", "-9", "listen");
			eval("listen", nvram_safe_get("lan_ifname"));
		}	
	SET_LED(RELEASE_IP);
	dprintf("done\n");
		
	return 1;
}

int
pppevent_main(int argc, char **argv)
{
	int argn;
	char *type = NULL;

	argn = 1;
        while ( argn < argc && argv[argn][0] == '-' ){
		if ( strcmp( argv[argn], "-t" ) == 0 ) {
			++ argn;
			type = argv[argn];
		}
		++ argn;
	}
	
	if(!type)
		return 1;


	if(!strcmp(type, "PAP_AUTH_FAIL") || !strcmp(type, "CHAP_AUTH_FAIL")) {

		buf_to_file("/tmp/ppp/log", type);

		if(check_hw_type() == BCM4704_BCM5325F_CHIP)	
			SET_LED(GET_IP_ERROR);
	}

	return 0;
}

//=============================================================================
int
set_pppoepid_to_nv_main(int argc, char **argv) // tallest 1219
{
        if(!strcmp(argv[1],"0"))
	{
		nvram_set("pppoe_pid0",getenv("PPPD_PID"));
		nvram_set("pppoe_ifname0",getenv("IFNAME"));
	}
	else if(!strcmp(argv[1],"1"))
	{
		nvram_set("pppoe_pid1",getenv("PPPD_PID"));
		nvram_set("pppoe_ifname1",getenv("IFNAME"));
	}

	dprintf("done.( IFNAME = %s DEVICE = %s )\n",getenv("IFNAME"),getenv("DEVICE"));
        return 0;
}

//by tallest 0407
int
disconnected_pppoe_main(int argc, char **argv)
{
	int pppoe_num = atoi(argv[1]);
	char ppp_demand[2][20]={"ppp_demand","ppp_demand_1"};

	if(nvram_match(ppp_demand[pppoe_num], "1") && nvram_match("action_service",""))
	{
		cprintf("tallest:=====( kill pppoe %d )=====\n", pppoe_num);
		stop_singe_pppoe(pppoe_num);
		start_pppoe(pppoe_num);
		dns_to_resolv();
		start_dns();
        	return 0;
	}
	cprintf("tallest:=====( PPPOE Dial On Demand Error!! )=====\n");
	return 0;
}
//=============================================================================
