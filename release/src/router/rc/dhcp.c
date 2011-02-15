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

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

#include <sys/sysinfo.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

static void expires(unsigned int seconds)
{
	struct sysinfo info;
	char s[32];

   	sysinfo(&info);
	sprintf(s, "%u", (unsigned int)info.uptime + seconds);
	f_write_string("/var/lib/misc/dhcpc.expires", s, 0, 0);
}

// copy env to nvram
// returns 1 if new/changed, 0 if not changed/no env
static int env2nv(char *env, char *nv)
{
	char *value;
	if ((value = getenv(env)) != NULL) {
		if (!nvram_match(nv, value)) {
			nvram_set(nv, value);
			return 1;
		}
	}
	return 0;
}

static void env2nv_gateway(const char *nv)
{
	char *v, *g;
	char *b;

	if ((v = getenv("router")) != NULL) {
		if ((b = strdup(v)) != NULL) {
			if ((v = strchr(b, ' ')) != NULL) *v = 0;	// truncate multiple entries
			nvram_set(nv, b);
			free(b);
		}
	}
	else if ((v = getenv("staticroutes")) != NULL) {
		if ((b = strdup(v)) == NULL) return;
		v = b;
		while ((g = strsep(&v, " ")) != NULL) {
			if (strcmp(g, "0.0.0.0/0") == 0) {
				if ((g = strsep(&v, " ")) && *g) {
					nvram_set(nv, g);
					break;
				}
			}
		}
		free(b);
	}
}

static const char renewing[] = "/var/lib/misc/dhcpc.renewing";

static int deconfig(char *ifname)
{
	TRACE_PT("begin\n");

	int wan_proto;

	wan_proto = get_wan_proto();
	ifconfig(ifname, IFUP, "0.0.0.0", NULL);

	if (using_dhcpc()) {
		nvram_set("wan_ipaddr", "0.0.0.0");
		nvram_set("wan_gateway", "0.0.0.0");
	}
	nvram_set("wan_lease", "0");
	nvram_set("wan_routes1", "");
	nvram_set("wan_routes2", "");
	expires(0);

	if (wan_proto == WP_DHCP) {
		nvram_set("wan_netmask", "0.0.0.0");
		nvram_set("wan_gateway_get", "0.0.0.0");
		nvram_set("wan_get_dns", "");
	}

	//	int i = 10;
	//	while ((route_del(ifname, 0, NULL, NULL, NULL) == 0) && (i-- > 0)) { }

	TRACE_PT("end\n");
	return 0;
}

static int bound(char *ifname);

static int renew(char *ifname)
{
	char *a, *b, *gw;
	int changed = 0, routes_changed = 0, metric;
	int wan_proto = get_wan_proto();

	TRACE_PT("begin\n");

	unlink(renewing);

	changed = env2nv("ip", "wan_ipaddr");
	if (changed) {
		/* DHCP WAN IP changed, restart/reconfigure everything */
		TRACE_PT("end\n");
		return bound(ifname);
	}

	if (wan_proto != WP_DHCP) {
		gw = "wan_gateway";
		metric = nvram_get_int("ppp_defgw") ? 2 : 0;
	}
	else {
		gw = "wan_gateway_get";
		metric = 0;

		changed |= env2nv("subnet", "wan_netmask");
		changed |= env2nv("domain", "wan_get_domain");
		changed |= env2nv("dns", "wan_get_dns");
	}
	a = strdup(nvram_safe_get(gw));
	env2nv_gateway(gw);
	b = nvram_safe_get(gw);
	if ((a) && (strcmp(a, b) != 0)) {
		route_del(ifname, metric, "0.0.0.0", a, "0.0.0.0");
		route_add(ifname, metric, "0.0.0.0", b, "0.0.0.0");
		changed = 1;
	}
	free(a);

	nvram_set("wan_routes1_save", nvram_safe_get("wan_routes1"));
	nvram_set("wan_routes2_save", nvram_safe_get("wan_routes2"));

	/* Classless Static Routes (option 121) or MS Classless Static Routes (option 249) */
	if (getenv("staticroutes"))
		routes_changed |= env2nv("staticroutes", "wan_routes1_save");
	else
		routes_changed |= env2nv("msstaticroutes", "wan_routes1_save");
	/* Static Routes (option 33) */
	routes_changed |= env2nv("routes", "wan_routes2_save");

	changed |= routes_changed;

	if ((a = getenv("lease")) != NULL) {
		nvram_set("wan_lease", a);
		expires(atoi(a));
	}

	if (changed) {
		set_host_domain_name();
		start_dnsmasq();	// (re)start
	}

	if (routes_changed) {
		do_wan_routes(ifname, 0, 0);
		nvram_set("wan_routes1", nvram_safe_get("wan_routes1_save"));
		nvram_set("wan_routes2", nvram_safe_get("wan_routes2_save"));
		do_wan_routes(ifname, 0, 1);
	}
	nvram_unset("wan_routes1_save");
	nvram_unset("wan_routes2_save");

	TRACE_PT("wan_ipaddr=%s\n", nvram_safe_get("wan_ipaddr"));
	TRACE_PT("wan_netmask=%s\n", nvram_safe_get("wan_netmask"));
	TRACE_PT("%s=%s\n", gw, nvram_safe_get(gw));
	TRACE_PT("wan_get_domain=%s\n", nvram_safe_get("wan_get_domain"));
	TRACE_PT("wan_get_dns=%s\n", nvram_safe_get("wan_get_dns"));
	TRACE_PT("wan_lease=%s\n", nvram_safe_get("wan_lease"));
	TRACE_PT("wan_routes1=%s\n", nvram_safe_get("wan_routes1"));
	TRACE_PT("wan_routes2=%s\n", nvram_safe_get("wan_routes2"));
	TRACE_PT("end\n");
	return 0;
}

static int bound(char *ifname)
{
	TRACE_PT("begin\n");

	unlink(renewing);

	char *netmask;
	int wan_proto = get_wan_proto();

	nvram_set("wan_routes1", "");
	nvram_set("wan_routes2", "");
	env2nv("ip", "wan_ipaddr");
	env2nv_gateway("wan_gateway_get");
	env2nv("dns", "wan_get_dns");
	env2nv("domain", "wan_get_domain");
	env2nv("lease", "wan_lease");
	netmask = getenv("subnet") ? : "255.255.255.255";
	if (wan_proto == WP_DHCP) {
		nvram_set("wan_netmask", netmask);
		nvram_set("wan_gateway", nvram_safe_get("wan_gateway_get"));
	}

	/* RFC3442: If the DHCP server returns both a Classless Static Routes option
	 * and a Router option, the DHCP client MUST ignore the Router option.
	 * Similarly, if the DHCP server returns both a Classless Static Routes
	 * option and a Static Routes option, the DHCP client MUST ignore the
	 * Static Routes option.
	 * Ref: http://www.faqs.org/rfcs/rfc3442.html
	 */
	/* Classless Static Routes (option 121) */
	if (!env2nv("staticroutes", "wan_routes1"))
		/* or MS Classless Static Routes (option 249) */
		env2nv("msstaticroutes", "wan_routes1");
	/* Static Routes (option 33) */
	env2nv("routes", "wan_routes2");

	expires(atoi(safe_getenv("lease")));

	TRACE_PT("wan_ipaddr=%s\n", nvram_safe_get("wan_ipaddr"));
	TRACE_PT("wan_netmask=%s\n", netmask);
	TRACE_PT("wan_gateway_get=%s\n", nvram_safe_get("wan_gateway_get"));
	TRACE_PT("wan_get_domain=%s\n", nvram_safe_get("wan_get_domain"));
	TRACE_PT("wan_get_dns=%s\n", nvram_safe_get("wan_get_dns"));
	TRACE_PT("wan_lease=%s\n", nvram_safe_get("wan_lease"));
	TRACE_PT("wan_routes1=%s\n", nvram_safe_get("wan_routes1"));
	TRACE_PT("wan_routes2=%s\n", nvram_safe_get("wan_routes2"));

	ifconfig(ifname, IFUP, nvram_safe_get("wan_ipaddr"), netmask);

	if (wan_proto != WP_DHCP) {
		char *gw = nvram_safe_get("wan_gateway_get");

		preset_wan(ifname, gw, netmask);

		/* Backup the default gateway. It should be used if PPP connection is broken */
		nvram_set("wan_gateway", gw);

		/* clear dns from the resolv.conf */
		nvram_set("wan_get_dns", "");

		switch (wan_proto) {
		case WP_PPTP:
			start_pptp(BOOT);
			// we don't need dhcp anymore ?
			// xstart("service", "dhcpc", "stop");
			break;
		case WP_L2TP:
			start_l2tp();
			break;
		}
	}
	else {
		start_wan_done(ifname);
	}

	TRACE_PT("end\n");
	return 0;
}

int dhcpc_event_main(int argc, char **argv)
{
	char *ifname;

	if (!wait_action_idle(10)) return 1;

	if ((argc == 2) && (ifname = getenv("interface")) != NULL) {
		TRACE_PT("event=%s\n", argv[1]);

		if (strcmp(argv[1], "deconfig") == 0) return deconfig(ifname);
		if (strcmp(argv[1], "bound") == 0) return bound(ifname);
		if ((strcmp(argv[1], "renew") == 0) || (strcmp(argv[1], "update") == 0)) return renew(ifname);
	}

	return 1;
}


// -----------------------------------------------------------------------------


int dhcpc_release_main(int argc, char **argv)
{
	TRACE_PT("begin\n");

	if (!using_dhcpc()) return 1;

	deconfig(nvram_safe_get("wan_ifname"));
	killall("udhcpc", SIGUSR2);
	unlink(renewing);
	unlink("/var/lib/misc/wan.connecting");

	TRACE_PT("end\n");
	return 0;
}

int dhcpc_renew_main(int argc, char **argv)
{
	int pid;

	TRACE_PT("begin\n");

	if (!using_dhcpc()) return 1;

	if ((pid = pidof("udhcpc")) > 1) {
		kill(pid, SIGUSR1);
		f_write(renewing, NULL, 0, 0, 0);
	}
	else {
		stop_dhcpc();
		start_dhcpc();
	}

	TRACE_PT("end\n");
	return 0;
}


// -----------------------------------------------------------------------------


void start_dhcpc(void)
{
	char cmd[256];
	char *ifname;
	char *p;
	int proto;

	TRACE_PT("begin\n");

	nvram_set("wan_get_dns", "");
	f_write(renewing, NULL, 0, 0, 0);

	ifname = nvram_safe_get("wan_ifname");
	proto = get_wan_proto();
	if (proto == WP_DHCP) {
		nvram_set("wan_iface", ifname);
	}

#if 1	// REMOVEME after 1/1/2012
	// temporary code for compatibility with old nvram variables
	int changed = 0;
	strcpy(cmd, nvram_safe_get("dhcpc_custom"));
	if (strstr(cmd, "-V ") == NULL) {
		if ((p = nvram_get("dhcpc_vendorclass")) && (*p)) {
			changed++;
			strcat(cmd, " -V ");
			strcat(cmd, p);
		}
	}
	if (strstr(cmd, "-r ") == NULL) {
		if ((p = nvram_get("dhcpc_requestip")) && (*p) && (strcmp(p, "0.0.0.0") != 0)) {
			changed++;
			strcat(cmd, " -r ");
			strcat(cmd, p);
		}
	}
	if (changed) {
		nvram_set("dhcpc_custom", cmd);
	}
#endif

	snprintf(cmd, sizeof(cmd),
		"udhcpc -i %s -b -s dhcpc-event %s %s %s %s %s",
		ifname,
		nvram_invmatch("wan_hostname", "") ? "-H" : "", nvram_safe_get("wan_hostname"),
		nvram_get_int("dhcpc_minpkt") ? "-m" : "",
		nvram_contains_word("log_events", "dhcpc") ? "-S" : "",
		nvram_safe_get("dhcpc_custom"));

	xstart("/bin/sh", "-c", cmd);

	TRACE_PT("end\n");
}

void stop_dhcpc(void)
{
	TRACE_PT("begin\n");

	killall("dhcpc-event", SIGTERM);
	if (killall("udhcpc", SIGUSR2) == 0) {	// release
		sleep(2);
	}
	killall_tk("udhcpc");
	unlink(renewing);

	TRACE_PT("end\n");
}

