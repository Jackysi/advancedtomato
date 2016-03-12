/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

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

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/
/*
	$Id: ppp.c,v 1.27 2005/03/29 02:00:06 honor Exp $
*/

#include "rc.h"

#include <sys/ioctl.h>

void ppp_prefix(char *wan_device, char *prefix)
{	
	if(!strcmp(wan_device, nvram_safe_get("wan_ifnameX"))) strcpy(prefix, "wan");
	if(!strcmp(wan_device, nvram_safe_get("wan2_ifnameX"))) strcpy(prefix, "wan2");
#ifdef TCONFIG_MULTIWAN
	if(!strcmp(wan_device, nvram_safe_get("wan3_ifnameX"))) strcpy(prefix, "wan3");
	if(!strcmp(wan_device, nvram_safe_get("wan4_ifnameX"))) strcpy(prefix, "wan4");
#endif
}

int ipup_main(int argc, char **argv)
{
	char *wan_ifname;
	char *value;
	char buf[256];
	const char *p;
	
	char prefix[] = "wanXX";
	char tmp[100];
	char ppplink_file[256];

	TRACE_PT("begin\n");

	killall("listen", SIGKILL);
	
	if (!wait_action_idle(10)) return -1;

	wan_ifname = safe_getenv("IFNAME");
	ppp_prefix(safe_getenv("DEVICE"), prefix);
	if ((!wan_ifname) || (!*wan_ifname)) return -1;
	nvram_set(strcat_r(prefix, "_iface", tmp), wan_ifname);	// ppp#
	nvram_set(strcat_r(prefix, "_pppd_pid", tmp), safe_getenv("PPPD_PID"));	

	// ipup receives six arguments:
	//   <interface name>  <tty device>  <speed> <local IP address> <remote IP address> <ipparam>
	//   ppp1 vlan1 0 71.135.98.32 151.164.184.87 0
	memset(ppplink_file, 0, 256);
	sprintf(ppplink_file, "/tmp/ppp/%s_link", prefix);
	f_write_string(ppplink_file, argv[1], 0, 0);
	
	if ((p = getenv("IPREMOTE"))) {
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), p);
		TRACE_PT("IPREMOTE=%s\n", p);
	}

	if ((value = getenv("IPLOCAL"))) {
		_dprintf("IPLOCAL=%s\n", value);

		switch (get_wanx_proto(prefix)) {
		case WP_PPPOE:
		case WP_PPP3G:
			nvram_set(strcat_r(prefix, "_ipaddr_buf", tmp), nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)));		// store last ip address
			nvram_set(strcat_r(prefix, "_ipaddr", tmp), value);
			nvram_set(strcat_r(prefix, "_netmask", tmp), "255.255.255.255");
			break;
		case WP_PPTP:
		case WP_L2TP:
			nvram_set(strcat_r(prefix, "_ipaddr_buf", tmp), nvram_safe_get(strcat_r(prefix, "_ppp_get_ip", tmp)));
			break;
		}

		if (!nvram_match(strcat_r(prefix, "_ppp_get_ip", tmp), value)) {
			ifconfig(wan_ifname, IFUP, "0.0.0.0", NULL);
			nvram_set(strcat_r(prefix, "_ppp_get_ip", tmp), value);
		}

		_ifconfig(wan_ifname, IFUP, value, "255.255.255.255", (p && (*p)) ? p : NULL);
	}

	buf[0] = 0;
	if ((p = getenv("DNS1")) != NULL) strlcpy(buf, p, sizeof(buf));
	if ((p = getenv("DNS2")) != NULL) {
		if (buf[0]) strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));
	}
	nvram_set(strcat_r(prefix, "_get_dns", tmp), buf);
	TRACE_PT("DNS=%s\n", buf);

	if ((value = getenv("AC_NAME"))) nvram_set(strcat_r(prefix, "_ppp_get_ac", tmp), value);
	if ((value = getenv("SRV_NAME"))) nvram_set(strcat_r(prefix, "_ppp_get_srv", tmp), value);
	if ((value = getenv("MTU"))) nvram_set(strcat_r(prefix, "_run_mtu", tmp), value);

	start_wan_done(wan_ifname,prefix);

	TRACE_PT("end\n");
	return 0;
}

int ipdown_main(int argc, char **argv)
{
	int proto;
	char prefix[] = "wanXX";
	char tmp[100];
	char ppplink_file[256];
	
	TRACE_PT("begin\n");

	ppp_prefix(safe_getenv("DEVICE"), prefix);
	if (!wait_action_idle(10)) return -1;

	//stop_ddns();	// avoid to trigger DOD
	//stop_ntpc();

	memset(ppplink_file, 0, 256);
	sprintf(ppplink_file, "/tmp/ppp/%s_link", prefix);
	unlink(ppplink_file);

	proto = get_wanx_proto(prefix);
	mwan_table_del(prefix);

	if (proto == WP_L2TP || proto == WP_PPTP) {
		/* clear dns from the resolv.conf */
		nvram_set(strcat_r(prefix, "_get_dns", tmp),"");
		dns_to_resolv();

		if (proto == WP_L2TP) {
			route_del(nvram_safe_get(strcat_r(prefix, "_ifname", tmp)), 0, nvram_safe_get(strcat_r(prefix, "_l2tp_server_ip", tmp)),
				nvram_safe_get(strcat_r(prefix, "_gateway", tmp)), "255.255.255.255"); // fixed routing problem in Israel by kanki
		}

		// Restore the default gateway for WAN interface
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), nvram_safe_get(strcat_r(prefix, "_gateway", tmp)));

		// Set default route to gateway if specified
		route_del(nvram_safe_get(strcat_r(prefix, "_ifname", tmp)), 0, "0.0.0.0", nvram_safe_get(strcat_r(prefix, "_gateway", tmp)), "0.0.0.0");
		route_add(nvram_safe_get(strcat_r(prefix, "_ifname", tmp)), 0, "0.0.0.0", nvram_safe_get(strcat_r(prefix, "_gateway", tmp)), "0.0.0.0");
	}

	if (nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp))) {
		killall("listen", SIGKILL);
		eval("listen", nvram_safe_get("lan_ifname"),prefix);
	}

	mwan_load_balance();

	return 1;
}

#ifdef TCONFIG_IPV6
int ip6up_main(int argc, char **argv)
{
/*
	char *wan_ifname;
	char *value;

	TRACE_PT("begin\n");
	if (!wait_action_idle(10)) return -1;

	wan_ifname = safe_getenv("IFNAME");
	if ((!wan_ifname) || (!*wan_ifname)) return -1;

	value = getenv("LLREMOTE");

	// ???

	start_wan6_done(wan_ifname);
	TRACE_PT("end\n");
*/
	return 0;
}

int ip6down_main(int argc, char **argv)
{
/*
	TRACE_PT("begin\n");
	if (!wait_action_idle(10)) return -1;

	// ???

	TRACE_PT("end\n");
*/
	return 1;
}
#endif	// IPV6

int pppevent_main(int argc, char **argv)
{
	char prefix[] = "wanXX";
	char ppplog_file[256];
	
	TRACE_PT("begin\n");

	ppp_prefix(safe_getenv("DEVICE"), prefix);
	int i;
	for (i = 1; i < argc; ++i) {
		TRACE_PT("arg%d=%s\n", i, argv[i]);
		if (strcmp(argv[i], "-t") == 0) {
			if (++i >= argc) return 1;
			if ((strcmp(argv[i], "PAP_AUTH_FAIL") == 0) || (strcmp(argv[i], "CHAP_AUTH_FAIL") == 0)) {
				memset(ppplog_file, 0, 256);
				sprintf(ppplog_file, "/tmp/ppp/%s_log", prefix);
				f_write_string(ppplog_file, argv[i], 0, 0);
				notice_set(prefix, "Authentication failed");	// !!!
				return 0;
			}			
		}
	}

	TRACE_PT("end\n");
	return 1;
}

#if 0
int set_pppoepid_main(int argc, char **argv)
{
	if (argc < 2) return 0;
	char prefix[] = "wanXX";
	char tmp[100];

	ppp_prefix(safe_getenv("DEVICE"), prefix);
	TRACE_PT("num=%s\n", argv[1]);

	if (atoi(argv[1]) != 0) return 0;

	nvram_set(strcat_r(prefix, "_pppoe_pid0", tmp), getenv("PPPD_PID"));
	nvram_set(strcat_r(prefix, "_pppoe_ifname0", tmp), getenv("IFNAME"));
	nvram_set(strcat_r(prefix, "_iface", tmp), getenv("IFNAME"));
	
	TRACE_PT("IFNAME=%s DEVICE=%s\n", getenv("IFNAME"), getenv("DEVICE"));
	return 0;
}
	
int pppoe_down_main(int argc, char **argv)
{
	if (argc < 2) return 0;

	char prefix[] = "wanXX";
	char tmp[100];
	ppp_prefix(safe_getenv("DEVICE"), prefix);
	TRACE_PT("num=%s\n", argv[1]);

	if (atoi(argv[1]) != 0) return 0;

	if ((nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp))) && (nvram_match("action_service", "")))	{
		stop_singe_pppoe(0,prefix);
		start_pppoe(0,prefix);
		
		stop_dnsmasq();
		dns_to_resolv();
		start_dnsmasq();
	}
	return 0;
}
#endif	// 0
