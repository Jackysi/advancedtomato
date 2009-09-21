/*
 * Misc utility routines used by kernel or app-level.
 * Contents are wifi-specific, used by any kernel or app-level
 * software that might want wifi things as it grows.
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 * $Id$
 */

#include <typedefs.h>

#ifdef BCMDRIVER
#include <osl.h>
#include <bcmutils.h>
#define strtoul(nptr, endptr, base) bcm_strtoul((nptr), (endptr), (base))
#define tolower(c) (bcm_isupper((c)) ? ((c) + 'a' - 'A') : (c))
#elif defined(__IOPOS__)
#include <bcmutils.h>
#define strtoul(nptr, endptr, base) bcm_strtoul((nptr), (endptr), (base))
#define tolower(c) (bcm_tolower((c)))
#else
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#endif /* BCMDRIVER */
#include <bcmwifi.h>

/* Chanspec ASCII representation:
 * <channel><band><bandwidth><ctl-sideband>
 *   digit   [AB]      N          [UL]
 *
 * <channel>: channel number of the 10MHz or 20MHz channel,
 *	or control sideband channel of 40MHz channel.
 * <band>: A for 5GHz, B for 2.4GHz
 * <bandwidth>: N for 10MHz, nothing for 20MHz or 40MHz
 *	(ctl-sideband spec implies 40MHz)
 * <ctl-sideband>: U for upper, L for lower
 *
 * <band> may be omitted on input, and will be assumed to be
 * 2.4GHz if channel number <= 14.
 *
 * Examples: ...
 */
/* given a chanspec and a string buffer, format the chanspec as a
 * string, and return the original pointer a. On error, return's NULL
 */
char *
wf_chspec_ntoa(chanspec_t chspec, char *buf)
{
	const char *band, *bw, *sb;
	uint channel;

	bw = "";
	sb = "";
	channel = CHSPEC_CHANNEL(chspec);
	band = (CHSPEC_IS2G(chspec)) ? "b" : "a";
	if (CHSPEC_IS40(chspec)) {
		if (CHSPEC_SB_UPPER(chspec)) {
			sb = "u";
			channel += CH_10MHZ_APART;
		} else {
			sb = "l";
			channel -= CH_10MHZ_APART;
		}
	} else if (CHSPEC_IS10(chspec)) {
		bw = "n";
	}

	sprintf(buf, "%d%s%s%s", channel, band, bw, sb);
	return (buf);
}

/* given a chanspec string, convert to a chanspec.
 * On error, return 0
 */

chanspec_t
wf_chspec_aton(char *a)
{
	char *endp;
	uint channel, band, bw, ctl_sb;
	bool band_set = FALSE, bw_set = FALSE, ctl_sb_set = FALSE;
	int error = 0;

	channel = strtoul(a, &endp, 10);
	if (endp == a)
		return 0;

	if (channel > MAXCHANNEL)
		return 0;

	band = ((channel <= WLC_MAX_2G_CHANNEL) ? WL_CHANSPEC_BAND_2G : WL_CHANSPEC_BAND_5G);
	bw = WL_CHANSPEC_BW_20;
	ctl_sb = WL_CHANSPEC_CTL_SB_NONE;

	a = endp;
	while (*a != 0 && error != -1) {
		switch (tolower((int)*a)) {
			case 'a':
			case 'b':
				if (!band_set) {
					band = (tolower((int)*a) == 'a') ?
					       WL_CHANSPEC_BAND_5G : WL_CHANSPEC_BAND_2G;
					band_set = TRUE;
				} else {
					error = -1;
				}
				break;
			case 'n':
				if (!bw_set) {
					bw = WL_CHANSPEC_BW_10;
					bw_set = TRUE;
				} else {
					error = -1;
				}
				break;
			case 'l':
			case 'u':
				if (!ctl_sb_set && !bw_set) {
					ctl_sb = (tolower((int)*a) == 'l') ?
						WL_CHANSPEC_CTL_SB_LOWER : WL_CHANSPEC_CTL_SB_UPPER;
					ctl_sb_set = TRUE;
					if (ctl_sb == WL_CHANSPEC_CTL_SB_LOWER)
						channel = UPPER_20_SB(channel);
					else
						channel = LOWER_20_SB(channel);
					bw = WL_CHANSPEC_BW_40;
					bw_set = TRUE;
				} else if (bw_set) {
					error = -1;
				} else {
					error = -1;
				}
				break;
			default:
				error = -1;
				break;
		}
		a++;
	}

	if (bw_set && (bw == WL_CHANSPEC_BW_40) && !ctl_sb_set)
		error = -1;

	if (ctl_sb_set && !bw_set)
		error = -1;

	if (!error)
		return ((channel | band | bw | ctl_sb));

	return 0;
}

#ifdef CONFIG_NET_RADIO
/* channel info structure */
typedef struct {
	uint	chan;		/* channel number */
	uint	freq;		/* in Mhz */
} chan_info_t;

static chan_info_t chan_info[] = {
	/* B channels */
	{ 1,	2412},
	{ 2,	2417},
	{ 3,	2422},
	{ 4,	2427},
	{ 5,	2432},
	{ 6,	2437},
	{ 7,	2442},
	{ 8,	2447},
	{ 9,	2452},
	{ 10,	2457},
	{ 11,	2462},
	{ 12,	2467},
	{ 13,	2472},
	{ 14,	2484},

	/* A channels */
	/* 11a usa low */
	{ 36,	5180},
	{ 40,	5200},
	{ 44,	5220},
	{ 48,	5240},
	{ 52,	5260},
	{ 56,	5280},
	{ 60,	5300},
	{ 64,	5320},

	/* 11a Europe */
	{ 100,	5500},
	{ 104,	5520},
	{ 108,	5540},
	{ 112,	5560},
	{ 116,	5580},
	{ 120,	5600},
	{ 124,	5620},
	{ 128,	5640},
	{ 132,	5660},
	{ 136,	5680},
	{ 140,	5700},

	/* 11a usa high */
	{ 149,	5745},
	{ 153,	5765},
	{ 157,	5785},
	{ 161,	5805},

	/* 11a japan */
	{ 184,	4920},
	{ 188,	4940},
	{ 192,	4960},
	{ 196,	4980},
	{ 200,	5000},
	{ 204,	5020},
	{ 208,	5040},
	{ 212,	5060},
	{ 216,	5080}
};


uint
freq2channel(uint freq)
{
	int i;

	for (i = 0; i < (int)ARRAYSIZE(chan_info); i++) {
		if (chan_info[i].freq == freq)
			return (chan_info[i].chan);
	}
	return (0);
}

uint
channel2freq(uint channel)
{
	uint i;

	for (i = 0; i < ARRAYSIZE(chan_info); i++)
		if (chan_info[i].chan == channel)
			return (chan_info[i].freq);
	return (0);
}
#endif /* CONFIG_NET_RADIO */
