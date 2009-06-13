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

char wanface[IFNAMSIZ];
char lanface[IFNAMSIZ];
char lan_cclass[sizeof("xxx.xxx.xxx.")];
char wanaddr[sizeof("xxx.xxx.xxx.xxx")];
static int web_lanport;

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

const char ipt_fname[] = "/etc/iptables";
FILE *ipt_file;


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

static void ipt_source(const char *s, char *src)
{
	char p[32];

	if ((*s) && (strlen(s) < 32))
	{
		if (sscanf(s, "%[0-9.]-%[0-9.]", p, p) == 2)
			sprintf(src, "-m iprange --src-range %s", s);
		else
			sprintf(src, "-s %s", s);
	}
	else
		*src = 0;
}

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
	int en;
	char **p;

	if (!layer7_in) return;

	en = nvram_match("nf_l7in", "1");
	if (en) {
		ipt_write(
			":L7in - [0:0]\n"
			"-A FORWARD -i %s -j L7in\n",
				wanface);
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

	modprobe("ipt_layer7");
	return 1;
}



// -----------------------------------------------------------------------------
// MANGLE
// -----------------------------------------------------------------------------

static void mangle_table(void)
{
	int ttl;
	char *p;

	ipt_write(
		"*mangle\n"
		":PREROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n");

	if (wanup) {
		ipt_qos();

		ttl = nvram_get_int("nf_ttl");
		if (ttl != 0) {
			modprobe("ipt_TTL");
			if (ttl > 0) {
				p = "in";
			}
			else {
				ttl = -ttl;
				p = "de";
			}
			ipt_write(
				"-I PREROUTING -i %s -j TTL --ttl-%sc %d\n"
				"-I POSTROUTING -o %s -j TTL --ttl-%sc %d\n",
					wanface, p, ttl,
					wanface, p, ttl);
		}
	}

	ipt_write("COMMIT\n");
}



// -----------------------------------------------------------------------------
// NAT
// -----------------------------------------------------------------------------

in_addr_t _inet_addr(const char *cp)
{
	struct in_addr a;

	if (!inet_aton(cp, &a))
		return INADDR_ANY;
	else
		return a.s_addr;
}

static void nat_table(void)
{
	char lanaddr[32];
	char lanmask[32];
	char dst[64];
	char src[64];
	char t[512];
	char *p, *c;

	ipt_write("*nat\n"
		":PREROUTING ACCEPT [0:0]\n"
		":POSTROUTING ACCEPT [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n");
	if (gateway_mode) {
		strlcpy(lanaddr, nvram_safe_get("lan_ipaddr"), sizeof(lanaddr));
		strlcpy(lanmask, nvram_safe_get("lan_netmask"), sizeof(lanmask));

		// Drop incoming packets which destination IP address is to our LAN side directly
		ipt_write("-A PREROUTING -i %s -d %s/%s -j DROP\n",
			wanface,
			lanaddr, lanmask);	// note: ipt will correct lanaddr

		if (wanup) {
			if (nvram_match("dns_intcpt", "1")) {
				ipt_write("-A PREROUTING -p udp -s %s/%s ! -d %s/%s --dport 53 -j DNAT --to-destination %s\n",
					lanaddr, lanmask,
					lanaddr, lanmask,
					lanaddr);
			}

			// ICMP packets are always redirected to INPUT chains
			ipt_write("-A PREROUTING -p icmp -d %s -j DNAT --to-destination %s\n", wanaddr, lanaddr);


			strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL) *c = 0;
				ipt_source(p, src);

				if (remotemanage) {
					ipt_write("-A PREROUTING -p tcp -m tcp %s -d %s --dport %s -j DNAT --to-destination %s:%d\n",
						src,
						wanaddr, nvram_safe_get("http_wanport"),
						lanaddr, web_lanport);
				}
				if (nvram_get_int("sshd_remote")) {
					ipt_write("-A PREROUTING %s -p tcp -m tcp -d %s --dport %s -j DNAT --to-destination %s:%s\n",
						src,
						wanaddr, nvram_safe_get("sshd_rport"),
						lanaddr, nvram_safe_get("sshd_port"));
				}

				if (!c) break;
				p = c + 1;
			} while (*p);

			ipt_forward(IPT_TABLE_NAT);
			ipt_triggered(IPT_TABLE_NAT);
		}

#ifdef USE_MINIUPNPD
		if (nvram_get_int("upnp_enable") & 3) {
			ipt_write(":upnp - [0:0]\n");
			if (wanup) {
				// ! for loopback (all) to work
				ipt_write("-A PREROUTING -d %s -j upnp\n", wanaddr);
			}
			else {
				ipt_write("-A PREROUTING -i %s -j upnp\n", wanface);
			}
		}
#else
		if (nvram_get_int("upnp_enable")) {
			ipt_write(
				":upnp - [0:0]\n"
				"-A PREROUTING -i %s -j upnp\n",
					wanface);
		}
#endif

		if (wanup) {
			if (dmz_dst(dst)) {
				strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
				p = t;
				do {
					if ((c = strchr(p, ',')) != NULL) *c = 0;
					ipt_source(p, src);
					ipt_write("-A PREROUTING %s -d %s -j DNAT --to-destination %s\n", src, wanaddr, dst);
					if (!c) break;
					p = c + 1;
				} while (*p);
			}
		}



		////

		/* Using SNAT instead of MASQUERADE can speed up routing since
		 * SNAT does not seek the external IP every time a chain is traversed.
		 * Recommended to use with kernel patch to drop SNAT'ed connections
		 * on wanface down or wanip change.
		 */
		if (wanup && nvram_match("ne_snat", "1")) {
			if (_inet_addr(wanaddr))
				ipt_write("-A POSTROUTING -o %s ! -s %s -j SNAT --to-source %s\n",
					wanface, 
					wanaddr, wanaddr);
		
			/* SNAT physical WAN port connection */
			char *wanip = nvram_safe_get("wan_ipaddr");
			if (nvram_invmatch("wan_ifname", wanface) && _inet_addr(wanip))
				ipt_write("-A POSTROUTING -o %s ! -s %s -j SNAT --to-source %s\n",
					nvram_safe_get("wan_ifname"),
					wanip, wanip);
		}
		else {
			ipt_write("-A POSTROUTING -o %s -j MASQUERADE\n", wanface);
		}

		switch (nvram_get_int("nf_loopback")) {
		case 1:		// 1 = forwarded-only
		case 2:		// 2 = disable
			break;
		default:	// 0 = all (same as block_loopback=0)
			if (nvram_match("ne_snat", "1"))
				ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j SNAT --to-source %s\n",
					lanface,
					lanaddr, lanmask,
					lanaddr, lanmask,
					lanaddr);
			else
				ipt_write("-A POSTROUTING -o %s -s %s/%s -d %s/%s -j MASQUERADE\n",
					lanface,
					lanaddr, lanmask,
					lanaddr, lanmask);
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
		ipt_write("-A INPUT -i %s -d %s -j DROP\n", lanface, wanaddr);
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
		modprobe("ipt_recent");

		ipt_write(
			"-N shlimit\n"
			"-A shlimit -m recent --set --name shlimit\n"
			"-A shlimit -m recent --update --hitcount %s --seconds %s --name shlimit -j DROP\n",
			hit, sec);

		if (n & 1) ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("sshd_port"));
		if (n & 2) ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j shlimit\n", nvram_safe_get("telnetd_port"));
	}

#ifdef TCONFIG_FTP
	strlcpy(s, nvram_safe_get("ftp_limit"), sizeof(s));
	if ((vstrsep(s, ",", &en, &hit, &sec) == 3) && (atoi(en)) && (nvram_get_int("ftp_enable") == 1)) {
		modprobe("ipt_recent");

		ipt_write(
			"-N ftplimit\n"
			"-A ftplimit -m recent --set --name ftp\n"
			"-A ftplimit -m recent --update --hitcount %s --seconds %s --name ftp -j DROP\n",
			hit, sec);
		ipt_write("-A INPUT -p tcp --dport %s -m state --state NEW -j ftplimit\n", nvram_safe_get("ftp_port"));
	}
#endif

	ipt_write(
		"-A INPUT -i %s -j ACCEPT\n"
		"-A INPUT -i lo -j ACCEPT\n",
			lanface);

	// ICMP request from WAN interface
	if (nvram_match("block_wan", "0")) {
		ipt_write("-A INPUT -p icmp -j ACCEPT\n");
	}


	strlcpy(t, nvram_safe_get("rmgt_sip"), sizeof(t));
	p = t;
	do {
		if ((c = strchr(p, ',')) != NULL) *c = 0;

		ipt_source(p, s);

		if (remotemanage) {
			ipt_write("-A INPUT -p tcp %s -m tcp -d %s --dport %d -j %s\n",
				s, nvram_safe_get("lan_ipaddr"), web_lanport, chain_in_accept);
		}

		if (nvram_get_int("sshd_remote")) {
			ipt_write("-A INPUT -p tcp %s -m tcp -d %s --dport %s -j %s\n",
				s, nvram_safe_get("lan_ipaddr"), nvram_safe_get("sshd_port"), chain_in_accept);
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
			ipt_source(p, s);

			ipt_write("-A INPUT -p tcp %s -m tcp --dport %s -j %s\n",
				s, nvram_safe_get("ftp_port"), chain_in_accept);

			if (!c) break;
			p = c + 1;
		} while (*p);
	}
#endif

	// IGMP query from WAN interface
	if (nvram_match("multicast_pass", "1")) {
		ipt_write("-A INPUT -p igmp -j ACCEPT\n");
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
	int rmtu = nvram_get_int("wan_run_mtu");

	ipt_write("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss %d: -j TCPMSS ", rmtu - 39);
	if (rmtu < 576) {
		ipt_write("--clamp-mss-to-pmtu\n");
	}
	else {
		ipt_write("--set-mss %d\n", rmtu - 40);
	}
}

static void filter_forward(void)
{
	char dst[64];
	char src[64];
	char t[512];
	char *p, *c;

	ipt_write(
		"-A FORWARD -i %s -o %s -j ACCEPT\n"				// accept all lan to lan
		"-A FORWARD -m state --state INVALID -j DROP\n",	// drop if INVALID state
		lanface, lanface);

	// clamp tcp mss to pmtu
	clampmss();

	if (wanup) {
		ipt_restrictions();
		ipt_layer7_inbound();
	}

	ipt_write(
		":wanin - [0:0]\n"
		":wanout - [0:0]\n"
		"-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n"	// already established or related (via helper)
		"-A FORWARD -i %s -j wanin\n"									// generic from wan
		"-A FORWARD -o %s -j wanout\n"									// generic to wan
		"-A FORWARD -i %s -j %s\n",										// from lan
		wanface, wanface, lanface, chain_out_accept);

#ifdef USE_MINIUPNPD
	if (nvram_get_int("upnp_enable") & 3) {
		ipt_write(
			":upnp - [0:0]\n"
			"-A FORWARD -i %s -j upnp\n",
			wanface);
	}
#else
	if (nvram_get_int("upnp_enable")) {
		ipt_write(
			":upnp - [0:0]\n"
			"-A FORWARD -i %s -j upnp\n",
				wanface);
	}
#endif

	if (wanup) {
		if (nvram_match("multicast_pass", "1")) {
			ipt_write("-A wanin -p udp -m udp -d 224.0.0.0/4 -j %s\n", chain_in_accept);
		}
		ipt_triggered(IPT_TABLE_FILTER);
		ipt_forward(IPT_TABLE_FILTER);

		if (dmz_dst(dst)) {
			strlcpy(t, nvram_safe_get("dmz_sip"), sizeof(t));
			p = t;
			do {
				if ((c = strchr(p, ',')) != NULL) *c = 0;
				ipt_source(p, src);
				ipt_write("-A FORWARD -o %s %s -d %s -j %s\n", lanface, src, dst, chain_in_accept);
				if (!c) break;
				p = c + 1;
			} while (*p);
		}
	}


	// default policy: DROP
}

static void filter_table(void)
{
	int n;
	char limit[128];

	ipt_write(
		"*filter\n"
		":INPUT DROP [0:0]\n"
		":OUTPUT ACCEPT [0:0]\n"
	);

	n = nvram_get_int("log_limit");
	if ((n >= 1) && (n <= 9999)) {
		sprintf(limit, "-m limit --limit %d/m", n);
	}
	else {
		limit[0] = 0;
	}

	if ((*chain_in_drop == 'l') || (*chain_out_drop == 'l'))  {
		ipt_write(
			":logdrop - [0:0]\n"
			"-A logdrop -m state --state NEW %s -j LOG --log-prefix \"DROP \" --log-tcp-options --log-ip-options\n"
			"-A logdrop -j DROP\n"
			":logreject - [0:0]\n"
			"-A logreject %s -j LOG --log-prefix \"REJECT \" --log-tcp-options --log-ip-options\n"
			"-A logreject -p tcp -j REJECT --reject-with tcp-reset\n",
			limit, limit);
	}
	if ((*chain_in_accept == 'l') || (*chain_out_accept == 'l'))  {
		ipt_write(
			":logaccept - [0:0]\n"
			"-A logaccept -m state --state NEW %s -j LOG --log-prefix \"ACCEPT \" --log-tcp-options --log-ip-options\n"
			"-A logaccept -j ACCEPT\n",
			limit);
	}

	filter_input();

	if ((gateway_mode) || (nvram_match("wk_mode_x", "1"))) {
		ipt_write(":FORWARD DROP [0:0]\n");
		filter_forward();
	}
	else {
		ipt_write(":FORWARD ACCEPT [0:0]\n");
		clampmss();
	}
	ipt_write("COMMIT\n");
}


// -----------------------------------------------------------------------------

int start_firewall(void)
{
	DIR *dir;
	struct dirent *dirent;
	char s[256];
	char *c;
	int n;
	int wanproto;

	simple_lock("firewall");
	simple_lock("restrictions");

	wanproto = get_wan_proto();
	wanup = check_wanup();


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
	if ((dir = opendir("/proc/sys/net/ipv4/conf")) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			sprintf(s, "/proc/sys/net/ipv4/conf/%s/rp_filter", dirent->d_name);
			f_write_string(s, "1", 0, 0);
		}
		closedir(dir);
	}

	f_write_string("/proc/sys/net/ipv4/tcp_syncookies", nvram_get_int("ne_syncookies") ? "1" : "0", 0, 0);

	n = nvram_get_int("log_in");
	chain_in_drop = (n & 1) ? "logdrop" : "DROP";
	chain_in_accept = (n & 2) ? "logaccept" : "ACCEPT";

	n = nvram_get_int("log_out");
	chain_out_drop = (n & 1) ? "logdrop" : "DROP";
	chain_out_reject = (n & 1) ? "logreject" : "REJECT --reject-with tcp-reset";
	chain_out_accept = (n & 2) ? "logaccept" : "ACCEPT";

//	if (nvram_match("nf_drop_reset", "1")) chain_out_drop = chain_out_reject;

	strlcpy(lanface, nvram_safe_get("lan_ifname"), IFNAMSIZ);

	if ((wanproto == WP_PPTP) || (wanproto == WP_L2TP) || (wanproto == WP_PPPOE)) {
		strcpy(wanface, "ppp+");
	}
	else {
		strlcpy(wanface, nvram_safe_get("wan_ifname"), sizeof(wanface));
	}

	strlcpy(wanaddr, get_wanip(), sizeof(wanaddr));

	strlcpy(s, nvram_safe_get("lan_ipaddr"), sizeof(s));
	if ((c = strrchr(s, '.')) != NULL) *(c + 1) = 0;
	strlcpy(lan_cclass, s, sizeof(lan_cclass));

	gateway_mode = !nvram_match("wk_mode", "router");
	if (gateway_mode) {
		/* Remote management */
		if (nvram_match("remote_management", "1") && nvram_invmatch("http_wanport", "") &&
			nvram_invmatch("http_wanport", "0")) remotemanage = 1;
				else remotemanage = 0;

		if (nvram_match("remote_mgt_https", "1")) {
			web_lanport = nvram_get_int("https_lanport");
			if (web_lanport <= 0) web_lanport = 443;
		}
		else {
			web_lanport = nvram_get_int("http_lanport");
			if (web_lanport <= 0) web_lanport = 80;
		}
	}


	if ((ipt_file = fopen(ipt_fname, "w")) == NULL) {
		syslog(LOG_CRIT, "Unable to create iptables restore file");
		simple_unlock("firewall");
		return 0;
	}

	mangle_table();
	nat_table();
	filter_table();

	fclose(ipt_file);
	ipt_file = NULL;

#ifdef DEBUG_IPTFILE
	if (debug_only) {
		simple_unlock("firewall");
		simple_unlock("restrictions");
		return 0;
	}
#endif

#ifdef USE_MINIUPNPD
	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/save", NULL, 0, 0, 0);
		if (killall("miniupnpd", SIGUSR2) == 0) {
			f_wait_notexists("/etc/upnp/save", 5);
		}
	}
#endif

	if (eval("iptables-restore", (char *)ipt_fname) == 0) {
		led(LED_DIAG, 0);
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

#ifdef USE_MINIUPNPD
	if (nvram_get_int("upnp_enable") & 3) {
		f_write("/etc/upnp/load", NULL, 0, 0, 0);
		killall("miniupnpd", SIGUSR2);
	}
#else
	if (nvram_get_int("upnp_enable")) {
		killall("upnp", SIGHUP);
	}
#endif

	simple_unlock("restrictions");
	sched_restrictions();
	enable_ip_forward();

	led(LED_DMZ, dmz_dst(NULL));

	modprobe_r("ipt_layer7");
	modprobe_r("ipt_ipp2p");
	modprobe_r("ipt_web");
	modprobe_r("ipt_TTL");

	run_nvscript("script_fire", NULL, 1);

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

