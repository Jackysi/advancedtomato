/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>
#include <wlutils.h>


//	returns: ['bssid','ssid',channel,capabilities,rssi,noise,[rates,]],  or  [null,'error message']
void asp_wlscan(int argc, char **argv)
{
	// scan

	wl_scan_params_t sp;
	int ap;
	int radio;
	char *wif;
	int zero = 0;

	web_puts("\nwlscandata = [");

	wif = nvram_safe_get("wl_ifname");

	memset(&sp, 0xff, sizeof(sp));		// most default to -1
	sp.ssid.SSID_len = 0;
	sp.bss_type = DOT11_BSSTYPE_ANY;	// =2
	sp.channel_num = 0;

	if (wl_ioctl(wif, WLC_GET_AP, &ap, sizeof(ap)) < 0) {
		web_puts("[null,'Unable to get AP mode.']];\n");
		return;
	}
	if (ap > 0) {
		wl_ioctl(wif, WLC_SET_AP, &zero, sizeof(zero));
	}
	
	radio = get_radio();
	if (!radio) set_radio(1);
	
	if (wl_ioctl(wif, WLC_SCAN, &sp, WL_SCAN_PARAMS_FIXED_SIZE) < 0) {
		if (ap > 0) wl_ioctl(wif, WLC_SET_AP, &ap, sizeof(ap));
		if (!radio) set_radio(0);
		web_puts("[null,'Unable to start scan.']];\n");
		return;
	}

	sleep(2);


	// get results

	wl_scan_results_t *results;
	wl_bss_info_t *bssi;
	int r;

	results = malloc(WLC_IOCTL_MAXLEN);
	if (!results) {
		if (ap > 0) wl_ioctl(wif, WLC_SET_AP, &ap, sizeof(ap));
		if (!radio) set_radio(0);
		web_puts("[null,'Not enough memory.']];");
		return;
	}
	results->buflen = WLC_IOCTL_MAXLEN - (sizeof(*results) - sizeof(results->bss_info));
	results->version = WL_BSS_INFO_VERSION;
	r = wl_ioctl(wif, WLC_SCAN_RESULTS, results, WLC_IOCTL_MAXLEN);

	if (ap > 0) {
		wl_ioctl(wif, WLC_SET_AP, &ap, sizeof(ap));
#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
		// no idea why this voodoo sequence works to wake up wl	-- zzz
		eval("wl", "ssid", nvram_safe_get("wl_ssid"));
		if (radio) set_radio(1);
#endif
	}
	if (!radio) set_radio(0);
	
	if (r < 0) {
		free(results);
		web_puts("[null,'Unable to obtain scan result.']];\n");
		return;
	}


	// format for javascript

	int i;
	int j;
	int k;
	char c;
	char ssid[64];
	char mac[32];
	char *ssidj;
	int channel;
	
	bssi = &results->bss_info[0];
	for (i = 0; i < results->count; ++i) {

#if WL_BSS_INFO_VERSION >= 108
		channel = CHSPEC_CHANNEL(bssi->chanspec);
#else
		channel = bssi->channel;
#endif

		// scrub ssid
		j = bssi->SSID_len;
		if (j < 0) j = 0;
		if (j > 32) j = 32;
		for (k = j - 1; k >= 0; --k) {
			c = bssi->SSID[k];
			if (!isprint(c)) c = '?';
			ssid[k] = c;
		}
		ssid[j] = 0;
		
		ssidj = js_string(ssid);
		web_printf("%s['%s','%s',%u,%u,%d,%d,[", (i > 0) ? "," : "",
			ether_etoa(bssi->BSSID.octet, mac), ssidj ? ssidj : "",
			channel,
			bssi->capability, bssi->RSSI, bssi->phy_noise);
		free(ssidj);
		
		for (j = 0; j < bssi->rateset.count; ++j) {
			web_printf("%s%u", j ? "," : "", bssi->rateset.rates[j]);
		}
		web_puts("]]");

		bssi = (wl_bss_info_t*)((uint8*)bssi + bssi->length);
	}

	web_puts("];\n");
	free(results);
}


void asp_wlradio(int argc, char **argv)
{
	web_printf("\nwlradio = %d;\n", get_radio());
}

void wo_wlradio(char *url)
{
	char *enable;
	
	parse_asp("saved.asp");
	if (nvram_match("wl_radio", "1")) {
		if ((enable = webcgi_get("enable")) != NULL) {
			web_close();
			sleep(2);
			eval("radio", atoi(enable) ? "on" : "off");
			return;
		}
	}
}

static int read_noise(void)
{
	int v;
	
	// WLC_GET_PHY_NOISE
	if (wl_ioctl(nvram_safe_get("wl_ifname"), 135, &v, sizeof(v)) == 0) {
		char s[32];
		sprintf(s, "%d", v);
		nvram_set("t_noise", s);
		return v;
	}
	return -99;
}

void asp_wlcrssi(int argc, char **argv)
{
	scb_val_t rssi;

	memset(&rssi, 0, sizeof(rssi));
	if (wl_client()) {
		if (wl_ioctl(nvram_safe_get("wl_ifname"), WLC_GET_RSSI, &rssi, sizeof(rssi)) != 0)
			rssi.val = -100;
	}
	web_printf("\nwlcrssi = %d;\n", rssi.val);
}

void asp_wlnoise(int argc, char **argv)
{
	int v;
	
	if (wl_client()) {
		v = read_noise();
	}
	else {
		v = nvram_get_int("t_noise");
		if ((v > 0) || (v < -100)) v = -99;
	}
	web_printf("\nwlnoise = %d;\n", v);
}

void wo_wlmnoise(char *url)
{
	int ap;
	int i;
	char *wif;
	
	parse_asp("mnoise.asp");
	web_close();
	sleep(3);

	wif = nvram_safe_get("wl_ifname");
	if (wl_ioctl(wif, WLC_GET_AP, &ap, sizeof(ap)) < 0) return;

	i = 0;
	wl_ioctl(wif, WLC_SET_AP, &i, sizeof(i));
	
	for (i = 10; i > 0; --i) {
		sleep(1);
		read_noise();
	}
	
	wl_ioctl(wif, WLC_SET_AP, &ap, sizeof(ap));	
}

void asp_wlclient(int argc, char **argv)
{
	web_puts(wl_client() ? "1" : "0");
}

void asp_wlchannel(int argc, char **argv)
{
	channel_info_t ch;
	
	if (wl_ioctl(nvram_safe_get("wl_ifname"), WLC_GET_CHANNEL, &ch, sizeof(ch)) < 0) {
		web_puts(nvram_safe_get("wl_channel"));
	}
	else {
		web_printf("%d", (ch.scan_channel > 0) ? -ch.scan_channel : ch.hw_channel);
	}
}
