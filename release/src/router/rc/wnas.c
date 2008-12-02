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

#include "rc.h"

#include <sys/sysinfo.h>
#include <bcmutils.h>
#include <wlutils.h>


//	ref: http://wiki.openwrt.org/OpenWrtDocs/nas

//	#define DEBUG_TIMING

void start_nas(void)
{
	mode_t m;

	if ((nvram_match("wl_mode", "wet")) || (nvram_match("wl0_radio", "0")) ||
		(nvram_match("security_mode", "disabled"))) {
		return;
	}

#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	_dprintf("%s: uptime=%ld\n", __FUNCTION__, si.uptime);
#else
	_dprintf("%s\n", __FUNCTION__);
#endif	

	m = umask(0077);
	xstart("nas", "/etc/nas.conf", "/var/run/nas.pid", nvram_match("wl_mode", "sta") ? "wan" : "lan");
	umask(m);
}

void stop_nas(void)
{
#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	_dprintf("%s: uptime=%ld\n", __FUNCTION__, si.uptime);
#else
	_dprintf("%s\n", __FUNCTION__);
#endif

	killall("nas", SIGTERM);
}

void notify_nas(const char *ifname)
{
#ifdef DEBUG_TIMING
	struct sysinfo si;
	sysinfo(&si);
	_dprintf("%s: ifname=%s uptime=%ld\n", __FUNCTION__, ifname, si.uptime);
#else
	_dprintf("%s: ifname=%s\n", __FUNCTION__, ifname);
#endif

	if (nvram_match("security_mode", "disabled")) return;
	
	int i;

	i = 10;
	while (pidof("nas") == -1) {
		_dprintf("waiting for nas i=%d\n", i);
		if (--i < 0) {
			syslog(LOG_ERR, "Unable to find nas");
			break;
		}
		sleep(1);
	}
	sleep(5);

	xstart("nas4not", "lan", ifname, "up", "auto",
		nvram_safe_get("wl_crypto"),	// aes, tkip (aes+tkip ok?)
		nvram_safe_get("wl_akm"),		// psk (only?)
		nvram_safe_get("wl_wpa_psk"),	// shared key
		nvram_safe_get("wl_ssid")		// ssid
	);
}




#if 0	// old stuff for ref


/*
void del_wds_wsec(int unit, int which)
{
	char name[32];

	sprintf(name, "wl%d_wds%d", unit, which);
	nvram_unset(name);
}
*/


/*
	xstart("nas",
		nvram_match("wl_mode", "sta") ? "-S" : "-A",
		"-H", "34954",
		"-i", nvram_safe_get("wl_ifname"),
		"-l", nvram_safe_get("lan_ifname"),
		"
#
*/

	// WPA doesn't support shared key		removed, handled during config set zzz
//	if (strstr(nvram_safe_get("security_mode2"), "wpa") != NULL) {
//		nvram_set("wl_auth", "0");
//	}

//	if ((nvram_match("wl_mode", "sta")) && (nvram_match("wl_akm", "psk psk2"))) {
//		nvram_set("wl_akm", "psk2");
//	}

//	convert_wds();



/*
static int get_wds_wsec(int unit, int which, char *mac, char *role, char *crypto, char *auth, char *ssid, char *pass)
{
	char buf[512];
	char *next;

	sprintf(buf, "wl%d_wds%d", unit, which);
	strlcpy(buf, nvram_safe_get(buf), sizeof(buf));
	next = buf;

	strcpy(mac, strsep(&next, ","));
	if (!next) return 0;

	strcpy(role, strsep(&next, ","));
	if (!next) return 0;

	strcpy(crypto, strsep(&next, ","));
	if (!next) return 0;

	strcpy(auth, strsep(&next, ","));
	if (!next) return 0;

	strcpy(ssid, strsep(&next, ","));
	if (!next) return 0;

	strcpy(pass, next);
	return 1;
}

static void convert_wds(void)
{
	char wds_mac[254];
	char buf[254];

	if (nvram_match("wl_wds", "")) {	// For Router, accept all WDS link
		strcpy(wds_mac, "*");
	}
	else {								// For AP, assign remote WDS MAC
		strlcpy(wds_mac, nvram_safe_get("wl_wds"), sizeof(wds_mac));
	}


	// For WPA-PSK mode, we want to convert wl_wds_mac to wl0_wds0 ... wl0_wds255
	if (strstr(nvram_safe_get("security_mode"), "psk")) {
		char wl_wds[32];
		int i = 0;
		int j;
		char mac[254];
		char *next;

		foreach(mac, wds_mac, next) {
			snprintf(wl_wds, sizeof(wl_wds), "wl0_wds%d", i);
			snprintf(buf, sizeof(buf), "%s,auto,%s,%s,%s,%s",
				mac,
				nvram_safe_get("wl_crypto"),
				nvram_safe_get("security_mode"),
				nvram_safe_get("wl_ssid"),
				nvram_safe_get("wl_wpa_psk"));

			nvram_set(wl_wds, buf);
			i++;
		}

		for (j = i; j < 20; j++)
			del_wds_wsec(0, j);
	}
}
*/

#if 0
void notify_nas(char *ifname)
{
	char *argv[] = {"nas4not", "lan", ifname, "up",
			NULL,	/* role */
			NULL,	/* crypto */
			NULL,	/* auth */
			NULL,	/* passphrase */
			NULL,	/* ssid */
			NULL};
	char tmp[100], prefix[32];
	int unit;
	char remote[ETHER_ADDR_LEN];
	char ssid[48], pass[80], auth[16], crypto[16], role[8];
	int i;

	/* the wireless interface must be configured to run NAS */
	wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));
	sprintf(prefix, "wl%d_", unit);
	if (nvram_match(strcat_r(prefix, "akm", tmp), "") && nvram_match(strcat_r(prefix, "auth_mode", tmp), "none")) {
		return;
	}

	// wait until nas is up and running
	char s[64];
	int r;

	r = 10;
	while (f_read("/tmp/nas.lan.pid", s, sizeof(s)) <= 0) {
		if (--r <= 0) {
			cprintf("notify_nas: unable to find nas");
			break;
		}
		sleep(1);
	}
	sleep(3);


	/* find WDS link configuration */
	wl_ioctl(ifname, WLC_WDS_GET_REMOTE_HWADDR, remote, ETHER_ADDR_LEN);
	for (i = 0; i < 20; ++i) {
		char mac[ETHER_ADDR_STR_LEN];
		uint8 ea[ETHER_ADDR_LEN];

		if (get_wds_wsec(unit, i, mac, role, crypto, auth, ssid, pass) &&
		    ether_atoe(mac, ea) && !bcmp(ea, remote, ETHER_ADDR_LEN)) {
			argv[4] = role;
			argv[5] = crypto;
			argv[6] = auth;
			argv[7] = pass;
			argv[8] = ssid;

			cprintf("notify_nas: get_wds_wsec(%d,%d) crypto=%s", unit, i, argv[5]);
			break;
		}
	}
	/* did not find WDS link configuration, use wireless' */
	if (i == 20) {
		/* role */
		argv[4] = "auto";
		/* crypto */
		argv[5] = nvram_safe_get(strcat_r(prefix, "crypto", tmp));
		/* auth mode */
		argv[6] = nvram_safe_get(strcat_r(prefix, "akm", tmp));
		/* passphrase */
		argv[7] = nvram_safe_get(strcat_r(prefix, "wpa_psk", tmp));
		/* ssid */
		argv[8] = nvram_safe_get(strcat_r(prefix, "ssid", tmp));

		cprintf("notify_nas: i==MAX crypto=%s", argv[5]);
	}

#if 0
	char cl[512];
	cl[0] = 0;
	for (i = 0; argv[i]; ++i) {
		strcat(cl, argv[i]);
		strcat(cl, " ");
	}
	cprintf("notify_nas: %s", cl);
#endif

	int pid;
	_eval(argv, ">/dev/console", 0, &pid);
}
#endif



#endif 	// 0
