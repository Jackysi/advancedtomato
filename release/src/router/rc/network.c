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

	wificonf, OpenWRT
	Copyright (C) 2005 Felix Fietkau <nbd@vd-s.ath.cx>
	
*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/

#include <rc.h>

#ifndef UU_INT
typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#endif

#include <linux/types.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>

#include <wlutils.h>
#include <bcmparams.h>
#include <wlioctl.h>

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif
#if WL_BSS_INFO_VERSION == 108
#include <etioctl.h>
#else
#include <etsockio.h>
#endif

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

#ifdef TCONFIG_SAMBASRV
//!!TB - hostname is required for Samba to work
void set_lan_hostname(const char *wan_hostname)
{
	const char *s;
	FILE *f;

	nvram_set("lan_hostname", wan_hostname);
	if ((wan_hostname == NULL) || (*wan_hostname == 0)) {
		/* derive from et0 mac address */
		s = nvram_get("et0macaddr");
		if (s && strlen(s) >= 17) {
			char hostname[16];
			sprintf(hostname, "RT-%c%c%c%c%c%c%c%c%c%c%c%c",
				s[0], s[1], s[3], s[4], s[6], s[7],
				s[9], s[10], s[12], s[13], s[15], s[16]);

			if ((f = fopen("/proc/sys/kernel/hostname", "w"))) {
				fputs(hostname, f);
				fclose(f);
			}
			nvram_set("lan_hostname", hostname);
		}
	}

	if ((f = fopen("/etc/hosts", "w"))) {
		fprintf(f, "127.0.0.1  localhost\n");
		fprintf(f, "%s  %s\n",
			nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_hostname"));
		fclose(f);
	}
}
#endif

void set_host_domain_name(void)
{
	const char *s;

	s = nvram_safe_get("wan_hostname");
	sethostname(s, strlen(s));

#ifdef TCONFIG_SAMBASRV
	//!!TB - hostname is required for Samba to work
	set_lan_hostname(s);
#endif

	s = nvram_get("wan_domain");
	if ((s == NULL) || (*s == 0)) s = nvram_safe_get("wan_get_domain");
	setdomainname(s, strlen(s));
}

static int wlconf(char *ifname)
{
	int r;

	r = eval("wlconf", ifname, "up");
	if (r == 0) {
//		set_mac(ifname, "mac_wl", 2);

		nvram_set("rrules_radio", "-1");

		eval("wl", "antdiv", nvram_safe_get("wl_antdiv"));
		eval("wl", "txant", nvram_safe_get("wl_txant"));
		eval("wl", "txpwr1", "-o", "-m", nvram_safe_get("wl_txpwr"));

		killall("wldist", SIGTERM);
		eval("wldist");

		if (wl_client()) {
			if (nvram_match("wl_mode", "wet")) {
				ifconfig(ifname, IFUP|IFF_ALLMULTI, NULL, NULL);
			}
			if (nvram_match("wl_radio", "1")) {
				xstart("radio", "join");
			}
		}
	}
	return r;
}

static void check_afterburner(void)
{
	char *p;

	if (nvram_match("wl_afterburner", "off")) return;
	if ((p = nvram_get("boardflags")) == NULL) return;

	if (strcmp(p, "0x0118") == 0) {			// G 2.2, 3.0, 3.1
		p = "0x0318";
	}
	else if (strcmp(p, "0x0188") == 0) {	// G 2.0
		p = "0x0388";
	}
	else if (strcmp(p, "0x2558") == 0) {	// G 4.0, GL 1.0, 1.1
		p = "0x2758";
	}
	else {
		return;
	}
	
	nvram_set("boardflags", p);
	
	if (!nvram_match("debug_abrst", "0")) {
		modprobe_r("wl");
		modprobe("wl");
	}
	

/*	safe?

	unsigned long bf;
	char s[64];

	bf = strtoul(p, &p, 0);
	if ((*p == 0) && ((bf & BFL_AFTERBURNER) == 0)) {
		sprintf(s, "0x%04lX", bf | BFL_AFTERBURNER);
		nvram_set("boardflags", s);
	}
*/
}


void start_lan(void)
{
	_dprintf("%s %d\n", __FUNCTION__, __LINE__);

	char *lan_ifname;
	struct ifreq ifr;
	char *lan_ifnames, *ifname, *p;
	int sfd;

	set_mac(nvram_safe_get("wl_ifname"), "mac_wl", 2);
	check_afterburner();

	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) return;

	lan_ifname = strdup(nvram_safe_get("lan_ifname"));
	if (strncmp(lan_ifname, "br", 2) == 0) {
		eval("brctl", "addbr", lan_ifname);
		eval("brctl", "setfd", lan_ifname, "0");
		eval("brctl", "stp", lan_ifname, nvram_safe_get("lan_stp"));

		if ((lan_ifnames = strdup(nvram_safe_get("lan_ifnames"))) != NULL) {
			p = lan_ifnames;
			while ((ifname = strsep(&p, " ")) != NULL) {
				while (*ifname == ' ') ++ifname;
				if (*ifname == 0) break;
				
				// bring up interface
				if (ifconfig(ifname, IFUP, NULL, NULL) != 0) continue;

				// set the logical bridge address to that of the first interface
				strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
				if ((ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) && (memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN) == 0)) {
					strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
					if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) {
						strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
						ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
						ioctl(sfd, SIOCSIFHWADDR, &ifr);
					}
				}

				if (wlconf(ifname) == 0) {
					const char *mode = nvram_get("wl0_mode");
					if ((mode) && ((strcmp(mode, "ap") != 0) && (strcmp(mode, "wet") != 0))) continue;
				}
				eval("brctl", "addif", lan_ifname, ifname);
			}
			
			if ((nvram_get_int("wan_islan")) &&
				((get_wan_proto() == WP_DISABLED) || (nvram_match("wl_mode", "sta")))) {
				ifname = nvram_get("wan_ifnameX");
				if (ifconfig(ifname, IFUP, NULL, NULL) == 0)
					eval("brctl", "addif", lan_ifname, ifname);
			}
			
			free(lan_ifnames);
		}
	}
	// --- this shouldn't happen ---
	else if (*lan_ifname) {
		ifconfig(lan_ifname, IFUP, NULL, NULL);
		wlconf(lan_ifname);
	}
	else {
		close(sfd);
		free(lan_ifname);
		return;
	}


	// Get current LAN hardware address
	char eabuf[32];
	strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
	if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) nvram_set("lan_hwaddr", ether_etoa(ifr.ifr_hwaddr.sa_data, eabuf));


	int i, qos;
	caddr_t ifrdata;
	struct ethtool_drvinfo info;

	qos = (strcmp(nvram_safe_get("wl_wme"), "on")) ? 0 : 1;
	for (i = 1; i <= DEV_NUMIFS; i++) {
		ifr.ifr_ifindex = i;
		if (ioctl(sfd, SIOCGIFNAME, &ifr)) continue;
		if (ioctl(sfd, SIOCGIFHWADDR, &ifr)) continue;
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) continue;
		/* get flags */
		if (ioctl(sfd, SIOCGIFFLAGS, &ifr)) continue;
		/* if up(wan not up yet at this point) */
		if (ifr.ifr_flags & IFF_UP) {
			ifrdata = ifr.ifr_data;
			memset(&info, 0, sizeof(info));
			info.cmd = ETHTOOL_GDRVINFO;
			ifr.ifr_data = (caddr_t)&info;
			if (ioctl(sfd, SIOCETHTOOL, &ifr) >= 0) {
				/* currently only need to set QoS to et devices */
				if (!strncmp(info.driver, "et", 2)) {
					ifr.ifr_data = (caddr_t)&qos;
					ioctl(sfd, SIOCSETCQOS, &ifr);
				}
			}
			ifr.ifr_data = ifrdata;
		}
	}

	close(sfd);

	// bring up and configure LAN interface
	ifconfig(lan_ifname, IFUP, nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"));

	config_loopback();
	do_static_routes(1);

#ifdef TCONFIG_SAMBASRV
	//!!TB - hostname is required for Samba to work
	set_lan_hostname(nvram_safe_get("wan_hostname"));
#endif

	if (nvram_match("wan_proto", "disabled")) {
		char *gateway = nvram_safe_get("lan_gateway") ;
		if ((*gateway) && (strcmp(gateway, "0.0.0.0") != 0)) {
			int tries = 5;
			while ((route_add(lan_ifname, 0, "0.0.0.0", gateway, "0.0.0.0") != 0) && (tries-- > 0)) sleep(1);
			_dprintf("%s: add gateway=%s tries=%d\n", __FUNCTION__, gateway, tries);
		}
	}

	free(lan_ifname);

	_dprintf("%s %d\n", __FUNCTION__, __LINE__);
}

void stop_lan(void)
{
	_dprintf("%s %d\n", __FUNCTION__, __LINE__);

	char *lan_ifname;
	char *lan_ifnames, *p, *ifname;

	lan_ifname = nvram_safe_get("lan_ifname");
	ifconfig(lan_ifname, 0, NULL, NULL);

	if (strncmp(lan_ifname, "br", 2) == 0) {
		if ((lan_ifnames = strdup(nvram_safe_get("lan_ifnames"))) != NULL) {
			p = lan_ifnames;
			while ((ifname = strsep(&p, " ")) != NULL) {
				while (*ifname == ' ') ++ifname;
				if (*ifname == 0) break;
				eval("wlconf", ifname, "down");
				ifconfig(ifname, 0, NULL, NULL);
				eval("brctl", "delif", lan_ifname, ifname);
			}
			free(lan_ifnames);
		}
		eval("brctl", "delbr", lan_ifname);
	}
	else if (*lan_ifname) {
		eval("wlconf", lan_ifname, "down");
	}

	_dprintf("%s %d\n", __FUNCTION__, __LINE__);
}


void do_static_routes(int add)
{
	char *buf;
	char *p, *q;
	char *dest, *mask, *gateway, *metric, *ifname;
	int r;

	if ((buf = strdup(nvram_safe_get(add ? "routes_static" : "routes_static_saved"))) == NULL) return;
	if (add) nvram_set("routes_static_saved", buf);
		else nvram_unset("routes_static_saved");
	p = buf;
	while ((q = strsep(&p, ">")) != NULL) {
		if (vstrsep(q, "<", &dest, &gateway, &mask, &metric, &ifname) != 5) continue;
		ifname = nvram_safe_get((*ifname == 'L') ? "lan_ifname" : "wan_ifname");
		if (add) {
			for (r = 3; r >= 0; --r) {
				if (route_add(ifname, atoi(metric) + 1, dest, gateway, mask) == 0) break;
				sleep(1);
			}
		}
		else {
			route_del(ifname, atoi(metric) + 1, dest, gateway, mask);
		}
	}
	free(buf);
}

void hotplug_net(void)
{
	char *interface, *action;
	char *lan_ifname;

	if (((interface = getenv("INTERFACE")) == NULL) || ((action = getenv("ACTION")) == NULL)) return;

	_dprintf("hotplug net INTERFACE=%s ACTION=%s\n", interface, action);

	if ((nvram_match("wds_enable", "1")) && (strncmp(interface, "wds", 3) == 0) && 
	    (strcmp(action, "register") == 0 || strcmp(action, "add") == 0)) {
		ifconfig(interface, IFUP, NULL, NULL);
		lan_ifname = nvram_safe_get("lan_ifname");
		if (strncmp(lan_ifname, "br", 2) == 0) {
			eval("brctl", "addif", lan_ifname, interface);
			notify_nas(interface);
		}
	}
}

int radio_main(int argc, char *argv[])
{
	if (argc != 2) {
HELP:
		usage_exit(argv[0], "on|off|toggle|join\n");
	}
	
	if (!nvram_match("wl_radio", "1")) {
		return 1;
	}

	if (strcmp(argv[1], "toggle") == 0) {
		argv[1] = get_radio() ? "off" : "on";
	}

	if (strcmp(argv[1], "off") == 0) {
		set_radio(0);
		led(LED_DIAG, 0);
		return 0;
	}
	else if (strcmp(argv[1], "on") == 0) {
		set_radio(1);
	}
	else if (strcmp(argv[1], "join") != 0) {
		goto HELP;
	}
		

	if (wl_client()) {
		int i;
		wlc_ssid_t ssid;
		char s[32];

		if (f_read_string("/var/run/radio.pid", s, sizeof(s)) > 0) {
			if ((i = atoi(s)) > 1) {
				kill(i, SIGTERM);
				sleep(1);
			}
		}

		if (fork() == 0) {
			sprintf(s, "%d", getpid());
			f_write("/var/run/radio.pid", s, sizeof(s), 0, 0644);

			while (1) {
				if (!get_radio()) break;

				eval("wl", "join", nvram_safe_get("wl_ssid"));
				for (i = 6; i > 0; --i) {
					sleep(1);
					if ((wl_ioctl(nvram_safe_get("wl_ifname"), WLC_GET_SSID, &ssid, sizeof(ssid)) == 0) &&
						(ssid.SSID_len > 0)) break;
				}
				if (i > 0) break;
				sleep(5);
			}

			unlink("/var/run/radio.pid");
		}
	}
	
	return 0;
}

/*
int wdist_main(int argc, char *argv[])
{
	int n;
	rw_reg_t r;
	int v;

	if (argc != 2) {
		r.byteoff = 0x684;
		r.size = 2;
		if (wl_ioctl(nvram_safe_get("wl_ifname"), 101, &r, sizeof(r)) == 0) {
			v = r.val - 510;
			if (v <= 9) v = 0;
				else v = (v - (9 + 1)) * 150;
			printf("Current: %d-%dm (0x%02x)\n\n", v + (v ? 1 : 0), v + 150, r.val);
		}
		usage_exit(argv[0], "<meters>");
	}
	if ((n = atoi(argv[1])) <= 0) setup_wldistance();
		else set_wldistance(n);
	return 0;
}
*/


// ref: wificonf.c
int wldist_main(int argc, char *argv[])
{
	rw_reg_t r;
	uint32 s;
	char *p;
	int n;

	if (fork() == 0) {
		p = nvram_safe_get("wl_distance");
		if ((*p == 0) || ((n = atoi(p)) < 0)) return 0;
		n = 9 + (n / 150) + ((n % 150) ? 1 : 0);

		while (1) {
			s = 0x10 | (n << 16);
			p = nvram_safe_get("wl_ifname");
			wl_ioctl(p, 197, &s, sizeof(s));

			r.byteoff = 0x684;
			r.val = n + 510;
			r.size = 2;
			wl_ioctl(p, 102, &r, sizeof(r));
			
			sleep(2);
		}
	}
	
	return 0;
}
