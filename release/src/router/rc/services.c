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

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

// -----------------------------------------------------------------------------

static const char dmhosts[] = "/etc/hosts.dnsmasq";
static const char dmresolv[] = "/etc/resolv.dnsmasq";
static const char dmpid[] = "/var/run/dnsmasq.pid";

static pid_t pid_dnsmasq = -1;


void start_dnsmasq()
{
	FILE *f;
	const char *nv;
	char buf[512];
	char lan[24];
	const char *router_ip;
	const char *lan_ifname;
	char sdhcp_lease[32];
	char *e;
	int n;
	char *mac, *ip, *name;
	char *p;
	int ipn;
	char ipbuf[32];
	FILE *hf;
	int dhcp_start;
	int dhcp_count;
	int dhcp_lease;
	int do_dhcpd;
	int do_dns;

	TRACE_PT("begin\n");

	if (getpid() != 1) {
		start_service("dnsmasq");
		return;
	}

	stop_dnsmasq();

	if (nvram_match("wl_mode", "wet")) return;
	if ((f = fopen("/etc/dnsmasq.conf", "w")) == NULL) return;

	lan_ifname = nvram_safe_get("lan_ifname");
	router_ip = nvram_safe_get("lan_ipaddr");
	strlcpy(lan, router_ip, sizeof(lan));
	if ((p = strrchr(lan, '.')) != NULL) *(p + 1) = 0;

	fprintf(f,
		"pid-file=%s\n"
		"interface=%s\n",
		dmpid, lan_ifname);
	if (((nv = nvram_get("wan_domain")) != NULL) || ((nv = nvram_get("wan_get_domain")) != NULL)) {
		if (*nv) fprintf(f, "domain=%s\n", nv);
	}

	// dns
	const dns_list_t *dns = get_dns();	// this always points to a static buffer

	if (((nv = nvram_get("dns_minport")) != NULL) && (*nv)) n = atoi(nv);
		else n = 4096;
	fprintf(f,
		"resolv-file=%s\n"		// the real stuff is here
		"addn-hosts=%s\n"		// "
		"expand-hosts\n"		// expand hostnames in hosts file
		"min-port=%u\n", 		// min port used for random src port
		dmresolv, dmhosts, n);
	do_dns = nvram_match("dhcpd_dmdns", "1");

	for (n = 0 ; n < dns->count; ++n) {
		if (dns->dns[n].port != 53) {
			fprintf(f, "server=%s#%u\n", inet_ntoa(dns->dns[n].addr), dns->dns[n].port);
		}
	}

	
	// dhcp
	do_dhcpd = nvram_match("lan_proto", "dhcp");
	if (do_dhcpd) {
		dhcp_lease = nvram_get_int("dhcp_lease");
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
				fprintf(f, "dhcp-option=6%s\n", buf);
			}
		}

		if ((p = nvram_get("dhcpd_startip")) && (*p) && (e = nvram_get("dhcpd_endip")) && (*e)) {
			fprintf(f, "dhcp-range=%s,%s,%s,%dm\n", p, e, nvram_safe_get("lan_netmask"), dhcp_lease);
		}
		else {
			// for compatibility
			dhcp_start = nvram_get_int("dhcp_start");
			dhcp_count = nvram_get_int("dhcp_num");
			fprintf(f, "dhcp-range=%s%d,%s%d,%s,%dm\n",
				lan, dhcp_start, lan, dhcp_start + dhcp_count - 1, nvram_safe_get("lan_netmask"), dhcp_lease);
		}

		nv = router_ip;
		if ((nvram_get_int("dhcpd_gwmode") == 1) && (get_wan_proto() == WP_DISABLED)) {
			p = nvram_safe_get("lan_gateway");
			if ((*p) && (strcmp(p, "0.0.0.0") != 0)) nv = p;
		}

		n = nvram_get_int("dhcpd_lmax");
		fprintf(f,
			"dhcp-option=3,%s\n"	// gateway
			"dhcp-lease-max=%d\n",
			nv,
			(n > 0) ? n : 255);

		if (nvram_get_int("dhcpd_auth") >= 0) {
			fprintf(f, "dhcp-authoritative\n");
		}

		if (((nv = nvram_get("wan_wins")) != NULL) && (*nv) && (strcmp(nv, "0.0.0.0") != 0)) {
			fprintf(f, "dhcp-option=44,%s\n", nv);
		}
#ifdef TCONFIG_SAMBASRV
		else if (nvram_get_int("smbd_enable") && nvram_invmatch("lan_hostname", "") && nvram_get_int("smbd_wins")) {
			if ((nv == NULL) || (*nv == 0) || (strcmp(nv, "0.0.0.0") == 0)) {
				// Samba will serve as a WINS server
				fprintf(f, "dhcp-option=44,0.0.0.0\n");
			}
		}
#endif
	}
	else {
		fprintf(f, "no-dhcp-interface=%s\n", lan_ifname);
	}

	// write static lease entries & create hosts file

	if ((hf = fopen(dmhosts, "w")) != NULL) {
		if (((nv = nvram_get("wan_hostname")) != NULL) && (*nv))
			fprintf(hf, "%s %s\n", router_ip, nv);
#ifdef TCONFIG_SAMBASRV
		else if (((nv = nvram_get("lan_hostname")) != NULL) && (*nv))
			fprintf(hf, "%s %s\n", router_ip, nv);
#endif
	}

	// 00:aa:bb:cc:dd:ee<123<xxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 53 w/ delim
	// 00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 85 w/ delim
	// 00:aa:bb:cc:dd:ee,00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 106 w/ delim
	p = nvram_safe_get("dhcpd_static");
	while ((e = strchr(p, '>')) != NULL) {
		n = (e - p);
		if (n > 105) {
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

		if ((hf) && (*name != 0)) {
			fprintf(hf, "%s %s\n", ip, name);
		}

		if ((do_dhcpd) && (*mac != 0) && (strcmp(mac, "00:00:00:00:00:00") != 0)) {
			fprintf(f, "dhcp-host=%s,%s,%s\n", mac, ip, sdhcp_lease);
		}
	}

	if (hf) fclose(hf);

	//

#ifdef TCONFIG_OPENVPN
	write_vpn_dnsmasq_config(f);
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

	eval("dnsmasq");

	if (!nvram_contains_word("debug_norestart", "dnsmasq")) {
		f_read_string(dmpid, buf, sizeof(buf));
		pid_dnsmasq = atol(buf);
	}

	TRACE_PT("end\n");
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

	TRACE_PT("end\n");
}

void clear_resolv(void)
{
	f_write(dmresolv, NULL, 0, 0, 0);	// blank
}

void dns_to_resolv(void)
{
	FILE *f;
	const dns_list_t *dns;
	int i;
	mode_t m;

	m = umask(022);	// 077 from pppoecd
	if ((f = fopen(dmresolv, "w")) != NULL) {
		// Check for VPN DNS entries
		if (!write_vpn_resolv(f)) {
			dns = get_dns();	// static buffer
			if (dns->count == 0) {
				// Put a pseudo DNS IP to trigger Connect On Demand
				if ((nvram_match("ppp_demand", "1")) &&
					(nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "pptp") || nvram_match("wan_proto", "l2tp"))) {
					fprintf(f, "nameserver 1.1.1.1\n");
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
	chdir("/www");
	if (!nvram_match("http_enable", "0")) {
		xstart("httpd");
	}
	if (!nvram_match("https_enable", "0")) {
		xstart("httpd", "-s");
	}
	chdir("/");
}

void stop_httpd(void)
{
	killall_tk("httpd");
}

// -----------------------------------------------------------------------------

void start_upnp(void)
{
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

				char *lanip = nvram_safe_get("lan_ipaddr");
				char *lanmask = nvram_safe_get("lan_netmask");
				
				fprintf(f,
					"ext_ifname=%s\n"
					"listening_ip=%s/%s\n"
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
					nvram_safe_get("wan_iface"),
					lanip, lanmask,
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
						https ? "s" : "", lanip,
						nvram_safe_get(https ? "https_lanport" : "http_lanport"));
				}
				else {
					// Empty parameters are not included into XML service description
					fprintf(f, "presentation_url=\n");
				}

				char uuid[45];
				f_read_string("/proc/sys/kernel/random/uuid", uuid, sizeof(uuid));
				fprintf(f, "uuid=%s\n", uuid);

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

				fappend(f, "/etc/upnp/config.custom");
				fprintf(f, "\ndeny 0-65535 0.0.0.0/0 0-65535\n");
				fclose(f);
				
				xstart("miniupnpd", "-f", "/etc/upnp/config");
			}
		}
	}
}

void stop_upnp(void)
{
	killall_tk("miniupnpd");
}

// -----------------------------------------------------------------------------

static pid_t pid_crond = -1;

void start_cron(void)
{
	char *argv[] = { "crond", "-l", "9", NULL };

	stop_cron();

	if (nvram_contains_word("log_events", "crond")) argv[1] = NULL;
	_eval(argv, NULL, 0, NULL);
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
	FILE *fp;

	char *lan_tx = nvram_safe_get("dr_lan_tx");
	char *lan_rx = nvram_safe_get("dr_lan_rx");
	char *wan_tx = nvram_safe_get("dr_wan_tx");
	char *wan_rx = nvram_safe_get("dr_wan_rx");

	if ((*lan_tx == '0') && (*lan_rx == '0') && (*wan_tx == '0') && (*wan_rx == '0')) {
		return;
	}

	// empty
	if ((fp = fopen("/etc/zebra.conf", "w")) != NULL) {
		fclose(fp);
	}

	//
	if ((fp = fopen("/etc/ripd.conf", "w")) != NULL) {
		char *lan_ifname = nvram_safe_get("lan_ifname");
		char *wan_ifname = nvram_safe_get("wan_ifname");

		fprintf(fp, "router rip\n");
		fprintf(fp, "network %s\n", lan_ifname);
		fprintf(fp, "network %s\n", wan_ifname);
		fprintf(fp, "redistribute connected\n");
		//fprintf(fp, "redistribute static\n");

		// 43011: modify by zg 2006.10.18 for cdrouter3.3 item 173(cdrouter_rip_30) bug
		// fprintf(fp, "redistribute kernel\n"); // 1.11: removed, redistributes indirect -- zzz

		fprintf(fp, "interface %s\n", lan_ifname);
		if (*lan_tx != '0') fprintf(fp, "ip rip send version %s\n", lan_tx);
		if (*lan_rx != '0') fprintf(fp, "ip rip receive version %s\n", lan_rx);

		fprintf(fp, "interface %s\n", wan_ifname);
		if (*wan_tx != '0') fprintf(fp, "ip rip send version %s\n", wan_tx);
		if (*wan_rx != '0') fprintf(fp, "ip rip receive version %s\n", wan_rx);

		fprintf(fp, "router rip\n");
		if (*lan_tx == '0') fprintf(fp, "distribute-list private out %s\n", lan_ifname);
		if (*lan_rx == '0') fprintf(fp, "distribute-list private in %s\n", lan_ifname);
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
	killall("zebra", SIGTERM);
	killall("ripd", SIGTERM);

	unlink("/etc/zebra.conf");
	unlink("/etc/ripd.conf");
#endif
}

// -----------------------------------------------------------------------------

void start_syslog(void)
{
#if 1
	char *argv[12];
	int argc;
	char *nv;
	char rem[256];
	int n;
	char s[64];

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
		argv[argc++] = "-s";
		argv[argc++] = "50";
	}

	if (argc > 1) {
		argv[argc] = NULL;
		_eval(argv, NULL, 0, NULL);
		//usleep(500000);
		sleep(1);

		argv[0] = "klogd";
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
		usleep(500000);

		// used to be available in syslogd -m
		n = nvram_get_int("log_mark");
		if (n > 0) {
			sprintf(s, "cru a syslogdmark \"%s %s * * * logger -p syslog.info -- -- MARK --\"",
				(n < 60) ? "*/30" : "0", (n < 120) ? "*" : "*/2");
			system(s);
		}
		else {
			system("cru d syslogdmark");
		}
	}

#else
	char *argv[12];
	int argc;
	char *nv;
	char rem[256];

	argv[0] = "syslogd";
	argv[1] = "-m";
	argv[2] = nvram_get("log_mark");
	argc = 3;

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
		argv[argc++] = "-s";
		argv[argc++] = "50";
	}

	if (argc > 3) {
		argv[argc] = NULL;
		_eval(argv, NULL, 0, NULL);
		usleep(500000);

		argv[0] = "klogd";
		argv[1] = NULL;
		_eval(argv, NULL, 0, NULL);
		usleep(500000);
	}
#endif
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
	char *p;

	pid_igmp = -1;
	if (nvram_match("multicast_pass", "1")) {
		switch (get_wan_proto()) {
		case WP_DISABLED:
			return;
		case WP_PPPOE:
		case WP_PPTP:
		case WP_L2TP:
			p = "wan_iface";
			break;
		default:
			p = "wan_ifname";
			break;
		}

		if (f_exists("/etc/igmp.alt")) {
			xstart("igmpproxy", "/etc/igmp.alt");
		}
		else if ((fp = fopen("/etc/igmp.conf", "w")) != NULL) {
			fprintf(fp,
				"quickleave\n"
				"phyint %s upstream\n"
				"\taltnet %s\n"
				"phyint %s downstream ratelimit 0\n",
				nvram_safe_get(p),
				nvram_get("multicast_altnet") ? : "0.0.0.0/0",
				nvram_safe_get("lan_ifname"));
			fclose(fp);
			xstart("igmpproxy", "/etc/igmp.conf");
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
	killall("igmpproxy", SIGTERM);
}


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
	int n;
	int pid;

	n = 60;
	while ((n-- > 0) && ((pid = pidof("rstats")) > 0)) {
		if (kill(pid, SIGTERM) != 0) break;
		sleep(1);
	}
}

static void start_rstats(int new)
{
	if (nvram_match("rstats_enable", "1")) {
		stop_rstats();
		if (new) xstart("rstats", "--new");
			else xstart("rstats");
	}
}

// -----------------------------------------------------------------------------

// !!TB - FTP Server

#ifdef TCONFIG_USB
/* 
 * Return non-zero if we created the directory,
 * and zero if it already existed.
 */
int mkdir_if_none(char *dir)
{
	DIR *dp;
	if (!(dp=opendir(dir))) {
		umask(0000);
		mkdir(dir, 0777);
		return 1;
	}
	closedir(dp);
	return 0;
}

char *get_full_storage_path(char *val)
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

char *nvram_storage_path(char *var)
{
	char *val = nvram_safe_get(var);
	return get_full_storage_path(val);
}
#endif // TCONFIG_USB

#ifdef TCONFIG_FTP

char vsftpd_conf[] = "/etc/vsftpd.conf";
char vsftpd_users[] = "/etc/vsftpd.users";
char vsftpd_passwd[] = "/etc/vsftpd.passwd";
#endif

#ifdef TCONFIG_FTP
/* VSFTPD code mostly stolen from Oleg's ASUS Custom Firmware GPL sources */
static void do_start_stop_ftpd(int stop, int start)
{
	if (stop) killall_tk("vsftpd");

	char tmp[256];
	FILE *fp, *f;

	if (!start || !nvram_get_int("ftp_enable")) return;

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
		"listen=yes\n"
		"listen_port=%s\n"
		"background=yes\n"
		"isolate=no\n"
		"max_clients=%d\n"
		"max_per_ip=%d\n"
		"idle_session_timeout=%s\n"
		"use_sendfile=no\n"
		"anon_max_rate=%d\n"
		"local_max_rate=%d\n"
		"%s\n",
		nvram_get_int("log_ftp") ? "yes" : "no",
		vsftpd_users, vsftpd_passwd,
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

	fprintf(fp, /* anonymous, admin, nobody */
		"ftp:x:0:0:ftp:%s:/sbin/nologin\n"
		"%s:%s:0:0:root:/:/sbin/nologin\n"
		"nobody:x:65534:65534:nobody:%s/:/sbin/nologin\n",
		nvram_storage_path("ftp_anonroot"), "admin",
		nvram_get_int("ftp_super") ? crypt(nvram_safe_get("http_passwd"), "$1$") : "x",
		MOUNT_ROOT);

	char *buf;
	char *p, *q;
	char *user, *pass, *rights;

	if ((buf = strdup(nvram_safe_get("ftp_users"))) != NULL)
	{
		/*
		username<password<rights
		rights:
			Read/Write
			Read Only
			View Only
			Private
		*/
		p = buf;
		while ((q = strsep(&p, ">")) != NULL) {
			if (vstrsep(q, "<", &user, &pass, &rights) != 3) continue;
			if (!user || !pass) continue;

			/* directory */
			if (strncmp(rights, "Private", 7) == 0)
			{
				sprintf(tmp, "%s/%s", nvram_storage_path("ftp_pvtroot"), user);
				mkdir_if_none(tmp);
			}
			else
				sprintf(tmp, "%s", nvram_storage_path("ftp_pubroot"));

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
		eval("vsftpd");
}
#endif

void start_ftpd(void)
{
#ifdef TCONFIG_FTP
	int fd = file_lock("usb");
	do_start_stop_ftpd(0, 1);
	file_unlock(fd);
#endif
}

void stop_ftpd(void)
{
#ifdef TCONFIG_FTP
	int fd = file_lock("usb");
	do_start_stop_ftpd(1, 0);
	unlink(vsftpd_passwd);
	unlink(vsftpd_conf);
	eval("rm", "-rf", vsftpd_users);
	file_unlock(fd);
#endif
}

// -----------------------------------------------------------------------------

// !!TB - Samba

#ifdef TCONFIG_SAMBASRV
void kill_samba(int sig)
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
#endif

#ifdef TCONFIG_SAMBASRV
static void do_start_stop_samba(int stop, int start)
{
	if (stop) kill_samba(SIGTERM);

	FILE *fp;
	DIR *dir = NULL;
	struct dirent *dp;
	char nlsmod[15];
	int mode;
	char *nv;
	
	mode = nvram_get_int("smbd_enable");
	if (!start || !mode || !nvram_invmatch("lan_hostname", ""))
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
		" security = %s\n"
		" browseable = yes\n"
		" guest ok = yes\n"
		" guest only = no\n"
		" log level = %d\n"
		" syslog only = yes\n"
		" timestamp logs = no\n"
		" syslog = 1\n"
		" dns proxy = no\n"
		" encrypt passwords = yes\n"
		" preserve case = yes\n"
		" short preserve case = yes\n",
		nvram_safe_get("lan_ifname"),
		nvram_get("smbd_wgroup") ? : "WORKGROUP",
		nvram_safe_get("lan_hostname"),
		nvram_get("router_name") ? : "Tomato",
		mode == 2 ? "user" : "share",
		nvram_get_int("smbd_loglevel")
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

	fprintf(fp, "%s\n\n", nvram_safe_get("smbd_custom"));
	
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
				fprintf(fp, " writable = yes\n force user = %s\n", "root");
			if (!strcmp(hidden, "1"))
				fprintf(fp, " browseable = no\n");

			/* comment */
			if (comment)
				fprintf(fp, " comment = %s\n", comment);

			cnt++;
		}
		free(buf);
	}

	/* share everything below MOUNT_ROOT */
	if (nvram_get_int("smbd_autoshare") && (dir = opendir(MOUNT_ROOT))) {
		while ((dp = readdir(dir))) {
			if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, "..")) {

				/* smbd_autoshare: 0 - disable, 1 - read-only, 2 - writable, 3 - hidden writable */
				fprintf(fp, "\n[%s]\n path = %s/%s\n comment = %s\n",
					dp->d_name, MOUNT_ROOT, dp->d_name, dp->d_name);
				if (nvram_match("smbd_autoshare", "3"))	// Hidden
					fprintf(fp, "\n[%s$]\n path = %s/%s\n browseable = no\n",
						dp->d_name, MOUNT_ROOT, dp->d_name);
				if (nvram_match("smbd_autoshare", "2") || nvram_match("smbd_autoshare", "3"))	// RW
					fprintf(fp, " writable = yes\n force user = %s\n", "root");

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
		ret1 = eval("nmbd", "-D");
	if (pidof("smbd") <= 0)
		ret2 = eval("smbd", "-D");

	if (ret1 || ret2) kill_samba(SIGTERM);
}
#endif

void start_samba(void)
{
#ifdef TCONFIG_SAMBASRV
	int fd = file_lock("usb");
	do_start_stop_samba(0, 1);
	file_unlock(fd);
#endif
}

void stop_samba(void)
{
#ifdef TCONFIG_SAMBASRV
	int fd = file_lock("usb");
	do_start_stop_samba(1, 0);

#if 0
	if (nvram_invmatch("smbd_nlsmod", "")) {
		modprobe_r(nvram_get("smbd_nlsmod"));
		nvram_set("smbd_nlsmod", "");
	}
#endif

	/* clean up */
	unlink("/var/log/smb");
	unlink("/var/log/nmb");
	eval("rm", "-rf", "/var/run/samba");
	file_unlock(fd);
#endif
}

#ifdef TCONFIG_USB
void restart_nas_services(int stop, int start)
{	
	/* restart all NAS applications */
#if TCONFIG_SAMBASRV || TCONFIG_FTP
	int fd = file_lock("usb");
	#ifdef TCONFIG_SAMBASRV
	do_start_stop_samba(stop, start && nvram_get_int("smbd_enable"));
	#endif
	#ifdef TCONFIG_FTP
	do_start_stop_ftpd(stop, start && nvram_get_int("ftp_enable"));
	#endif
	file_unlock(fd);
#endif	// TCONFIG_SAMBASRV || TCONFIG_FTP
}
#endif // TCONFIG_USB

// -----------------------------------------------------------------------------

static void _check(pid_t *pid, const char *name, void (*func)(void) )
{
	if (*pid != -1) {
		if (kill(*pid, 0) != 0) {
			if ((*pid = pidof(name)) == -1) func();
		}
	}
}

void check_services(void)
{
#ifdef LINUX26
	_check(&pid_hotplug2, "hotplug2", start_hotplug2);
#endif
	_check(&pid_dnsmasq, "dnsmasq", start_dnsmasq);
	_check(&pid_crond, "crond", start_cron);
	_check(&pid_igmp, "igmpproxy", start_igmp_proxy);
}

// -----------------------------------------------------------------------------

void start_services(void)
{
	static int once = 1;

	if (once) {
		once = 0;

		create_passwd();
		if (nvram_get_int("telnetd_eas")) start_telnetd();
		if (nvram_get_int("sshd_eas")) start_sshd();
	}

//	start_syslog();
	start_nas();
	start_zebra();
	start_dnsmasq();
	start_cifs();
	start_httpd();
	start_cron();
//	start_upnp();
	start_rstats(0);
	start_sched();
	restart_nas_services(1, 1);	// !!TB - Samba and FTP Server
}

void stop_services(void)
{
	clear_resolv();

	stop_ftpd();		// !!TB - FTP Server
	stop_samba();		// !!TB - Samba
	stop_sched();
	stop_rstats();
//	stop_upnp();
	stop_cron();
	stop_httpd();
	stop_cifs();
	stop_dnsmasq();
	stop_zebra();
	stop_nas();
//	stop_syslog();
}

// -----------------------------------------------------------------------------

void exec_service(void)
{
	const int A_START = 1;
	const int A_STOP = 2;
	const int A_RESTART = 1|2;
	char buffer[128];
	char *service;
	char *act;
	char *next;
	int action;
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

	TRACE_PT("service=%s action=%s\n", service, act);

	if (strcmp(act, "start") == 0) action = A_START;
		else if (strcmp(act, "stop") == 0) action = A_STOP;
		else if (strcmp(act, "restart") == 0) action = A_RESTART;
		else action = 0;


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
		}
		if (action & A_START) {
			start_firewall();
			start_igmp_proxy();
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
					if (!get_radio()) eval("radio", "on");
				}
			}
		}
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
			stop_cron();
		}
		stop_firewall(); start_firewall();		// always restarted
		if (action & A_START) {
			start_cron();
			start_syslog();
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
			stop_ftpd();		// !!TB - FTP Server
			stop_samba();		// !!TB - Samba
			stop_jffs2();
//			stop_cifs();
			stop_zebra();
			stop_cron();
			stop_ntpc();
			stop_upnp();
//			stop_dhcpc();
			killall("rstats", SIGTERM);
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
	if (strcmp(service, "jffs2") == 0) {
		if (action & A_STOP) stop_jffs2();
		if (action & A_START) start_jffs2();
		goto CLEAR;
	}
#endif

	if (strcmp(service, "routing") == 0) {
		if (action & A_STOP) {
			stop_zebra();
			do_static_routes(0);	// remove old '_saved'
			eval("brctl", "stp", nvram_safe_get("lan_ifname"), "0");
		}
		stop_firewall();
		start_firewall();
		if (action & A_START) {
			do_static_routes(1);	// add new
			start_zebra();
			eval("brctl", "stp", nvram_safe_get("lan_ifname"), nvram_safe_get("lan_stp"));
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
			if (get_wan_proto() == WP_PPPOE) {
				stop_dnsmasq();
				stop_redial();
				stop_singe_pppoe(PPPOE0);
				if (((action & A_START) == 0) && (nvram_match("ppp_demand", "1"))) {
					sleep(1);
					start_pppoe(PPPOE0);
				}
				start_dnsmasq();
			}
			else {
				stop_wan();
			}
		}

		if (action & A_START) {
			rename("/tmp/ppp/log", "/tmp/ppp/log.~");

			if (get_wan_proto() == WP_PPPOE) {
				stop_singe_pppoe(PPPOE0);
				start_pppoe(PPPOE0);
				if (nvram_invmatch("ppp_demand", "1")) {
					start_redial();
				}
			}
			else {
				start_wan(BOOT);
			}
			sleep(2);
			force_to_dial();
		}
		goto CLEAR;
	}

	if (strcmp(service, "net") == 0) {
		if (action & A_STOP) {
			stop_dnsmasq();
			stop_nas();
			stop_wan();
			stop_lan();
			stop_vlan();
		}
		if (action & A_START) {
			start_vlan();
			start_lan();
			start_wan(BOOT);
			start_nas();
			start_dnsmasq();
			start_wl();
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

	if (strcmp(service, "sched") == 0) {
		if (action & A_STOP) stop_sched();
		if (action & A_START) start_sched();
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
		}
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

CLEAR:
	if (next) goto TOP;

	// some functions check action_service and must be cleared at end	-- zzz
	nvram_set("action_service", "");
}

static void do_service(const char *name, const char *action, int user)
{
	int n;
	char s[64];

	n = 15;
	while (!nvram_match("action_service", "")) {
		if (user) {
			putchar('*');
			fflush(stdout);
		}
		else if (--n < 0) break;
		sleep(1);
	}

	snprintf(s, sizeof(s), "%s-%s", name, action);
	nvram_set("action_service", s);
	kill(1, SIGUSR1);

	n = 15;
	while (nvram_match("action_service", s)) {
		if (user) {
			putchar('.');
			fflush(stdout);
		}
		else if (--n < 0) {
			break;
		}
		sleep(1);
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
