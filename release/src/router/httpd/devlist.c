/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <wlutils.h>


void asp_arplist(int argc, char **argv)
{
	FILE *f;
	char s[512];
	char ip[16];
	char mac[18];
	char dev[17];
	char comma;
	unsigned int flags;

	/*
		cat /proc/net/arp
		IP address       HW type     Flags       HW address            Mask     Device
		192.168.0.1      0x1         0x2         00:01:02:03:04:05     *        vlan1
	*/

	web_puts("\narplist = [");
	comma = ' ';
	if ((f = fopen("/proc/net/arp", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%15s %*s 0x%X %17s %*s %16s", ip, &flags, mac, dev) != 4) continue;
			if ((strlen(mac) != 17) || (strcmp(mac, "00:00:00:00:00:00") == 0)) continue;
			if (flags == 0) continue;
			if ( nvram_match( "devlist_hidewan", "1" ) && nvram_match( "wan_ifname", dev ) && ( !nvram_match( "wan_ipaddr", ip ))) continue; // half
			web_printf("%c['%s','%s','%s']", comma, ip, mac, dev);
			comma = ',';
		}
		fclose(f);
	}
	web_puts("];\n");
}

// checkme: any easier way to do this?	zzz
static int get_wds_ifname(const struct ether_addr *ea, char *ifname)
{
	struct ifreq ifr;
	int sd;
	int i;
	struct ether_addr e;

	if ((sd = socket(PF_INET, SOCK_DGRAM, 0)) >= 0) {
		// wds doesn't show up under SIOCGIFCONF; seems to start at 17 (?)
		for (i = 1; i < 32; ++i) {
			ifr.ifr_ifindex = i;
			if ((ioctl(sd, SIOCGIFNAME, &ifr) == 0) &&
				(strncmp(ifr.ifr_name, "wds", 3) == 0) &&
				(wl_ioctl(ifr.ifr_name, WLC_WDS_GET_REMOTE_HWADDR, &e.octet, sizeof(e.octet)) == 0)) {
				if (memcmp(ea->octet, e.octet, sizeof(e.octet)) == 0) {
					close(sd);
					strlcpy(ifname, ifr.ifr_name, 16);
					return 1;
				}
			}
		}
		close(sd);
	}
	return 0;
}

static int get_wl_clients(int idx, int unit, int subunit, void *param)
{
	char *comma = param;
	int i;
	char *p;
	char buf[32];
#if 1
	char *wlif;
	scb_val_t rssi;
	sta_info_t sti;
	int cmd;
	struct maclist *mlist;
	int mlsize;
	char ifname[16];

	mlsize = sizeof(struct maclist) + (255 * sizeof(struct ether_addr));
	if ((mlist = malloc(mlsize)) != NULL) {
//		wlif = nvram_safe_get(wl_nvname("ifname", unit, 0));
		wlif = nvram_safe_get(wl_nvname("ifname", unit, subunit)); // AB multiSSID
		cmd = WLC_GET_ASSOCLIST;
		while (1) {
			mlist->count = 255;
			if (wl_ioctl(wlif, cmd, mlist, mlsize) == 0) {
				for (i = 0; i < mlist->count; ++i) {
					rssi.ea = mlist->ea[i];
					rssi.val = 0;
					if (wl_ioctl(wlif, WLC_GET_RSSI, &rssi, sizeof(rssi)) != 0) continue;

					// sta_info0<mac>
					memset(&sti, 0, sizeof(sti));
					strcpy((char *)&sti, "sta_info");
					memcpy((char *)&sti + 9, rssi.ea.octet, 6);
					if (wl_ioctl(wlif, WLC_GET_VAR, &sti, sizeof(sti)) != 0) continue;

					p = wlif;
					if (sti.flags & WL_STA_WDS) {
						if (cmd != WLC_GET_WDSLIST) continue;
						if ((sti.flags & WL_WDS_LINKUP) == 0) continue;
						if (get_wds_ifname(&rssi.ea, ifname)) p = ifname;
					}

					web_printf("%c['%s','%s',%d,%d,%d,%u,%d]",
						*comma,
						p,
						ether_etoa(rssi.ea.octet, buf),
						rssi.val,
						sti.tx_rate, sti.rx_rate, sti.in, unit);
					*comma = ',';
				}
			}
			if (cmd == WLC_GET_WDSLIST) break;
			cmd = WLC_GET_WDSLIST;
		}
		free(mlist);
	}
#else
	char *wlif;
	scb_val_t rssi;
	sta_info_t sti;
	int j;
	struct maclist *mlist;
	int mlsize;
	char ifname[16];

	mlsize = sizeof(struct maclist) + (127 * sizeof(struct ether_addr));
	if ((mlist = malloc(mlsize)) != NULL) {
		for (j = 0; j < 2; ++j) {
			wlif = nvram_safe_get("wl0_ifname");
			strcpy((char *)mlist, j ? "autho_sta_list" : "authe_sta_list");
			if (wl_ioctl(wlif, WLC_GET_VAR, mlist, mlsize) == 0) {
				for (i = 0; i < mlist->count; ++i) {
					rssi.ea = mlist->ea[i];
					rssi.val = 0;
					if (wl_ioctl(wlif, WLC_GET_RSSI, &rssi, sizeof(rssi)) != 0) continue;

					// sta_info0<mac>
					memset(&sti, 0, sizeof(sti));
					strcpy((char *)&sti, "sta_info");
					memcpy((char *)&sti + 9, rssi.ea.octet, 6);
					if (wl_ioctl(wlif, WLC_GET_VAR, &sti, sizeof(sti)) != 0) continue;

					p = wlif;
					if (sti.flags & WL_STA_WDS) {
						if ((sti.flags & WL_WDS_LINKUP) == 0) continue;
						if (get_wds_ifname(&rssi.ea, ifname)) p = ifname;
					}

					web_printf("%c['%s','%s',%d]",
						*comma,
						p,
						ether_etoa(rssi.ea.octet, buf),
						rssi.val);
					*comma = ',';
				}
			}
		}
		free(mlist);
	}
#endif

	return 0;
}

void asp_devlist(int argc, char **argv)
{
	char *p;
	FILE *f;
	char buf[1024];
	char comma;

	// must be here for easier call via update.cgi. arg is ignored
	asp_arplist(0, NULL);
	asp_wlnoise(0, NULL);

	//

	p = js_string(nvram_safe_get("dhcpd_static"));
	web_printf("dhcpd_static = '%s'.split('>');\n", p ? p : "");
	free(p);

	//

	web_puts("wldev = [");
	comma = ' ';
	foreach_wif(1, &comma, get_wl_clients);
	web_puts("];\n");

	//

	unsigned long expires;
	char mac[32];
	char ip[40];
	char hostname[256];
	char *host;

	web_puts("dhcpd_lease = [");
	if ((nvram_match("lan_proto", "dhcp")) || (nvram_match("lan1_proto", "dhcp")) || (nvram_match("lan2_proto", "dhcp")) || (nvram_match("lan3_proto", "dhcp")) ) {
		f_write("/var/tmp/dhcp/leases.!", NULL, 0, 0, 0666);

		// dump the leases to a file
		if (killall("dnsmasq", SIGUSR2) == 0) {
			// helper in dnsmasq will remove this when it's done
			f_wait_notexists("/var/tmp/dhcp/leases.!", 5);
		}

		if ((f = fopen("/var/tmp/dhcp/leases", "r")) != NULL) {
			comma = ' ';
			while (fgets(buf, sizeof(buf), f)) {
				if (sscanf(buf, "%lu %17s %39s %255s", &expires, mac, ip, hostname) != 4) continue;
				host = js_string((hostname[0] == '*') ? "" : hostname);
				web_printf("%c['%s','%s','%s','%s']", comma,
						(host ? host : ""), ip, mac, ((expires == 0) ? "non-expiring" : reltime(buf, expires)));
				free(host);
				comma = ',';
			}
			fclose(f);
		}
		unlink("/var/tmp/dhcp/leases");
	}
	web_puts("];");
}
