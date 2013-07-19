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

#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

// !!TB
#include <sys/mount.h>
#include <mntent.h>
#include <dirent.h>

// Pop an alarm to recheck pids in 500 msec.
static const struct itimerval pop_tv = { {0,0}, {0, 500 * 1000} };

// Pop an alarm to reap zombies. 
static const struct itimerval zombie_tv = { {0,0}, {307, 0} };

// -----------------------------------------------------------------------------

static const char dmhosts[] = "/etc/dnsmasq/hosts";
static const char dmdhcp[] = "/etc/dnsmasq/dhcp";
static const char dmresolv[] = "/etc/resolv.dnsmasq";

static pid_t pid_dnsmasq = -1;

static int is_wet(int idx, int unit, int subunit, void *param)
{
	return nvram_match(wl_nvname("mode", unit, subunit), "wet");
}

void start_dnsmasq()
{
	FILE *f;
	const char *nv;
	char buf[512];
	char lan[24];
	const char *router_ip;
	char sdhcp_lease[32];
	char *e;
	int n;
	char *mac, *ip, *name;
	char *p;
	int ipn;
	char ipbuf[32];
	FILE *hf, *df;
	int dhcp_start;
	int dhcp_count;
	int dhcp_lease;
	int do_dhcpd;
	int do_dns;
	int do_dhcpd_hosts;

#ifdef TCONFIG_IPV6
	char *prefix, *ipv6, *mtu;
	int do_6to4, do_6rd;
	int service;
#endif

	TRACE_PT("begin\n");

	if (getpid() != 1) {
		start_service("dnsmasq");
		return;
	}

	stop_dnsmasq();

	if (foreach_wif(1, NULL, is_wet)) return;

	if ((f = fopen("/etc/dnsmasq.conf", "w")) == NULL) return;

	router_ip = nvram_safe_get("lan_ipaddr");

	fprintf(f,
		"pid-file=/var/run/dnsmasq.pid\n");
	if (((nv = nvram_get("wan_domain")) != NULL) || ((nv = nvram_get("wan_get_domain")) != NULL)) {
		if (*nv) fprintf(f, "domain=%s\n", nv);
	}

	// dns
	const dns_list_t *dns = get_dns();	// this always points to a static buffer

	if (((nv = nvram_get("dns_minport")) != NULL) && (*nv)) n = atoi(nv);
		else n = 4096;
	fprintf(f,
		"resolv-file=%s\n"		// the real stuff is here
		"addn-hosts=%s\n"		// directory with additional hosts files
		"dhcp-hostsfile=%s\n"		// directory with dhcp hosts files
		"expand-hosts\n"		// expand hostnames in hosts file
		"min-port=%u\n", 		// min port used for random src port
		dmresolv, dmhosts, dmdhcp, n);
	do_dns = nvram_match("dhcpd_dmdns", "1");

	// DNS rebinding protection, will discard upstream RFC1918 responses
	if (nvram_get_int("dns_norebind")) {
		fprintf(f,
			"stop-dns-rebind\n"
			"rebind-localhost-ok\n");
		// allow RFC1918 responses for server domain
		switch (get_wan_proto()) {
		case WP_PPTP:
			nv = nvram_get("pptp_server_ip");
			break;
		case WP_L2TP:
			nv = nvram_get("l2tp_server_ip");
			break;
		default:
			nv = NULL;
			break;
		}
		if (nv && *nv) fprintf(f, "rebind-domain-ok=%s\n", nv);
	}

#ifdef TCONFIG_DNSCRYPT
	if (nvram_match("dnscrypt_proxy", "1")) {
		fprintf(f, "server=127.0.0.1#%s\n", nvram_safe_get("dnscrypt_port") );
	}
#endif

	for (n = 0 ; n < dns->count; ++n) {
		if (dns->dns[n].port != 53) {
			fprintf(f, "server=%s#%u\n", inet_ntoa(dns->dns[n].addr), dns->dns[n].port);
		}
	}

	if (nvram_get_int("dhcpd_static_only")) {
		fprintf(f, "dhcp-ignore=tag:!known\n");
	}

	if ((n = nvram_get_int("dnsmasq_q"))) { //process quiet flags
		if (n & 1) fprintf(f, "quiet-dhcp\n");
		if (n & 2) fprintf(f, "quiet-dhcp6\n");
		if (n & 4) fprintf(f, "quiet-ra\n");
	}

	// dhcp
	do_dhcpd_hosts=0;
	char lanN_proto[] = "lanXX_proto";
	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ipaddr[] = "lanXX_ipaddr";
	char lanN_netmask[] = "lanXX_netmask";
	char dhcpdN_startip[] = "dhcpdXX_startip";
	char dhcpdN_endip[] = "dhcpdXX_endip";
	char dhcpN_start[] = "dhcpXX_start";
	char dhcpN_num[] = "dhcpXX_num";
	char dhcpN_lease[] = "dhcpXX_lease";
	char br;
	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(lanN_proto, "lan%s_proto", bridge);
		sprintf(lanN_ifname, "lan%s_ifname", bridge);
		sprintf(lanN_ipaddr, "lan%s_ipaddr", bridge);
		do_dhcpd = nvram_match(lanN_proto, "dhcp");
		if (do_dhcpd) {
			do_dhcpd_hosts++;

			router_ip = nvram_safe_get(lanN_ipaddr);
			strlcpy(lan, router_ip, sizeof(lan));
			if ((p = strrchr(lan, '.')) != NULL) *(p + 1) = 0;

			fprintf(f,
				"interface=%s\n",
				nvram_safe_get(lanN_ifname));

			sprintf(dhcpN_lease, "dhcp%s_lease", bridge);
			dhcp_lease = nvram_get_int(dhcpN_lease);

			if (dhcp_lease <= 0) dhcp_lease = 1440;

			if ((e = nvram_get("dhcpd_slt")) != NULL) n = atoi(e); else n = 0;
			if (n < 0) strcpy(sdhcp_lease, "infinite");
				else sprintf(sdhcp_lease, "%dm", (n > 0) ? n : dhcp_lease);

			if (!do_dns) {
				// if not using dnsmasq for dns

				if ((dns->count == 0) && (nvram_get_int("dhcpd_llndns"))) {
					// no DNS might be temporary. use a low lease time to force clients to update.
					dhcp_lease = 2;
					strcpy(sdhcp_lease, "2m");
					do_dns = 1;
				}
				else {
					// pass the dns directly
					buf[0] = 0;
					for (n = 0 ; n < dns->count; ++n) {
						if (dns->dns[n].port == 53) {	// check: option 6 doesn't seem to support other ports
							sprintf(buf + strlen(buf), ",%s", inet_ntoa(dns->dns[n].addr));
						}
					}
					fprintf(f, "dhcp-option=tag:%s,6%s\n", nvram_safe_get(lanN_ifname), buf);
				}
			}

			sprintf(dhcpdN_startip, "dhcpd%s_startip", bridge);
			sprintf(dhcpdN_endip, "dhcpd%s_endip", bridge);
			sprintf(lanN_netmask, "lan%s_netmask", bridge);

			if ((p = nvram_get(dhcpdN_startip)) && (*p) && (e = nvram_get(dhcpdN_endip)) && (*e)) {
				fprintf(f, "dhcp-range=tag:%s,%s,%s,%s,%dm\n", nvram_safe_get(lanN_ifname), p, e, nvram_safe_get(lanN_netmask), dhcp_lease);
			}
			else {
				// for compatibility
				sprintf(dhcpN_start, "dhcp%s_start", bridge);
				sprintf(dhcpN_num, "dhcp%s_num", bridge);
				sprintf(lanN_netmask, "lan%s_netmask", bridge);
				dhcp_start = nvram_get_int(dhcpN_start);
				dhcp_count = nvram_get_int(dhcpN_num);
				fprintf(f, "dhcp-range=tag:%s,%s%d,%s%d,%s,%dm\n",
					nvram_safe_get(lanN_ifname), lan, dhcp_start, lan, dhcp_start + dhcp_count - 1, nvram_safe_get(lanN_netmask), dhcp_lease);
			}

			nv = nvram_safe_get(lanN_ipaddr);
			if ((nvram_get_int("dhcpd_gwmode") == 1) && (get_wan_proto() == WP_DISABLED)) {
				p = nvram_safe_get("lan_gateway");
				if ((*p) && (strcmp(p, "0.0.0.0") != 0)) nv = p;
			}

			fprintf(f,
				"dhcp-option=tag:%s,3,%s\n",	// gateway
				nvram_safe_get(lanN_ifname), nv);

			if (((nv = nvram_get("wan_wins")) != NULL) && (*nv) && (strcmp(nv, "0.0.0.0") != 0)) {
				fprintf(f, "dhcp-option=tag:%s,44,%s\n", nvram_safe_get(lanN_ifname), nv);
			}
#ifdef TCONFIG_SAMBASRV
			else if (nvram_get_int("smbd_enable") && nvram_invmatch("lan_hostname", "") && nvram_get_int("smbd_wins")) {
				if ((nv == NULL) || (*nv == 0) || (strcmp(nv, "0.0.0.0") == 0)) {
					// Samba will serve as a WINS server
					fprintf(f, "dhcp-option=tag:%s,44,%s\n", nvram_safe_get(lanN_ifname), nvram_safe_get(lanN_ipaddr));
				}
			}
#endif
		} else {
			if (strcmp(nvram_safe_get(lanN_ifname),"")!=0) {
				fprintf(f, "interface=%s\n", nvram_safe_get(lanN_ifname));
// if no dhcp range is set then no dhcp service will be offered so following
// line is superflous.
//				fprintf(f, "no-dhcp-interface=%s\n", nvram_safe_get(lanN_ifname));
			}
		}
	}
	// write static lease entries & create hosts file

	mkdir_if_none(dmhosts);
	snprintf(buf, sizeof(buf), "%s/hosts", dmhosts);
	if ((hf = fopen(buf, "w")) != NULL) {
		if (((nv = nvram_get("wan_hostname")) != NULL) && (*nv))
			fprintf(hf, "%s %s\n", router_ip, nv);
#ifdef TCONFIG_SAMBASRV
		else if (((nv = nvram_get("lan_hostname")) != NULL) && (*nv))
			fprintf(hf, "%s %s\n", router_ip, nv);
#endif
		p = (char *)get_wanip();
		if ((*p == 0) || strcmp(p, "0.0.0.0") == 0)
			p = "127.0.0.1";
		fprintf(hf, "%s wan-ip\n", p);
		if (nv && (*nv))
			fprintf(hf, "%s %s-wan\n", p, nv);
	}

	mkdir_if_none(dmdhcp);
	snprintf(buf, sizeof(buf), "%s/dhcp-hosts", dmdhcp);
	df = fopen(buf, "w");

	// PREVIOUS/OLD FORMAT:
	// 00:aa:bb:cc:dd:ee<123<xxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 53 w/ delim
	// 00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 85 w/ delim
	// 00:aa:bb:cc:dd:ee,00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 106 w/ delim

	// NEW FORMAT (+static ARP binding after hostname):
	// 00:aa:bb:cc:dd:ee<123<xxxxxxxxxxxxxxxxxxxxxxxxxx.xyz<a> = 55 w/ delim
	// 00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz<a> = 87 w/ delim
	// 00:aa:bb:cc:dd:ee,00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz<a> = 108 w/ delim

	p = nvram_safe_get("dhcpd_static");
	while ((e = strchr(p, '>')) != NULL) {
		n = (e - p);
		if (n > 107) {
			p = e + 1;
			continue;
		}

		strncpy(buf, p, n);
		buf[n] = 0;
		p = e + 1;

		if ((e = strchr(buf, '<')) == NULL) continue;
		*e = 0;
		mac = buf;

		ip = e + 1;
		if ((e = strchr(ip, '<')) == NULL) continue;
		*e = 0;
		if (strchr(ip, '.') == NULL) {
			ipn = atoi(ip);
			if ((ipn <= 0) || (ipn > 255)) continue;
			sprintf(ipbuf, "%s%d", lan, ipn);
			ip = ipbuf;
		}
		else {
			if (inet_addr(ip) == INADDR_NONE) continue;
		}

		name = e + 1;

		if ((e = strchr(name, '<')) != NULL) {
			*e = 0;
		}

		if ((hf) && (*name != 0)) {
			fprintf(hf, "%s %s\n", ip, name);
		}

		if ((do_dhcpd_hosts > 0) && (*mac != 0) && (strcmp(mac, "00:00:00:00:00:00") != 0)) {
			if (nvram_get_int("dhcpd_slt") == 0) {
				fprintf(f, "dhcp-host=%s,%s\n", mac, ip);
			} else {
				fprintf(f, "dhcp-host=%s,%s,%s\n", mac, ip, sdhcp_lease);
			}
		}
	}

	if (df) fclose(df);
	if (hf) fclose(hf);

	n = nvram_get_int("dhcpd_lmax");
	fprintf(f,
		"dhcp-lease-max=%d\n",
		(n > 0) ? n : 255);
	if (nvram_get_int("dhcpd_auth") >= 0) {
		fprintf(f, "dhcp-authoritative\n");
	}

#ifdef TCONFIG_DNSCRYPT
	if (nvram_match("dnscrypt_proxy", "1")) {
		fprintf(f, "strict-order\n");
	}
#endif

	//

#ifdef TCONFIG_OPENVPN
	write_vpn_dnsmasq_config(f);
#endif

#ifdef TCONFIG_PPTPD
	write_pptpd_dnsmasq_config(f);
#endif

#ifdef TCONFIG_IPV6
	if (ipv6_enabled() && nvram_get_int("ipv6_radvd")) {
                service = get_ipv6_service();
                do_6to4 = (service == IPV6_ANYCAST_6TO4);
                do_6rd = (service == IPV6_6RD || service == IPV6_6RD_DHCP);
                mtu = NULL;

                switch (service) {
                case IPV6_NATIVE_DHCP:
                case IPV6_ANYCAST_6TO4:
                case IPV6_6IN4:
                case IPV6_6RD:
                case IPV6_6RD_DHCP:
                        mtu = (nvram_get_int("ipv6_tun_mtu") > 0) ? nvram_safe_get("ipv6_tun_mtu") : "1480";
                        // fall through
                default:
                        prefix = do_6to4 ? "0:0:0:1::" : nvram_safe_get("ipv6_prefix");
                        break;
                }
                if (!(*prefix)) prefix = "::";
                ipv6 = (char *)ipv6_router_address(NULL);

		fprintf(f, "enable-ra\ndhcp-range=tag:br0,%s, slaac, ra-names, 64\n", prefix);

	}
#endif

	fprintf(f, "%s\n\n", nvram_safe_get("dnsmasq_custom"));

	fappend(f, "/etc/dnsmasq.custom");

	//

	fclose(f);

	if (do_dns) {
		unlink("/etc/resolv.conf");
		symlink("/rom/etc/resolv.conf", "/etc/resolv.conf");	// nameserver 127.0.0.1
	}

	TRACE_PT("run dnsmasq\n");

	// Default to some values we like, but allow the user to override them.
	eval("dnsmasq", "-c", "1500", "--log-async");

	if (!nvram_contains_word("debug_norestart", "dnsmasq")) {
		pid_dnsmasq = -2;
	}

	TRACE_PT("end\n");

#ifdef TCONFIG_DNSCRYPT
	//start dnscrypt-proxy
	if (nvram_match("dnscrypt_proxy", "1")) {
		char dnscrypt_local[30];
		sprintf(dnscrypt_local, "127.0.0.1:%s", nvram_safe_get("dnscrypt_port") );

		eval("ntp2ip");
		eval("dnscrypt-proxy", "-d", "-a", dnscrypt_local, nvram_safe_get("dnscrypt_cmd") );

#ifdef TCONFIG_IPV6
		char dnscrypt_local_ipv6[30];
		sprintf(dnscrypt_local_ipv6, "::1:%s", nvram_safe_get("dnscrypt_port") );

		if (get_ipv6_service() != NULL) //if ipv6 enabled
			eval("dnscrypt-proxy", "-d", "-a", dnscrypt_local_ipv6, nvram_safe_get("dnscrypt_cmd") );
#endif
	}
#endif

}

void stop_dnsmasq(void)
{
	TRACE_PT("begin\n");

	if (getpid() != 1) {
		stop_service("dnsmasq");
		return;
	}

	pid_dnsmasq = -1;

	unlink("/etc/resolv.conf");
	symlink(dmresolv, "/etc/resolv.conf");

	killall_tk("dnsmasq");
#ifdef TCONFIG_DNSCRYPT
	killall_tk("dnscrypt-proxy");
#endif

	TRACE_PT("end\n");
}

void clear_resolv(void)
{
	f_write(dmresolv, NULL, 0, 0, 0);	// blank
}

#ifdef TCONFIG_IPV6
static int write_ipv6_dns_servers(FILE *f, const char *prefix, char *dns, const char *suffix, int once)
{
	char p[INET6_ADDRSTRLEN + 1], *next = NULL;
	struct in6_addr addr;
	int cnt = 0;

	foreach(p, dns, next) {
		// verify that this is a valid IPv6 address
		if (inet_pton(AF_INET6, p, &addr) == 1) {
			fprintf(f, "%s%s%s", (once && cnt) ? "" : prefix, p, suffix);
			++cnt;
		}
	}

	return cnt;
}
#endif

void dns_to_resolv(void)
{
	FILE *f;
	const dns_list_t *dns;
	int i;
	mode_t m;

	m = umask(022);	// 077 from pppoecd
	if ((f = fopen(dmresolv, "w")) != NULL) {
		// Check for VPN DNS entries
		if (!write_pptpvpn_resolv(f) && !write_vpn_resolv(f)) {
#ifdef TCONFIG_IPV6
			if (write_ipv6_dns_servers(f, "nameserver ", nvram_safe_get("ipv6_dns"), "\n", 0) == 0 || nvram_get_int("dns_addget"))
				write_ipv6_dns_servers(f, "nameserver ", nvram_safe_get("ipv6_get_dns"), "\n", 0);
#endif
			dns = get_dns();	// static buffer
			if (dns->count == 0) {
				// Put a pseudo DNS IP to trigger Connect On Demand
				if (nvram_match("ppp_demand", "1")) {
					switch (get_wan_proto()) {
					case WP_PPPOE:
					case WP_PPP3G:
					case WP_PPTP:
					case WP_L2TP:
						fprintf(f, "nameserver 1.1.1.1\n");
						break;
					}
				}
			}
			else {
				for (i = 0; i < dns->count; i++) {
					if (dns->dns[i].port == 53) {	// resolv.conf doesn't allow for an alternate port
						fprintf(f, "nameserver %s\n", inet_ntoa(dns->dns[i].addr));
					}
				}
			}
		}
		fclose(f);
	}
	umask(m);
}

// -----------------------------------------------------------------------------

void start_httpd(void)
{
	if (getpid() != 1) {
		start_service("httpd");
		return;
	}

	if( nvram_match( "web_css", "online" ) )
		xstart( "/usr/sbin/ttb" );

	stop_httpd();
	chdir("/www");
	eval("httpd");
	chdir("/");
}

void stop_httpd(void)
{
	if (getpid() != 1) {
		stop_service("httpd");
		return;
	}

	killall_tk("httpd");
}

// -----------------------------------------------------------------------------
#ifdef TCONFIG_IPV6

static void add_ip6_lanaddr(void)
{
	char ip[INET6_ADDRSTRLEN + 4];
	const char *p;

	p = ipv6_router_address(NULL);
	if (*p) {
		snprintf(ip, sizeof(ip), "%s/%d", p, nvram_get_int("ipv6_prefix_length") ? : 64);
		eval("ip", "-6", "addr", "add", ip, "dev", nvram_safe_get("lan_ifname"));
	}
}

void start_ipv6_tunnel(void)
{
	char ip[INET6_ADDRSTRLEN + 4];
	struct in_addr addr4;
	struct in6_addr addr;
	const char *wanip, *mtu, *tun_dev;
	int service;

	service = get_ipv6_service();
	tun_dev = get_wan6face();
	wanip = get_wanip();
	mtu = (nvram_get_int("ipv6_tun_mtu") > 0) ? nvram_safe_get("ipv6_tun_mtu") : "1480";
	modprobe("sit");

	if (service == IPV6_ANYCAST_6TO4)
		snprintf(ip, sizeof(ip), "192.88.99.%d", nvram_get_int("ipv6_relay"));
	else
		strlcpy(ip, (char *)nvram_safe_get("ipv6_tun_v4end"), sizeof(ip));
	eval("ip", "tunnel", "add", (char *)tun_dev, "mode", "sit",
		"remote", ip,
		"local", (char *)wanip,
		"ttl", nvram_safe_get("ipv6_tun_ttl"));

	eval("ip", "link", "set", (char *)tun_dev, "mtu", (char *)mtu, "up");
	nvram_set("ipv6_ifname", (char *)tun_dev);

	if (service == IPV6_ANYCAST_6TO4) {
		add_ip6_lanaddr();
		addr4.s_addr = 0;
		memset(&addr, 0, sizeof(addr));
		inet_aton(wanip, &addr4);
		addr.s6_addr16[0] = htons(0x2002);
		ipv6_mapaddr4(&addr, 16, &addr4, 0);
		addr.s6_addr16[7] = htons(0x0001);
		inet_ntop(AF_INET6, &addr, ip, sizeof(ip));
		strncat(ip, "/16", sizeof(ip));
	}
	else {
		snprintf(ip, sizeof(ip), "%s/%d",
			nvram_safe_get("ipv6_tun_addr"),
			nvram_get_int("ipv6_tun_addrlen") ? : 64);
	}
	eval("ip", "addr", "add", ip, "dev", (char *)tun_dev);
	eval("ip", "route", "add", "::/0", "dev", (char *)tun_dev);

	// (re)start radvd - now dnsmasq provided
	if (service == IPV6_ANYCAST_6TO4)
		start_dnsmasq();
}

void stop_ipv6_tunnel(void)
{
	eval("ip", "tunnel", "del", (char *)get_wan6face());
	if (get_ipv6_service() == IPV6_ANYCAST_6TO4) {
		// get rid of old IPv6 address from lan iface
		eval("ip", "-6", "addr", "flush", "dev", nvram_safe_get("lan_ifname"), "scope", "global");
	}
	modprobe_r("sit");
}

void start_6rd_tunnel(void)
{
	const char *tun_dev, *wanip;
	int service, mask_len, prefix_len, local_prefix_len;
	char mtu[10], prefix[INET6_ADDRSTRLEN], relay[INET_ADDRSTRLEN];
	struct in_addr netmask_addr, relay_addr, relay_prefix_addr, wanip_addr;
	struct in6_addr prefix_addr, local_prefix_addr;
	char local_prefix[INET6_ADDRSTRLEN];
	char tmp_ipv6[INET6_ADDRSTRLEN + 4], tmp_ipv4[INET_ADDRSTRLEN + 4];
	char tmp[256];
	FILE *f;

	service = get_ipv6_service();
	wanip = get_wanip();
	tun_dev = get_wan6face();
	sprintf(mtu, "%d", (nvram_get_int("wan_mtu") > 0) ? (nvram_get_int("wan_mtu") - 20) : 1280);

	// maybe we can merge the ipv6_6rd_* variables into a single ipv_6rd_string (ala wan_6rd)
	// to save nvram space?
	if (service == IPV6_6RD) {
		_dprintf("starting 6rd tunnel using manual settings.\n");
		mask_len = nvram_get_int("ipv6_6rd_ipv4masklen");
		prefix_len = nvram_get_int("ipv6_6rd_prefix_length");
		strcpy(prefix, nvram_safe_get("ipv6_6rd_prefix"));
		strcpy(relay, nvram_safe_get("ipv6_6rd_borderrelay"));
	}
	else {
		_dprintf("starting 6rd tunnel using automatic settings.\n");
		char *wan_6rd = nvram_safe_get("wan_6rd");
		if (sscanf(wan_6rd, "%d %d %s %s", &mask_len,  &prefix_len, prefix, relay) < 4) {
			_dprintf("wan_6rd string is missing or invalid (%s)\n", wan_6rd);
			return;
		}
	}

	// validate values that were passed
	if (mask_len < 0 || mask_len > 32) {
		_dprintf("invalid mask_len value (%d)\n", mask_len);
		return;
	}
	if (prefix_len < 0 || prefix_len > 128) {
		_dprintf("invalid prefix_len value (%d)\n", prefix_len);
		return;
	}
	if ((32 - mask_len) + prefix_len > 128) {
		_dprintf("invalid combination of mask_len and prefix_len!\n");
		return;
	}

	sprintf(tmp, "ping -q -c 2 %s | grep packet", relay);
	if ((f = popen(tmp, "r")) == NULL) {
		_dprintf("error obtaining data\n");
		return;
	}
	fgets(tmp, sizeof(tmp), f);
	pclose(f);
	if (strstr(tmp, " 0% packet loss") == NULL) {
		_dprintf("failed to ping border relay\n");
		return;
	}

	// get relay prefix from border relay address and mask
	netmask_addr.s_addr = htonl(0xffffffff << (32 - mask_len));
	inet_aton(relay, &relay_addr);
	relay_prefix_addr.s_addr = relay_addr.s_addr & netmask_addr.s_addr;

	// calculate the local prefix
	inet_pton(AF_INET6, prefix, &prefix_addr);
	inet_pton(AF_INET, wanip, &wanip_addr);
	if (calc_6rd_local_prefix(&prefix_addr, prefix_len, mask_len,
	    &wanip_addr, &local_prefix_addr, &local_prefix_len) == 0) {
		_dprintf("error calculating local prefix.");
		return;
	}
	inet_ntop(AF_INET6, &local_prefix_addr, local_prefix, sizeof(local_prefix));

	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s1", local_prefix);
	nvram_set("ipv6_rtr_addr", tmp_ipv6);
	nvram_set("ipv6_prefix", local_prefix);

	// load sit module needed for the 6rd tunnel
	modprobe("sit");

	// creating the 6rd tunnel
	eval("ip", "tunnel", "add", (char *)tun_dev, "mode", "sit", "local", (char *)wanip, "ttl", nvram_safe_get("ipv6_tun_ttl"));

	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s/%d", prefix, prefix_len);
	snprintf(tmp_ipv4, sizeof(tmp_ipv4), "%s/%d", inet_ntoa(relay_prefix_addr), mask_len);
	eval("ip", "tunnel" "6rd", "dev", (char *)tun_dev, "6rd-prefix", tmp_ipv6, "6rd-relay_prefix", tmp_ipv4);

	// bringing up the link
	eval("ip", "link", "set", "dev", (char *)tun_dev, "mtu", (char *)mtu, "up");

	// setting the WAN address Note: IPv6 WAN CIDR should be: ((32 - ip6rd_ipv4masklen) + ip6rd_prefixlen)
	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s1/%d", local_prefix, local_prefix_len);
	eval("ip", "-6", "addr", "add", tmp_ipv6, "dev", (char *)tun_dev);

	// setting the LAN address Note: IPv6 LAN CIDR should be 64
	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "%s1/%d", local_prefix, nvram_get_int("ipv6_prefix_length") ? : 64);
	eval("ip", "-6", "addr", "add", tmp_ipv6, "dev", nvram_safe_get("lan_ifname"));

	// adding default route via the border relay
	snprintf(tmp_ipv6, sizeof(tmp_ipv6), "::%s", relay);
	eval("ip", "-6", "route", "add", "default", "via", tmp_ipv6, "dev", (char *)tun_dev);

	nvram_set("ipv6_ifname", (char *)tun_dev);

	// (re)start radvd now dnsmasq
	start_dnsmasq();

	printf("6rd end\n");
}

void stop_6rd_tunnel(void)
{
	eval("ip", "tunnel", "del", (char *)get_wan6face());
	eval("ip", "-6", "addr", "flush", "dev", nvram_safe_get("lan_ifname"), "scope", "global");
	modprobe_r("sit");
}


void start_ipv6(void)
{
	int service;

	service = get_ipv6_service();
	enable_ip6_forward();

	// Check if turned on
	switch (service) {
	case IPV6_NATIVE:
	case IPV6_6IN4:
	case IPV6_MANUAL:
		add_ip6_lanaddr();
		break;
	case IPV6_NATIVE_DHCP:
	case IPV6_ANYCAST_6TO4:
		nvram_set("ipv6_rtr_addr", "");
		nvram_set("ipv6_prefix", "");
		break;
	}

	if (service != IPV6_DISABLED) {
		if ((nvram_get_int("ipv6_accept_ra") & 2) != 0 && !nvram_get_int("ipv6_radvd"))
			accept_ra(nvram_safe_get("lan_ifname"));
	}
}

void stop_ipv6(void)
{
	stop_ipv6_tunnel();
	stop_dhcp6c();
	eval("ip", "-6", "addr", "flush", "scope", "global");
}

#endif

// -----------------------------------------------------------------------------

void start_upnp(void)
{
	if (getpid() != 1) {
		start_service("upnp");
		return;
	}

	if (get_wan_proto() == WP_DISABLED) return;

	int enable;
	FILE *f;
	int upnp_port;
	
	if (((enable = nvram_get_int("upnp_enable")) & 3) != 0) {
		mkdir("/etc/upnp", 0777);
		if (f_exists("/etc/upnp/config.alt")) {
			xstart("miniupnpd", "-f", "/etc/upnp/config.alt");
		}
		else {
			if ((f = fopen("/etc/upnp/config", "w")) != NULL) {
				upnp_port = nvram_get_int("upnp_port");
				if ((upnp_port < 0) || (upnp_port >= 0xFFFF)) upnp_port = 0;

				
				fprintf(f,
					"ext_ifname=%s\n"
					"port=%d\n"
					"enable_upnp=%s\n"
					"enable_natpmp=%s\n"
					"secure_mode=%s\n"
					"upnp_forward_chain=upnp\n"
					"upnp_nat_chain=upnp\n"
					"notify_interval=%d\n"
					"system_uptime=yes\n"
					"\n"
					,
					get_wanface(),
					upnp_port,
					(enable & 1) ? "yes" : "no",						// upnp enable
					(enable & 2) ? "yes" : "no",						// natpmp enable
					nvram_get_int("upnp_secure") ? "yes" : "no",			// secure_mode (only forward to self)
					nvram_get_int("upnp_ssdp_interval")
				);

				if (nvram_get_int("upnp_clean")) {
					int interval = nvram_get_int("upnp_clean_interval");
					if (interval < 60) interval = 60;
					fprintf(f,
						"clean_ruleset_interval=%d\n"
						"clean_ruleset_threshold=%d\n",
						interval,
						nvram_get_int("upnp_clean_threshold")
					);
				}
				else
					fprintf(f,"clean_ruleset_interval=0\n");

				if (nvram_match("upnp_mnp", "1")) {
					int https = nvram_get_int("https_enable");
					fprintf(f, "presentation_url=http%s://%s:%s/forward-upnp.asp\n",
						https ? "s" : "", nvram_safe_get("lan_ipaddr"),
						nvram_safe_get(https ? "https_lanport" : "http_lanport"));
				}
				else {
					// Empty parameters are not included into XML service description
					fprintf(f, "presentation_url=\n");
				}

				char uuid[45];
				f_read_string("/proc/sys/kernel/random/uuid", uuid, sizeof(uuid));
				fprintf(f, "uuid=%s\n", uuid);

				char lanN_ipaddr[] = "lanXX_ipaddr";
				char lanN_netmask[] = "lanXX_netmask";
				char upnp_lanN[] = "upnp_lanXX";
				char br;

				for(br=0 ; br<4 ; br++) {
					char bridge[2] = "0";
					if (br!=0)
						bridge[0]+=br;
					else
						strcpy(bridge, "");

					sprintf(lanN_ipaddr, "lan%s_ipaddr", bridge);
					sprintf(lanN_netmask, "lan%s_netmask", bridge);
					sprintf(upnp_lanN, "upnp_lan%s", bridge);

					char *lanip = nvram_safe_get(lanN_ipaddr);
					char *lanmask = nvram_safe_get(lanN_netmask);
					char *lanlisten = nvram_safe_get(upnp_lanN);

					if((strcmp(lanlisten,"1")==0) && (strcmp(lanip,"")!=0) && (strcmp(lanip,"0.0.0.0")!=0)) {
						fprintf(f,
							"listening_ip=%s/%s\n",
							lanip, lanmask);
						int ports[4];
						if ((ports[0] = nvram_get_int("upnp_min_port_int")) > 0 &&
							(ports[1] = nvram_get_int("upnp_max_port_int")) > 0 &&
							(ports[2] = nvram_get_int("upnp_min_port_ext")) > 0 &&
							(ports[3] = nvram_get_int("upnp_max_port_ext")) > 0) {
							fprintf(f,
								"allow %d-%d %s/%s %d-%d\n",
								ports[0], ports[1],
								lanip, lanmask,
								ports[2], ports[3]
							);
						}
						else {
							// by default allow only redirection of ports above 1024
							fprintf(f, "allow 1024-65535 %s/%s 1024-65535\n", lanip, lanmask);
						}
					}
				}

				fappend(f, "/etc/upnp/config.custom");
				fprintf(f, "%s\n", nvram_safe_get("upnp_custom"));
				fprintf(f, "\ndeny 0-65535 0.0.0.0/0 0-65535\n");
				
				fclose(f);
				
				xstart("miniupnpd", "-f", "/etc/upnp/config");
			}
		}
	}
}

void stop_upnp(void)
{
	if (getpid() != 1) {
		stop_service("upnp");
		return;
	}

	killall_tk("miniupnpd");
}

// -----------------------------------------------------------------------------

static pid_t pid_crond = -1;

void start_cron(void)
{
	stop_cron();

	eval("crond", nvram_contains_word("log_events", "crond") ? NULL : "-l", "9");
	if (!nvram_contains_word("debug_norestart", "crond")) {
		pid_crond = -2;
	}
}

void stop_cron(void)
{
	pid_crond = -1;
	killall_tk("crond");
}

// -----------------------------------------------------------------------------
#ifdef LINUX26

static pid_t pid_hotplug2 = -1;

void start_hotplug2()
{
	stop_hotplug2();

	f_write_string("/proc/sys/kernel/hotplug", "", FW_NEWLINE, 0);
	xstart("hotplug2", "--persistent", "--no-coldplug");
	// FIXME: Don't remember exactly why I put "sleep" here -
	// but it was not for a race with check_services()... - TB
	sleep(1);

	if (!nvram_contains_word("debug_norestart", "hotplug2")) {
		pid_hotplug2 = -2;
	}
}

void stop_hotplug2(void)
{
	pid_hotplug2 = -1;
	killall_tk("hotplug2");
}

#endif	/* LINUX26 */
// -----------------------------------------------------------------------------

// Written by Sparq in 2002/07/16
void start_zebra(void)
{
#ifdef TCONFIG_ZEBRA
	if (getpid() != 1) {
		start_service("zebra");
		return;
	}

	FILE *fp;

	char *lan_tx = nvram_safe_get("dr_lan_tx");
	char *lan_rx = nvram_safe_get("dr_lan_rx");
	char *lan1_tx = nvram_safe_get("dr_lan1_tx");
	char *lan1_rx = nvram_safe_get("dr_lan1_rx");
	char *lan2_tx = nvram_safe_get("dr_lan2_tx");
	char *lan2_rx = nvram_safe_get("dr_lan2_rx");
	char *lan3_tx = nvram_safe_get("dr_lan3_tx");
	char *lan3_rx = nvram_safe_get("dr_lan3_rx");
	char *wan_tx = nvram_safe_get("dr_wan_tx");
	char *wan_rx = nvram_safe_get("dr_wan_rx");

	if ((*lan_tx == '0') && (*lan_rx == '0') && 
		(*lan1_tx == '0') && (*lan1_rx == '0') && 
		(*lan2_tx == '0') && (*lan2_rx == '0') && 
		(*lan3_tx == '0') && (*lan3_rx == '0') && 
		(*wan_tx == '0') && (*wan_rx == '0')) {
		return;
	}

	// empty
	if ((fp = fopen("/etc/zebra.conf", "w")) != NULL) {
		fclose(fp);
	}

	//
	if ((fp = fopen("/etc/ripd.conf", "w")) != NULL) {
		char *lan_ifname = nvram_safe_get("lan_ifname");
		char *lan1_ifname = nvram_safe_get("lan1_ifname");
		char *lan2_ifname = nvram_safe_get("lan2_ifname");
		char *lan3_ifname = nvram_safe_get("lan3_ifname");
		char *wan_ifname = nvram_safe_get("wan_ifname");

		fprintf(fp, "router rip\n");
		if(strcmp(lan_ifname,"")!=0)
			fprintf(fp, "network %s\n", lan_ifname);
		if(strcmp(lan1_ifname,"")!=0)
			fprintf(fp, "network %s\n", lan1_ifname);
		if(strcmp(lan2_ifname,"")!=0)
			fprintf(fp, "network %s\n", lan2_ifname);
		if(strcmp(lan3_ifname,"")!=0)
			fprintf(fp, "network %s\n", lan3_ifname);
		fprintf(fp, "network %s\n", wan_ifname);
		fprintf(fp, "redistribute connected\n");
		//fprintf(fp, "redistribute static\n");

		// 43011: modify by zg 2006.10.18 for cdrouter3.3 item 173(cdrouter_rip_30) bug
		// fprintf(fp, "redistribute kernel\n"); // 1.11: removed, redistributes indirect -- zzz

		if(strcmp(lan_ifname,"")!=0) {
			fprintf(fp, "interface %s\n", lan_ifname);
			if (*lan_tx != '0') fprintf(fp, "ip rip send version %s\n", lan_tx);
			if (*lan_rx != '0') fprintf(fp, "ip rip receive version %s\n", lan_rx);
		}
		if(strcmp(lan1_ifname,"")!=0) {
			fprintf(fp, "interface %s\n", lan1_ifname);
			if (*lan1_tx != '0') fprintf(fp, "ip rip send version %s\n", lan1_tx);
			if (*lan1_rx != '0') fprintf(fp, "ip rip receive version %s\n", lan1_rx);
		}
		if(strcmp(lan2_ifname,"")!=0) {
			fprintf(fp, "interface %s\n", lan2_ifname);
			if (*lan2_tx != '0') fprintf(fp, "ip rip send version %s\n", lan2_tx);
			if (*lan2_rx != '0') fprintf(fp, "ip rip receive version %s\n", lan2_rx);
		}
		if(strcmp(lan3_ifname,"")!=0) {
		fprintf(fp, "interface %s\n", lan3_ifname);
			if (*lan3_tx != '0') fprintf(fp, "ip rip send version %s\n", lan3_tx);
			if (*lan3_rx != '0') fprintf(fp, "ip rip receive version %s\n", lan3_rx);
		}
		fprintf(fp, "interface %s\n", wan_ifname);
		if (*wan_tx != '0') fprintf(fp, "ip rip send version %s\n", wan_tx);
		if (*wan_rx != '0') fprintf(fp, "ip rip receive version %s\n", wan_rx);

		fprintf(fp, "router rip\n");
		if(strcmp(lan_ifname,"")!=0) {
			if (*lan_tx == '0') fprintf(fp, "distribute-list private out %s\n", lan_ifname);
			if (*lan_rx == '0') fprintf(fp, "distribute-list private in %s\n", lan_ifname);
		}
		if(strcmp(lan1_ifname,"")!=0) {
			if (*lan1_tx == '0') fprintf(fp, "distribute-list private out %s\n", lan1_ifname);
			if (*lan1_rx == '0') fprintf(fp, "distribute-list private in %s\n", lan1_ifname);
		}
		if(strcmp(lan2_ifname,"")!=0) {
			if (*lan2_tx == '0') fprintf(fp, "distribute-list private out %s\n", lan2_ifname);
			if (*lan2_rx == '0') fprintf(fp, "distribute-list private in %s\n", lan2_ifname);
		}
		if(strcmp(lan3_ifname,"")!=0) {
			if (*lan3_tx == '0') fprintf(fp, "distribute-list private out %s\n", lan3_ifname);
			if (*lan3_rx == '0') fprintf(fp, "distribute-list private in %s\n", lan3_ifname);
		}
		if (*wan_tx == '0') fprintf(fp, "distribute-list private out %s\n", wan_ifname);
		if (*wan_rx == '0') fprintf(fp, "distribute-list private in %s\n", wan_ifname);
		fprintf(fp, "access-list private deny any\n");

		//fprintf(fp, "debug rip events\n");
		//fprintf(fp, "log file /etc/ripd.log\n");
		fclose(fp);
	}

	xstart("zebra", "-d");
	xstart("ripd",  "-d");
#endif
}

void stop_zebra(void)
{
#ifdef TCONFIG_ZEBRA
	if (getpid() != 1) {
		stop_service("zebra");
		return;
	}

	killall("zebra", SIGTERM);
	killall("ripd", SIGTERM);

	unlink("/etc/zebra.conf");
	unlink("/etc/ripd.conf");
#endif
}

// -----------------------------------------------------------------------------

void start_syslog(void)
{
	char *argv[16];
	int argc;
	char *nv;
	char *b_opt = "";
	char rem[256];
	int n;
	char s[64];
	char cfg[256];
	char *rot_siz = "50";
	char *rot_keep = "1";
	char *log_file_path;

	argv[0] = "syslogd";
	argc = 1;

	if (nvram_match("log_remote", "1")) {
		nv = nvram_safe_get("log_remoteip");
		if (*nv) {
			snprintf(rem, sizeof(rem), "%s:%s", nv, nvram_safe_get("log_remoteport"));
			argv[argc++] = "-R";
			argv[argc++] = rem;
		}
	}

	if (nvram_match("log_file", "1")) {
		argv[argc++] = "-L";

		if (strcmp(nvram_safe_get("log_file_size"), "") != 0) {
			rot_siz = nvram_safe_get("log_file_size");
		}
		if (nvram_get_int("log_file_size") > 0) {
			rot_keep = nvram_safe_get("log_file_keep");
		}

		// log to custom path - shibby
		if (nvram_match("log_file_custom", "1")) {
			log_file_path = nvram_safe_get("log_file_path");
			argv[argc++] = "-s";
			argv[argc++] = rot_siz;
			argv[argc++] = "-O";
			argv[argc++] = log_file_path;
			if (strcmp(nvram_safe_get("log_file_path"), "/var/log/messages") != 0) {
				remove("/var/log/messages");
				symlink(log_file_path, "/var/log/messages");
			}
		}
		else

		/* Read options:    rotate_size(kb)    num_backups    logfilename.
		 * Ignore these settings and use defaults if the logfile cannot be written to.
		 */
		if (f_read_string("/etc/syslogd.cfg", cfg, sizeof(cfg)) > 0) {
			if ((nv = strchr(cfg, '\n')))
				*nv = 0;

			if ((nv = strtok(cfg, " \t"))) {
				if (isdigit(*nv))
					rot_siz = nv;
			}

			if ((nv = strtok(NULL, " \t")))
				b_opt = nv;

			if ((nv = strtok(NULL, " \t")) && *nv == '/') {
				if (f_write(nv, cfg, 0, FW_APPEND, 0) >= 0) {
					argv[argc++] = "-O";
					argv[argc++] = nv;
				}
				else {
					rot_siz = "50";
					b_opt = "";
				}
			}
		}

		if (nvram_match("log_file_custom", "0")) {
		argv[argc++] = "-s";
		argv[argc++] = rot_siz;
		struct stat sb;
		if (lstat("/var/log/messages", &sb) != -1)
			if (S_ISLNK(sb.st_mode))
				remove("/var/log/messages");
		}

		if (isdigit(*b_opt)) {
			argv[argc++] = "-b";
			argv[argc++] = b_opt;
		} else
		if (nvram_get_int("log_file_size") > 0) {
			argv[argc++] = "-b";
			argv[argc++] = rot_keep;
		}
	}

	if (argc > 1) {
		argv[argc] = NULL;
		_eval(argv, NULL, 0, NULL);

		argv[0] = "klogd";
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);

		// used to be available in syslogd -m
		n = nvram_get_int("log_mark");
		if (n > 0) {
			// n is in minutes
			if (n < 60)
				sprintf(rem, "*/%d * * * *", n);
			else if (n < 60 * 24)
				sprintf(rem, "0 */%d * * *", n / 60);
			else
				sprintf(rem, "0 0 */%d * *", n / (60 * 24));
			sprintf(s, "%s logger -p syslog.info -- -- MARK --", rem);
			eval("cru", "a", "syslogdmark", s);
		}
		else {
			eval("cru", "d", "syslogdmark");
		}
	}
}

void stop_syslog(void)
{
	killall("klogd", SIGTERM);
	killall("syslogd", SIGTERM);
}

// -----------------------------------------------------------------------------

static pid_t pid_igmp = -1;

void start_igmp_proxy(void)
{
	FILE *fp;

	pid_igmp = -1;
	if (nvram_match("multicast_pass", "1")) {
		if (get_wan_proto() == WP_DISABLED)
			return;

		if (f_exists("/etc/igmp.alt")) {
			eval("igmpproxy", "/etc/igmp.alt");
		}
		else if ((fp = fopen("/etc/igmp.conf", "w")) != NULL) {
			fprintf(fp,
				"quickleave\n"
				"phyint %s upstream\n"
				"\taltnet %s\n",
//				"phyint %s downstream ratelimit 0\n",
				get_wanface(),
				nvram_get("multicast_altnet") ? : "0.0.0.0/0");
//				nvram_safe_get("lan_ifname"));

				char lanN_ifname[] = "lanXX_ifname";
				char multicast_lanN[] = "multicast_lanXX";
				char br;

				for(br=0 ; br<4 ; br++) {
					char bridge[2] = "0";
					if (br!=0)
						bridge[0]+=br;
					else
						strcpy(bridge, "");

					sprintf(lanN_ifname, "lan%s_ifname", bridge);
					sprintf(multicast_lanN, "multicast_lan%s", bridge);

					if((strcmp(nvram_safe_get(multicast_lanN),"1")==0) && (strcmp(nvram_safe_get(lanN_ifname),"")!=0)) {
						fprintf(fp,
							"phyint %s downstream ratelimit 0\n",
							nvram_safe_get(lanN_ifname));
					}
				}
			fclose(fp);
			eval("igmpproxy", "/etc/igmp.conf");
		}
		else {
			return;
		}
		if (!nvram_contains_word("debug_norestart", "igmprt")) {
			pid_igmp = -2;
		}
	}
}

void stop_igmp_proxy(void)
{
	pid_igmp = -1;
	killall_tk("igmpproxy");
}

// -----------------------------------------------------------------------------

void start_udpxy(void)
{
	if (nvram_match("udpxy_enable", "1")) {
		if (get_wan_proto() == WP_DISABLED)
			return;
		eval("udpxy", (nvram_get_int("udpxy_stats") ? "-S" : ""), "-p", nvram_safe_get("udpxy_port"), "-c", nvram_safe_get("udpxy_clients"), "-m", nvram_safe_get("wan_ifname") );
	}
}

void stop_udpxy(void)
{
	killall_tk("udpxy");
}

// -----------------------------------------------------------------------------

#ifdef TCONFIG_NOCAT

static pid_t pid_splashd = -1;
void start_splashd(void)
{
	pid_splashd = -1;
	start_nocat();
	if (!nvram_contains_word("debug_norestart", "splashd")) {
		pid_splashd = -2;
	}
}

void stop_splashd(void)
{
	pid_splashd = -1;
	stop_nocat();
	start_wan(BOOT);
}
#endif

// -----------------------------------------------------------------------------

void set_tz(void)
{
	f_write_string("/etc/TZ", nvram_safe_get("tm_tz"), FW_CREATE|FW_NEWLINE, 0644);
}

void start_ntpc(void)
{
	set_tz();

	stop_ntpc();

	if (nvram_get_int("ntp_updates") >= 0) {
		xstart("ntpsync", "--init");
	}
}

void stop_ntpc(void)
{
	killall("ntpsync", SIGTERM);
}

// -----------------------------------------------------------------------------

static void stop_rstats(void)
{
	int n, m;
	int pid;
	int pidz;
	int ppidz;
	int w = 0;

	n = 60;
	m = 15;
	while ((n-- > 0) && ((pid = pidof("rstats")) > 0)) {
		w = 1;
		pidz = pidof("gzip");
		if (pidz < 1) pidz = pidof("cp");
		ppidz = ppid(ppid(pidz));
		if ((m > 0) && (pidz > 0) && (pid == ppidz)) {
			syslog(LOG_DEBUG, "rstats(PID %d) shutting down, waiting for helper process to complete(PID %d, PPID %d).\n", pid, pidz, ppidz);
			--m;
		} else {
			kill(pid, SIGTERM);
		}
		sleep(1);
	}
	if ((w == 1) && (n > 0))
		syslog(LOG_DEBUG, "rstats stopped.\n");
}

static void start_rstats(int new)
{
	if (nvram_match("rstats_enable", "1")) {
		stop_rstats();
		if (new) {
			syslog(LOG_DEBUG, "starting rstats (new datafile).\n");
			xstart("rstats", "--new");
		} else {
			syslog(LOG_DEBUG, "starting rstats.\n");
			xstart("rstats");
		}
	}
}

static void stop_cstats(void)
{
	int n, m;
	int pid;
	int pidz;
	int ppidz;
	int w = 0;

	n = 60;
	m = 15;
	while ((n-- > 0) && ((pid = pidof("cstats")) > 0)) {
		w = 1;
		pidz = pidof("gzip");
		if (pidz < 1) pidz = pidof("cp");
		ppidz = ppid(ppid(pidz));
		if ((m > 0) && (pidz > 0) && (pid == ppidz)) {
			syslog(LOG_DEBUG, "cstats(PID %d) shutting down, waiting for helper process to complete(PID %d, PPID %d).\n", pid, pidz, ppidz);
			--m;
		} else {
			kill(pid, SIGTERM);
		}
		sleep(1);
	}
	if ((w == 1) && (n > 0))
		syslog(LOG_DEBUG, "cstats stopped.\n");
}

static void start_cstats(int new)
{
	if (nvram_match("cstats_enable", "1")) {
		stop_cstats();
		if (new) {
			syslog(LOG_DEBUG, "starting cstats (new datafile).\n");
			xstart("cstats", "--new");
		} else {
			syslog(LOG_DEBUG, "starting cstats.\n");
			xstart("cstats");
		}
	}
}

// -----------------------------------------------------------------------------

// !!TB - FTP Server

#ifdef TCONFIG_FTP
static char *get_full_storage_path(char *val)
{
	static char buf[128];
	int len;

	if (val[0] == '/')
		len = sprintf(buf, "%s", val);
	else
		len = sprintf(buf, "%s/%s", MOUNT_ROOT, val);

	if (len > 1 && buf[len - 1] == '/')
		buf[len - 1] = 0;

	return buf;
}

static char *nvram_storage_path(char *var)
{
	char *val = nvram_safe_get(var);
	return get_full_storage_path(val);
}

char vsftpd_conf[] = "/etc/vsftpd.conf";
char vsftpd_users[] = "/etc/vsftpd.users";
char vsftpd_passwd[] = "/etc/vsftpd.passwd";

/* VSFTPD code mostly stolen from Oleg's ASUS Custom Firmware GPL sources */

static void start_ftpd(void)
{
	char tmp[256];
	FILE *fp, *f;
	char *buf;
	char *p, *q;
	char *user, *pass, *rights, *root_dir;
	int i;

	if (getpid() != 1) {
		start_service("ftpd");
		return;
	}

	if (!nvram_get_int("ftp_enable")) return;

	mkdir_if_none(vsftpd_users);
	mkdir_if_none("/var/run/vsftpd");

	if ((fp = fopen(vsftpd_conf, "w")) == NULL)
		return;

	if (nvram_get_int("ftp_super"))
	{
		/* rights */
		sprintf(tmp, "%s/%s", vsftpd_users, "admin");
		if ((f = fopen(tmp, "w")))
		{
			fprintf(f,
				"dirlist_enable=yes\n"
				"write_enable=yes\n"
				"download_enable=yes\n");
			fclose(f);
		}
	}

#ifdef TCONFIG_SAMBASRV
	if (nvram_match("smbd_cset", "utf8"))
		fprintf(fp, "utf8=yes\n");
#endif

	if (nvram_invmatch("ftp_anonymous", "0"))
	{
		fprintf(fp,
			"anon_allow_writable_root=yes\n"
			"anon_world_readable_only=no\n"
			"anon_umask=022\n");
		
		/* rights */
		sprintf(tmp, "%s/ftp", vsftpd_users);
		if ((f = fopen(tmp, "w")))
		{
			if (nvram_match("ftp_dirlist", "0"))
				fprintf(f, "dirlist_enable=yes\n");
			if (nvram_match("ftp_anonymous", "1") || 
			    nvram_match("ftp_anonymous", "3"))
				fprintf(f, "write_enable=yes\n");
			if (nvram_match("ftp_anonymous", "1") || 
			    nvram_match("ftp_anonymous", "2"))
				fprintf(f, "download_enable=yes\n");
			fclose(f);
		}
		if (nvram_match("ftp_anonymous", "1") || 
		    nvram_match("ftp_anonymous", "3"))
			fprintf(fp, 
				"anon_upload_enable=yes\n"
				"anon_mkdir_write_enable=yes\n"
				"anon_other_write_enable=yes\n");
	} else {
		fprintf(fp, "anonymous_enable=no\n");
	}
	
	fprintf(fp,
		"dirmessage_enable=yes\n"
		"download_enable=no\n"
		"dirlist_enable=no\n"
		"hide_ids=yes\n"
		"syslog_enable=yes\n"
		"local_enable=yes\n"
		"local_umask=022\n"
		"chmod_enable=no\n"
		"chroot_local_user=yes\n"
		"check_shell=no\n"
		"log_ftp_protocol=%s\n"
		"user_config_dir=%s\n"
		"passwd_file=%s\n"
		"listen%s=yes\n"
		"listen_port=%s\n"
		"background=yes\n"
		"isolate=no\n"
		"max_clients=%d\n"
		"max_per_ip=%d\n"
		"max_login_fails=1\n"
		"idle_session_timeout=%s\n"
		"use_sendfile=no\n"
		"anon_max_rate=%d\n"
		"local_max_rate=%d\n"
		"%s\n",
		nvram_get_int("log_ftp") ? "yes" : "no",
		vsftpd_users, vsftpd_passwd,
#ifdef TCONFIG_IPV6
		ipv6_enabled() ? "_ipv6" : "",
#else
		"",
#endif
		nvram_get("ftp_port") ? : "21",
		nvram_get_int("ftp_max"),
		nvram_get_int("ftp_ipmax"),
		nvram_get("ftp_staytimeout") ? : "300",
		nvram_get_int("ftp_anonrate") * 1024,
		nvram_get_int("ftp_rate") * 1024,
		nvram_safe_get("ftp_custom"));

	fclose(fp);

	/* prepare passwd file and default users */
	if ((fp = fopen(vsftpd_passwd, "w")) == NULL)
		return;

	if (((user = nvram_get("http_username")) == NULL) || (*user == 0)) user = "admin";
	if (((pass = nvram_get("http_passwd")) == NULL) || (*pass == 0)) pass = "admin";

	fprintf(fp, /* anonymous, admin, nobody */
		"ftp:x:0:0:ftp:%s:/sbin/nologin\n"
		"%s:%s:0:0:root:/:/sbin/nologin\n"
		"nobody:x:65534:65534:nobody:%s/:/sbin/nologin\n",
		nvram_storage_path("ftp_anonroot"), user,
		nvram_get_int("ftp_super") ? crypt(pass, "$1$") : "x",
		MOUNT_ROOT);

	if ((buf = strdup(nvram_safe_get("ftp_users"))) != NULL)
	{
		/*
		username<password<rights[<root_dir>]
		rights:
			Read/Write
			Read Only
			View Only
			Private
		*/
		p = buf;
		while ((q = strsep(&p, ">")) != NULL) {
			i = vstrsep(q, "<", &user, &pass, &rights, &root_dir);
			if (i < 3 || i > 4) continue;
			if (!user || !pass) continue;

			if (i == 3 || !root_dir || !(*root_dir))

			root_dir = nvram_safe_get("ftp_pubroot");

			/* directory */
			if (strncmp(rights, "Private", 7) == 0)
			{
				sprintf(tmp, "%s/%s", nvram_storage_path("ftp_pvtroot"), user);
				mkdir_if_none(tmp);
			}
			else
				sprintf(tmp, "%s", get_full_storage_path(root_dir));

			fprintf(fp, "%s:%s:0:0:%s:%s:/sbin/nologin\n",
				user, crypt(pass, "$1$"), user, tmp);

			/* rights */
			sprintf(tmp, "%s/%s", vsftpd_users, user);
			if ((f = fopen(tmp, "w")))
			{
				tmp[0] = 0;
				if (nvram_invmatch("ftp_dirlist", "1"))
					strcat(tmp, "dirlist_enable=yes\n");
				if (strstr(rights, "Read") || !strcmp(rights, "Private"))
					strcat(tmp, "download_enable=yes\n");
				if (strstr(rights, "Write") || !strncmp(rights, "Private", 7))
					strcat(tmp, "write_enable=yes\n");
					
				fputs(tmp, f);
				fclose(f);
			}
		}
		free(buf);
	}

	fclose(fp);
	killall("vsftpd", SIGHUP);

	/* start vsftpd if it's not already running */
	if (pidof("vsftpd") <= 0)
		xstart("vsftpd");
}

static void stop_ftpd(void)
{
	if (getpid() != 1) {
		stop_service("ftpd");
		return;
	}

	killall_tk("vsftpd");
	unlink(vsftpd_passwd);
	unlink(vsftpd_conf);
	eval("rm", "-rf", vsftpd_users);
}
#endif	// TCONFIG_FTP

// -----------------------------------------------------------------------------

// !!TB - Samba

#ifdef TCONFIG_SAMBASRV
static void kill_samba(int sig)
{
	if (sig == SIGTERM) {
		killall_tk("smbd");
		killall_tk("nmbd");
	}
	else {
		killall("smbd", sig);
		killall("nmbd", sig);
	}
}

static void start_samba(void)
{
	FILE *fp;
	DIR *dir = NULL;
	struct dirent *dp;
	char nlsmod[15];
	int mode;
	char *nv;

	if (getpid() != 1) {
		start_service("smbd");
		return;
	}

	mode = nvram_get_int("smbd_enable");
	if (!mode || !nvram_invmatch("lan_hostname", ""))
		return;

	if ((fp = fopen("/etc/smb.conf", "w")) == NULL)
		return;

	fprintf(fp, "[global]\n"
		" interfaces = %s\n"
		" bind interfaces only = yes\n"
		" workgroup = %s\n"
		" netbios name = %s\n"
		" server string = %s\n"
		" guest account = nobody\n"
		" security = user\n"
		" %s\n"
		" guest ok = %s\n"
		" guest only = no\n"
		" browseable = yes\n"
		" syslog only = yes\n"
		" timestamp logs = no\n"
		" syslog = 1\n"
		" encrypt passwords = yes\n"
		" preserve case = yes\n"
		" short preserve case = yes\n",
		nvram_safe_get("lan_ifname"),
		nvram_get("smbd_wgroup") ? : "WORKGROUP",
		nvram_safe_get("lan_hostname"),
		nvram_get("router_name") ? : "Tomato",
		mode == 2 ? "" : "map to guest = Bad User",
		mode == 2 ? "no" : "yes"	// guest ok
	);

	if (nvram_get_int("smbd_wins")) {
		nv = nvram_safe_get("wan_wins");
		if ((*nv == 0) || (strcmp(nv, "0.0.0.0") == 0)) {
			fprintf(fp, " wins support = yes\n");
		}
	}

	if (nvram_get_int("smbd_master")) {
		fprintf(fp,
			" domain master = yes\n"
			" local master = yes\n"
			" preferred master = yes\n"
			" os level = 65\n");
	}

	nv = nvram_safe_get("smbd_cpage");
	if (*nv) {
#ifndef TCONFIG_SAMBA3
		fprintf(fp, " client code page = %s\n", nv);
#endif
		sprintf(nlsmod, "nls_cp%s", nv);

		nv = nvram_safe_get("smbd_nlsmod");
		if ((*nv) && (strcmp(nv, nlsmod) != 0))
			modprobe_r(nv);

		modprobe(nlsmod);
		nvram_set("smbd_nlsmod", nlsmod);
	}

#ifndef TCONFIG_SAMBA3
	if (nvram_match("smbd_cset", "utf8"))
		fprintf(fp, " coding system = utf8\n");
	else if (nvram_invmatch("smbd_cset", ""))
		fprintf(fp, " character set = %s\n", nvram_safe_get("smbd_cset"));
#endif

	nv = nvram_safe_get("smbd_custom");
	/* add socket options unless overriden by the user */
	if (strstr(nv, "socket options") == NULL) {
		fprintf(fp, " socket options = TCP_NODELAY SO_KEEPALIVE IPTOS_LOWDELAY SO_RCVBUF=65536 SO_SNDBUF=65536\n");
	}
	fprintf(fp, "%s\n\n", nv);

	/* configure shares */

	char *buf;
	char *p, *q;
	char *name, *path, *comment, *writeable, *hidden;
	int cnt = 0;

	if ((buf = strdup(nvram_safe_get("smbd_shares"))) != NULL)
	{
		/* sharename<path<comment<writeable[0|1]<hidden[0|1] */

		p = buf;
		while ((q = strsep(&p, ">")) != NULL) {
			if (vstrsep(q, "<", &name, &path, &comment, &writeable, &hidden) != 5) continue;
			if (!path || !name) continue;

			/* share name */
			fprintf(fp, "\n[%s]\n", name);

			/* path */
			fprintf(fp, " path = %s\n", path);

			/* access level */
			if (!strcmp(writeable, "1"))
				fprintf(fp, " writable = yes\n delete readonly = yes\n force user = root\n");
			if (!strcmp(hidden, "1"))
				fprintf(fp, " browseable = no\n");

			/* comment */
			if (comment)
				fprintf(fp, " comment = %s\n", comment);

			cnt++;
		}
		free(buf);
	}

	/* Share every mountpoint below MOUNT_ROOT */
	if (nvram_get_int("smbd_autoshare") && (dir = opendir(MOUNT_ROOT))) {
		while ((dp = readdir(dir))) {
			if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {

				/* Only if is a directory and is mounted */
				if (!dir_is_mountpoint(MOUNT_ROOT, dp->d_name))
					continue;

				/* smbd_autoshare: 0 - disable, 1 - read-only, 2 - writable, 3 - hidden writable */
				fprintf(fp, "\n[%s]\n path = %s/%s\n comment = %s\n",
					dp->d_name, MOUNT_ROOT, dp->d_name, dp->d_name);
				if (nvram_match("smbd_autoshare", "3"))	// Hidden
					fprintf(fp, "\n[%s$]\n path = %s/%s\n browseable = no\n",
						dp->d_name, MOUNT_ROOT, dp->d_name);
				if (nvram_match("smbd_autoshare", "2") || nvram_match("smbd_autoshare", "3"))	// RW
					fprintf(fp, " writable = yes\n delete readonly = yes\n force user = root\n");

				cnt++;
			}
		}
	}
	if (dir) closedir(dir);

	if (cnt == 0) {
		/* by default share MOUNT_ROOT as read-only */
		fprintf(fp, "\n[share]\n"
			" path = %s\n"
			" writable = no\n",
			MOUNT_ROOT);
	}

	fclose(fp);

	mkdir_if_none("/var/run/samba");
	mkdir_if_none("/etc/samba");

	/* write smbpasswd */
#ifdef TCONFIG_SAMBA3
	eval("smbpasswd", "nobody", "\"\"");
#else
	eval("smbpasswd", "-a", "nobody", "\"\"");
#endif
	if (mode == 2) {
		char *smbd_user;
		if (((smbd_user = nvram_get("smbd_user")) == NULL) || (*smbd_user == 0) || !strcmp(smbd_user, "root"))
			smbd_user = "nas";
#ifdef TCONFIG_SAMBA3
		eval("smbpasswd", smbd_user, nvram_safe_get("smbd_passwd"));
#else
		eval("smbpasswd", "-a", smbd_user, nvram_safe_get("smbd_passwd"));
#endif
	}

	kill_samba(SIGHUP);
	int ret1 = 0, ret2 = 0;
	/* start samba if it's not already running */
	if (pidof("nmbd") <= 0)
		ret1 = xstart("nmbd", "-D");
	if (pidof("smbd") <= 0)
		ret2 = xstart("smbd", "-D");

	if (ret1 || ret2) kill_samba(SIGTERM);
}

static void stop_samba(void)
{
	if (getpid() != 1) {
		stop_service("smbd");
		return;
	}

	kill_samba(SIGTERM);
	/* clean up */
	unlink("/var/log/smb");
	unlink("/var/log/nmb");
	eval("rm", "-rf", "/var/run/samba");
}
#endif	// TCONFIG_SAMBASRV

#ifdef TCONFIG_MEDIA_SERVER
#define MEDIA_SERVER_APP	"minidlna"

static void start_media_server(void)
{
	FILE *f;
	int port, pid, https;
	char *dbdir;
	char *argv[] = { MEDIA_SERVER_APP, "-f", "/etc/"MEDIA_SERVER_APP".conf", "-R", NULL };
	static int once = 1;

	if (getpid() != 1) {
		start_service("media");
		return;
	}

	if (nvram_get_int("ms_sas") == 0)
		once = 0;

	if (nvram_get_int("ms_enable") != 0) {
		if ((!once) && (nvram_get_int("ms_rescan") == 0)) {
			// no forced rescan
			argv[3] = NULL;
		}
		nvram_unset("ms_rescan");

		if (f_exists("/etc/"MEDIA_SERVER_APP".alt")) {
			argv[2] = "/etc/"MEDIA_SERVER_APP".alt";
		}
		else {
			if ((f = fopen(argv[2], "w")) != NULL) {
				port = nvram_get_int("ms_port");
				https = nvram_get_int("https_enable");
				dbdir = nvram_safe_get("ms_dbdir");
				if (!(*dbdir)) dbdir = NULL;
				mkdir_if_none(dbdir ? : "/var/run/"MEDIA_SERVER_APP);

				fprintf(f,
					"network_interface=%s\n"
					"port=%d\n"
					"friendly_name=%s\n"
					"db_dir=%s/.db\n"
					"enable_tivo=%s\n"
					"strict_dlna=%s\n"
					"presentation_url=http%s://%s:%s/nas-media.asp\n"
					"inotify=yes\n"
					"notify_interval=600\n"
					"album_art_names=Cover.jpg/cover.jpg/AlbumArtSmall.jpg/albumartsmall.jpg/AlbumArt.jpg/albumart.jpg/Album.jpg/album.jpg/Folder.jpg/folder.jpg/Thumb.jpg/thumb.jpg\n"
					"\n",
					nvram_safe_get("lan_ifname"),
					(port < 0) || (port >= 0xffff) ? 0 : port,
					nvram_get("router_name") ? : "Tomato",
					dbdir ? : "/var/run/"MEDIA_SERVER_APP,
					nvram_get_int("ms_tivo") ? "yes" : "no",
					nvram_get_int("ms_stdlna") ? "yes" : "no",
					https ? "s" : "", nvram_safe_get("lan_ipaddr"), nvram_safe_get(https ? "https_lanport" : "http_lanport")
				);

				// media directories
				char *buf, *p, *q;
				char *path, *restrict;

				if ((buf = strdup(nvram_safe_get("ms_dirs"))) != NULL) {
					/* path<restrict[A|V|P|] */

					p = buf;
					while ((q = strsep(&p, ">")) != NULL) {
						if (vstrsep(q, "<", &path, &restrict) < 1 || !path || !(*path))
							continue;
						fprintf(f, "media_dir=%s%s%s\n",
							restrict ? : "", (restrict && *restrict) ? "," : "", path);
					}
					free(buf);
				}

				fclose(f);
			}
		}

		/* start media server if it's not already running */
		if (pidof(MEDIA_SERVER_APP) <= 0) {
			if ((_eval(argv, NULL, 0, &pid) == 0) && (once)) {
				/* If we started the media server successfully, wait 1 sec
				 * to let it die if it can't open the database file.
				 * If it's still alive after that, assume it's running and
				 * disable forced once-after-reboot rescan.
				 */
				sleep(1);
				if (pidof(MEDIA_SERVER_APP) > 0)
					once = 0;
			}
		}
	}
}

static void stop_media_server(void)
{
	if (getpid() != 1) {
		stop_service("media");
		return;
	}

	killall_tk(MEDIA_SERVER_APP);
}
#endif	// TCONFIG_MEDIA_SERVER

#ifdef TCONFIG_USB
static void start_nas_services(void)
{
	if (getpid() != 1) {
		start_service("usbapps");
		return;
	}

#ifdef TCONFIG_SAMBASRV
	start_samba();
#endif
#ifdef TCONFIG_FTP
	start_ftpd();
#endif
#ifdef TCONFIG_MEDIA_SERVER
	start_media_server();
#endif
}

static void stop_nas_services(void)
{
	if (getpid() != 1) {
		stop_service("usbapps");
		return;
	}

#ifdef TCONFIG_MEDIA_SERVER
	stop_media_server();
#endif
#ifdef TCONFIG_FTP
	stop_ftpd();
#endif
#ifdef TCONFIG_SAMBASRV
	stop_samba();
#endif
}

void restart_nas_services(int stop, int start)
{
	int fd = file_lock("usb");
	/* restart all NAS applications */
	if (stop)
		stop_nas_services();
	if (start)
		start_nas_services();
	file_unlock(fd);
}
#endif // TCONFIG_USB

// -----------------------------------------------------------------------------

/* -1 = Don't check for this program, it is not expected to be running.
 * Other = This program has been started and should be kept running.  If no
 * process with the name is running, call func to restart it.
 * Note: At startup, dnsmasq forks a short-lived child which forks a
 * long-lived (grand)child.  The parents terminate.
 * Many daemons use this technique.
 */
static void _check(pid_t pid, const char *name, void (*func)(void))
{
	if (pid == -1) return;

	if (pidof(name) > 0) return;

	syslog(LOG_DEBUG, "%s terminated unexpectedly, restarting.\n", name);
	func();

	// Force recheck in 500 msec
	setitimer(ITIMER_REAL, &pop_tv, NULL);
}

void check_services(void)
{
	TRACE_PT("keep alive\n");

	// Periodically reap any zombies
	setitimer(ITIMER_REAL, &zombie_tv, NULL);

#ifdef LINUX26
	_check(pid_hotplug2, "hotplug2", start_hotplug2);
#endif
	_check(pid_dnsmasq, "dnsmasq", start_dnsmasq);
	_check(pid_crond, "crond", start_cron);
	_check(pid_igmp, "igmpproxy", start_igmp_proxy);

//#ifdef TCONFIG_NOCAT
//	if (nvram_get_int("NC_enable"))
//		_check(&pid_splashd, "splashd", start_splashd);
//#endif

}

// -----------------------------------------------------------------------------

void start_services(void)
{
	static int once = 1;

	if (once) {
		once = 0;

		if (nvram_get_int("telnetd_eas")) start_telnetd();
		if (nvram_get_int("sshd_eas")) start_sshd();
	}

//	start_syslog();
	start_nas();
	start_zebra();
#ifdef TCONFIG_SDHC
	start_mmc();
#endif
	start_dnsmasq();
	start_cifs();
	start_httpd();
	start_cron();
//	start_upnp();
	start_rstats(0);
	start_cstats(0);
	start_sched();
#ifdef TCONFIG_PPTPD
	start_pptpd();
#endif
	restart_nas_services(1, 1);	// !!TB - Samba, FTP and Media Server

#ifdef TCONFIG_SNMP
	start_snmp();
#endif

	start_tomatoanon();

#ifdef TCONFIG_TOR
	start_tor();
#endif

#ifdef TCONFIG_BT
	start_bittorrent();
#endif

#ifdef TCONFIG_NOCAT
	start_splashd();
#endif

#ifdef TCONFIG_NFS
	start_nfs();
#endif
}

void stop_services(void)
{
	clear_resolv();

#ifdef TCONFIG_BT
	stop_bittorrent();
#endif

#ifdef TCONFIG_NOCAT
	stop_splashd();
#endif

#ifdef TCONFIG_SNMP
	stop_snmp();
#endif

#ifdef TCONFIG_TOR
	stop_tor();
#endif

	stop_tomatoanon();

#ifdef TCONFIG_NFS
	stop_nfs();
#endif
	restart_nas_services(1, 0);	// stop Samba, FTP and Media Server
#ifdef TCONFIG_PPTPD
	stop_pptpd();
#endif
	stop_sched();
	stop_rstats();
	stop_cstats();
//	stop_upnp();
	stop_cron();
	stop_httpd();
#ifdef TCONFIG_SDHC
	stop_mmc();
#endif
	stop_cifs();
	stop_dnsmasq();
	stop_zebra();
	stop_nas();
//	stop_syslog();
}

// -----------------------------------------------------------------------------

/* nvram "action_service" is: "service-action[-modifier]"
 * action is something like "stop" or "start" or "restart"
 * optional modifier is "c" for the "service" command-line command
 */
void exec_service(void)
{
	const int A_START = 1;
	const int A_STOP = 2;
	const int A_RESTART = 1|2;
	char buffer[128];
	char *service;
	char *act;
	char *next;
	char *modifier;
	int action, user;
	int i;

	strlcpy(buffer, nvram_safe_get("action_service"), sizeof(buffer));
	next = buffer;

TOP:
	act = strsep(&next, ",");
	service = strsep(&act, "-");
	if (act == NULL) {
		next = NULL;
		goto CLEAR;
	}
	modifier = act;
	strsep(&modifier, "-");

	TRACE_PT("service=%s action=%s modifier=%s\n", service, act, modifier ? : "");

	if (strcmp(act, "start") == 0) action = A_START;
		else if (strcmp(act, "stop") == 0) action = A_STOP;
		else if (strcmp(act, "restart") == 0) action = A_RESTART;
		else action = 0;
	user = (modifier != NULL && *modifier == 'c');

	if (strcmp(service, "dhcpc") == 0) {
		if (action & A_STOP) stop_dhcpc();
		if (action & A_START) start_dhcpc();
		goto CLEAR;
	}

	if ((strcmp(service, "dhcpd") == 0) || (strcmp(service, "dns") == 0) || (strcmp(service, "dnsmasq") == 0)) {
		if (action & A_STOP) stop_dnsmasq();
		if (action & A_START) {
			dns_to_resolv();
			start_dnsmasq();
		}
		goto CLEAR;
	}

	if (strcmp(service, "firewall") == 0) {
		if (action & A_STOP) {
			stop_firewall();
			stop_igmp_proxy();
			stop_udpxy();
		}
		if (action & A_START) {
			start_firewall();
			start_igmp_proxy();
			start_udpxy();
		}
		goto CLEAR;
	}

	if (strcmp(service, "restrict") == 0) {
		if (action & A_STOP) {
			stop_firewall();
		}
		if (action & A_START) {
			i = nvram_get_int("rrules_radio");	// -1 = not used, 0 = enabled by rule, 1 = disabled by rule

			start_firewall();

			// if radio was disabled by access restriction, but no rule is handling it now, enable it
			if (i == 1) {
				if (nvram_get_int("rrules_radio") < 0) {
					eval("radio", "on");
				}
			}
		}
		goto CLEAR;
	}

	if (strcmp(service, "arpbind") == 0) {
		if (action & A_STOP) stop_arpbind();
		if (action & A_START) start_arpbind();
		goto CLEAR;
	}

	if (strcmp(service, "qos") == 0) {
		if (action & A_STOP) {
			stop_qos();
		}
		stop_firewall(); start_firewall();		// always restarted
		if (action & A_START) {
			start_qos();
			if (nvram_match("qos_reset", "1")) f_write_string("/proc/net/clear_marks", "1", 0, 0);
		}
		goto CLEAR;
	}

	if (strcmp(service, "qoslimit") == 0) {
		if (action & A_STOP) {
			new_qoslimit_stop();
		}
#ifdef TCONFIG_NOCAT
		stop_splashd();
#endif
		stop_firewall(); start_firewall();		// always restarted
		if (action & A_START) {
			new_qoslimit_start();
		}
#ifdef TCONFIG_NOCAT
		start_splashd();
#endif
		goto CLEAR;
	}

	if (strcmp(service, "upnp") == 0) {
		if (action & A_STOP) {
			stop_upnp();
		}
		stop_firewall(); start_firewall();		// always restarted
		if (action & A_START) {
			start_upnp();
		}
		goto CLEAR;
	}

	if (strcmp(service, "telnetd") == 0) {
		if (action & A_STOP) stop_telnetd();
		if (action & A_START) start_telnetd();
		goto CLEAR;
	}

	if (strcmp(service, "sshd") == 0) {
		if (action & A_STOP) stop_sshd();
		if (action & A_START) start_sshd();
		goto CLEAR;
	}

	if (strcmp(service, "httpd") == 0) {
		if (action & A_STOP) stop_httpd();
		if (action & A_START) start_httpd();
		goto CLEAR;
	}
	
#ifdef TCONFIG_IPV6
	if (strcmp(service, "ipv6") == 0) {
		if (action & A_STOP) {
			stop_dnsmasq();
			stop_ipv6();
		}
		if (action & A_START) {
			start_ipv6();
			start_dnsmasq();
		}
		goto CLEAR;
	}
	
	if (strncmp(service, "dhcp6", 5) == 0) {
		if (action & A_STOP) {
			stop_dhcp6c();
		}
		if (action & A_START) {
			start_dhcp6c();
		}
		goto CLEAR;
	}
#endif
	
	if (strcmp(service, "admin") == 0) {
		if (action & A_STOP) {
			stop_sshd();
			stop_telnetd();
			stop_httpd();
		}
		stop_firewall(); start_firewall();		// always restarted
		if (action & A_START) {
			start_httpd();
			create_passwd();
			if (nvram_match("telnetd_eas", "1")) start_telnetd();
			if (nvram_match("sshd_eas", "1")) start_sshd();
		}
		goto CLEAR;
	}

	if (strcmp(service, "ddns") == 0) {
		if (action & A_STOP) stop_ddns();
		if (action & A_START) start_ddns();
		goto CLEAR;
	}

	if (strcmp(service, "ntpc") == 0) {
		if (action & A_STOP) stop_ntpc();
		if (action & A_START) start_ntpc();
		goto CLEAR;
	}

	if (strcmp(service, "logging") == 0) {
		if (action & A_STOP) {
			stop_syslog();
		}
		if (action & A_START) {
			start_syslog();
		}
		if (!user) {
			// always restarted except from "service" command
			stop_cron(); start_cron();
			stop_firewall(); start_firewall();
		}
		goto CLEAR;
	}

	if (strcmp(service, "crond") == 0) {
		if (action & A_STOP) {
			stop_cron();
		}
		if (action & A_START) {
			start_cron();
		}
		goto CLEAR;
	}

#ifdef LINUX26
	if (strncmp(service, "hotplug", 7) == 0) {
		if (action & A_STOP) {
			stop_hotplug2();
		}
		if (action & A_START) {
			start_hotplug2(1);
		}
		goto CLEAR;
	}
#endif

	if (strcmp(service, "upgrade") == 0) {
		if (action & A_START) {
#if TOMATO_SL
			stop_usbevent();
			stop_smbd();
#endif
			restart_nas_services(1, 0);	// stop Samba, FTP and Media Server
			stop_jffs2();
//			stop_cifs();
			stop_zebra();
			stop_cron();
			stop_ntpc();
			stop_upnp();
//			stop_dhcpc();
			killall("rstats", SIGTERM);
			killall("cstats", SIGTERM);
			killall("buttons", SIGTERM);
			stop_syslog();
			remove_storage_main(1);	// !!TB - USB Support
			stop_usb();		// !!TB - USB Support
		}
		goto CLEAR;
	}

#ifdef TCONFIG_CIFS
	if (strcmp(service, "cifs") == 0) {
		if (action & A_STOP) stop_cifs();
		if (action & A_START) start_cifs();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_JFFS2
	if (strncmp(service, "jffs", 4) == 0) {
		if (action & A_STOP) stop_jffs2();
		if (action & A_START) start_jffs2();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "zebra") == 0) {
		if (action & A_STOP) stop_zebra();
		if (action & A_START) start_zebra();
		goto CLEAR;
	}

#ifdef TCONFIG_SDHC
	if (strcmp(service, "mmc") == 0) {
		if (action & A_STOP) stop_mmc();
		if (action & A_START) start_mmc();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "routing") == 0) {
		if (action & A_STOP) {
			stop_zebra();
			do_static_routes(0);	// remove old '_saved'
			eval("brctl", "stp", nvram_safe_get("lan_ifname"), "0");
			if(strcmp(nvram_safe_get("lan1_ifname"),"")!=0)
				eval("brctl", "stp", nvram_safe_get("lan1_ifname"), "0");
			if(strcmp(nvram_safe_get("lan2_ifname"),"")!=0)
				eval("brctl", "stp", nvram_safe_get("lan2_ifname"), "0");
			if(strcmp(nvram_safe_get("lan3_ifname"),"")!=0)
				eval("brctl", "stp", nvram_safe_get("lan3_ifname"), "0");
		}
		stop_firewall();
		start_firewall();
		if (action & A_START) {
			do_static_routes(1);	// add new
			start_zebra();
			eval("brctl", "stp", nvram_safe_get("lan_ifname"), nvram_safe_get("lan_stp"));
			if(strcmp(nvram_safe_get("lan1_ifname"),"")!=0)
				eval("brctl", "stp", nvram_safe_get("lan1_ifname"), nvram_safe_get("lan1_stp"));
			if(strcmp(nvram_safe_get("lan2_ifname"),"")!=0)
				eval("brctl", "stp", nvram_safe_get("lan2_ifname"), nvram_safe_get("lan2_stp"));
			if(strcmp(nvram_safe_get("lan3_ifname"),"")!=0)
				eval("brctl", "stp", nvram_safe_get("lan3_ifname"), nvram_safe_get("lan3_stp"));
		}
		goto CLEAR;
	}

	if (strcmp(service, "ctnf") == 0) {
		if (action & A_START) {
			setup_conntrack();
			stop_firewall();
			start_firewall();
		}
		goto CLEAR;
	}

	if (strcmp(service, "wan") == 0) {
		if (action & A_STOP) {
			stop_wan();
		}

		if (action & A_START) {
			rename("/tmp/ppp/log", "/tmp/ppp/log.~");
			start_wan(BOOT);
			sleep(2);
			force_to_dial();
		}
		goto CLEAR;
	}

	if (strcmp(service, "net") == 0) {
		if (action & A_STOP) {
#ifdef TCONFIG_USB
			stop_nas_services();
#endif
			stop_httpd();
			stop_dnsmasq();
			stop_nas();
			stop_wan();
			stop_arpbind();
			stop_lan();
			stop_vlan();
		}
		if (action & A_START) {
			start_vlan();
			start_lan();
			start_arpbind();
			start_wan(BOOT);
			start_nas();
			start_dnsmasq();
			start_httpd();
			start_wl();
#ifdef TCONFIG_USB
			start_nas_services();
#endif
		}
		goto CLEAR;
	}

	if (strcmp(service, "wireless") == 0) {
		if(action & A_STOP) {
			stop_wireless();
		}
		if(action & A_START) {
			start_wireless();
		}
		goto CLEAR;
	}

	if (strcmp(service, "wl") == 0) {
		if(action & A_STOP) {
			stop_wireless();
			unload_wl();
		}
		if(action & A_START) {
			load_wl();
			start_wireless();
			stop_wireless();
			start_wireless();
		}
		goto CLEAR;
	}

	if (strcmp(service, "nas") == 0) {
		if (action & A_STOP) {
			stop_nas();
		}
		if (action & A_START) {
			start_nas();
			start_wl();
		}
		goto CLEAR;
	}

	if (strcmp(service, "rstats") == 0) {
		if (action & A_STOP) stop_rstats();
		if (action & A_START) start_rstats(0);
		goto CLEAR;
	}

	if (strcmp(service, "rstatsnew") == 0) {
		if (action & A_STOP) stop_rstats();
		if (action & A_START) start_rstats(1);
		goto CLEAR;
	}

	if (strcmp(service, "cstats") == 0) {
		if (action & A_STOP) stop_cstats();
		if (action & A_START) start_cstats(0);
		goto CLEAR;
	}

	if (strcmp(service, "cstatsnew") == 0) {
		if (action & A_STOP) stop_cstats();
		if (action & A_START) start_cstats(1);
		goto CLEAR;
	}

	if (strcmp(service, "sched") == 0) {
		if (action & A_STOP) stop_sched();
		if (action & A_START) start_sched();
		goto CLEAR;
	}

#ifdef TCONFIG_BT
	if (strcmp(service, "bittorrent") == 0) {
		if (action & A_STOP) {
			stop_bittorrent();
		}
		stop_firewall(); start_firewall();		// always restarted
		if (action & A_START) {
			start_bittorrent();
		}
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_NFS
	if (strcmp(service, "nfs") == 0) {
		if (action & A_STOP) stop_nfs();
		if (action & A_START) start_nfs();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_SNMP
	if (strcmp(service, "snmp") == 0) {
		if (action & A_STOP) stop_snmp();
		if (action & A_START) start_snmp();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_TOR
	if (strcmp(service, "tor") == 0) {
		if (action & A_STOP) stop_tor();

		stop_firewall(); start_firewall();		// always restarted

		if (action & A_START) start_tor();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_UPS
	if (strcmp(service, "ups") == 0) {
		if (action & A_STOP) stop_ups();
		if (action & A_START) start_ups();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "tomatoanon") == 0) {
		if (action & A_STOP) stop_tomatoanon();
		if (action & A_START) start_tomatoanon();
		goto CLEAR;
	}

#ifdef TCONFIG_USB
	// !!TB - USB Support
	if (strcmp(service, "usb") == 0) {
		if (action & A_STOP) stop_usb();
		if (action & A_START) {
			start_usb();
			// restart Samba and ftp since they may be killed by stop_usb()
			restart_nas_services(0, 1);
			// remount all partitions by simulating hotplug event
			add_remove_usbhost("-1", 1);
		}
		goto CLEAR;
	}

	if (strcmp(service, "usbapps") == 0) {
		if (action & A_STOP) stop_nas_services();
		if (action & A_START) start_nas_services();
		goto CLEAR;
	}
#endif
	
#ifdef TCONFIG_FTP
	// !!TB - FTP Server
	if (strcmp(service, "ftpd") == 0) {
		if (action & A_STOP) stop_ftpd();
		setup_conntrack();
		stop_firewall();
		start_firewall();
		if (action & A_START) start_ftpd();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_MEDIA_SERVER
	if (strcmp(service, "media") == 0 || strcmp(service, "dlna") == 0) {
		if (action & A_STOP) stop_media_server();
		if (action & A_START) start_media_server();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_SAMBASRV
	// !!TB - Samba
	if (strcmp(service, "samba") == 0 || strcmp(service, "smbd") == 0) {
		if (action & A_STOP) stop_samba();
		if (action & A_START) {
			create_passwd();
			stop_dnsmasq();
			start_dnsmasq();
			start_samba();
		}
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_OPENVPN
	if (strncmp(service, "vpnclient", 9) == 0) {
		if (action & A_STOP) stop_vpnclient(atoi(&service[9]));
		if (action & A_START) start_vpnclient(atoi(&service[9]));
		goto CLEAR;
	}

	if (strncmp(service, "vpnserver", 9) == 0) {
		if (action & A_STOP) stop_vpnserver(atoi(&service[9]));
		if (action & A_START) start_vpnserver(atoi(&service[9]));
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_NOCAT
	if (strcmp(service, "splashd") == 0) {
		if (action & A_STOP) stop_splashd();
		if (action & A_START) start_splashd();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_PPTPD
	if (strcmp(service, "pptpd") == 0) {
		if (action & A_STOP) stop_pptpd();
		if (action & A_START) start_pptpd();
		goto CLEAR;
	}
#endif

#ifdef TCONFIG_PPTPD
 	if (strcmp(service, "pptpclient") == 0) {
 		if (action & A_STOP) stop_pptp_client();
 		if (action & A_START) start_pptp_client();
		if (action & (A_START | A_STOP))
		{
			stop_dnsmasq();
			dns_to_resolv();
			start_dnsmasq();
			if ((action & A_START) == 0)
				clear_pptp_route();
		}
 		goto CLEAR;
 	}
#endif

CLEAR:
	if (next) goto TOP;

	// some functions check action_service and must be cleared at end	-- zzz
	nvram_set("action_service", "");

	// Force recheck in 500 msec
	setitimer(ITIMER_REAL, &pop_tv, NULL);
}

static void do_service(const char *name, const char *action, int user)
{
	int n;
	char s[64];

	n = 150;
	while (!nvram_match("action_service", "")) {
		if (user) {
			putchar('*');
			fflush(stdout);
		}
		else if (--n < 0) break;
		usleep(100 * 1000);
	}

	snprintf(s, sizeof(s), "%s-%s%s", name, action, (user ? "-c" : ""));
	nvram_set("action_service", s);

	if (nvram_match("debug_rc_svc", "1")) {
		nvram_unset("debug_rc_svc");
		exec_service();
	} else {
		kill(1, SIGUSR1);
	}

	n = 150;
	while (nvram_match("action_service", s)) {
		if (user) {
			putchar('.');
			fflush(stdout);
		}
		else if (--n < 0) {
			break;
		}
		usleep(100 * 1000);
	}
}

int service_main(int argc, char *argv[])
{
	if (argc != 3) usage_exit(argv[0], "<service> <action>");
	do_service(argv[1], argv[2], 1);
	printf("\nDone.\n");
	return 0;
}

void start_service(const char *name)
{
	do_service(name, "start", 0);
}

void stop_service(const char *name)
{
	do_service(name, "stop", 0);
}

/*
void restart_service(const char *name)
{
	do_service(name, "restart", 0);
}
*/
