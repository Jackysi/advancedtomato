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

static int env2nv_gateway(const char *nv)
{
	char *v, *g;
	char *b;
	int r;

	r = 0;
	if ((v = getenv("router")) != NULL) {
		if ((b = strdup(v)) != NULL) {
			if ((v = strchr(b, ' ')) != NULL) *v = 0;	// truncate multiple entries
			if (!nvram_match((char *)nv, b)) {
				nvram_set(nv, b);
				r = 1;
			}
			free(b);
		}
	}
	else if ((v = getenv("staticroutes")) != NULL) {
		if ((b = strdup(v)) == NULL) return 0;
		v = b;
		while ((g = strsep(&v, " ")) != NULL) {
			if (strcmp(g, "0.0.0.0/0") == 0) {
				if ((g = strsep(&v, " ")) && *g) {
					if (!nvram_match((char *)nv, g)) {
						nvram_set(nv, g);
						r = 1;
					}
					break;
				}
			}
		}
		free(b);
	}

	return r;
}

static const char renewing[] = "/var/lib/misc/dhcpc.renewing";

static int deconfig(char *ifname)
{
	TRACE_PT("begin\n");

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);

	if (using_dhcpc()) {
		nvram_set("wan_ipaddr", "0.0.0.0");
		nvram_set("wan_gateway", "0.0.0.0");
	}
	nvram_set("wan_lease", "0");
	nvram_set("wan_routes1", "");
	nvram_set("wan_routes2", "");
	expires(0);

	if (get_wan_proto() == WP_DHCP) {
		nvram_set("wan_netmask", "0.0.0.0");
		nvram_set("wan_gateway_get", "0.0.0.0");
		nvram_set("wan_get_dns", "");
	}

	//	route_del(ifname, 0, NULL, NULL, NULL);

#ifdef TCONFIG_IPV6
	nvram_set("wan_6rd", "");
#endif

	TRACE_PT("end\n");
	return 0;
}

static int bound(char *ifname, int renew);

static int renew(char *ifname)
{
	char *a;
	int changed = 0, routes_changed = 0;
	int wan_proto = get_wan_proto();

	TRACE_PT("begin\n");

	unlink(renewing);

	if (env2nv("ip", "wan_ipaddr") ||
	    env2nv_gateway("wan_gateway") ||
	    (wan_proto == WP_DHCP && env2nv("subnet", "wan_netmask"))) {
		/* WAN IP or gateway changed, restart/reconfigure everything */
		TRACE_PT("end\n");
		return bound(ifname, 1);
	}

	if (wan_proto == WP_DHCP) {
		changed |= env2nv("domain", "wan_get_domain");
		changed |= env2nv("dns", "wan_get_dns");
	}

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
	TRACE_PT("wan_gateway=%s\n", nvram_safe_get("wan_gateway"));
	TRACE_PT("wan_get_domain=%s\n", nvram_safe_get("wan_get_domain"));
	TRACE_PT("wan_get_dns=%s\n", nvram_safe_get("wan_get_dns"));
	TRACE_PT("wan_lease=%s\n", nvram_safe_get("wan_lease"));
	TRACE_PT("wan_routes1=%s\n", nvram_safe_get("wan_routes1"));
	TRACE_PT("wan_routes2=%s\n", nvram_safe_get("wan_routes2"));
	TRACE_PT("end\n");
	return 0;
}

static int bound(char *ifname, int renew)
{
	TRACE_PT("begin\n");

	unlink(renewing);

	char *netmask, *dns;
	int wan_proto = get_wan_proto();

	dns = nvram_safe_get("wan_get_dns");
	nvram_set("wan_routes1", "");
	nvram_set("wan_routes2", "");
	env2nv("ip", "wan_ipaddr");
	env2nv_gateway("wan_gateway");
	env2nv("dns", "wan_get_dns");
	env2nv("domain", "wan_get_domain");
	env2nv("lease", "wan_lease");
	netmask = getenv("subnet") ? : "255.255.255.255";
	if (wan_proto == WP_DHCP) {
		nvram_set("wan_netmask", netmask);
		nvram_set("wan_gateway_get", nvram_safe_get("wan_gateway"));
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

#ifdef TCONFIG_IPV6
	env2nv("6rd", "wan_6rd");
#endif

	TRACE_PT("wan_ipaddr=%s\n", nvram_safe_get("wan_ipaddr"));
	TRACE_PT("wan_netmask=%s\n", netmask);
	TRACE_PT("wan_gateway=%s\n", nvram_safe_get("wan_gateway"));
	TRACE_PT("wan_get_domain=%s\n", nvram_safe_get("wan_get_domain"));
	TRACE_PT("wan_get_dns=%s\n", nvram_safe_get("wan_get_dns"));
	TRACE_PT("wan_lease=%s\n", nvram_safe_get("wan_lease"));
	TRACE_PT("wan_routes1=%s\n", nvram_safe_get("wan_routes1"));
	TRACE_PT("wan_routes2=%s\n", nvram_safe_get("wan_routes2"));
#ifdef TCONFIG_IPV6
	TRACE_PT("wan_6rd=%s\n", nvram_safe_get("wan_6rd"));
#endif

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);
	ifconfig(ifname, IFUP, nvram_safe_get("wan_ipaddr"), netmask);

	if (wan_proto != WP_DHCP) {
		char *gw = nvram_safe_get("wan_gateway");

		preset_wan(ifname, gw, netmask);

		/* clear dns from the resolv.conf */
		nvram_set("wan_get_dns", renew ? dns : "");

		switch (wan_proto) {
		case WP_PPTP:
			start_pptp(BOOT);
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
		if (strcmp(argv[1], "bound") == 0) return bound(ifname, 0);
		if ((strcmp(argv[1], "renew") == 0) || (strcmp(argv[1], "update") == 0)) return renew(ifname);
	}

	return 1;
}


// -----------------------------------------------------------------------------


int dhcpc_release_main(int argc, char **argv)
{
	TRACE_PT("begin\n");

	if (!using_dhcpc()) return 1;

	if (killall("udhcpc", SIGUSR2) == 0) {
		sleep(2);
	}

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
		"udhcpc -i %s -b -s dhcpc-event %s %s %s %s %s %s",
		ifname,
		nvram_invmatch("wan_hostname", "") ? "-H" : "", nvram_safe_get("wan_hostname"),
		nvram_get_int("dhcpc_minpkt") ? "-m" : "",
		nvram_contains_word("log_events", "dhcpc") ? "-S" : "",
		nvram_safe_get("dhcpc_custom"),
#ifdef TCONFIG_IPV6
		(get_ipv6_service() == IPV6_6RD_DHCP) ? "-O 6rd" : ""
#else
		""
#endif
		);

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

// -----------------------------------------------------------------------------

#ifdef TCONFIG_IPV6

int dhcp6c_state_main(int argc, char **argv)
{
	char prefix[INET6_ADDRSTRLEN];
	struct in6_addr addr;
	int i, r;

	TRACE_PT("begin\n");

	if (!wait_action_idle(10)) return 1;

	nvram_set("ipv6_rtr_addr", getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0));

	// extract prefix from configured IPv6 address
	if (inet_pton(AF_INET6, nvram_safe_get("ipv6_rtr_addr"), &addr) > 0) {
		r = nvram_get_int("ipv6_prefix_length") ? : 64;
		for (r = 128 - r, i = 15; r > 0; r -= 8) {
			if (r >= 8)
				addr.s6_addr[i--] = 0;
			else
				addr.s6_addr[i--] &= (0xff << r);
		}
		inet_ntop(AF_INET6, &addr, prefix, sizeof(prefix));
		nvram_set("ipv6_prefix", prefix);
	}

	if (env2nv("new_domain_name_servers", "ipv6_get_dns")) {
		dns_to_resolv();
//		start_dnsmasq();	// (re)start KDB don't do twice!
	}

	// (re)start dnsmasq and httpd
	start_dnsmasq();
	start_httpd();

	TRACE_PT("ipv6_get_dns=%s\n", nvram_safe_get("ipv6_get_dns"));
	TRACE_PT("end\n");
	return 0;
}

void start_dhcp6c(void)
{
	FILE *f;
	int prefix_len;
	char *wan6face;
	char *argv[] = { "dhcp6c", "-T", "LL", NULL, NULL, NULL };
	int argc;

	TRACE_PT("begin\n");

	// Check if turned on
	if (get_ipv6_service() != IPV6_NATIVE_DHCP) return;

	prefix_len = 64 - (nvram_get_int("ipv6_prefix_length") ? : 64);
	if (prefix_len < 0)
		prefix_len = 0;
	wan6face = nvram_safe_get("wan_iface");

	nvram_set("ipv6_get_dns", "");
	nvram_set("ipv6_rtr_addr", "");
	nvram_set("ipv6_prefix", "");

	// Create dhcp6c.conf
	if ((f = fopen("/etc/dhcp6c.conf", "w"))) {
		fprintf(f,
			"interface %s {\n"
			" send ia-pd 0;\n"
			" send rapid-commit;\n"
			" request domain-name-servers;\n"
			" script \"/sbin/dhcp6c-state\";\n"
			"};\n"
			"id-assoc pd 0 {\n"
			" prefix-interface %s {\n"
			"  sla-id 0;\n"
			"  sla-len %d;\n"
			" };\n"
			"};\n"
			"id-assoc na 0 { };\n",
			wan6face,
			nvram_safe_get("lan_ifname"),
			prefix_len);
		fclose(f);
	}

	argc = 3;
	if (nvram_get_int("debug_ipv6"))
		argv[argc++] = "-D";
	argv[argc++] = wan6face;
	argv[argc] = NULL;
	_eval(argv, NULL, 0, NULL);

	TRACE_PT("end\n");
}

void stop_dhcp6c(void)
{
	TRACE_PT("begin\n");

	killall("dhcp6c-event", SIGTERM);
	killall_tk("dhcp6c");

	TRACE_PT("end\n");
}

#endif	// TCONFIG_IPV6
