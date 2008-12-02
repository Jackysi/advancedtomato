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


#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)



int ipup_main(int argc, char **argv)
{
	char *wan_ifname;
	char *value;
	char buf[256];
	const char *p;

	_dprintf("%s: begin\n", __FUNCTION__);

	killall("listen", SIGKILL);
	
	if (!wait_action_idle(10)) return -1;
	

	wan_ifname = safe_getenv("IFNAME");
	if ((!wan_ifname) || (!*wan_ifname)) return -1;
	nvram_set("wan_iface", wan_ifname);	// ppp#

	// ipup receives six arguments:
	//   <interface name>  <tty device>  <speed> <local IP address> <remote IP address> <ipparam>
	//   ppp1 vlan1 0 71.135.98.32 151.164.184.87 0

	f_write_string("/tmp/ppp/link", argv[1], 0, 0);
	
	if ((value = getenv("IPLOCAL"))) {
		_dprintf("IPLOCAL=%s\n", getenv("IPLOCAL"));

		ifconfig(wan_ifname, IFUP, value, "255.255.255.255");

		switch (get_wan_proto()) {
		case WP_PPPOE:
			nvram_set("wan_ipaddr_buf", nvram_safe_get("wan_ipaddr"));		// store last ip address
			nvram_set("wan_ipaddr", value);
			nvram_set("wan_netmask", "255.255.255.255");
			break;
		case WP_PPTP:
			nvram_set("wan_ipaddr_buf", nvram_safe_get("pptp_get_ip"));
			nvram_set("pptp_get_ip", value);
			break;
		case WP_L2TP:
			nvram_set("wan_ipaddr_buf", nvram_safe_get("l2tp_get_ip"));
			nvram_set("l2tp_get_ip", value);
			break;
		}
	}

	if ((value = getenv("IPREMOTE"))) {
		nvram_set("wan_gateway", value);
		_dprintf("IPREMOTE=%s\n", getenv("IPREMOTE"));
	}

	buf[0] = 0;
	if ((p = getenv("DNS1")) != NULL) strlcpy(buf, p, sizeof(buf));
	if ((p = getenv("DNS2")) != NULL) {
		if (buf[0]) strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));
	}
	nvram_set("wan_get_dns", buf);

	if ((value = getenv("AC_NAME"))) nvram_set("ppp_get_ac", value);
	if ((value = getenv("SRV_NAME"))) nvram_set("ppp_get_srv", value);
	if ((value = getenv("MTU"))) nvram_set("wan_run_mtu", value);
	
	start_wan_done(wan_ifname);

	_dprintf("%s: end\n", __FUNCTION__);
	return 0;
}

int ipdown_main(int argc, char **argv)
{
	int proto;
	
	_dprintf("%s: begin\n", __FUNCTION__);

	if (!wait_action_idle(10)) return -1;

	stop_ddns();	// avoid to trigger DOD
	stop_ntpc();

	unlink("/tmp/ppp/link");

	proto = get_wan_proto();
	if (proto == WP_L2TP) {
		/* clear dns from the resolv.conf */
		nvram_set("wan_get_dns","");
		dns_to_resolv();

		route_del(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"),
			nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); // fixed routing problem in Israel by kanki

		// Restore the default gateway for WAN interface
		nvram_set("wan_gateway", nvram_safe_get("wan_gateway_buf"));

		// Set default route to gateway if specified
		route_add(nvram_safe_get("wan_ifname"), 0, "0.0.0.0", nvram_safe_get("wan_gateway"), "0.0.0.0");
	}

	if ((nvram_match("ppp_demand", "1")) && ((proto == WP_PPTP) || (proto == WP_L2TP))) {
		killall("listen", SIGKILL);
		eval("listen", nvram_safe_get("lan_ifname"));
	}

	_dprintf("%s: end\n", __FUNCTION__);
	return 1;
}

int pppevent_main(int argc, char **argv)
{
	int i;
	
	_dprintf("%s: begin\n", __FUNCTION__);

	for (i = 1; i < argc; ++i) {
		_dprintf("%s: arg%d=%s\n", __FUNCTION__, i, argv[i]);
		if (strcmp(argv[i], "-t") == 0) {
			if (++i >= argc) return 1;
			if ((strcmp(argv[i], "PAP_AUTH_FAIL") == 0) || (strcmp(argv[i], "CHAP_AUTH_FAIL") == 0)) {
				f_write_string("/tmp/ppp/log", argv[i], 0, 0);
				notice_set("wan", "Authentication failed");	// !!!
				return 0;
			}			
		}
	}

	_dprintf("%s: end\n", __FUNCTION__);
	return 1;
}

int set_pppoepid_main(int argc, char **argv)
{
	if (argc < 2) return 0;

	_dprintf("%s: num=%s\n", __FUNCTION__, argv[1]);

	if (atoi(argv[1]) != 0) return 0;

	nvram_set("pppoe_pid0", getenv("PPPD_PID"));
	nvram_set("pppoe_ifname0", getenv("IFNAME"));
	nvram_set("wan_iface", getenv("IFNAME"));
	
	_dprintf("IFNAME=%s DEVICE=%s\n", getenv("IFNAME"), getenv("DEVICE"));
	return 0;
}
	
int pppoe_down_main(int argc, char **argv)
{
	if (argc < 2) return 0;

	_dprintf("%s: num=%s\n", __FUNCTION__, argv[1]);

	if (atoi(argv[1]) != 0) return 0;

	if ((nvram_match("ppp_demand", "1")) && (nvram_match("action_service", "")))	{
		stop_singe_pppoe(0);
		start_pppoe(0);
		
		stop_dnsmasq();
		dns_to_resolv();
		start_dnsmasq();
	}
	return 0;
}
