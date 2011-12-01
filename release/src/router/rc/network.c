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
#include <arpa/inet.h>
#include <dirent.h>

#include <wlutils.h>
#include <bcmparams.h>
#include <wlioctl.h>

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif
#if WL_BSS_INFO_VERSION >= 108
#include <etioctl.h>
#else
#include <etsockio.h>
#endif

static void set_lan_hostname(const char *wan_hostname)
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
		if ((s = nvram_get("lan_ipaddr")) && (*s))
			fprintf(f, "%s  %s\n", s, nvram_safe_get("lan_hostname"));
#ifdef TCONFIG_IPV6
		if (ipv6_enabled()) {
			fprintf(f, "::1  localhost\n");
			s = ipv6_router_address(NULL);
			if (*s) fprintf(f, "%s  %s\n", s, nvram_safe_get("lan_hostname"));
		}
#endif
		fclose(f);
	}
}

void set_host_domain_name(void)
{
	const char *s;

	s = nvram_safe_get("wan_hostname");
	sethostname(s, strlen(s));
	set_lan_hostname(s);

	s = nvram_get("wan_domain");
	if ((s == NULL) || (*s == 0)) s = nvram_safe_get("wan_get_domain");
	setdomainname(s, strlen(s));
}

static int wlconf(char *ifname, int unit, int subunit)
{
	int r;
	char wl[24];

	if (/* !wl_probe(ifname) && */ unit >= 0) {
		// validate nvram settings for wireless i/f
		snprintf(wl, sizeof(wl), "--wl%d", unit);
		eval("nvram", "validate", wl);
	}

	r = eval("wlconf", ifname, "up");
	if (r == 0) {
		if (unit >= 0 && subunit <= 0) {
			// setup primary wl interface
			nvram_set("rrules_radio", "-1");

			eval("wl", "-i", ifname, "antdiv", nvram_safe_get(wl_nvname("antdiv", unit, 0)));
			eval("wl", "-i", ifname, "txant", nvram_safe_get(wl_nvname("txant", unit, 0)));
			eval("wl", "-i", ifname, "txpwr1", "-o", "-m", nvram_get_int(wl_nvname("txpwr", unit, 0)) ? nvram_safe_get(wl_nvname("txpwr", unit, 0)) : "-1");
			eval("wl", "-i", ifname, "interference", nvram_safe_get(wl_nvname("mitigation", unit, 0)));
		}

		if (wl_client(unit, subunit)) {
			if (nvram_match(wl_nvname("mode", unit, subunit), "wet")) {
				ifconfig(ifname, IFUP|IFF_ALLMULTI, NULL, NULL);
			}
			if (nvram_get_int(wl_nvname("radio", unit, 0))) {
				snprintf(wl, sizeof(wl), "%d", unit);
				xstart("radio", "join", wl);
			}
		}
	}
	return r;
}

// -----------------------------------------------------------------------------

#ifdef TCONFIG_EMF
static void emf_mfdb_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *mgrp, *ifname;

	/* Add/Delete MFDB entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_entry"), next) {
		ifname = word;
		mgrp = strsep(&ifname, ":");

		if ((mgrp == NULL) || (ifname == NULL)) continue;

		/* Add/Delete MFDB entry using the group addr and interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "mfdb", lan_ifname, mgrp, ifname);
		}
	}
}

static void emf_uffp_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *ifname;

	/* Add/Delete UFFP entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_uffp_entry"), next) {
		ifname = word;

		if (ifname == NULL) continue;

		/* Add/Delete UFFP entry for the interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "uffp", lan_ifname, ifname);
		}
	}
}

static void emf_rtport_update(char *lan_ifname, char *lan_port_ifname, bool add)
{
	char word[256], *next;
	char *ifname;

	/* Add/Delete RTPORT entries corresponding to new interface */
	foreach (word, nvram_safe_get("emf_rtport_entry"), next) {
		ifname = word;

		if (ifname == NULL) continue;

		/* Add/Delete RTPORT entry for the interface */
		if (lan_port_ifname == NULL || strcmp(lan_port_ifname, ifname) == 0) {
			eval("emf", ((add) ? "add" : "del"), "rtport", lan_ifname, ifname);
		}
	}
}

static void start_emf(char *lan_ifname)
{
	/* Start EMF */
	eval("emf", "start", lan_ifname);

	/* Add the static MFDB entries */
	emf_mfdb_update(lan_ifname, NULL, 1);

	/* Add the UFFP entries */
	emf_uffp_update(lan_ifname, NULL, 1);

	/* Add the RTPORT entries */
	emf_rtport_update(lan_ifname, NULL, 1);
}

static void stop_emf(char *lan_ifname)
{
	eval("emf", "stop", lan_ifname);
	eval("igs", "del", "bridge", lan_ifname);
	eval("emf", "del", "bridge", lan_ifname);
}
#endif

// -----------------------------------------------------------------------------

/* Set initial QoS mode for all et interfaces that are up. */
void set_et_qos_mode(int sfd)
{
	int i, qos;
	caddr_t ifrdata;
	struct ifreq ifr;
	struct ethtool_drvinfo info;

	qos = (strcmp(nvram_safe_get("wl_wme"), "off") != 0);
	for (i = 1; i <= DEV_NUMIFS; i++) {
		ifr.ifr_ifindex = i;
		if (ioctl(sfd, SIOCGIFNAME, &ifr)) continue;
		if (ioctl(sfd, SIOCGIFHWADDR, &ifr)) continue;
		if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER) continue;
		/* get flags */
		if (ioctl(sfd, SIOCGIFFLAGS, &ifr)) continue;
		/* if up (wan may not be up yet at this point) */
		if (ifr.ifr_flags & IFF_UP) {
			ifrdata = ifr.ifr_data;
			memset(&info, 0, sizeof(info));
			info.cmd = ETHTOOL_GDRVINFO;
			ifr.ifr_data = (caddr_t)&info;
			if (ioctl(sfd, SIOCETHTOOL, &ifr) >= 0) {
				/* Set QoS for et & bcm57xx devices */
				if (!strncmp(info.driver, "et", 2) ||
				    !strncmp(info.driver, "bcm57", 5)) {
					ifr.ifr_data = (caddr_t)&qos;
					ioctl(sfd, SIOCSETCQOS, &ifr);
				}
			}
			ifr.ifr_data = ifrdata;
		}
	}
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

static int set_wlmac(int idx, int unit, int subunit, void *param)
{
	char *ifname;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	// skip disabled wl vifs
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
		!nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 0;

//	set_mac(ifname, wl_nvname("macaddr", unit, subunit),
	set_mac(ifname, wl_nvname("hwaddr", unit, subunit),  // AB multiSSID
		2 + unit + ((subunit > 0) ? ((unit + 1) * 0x10 + subunit) : 0));

	return 1;
}

void start_wl(void)
{
	char *lan_ifname, *lan_ifnames, *ifname, *p;
	int unit, subunit;
	int is_client = 0;

	char tmp[32];
	char br;

	foreach_wif(1, NULL, set_wlmac);

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = nvram_safe_get(tmp);
//		lan_ifname = nvram_safe_get("lan_ifname");
		if (strncmp(lan_ifname, "br", 2) == 0) {
			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ifnames");
//			if ((lan_ifnames = strdup(nvram_safe_get("lan_ifnames"))) != NULL) {
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((ifname = strsep(&p, " ")) != NULL) {
					while (*ifname == ' ') ++ifname;
					if (*ifname == 0) break;

					unit = -1; subunit = -1;

					// ignore disabled wl vifs
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
						char nv[40];
						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;
					}
					// get the instance number of the wl i/f
					else if (wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
						continue;

					is_client |= wl_client(unit, subunit) && nvram_get_int(wl_nvname("radio", unit, 0));

#ifdef CONFIG_BCMWL5
					eval("wlconf", ifname, "start"); /* start wl iface */
#endif	// CONFIG_BCMWL5
				}
				free(lan_ifnames);
			}
		}
#ifdef CONFIG_BCMWL5
		else if (strcmp(lan_ifname, "")) {
			/* specific non-bridged lan iface */
			eval("wlconf", lan_ifname, "start");
		}
#endif	// CONFIG_BCMWL5
	}

	killall("wldist", SIGTERM);
	eval("wldist");

	if (is_client)
		xstart("radio", "join");
}

#ifdef TCONFIG_IPV6
void enable_ipv6(int enable)
{
	DIR *dir;
	struct dirent *dirent;
	char s[256];

	if ((dir = opendir("/proc/sys/net/ipv6/conf")) != NULL) {
		while ((dirent = readdir(dir)) != NULL) {
			sprintf(s, "/proc/sys/net/ipv6/conf/%s/disable_ipv6", dirent->d_name);
			f_write_string(s, enable ? "0" : "1", 0, 0);
		}
		closedir(dir);
	}
}

void accept_ra(const char *ifname)
{
	char s[256];

	sprintf(s, "/proc/sys/net/ipv6/conf/%s/accept_ra", ifname);
	f_write_string(s, "2", 0, 0);

	sprintf(s, "/proc/sys/net/ipv6/conf/%s/forwarding", ifname);
	f_write_string(s, "2", 0, 0);
}
#endif

void start_lan(void)
{
	_dprintf("%s %d\n", __FUNCTION__, __LINE__);

	char *lan_ifname;
	struct ifreq ifr;
	char *lan_ifnames, *ifname, *p;
	int sfd;
	uint32 ip;
	int unit, subunit, sta;
	int hwaddrset;
	char eabuf[32];
	char tmp[32];
	char tmp2[32];
	char br;

	foreach_wif(1, NULL, set_wlmac);
	check_afterburner();
#ifdef TCONFIG_IPV6
	enable_ipv6(ipv6_enabled());
#endif

	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) return;
	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

//		lan_ifname = strdup(nvram_safe_get("lan_ifname"));
		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = strdup(nvram_safe_get(tmp));

		if (strncmp(lan_ifname, "br", 2) == 0) {
			_dprintf("%s: setting up the bridge %s\n", __FUNCTION__, lan_ifname);

			eval("brctl", "addbr", lan_ifname);
			eval("brctl", "setfd", lan_ifname, "0");
			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_stp");
			eval("brctl", "stp", lan_ifname, nvram_safe_get(tmp));

#ifdef TCONFIG_EMF
			if (nvram_get_int("emf_enable")) {
				eval("emf", "add", "bridge", lan_ifname);
				eval("igs", "add", "bridge", lan_ifname);
			}
#endif

			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ipaddr");
			inet_aton(nvram_safe_get(tmp), (struct in_addr *)&ip);

			hwaddrset = 0;
			sta = 0;

			strcpy(tmp,"lan");
			strcat(tmp,bridge);
			strcat(tmp, "_ifnames");
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
				p = lan_ifnames;
				while ((ifname = strsep(&p, " ")) != NULL) {
					while (*ifname == ' ') ++ifname;
					if (*ifname == 0) break;

					unit = -1; subunit = -1;

					// ignore disabled wl vifs
					if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.')) {
						char nv[64];

						snprintf(nv, sizeof(nv) - 1, "%s_bss_enabled", ifname);
						if (!nvram_get_int(nv))
							continue;
						if (get_ifname_unit(ifname, &unit, &subunit) < 0)
							continue;
					}
					else
						wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));

					// bring up interface
					if (ifconfig(ifname, IFUP|IFF_ALLMULTI, NULL, NULL) != 0) continue;

					// set the logical bridge address to that of the first interface
					strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
					if ((!hwaddrset) ||
						(ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0 &&
						memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN) == 0)) {
						strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);
						if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) {
							strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
							ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
							_dprintf("%s: setting MAC of %s bridge to %s\n", __FUNCTION__,
								ifr.ifr_name, ether_etoa(ifr.ifr_hwaddr.sa_data, eabuf));
							ioctl(sfd, SIOCSIFHWADDR, &ifr);
							hwaddrset = 1;
						}
					}

					if (wlconf(ifname, unit, subunit) == 0) {
						const char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

						if (strcmp(mode, "wet") == 0) {
							// Enable host DHCP relay
							if (nvram_get_int("dhcp_relay")) {
								wl_iovar_set(ifname, "wet_host_mac", ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN);
								wl_iovar_setint(ifname, "wet_host_ipv4", ip);
							}
						}

						sta |= (strcmp(mode, "sta") == 0);
						if ((strcmp(mode, "ap") != 0) && (strcmp(mode, "wet") != 0)) continue;
					}
					eval("brctl", "addif", lan_ifname, ifname);
#ifdef TCONFIG_EMF
					if (nvram_get_int("emf_enable"))
						eval("emf", "add", "iface", lan_ifname, ifname);
#endif
				}
			
				if ((nvram_get_int("wan_islan")) && (br==0) &&
					((get_wan_proto() == WP_DISABLED) || (sta))) {
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
			wlconf(lan_ifname, -1, -1);
		}
		else {
			close(sfd);
			free(lan_ifname);
			return;
		}

		// Get current LAN hardware address
		strlcpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_hwaddr");
//		if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) nvram_set("lan_hwaddr", ether_etoa(ifr.ifr_hwaddr.sa_data, eabuf));
		if (ioctl(sfd, SIOCGIFHWADDR, &ifr) == 0) nvram_set(tmp, ether_etoa(ifr.ifr_hwaddr.sa_data, eabuf));

		// Set initial QoS mode for LAN ports
		set_et_qos_mode(sfd);

		close(sfd);

		// bring up and configure LAN interface
		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ipaddr");
		strcpy(tmp2,"lan");
		strcat(tmp2,bridge);
		strcat(tmp2, "_netmask");
		ifconfig(lan_ifname, IFUP, nvram_safe_get(tmp), nvram_safe_get(tmp2));

		config_loopback();
		do_static_routes(1);

		if(br==0)
			set_lan_hostname(nvram_safe_get("wan_hostname"));

		if ((get_wan_proto() == WP_DISABLED) && (br==0)) {
			char *gateway = nvram_safe_get("lan_gateway") ;
			if ((*gateway) && (strcmp(gateway, "0.0.0.0") != 0)) {
				int tries = 5;
				while ((route_add(lan_ifname, 0, "0.0.0.0", gateway, "0.0.0.0") != 0) && (tries-- > 0)) sleep(1);
				_dprintf("%s: add gateway=%s tries=%d\n", __FUNCTION__, gateway, tries);
			}
		}

#ifdef TCONFIG_IPV6
		start_ipv6();
#endif

#ifdef TCONFIG_EMF
		if (nvram_get_int("emf_enable")) start_emf(lan_ifname);
#endif

		free(lan_ifname);
	}
	_dprintf("%s %d\n", __FUNCTION__, __LINE__);
}

void stop_lan(void)
{
	_dprintf("%s %d\n", __FUNCTION__, __LINE__);

	char *lan_ifname;
	char *lan_ifnames, *p, *ifname;
	char tmp[32];
	char br;

	for(br=0 ; br<4 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifname");
		lan_ifname = nvram_safe_get(tmp);
		ifconfig(lan_ifname, 0, NULL, NULL);

#ifdef TCONFIG_IPV6
		stop_ipv6();
#endif

		if (strncmp(lan_ifname, "br", 2) == 0) {
#ifdef TCONFIG_EMF
			stop_emf(lan_ifname);
#endif
		strcpy(tmp,"lan");
		strcat(tmp,bridge);
		strcat(tmp, "_ifnames");
			if ((lan_ifnames = strdup(nvram_safe_get(tmp))) != NULL) {
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
#ifdef TCONFIG_VLAN
		ifname = nvram_safe_get(((strcmp(ifname,"LAN")==0) ? "lan_ifname" :
					((strcmp(ifname,"LAN1")==0) ? "lan1_ifname" :
					((strcmp(ifname,"LAN2")==0) ? "lan2_ifname" :
					((strcmp(ifname,"LAN3")==0) ? "lan3_ifname" :
					((*ifname == 'W') ? "wan_iface" : "wan_ifname"))))));
#else
		ifname = nvram_safe_get((*ifname == 'L') ? "lan_ifname" :
					((*ifname == 'W') ? "wan_iface" : "wan_ifname"));
#endif
		if (add) {
			for (r = 3; r >= 0; --r) {
				if (route_add(ifname, atoi(metric), dest, gateway, mask) == 0) break;
				sleep(1);
			}
		}
		else {
			route_del(ifname, atoi(metric), dest, gateway, mask);
		}
	}
	free(buf);

	char *modem_ipaddr;
	if ( (nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "dhcp") )
		&& (modem_ipaddr = nvram_safe_get("modem_ipaddr")) && *modem_ipaddr && !nvram_match("modem_ipaddr","0.0.0.0") ) {
		char ip[16];
		char *end = rindex(modem_ipaddr,'.')+1;
		unsigned char c = atoi(end);
		char *iface = nvram_safe_get("wan_ifname");

		sprintf(ip, "%.*s%hhu", end-modem_ipaddr, modem_ipaddr, (unsigned char)(c^1^((c&2)^((c&1)<<1))) );
		eval("ip", "addr", add ?"add":"del", ip, "peer", modem_ipaddr, "dev", iface);
	}


}

void hotplug_net(void)
{
	char *interface, *action;
	char *lan_ifname;

	if (((interface = getenv("INTERFACE")) == NULL) || ((action = getenv("ACTION")) == NULL)) return;

	_dprintf("hotplug net INTERFACE=%s ACTION=%s\n", interface, action);

	if ((strncmp(interface, "wds", 3) == 0) &&
	    (strcmp(action, "register") == 0 || strcmp(action, "add") == 0)) {
		ifconfig(interface, IFUP, NULL, NULL);
		lan_ifname = nvram_safe_get("lan_ifname");
#ifdef TCONFIG_EMF
		if (nvram_get_int("emf_enable")) {
			eval("emf", "add", "iface", lan_ifname, interface);
			emf_mfdb_update(lan_ifname, interface, 1);
			emf_uffp_update(lan_ifname, interface, 1);
			emf_rtport_update(lan_ifname, interface, 1);
		}
#endif
		if (strncmp(lan_ifname, "br", 2) == 0) {
			eval("brctl", "addif", lan_ifname, interface);
			notify_nas(interface);
		}
	}
}


static int is_same_addr(struct ether_addr *addr1, struct ether_addr *addr2)
{
	int i;
	for (i = 0; i < 6; i++) {
		if (addr1->octet[i] != addr2->octet[i])
			return 0;
	}
	return 1;
}

#define WL_MAX_ASSOC	128
static int check_wl_client(char *ifname, int unit, int subunit)
{
	struct ether_addr bssid;
	wl_bss_info_t *bi;
	char buf[WLC_IOCTL_MAXLEN];
	struct maclist *mlist;
	int mlsize, i;
	int associated, authorized;

	*(uint32 *)buf = WLC_IOCTL_MAXLEN;
	if (wl_ioctl(ifname, WLC_GET_BSSID, &bssid, ETHER_ADDR_LEN) < 0 ||
	    wl_ioctl(ifname, WLC_GET_BSS_INFO, buf, WLC_IOCTL_MAXLEN) < 0)
		return 0;

	bi = (wl_bss_info_t *)(buf + 4);
	if ((bi->SSID_len == 0) ||
	    (bi->BSSID.octet[0] + bi->BSSID.octet[1] + bi->BSSID.octet[2] +
	     bi->BSSID.octet[3] + bi->BSSID.octet[4] + bi->BSSID.octet[5] == 0))
		return 0;

	associated = 0;
	authorized = strstr(nvram_safe_get(wl_nvname("akm", unit, subunit)), "psk") == 0;

	mlsize = sizeof(struct maclist) + (WL_MAX_ASSOC * sizeof(struct ether_addr));
	if ((mlist = malloc(mlsize)) != NULL) {
		mlist->count = WL_MAX_ASSOC;
		if (wl_ioctl(ifname, WLC_GET_ASSOCLIST, mlist, mlsize) == 0) {
			for (i = 0; i < mlist->count; ++i) {
				if (is_same_addr(&mlist->ea[i], &bi->BSSID)) {
					associated = 1;
					break;
				}
			}
		}

		if (associated && !authorized) {
			memset(mlist, 0, mlsize);
			mlist->count = WL_MAX_ASSOC;
			strcpy((char*)mlist, "autho_sta_list");
			if (wl_ioctl(ifname, WLC_GET_VAR, mlist, mlsize) == 0) {
				for (i = 0; i < mlist->count; ++i) {
					if (is_same_addr(&mlist->ea[i], &bi->BSSID)) {
						authorized = 1;
						break;
					}
				}
			}
		}
		free(mlist);
	}

	return (associated && authorized);
}

#define STACHECK_CONNECT	30
#define STACHECK_DISCONNECT	5

static int radio_join(int idx, int unit, int subunit, void *param)
{
	int i;
	char s[32], f[64];
	char *ifname;

	int *unit_filter = param;
	if (*unit_filter >= 0 && *unit_filter != unit) return 0;

	if (!nvram_get_int(wl_nvname("radio", unit, 0)) || !wl_client(unit, subunit)) return 0;

	ifname = nvram_safe_get(wl_nvname("ifname", unit, subunit));

	// skip disabled wl vifs
	if (strncmp(ifname, "wl", 2) == 0 && strchr(ifname, '.') &&
		!nvram_get_int(wl_nvname("bss_enabled", unit, subunit)))
		return 0;

	sprintf(f, "/var/run/radio.%d.%d.pid", unit, subunit < 0 ? 0 : subunit);
	if (f_read_string(f, s, sizeof(s)) > 0) {
		if ((i = atoi(s)) > 1) {
			kill(i, SIGTERM);
			sleep(1);
		}
	}

	if (fork() == 0) {
		sprintf(s, "%d", getpid());
		f_write(f, s, sizeof(s), 0, 0644);

		int stacheck_connect = nvram_get_int("sta_chkint");
		if (stacheck_connect <= 0)
			stacheck_connect = STACHECK_CONNECT;
		int stacheck;

		while (get_radio(unit) && wl_client(unit, subunit)) {

			if (check_wl_client(ifname, unit, subunit)) {
				stacheck = stacheck_connect;
			}
			else {
				eval("wl", "-i", ifname, "disassoc");
#ifdef CONFIG_BCMWL5
				char *amode, *sec = nvram_safe_get(wl_nvname("akm", unit, subunit));

				if (strstr(sec, "psk2")) amode = "wpa2psk";
				else if (strstr(sec, "psk")) amode = "wpapsk";
				else if (strstr(sec, "wpa2")) amode = "wpa2";
				else if (strstr(sec, "wpa")) amode = "wpa";
				else if (nvram_get_int(wl_nvname("auth", unit, subunit))) amode = "shared";
				else amode = "open";

				eval("wl", "-i", ifname, "join", nvram_safe_get(wl_nvname("ssid", unit, subunit)),
					"imode", "bss", "amode", amode);
#else
				eval("wl", "-i", ifname, "join", nvram_safe_get(wl_nvname("ssid", unit, subunit)));
#endif
				stacheck = STACHECK_DISCONNECT;
			}
			sleep(stacheck);
		}
		unlink(f);
	}

	return 1;
}

enum {
	RADIO_OFF = 0,
	RADIO_ON = 1,
	RADIO_TOGGLE = 2
};

static int radio_toggle(int idx, int unit, int subunit, void *param)
{
	if (!nvram_get_int(wl_nvname("radio", unit, 0))) return 0;

	int *op = param;

	if (*op == RADIO_TOGGLE) {
		*op = get_radio(unit) ? RADIO_OFF : RADIO_ON;
	}

	set_radio(*op, unit);
	return *op;
}

int radio_main(int argc, char *argv[])
{
	int op = RADIO_OFF;
	int unit;

	if (argc < 2) {
HELP:
		usage_exit(argv[0], "on|off|toggle|join [N]\n");
	}
	unit = (argc == 3) ? atoi(argv[2]) : -1;

	if (strcmp(argv[1], "toggle") == 0)
		op = RADIO_TOGGLE;
	else if (strcmp(argv[1], "off") == 0)
		op = RADIO_OFF;
	else if (strcmp(argv[1], "on") == 0)
		op = RADIO_ON;
	else if (strcmp(argv[1], "join") == 0)
		goto JOIN;
	else
		goto HELP;

	if (unit >= 0)
		op = radio_toggle(0, unit, 0, &op);
	else
		op = foreach_wif(0, &op, radio_toggle);
		
	if (!op) {
		led(LED_DIAG, 0);
		return 0;
	}
JOIN:
	foreach_wif(1, &unit, radio_join);
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

static int get_wldist(int idx, int unit, int subunit, void *param)
{
	int n;

	char *p = nvram_safe_get(wl_nvname("distance", unit, 0));
	if ((*p == 0) || ((n = atoi(p)) < 0)) return 0;

	return (9 + (n / 150) + ((n % 150) ? 1 : 0));
}

static int wldist(int idx, int unit, int subunit, void *param)
{
	rw_reg_t r;
	uint32 s;
	char *p;
	int n;

	n = get_wldist(idx, unit, subunit, param);
	if (n > 0) {
		s = 0x10 | (n << 16);
		p = nvram_safe_get(wl_nvname("ifname", unit, 0));
		wl_ioctl(p, 197, &s, sizeof(s));

		r.byteoff = 0x684;
		r.val = n + 510;
		r.size = 2;
		wl_ioctl(p, 102, &r, sizeof(r));
	}
	return 0;
}

// ref: wificonf.c
int wldist_main(int argc, char *argv[])
{
	if (fork() == 0) {
		if (foreach_wif(0, NULL, get_wldist) == 0) return 0;

		while (1) {
			foreach_wif(0, NULL, wldist);
			sleep(2);
		}
	}

	return 0;
}
