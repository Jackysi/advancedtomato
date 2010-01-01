/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

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
	memset(&sp.bssid, 0xff, sizeof(sp.bssid));		//!!TB
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

		eval("wl", "up"); //!!TB - without this the router may reboot
#if WL_BSS_INFO_VERSION >= 108
		//!!TB - it seems that the new WL driver needs another voodoo sequence
		eval("wl", "ssid", "");

		// no idea why this voodoo sequence works to wake up wl	-- zzz
		eval("wl", "ssid", nvram_safe_get("wl_ssid"));
		if (radio) set_radio(1);
#endif
	}
	//!!TB if (!radio)
	set_radio(radio);
	
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
		if (CHSPEC_IS40(bssi->chanspec))
			channel = channel + (CHSPEC_SB_LOWER(bssi->chanspec) ? -2 : 2);
#else
		channel = bssi->channel;
#endif

		j = bssi->SSID_len;
		if (j < 0) j = 0;
		if (j > 32) j = 32;
		if (nvram_get_int("wlx_scrubssid")) {
			for (k = j - 1; k >= 0; --k) {
				c = bssi->SSID[k];
				if (!isprint(c)) c = '?';
				ssid[k] = c;
			}
		}
		else {
			memcpy(ssid, bssi->SSID, j);
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
	
	// WLC_GET_PHY_NOISE = 135
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

	int radio = get_radio();	//!!TB

	wif = nvram_safe_get("wl_ifname");
	if (wl_ioctl(wif, WLC_GET_AP, &ap, sizeof(ap)) < 0) return;

	i = 0;
	wl_ioctl(wif, WLC_SET_AP, &i, sizeof(i));

	for (i = 10; i > 0; --i) {
		sleep(1);
		read_noise();
	}
	
	wl_ioctl(wif, WLC_SET_AP, &ap, sizeof(ap));

	//!!TB - again, the same voodoo sequence seems to be needed for new WL driver
	if (!radio) set_radio(1);
	eval("wl", "up");
#if WL_BSS_INFO_VERSION >= 108
	eval("wl", "ssid", "");
	eval("wl", "ssid", nvram_safe_get("wl_ssid"));
#endif
	set_radio(radio);
}

void asp_wlclient(int argc, char **argv)
{
	web_puts(wl_client() ? "1" : "0");
}

void asp_wlnbw(int argc, char **argv)
{
	int chanspec;

	if (wl_iovar_getint(nvram_safe_get("wl_ifname"), "chanspec", &chanspec))
		web_puts(nvram_safe_get("wl_nbw"));
	else
		web_printf("%d", CHSPEC_IS40(chanspec) ? 40 : 20);
}

void asp_wlnctrlsb(int argc, char **argv)
{
	int chanspec;

	if (wl_iovar_getint(nvram_safe_get("wl_ifname"), "chanspec", &chanspec))
		web_puts(nvram_safe_get("wl_nctrlsb"));
	else
		web_puts(CHSPEC_SB_LOWER(chanspec) ? "lower" : CHSPEC_SB_UPPER(chanspec) ? "upper" : "none");
}

static int wl_chanfreq(uint ch, int band)
{
	if ((band == WLC_BAND_2G && (ch < 1 || ch > 14)) || (ch > 200))
		return -1;
	else if ((band == WLC_BAND_2G) && (ch == 14))
		return 2484;
	else
		return ch * 5 + ((band == WLC_BAND_2G) ? 4814 : 10000) / 2;
}

void asp_wlchannel(int argc, char **argv)
{
	channel_info_t ch;
	int chanspec, channel;
	int phytype, band, scan = 0;

	char *ifname = nvram_safe_get("wl_ifname");

	/* Get configured phy type */
	wl_ioctl(ifname, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));
	wl_ioctl(ifname, WLC_GET_BAND, &band, sizeof(band));

	channel = nvram_get_int("wl_channel");
	if (phytype != WLC_PHY_TYPE_N) {
		if (wl_ioctl(ifname, WLC_GET_CHANNEL, &ch, sizeof(ch)) == 0) {
			scan = (ch.scan_channel > 0);
			channel = (scan) ? ch.scan_channel : ch.hw_channel;
		}
	} else {
		if (wl_iovar_getint(ifname, "chanspec", &chanspec) == 0) {
			channel = CHSPEC_CHANNEL(chanspec);
			if (CHSPEC_IS40(chanspec))
				channel = channel + (CHSPEC_SB_LOWER(chanspec) ? -2 : 2);
		}
	}

	if (argc == 0) {
		int mhz;
		if (channel == 0)
			web_puts("Auto");
		else if ((mhz = wl_chanfreq(channel, band)) > 0)
			web_printf("%d - %.3f <small>GHz</small>%s", channel, mhz / 1000.0,
				(scan) ? " <small>(scanning...)</small>" : "");
		else
			web_printf("%d%s", channel,
				(scan) ? " <small>(scanning...)</small>" : "");
	}
	else
		web_printf("%s%d", scan ? "-" : "", channel);
}

static void web_print_wlchan(uint chan, int band)
{
	int mhz;
	if ((mhz = wl_chanfreq(chan, band)) > 0)
		web_printf(",['%d', '%d - %.3f GHz']", chan, chan, mhz / 1000.0);
	else
		web_printf(",['%d', '%d']", chan, chan);
}

static void _wlchanspecs(char *ifname, char *country, int band, int bw, int ctrlsb)
{
	chanspec_t c = 0, *chanspec;
	int buflen;
	wl_uint32_list_t *list;
	int i = 0;

	char *buf = (char *)malloc(WLC_IOCTL_MAXLEN);
	if (!buf)
		return;

	strcpy(buf, "chanspecs");
	buflen = strlen(buf) + 1;

	c |= (band == WLC_BAND_5G) ? WL_CHANSPEC_BAND_5G : WL_CHANSPEC_BAND_2G;
	c |= (bw == 20) ? WL_CHANSPEC_BW_20 : WL_CHANSPEC_BW_40;

	chanspec = (chanspec_t *)(buf + buflen);
	*chanspec = c;
	buflen += (sizeof(chanspec_t));
	strncpy(buf + buflen, country, WLC_CNTRY_BUF_SZ);
	buflen += WLC_CNTRY_BUF_SZ;

	/* Add list */
	list = (wl_uint32_list_t *)(buf + buflen);
	list->count = WL_NUMCHANSPECS;
	buflen += sizeof(uint32)*(WL_NUMCHANSPECS + 1);

	if (wl_ioctl(ifname, WLC_GET_VAR, buf, buflen) < 0) {
		free((void *)buf);
		return;
	}

	list = (wl_uint32_list_t *)buf;
	for (i = 0; i < list->count; i++) {
		c = (chanspec_t)list->element[i];
		/* Skip upper.. (take only one of the lower or upper) */
		if (bw == 40 && (CHSPEC_CTL_SB(c) != ctrlsb))
			continue;
		/* Create the actual control channel number from sideband */
		int chan = CHSPEC_CHANNEL(c);
		if (bw == 40)
			chan += ((ctrlsb == WL_CHANSPEC_CTL_SB_UPPER) ? 2 : -2);
		web_print_wlchan(chan, band);
	}

	free((void *)buf);
}

static void _wlchannels(char *ifname, char *country, int band)
{
	int i;
	wl_channels_in_country_t *cic;

	cic = (wl_channels_in_country_t *)malloc(WLC_IOCTL_MAXLEN);
	if (cic) {
		cic->buflen = WLC_IOCTL_MAXLEN;
		strcpy(cic->country_abbrev, country);
		cic->band = band;

		if (wl_ioctl(ifname, WLC_GET_CHANNELS_IN_COUNTRY, cic, cic->buflen) == 0) {
			for (i = 0; i < cic->count; i++) {
				web_print_wlchan(cic->channel[i], band);
			}
		}
		free((void *)cic);
	}
}

void asp_wlchannels(int argc, char **argv)
{
	char buf[WLC_CNTRY_BUF_SZ];
	int band, phytype, nphy;
	int bw, ctrlsb, chanspec;
	char *ifname = nvram_safe_get("wl_ifname");

	wl_ioctl(ifname, WLC_GET_COUNTRY, buf, sizeof(buf));
	wl_ioctl(ifname, WLC_GET_BAND, &band, sizeof(band));
	wl_ioctl(ifname, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));
	wl_iovar_getint(ifname, "chanspec", &chanspec);

	if (argc > 0)
		nphy = atoi(argv[0]);
	else
		nphy = (phytype == WLC_PHY_TYPE_N);
	if (argc > 1)
		bw = atoi(argv[1]);
	else
		bw = CHSPEC_IS40(chanspec) ? 40 : 20;
	if (argc > 2) {
		if (strcmp(argv[2], "upper") == 0)
			ctrlsb = WL_CHANSPEC_CTL_SB_UPPER;
		else
			ctrlsb = WL_CHANSPEC_CTL_SB_LOWER;
	}
	else
		ctrlsb = CHSPEC_CTL_SB(chanspec);

	web_puts("\nwl_channels = [\n['0', 'Auto']");
	if (nphy && (phytype == WLC_PHY_TYPE_N))
		_wlchanspecs(ifname, buf, band, bw, ctrlsb);
	else
		_wlchannels(ifname, buf, band);
	web_puts("];\n");
}

#if 0
void asp_wlcountries(int argc, char **argv)
{
	char *js, s[128], code[15], country[64];
	FILE *f;
	int i = 0;

	web_puts("\nwl_countries = [\n");
	if ((f = popen("wl country list", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%s %s", code, country) == 2) {
				// skip all bogus country names
				if (strlen(code) < 5 && strcmp(code, country) != 0) {
					js = js_string(strstr(s, country));
					web_printf("%c['%s', '%s']", i == 0 ? ' ' : ',', code, js);
					free(js);
					i++;
				}
			}
		}
		fclose(f);
	}
	web_puts("];\n");
}
#endif
