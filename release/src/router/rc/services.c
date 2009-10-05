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
	if (((nv = nvram_get("dns_minport")) != NULL) && (*nv)) n = atoi(nv);
		else n = 4096;
	fprintf(f,
		"resolv-file=%s\n"		// the real stuff is here
		"addn-hosts=%s\n"		// "
		"expand-hosts\n"		// expand hostnames in hosts file
		"min-port=%u\n", 		// min port used for random src port
		dmresolv, dmhosts, n);
	do_dns = nvram_match("dhcpd_dmdns", "1");


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

			const dns_list_t *dns = get_dns();	// this always points to a static buffer
			if ((dns->count == 0) && (nvram_match("dhcpd_llndns", "1"))) {
				// no DNS might be temporary. use a low lease time to force clients to update.
				dhcp_lease = 2;
				strcpy(sdhcp_lease, "2m");
				do_dns = 1;
			}
			else {
				// pass the dns directly
				buf[0] = 0;
				for (n = 0 ; n < dns->count; ++n) {
					sprintf(buf + strlen(buf), ",%s", inet_ntoa(dns->dns[n]));
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
		n = nvram_get_int("dhcpd_lmax");
		fprintf(f,
			"dhcp-option=3,%s\n"	// gateway
			"dhcp-lease-max=%d\n",
			router_ip,
			(n > 0) ? n : 255);

		if (nvram_get_int("dhcpd_auth") >= 0) {
			fprintf(f, "dhcp-authoritative\n");
		}

		if (((nv = nvram_get("wan_wins")) != NULL) && (*nv) && (strcmp(nv, "0.0.0.0") != 0)) {
			fprintf(f, "dhcp-option=44,%s\n", nv);
		}
	}
	else {
		fprintf(f, "no-dhcp-interface=%s\n", lan_ifname);
	}

	// write static lease entries & create hosts file

	if ((hf = fopen(dmhosts, "w")) != NULL) {
		if (((nv = nvram_get("wan_hostname")) != NULL) && (*nv))
			fprintf(hf, "%s %s\n", router_ip, nv);
	}

	// 00:aa:bb:cc:dd:ee<123<xxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 53 w/ delim
	// 00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 85 w/ delim
	p = nvram_safe_get("dhcpd_static");
	while ((e = strchr(p, '>')) != NULL) {
		n = (e - p);
		if (n > 84) {
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
	_dprintf("%s\n", __FUNCTION__);

	f_write(dmresolv, NULL, 0, 0, 0);	// blank
}

void dns_to_resolv(void)
{
	FILE *f;
	const dns_list_t *dns;
	int i;
	mode_t m;

	_dprintf("%s\n", __FUNCTION__);

	m = umask(022);	// 077 from pppoecd
	if ((f = fopen(dmresolv, "w")) != NULL) {
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
				fprintf(f, "nameserver %s\n", inet_ntoa(dns->dns[i]));
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

#ifdef USE_MINIUPNPD
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
#else
	if (nvram_get_int("upnp_enable")) {
		xstart("upnp",
			"-D",
			"-L", nvram_safe_get("lan_ifname"),
			"-W", nvram_safe_get("wan_iface"),
			"-I", nvram_safe_get("upnp_ssdp_interval"),
			"-A", nvram_safe_get("upnp_max_age"));
	}
#endif
}

void stop_upnp(void)
{
#ifdef USE_MINIUPNPD
	killall_tk("miniupnpd");
#else
	killall_tk("upnp");
#endif
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

	xstart("zebra", "-d", "-f", "/etc/zebra.conf");
	xstart("ripd",  "-d", "-f", "/etc/ripd.conf");
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
		usleep(500000);

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
	static char *igmpproxy_conf = "/etc/igmpproxy.conf";
	FILE *fp;
	char *p;

	pid_igmp = -1;
	if (nvram_match("multicast_pass", "1")) {
		switch (get_wan_proto()) {
		case WP_PPPOE:
		case WP_PPTP:
		case WP_L2TP:
			p = "wan_iface";
			break;
		default:
			p = "wan_ifname";
			break;
		}

		if ((fp = fopen(igmpproxy_conf, "w")) != NULL) {
			fprintf(fp,
				"quickleave\n"
				"phyint %s upstream\n"
				"\taltnet %s\n"
				"phyint %s downstream ratelimit 0\n",
				nvram_safe_get(p),
				nvram_get("multicast_altnet") ? : "0.0.0.0/0",
				nvram_safe_get("lan_ifname") ? : "br0");
			fclose(fp);
			xstart("igmpproxy", igmpproxy_conf);

			if (!nvram_contains_word("debug_norestart", "igmprt")) {
				pid_igmp = -2;
			}
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

	start_syslog();
#if TOMATO_SL
	start_usbevent();
#endif
	start_nas();
	start_zebra();
	start_dnsmasq();
	start_cifs();
	start_httpd();
	start_cron();
	//start_upnp();	//!!TB - already in start_wan_done()
	start_rstats(0);
	start_sched();
#ifdef TCONFIG_SAMBA
	start_smbd();
#endif
}

void stop_services(void)
{
	clear_resolv();

#ifdef TCONFIG_SAMBA
	stop_smbd();
#endif
	stop_sched();
	stop_rstats();
	//stop_upnp();	//!!TB - moved to stop_wan()
	stop_cron();
	stop_httpd();
	stop_cifs();
	stop_dnsmasq();
	stop_zebra();
	stop_nas();
#if TOMATO_SL
	stop_usbevent();
#endif
	stop_syslog();
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

	if (strcmp(service, "upgrade") == 0) {
		if (action & A_START) {
#if TOMATO_SL
			stop_usbevent();
			stop_smbd();
#endif
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
			stop_wan();
			stop_lan();
			stop_vlan();
		}
		if (action & A_START) {
			start_vlan();
			start_lan();
			start_wan(BOOT);
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
