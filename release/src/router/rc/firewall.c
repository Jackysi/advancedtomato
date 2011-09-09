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

wanface_list_t wanfaces;
char lanface[IFNAMSIZ + 1];
char lan1face[IFNAMSIZ + 1];
char lan2face[IFNAMSIZ + 1];
char lan3face[IFNAMSIZ + 1];
#ifdef TCONFIG_IPV6
char wan6face[IFNAMSIZ + 1];
#endif
char lan_cclass[sizeof("xxx.xxx.xxx.") + 1];

#ifdef DEBUG_IPTFILE
static int debug_only = 0;
#endif

static int gateway_mode;
static int remotemanage;
static int wanup;

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
#endif


/*
struct {
} firewall_data;
*/

// -----------------------------------------------------------------------------


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

#ifdef TCONFIG_IPV6
	if (ipv6_enabled()) {
		f_write_string("/proc/sys/net/ipv6/conf/default/forwarding", "1", 0, 0);
		f_write_string("/proc/sys/net/ipv6/conf/all/forwarding", "1", 0, 0);
	}
#endif
}


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

int ipt_addr(char *addr, int maxlen, const char *s, const char *dir, int family,
	const char *categ, const char *name)
{
	char p[INET6_ADDRSTRLEN * 2];
	int r = 1;

	if ((s) && (*s) && (*dir))
	{
		if (sscanf(s, "%[0-9.]-%[0-9.]", p, p) == 2) {
			snprintf(addr, maxlen, "-m iprange --%s-range %s", dir, s);
			r = (family == AF_INET);
		}
#ifdef TCONFIG_IPV6
		else if (sscanf(s, "%[0-9A-Fa-f:]-%[0-9A-Fa-f:]", p, p) == 2) {
			snprintf(addr, maxlen, "-m iprange --%s-range %s", dir, s);
			r = (family == AF_INET6);
		}
#endif
		else {
			snprintf(addr, maxlen, "-%c %s", dir[0], s);
			r = (host_to_addr(s, family) != NULL);
		}
	}
	else
		*addr = 0;

	if (r == 0 && (categ && *categ)) {
		syslog(LOG_WARNING,
			"IPv%d firewall: %s: not using %s%s%s (could not resolve as valid IPv%d address)",
			(family == AF_INET6) ? 6 : 4, categ, s,
			(name && *name) ? " for " : "", (name && *name) ? name : "",
			(family == AF_INET6) ? 6 : 4);
	}

	return r;
}

#define ipt_source(s, src, categ, name) ipt_addr(src, 64, s, "src", AF_INET, categ, name)
#define ip6t_source(s, src, categ, name) ipt_addr(src, 128, s, "src", AF_INET6, categ, name)

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

	strcpy(opt, "-m ipp2p ");
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
		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				ipt_write("-A FORWARD -i %s -j L7in\n",
					wanfaces.iface[i].name);
			}
		}
	}

	p = layer7_in;
	while (*p) {
		if (en) ipt_write("-A L7in %s -j RETURN\n", *p);
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

	sprintf(opt, "-m layer7 --l7dir %s --l7proto %s", path, v);

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

static void save_webmon(void)
{
	system("cp /proc/webmon_recent_domains /var/webmon/domain");
	system("cp /proc/webmon_recent_searches /var/webmon/search");
}

static void ipt_webmon(int do_ip6t)
{
	int wmtype, clear, i;
	char t[512];
	char src[128];
	char *p, *c;

	if (!nvram_get_int("log_wm")) return;
	wmtype = nvram_get_int("log_wmtype");
	clear = nvram_get_int("log_wmclear");

	ip46t_cond_write(do_ip6t, ":monitor - [0:0]\n");

	// include IPs
	strlcpy(t, wmtype == 1 ? nvram_safe_get("log_wmip") : "", sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if (ipt_addr(src, sizeof(src), p, "src", do_ip6t ? AF_INET6 : AF_INET, "webmon", "filtering")) {
#ifdef TCONFIG_IPV6
			if (do_ip6t) {
				if (*wan6face)
					ip6t_write("-A FORWARD -o %s %s -j monitor\n", wan6face, src);
			}
			else
#endif
			for (i = 0; i < wanfaces.count; ++i) {
				if (*(wanfaces.iface[i].name)) {
					ipt_write("-A FORWARD -o %s %s -j monitor\n",
						wanfaces.iface[i].name, src);
				}
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
			if (ipt_addr(src, sizeof(src), p, "src", do_ip6t ? AF_INET6 : AF_INET, "webmon", "filtering")) {
				if (*src)
					ip46t_cond_write(do_ip6t, "-A monitor %s -j RETURN\n", src);
			}
			if (!c) break;
			p = c + 1;
		} while (*p);
	}

	ip46t_cond_write(do_ip6t,
		"-A monitor -p tcp -m webmon "
		"--max_domains %d --max_searches %d %s %s -j RETURN\n",
		nvram_get_int("log_wmdmax") ? : 1, nvram_get_int("log_wmsmax") ? : 1,
		(clear & 1) == 0 ? "--domain_load_file /var/webmon/domain" : "--clear_domain",
		(clear & 2) == 0 ? "--search_load_file /var/webmon/search" : "--clear_search");

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
	char *p, *wanface;

	ip46t_write(
		"*mangle\n"
		":PREROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n");

	if (wanup) {

		ipt_qos();

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
			// set TTL on primary WAN iface only
			wanface = wanfaces.iface[0].name;
			ip46t_write(
				"-I PREROUTING -i %s -j TTL --ttl-%s %d\n"
				"-I POSTROUTING -o %s -j TTL --ttl-%s %d\n",
					wanface, p, ttl,
					wanface, p, ttl);
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
	char *p, *c;
	int i;

	ipt_write("*nat\n"
		":PREROUTING ACCEPT [0:0]\n"
		":POSTROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
		":%s - [0:0]\n",
		chain_wan_prerouting);

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
				if (wanup)
					ipt_write("-A PREROUTING -d %s -j %s\n",
						wanfaces.iface[i].ip, chain_wan_prerouting);

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

		if (wanup) {
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

			ipt_forward(IPT_TABLE_NAT);
			ipt_triggered(IPT_TABLE_NAT);
		}

		if (nvram_get_int("upnp_enable") & 3) {
			ipt_write(":upnp - [0:0]\n");
			if (wanup) {
				// ! for loopback (all) to work
				ipt_write("-A %s -j upnp\n", chain_wan_prerouting);
			}
			else {
				for (i = 0; i < wanfaces.count; ++i) {
					if (*(wanfaces.iface[i].name)) {
						ipt_write("-A PREROUTING -i %s -j upnp\n", wanfaces.iface[i].name);
					}
				}
			}
		}

		if (wanup) {
			if (dmz_dst(dst)) {
				strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
				p = t;
				do {
					if ((c = strchr(p, ',')) != NULL) *c = 0;
					if (ipt_source(p, src, "dmz", NULL))
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
				if ((!wanup) || (nvram_get_int("net_snat") != 1))
					ipt_write("-A POSTROUTING %s -o %s -j MASQUERADE\n", p, wanfaces.iface[i].name);
				else
					ipt_write("-A POSTROUTING %s -o %s -j SNAT --to-source %s\n", p, wanfaces.iface[i].name, wanfaces.iface[i].ip);
			}
		}

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

	ipt_write(
		"-A INPUT -m state --state INVALID -j %s\n"
		"-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n",
		chain_in_drop);


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
	switch (get_ipv6_service()) {
	case IPV6_6IN4:
		// Accept ICMP requests from the remote tunnel endpoint
		if ((p = nvram_get("ipv6_tun_v4end")) && *p && strcmp(p, "0.0.0.0") != 0)
			ipt_write("-A INPUT -p icmp -s %s -j %s\n", p, chain_in_accept);
		ipt_write("-A INPUT -p 41 -j %s\n", chain_in_accept);
		break;
	}
#endif

	// ICMP request from WAN interface
	if (nvram_match("block_wan", "0")) {
		// allow ICMP packets to be received, but restrict the flow to avoid ping flood attacks
		ipt_write("-A INPUT -p icmp -m limit --limit 1/second -j %s\n", chain_in_accept);
		// allow udp traceroute packets
		ipt_write("-A INPUT -p udp -m udp --dport 33434:33534 -m limit --limit 5/second -j %s\n", chain_in_accept);
	}

	/* Accept incoming packets from broken dhcp servers, which are sending replies
	 * from addresses other than used for query. This could lead to a lower level
	 * of security, so allow to disable it via nvram variable.
	 */
	if (nvram_invmatch("dhcp_pass", "0") && using_dhcpc()) {
		ipt_write("-A INPUT -p udp --sport 67 --dport 68 -j %s\n", chain_in_accept);
	}

	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if (ipt_source(p, s, "remote management", NULL)) {

			if (remotemanage) {
				ipt_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
					s, nvram_safe_get("http_wanport"), chain_in_accept);
			}

			if (nvram_get_int("sshd_remote")) {
				ipt_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
					s, nvram_safe_get("sshd_rport"), chain_in_accept);
			}
		}

		if (!c) break;
		p = c + 1;
	} while (*p);

#ifdef TCONFIG_FTP	// !!TB - FTP Server
	if (nvram_match("ftp_enable", "1")) {	// FTP WAN access enabled
		strlcpy(t, nvram_safe_get("ftp_sip"), sizeof(t));
		p = t;
		do {
			if ((c = strchr(p, ',')) != NULL) *c = 0;
			if (ipt_source(p, s, "ftp", "remote access")) {
				ipt_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
					s, nvram_safe_get("ftp_port"), chain_in_accept);
			}
			if (!c) break;
			p = c + 1;
		} while (*p);
	}
#endif

	// IGMP query from WAN interface
	if (nvram_match("multicast_pass", "1")) {
		ipt_write("-A INPUT -p igmp -d 224.0.0.0/4 -j ACCEPT\n");
		ipt_write("-A INPUT -p udp -d 224.0.0.0/4 ! --dport 1900 -j ACCEPT\n");
	}

	// Routing protocol, RIP, accept
	if (nvram_invmatch("dr_wan_rx", "0")) {
		ipt_write("-A INPUT -p udp -m udp --dport 520 -j ACCEPT\n");
	}

	// if logging
	if (*chain_in_drop == 'l') {
		ipt_write( "-A INPUT -j %s\n", chain_in_drop);
	}

	// default policy: DROP
}

// clamp TCP MSS to PMTU of WAN interface
static void clampmss(void)
{
#if 1
	ipt_write("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --clamp-mss-to-pmtu\n");
#else
	int rmtu = nvram_get_int("wan_run_mtu");
	ipt_write("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss %d: -j TCPMSS ", rmtu - 39);
	if (rmtu < 576) {
		ipt_write("--clamp-mss-to-pmtu\n");
	}
	else {
		ipt_write("--set-mss %d\n", rmtu - 40);
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

	ipt_write(
		"-A FORWARD -i %s -o %s -j ACCEPT\n",			// accept all lan to lan
		lanface, lanface);
	if (strcmp(lan1face,"")!=0)
		ipt_write(
			"-A FORWARD -i %s -o %s -j ACCEPT\n",
			lan1face, lan1face);
	if (strcmp(lan2face,"")!=0)
		ipt_write(
			"-A FORWARD -i %s -o %s -j ACCEPT\n",
			lan2face, lan2face);
	if (strcmp(lan3face,"")!=0)
		ipt_write(
			"-A FORWARD -i %s -o %s -j ACCEPT\n",
			lan3face, lan3face);
	ipt_write(
		"-A FORWARD -m state --state INVALID -j DROP\n");		// drop if INVALID state

	// clamp tcp mss to pmtu
	clampmss();

	if (wanup) {
		ipt_restrictions();
		ipt_layer7_inbound();
	}

	ipt_webmon(0);

	ipt_write(
		":wanin - [0:0]\n"
		":wanout - [0:0]\n"
		"-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n");	// already established or related (via helper)

	for (i = 0; i < wanfaces.count; ++i) {
		if (*(wanfaces.iface[i].name)) {
			ipt_write(
				"-A FORWARD -i %s -j wanin\n"			// generic from wan
				"-A FORWARD -o %s -j wanout\n",			// generic to wan
				wanfaces.iface[i].name, wanfaces.iface[i].name);
		}
	}

	for (i = 0; i < wanfaces.count; ++i) {
		if (*(wanfaces.iface[i].name)) {
			ipt_write("-A FORWARD -i %s -o %s -j %s\n", lanface, wanfaces.iface[i].name, chain_out_accept);
			if (strcmp(lan1face,"")!=0)
				ipt_write("-A FORWARD -i %s -o %s -j %s\n", lan1face, wanfaces.iface[i].name, chain_out_accept);
			if (strcmp(lan2face,"")!=0)
				ipt_write("-A FORWARD -i %s -o %s -j %s\n", lan2face, wanfaces.iface[i].name, chain_out_accept);
			if (strcmp(lan3face,"")!=0)
				ipt_write("-A FORWARD -i %s -o %s -j %s\n", lan3face, wanfaces.iface[i].name, chain_out_accept);
		}
	}

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
			if (!ipt_addr(src, sizeof(src), saddr, "src", AF_INET, "LAN access", desc))
				continue;
			if (!ipt_addr(dst, sizeof(dst), daddr, "dst", AF_INET, "LAN access", desc))
				continue;

			ipt_write("-A FORWARD -i %s%s -o %s%s %s %s -j ACCEPT\n",
				"br",
				sbr,
				"br",
				dbr,
				src,
				dst);
		}
	}
	free(nv);

	if (nvram_get_int("upnp_enable") & 3) {
		ipt_write(":upnp - [0:0]\n");
		for (i = 0; i < wanfaces.count; ++i) {
			if (*(wanfaces.iface[i].name)) {
				ipt_write("-A FORWARD -i %s -j upnp\n",
					wanfaces.iface[i].name);
			}
		}
	}

	if (wanup) {
		if (nvram_match("multicast_pass", "1")) {
			ipt_write("-A wanin -p udp -d 224.0.0.0/4 -j %s\n", chain_in_accept);
		}
		ipt_triggered(IPT_TABLE_FILTER);
		ipt_forward(IPT_TABLE_FILTER);

		if (dmz_dst(dst)) {
			strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL) *c = 0;
				if (ipt_source(p, src, "dmz", NULL))
					ipt_write("-A FORWARD -o %s %s -d %s -j %s\n", lanface, src, dst, chain_in_accept);
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
			"-A logdrop -m state --state NEW %s -j LOG --log-prefix \"DROP \" --log-tcp-options --log-ip-options\n"
			"-A logdrop -j DROP\n"
			":logreject - [0:0]\n"
			"-A logreject %s -j LOG --log-prefix \"REJECT \" --log-tcp-options --log-ip-options\n"
			"-A logreject -p tcp -j REJECT --reject-with tcp-reset\n",
			limit, limit);
	}
	if ((*chain_in_accept == 'l') || (*chain_out_accept == 'l'))  {
		ip46t_write(
			":logaccept - [0:0]\n"
			"-A logaccept -m state --state NEW %s -j LOG --log-prefix \"ACCEPT \" --log-tcp-options --log-ip-options\n"
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

	ip6t_write(
		"-A INPUT -m rt --rt-type 0 -j %s\n"
		/* "-A INPUT -m state --state INVALID -j %s\n" */
		"-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n",
		chain_in_drop/*, chain_in_drop*/);

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
	case IPV6_NATIVE_DHCP:
		// allow responses from the dhcpv6 server
		ip6t_write("-A INPUT -p udp --dport 546 -j %s\n", chain_in_accept);
		break;
	}

	// ICMPv6 rules
	const int allowed_icmpv6[6] = { 1, 2, 3, 4, 128, 129 };
	for (n = 0; n < sizeof(allowed_icmpv6)/sizeof(int); n++) {
		ip6t_write("-A INPUT -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_icmpv6[n], chain_in_accept);
	}

	// Remote Managment
	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		if (ip6t_source(p, s, "remote management", NULL)) {

			if (remotemanage) {
				ip6t_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
					s, nvram_safe_get("http_wanport"), chain_in_accept);
			}

			if (nvram_get_int("sshd_remote")) {
				ip6t_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
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
				ip6t_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
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

static void filter6_forward(void)
{
	int n;
	
	ip6t_write(
		"-A FORWARD -m rt --rt-type 0 -j DROP\n"
		"-A FORWARD -i %s -o %s -j ACCEPT\n"			// accept all lan to lan
		/*"-A FORWARD -m state --state INVALID -j DROP\n"*/,	// drop if INVALID state
		lanface, lanface);

	// Filter out invalid WAN->WAN connections
	if (*wan6face)
		ip6t_write("-A FORWARD -o %s ! -i %s -j %s\n", wan6face, lanface, chain_in_drop);

#ifdef LINUX26
	modprobe("xt_length");
	ip6t_write("-A FORWARD -p ipv6-nonxt -m length --length 40 -j ACCEPT\n");
#endif

	// clamp tcp mss to pmtu TODO?
	// clampmss();

	// TODO: support l7, access restrictions on ipv6?
/*
	if (wanup) {
		ipt_restrictions();
		ipt_layer7_inbound();
	}
*/
#ifdef LINUX26
	ipt_webmon(1);
#endif

	ip6t_write(
		":wanin - [0:0]\n"
		":wanout - [0:0]\n"
		"-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n");	// already established or related (via helper)

	if (*wan6face) {
		ip6t_write(
			"-A FORWARD -i %s -j wanin\n"				// generic from wan
			"-A FORWARD -o %s -j wanout\n",				// generic to wan
			wan6face, wan6face);
	}

	ip6t_write(
		"-A FORWARD -i %s -j %s\n",					// from lan
		lanface, chain_out_accept);

	// ICMPv6 rules
	const int allowed_icmpv6[6] = { 1, 2, 3, 4, 128, 129 };
	for (n = 0; n < sizeof(allowed_icmpv6)/sizeof(int); n++) {
		ip6t_write("-A FORWARD -p ipv6-icmp --icmpv6-type %i -j %s\n", allowed_icmpv6[n], chain_in_accept);
	}

	if (wanup) {
		//ipt_triggered(IPT_TABLE_FILTER);
		ip6t_forward();
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
#ifdef TCONFIG_IPV6
		filter6_forward();
#endif
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
	char *c, *wanface;
	int n;
	int wanproto;
	char *iptrestore_argv[] = { "iptables-restore", (char *)ipt_fname, NULL };
#ifdef TCONFIG_IPV6
	char *ip6trestore_argv[] = { "ip6tables-restore", (char *)ip6t_fname, NULL };
#endif

	simple_lock("firewall");
	simple_lock("restrictions");

	wanproto = get_wan_proto();
	wanup = check_wanup();

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

	memcpy(&wanfaces, get_wanfaces(), sizeof(wanfaces));
	wanface = wanfaces.iface[0].name;
#ifdef TCONFIG_IPV6
	strlcpy(wan6face, get_wan6face(), sizeof(wan6face));
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
	if (!(nvram_match("multicast_pass", "1")) || strcmp(wanface, c) == 0) c = NULL;

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
#endif

	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/load", NULL, 0, 0, 0);
		killall("miniupnpd", SIGUSR2);
	}

	simple_unlock("restrictions");
	sched_restrictions();
	enable_ip_forward();

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
	run_nvscript("script_fire", NULL, 1);

//	start_bwclimon();
	start_account();

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

