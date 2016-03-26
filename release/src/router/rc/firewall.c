/*

	Copyright 2003-2005, CyberTAN Inc.  All Rights Reserved

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

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

#include <stdarg.h>
#include <arpa/inet.h>
#include <dirent.h>

static int web_lanport;
wanface_list_t wanfaces;
wanface_list_t wan2faces;
#ifdef TCONFIG_MULTIWAN
wanface_list_t wan3faces;
wanface_list_t wan4faces;
#endif
char lanface[IFNAMSIZ + 1];
char lan1face[IFNAMSIZ + 1];
char lan2face[IFNAMSIZ + 1];
char lan3face[IFNAMSIZ + 1];
#ifdef TCONFIG_IPV6
char wan6face[IFNAMSIZ + 1];
#endif
char lan_cclass[sizeof("xxx.xxx.xxx.") + 1];
#ifdef LINUX26
static int can_enable_fastnat;
#endif

#ifdef DEBUG_IPTFILE
static int debug_only = 0;
#endif

static int gateway_mode;
static int remotemanage;
static int wanup;
static int wan2up;
#ifdef TCONFIG_MULTIWAN
static int wan3up;
static int wan4up;
#endif

const char *chain_in_drop;
const char *chain_in_accept;
const char *chain_out_drop;
const char *chain_out_accept;
const char *chain_out_reject;

const char chain_wan_prerouting[] = "WANPREROUTING";
const char ipt_fname[] = "/etc/iptables";
FILE *ipt_file;

#ifdef TCONFIG_IPV6
const char ip6t_fname[] = "/etc/ip6tables";
FILE *ip6t_file;

// RFC-4890, sec. 4.3.1
const int allowed_icmpv6[] = { 1, 2, 3, 4, 128, 129 };
#endif

static int is_sta(int idx, int unit, int subunit, void *param)
{
	return (nvram_match(wl_nvname("mode", unit, subunit), "sta") && (nvram_match(wl_nvname("bss_enabled", unit, subunit), "1")));
}

/*
struct {
} firewall_data;
*/

// -----------------------------------------------------------------------------

#ifdef LINUX26
static const char *fastnat_run_dir = "/var/run/fastnat";

void allow_fastnat(const char *service, int allow)
{
	char p[128];

	snprintf(p, sizeof(p), "%s/%s", fastnat_run_dir, service);
	if (allow) {
		unlink(p);
	}
	else {
		mkdir_if_none(fastnat_run_dir);
		f_write_string(p, "", 0, 0);
	}
}

static inline int fastnat_allowed(void)
{
	DIR *dir;
	struct dirent *dp;
	int enabled;

	enabled = !nvram_get_int("qos_enable") && !nvram_get_int("fastnat_disable");

	if (enabled && (dir = opendir(fastnat_run_dir))) {
		while ((dp = readdir(dir))) {
			if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
				continue;
			enabled = 0;
			break;
		}
		closedir(dir);
	}

	return (enabled);
}

void try_enabling_fastnat(void)
{
	f_write_string("/proc/sys/net/ipv4/netfilter/ip_conntrack_fastnat",
		fastnat_allowed() ? "1" : "0", 0, 0);
}
#endif

void enable_ip_forward(void)
{
	/*
		ip_forward - BOOLEAN
			0 - disabled (default)
			not 0 - enabled

			Forward Packets between interfaces.

			This variable is special, its change resets all configuration
			parameters to their default state (RFC1122 for hosts, RFC1812
			for routers)
	*/
	f_write_string("/proc/sys/net/ipv4/ip_forward", "1", 0, 0);

}

#ifdef TCONFIG_IPV6
void enable_ip6_forward(void)
{
	if (ipv6_enabled()) {
		f_write_string("/proc/sys/net/ipv6/conf/default/forwarding", "1", 0, 0);
		f_write_string("/proc/sys/net/ipv6/conf/all/forwarding", "1", 0, 0);
	}
	else {
		f_write_string("/proc/sys/net/ipv6/conf/default/forwarding", "0", 0, 0);
		f_write_string("/proc/sys/net/ipv6/conf/all/forwarding", "0", 0, 0);
	}
}
#endif


// -----------------------------------------------------------------------------

/*
static int ip2cclass(char *ipaddr, char *new, int count)
{
	int ip[4];

	if (sscanf(ipaddr,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]) != 4) return 0;
	return snprintf(new, count, "%d.%d.%d.",ip[0],ip[1],ip[2]);
}
*/


static int dmz_dst(char *s)
{
	struct in_addr ia;
	char *p;
	int n;

	if (nvram_get_int("dmz_enable") <= 0) return 0;

	p = nvram_safe_get("dmz_ipaddr");
	if ((ia.s_addr = inet_addr(p)) == (in_addr_t)-1) {
		if (((n = atoi(p)) <= 0) || (n >= 255)) return 0;
		if (s) sprintf(s, "%s%d", lan_cclass, n);
		return 1;
	}

	if (s) strcpy(s, inet_ntoa(ia));
	return 1;
}

void ipt_log_unresolved(const char *addr, const char *addrtype, const char *categ, const char *name)
{
	char *pre, *post;

	pre = (name && *name) ? " for \"" : "";
	post = (name && *name) ? "\"" : "";

	syslog(LOG_WARNING, "firewall: "
		"%s: not using %s%s%s%s (could not resolve as valid %s address)",
		categ, addr, pre, (name) ? : "", post, (addrtype) ? : "IP");
}

int ipt_addr(char *addr, int maxlen, const char *s, const char *dir, int af,
	int strict, const char *categ, const char *name)
{
	char p[INET6_ADDRSTRLEN * 2];
	int r = 0;

	if ((s) && (*s) && (*dir))
	{
		if (sscanf(s, "%[0-9.]-%[0-9.]", p, p) == 2) {
			snprintf(addr, maxlen, "-m iprange --%s-range %s", dir, s);
			r = IPT_V4;
		}
#ifdef TCONFIG_IPV6
		else if (sscanf(s, "%[0-9A-Fa-f:]-%[0-9A-Fa-f:]", p, p) == 2) {
			snprintf(addr, maxlen, "-m iprange --%s-range %s", dir, s);
			r = IPT_V6;
		}
#endif
		else {
			snprintf(addr, maxlen, "-%c %s", dir[0], s);
			if (sscanf(s, "%[^/]/", p)) {
#ifdef TCONFIG_IPV6
				r = host_addrtypes(p, strict ? af : (IPT_V4 | IPT_V6));
#else
				r = host_addrtypes(p, IPT_V4);
#endif
			}
		}
	}
	else
	{
		*addr = 0;
		r = (IPT_V4 | IPT_V6);
	}

	if ((r == 0 || (strict && ((r & af) != af))) && (categ && *categ)) {
		ipt_log_unresolved(s, categ, name,
			(af & IPT_V4 & ~r) ? "IPv4" : ((af & IPT_V6 & ~r) ? "IPv6" : NULL));
	}

	return (r & af);
}

#define ipt_source_strict(s, src, categ, name) ipt_addr(src, 64, s, "src", IPT_V4, 1, categ, name)
#define ipt_source(s, src, categ, name) ipt_addr(src, 64, s, "src", IPT_V4, 0, categ, name)
#define ip6t_source(s, src, categ, name) ipt_addr(src, 128, s, "src", IPT_V6, 0, categ, name)

/*
static void get_src(const char *nv, char *src)
{
	char *p;

	if (((p = nvram_get(nv)) != NULL) && (*p) && (strlen(p) < 32)) {
		sprintf(src, "-%s %s", strchr(p, '-') ? "m iprange --src-range" : "s", p);
	}
	else {
		*src = 0;
	}
}
*/

void ipt_write(const char *format, ...)
{
	va_list args;

	va_start(args, format);
	vfprintf(ipt_file, format, args);
	va_end(args);
}

void ip6t_write(const char *format, ...)
{
#ifdef TCONFIG_IPV6
	va_list args;

	va_start(args, format);
	vfprintf(ip6t_file, format, args);
	va_end(args);
#endif
}

// -----------------------------------------------------------------------------

int ipt_dscp(const char *v, char *opt)
{
	unsigned int n;

	if (*v == 0) {
		*opt = 0;
		return 0;
	}

	n = strtoul(v, NULL, 0);
	if (n > 63) n = 63;
	sprintf(opt, " -m dscp --dscp 0x%02X", n);

#ifdef LINUX26
	modprobe("xt_dscp");
#else
	modprobe("ipt_dscp");
#endif
	return 1;
}

// -----------------------------------------------------------------------------


int ipt_ipp2p(const char *v, char *opt)
{
	int n = atoi(v);

	if (n == 0) {
		*opt = 0;
		return 0;
	}

	strcpy(opt, " -m ipp2p ");
	if ((n & 0xFFF) == 0xFFF) {
		strcat(opt, "--ipp2p");
	}
	else {
		// x12
		if (n & 0x0001) strcat(opt, "--apple ");
		if (n & 0x0002) strcat(opt, "--ares ");
		if (n & 0x0004) strcat(opt, "--bit ");
		if (n & 0x0008) strcat(opt, "--dc ");
		if (n & 0x0010) strcat(opt, "--edk ");
		if (n & 0x0020) strcat(opt, "--gnu ");
		if (n & 0x0040) strcat(opt, "--kazaa ");
		if (n & 0x0080) strcat(opt, "--mute ");
		if (n & 0x0100) strcat(opt, "--soul ");
		if (n & 0x0200) strcat(opt, "--waste ");
		if (n & 0x0400) strcat(opt, "--winmx ");
		if (n & 0x0800) strcat(opt, "--xdcc ");
#ifdef LINUX26
		if (n & 0x1000) strcat(opt, "--pp ");
		if (n & 0x2000) strcat(opt, "--xunlei ");
#endif
	}

	modprobe("ipt_ipp2p");
	return 1;
}


// -----------------------------------------------------------------------------


char **layer7_in;

// This L7 matches inbound traffic, caches the results, then the L7 outbound
// should read the cached result and set the appropriate marks	-- zzz
void ipt_layer7_inbound(void)
{
	int en, i;
	char **p;

	if (!layer7_in) return;

	en = nvram_match("nf_l7in", "1");
	if (en) {
		ipt_write(":L7in - [0:0]\n");
		if(wanup){
		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				ipt_write("-A FORWARD -i %s -j L7in\n",
					wanfaces.iface[i].name);
				}
			}
		}
		if(wan2up){
			for (i = 0; i < wan2faces.count; ++i) {
				if (*(wan2faces.iface[i].name)) {
					ipt_write("-A FORWARD -i %s -j L7in\n",
						wan2faces.iface[i].name);
				}
			}
		}
#ifdef TCONFIG_MULTIWAN
		if(wan3up){
			for (i = 0; i < wan3faces.count; ++i) {
				if (*(wan3faces.iface[i].name)) {
					ipt_write("-A FORWARD -i %s -j L7in\n",
						wan3faces.iface[i].name);
				}
			}
		}
		if(wan4up){
			for (i = 0; i < wan4faces.count; ++i) {
				if (*(wan4faces.iface[i].name)) {
					ipt_write("-A FORWARD -i %s -j L7in\n",
						wan4faces.iface[i].name);
				}
			}
		}
#endif
	}

	p = layer7_in;
	while (*p) {
		if (en) {
			ipt_write("-A L7in %s -j RETURN\n", *p);
#ifdef LINUX26
			can_enable_fastnat = 0;
#endif
		}
		free(*p);
		++p;
	}
	free(layer7_in);
	layer7_in = NULL;
}

int ipt_layer7(const char *v, char *opt)
{
	char s[128];
	char *path;

	*opt = 0;
	if (*v == 0) return 0;
	if (strlen(v) > 32) return -1;

	path = "/etc/l7-extra";
	sprintf(s, "%s/%s.pat", path, v);
	if (!f_exists(s)) {
		path = "/etc/l7-protocols";
		sprintf(s, "%s/%s.pat", path, v);
		if (!f_exists(s)) {
			syslog(LOG_ERR, "L7 %s was not found", v);
			return -1;
		}
	}

	sprintf(opt, " -m layer7 --l7dir %s --l7proto %s", path, v);

	if (nvram_match("nf_l7in", "1")) {
		if (!layer7_in) layer7_in = calloc(51, sizeof(char *));
		if (layer7_in) {
			char **p;

			p = layer7_in;
			while (*p) {
				if (strcmp(*p, opt) == 0) return 1;
				++p;
			}
			if (((p - layer7_in) / sizeof(char *)) < 50) *p = strdup(opt);
		}
	}

#ifdef LINUX26
	modprobe("xt_layer7");
#else
	modprobe("ipt_layer7");
#endif
	return 1;
}

// -----------------------------------------------------------------------------

static void ipt_account(void) {
	struct in_addr ipaddr, netmask, network;
	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ipaddr[] = "lanXX_ipaddr";
	char lanN_netmask[] = "lanXX_netmask";
	char lanN[] = "lanXX";
	char netaddrnetmask[] = "255.255.255.255/255.255.255.255 ";
	char br;
	// If the IP Address changes, the below rule will cause things to choke, and blocking rules don't get applied
	// As a workaround, flush the entire FORWARD chain
	system("iptables -F FORWARD");

	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(lanN_ifname, "lan%s_ifname", bridge);

		if (strcmp(nvram_safe_get(lanN_ifname), "")!=0) {

			sprintf(lanN_ipaddr, "lan%s_ipaddr", bridge);
			sprintf(lanN_netmask, "lan%s_netmask", bridge);
			sprintf(lanN, "lan%s", bridge);

			inet_aton(nvram_safe_get(lanN_ipaddr), &ipaddr);
			inet_aton(nvram_safe_get(lanN_netmask), &netmask);

			// bitwise AND of ip and netmask gives the network
			network.s_addr = ipaddr.s_addr & netmask.s_addr;

			sprintf(netaddrnetmask, "%s/%s", inet_ntoa(network), nvram_safe_get(lanN_netmask));

			//ipv4 only
			ipt_write("-A FORWARD -m account --aaddr %s --aname %s\n", netaddrnetmask, lanN);
		}
	}
}

// -----------------------------------------------------------------------------

static void save_webmon(void)
{
	eval("cp", "/proc/webmon_recent_domains", "/var/webmon/domain");
	eval("cp", "/proc/webmon_recent_searches", "/var/webmon/search");
}

static void ipt_webmon()
{
	int wmtype, clear, i;
	char t[512];
	char src[128];
	char *p, *c;
	int ok;

	if (!nvram_get_int("log_wm")) return;

#ifdef LINUX26
	can_enable_fastnat = 0;
#endif
	wmtype = nvram_get_int("log_wmtype");
	clear = nvram_get_int("log_wmclear");

	ip46t_write(":monitor - [0:0]\n");

	// include IPs
	strlcpy(t, wmtype == 1 ? nvram_safe_get("log_wmip") : "", sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if ((ok = ipt_addr(src, sizeof(src), p, "src", IPT_V4|IPT_V6, 0, "webmon", NULL))) {
#ifdef TCONFIG_IPV6
			if (*wan6face && (ok & IPT_V6))
				ip6t_write("-A FORWARD -o %s %s -j monitor\n", wan6face, src);
#endif
			if (ok & IPT_V4) {
				for (i = 0; i < wanfaces.count; ++i) {
					if (*(wanfaces.iface[i].name)) {
						ipt_write("-A FORWARD -o %s %s -j monitor\n",
							wanfaces.iface[i].name, src);
					}
				}
				for (i = 0; i < wan2faces.count; ++i) {
					if (*(wan2faces.iface[i].name)) {
						ipt_write("-A FORWARD -o %s %s -j monitor\n",
							wan2faces.iface[i].name, src);
					}
				}
#ifdef TCONFIG_MULTIWAN
				for (i = 0; i < wan3faces.count; ++i) {
					if (*(wan3faces.iface[i].name)) {
						ipt_write("-A FORWARD -o %s %s -j monitor\n",
							wan3faces.iface[i].name, src);
					}
				}
				for (i = 0; i < wan4faces.count; ++i) {
					if (*(wan4faces.iface[i].name)) {
						ipt_write("-A FORWARD -o %s %s -j monitor\n",
							wan4faces.iface[i].name, src);
					}
				}
#endif
			}
		}

		if (!c) break;
		p = c + 1;
	} while (*p);

	// exclude IPs
	if (wmtype == 2) {
		strlcpy(t, nvram_safe_get("log_wmip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL) *c = 0;
			if ((ok = ipt_addr(src, sizeof(src), p, "src", IPT_V4|IPT_V6, 0, "webmon", NULL))) {
				if (*src)
					ip46t_flagged_write(ok, "-A monitor %s -j RETURN\n", src);
			}
			if (!c) break;
			p = c + 1;
		} while (*p);
	}

	char webdomain[100];
	char websearch[100];

	if( nvram_match( "webmon_bkp", "1" ) ) {
		xstart( "/usr/sbin/webmon_bkp", "add" ); // add jobs to cru

		sprintf(webdomain, "--domain_load_file %s/webmon_recent_domains", nvram_safe_get("webmon_dir"));
		sprintf(websearch, "--search_load_file %s/webmon_recent_searches", nvram_safe_get("webmon_dir"));
	} else {
		sprintf(webdomain, "--domain_load_file /var/webmon/domain");
		sprintf(websearch, "--search_load_file /var/webmon/search");
	}

	ip46t_write(
		"-A monitor -p tcp -m webmon "
		"--max_domains %d --max_searches %d %s %s -j RETURN\n",
		nvram_get_int("log_wmdmax") ? : 1, nvram_get_int("log_wmsmax") ? : 1,
		(clear & 1) == 0 ? webdomain : "--clear_domain",
		(clear & 2) == 0 ? websearch : "--clear_search");

	if( nvram_match( "webmon_bkp", "1" ) )
		xstart( "/usr/sbin/webmon_bkp", "hourly" ); // make a copy immediately


#ifdef LINUX26
	modprobe("xt_webmon");
#else
	modprobe("ipt_webmon");
#endif

}


// -----------------------------------------------------------------------------
// MANGLE
// -----------------------------------------------------------------------------

static void mangle_table(void)
{
	int ttl;
	
	char *p, *wanface, *wan2face;
#ifdef TCONFIG_MULTIWAN
	char *wan3face, *wan4face;
#endif

	ip46t_write(
		"*mangle\n"
		":PREROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		);

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	|| wan3up || wan4up
#endif
	) {
		ipt_qos();
		//1 for mangle
		ipt_qoslimit(1);

		p = nvram_safe_get("nf_ttl");
		if (strncmp(p, "c:", 2) == 0) {
			p += 2;
			ttl = atoi(p);
			p = (ttl >= 0 && ttl <= 255) ? "set" : NULL;
		}
		else if ((ttl = atoi(p)) != 0) {
			if (ttl > 0) {
				p = "inc";
			}
			else {
				ttl = -ttl;
				p = "dec";
			}
			if (ttl > 255) p = NULL;
		}
		else p = NULL;

		if (p) {
#ifdef LINUX26
			modprobe("xt_HL");
#else
			modprobe("ipt_TTL");
#endif
			if(wanup){
			// set TTL on primary WAN iface only
			wanface = wanfaces.iface[0].name;
			ipt_write(
				"-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
				"-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
					wanface, p, ttl,
					wanface, p, ttl);
			}
			if(wan2up){
				// set TTL on primary WAN2 iface only
				wan2face = wan2faces.iface[0].name;
				ipt_write(
					"-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
					"-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
						wan2face, p, ttl,
						wan2face, p, ttl);
			}
#ifdef TCONFIG_MULTIWAN
			if(wan3up){
				// set TTL on primary WAN3 iface only
				wan3face = wan3faces.iface[0].name;
				ipt_write(
					"-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
					"-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
						wan3face, p, ttl,
						wan3face, p, ttl);
				}
			if(wan4up){
				// set TTL on primary WAN4 iface only
				wan4face = wan4faces.iface[0].name;
				ipt_write(
					"-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
					"-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
						wan4face, p, ttl,
						wan4face, p, ttl);
			}
#endif

#ifdef TCONFIG_IPV6
	// FIXME: IPv6 HL should be configurable separately from TTL.
	//        disable it until GUI setting is implemented.
	#if 0
			ip6t_write(
				"-I PREROUTING -i %s -j HL --hl-%s %d\n"
				"-I POSTROUTING -o %s -j HL --hl-%s %d\n",
					wan6face, p, ttl,
					wan6face, p, ttl);
	#endif
#endif
		}
// Reset Incoming DSCP to 0x00
		if (nvram_match("DSCP_fix_enable", "1")) {
#ifdef LINUX26
			modprobe("xt_DSCP");
#else
			modprobe("ipt_DSCP");
#endif
			ipt_write("-I PREROUTING -i %s -j DSCP --set-dscp 0\n", wanface);
		}
	}

	ip46t_write("COMMIT\n");
}

// -----------------------------------------------------------------------------
// NAT
// -----------------------------------------------------------------------------

static void nat_table(void)
{
	char lanaddr[32];
	char lanmask[32];
	char lan1addr[32];
	char lan1mask[32];
	char lan2addr[32];
	char lan2mask[32];
	char lan3addr[32];
	char lan3mask[32];
	char dst[64];
	char src[64];
	char t[512];
	char *p, *c, *torports;
	int i;

	ipt_write("*nat\n"
		":PREROUTING ACCEPT [0:0]\n"
		":POSTROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		":%s - [0:0]\n",
		chain_wan_prerouting);
	
	//2 for nat
	ipt_qoslimit(2);
	
	if (gateway_mode) {
		strlcpy(lanaddr, nvram_safe_get("lan_ipaddr"), sizeof(lanaddr));
		strlcpy(lanmask, nvram_safe_get("lan_netmask"), sizeof(lanmask));
		strlcpy(lan1addr, nvram_safe_get("lan1_ipaddr"), sizeof(lan1addr));
		strlcpy(lan1mask, nvram_safe_get("lan1_netmask"), sizeof(lan1mask));
		strlcpy(lan2addr, nvram_safe_get("lan2_ipaddr"), sizeof(lan2addr));
		strlcpy(lan2mask, nvram_safe_get("lan2_netmask"), sizeof(lan2mask));
		strlcpy(lan3addr, nvram_safe_get("lan3_ipaddr"), sizeof(lan3addr));
		strlcpy(lan3mask, nvram_safe_get("lan3_netmask"), sizeof(lan3mask));


		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				// chain_wan_prerouting
				if (wanup) {
					ipt_write("-A PREROUTING -d %s -j %s\n",
						wanfaces.iface[i].ip, chain_wan_prerouting);
				}

				// Drop incoming packets which destination IP address is to our LAN side directly
				ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
					wanfaces.iface[i].name,
					lanaddr, lanmask);	// note: ipt will correct lanaddr
				if(strcmp(lan1addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wanfaces.iface[i].name,
						lan1addr, lan1mask);
				if(strcmp(lan2addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wanfaces.iface[i].name,
						lan2addr, lan2mask);
				if(strcmp(lan3addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wanfaces.iface[i].name,
						lan3addr, lan3mask);
			}
		}

		for (i = 0; i < wan2faces.count; ++i) {
			if (*(wan2faces.iface[i].name)) {
				// chain_wan_prerouting
				if (wan2up) {
					ipt_write("-A PREROUTING -d %s -j %s\n",
						wan2faces.iface[i].ip, chain_wan_prerouting);
				}

				// Drop incoming packets which destination IP address is to our LAN side directly
				ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
					wan2faces.iface[i].name,
					lanaddr, lanmask);	// note: ipt will correct lanaddr
				if(strcmp(lan1addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan2faces.iface[i].name,
						lan1addr, lan1mask);
				if(strcmp(lan2addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan2faces.iface[i].name,
						lan2addr, lan2mask);
				if(strcmp(lan3addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan2faces.iface[i].name,
						lan3addr, lan3mask);
			}
		}
#ifdef TCONFIG_MULTIWAN
		for (i = 0; i < wan3faces.count; ++i) {
			if (*(wan3faces.iface[i].name)) {
				// chain_wan_prerouting
				if (wan3up) {
					ipt_write("-A PREROUTING -d %s -j %s\n",
						wan3faces.iface[i].ip, chain_wan_prerouting);
				}

				// Drop incoming packets which destination IP address is to our LAN side directly
				ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
					wan3faces.iface[i].name,
					lanaddr, lanmask);	// note: ipt will correct lanaddr
				if(strcmp(lan1addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan3faces.iface[i].name,
						lan1addr, lan1mask);
				if(strcmp(lan2addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan3faces.iface[i].name,
						lan2addr, lan2mask);
				if(strcmp(lan3addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan3faces.iface[i].name,
						lan3addr, lan3mask);
			}
		}

		for (i = 0; i < wan4faces.count; ++i) {
			if (*(wan4faces.iface[i].name)) {
				// chain_wan_prerouting
				if (wan4up) {
					ipt_write("-A PREROUTING -d %s -j %s\n",
						wan4faces.iface[i].ip, chain_wan_prerouting);
				}

				// Drop incoming packets which destination IP address is to our LAN side directly
				ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
					wan4faces.iface[i].name,
					lanaddr, lanmask);	// note: ipt will correct lanaddr
				if(strcmp(lan1addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan4faces.iface[i].name,
						lan1addr, lan1mask);
				if(strcmp(lan2addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan4faces.iface[i].name,
						lan2addr, lan2mask);
				if(strcmp(lan3addr,"")!=0)
					ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
						wan4faces.iface[i].name,
						lan3addr, lan3mask);
			}
		}
#endif

		if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
		|| wan3up || wan4up
#endif
		) {
			if (nvram_match("dns_intcpt", "1")) {
				ipt_write("-A PREROUTING -p udp -s %s/%s ! -d %s/%s --dport 53 -j DNAT --to-destination %s\n",
					lanaddr, lanmask,
					lanaddr, lanmask,
					lanaddr);
				if(strcmp(lan1addr,"")!=0)
					ipt_write("-A PREROUTING -p udp -s %s/%s ! -d %s/%s --dport 53 -j DNAT --to-destination %s\n",
						lan1addr, lan1mask,
						lan1addr, lan1mask,
						lan1addr);
				if(strcmp(lan2addr,"")!=0)
					ipt_write("-A PREROUTING -p udp -s %s/%s ! -d %s/%s --dport 53 -j DNAT --to-destination %s\n",
						lan2addr, lan2mask,
						lan2addr, lan2mask,
						lan2addr);
				if(strcmp(lan3addr,"")!=0)
					ipt_write("-A PREROUTING -p udp -s %s/%s ! -d %s/%s --dport 53 -j DNAT --to-destination %s\n",
						lan3addr, lan3mask,
						lan3addr, lan3mask,
						lan3addr);
			}

			// ICMP packets are always redirected to INPUT chains
			ipt_write("-A %s -p icmp -j DNAT --to-destination %s\n", chain_wan_prerouting, lanaddr);


			//force remote access to router if DMZ is enabled - shibby
			if( (nvram_match("dmz_enable", "1")) && (nvram_match("dmz_ra", "1")) ) {
				strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
				p = t;
				do {
					if ((c = strchr(p, ',')) != NULL) *c = 0;
					ipt_source(p, src, "ra", NULL);

					if (remotemanage) {
						ipt_write("-A %s -p tcp -m tcp %s --dport %s -j DNAT --to-destination %s:%d\n",
							chain_wan_prerouting, src, nvram_safe_get("http_wanport"), lanaddr, web_lanport);
					}

					if (nvram_get_int("sshd_remote")) {
						ipt_write("-A %s %s -p tcp -m tcp --dport %s -j DNAT --to-destination %s:%s\n",
							chain_wan_prerouting, src, nvram_safe_get("sshd_rport"), lanaddr, nvram_safe_get("sshd_port"));
					}

					if (!c) break;
					p = c + 1;
				} while (*p);
			}


			ipt_forward(IPT_TABLE_NAT);
			ipt_triggered(IPT_TABLE_NAT);
		}

		if (nvram_get_int("upnp_enable") & 3) {
			ipt_write(":upnp - [0:0]\n");
			ipt_write(":pupnp - [0:0]\n");

			for (i = 0; i < wanfaces.count; ++i) {
				if (*(wanfaces.iface[i].name)) {
					if (wanup) {
						// ! for loopback (all) to work
						ipt_write("-A PREROUTING -d %s -j upnp\n", wanfaces.iface[i].ip);
					}
					else {
						ipt_write("-A PREROUTING -i %s -j upnp\n", wanfaces.iface[i].name);
					}
				}
			}

			for (i = 0; i < wan2faces.count; ++i) {
				if (*(wan2faces.iface[i].name)) {
					if (wan2up) {
						// ! for loopback (all) to work
						ipt_write("-A PREROUTING -d %s -j upnp\n", wan2faces.iface[i].ip);
					}
					else {
						ipt_write("-A PREROUTING -i %s -j upnp\n", wan2faces.iface[i].name);
					}
				}
			}
#ifdef TCONFIG_MULTIWAN
			for (i = 0; i < wan3faces.count; ++i) {
				if (*(wan3faces.iface[i].name)) {
					if (wan3up) {
						// ! for loopback (all) to work
						ipt_write("-A PREROUTING -d %s -j upnp\n", wan3faces.iface[i].ip);
					}
					else {
						ipt_write("-A PREROUTING -i %s -j upnp\n", wan3faces.iface[i].name);
					}
				}
			}
			for (i = 0; i < wan4faces.count; ++i) {
				if (*(wan4faces.iface[i].name)) {
					if (wan4up) {
						// ! for loopback (all) to work
						ipt_write("-A PREROUTING -d %s -j upnp\n", wan4faces.iface[i].ip);
					}
					else {
						ipt_write("-A PREROUTING -i %s -j upnp\n", wan4faces.iface[i].name);
					}
				}
			}
#endif

		}

#ifdef TCONFIG_TOR
		//TOR
		if (nvram_match("tor_enable", "1")) {

			if (nvram_match( "tor_ports", "custom" ) ) {
				torports = nvram_safe_get( "tor_ports_custom" );
			} else {
				torports = nvram_safe_get( "tor_ports" );
			}

			if (nvram_match("tor_iface", "br0"))  {
				ipt_write("-A PREROUTING -i %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
					nvram_safe_get("tor_iface"), torports, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_ipaddr"), nvram_safe_get("tor_transport") );
			} else if (nvram_match("tor_iface", "br1")) {
				ipt_write("-A PREROUTING -i %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
					nvram_safe_get("tor_iface"), torports, nvram_safe_get("lan1_ipaddr"), nvram_safe_get("lan1_ipaddr"), nvram_safe_get("tor_transport") );
			} else if (nvram_match("tor_iface", "br2")) {
				ipt_write("-A PREROUTING -i %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
					nvram_safe_get("tor_iface"), torports, nvram_safe_get("lan2_ipaddr"), nvram_safe_get("lan2_ipaddr"), nvram_safe_get("tor_transport") );
			} else if (nvram_match("tor_iface", "br3")) {
				ipt_write("-A PREROUTING -i %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
					nvram_safe_get("tor_iface"), torports, nvram_safe_get("lan3_ipaddr"), nvram_safe_get("lan3_ipaddr"), nvram_safe_get("tor_transport") );
			} else {
				strlcpy(t, nvram_safe_get("tor_users"), sizeof(t));
				p = t;
				do {
					if ((c = strchr(p, ',')) != NULL) *c = 0;

					if (ipt_source_strict(p, src, "tor", NULL))
						ipt_write("-A PREROUTING %s -p tcp -m multiport --dport %s ! -d %s -j DNAT --to-destination %s:%s\n",
							src, torports, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_ipaddr"), nvram_safe_get("tor_transport") );

					if (!c) break;
					p = c + 1;
				} while (*p);
			}
		}
#endif

#ifdef TCONFIG_SNMP
		if( nvram_match( "snmp_enable", "1" ) && nvram_match("snmp_remote", "1"))
		{
			ipt_write("-A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n", nvram_safe_get("snmp_port"), lanaddr);
		}
#endif	
		
#ifdef TCONFIG_FTP	// !!TB - FTP Server
		if (nvram_match("ftp_enable", "1")) 
		{	// FTP WAN access enabled
			ipt_write("-A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n", nvram_safe_get("ftp_port"), lanaddr);
		}
#endif

#ifdef TCONFIG_BT
		//BT Client ports from WAN interface
		if (nvram_get_int( "bt_enable" ) && nvram_match( "bt_rpc_wan", "1"))
		{
			ipt_write("-A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n", nvram_safe_get("bt_port_gui"), lanaddr);
		}
#endif

#ifdef TCONFIG_ARUI
		//ARIA2 RPC from WAN interface
		if (nvram_get_int( "ar_enable" ) && nvram_match( "ar_rpc_wan", "1"))
		{
			ipt_write("-A WANPREROUTING -p tcp --dport %s -j DNAT --to-destination %s\n", nvram_safe_get("ar_rpc_port"), lanaddr);
		}
#endif

		
#ifdef TCONFIG_SAMBASRV
		//Samba WAN Access
		if (nvram_get_int("smbd_enable") && nvram_match("smbd_wanaccess", "1")) {
			ipt_write("-A WANPREROUTING -p tcp -m multiport --dport 139,445 -j DNAT --to-destination %s\n", lanaddr);
		}
#endif

		if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
		|| wan3up || wan4up
#endif
		) {
			if (dmz_dst(dst)) {
				strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
				p = t;
				do {
					if ((c = strchr(p, ',')) != NULL) *c = 0;
					if (ipt_source_strict(p, src, "dmz", NULL))
						ipt_write("-A %s %s -j DNAT --to-destination %s\n", chain_wan_prerouting, src, dst);
					if (!c) break;
					p = c + 1;
				} while (*p);
			}
		}
		
		p = "";
#ifdef TCONFIG_IPV6
		switch (get_ipv6_service()) {
		case IPV6_6IN4:
			// avoid NATing proto-41 packets when using 6in4 tunnel
			p = "-p ! 41";
			break;
		}
#endif

		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				if ((!wanup) || (nvram_get_int("ne_snat") != 1))
					ipt_write("-A POSTROUTING %s -o %s -j MASQUERADE\n", p, wanfaces.iface[i].name);
				else
					ipt_write("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", p, wanfaces.iface[i].name, wanfaces.iface[i].ip);
			}
		}

		for (i = 0; i < wan2faces.count; ++i) {
			if (*(wan2faces.iface[i].name)) {
				if ((!wan2up) || (nvram_get_int("ne_snat") != 1))
					ipt_write("-A POSTROUTING %s -o %s -j MASQUERADE\n", p, wan2faces.iface[i].name);
				else
					ipt_write("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", p, wan2faces.iface[i].name, wan2faces.iface[i].ip);
			}
		}
#ifdef TCONFIG_MULTIWAN
		for (i = 0; i < wan3faces.count; ++i) {
			if (*(wan3faces.iface[i].name)) {
				if ((!wan3up) || (nvram_get_int("ne_snat") != 1))
					ipt_write("-A POSTROUTING %s -o %s -j MASQUERADE\n", p, wan3faces.iface[i].name);
				else
					ipt_write("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", p, wan3faces.iface[i].name, wan3faces.iface[i].ip);
			}
		}
		for (i = 0; i < wan4faces.count; ++i) {
			if (*(wan4faces.iface[i].name)) {
				if ((!wan4up) || (nvram_get_int("ne_snat") != 1))
					ipt_write("-A POSTROUTING %s -o %s -j MASQUERADE\n", p, wan4faces.iface[i].name);
				else
					ipt_write("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", p, wan4faces.iface[i].name, wan4faces.iface[i].ip);
			}
		}
#endif

		char *wan_modem_ipaddr;
		if ( (nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "dhcp") || nvram_match("wan_proto", "static") )
		    && (wan_modem_ipaddr = nvram_safe_get("wan_modem_ipaddr")) && *wan_modem_ipaddr && !nvram_match("wan_modem_ipaddr","0.0.0.0")
		    && (!foreach_wif(1, NULL, is_sta)) )
			ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan_ifname"), wan_modem_ipaddr);

		char *wan2_modem_ipaddr;
		if ( (nvram_match("wan2_proto", "pppoe") || nvram_match("wan2_proto", "dhcp") || nvram_match("wan2_proto", "static") )
		    && (wan2_modem_ipaddr = nvram_safe_get("wan2_modem_ipaddr")) && *wan2_modem_ipaddr && !nvram_match("wan2_modem_ipaddr","0.0.0.0")
		    && (!foreach_wif(1, NULL, is_sta)) )
			ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan2_ifname"), wan2_modem_ipaddr);

#ifdef TCONFIG_MULTIWAN
		char *wan3_modem_ipaddr;
		if ( (nvram_match("wan3_proto", "pppoe") || nvram_match("wan3_proto", "dhcp") || nvram_match("wan3_proto", "static") )
		    && (wan3_modem_ipaddr = nvram_safe_get("wan3_modem_ipaddr")) && *wan3_modem_ipaddr && !nvram_match("wan3_modem_ipaddr","0.0.0.0")
		    && (!foreach_wif(1, NULL, is_sta)) )
			ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan3_ifname"), wan3_modem_ipaddr);

		char *wan4_modem_ipaddr;
		if ( (nvram_match("wan4_proto", "pppoe") || nvram_match("wan4_proto", "dhcp") || nvram_match("wan4_proto", "static") )
		    && (wan4_modem_ipaddr = nvram_safe_get("wan4_modem_ipaddr")) && *wan4_modem_ipaddr && !nvram_match("wan4_modem_ipaddr","0.0.0.0")
		    && (!foreach_wif(1, NULL, is_sta)) )
			ipt_write("-A POSTROUTING -o %s -d %s -j MASQUERADE\n", nvram_safe_get("wan4_ifname"), wan4_modem_ipaddr);
#endif

		switch (nvram_get_int("nf_loopback")) {
		case 1:		// 1 = forwarded-only
		case 2:		// 2 = disable
			break;
		default:	// 0 = all (same as block_loopback=0)
			ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j SNAT --to-source %s\n",
				lanface,
				lanaddr, lanmask,
				lanaddr, lanmask,
				lanaddr);
			if (strcmp(lan1face,"")!=0)
				ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j SNAT --to-source %s\n",
					lan1face,
					lan1addr, lan1mask,
					lan1addr, lan1mask,
					lan1addr);
			if (strcmp(lan2face,"")!=0)
				ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j SNAT --to-source %s\n",
					lan2face,
					lan2addr, lan2mask,
					lan2addr, lan2mask,
					lan2addr);
			if (strcmp(lan3face,"")!=0)
				ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j SNAT --to-source %s\n",
					lan3face,
					lan3addr, lan3mask,
					lan3addr, lan3mask,
					lan3addr);
			break;
		}		
	}
	ipt_write("COMMIT\n");
}

// -----------------------------------------------------------------------------
// FILTER
// -----------------------------------------------------------------------------

static void filter_input(void)
{
	char s[64];
	char t[512];
	char *en;
	char *sec;
	char *hit;
	int n;
	char *p, *c;

	if ((nvram_get_int("nf_loopback") != 0) && (wanup)) {	// 0 = all
		for (n = 0; n < wanfaces.count; ++n) {
			if (*(wanfaces.iface[n].name)) {
				ipt_write("-A INPUT -i %s -d %s -j DROP\n", lanface, wanfaces.iface[n].ip);
				if (strcmp(lan1face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan1face, wanfaces.iface[n].ip);
				if (strcmp(lan2face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan2face, wanfaces.iface[n].ip);
				if (strcmp(lan3face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan3face, wanfaces.iface[n].ip);
			}
		}
	}

	if ((nvram_get_int("nf_loopback") != 0) && (wan2up)) {	// 0 = all
		for (n = 0; n < wan2faces.count; ++n) {
			if (*(wan2faces.iface[n].name)) {
				ipt_write("-A INPUT -i %s -d %s -j DROP\n", lanface, wan2faces.iface[n].ip);
				if (strcmp(lan1face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan1face, wan2faces.iface[n].ip);
				if (strcmp(lan2face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan2face, wan2faces.iface[n].ip);
				if (strcmp(lan3face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan3face, wan2faces.iface[n].ip);
			}
		}
	}

#ifdef TCONFIG_MULTIWAN
	if ((nvram_get_int("nf_loopback") != 0) && (wan3up)) {	// 0 = all
		for (n = 0; n < wan3faces.count; ++n) {
			if (*(wan3faces.iface[n].name)) {
				ipt_write("-A INPUT -i %s -d %s -j DROP\n", lanface, wan3faces.iface[n].ip);
				if (strcmp(lan1face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan1face, wan3faces.iface[n].ip);
				if (strcmp(lan2face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan2face, wan3faces.iface[n].ip);
				if (strcmp(lan3face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan3face, wan3faces.iface[n].ip);
			}
		}
	}
	
	if ((nvram_get_int("nf_loopback") != 0) && (wan4up)) {	// 0 = all
		for (n = 0; n < wan4faces.count; ++n) {
			if (*(wan4faces.iface[n].name)) {
				ipt_write("-A INPUT -i %s -d %s -j DROP\n", lanface, wan4faces.iface[n].ip);
				if (strcmp(lan1face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan1face, wan4faces.iface[n].ip);
				if (strcmp(lan2face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan2face, wan4faces.iface[n].ip);
				if (strcmp(lan3face,"")!=0)
					ipt_write("-A INPUT -i %s -d %s -j DROP\n", lan3face, wan4faces.iface[n].ip);
			}
		}
	}
#endif

	ipt_write(
		"-A INPUT -m state --state INVALID -j DROP\n"
		"-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n");

	strlcpy(s, nvram_safe_get("ne_shlimit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && ((n = atoi(en) & 3) != 0)) {
/*
		? what if the user uses the start button in GUI ?
		if (nvram_get_int("telnetd_eas"))
		if (nvram_get_int("sshd_eas"))
*/
#ifdef LINUX26
		modprobe("xt_recent");
#else
		modprobe("ipt_recent");
#endif

		ipt_write(
			"-N shlimit\n"
			"-A shlimit -m recent --set --name shlimit\n"
			"-A shlimit -m recent --update --hitcount %d --seconds %s --name shlimit -j %s\n",
			atoi(hit) + 1, sec, chain_in_drop);

		if (n & 1) {
			ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_port"));
			if (nvram_get_int("sshd_remote") && nvram_invmatch("sshd_rport", nvram_safe_get("sshd_port"))) {
				ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_rport"));
			}
		}
		if (n & 2) ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("telnetd_port"));
	}

#ifdef TCONFIG_FTP
	strlcpy(s, nvram_safe_get("ftp_limit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && (atoi(en)) && (nvram_get_int("ftp_enable") == 1)) {
#ifdef LINUX26
		modprobe("xt_recent");
#else
		modprobe("ipt_recent");
#endif

		ipt_write(
			"-N ftplimit\n"
			"-A ftplimit -m recent --set --name ftp\n"
			"-A ftplimit -m recent --update --hitcount %d --seconds %s --name ftp -j %s\n",
			atoi(hit) + 1, sec, chain_in_drop);
		ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j ftplimit\n", nvram_safe_get("ftp_port"));
	}
#endif

	ipt_write(
		"-A INPUT -i lo -j ACCEPT\n"
		"-A INPUT -i %s -j ACCEPT\n",
			lanface);
	if (strcmp(lan1face,"")!=0)
		ipt_write(
			"-A INPUT -i %s -j ACCEPT\n",
				lan1face);
	if (strcmp(lan2face,"")!=0)
		ipt_write(
			"-A INPUT -i %s -j ACCEPT\n",
				lan2face);
	if (strcmp(lan3face,"")!=0)
		ipt_write(
			"-A INPUT -i %s -j ACCEPT\n",
				lan3face);

#ifdef TCONFIG_IPV6
	n = get_ipv6_service();
	switch (n) {
	case IPV6_ANYCAST_6TO4:
	case IPV6_6IN4:
		// Accept ICMP requests from the remote tunnel endpoint
		if (n == IPV6_ANYCAST_6TO4)
			sprintf(s, "192.88.99.%d", nvram_get_int("ipv6_relay"));
		else
			strlcpy(s, nvram_safe_get("ipv6_tun_v4end"), sizeof(s));
		if (*s && strcmp(s, "0.0.0.0") != 0)
			ipt_write("-A INPUT -p icmp -s %s -j %s\n", s, chain_in_accept);
		ipt_write("-A INPUT -p 41 -j %s\n", chain_in_accept);
		break;
	}
#endif

	// ICMP request from WAN interface
	if (nvram_match("block_wan", "0")) {
		if (nvram_match("block_wan_limit", "0")) {
			// allow ICMP packets to be received
			ipt_write("-A INPUT -p icmp -j %s\n", chain_in_accept);
			// allow udp traceroute packets
			ipt_write("-A INPUT -p udp --dport 33434:33534 -j %s\n", chain_in_accept);
		} else {
			// allow ICMP packets to be received, but restrict the flow to avoid ping flood attacks
			ipt_write("-A INPUT -p icmp -m limit --limit %d/second -j %s\n", nvram_get_int("block_wan_limit_icmp"), chain_in_accept);
			// allow udp traceroute packets, but restrict the flow to avoid ping flood attacks
			ipt_write("-A INPUT -p udp --dport 33434:33534 -m limit --limit %d/second -j %s\n", nvram_get_int("block_wan_limit_tr"), chain_in_accept);
		}
	}

	/* Accept incoming packets from broken dhcp servers, which are sending replies
	 * from addresses other than used for query. This could lead to a lower level
	 * of security, so allow to disable it via nvram variable.
	 */
	if (nvram_invmatch("wan_dhcp_pass", "0") && (using_dhcpc("wan") || using_dhcpc("wan2")
#ifdef TCONFIG_MULTIWAN
	|| using_dhcpc("wan3") || using_dhcpc("wan4")
#endif
	)) {
		ipt_write("-A INPUT -p udp --sport 67 --dport 68 -j %s\n", chain_in_accept);
	}

	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if (ipt_source(p, s, "remote management", NULL)) {

			if (remotemanage) {
				ipt_write("-A INPUT -p tcp %s --dport %s -j %s\n",
					s, nvram_safe_get("http_wanport"), chain_in_accept);
			}

			if (nvram_get_int("sshd_remote")) {
				ipt_write("-A INPUT -p tcp %s --dport %s -j %s\n",
					s, nvram_safe_get("sshd_rport"), chain_in_accept);
			}
		}

		if (!c) break;
		p = c + 1;
	} while (*p);

#ifdef TCONFIG_NGINX //Tomato RAF - Web Server
		if (nvram_match("nginx_enable", "1") && nvram_match("nginx_remote", "1"))
			ipt_write("-A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "nginx_port" ));
#endif

#ifdef TCONFIG_FTP	// !!TB - FTP Server
	if (nvram_match("ftp_enable", "1")) {	// FTP WAN access enabled
		strlcpy(t, nvram_safe_get("ftp_sip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL) *c = 0;
			if (ipt_source(p, s, "ftp", "remote access")) {
				ipt_write("-A INPUT -p tcp %s --dport %s -j %s\n",
					s, nvram_safe_get("ftp_port"), chain_in_accept);
			}
			if (!c) break;
			p = c + 1;
		} while (*p);
	}
#endif

#ifdef TCONFIG_SNMP
	if( nvram_match( "snmp_enable", "1" ) && nvram_match("snmp_remote", "1"))
	{
		strlcpy(t, nvram_safe_get("snmp_remote_sip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL) *c = 0;

			if (ipt_source(p, s, "snmp", "remote")) {
				ipt_write("-A INPUT -p udp %s --dport %s -j %s\n",
					s, nvram_safe_get("snmp_port"), chain_in_accept);
			}

			if (!c) break;
			p = c + 1;
		} while (*p);
	}
#endif

	// IGMP query from WAN interface
	if ((nvram_match("multicast_pass", "1")) || (nvram_match("udpxy_enable", "1"))) {
		ipt_write("-A INPUT -p igmp -d 224.0.0.0/4 -j ACCEPT\n");
		ipt_write("-A INPUT -p udp -d 224.0.0.0/4 ! --dport 1900 -j ACCEPT\n");
	}

	// Routing protocol, RIP, accept
	if (nvram_invmatch("dr_wan_rx", "0")) {
		ipt_write("-A INPUT -p udp --dport 520 -j ACCEPT\n");
	}

#ifdef TCONFIG_BT
	//BT Client ports from WAN interface
	if (nvram_match("bt_enable", "1")) {
		ipt_write( "-A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "bt_port" ) );
		if (nvram_match( "bt_rpc_wan", "1") )
		{
			ipt_write( "-A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "bt_port_gui" ) );
		}
	}
#endif	

#ifdef TCONFIG_ARUI
	//ARIA2 RPC from WAN interface
	if (nvram_match("ar_enable", "1")) {
		if (nvram_match( "ar_bt_en", "1" ) )
		{
			ipt_write( "-A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "ar_bt_port" ) );
		}
		if (nvram_match( "ar_rpc_wan", "1") )
		{
			ipt_write( "-A INPUT -p tcp --dport %s -j ACCEPT\n", nvram_safe_get( "ar_rpc_port" ) );
		}
	}
#endif	


#ifdef TCONFIG_SAMBASRV
	//Samba WAN Access
	if (nvram_get_int("smbd_enable") && nvram_match("smbd_wanaccess", "1")) {
		ipt_write("-A INPUT -p tcp -m multiport --dport 139,445 -j ACCEPT \n");
	}
#endif

#ifdef TCONFIG_PPTPD
	//Add for pptp server
	if (nvram_match("pptpd_enable", "1")) {
		ipt_write("-A INPUT -p tcp --dport 1723 -j ACCEPT\n");
		ipt_write("-A INPUT -p 47 -j ACCEPT\n");
	}
#endif

	// if logging
	if (*chain_in_drop == 'l') {
		ipt_write( "-A INPUT -j %s\n", chain_in_drop);
	}

	// default policy: DROP
}

// clamp TCP MSS to PMTU of WAN interface (IPv4 only?)
static void clampmss(void)
{
	ipt_write("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n");
#ifdef TCONFIG_IPV6
	switch (get_ipv6_service()) {
	case IPV6_ANYCAST_6TO4:
	case IPV6_6IN4:
		ip6t_write("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n");
		break;
	}
#endif
}

static void filter_forward(void)
{
	char dst[64];
	char src[64];
	char t[512];
	char *p, *c;
	int i;

#ifdef TCONFIG_IPV6
	ip6t_write(
		"-A FORWARD -m rt --rt-type 0 -j DROP\n");
#endif

	if (nvram_match("cstats_enable", "1")) {
		ipt_account();
	}

	ip46t_write(
		"-A FORWARD -i %s -o %s -j ACCEPT\n",			// accept all lan to lan
		lanface, lanface);
	if (strcmp(lan1face,"")!=0)
		ip46t_write(
			"-A FORWARD -i %s -o %s -j ACCEPT\n",
			lan1face, lan1face);
	if (strcmp(lan2face,"")!=0)
		ip46t_write(
			"-A FORWARD -i %s -o %s -j ACCEPT\n",
			lan2face, lan2face);
	if (strcmp(lan3face,"")!=0)
		ip46t_write(
			"-A FORWARD -i %s -o %s -j ACCEPT\n",
			lan3face, lan3face);

	char lanAccess[17] = "0000000000000000";

	const char *d, *sbr, *saddr, *dbr, *daddr, *desc;
	char *nv, *nvp, *b;
	int n;
	nvp = nv = strdup(nvram_safe_get("lan_access"));
	if (nv) {
		while ((b = strsep(&nvp, ">")) != NULL) {
			/*
				1<0<1.2.3.4<1<5.6.7.8<30,45-50<desc

				1 = enabled
				0 = src bridge
				1.2.3.4 = src addr
				1 = dst bridge
				5.6.7.8 = dst addr
				desc = desc
			*/
			n = vstrsep(b, "<", &d, &sbr, &saddr, &dbr, &daddr, &desc);
			if (*d != '1')
				continue;
			if (!ipt_addr(src, sizeof(src), saddr, "src", IPT_V4|IPT_V6, 0, "LAN access", desc))
				continue;
			if (!ipt_addr(dst, sizeof(dst), daddr, "dst", IPT_V4|IPT_V6, 0, "LAN access", desc))
				continue;

			//ipv4 only
			ipt_write("-A FORWARD -i %s%s -o %s%s %s %s -j ACCEPT\n",
				"br",
				sbr,
				"br",
				dbr,
				src,
				dst);

			if ((strcmp(src,"")==0) && (strcmp(dst,"")==0))
				lanAccess[((*sbr-48)+(*dbr-48)*4)] = '1';

		}
	}
	free(nv);

	ip46t_write(
		"-A FORWARD -m state --state INVALID -j DROP\n");		// drop if INVALID state

	// clamp tcp mss to pmtu
	clampmss();

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	|| wan3up || wan4up
#endif
	) {
		ipt_restrictions();

		ipt_layer7_inbound();
	}

	ipt_webmon();

	ip46t_write(
		":wanin - [0:0]\n"
		":wanout - [0:0]\n"
		"-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n");	// already established or related (via helper)

	char lanN_ifname[] = "lanXX_ifname";
	char br;
	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(lanN_ifname, "lan%s_ifname", bridge);
		if (strncmp(nvram_safe_get(lanN_ifname), "br", 2) == 0) {
			char lanN_ifname2[] = "lanXX_ifname";
			char br2;
			for(br2=0 ; br2<=3 ; br2++) {
				if (br==br2) continue;

				if (lanAccess[((br)+(br2)*4)] == '1') continue;

				char bridge2[2] = "0";
				if (br2!=0)
					bridge2[0]+=br2;
				else
					strcpy(bridge2, "");

				sprintf(lanN_ifname2, "lan%s_ifname", bridge2);
				if (strncmp(nvram_safe_get(lanN_ifname2), "br", 2) == 0) {
					ipt_write("-A FORWARD -i %s -o %s -j DROP\n",
						nvram_safe_get(lanN_ifname),
						nvram_safe_get(lanN_ifname2));
				}
			}
//		ip46t_write("-A FORWARD -i %s -j %s\n", nvram_safe_get(lanN_ifname), chain_out_accept);
		}
	}

#ifdef TCONFIG_IPV6
	// Filter out invalid WAN->WAN connections
	if (*wan6face)
//		ip6t_write("-A FORWARD -o %s ! -i %s -j %s\n", wan6face, lanface, chain_in_drop); //shibby - we cant drop connections from WAN to LAN1-3
		ip6t_write("-A FORWARD -o %s -i %s -j %s\n", wan6face, wan6face, chain_in_drop); //shibby - drop connection from WAN -> WAN only

#ifdef LINUX26
	modprobe("xt_length");
	ip6t_write("-A FORWARD -p ipv6-nonxt -m length --length 40 -j ACCEPT\n");
#endif

	// ICMPv6 rules
	for (i = 0; i < sizeof(allowed_icmpv6)/sizeof(int); ++i) {
		ip6t_write("-A FORWARD -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_icmpv6[i], chain_in_accept);
	}

	//IPv6
	if (*wan6face) {
		ip6t_write(
			"-A FORWARD -i %s -j wanin\n"				// generic from wan
			"-A FORWARD -o %s -j wanout\n",				// generic to wan
			wan6face, wan6face);
	}
#endif

	//IPv4
	for (i = 0; i < wanfaces.count; ++i) {
		if (*(wanfaces.iface[i].name)) {
			ipt_write(
				"-A FORWARD -i %s -j wanin\n"			// generic from wan
				"-A FORWARD -o %s -j wanout\n",			// generic to wan
				wanfaces.iface[i].name, wanfaces.iface[i].name);
		}
	}

	for (i = 0; i < wan2faces.count; ++i) {
		if (*(wan2faces.iface[i].name)) {
			ipt_write(
				"-A FORWARD -i %s -j wanin\n"			// generic from wan
				"-A FORWARD -o %s -j wanout\n",			// generic to wan
				wan2faces.iface[i].name, wan2faces.iface[i].name);
		}
	}

#ifdef TCONFIG_MULTIWAN
	for (i = 0; i < wan3faces.count; ++i) {
		if (*(wan3faces.iface[i].name)) {
			ipt_write(
				"-A FORWARD -i %s -j wanin\n"			// generic from wan
				"-A FORWARD -o %s -j wanout\n",			// generic to wan
				wan3faces.iface[i].name, wan3faces.iface[i].name);
		}
	}
	for (i = 0; i < wan4faces.count; ++i) {
		if (*(wan4faces.iface[i].name)) {
			ipt_write(
				"-A FORWARD -i %s -j wanin\n"			// generic from wan
				"-A FORWARD -o %s -j wanout\n",			// generic to wan
				wan4faces.iface[i].name, wan4faces.iface[i].name);
		}
	}
#endif

	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(lanN_ifname, "lan%s_ifname", bridge);
		if (strncmp(nvram_safe_get(lanN_ifname), "br", 2) == 0) {
			ip46t_write("-A FORWARD -i %s -j %s\n", nvram_safe_get(lanN_ifname), chain_out_accept);
		}
	}

#ifdef TCONFIG_IPV6
//IPv6 forward LAN->WAN accept
	ip6t_write("-A FORWARD -i %s -o %s -j %s\n", lanface, wan6face, chain_out_accept);

	if (strcmp(lan1face,"")!=0)
		ip6t_write("-A FORWARD -i %s -o %s -j %s\n", lan1face, wan6face, chain_out_accept);
	if (strcmp(lan2face,"")!=0)
		ip6t_write("-A FORWARD -i %s -o %s -j %s\n", lan2face, wan6face, chain_out_accept);
	if (strcmp(lan3face,"")!=0)
		ip6t_write("-A FORWARD -i %s -o %s -j %s\n", lan3face, wan6face, chain_out_accept);
#endif

	// IPv4 only
	if (nvram_get_int("upnp_enable") & 3) {
		ipt_write(":upnp - [0:0]\n");
		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				ipt_write("-A FORWARD -i %s -j upnp\n",
					wanfaces.iface[i].name);
			}
		}
	}

	if (wanup || wan2up
#ifdef TCONFIG_MULTIWAN
	|| wan3up || wan4up
#endif
	) {
		if ((nvram_match("multicast_pass", "1")) || (nvram_match("udpxy_enable", "1"))) {
			ipt_write("-A wanin -p udp -d 224.0.0.0/4 -j %s\n", chain_in_accept);
		}
		ipt_triggered(IPT_TABLE_FILTER);
		ipt_forward(IPT_TABLE_FILTER);
#ifdef TCONFIG_IPV6
		ip6t_forward();
#endif

		if (dmz_dst(dst)) {
			char dmz_ifname[IFNAMSIZ+1];
			strlcpy(dmz_ifname, nvram_safe_get("dmz_ifname"), sizeof(dmz_ifname));
			if(strcmp(dmz_ifname, "") == 0)
				strlcpy(dmz_ifname, lanface, sizeof(lanface));
			strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL) *c = 0;
				if (ipt_source(p, src, "dmz", NULL))
					ipt_write("-A FORWARD -o %s %s -d %s -j %s\n", dmz_ifname, src, dst, chain_in_accept);
				if (!c) break;
				p = c + 1;
			} while (*p);
		}
	}
	// default policy: DROP
}

static void filter_log(void)
{
	int n;
	char limit[128];

	n = nvram_get_int("log_limit");
	if ((n >= 1) && (n <= 9999)) {
		sprintf(limit, "-m limit --limit %d/m", n);
	}
	else {
		limit[0] = 0;
	}

#ifdef TCONFIG_IPV6
	modprobe("ip6t_LOG");
#endif
	if ((*chain_in_drop == 'l') || (*chain_out_drop == 'l'))  {
		ip46t_write(
			":logdrop - [0:0]\n"
			"-A logdrop -m state --state NEW %s -j LOG --log-prefix \"DROP \""
#ifdef LINUX26
				" --log-macdecode"
#endif
				" --log-tcp-sequence --log-tcp-options --log-ip-options\n"
			"-A logdrop -j DROP\n"
			":logreject - [0:0]\n"
			"-A logreject %s -j LOG --log-prefix \"REJECT \""
#ifdef LINUX26
				" --log-macdecode"
#endif
				" --log-tcp-sequence --log-tcp-options --log-ip-options\n"
			"-A logreject -p tcp -j REJECT --reject-with tcp-reset\n",
			limit, limit);
	}
	if ((*chain_in_accept == 'l') || (*chain_out_accept == 'l'))  {
		ip46t_write(
			":logaccept - [0:0]\n"
			"-A logaccept -m state --state NEW %s -j LOG --log-prefix \"ACCEPT \""
#ifdef LINUX26
				" --log-macdecode"
#endif
				" --log-tcp-sequence --log-tcp-options --log-ip-options\n"
			"-A logaccept -j ACCEPT\n",
			limit);
	}
}

#ifdef TCONFIG_IPV6
static void filter6_input(void)
{
	char s[128];
	char t[512];
	char *en;
	char *sec;
	char *hit;
	int n;	
	char *p, *c;

	// RFC-4890, sec. 4.4.1
	const int allowed_local_icmpv6[] =
		{ 130, 131, 132, 133, 134, 135, 136,
		  141, 142, 143,
		  148, 149, 151, 152, 153 };

	ip6t_write(
		"-A INPUT -m rt --rt-type 0 -j %s\n"
		/* "-A INPUT -m state --state INVALID -j DROP\n" */
		"-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n",
		chain_in_drop);

#ifdef LINUX26
	modprobe("xt_length");
	ip6t_write("-A INPUT -p ipv6-nonxt -m length --length 40 -j ACCEPT\n");
#endif

	strlcpy(s, nvram_safe_get("ne_shlimit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && ((n = atoi(en) & 3) != 0)) {
#ifdef LINUX26
		modprobe("xt_recent");
#else
		modprobe("ipt_recent");
#endif

		ip6t_write(
			"-N shlimit\n"
			"-A shlimit -m recent --set --name shlimit\n"
			"-A shlimit -m recent --update --hitcount %d --seconds %s --name shlimit -j %s\n",
			atoi(hit) + 1, sec, chain_in_drop);

		if (n & 1) {
			ip6t_write("-A INPUT -i %s -p tcp --dport %s -m state --state NEW -j shlimit\n", lanface, nvram_safe_get("sshd_port"));
			if (nvram_get_int("sshd_remote") && nvram_invmatch("sshd_rport", nvram_safe_get("sshd_port"))) {
				ip6t_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_rport"));
			}
		}
		if (n & 2) ip6t_write("-A INPUT -i %s -p tcp --dport %s -m state --state NEW -j shlimit\n", lanface, nvram_safe_get("telnetd_port"));
	}

#ifdef TCONFIG_FTP
	strlcpy(s, nvram_safe_get("ftp_limit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && (atoi(en)) && (nvram_get_int("ftp_enable") == 1)) {
#ifdef LINUX26
		modprobe("xt_recent");
#else
		modprobe("ipt_recent");
#endif

		ip6t_write(
			"-N ftplimit\n"
			"-A ftplimit -m recent --set --name ftp\n"
			"-A ftplimit -m recent --update --hitcount %d --seconds %s --name ftp -j %s\n",
			atoi(hit) + 1, sec, chain_in_drop);
		ip6t_write("-A INPUT -p tcp --dport %s -m state --state NEW -j ftplimit\n", nvram_safe_get("ftp_port"));
	}
#endif	// TCONFIG_FTP

	ip6t_write(
		"-A INPUT -i %s -j ACCEPT\n" // anything coming from LAN
		"-A INPUT -i lo -j ACCEPT\n",
			lanface );

	switch (get_ipv6_service()) {
	case IPV6_ANYCAST_6TO4:
	case IPV6_NATIVE_DHCP:
		// allow responses from the dhcpv6 server
		ip6t_write("-A INPUT -p udp --dport 546 -j %s\n", chain_in_accept);
		break;
	}

	// ICMPv6 rules
	for (n = 0; n < sizeof(allowed_icmpv6)/sizeof(int); n++) {
		ip6t_write("-A INPUT -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_icmpv6[n], chain_in_accept);
	}
	for (n = 0; n < sizeof(allowed_local_icmpv6)/sizeof(int); n++) {
		ip6t_write("-A INPUT -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_local_icmpv6[n], chain_in_accept);
	}

	// Remote Managment
	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if (ip6t_source(p, s, "remote management", NULL)) {

			if (remotemanage) {
				ip6t_write("-A INPUT -p tcp %s --dport %s -j %s\n",
					s, nvram_safe_get("http_wanport"), chain_in_accept);
			}

			if (nvram_get_int("sshd_remote")) {
				ip6t_write("-A INPUT -p tcp %s --dport %s -j %s\n",
					s, nvram_safe_get("sshd_rport"), chain_in_accept);
			}
		}

		if (!c) break;
		p = c + 1;
	} while (*p);

#ifdef TCONFIG_FTP
	// FTP server
	if (nvram_match("ftp_enable", "1")) {	// FTP WAN access enabled
		strlcpy(t, nvram_safe_get("ftp_sip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL) *c = 0;
			if (ip6t_source(p, s, "ftp", "remote access")) {
				ip6t_write("-A INPUT -p tcp %s --dport %s -j %s\n",
					s, nvram_safe_get("ftp_port"), chain_in_accept);
			}
			if (!c) break;
			p = c + 1;
		} while (*p);
	}
#endif

	// if logging
	if (*chain_in_drop == 'l') {
		ip6t_write( "-A INPUT -j %s\n", chain_in_drop);
	}

	// default policy: DROP
}

#endif

static void filter_table(void)
{
	ip46t_write(
		"*filter\n"
		":INPUT DROP [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
	);

	filter_log();

	filter_input();
#ifdef TCONFIG_IPV6
	filter6_input();
	ip6t_write("-A OUTPUT -m rt --rt-type 0 -j %s\n", chain_in_drop);
#endif

	if ((gateway_mode) || (nvram_match("wk_mode_x", "1"))) {
		ip46t_write(":FORWARD DROP [0:0]\n");
		filter_forward();
	}
	else {
		ip46t_write(":FORWARD ACCEPT [0:0]\n");
		clampmss();
	}
	ip46t_write("COMMIT\n");
}

// -----------------------------------------------------------------------------

int start_firewall(void)
{
	DIR *dir;
	struct dirent *dirent;
	char s[256];
	
	char *c, *wanface, *wan2face;
#ifdef TCONFIG_MULTIWAN
	char *wan3face, *wan4face;
#endif
	int n;
	int wanproto;
	char *iptrestore_argv[] = { "iptables-restore", (char *)ipt_fname, NULL };
#ifdef TCONFIG_IPV6
	char *ip6trestore_argv[] = { "ip6tables-restore", (char *)ip6t_fname, NULL };
#endif

	simple_lock("firewall");
	simple_lock("restrictions");

	wanup = check_wanup("wan");
	wan2up = check_wanup("wan2");
#ifdef TCONFIG_MULTIWAN
	wan3up = check_wanup("wan3");
	wan4up = check_wanup("wan4");
#endif

	f_write_string("/proc/sys/net/ipv4/tcp_syncookies", nvram_get_int("ne_syncookies") ? "1" : "0", 0, 0);

	/* NAT performance tweaks
	 * These values can be overriden later if needed via firewall script
	 */
	f_write_string("/proc/sys/net/core/netdev_max_backlog", "3072", 0, 0);
	f_write_string("/proc/sys/net/core/somaxconn", "3072", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_max_syn_backlog", "8192", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_fin_timeout", "30", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_keepalive_intvl", "24", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_keepalive_probes", "3", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_keepalive_time", "1800", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_retries2", "5", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_syn_retries", "3", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_synack_retries", "3", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_tw_recycle", "1", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_tw_reuse", "1", 0, 0);

	/* DoS-related tweaks */
	f_write_string("/proc/sys/net/ipv4/icmp_ignore_bogus_error_responses", "1", 0, 0);
	f_write_string("/proc/sys/net/ipv4/tcp_rfc1337", "1", 0, 0);
	f_write_string("/proc/sys/net/ipv4/ip_local_port_range", "1024 65535", 0, 0);

	wanproto = get_wan_proto();
	f_write_string("/proc/sys/net/ipv4/ip_dynaddr", (wanproto == WP_DISABLED || wanproto == WP_STATIC) ? "0" : "1", 0, 0);

#ifdef TCONFIG_EMF
	/* Force IGMPv2 due EMF limitations */
	if (nvram_get_int("emf_enable")) {
		f_write_string("/proc/sys/net/ipv4/conf/default/force_igmp_version", "2", 0, 0);
		f_write_string("/proc/sys/net/ipv4/conf/all/force_igmp_version", "2", 0, 0);
	}
#endif

	n = nvram_get_int("log_in");
	chain_in_drop = (n & 1) ? "logdrop" : "DROP";
	chain_in_accept = (n & 2) ? "logaccept" : "ACCEPT";

	n = nvram_get_int("log_out");
	chain_out_drop = (n & 1) ? "logdrop" : "DROP";
	chain_out_reject = (n & 1) ? "logreject" : "REJECT --reject-with tcp-reset";
	chain_out_accept = (n & 2) ? "logaccept" : "ACCEPT";

//	if (nvram_match("nf_drop_reset", "1")) chain_out_drop = chain_out_reject;

	strlcpy(lanface, nvram_safe_get("lan_ifname"), IFNAMSIZ);
	strlcpy(lan1face, nvram_safe_get("lan1_ifname"), IFNAMSIZ);
	strlcpy(lan2face, nvram_safe_get("lan2_ifname"), IFNAMSIZ);
	strlcpy(lan3face, nvram_safe_get("lan3_ifname"), IFNAMSIZ);

	memcpy(&wanfaces, get_wanfaces("wan"), sizeof(wanfaces));
	memcpy(&wan2faces, get_wanfaces("wan2"), sizeof(wan2faces));
#ifdef TCONFIG_MULTIWAN
	memcpy(&wan3faces, get_wanfaces("wan3"), sizeof(wan3faces));
	memcpy(&wan4faces, get_wanfaces("wan4"), sizeof(wan4faces));
#endif
	wanface = wanfaces.iface[0].name;
	wan2face = wan2faces.iface[0].name;
#ifdef TCONFIG_MULTIWAN
	wan3face = wan3faces.iface[0].name;
	wan4face = wan4faces.iface[0].name;
#endif
#ifdef TCONFIG_IPV6
	strlcpy(wan6face, get_wan6face(), sizeof(wan6face));
#endif

#ifdef LINUX26
	can_enable_fastnat = 1;
#endif

	strlcpy(s, nvram_safe_get("lan_ipaddr"), sizeof(s));
	if ((c = strrchr(s, '.')) != NULL) *(c + 1) = 0;
	strlcpy(lan_cclass, s, sizeof(lan_cclass));
/*
	strlcpy(s, nvram_safe_get("lan1_ipaddr"), sizeof(s));
	if ((c = strrchr(s, '.')) != NULL) *(c + 1) = 0;
	strlcpy(lan1_cclass, s, sizeof(lan1_cclass));

	strlcpy(s, nvram_safe_get("lan2_ipaddr"), sizeof(s));
	if ((c = strrchr(s, '.')) != NULL) *(c + 1) = 0;
	strlcpy(lan2_cclass, s, sizeof(lan2_cclass));

	strlcpy(s, nvram_safe_get("lan3_ipaddr"), sizeof(s));
	if ((c = strrchr(s, '.')) != NULL) *(c + 1) = 0;
	strlcpy(lan3_cclass, s, sizeof(lan3_cclass));
*/

	/*
		block obviously spoofed IP addresses

		rp_filter - BOOLEAN
			1 - do source validation by reversed path, as specified in RFC1812
			    Recommended option for single homed hosts and stub network
			    routers. Could cause troubles for complicated (not loop free)
			    networks running a slow unreliable protocol (sort of RIP),
			    or using static routes.
			0 - No source validation.
	*/
	c = nvram_get("wan_ifname");
	/* mcast needs rp filter to be turned off only for non default iface */
	if (!(nvram_match("multicast_pass", "1")) || !(nvram_match("udpxy_enable", "1")) || strcmp(wanface, c) == 0) c = NULL;

	if ((dir = opendir("/proc/sys/net/ipv4/conf")) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			sprintf(s, "/proc/sys/net/ipv4/conf/%s/rp_filter", dirent->d_name);
			f_write_string(s, (c && strcmp(dirent->d_name, c) == 0) ? "0" : "1", 0, 0);
		}
		closedir(dir);
	}

	remotemanage = 0;
	gateway_mode = !nvram_match("wk_mode", "router");
	if (gateway_mode) {
		/* Remote management */
		if (nvram_match("remote_management", "1") && nvram_invmatch("http_wanport", "") &&
			nvram_invmatch("http_wanport", "0")) remotemanage = 1;

		if (nvram_match("remote_mgt_https", "1")) {
			web_lanport = nvram_get_int("https_lanport");
			if (web_lanport <= 0) web_lanport = 443;
		} else {
			web_lanport = nvram_get_int("http_lanport");
			if (web_lanport <= 0) web_lanport = 80;
		}
	}

	if ((ipt_file = fopen(ipt_fname, "w")) == NULL) {
		notice_set("iptables", "Unable to create iptables restore file");
		simple_unlock("firewall");
		return 0;
	}
	
#ifdef TCONFIG_IPV6
	if ((ip6t_file = fopen(ip6t_fname, "w")) == NULL) {
		notice_set("ip6tables", "Unable to create ip6tables restore file");
		simple_unlock("firewall");
		return 0;
	}
	modprobe("nf_conntrack_ipv6");
	modprobe("ip6t_REJECT");
#endif

//shibby
//if (nvram_match("imq_enable", "1")) {
//	char numdevs[10];
//	sprintf(numdevs, "numdevs=%d", nvram_get_int("imq_numdevs"));
//	modprobe("imq", numdevs );

	modprobe("imq");
#ifdef LINUX26
	modprobe("xt_IMQ");
#else
	modprobe("ipt_IMQ");
#endif
//	}


	mangle_table();
	nat_table();
	filter_table();

	fclose(ipt_file);
	ipt_file = NULL;
	
#ifdef TCONFIG_IPV6
	fclose(ip6t_file);
	ip6t_file = NULL;
#endif

#ifdef DEBUG_IPTFILE
	if (debug_only) {
		simple_unlock("firewall");
		simple_unlock("restrictions");
		return 0;
	}
#endif

	save_webmon();

	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/save", NULL, 0, 0, 0);
		if (killall("miniupnpd", SIGUSR2) == 0) {
			f_wait_notexists("/etc/upnp/save", 5);
		}
	}

	notice_set("iptables", "");
	if (_eval(iptrestore_argv, ">/var/notice/iptables", 0, NULL) == 0) {
		led(LED_DIAG, 0);
		notice_set("iptables", "");
	}
	else {
		sprintf(s, "%s.error", ipt_fname);
		rename(ipt_fname, s);
		syslog(LOG_CRIT, "Error while loading rules. See %s file.", s);
		led(LED_DIAG, 1);

		/*

		-P INPUT DROP
		-F INPUT
		-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT
		-A INPUT -i br0 -j ACCEPT

		-P FORWARD DROP
		-F FORWARD
		-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT
		-A FORWARD -i br0 -j ACCEPT

		*/
	}
	
#ifdef TCONFIG_IPV6
	if (ipv6_enabled()) {
		notice_set("ip6tables", "");
		if (_eval(ip6trestore_argv, ">/var/notice/ip6tables", 0, NULL) == 0) {
			notice_set("ip6tables", "");
		}
		else {
			sprintf(s, "%s.error", ip6t_fname);
			rename(ip6t_fname, s);
			syslog(LOG_CRIT, "Error while loading rules. See %s file.", s);
			led(LED_DIAG, 1);
		}
	}
	else {
		eval("ip6tables", "-F");
		eval("ip6tables", "-t", "mangle", "-F");
	}
#endif

	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/load", NULL, 0, 0, 0);
		killall("miniupnpd", SIGUSR2);
	}

	simple_unlock("restrictions");
	sched_restrictions();
	enable_ip_forward();
#ifdef TCONFIG_IPV6
	if (ipv6_enabled()) enable_ip6_forward();
#endif

	led(LED_DMZ, dmz_dst(NULL));

#ifdef TCONFIG_IPV6
	modprobe_r("nf_conntrack_ipv6");
	modprobe_r("ip6t_LOG");
	modprobe_r("ip6t_REJECT");
#endif
#ifdef LINUX26
	modprobe_r("xt_layer7");
	modprobe_r("xt_recent");
	modprobe_r("xt_HL");
	modprobe_r("xt_length");
	modprobe_r("xt_web");
	modprobe_r("xt_webmon");
	modprobe_r("xt_dscp");
#else
	modprobe_r("ipt_layer7");
	modprobe_r("ipt_recent");
	modprobe_r("ipt_TTL");
	modprobe_r("ipt_web");
	modprobe_r("ipt_webmon");
	modprobe_r("ipt_dscp");
#endif
	modprobe_r("ipt_ipp2p");

	unlink("/var/webmon/domain");
	unlink("/var/webmon/search");

#ifdef TCONFIG_OPENVPN
	run_vpn_firewall_scripts();
#endif

#ifdef TCONFIG_TINC
	run_tinc_firewall_script();
#endif

	run_nvscript("script_fire", NULL, 1);

#ifdef LINUX26
	allow_fastnat("firewall", can_enable_fastnat);
	try_enabling_fastnat();
#endif
	simple_unlock("firewall");
	return 0;
}

int stop_firewall(void)
{
	led(LED_DMZ, 0);
	return 0;
}

#ifdef DEBUG_IPTFILE
void create_test_iptfile(void)
{
	debug_only = 1;
	start_firewall();
	debug_only = 0;
}
#endif

