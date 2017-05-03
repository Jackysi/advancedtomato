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

#define mwanlog(level,x...) if(nvram_get_int("mwan_debug")>=level) syslog(level, x)

static void expires(unsigned int seconds, char *prefix)
{
	struct sysinfo info;
	char s[32];
	char expires_file[256];

   	sysinfo(&info);
	sprintf(s, "%u", (unsigned int)info.uptime + seconds);
	
	memset(expires_file, 0, 256);
	sprintf(expires_file, "/var/lib/misc/dhcpc-%s.expires", prefix);
	f_write_string(expires_file, s, 0, 0);
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

//static const char renewing[] = "/var/lib/misc/dhcpc.renewing";

static int deconfig(char *ifname,char *prefix)
{
	char tmp[100];
	TRACE_PT("begin\n");

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);

	if (using_dhcpc(prefix)) {
		nvram_set(strcat_r(prefix, "_ipaddr", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_gateway", tmp), "0.0.0.0");
	}
	nvram_set(strcat_r(prefix, "_lease", tmp), "0");
	nvram_set(strcat_r(prefix, "_routes1", tmp), "");
	nvram_set(strcat_r(prefix, "_routes2", tmp), "");
	expires(0, prefix);

	if (get_wan_proto() == WP_DHCP || get_wan_proto() == WP_LTE) {
		nvram_set(strcat_r(prefix, "_netmask", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), "0.0.0.0");
		nvram_set(strcat_r(prefix, "_get_dns", tmp), "");
	}

	//	route_del(ifname, 0, NULL, NULL, NULL);

#ifdef TCONFIG_IPV6
	nvram_set("wan_6rd", "");
#endif

	TRACE_PT("end\n");
	return 0;
}

static int bound(char *ifname, int renew, char *prefix)
{
	char tmp [100];

	TRACE_PT("begin\n");

	char renew_file[256];
	memset(renew_file, 0, 256);
	sprintf(renew_file, "/var/lib/misc/%s_dhcpc.renewing", prefix);
	unlink(renew_file);

	char *netmask, *dns;
	int wan_proto = get_wanx_proto(prefix);

	mwanlog(LOG_INFO, "dhcpc_bound, interface=%s, wan_prefix=%s, renew=%d, proto=%d", ifname, prefix, renew, wan_proto);

	dns = nvram_safe_get(strcat_r(prefix, "_get_dns", tmp));
	nvram_set(strcat_r(prefix, "_routes1", tmp), "");
	nvram_set(strcat_r(prefix, "_routes2", tmp), "");
	env2nv("ip", strcat_r(prefix, "_ipaddr", tmp));
	env2nv_gateway(strcat_r(prefix, "_gateway", tmp));
	env2nv("dns", strcat_r(prefix, "_get_dns", tmp));
	env2nv("domain", strcat_r(prefix, "_get_domain", tmp));
	env2nv("lease", strcat_r(prefix, "_lease", tmp));
	netmask = getenv("subnet") ? : "255.255.255.255";
	if (wan_proto == WP_DHCP || wan_proto == WP_LTE) {
		nvram_set(strcat_r(prefix, "_netmask", tmp), netmask);
		nvram_set(strcat_r(prefix, "_gateway_get", tmp), nvram_safe_get(strcat_r(prefix, "_gateway", tmp)));
	}

	/* RFC3442: If the DHCP server returns both a Classless Static Routes option
	 * and a Router option, the DHCP client MUST ignore the Router option.
	 * Similarly, if the DHCP server returns both a Classless Static Routes
	 * option and a Static Routes option, the DHCP client MUST ignore the
	 * Static Routes option.
	 * Ref: http://www.faqs.org/rfcs/rfc3442.html
	 */
	/* Classless Static Routes (option 121) */
	if (!env2nv("staticroutes", strcat_r(prefix, "_routes1", tmp)))
		/* or MS Classless Static Routes (option 249) */
		env2nv("msstaticroutes", strcat_r(prefix, "_routes1", tmp));
	/* Static Routes (option 33) */
	env2nv("routes", strcat_r(prefix, "_routes2", tmp));

	expires(atoi(safe_getenv("lease")), prefix);

#ifdef TCONFIG_IPV6
	env2nv("ip6rd", "wan_6rd");
#endif

	TRACE_PT(strcat_r(prefix, "_ipaddr=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)));
	TRACE_PT(strcat_r(prefix, "_netmask=%s\n", tmp), netmask);
	TRACE_PT(strcat_r(prefix, "_gateway=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_gateway", tmp)));
	TRACE_PT(strcat_r(prefix, "_get_domain=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_get_domain", tmp)));
	TRACE_PT(strcat_r(prefix, "_get_dns=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_get_dns", tmp)));
	TRACE_PT(strcat_r(prefix, "_lease=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_lease", tmp)));
	TRACE_PT(strcat_r(prefix, "_routes1=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_routes1", tmp)));
	TRACE_PT(strcat_r(prefix, "_routes2=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_routes2", tmp)));
#ifdef TCONFIG_IPV6
	TRACE_PT("wan_6rd=%s\n", nvram_safe_get("wan_6rd"));
#endif

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);
	ifconfig(ifname, IFUP, nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)), netmask);

	if (wan_proto != WP_DHCP && wan_proto != WP_LTE) {
		char *gw = nvram_safe_get(strcat_r(prefix, "_gateway", tmp));

		preset_wan(ifname, gw, netmask, prefix);

		/* clear dns from the resolv.conf */
		nvram_set(strcat_r(prefix, "_get_dns", tmp), renew ? dns : "");

		switch (wan_proto) {
		case WP_PPTP:
			start_pptp(BOOT,prefix);
			break;
		case WP_L2TP:
			start_l2tp(prefix);
			break;
		}
	}
	else {
		start_wan_done(ifname,prefix);
	}

	TRACE_PT("end\n");
	return 0;
}

static int renew(char *ifname, char *prefix)
{
	char *a;
	int changed = 0, routes_changed = 0;
	int wan_proto = get_wanx_proto(prefix);
	char tmp[100];

	TRACE_PT("begin\n");

	mwanlog(LOG_INFO, "dhcpc_renew, interface=%s, wan_prefix=%s", ifname, prefix);

	char renew_file[256];
	memset(renew_file, 0, 256);
	sprintf(renew_file, "/var/lib/misc/%s_dhcpc.renewing", prefix);
	unlink(renew_file);

	if (env2nv("ip", strcat_r(prefix, "_ipaddr", tmp)) ||
	    env2nv_gateway(strcat_r(prefix, "_gateway", tmp)) ||
	    (wan_proto == WP_LTE && env2nv("subnet", strcat_r(prefix, "_netmask", tmp))) ||
	    (wan_proto == WP_DHCP && env2nv("subnet", strcat_r(prefix, "_netmask", tmp)))) {
		/* WAN IP or gateway changed, restart/reconfigure everything */
		TRACE_PT("end\n");
		return bound(ifname, 1, prefix);
	}

	if (wan_proto == WP_DHCP || wan_proto == WP_LTE) {
		changed |= env2nv("domain", strcat_r(prefix, "_get_domain", tmp));
		changed |= env2nv("dns", strcat_r(prefix, "_get_dns", tmp));
	}

	nvram_set(strcat_r(prefix, "_routes1_save", tmp), nvram_safe_get(strcat_r(prefix, "_routes1", tmp)));
	nvram_set(strcat_r(prefix, "_routes2_save", tmp), nvram_safe_get(strcat_r(prefix, "_routes2", tmp)));

	/* Classless Static Routes (option 121) or MS Classless Static Routes (option 249) */
	if (getenv("staticroutes"))
		routes_changed |= env2nv("staticroutes", strcat_r(prefix, "_routes1_save", tmp));
	else
		routes_changed |= env2nv("msstaticroutes", strcat_r(prefix, "_routes1_save", tmp));
	/* Static Routes (option 33) */
	routes_changed |= env2nv("routes", strcat_r(prefix, "_routes2_save", tmp));

	changed |= routes_changed;

	if ((a = getenv("lease")) != NULL) {
		nvram_set(strcat_r(prefix, "_lease", tmp), a);
		expires(atoi(a), prefix);
	}

	if (changed) {
		set_host_domain_name();
		start_dnsmasq();	// (re)start
	}

	if (routes_changed) {
		do_wan_routes(ifname, 0, 0, prefix);
		nvram_set(strcat_r(prefix, "_routes1", tmp), nvram_safe_get(strcat_r(prefix, "_routes1_save", tmp)));
		nvram_set(strcat_r(prefix, "_routes2", tmp), nvram_safe_get(strcat_r(prefix, "_routes2_save", tmp)));
		do_wan_routes(ifname, 0, 1, prefix);
	}
	nvram_unset(strcat_r(prefix, "_routes1_save", tmp));
	nvram_unset(strcat_r(prefix, "_routes2_save", tmp));

	TRACE_PT(strcat_r(prefix, "_ipaddr=%s\n", tmp), nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)));
	TRACE_PT(strcat_r(prefix, "_netmask=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_netmask", tmp)));
	TRACE_PT(strcat_r(prefix, "_gateway=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_gateway", tmp)));
	TRACE_PT(strcat_r(prefix, "_get_domain=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_get_domain", tmp)));
	TRACE_PT(strcat_r(prefix, "_get_dns=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_get_dns", tmp)));
	TRACE_PT(strcat_r(prefix, "_lease=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_lease", tmp)));
	TRACE_PT(strcat_r(prefix, "_routes1=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_routes1", tmp)));
	TRACE_PT(strcat_r(prefix, "_routes2=%s\n" tmp), nvram_safe_get(strcat_r(prefix, "_routes2", tmp)));
	TRACE_PT("end\n");
	return 0;
}


int dhcpc_event_main(int argc, char **argv)
{
	char *ifname;
	ifname = getenv("interface");
	char prefix[] = "wanXXXXXXXXXX_";

	if (nvram_match( "wan2_ifname", ifname )) strcpy(prefix, "wan2");
#ifdef TCONFIG_MULTIWAN
	else if (nvram_match( "wan3_ifname", ifname )) strcpy(prefix, "wan3");
	else if (nvram_match( "wan4_ifname", ifname )) strcpy(prefix, "wan4");
#endif
	else strcpy(prefix, "wan");

	if (!wait_action_idle(10)) return 1;

	mwanlog(LOG_INFO, "dhcpc_event_main, interface=%s, wan_prefix=%s, argc=%d, argv=%s", ifname, prefix, argc, argv[1]);

	if ((argc == 2) && (ifname = getenv("interface")) != NULL) {
		TRACE_PT("event=%s\n", argv[1]);

		if (strcmp(argv[1], "deconfig") == 0) return deconfig(ifname, prefix);
		if (strcmp(argv[1], "bound") == 0) return bound(ifname, 0, prefix);
		if ((strcmp(argv[1], "renew") == 0) || (strcmp(argv[1], "update") == 0)) return renew(ifname,prefix);
	}

	return 1;
}


// -----------------------------------------------------------------------------


int dhcpc_release_main(int argc, char **argv)
{
	char prefix[] = "wanXXXXXXXXXX_";
	if(argc > 1){
		strcpy(prefix, argv[1]); } 
	else{
		strcpy(prefix, "wan"); }
	mwanlog(LOG_INFO, "dhcpc_release_main, argc=%d, wan_prefix=%s", argc, prefix);

	TRACE_PT("begin\n");
#ifdef TCONFIG_MULTIWAN
	mwan_table_del(prefix);
#endif

	if (!using_dhcpc(prefix)) return 1;
	
	char dhcpcpid_file[256];
	memset(dhcpcpid_file, 0, 256);
	sprintf(dhcpcpid_file, "/var/run/udhcpc-%s.pid", prefix);
	if (kill_pidfile_s(dhcpcpid_file, SIGUSR2) == 0) {
		sleep(2);
	}

	char renew_file[256];
	memset(renew_file, 0, 256);
	sprintf(renew_file, "/var/lib/misc/%s_dhcpc.renewing", prefix);
	unlink(renew_file);

	char wanconn_file[256];
	memset(wanconn_file, 0, 256);
	sprintf(wanconn_file, "/var/lib/misc/%s.connecting", prefix);
	unlink(wanconn_file);
#ifdef TCONFIG_MULTIWAN
	mwan_load_balance();
#endif
	TRACE_PT("end\n");
	return 0;
}

int dhcpc_renew_main(int argc, char **argv)
{
	char prefix[] = "wanXXXXXXXXXX_";
	if(argc > 0){
		strcpy(prefix, argv[1]); } 
	else{
		strcpy(prefix, "wan"); }
	mwanlog(LOG_INFO, "dhcpc_renew_main, argc=%d, wan_prefix=%s", argc, prefix);

	TRACE_PT("begin\n");

	if (!using_dhcpc(prefix)) return 1;
	
	char dhcpcpid_file[256];
	memset(dhcpcpid_file, 0, 256);
	sprintf(dhcpcpid_file, "/var/run/udhcpc-%s.pid", prefix);
	if (kill_pidfile_s(dhcpcpid_file, SIGUSR1) == 0) {
		char renew_file[256];
		memset(renew_file, 0, 256);
		sprintf(renew_file, "/var/lib/misc/%s_dhcpc.renewing", prefix);
		f_write(renew_file, NULL, 0, 0, 0);
	}
	else {
		stop_dhcpc(prefix);
		start_dhcpc(prefix);
	}

	TRACE_PT("end\n");
	return 0;
}


// -----------------------------------------------------------------------------


void start_dhcpc(char *prefix)
{
	char cmd[256];
	char *ifname;
	char *p;
	int proto;
	char tmp[100];

	TRACE_PT("begin\n");


	nvram_set(strcat_r(prefix, "_get_dns", tmp), "");

	char renew_file[256];
	memset(renew_file, 0, 256);
	sprintf(renew_file, "/var/lib/misc/%s_dhcpc.renewing", prefix);
	f_write(renew_file, NULL, 0, 0, 0);

	proto = get_wanx_proto(prefix);

	if (proto == WP_LTE) {
		ifname = nvram_safe_get("wan_4g");
	} else {
		ifname = nvram_safe_get(strcat_r(prefix, "_ifname", tmp));
	}
	if (proto == WP_DHCP || proto == WP_LTE) {
		nvram_set(strcat_r(prefix, "_iface", tmp), ifname);
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

	char dhcpcpid_file[256];
	memset(dhcpcpid_file, 0, 256);
	sprintf(dhcpcpid_file, "/var/run/udhcpc-%s.pid", prefix);
	snprintf(cmd, sizeof(cmd),
		"udhcpc -i %s -b -s dhcpc-event %s %s %s %s %s %s %s -p %s",
		ifname,
		nvram_invmatch("wan_hostname", "") ? "-H" : "", nvram_safe_get("wan_hostname"),
		// This params required to get static / classless routes from DHCP server
		nvram_get_int("dhcp_routes") ? "-O 33 -O 121 -O 249" : "",
		nvram_get_int("dhcpc_minpkt") ? "-m" : "",
		nvram_contains_word("log_events", "dhcpc") ? "-S" : "",
		nvram_safe_get("dhcpc_custom"),
#ifdef TCONFIG_IPV6
		(get_ipv6_service() == IPV6_6RD_DHCP) ? "-O ip6rd" : ""
#else
		""
#endif
		,
		dhcpcpid_file
		);

	mwanlog(LOG_DEBUG, "MultiWAN: start_dhcpc(%s), cmd = \"/bin/sh -c %s\"", prefix, cmd);
	xstart("/bin/sh", "-c", cmd);

	TRACE_PT("end\n");
}

void stop_dhcpc(char *prefix)
{
	TRACE_PT("begin\n");

	killall("dhcpc-event", SIGTERM);
	char dhcpcpid_file[256];
	memset(dhcpcpid_file, 0, 256);
	sprintf(dhcpcpid_file, "/var/run/udhcpc-%s.pid", prefix);
	if (kill_pidfile_s(dhcpcpid_file, SIGUSR2) == 0) {	// release
		sleep(2);
	}
	kill_pidfile_s(dhcpcpid_file, SIGTERM);
	unlink(dhcpcpid_file);

	char renew_file[256];
	memset(renew_file, 0, 256);
	sprintf(renew_file, "/var/lib/misc/%s_dhcpc.renewing", prefix);
	unlink(renew_file);

	TRACE_PT("end\n");
}

// -----------------------------------------------------------------------------

#ifdef TCONFIG_IPV6

int dhcp6c_state_main(int argc, char **argv)
{
	char prefix[INET6_ADDRSTRLEN];
	char *lanif;
	struct in6_addr addr;
	int i, r;

	TRACE_PT("begin\n");

	if (!wait_action_idle(10)) return 1;

    lanif = getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0);
    if (!nvram_match("ipv6_rtr_addr", lanif)) {
        nvram_set("ipv6_rtr_addr", lanif);
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
// (re)start dnsmasq and httpd
        set_host_domain_name();
        start_dnsmasq();
        start_httpd(); 
	}

	if (env2nv("new_domain_name_servers", "ipv6_get_dns")) {
		dns_to_resolv();
	}


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
	int ipv6_vlan;

	TRACE_PT("begin\n");

	// Check if turned on
	if (get_ipv6_service() != IPV6_NATIVE_DHCP) return;

	prefix_len = 64 - (nvram_get_int("ipv6_prefix_length") ? : 64);
	if (prefix_len < 0)
		prefix_len = 0;
	wan6face = nvram_safe_get("wan_iface");
	ipv6_vlan = nvram_get_int("ipv6_vlan");

	nvram_set("ipv6_get_dns", "");
	nvram_set("ipv6_rtr_addr", "");
	nvram_set("ipv6_prefix", "");

	// Create dhcp6c.conf
	unlink("/var/dhcp6c_duid");
	if ((f = fopen("/etc/dhcp6c.conf", "w"))) {
		fprintf(f,
			"interface %s {\n", wan6face);
		if (nvram_get_int("ipv6_pdonly") == 0) {
		fprintf(f,
			" send ia-na 0;\n");
		};
		fprintf(f,
			" send ia-pd 0;\n"
			" request domain-name-servers;\n"
			" script \"/sbin/dhcp6c-state\";\n"
			"};\n"
			"id-assoc pd 0 {\n"
			" prefix ::/%d infinity;\n"			
			" prefix-interface %s {\n"
			"  sla-id 0;\n"
			"  sla-len %d;\n"
			" 	};\n",
			nvram_get_int("ipv6_prefix_length"),
			nvram_safe_get("lan_ifname"),
			prefix_len);
		if ((ipv6_vlan & 1) && (prefix_len >= 1)) { //2 ipv6 /64 networks
		fprintf(f,
			"	prefix-interface %s {\n"
			"		sla-id 1;\n"
			"		sla-len %d;\n"
			"	};\n", nvram_safe_get("lan1_ifname"), prefix_len);
		};
		if ((ipv6_vlan & 2) && (prefix_len >= 2)) { //4 ipv6 /64 networks
		fprintf(f,
			"	prefix-interface %s {\n"
			"		sla-id 2;\n"
			"		sla-len %d;\n"
			"	};\n", nvram_safe_get("lan2_ifname"), prefix_len);
		};
		if ((ipv6_vlan & 4) && (prefix_len >= 2)) {
		fprintf(f,
			"	prefix-interface %s {\n"
			"		sla-id 3;\n"
			"		sla-len %d;\n"
			"	};\n", nvram_safe_get("lan3_ifname"), prefix_len);
		};		
		fprintf(f,
			"};\n"
			"id-assoc na 0 { };\n");
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
