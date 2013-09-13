/*
 * Wireless Network Adapter Configuration Utility
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: wlconf.c,v 1.187.2.40 2011-02-11 22:07:17 Exp $
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <bcmparams.h>
#include <bcmdevs.h>
#include <shutils.h>
#include <wlutils.h>
#include <wlioctl.h>
#include <proto/802.1d.h>
#include <shared.h>

#ifndef BCM4331_CHIP_ID
#define BCM4331_CHIP_ID	0x4331	/* 4331 chipcommon chipid */
#endif

/* phy types */
#define	PHY_TYPE_A		0
#define	PHY_TYPE_B		1
#define	PHY_TYPE_G		2
#define	PHY_TYPE_N		4
#define	PHY_TYPE_LP		5
#define PHY_TYPE_SSN		6
#define	PHY_TYPE_HT		7
#define	PHY_TYPE_LCN		8
#define	PHY_TYPE_NULL		0xf

/* how many times to attempt to bring up a virtual i/f when
 * we are in APSTA mode and IOVAR set of "bss" "up" returns busy
 */
#define MAX_BSS_UP_RETRIES 5

/* notify the average dma xfer rate (in kbps) to the driver */
#define AVG_DMA_XFER_RATE 120000

/* parts of an idcode: */
#define	IDCODE_MFG_MASK		0x00000fff
#define	IDCODE_MFG_SHIFT	0
#define	IDCODE_ID_MASK		0x0ffff000
#define	IDCODE_ID_SHIFT		12
#define	IDCODE_REV_MASK		0xf0000000
#define	IDCODE_REV_SHIFT	28

/* Length of the wl capabilities string */
#define CAP_STR_LEN 250

/*
 * Debugging Macros
 */
#ifdef BCMDBG
#define WLCONF_DBG(fmt, arg...)	printf("%s: "fmt, __FUNCTION__ , ## arg)
#define WL_IOCTL(ifname, cmd, buf, len)					\
	if ((ret = wl_ioctl(ifname, cmd, buf, len)))			\
		fprintf(stderr, "%s:%d:(%s): %s failed, err = %d\n",	\
		        __FUNCTION__, __LINE__, ifname, #cmd, ret);
#define WL_SETINT(ifname, cmd, val)								\
	if ((ret = wlconf_setint(ifname, cmd, val)))						\
		fprintf(stderr, "%s:%d:(%s): setting %s to %d (0x%x) failed, err = %d\n",	\
		        __FUNCTION__, __LINE__, ifname, #cmd, (int)val, (unsigned int)val, ret);
#define WL_GETINT(ifname, cmd, pval)								\
	if ((ret = wlconf_getint(ifname, cmd, pval)))						\
		fprintf(stderr, "%s:%d:(%s): getting %s failed, err = %d\n",			\
		        __FUNCTION__, __LINE__, ifname, #cmd, ret);
#define WL_IOVAR_SET(ifname, iovar, param, paramlen)					\
	if ((ret = wl_iovar_set(ifname, iovar, param, paramlen)))			\
		fprintf(stderr, "%s:%d:(%s): setting iovar \"%s\" failed, err = %d\n",	\
		        __FUNCTION__, __LINE__, ifname, iovar, ret);
#define WL_IOVAR_SETINT(ifname, iovar, val)							\
	if ((ret = wl_iovar_setint(ifname, iovar, val)))					\
		fprintf(stderr, "%s:%d:(%s): setting iovar \"%s\" to 0x%x failed, err = %d\n",	\
		        __FUNCTION__, __LINE__, ifname, iovar, (unsigned int)val, ret);
#define WL_IOVAR_GETINT(ifname, iovar, val)							\
	if ((ret = wl_iovar_getint(ifname, iovar, val)))					\
		fprintf(stderr, "%s:%d:(%s): getting iovar \"%s\" failed, err = %d\n",	\
		        __FUNCTION__, __LINE__, ifname, iovar, ret);
#define WL_BSSIOVAR_SETBUF(ifname, iovar, bssidx, param, paramlen, buf, buflen)			\
	if ((ret = wl_bssiovar_setbuf(ifname, iovar, bssidx, param, paramlen, buf, buflen)))	\
		fprintf(stderr, "%s:%d:(%s): setting bsscfg #%d iovar \"%s\" failed, err = %d\n", \
		        __FUNCTION__, __LINE__, ifname, bssidx, iovar, ret);
#define WL_BSSIOVAR_SET(ifname, iovar, bssidx, param, paramlen)					\
	if ((ret = wl_bssiovar_set(ifname, iovar, bssidx, param, paramlen)))			\
		fprintf(stderr, "%s:%d:(%s): setting bsscfg #%d iovar \"%s\" failed, err = %d\n", \
		        __FUNCTION__, __LINE__, ifname, bssidx, iovar, ret);
#define WL_BSSIOVAR_GET(ifname, iovar, bssidx, param, paramlen)					\
	if ((ret = wl_bssiovar_get(ifname, iovar, bssidx, param, paramlen)))			\
		fprintf(stderr, "%s:%d:(%s): getting bsscfg #%d iovar \"%s\" failed, err = %d\n", \
		        __FUNCTION__, __LINE__, ifname, bssidx, iovar, ret);
#define WL_BSSIOVAR_SETINT(ifname, iovar, bssidx, val)						\
	if ((ret = wl_bssiovar_setint(ifname, iovar, bssidx, val)))				\
		fprintf(stderr, "%s:%d:(%s): setting bsscfg #%d iovar \"%s\" " \
				"to val 0x%x failed, err = %d\n",	\
		        __FUNCTION__, __LINE__, ifname, bssidx, iovar, (unsigned int)val, ret);
#else
#define WLCONF_DBG(fmt, arg...)
#define WL_IOCTL(name, cmd, buf, len)			(ret = wl_ioctl(name, cmd, buf, len))
#define WL_SETINT(name, cmd, val)			(ret = wlconf_setint(name, cmd, val))
#define WL_GETINT(name, cmd, pval)			(ret = wlconf_getint(name, cmd, pval))
#define WL_IOVAR_SET(ifname, iovar, param, paramlen)	(ret = wl_iovar_set(ifname, iovar, \
							param, paramlen))
#define WL_IOVAR_SETINT(ifname, iovar, val)		(ret = wl_iovar_setint(ifname, iovar, val))
#define WL_IOVAR_GETINT(ifname, iovar, val)		(ret = wl_iovar_getint(ifname, iovar, val))
#define WL_BSSIOVAR_SETBUF(ifname, iovar, bssidx, param, paramlen, buf, buflen) \
		(ret = wl_bssiovar_setbuf(ifname, iovar, bssidx, param, paramlen, buf, buflen))
#define WL_BSSIOVAR_SET(ifname, iovar, bssidx, param, paramlen) \
		(ret = wl_bssiovar_set(ifname, iovar, bssidx, param, paramlen))
#define WL_BSSIOVAR_GET(ifname, iovar, bssidx, param, paramlen) \
		(ret = wl_bssiovar_get(ifname, iovar, bssidx, param, paramlen))
#define WL_BSSIOVAR_SETINT(ifname, iovar, bssidx, val)	(ret = wl_bssiovar_setint(ifname, iovar, \
			bssidx, val))
#endif /* BCMDBG */

#ifdef BCMWPA2
#define CHECK_PSK(mode) ((mode) & (WPA_AUTH_PSK | WPA2_AUTH_PSK))
#else
#define CHECK_PSK(mode) ((mode) & WPA_AUTH_PSK)
#endif

/* prototypes */
struct bsscfg_list *wlconf_get_bsscfgs(char* ifname, char* prefix);
int wlconf(char *name);
int wlconf_down(char *name);

static int
wlconf_getint(char* ifname, int cmd, int *pval)
{
	return wl_ioctl(ifname, cmd, pval, sizeof(int));
}

static int
wlconf_setint(char* ifname, int cmd, int val)
{
	return wl_ioctl(ifname, cmd, &val, sizeof(int));
}

static int
wlconf_wds_clear(char *name)
{
	struct maclist maclist;
	int    ret;

	maclist.count = 0;
	WL_IOCTL(name, WLC_SET_WDSLIST, &maclist, sizeof(maclist));

	return ret;
}

/* set WEP key */
static int
wlconf_set_wep_key(char *name, char *prefix, int bsscfg_idx, int i)
{
	wl_wsec_key_t key;
	char wl_key[] = "wlXXXXXXXXXX_keyXXXXXXXXXX";
	char *keystr, hex[] = "XX";
	unsigned char *data = key.data;
	int ret = 0;

	memset(&key, 0, sizeof(key));
	key.index = i - 1;
	sprintf(wl_key, "%skey%d", prefix, i);
	keystr = nvram_safe_get(wl_key);

	switch (strlen(keystr)) {
	case WEP1_KEY_SIZE:
	case WEP128_KEY_SIZE:
		key.len = strlen(keystr);
		strcpy((char *)key.data, keystr);
		break;
	case WEP1_KEY_HEX_SIZE:
	case WEP128_KEY_HEX_SIZE:
		key.len = strlen(keystr) / 2;
		while (*keystr) {
			strncpy(hex, keystr, 2);
			*data++ = (unsigned char) strtoul(hex, NULL, 16);
			keystr += 2;
		}
		break;
	default:
		key.len = 0;
		break;
	}

	/* Set current WEP key */
	if (key.len && i == atoi(nvram_safe_get(strcat_r(prefix, "key", wl_key))))
		key.flags = WL_PRIMARY_KEY;

	WL_BSSIOVAR_SET(name, "wsec_key", bsscfg_idx, &key, sizeof(key));

	return ret;
}

extern struct nvram_tuple router_defaults[];

/* validate/restore all per-interface related variables */
static void
wlconf_validate_all(char *prefix, bool restore)
{
	struct nvram_tuple *t;
	char tmp[100];
	char *v;
	for (t = router_defaults; t->name; t++) {
		if (!strncmp(t->name, "wl_", 3)) {
			strcat_r(prefix, &t->name[3], tmp);
			if (!restore && nvram_get(tmp))
				continue;
			v = nvram_get(t->name);
			nvram_set(tmp, v ? v : t->value);
		}
	}
}

/* restore specific per-interface variable */
static void
wlconf_restore_var(char *prefix, char *name)
{
	struct nvram_tuple *t;
	char tmp[100];
	for (t = router_defaults; t->name; t++) {
		if (!strncmp(t->name, "wl_", 3) && !strcmp(&t->name[3], name)) {
			nvram_set(strcat_r(prefix, name, tmp), t->value);
			break;
		}
	}
}
static int
wlconf_akm_options(char *prefix)
{
	char comb[32];
	char *wl_akm;
	int akm_ret_val = 0;
	char akm[32];
	char *next;

	wl_akm = nvram_safe_get(strcat_r(prefix, "akm", comb));
	foreach(akm, wl_akm, next) {
		if (!strcmp(akm, "wpa"))
			akm_ret_val |= WPA_AUTH_UNSPECIFIED;
		if (!strcmp(akm, "psk"))
			akm_ret_val |= WPA_AUTH_PSK;
#ifdef BCMWPA2
		if (!strcmp(akm, "wpa2"))
			akm_ret_val |= WPA2_AUTH_UNSPECIFIED;
		if (!strcmp(akm, "psk2"))
			akm_ret_val |= WPA2_AUTH_PSK;
		if (!strcmp(akm, "brcm_psk"))
			akm_ret_val |= BRCM_AUTH_PSK;
#endif /* BCMWPA2 */
	}
	return akm_ret_val;
}

/* Set up wsec */
static int
wlconf_set_wsec(char *ifname, char *prefix, int bsscfg_idx)
{
	char tmp[100];
	int val = 0;
	int akm_val;
	int ret;

	/* Set wsec bitvec */
	akm_val = wlconf_akm_options(prefix);
	if (akm_val != 0) {
		if (nvram_match(strcat_r(prefix, "crypto", tmp), "tkip"))
			val = TKIP_ENABLED;
		else if (nvram_match(strcat_r(prefix, "crypto", tmp), "aes"))
			val = AES_ENABLED;
		else if (nvram_match(strcat_r(prefix, "crypto", tmp), "tkip+aes"))
			val = TKIP_ENABLED | AES_ENABLED;
	}
	if (nvram_match(strcat_r(prefix, "wep", tmp), "enabled"))
		val |= WEP_ENABLED;
	WL_BSSIOVAR_SETINT(ifname, "wsec", bsscfg_idx, val);
	/* Set wsec restrict if WSEC_ENABLED */
	WL_BSSIOVAR_SETINT(ifname, "wsec_restrict", bsscfg_idx, val ? 1 : 0);

	return 0;
}

#ifdef BCMWPA2
static int
wlconf_set_preauth(char *name, int bsscfg_idx, int preauth)
{
	uint cap;
	int ret;

	WL_BSSIOVAR_GET(name, "wpa_cap", bsscfg_idx, &cap, sizeof(uint));
	if (ret != 0) return -1;

	if (preauth)
		cap |= WPA_CAP_WPA2_PREAUTH;
	else
		cap &= ~WPA_CAP_WPA2_PREAUTH;

	WL_BSSIOVAR_SETINT(name, "wpa_cap", bsscfg_idx, cap);

	return ret;
}
#endif /* BCMWPA2 */

static void
wlconf_set_radarthrs(char *name, char *prefix)
{
	wl_radar_thr_t  radar_thr;
	int  i, ret, len;
	char nv_buf[NVRAM_MAX_VALUE_LEN], *rargs, *v, *endptr;
	char buf[WLC_IOCTL_SMLEN];
	char *version = NULL, *thr0_20_lo = NULL, *thr1_20_lo = NULL, *thr0_40_lo = NULL;
	char *thr1_40_lo = NULL, *thr0_20_hi = NULL, *thr1_20_hi = NULL, *thr0_40_hi = NULL;
	char *thr1_40_hi = NULL;
	char **locals[] = { &version, &thr0_20_lo, &thr1_20_lo, &thr0_40_lo, &thr1_40_lo,
	                    &thr0_20_hi, &thr1_20_hi, &thr0_40_hi, &thr1_40_hi };

	rargs = nvram_safe_get(strcat_r(prefix, "radarthrs", nv_buf));
	if (!rargs)
		goto err;

	len = strlen(rargs);
	if ((len > NVRAM_MAX_VALUE_LEN) || (len == 0))
		goto err;

	memset(nv_buf, 0, sizeof(nv_buf));
	strncpy(nv_buf, rargs, len);
	v = nv_buf;
	for (i = 0; i < (sizeof(locals) / sizeof(locals[0])); i++) {
		*locals[i] = v;
		while (*v && *v != ' ') {
			v++;
		}
		if (*v) {
			*v = 0;
			v++;
		}
		if (v >= (nv_buf + len)) /* Check for complete list, if not caught later */
			break;
	}

	/* Start building request */
	memset(buf, 0, sizeof(buf));
	strcpy(buf, "radarthrs");
	/* Retrieve radar thrs parameters */
	if (!version)
		goto err;
	radar_thr.version = atoi(version);
	if (radar_thr.version > WL_RADAR_THR_VERSION)
		goto err;

	/* Retrieve ver 0 params */
	if (!thr0_20_lo)
		goto err;
	radar_thr.thresh0_20_lo = (uint16)strtol(thr0_20_lo, &endptr, 0);
	if (*endptr != '\0')
		goto err;

	if (!thr1_20_lo)
		goto err;
	radar_thr.thresh1_20_lo = (uint16)strtol(thr1_20_lo, &endptr, 0);
	if (*endptr != '\0')
		goto err;

	if (!thr0_40_lo)
		goto err;
	radar_thr.thresh0_40_lo = (uint16)strtol(thr0_40_lo, &endptr, 0);
	if (*endptr != '\0')
		goto err;

	if (!thr1_40_lo)
		goto err;
	radar_thr.thresh1_40_lo = (uint16)strtol(thr1_40_lo, &endptr, 0);
	if (*endptr != '\0')
		goto err;

	if (radar_thr.version == 0) {
		/* 
		 * Attempt a best effort update of ver 0 to ver 1 by updating
		 * the appropriate values with the specified defaults.  The defaults
		 * are from the reference design.
		 */
		radar_thr.version = WL_RADAR_THR_VERSION; /* avoid driver rejecting it */
		radar_thr.thresh0_20_hi = 0x6ac;
		radar_thr.thresh1_20_hi = 0x6cc;
		radar_thr.thresh0_40_hi = 0x6bc;
		radar_thr.thresh1_40_hi = 0x6e0;
	} else {
		/* Retrieve ver 1 params */
		if (!thr0_20_hi)
			goto err;
		radar_thr.thresh0_20_hi = (uint16)strtol(thr0_20_hi, &endptr, 0);
		if (*endptr != '\0')
			goto err;

		if (!thr1_20_hi)
			goto err;
		radar_thr.thresh1_20_hi = (uint16)strtol(thr1_20_hi, &endptr, 0);
		if (*endptr != '\0')
			goto err;

		if (!thr0_40_hi)
			goto err;
		radar_thr.thresh0_40_hi = (uint16)strtol(thr0_40_hi, &endptr, 0);
		if (*endptr != '\0')
			goto err;

		if (!thr1_40_hi)
			goto err;
		radar_thr.thresh1_40_hi = (uint16)strtol(thr1_40_hi, &endptr, 0);
		if (*endptr != '\0')
			goto err;
	}

	/* Copy radar parameters into buffer and plug them to the driver */
	memcpy((char*)(buf + strlen(buf) + 1), (char*)&radar_thr, sizeof(wl_radar_thr_t));
	WL_IOCTL(name, WLC_SET_VAR, buf, sizeof(buf));

	return;

err:
	WLCONF_DBG("Did not parse radar thrs params, using driver defaults\n");
	return;
}


/* Set up WME */
static void
wlconf_set_wme(char *name, char *prefix)
{
	int i, j, k;
	int val, ret;
	int phytype, gmode, no_ack, apsd, dp[2];
	edcf_acparam_t *acparams;
	/* Pay attention to buffer length requirements when using this */
	char buf[WLC_IOCTL_SMLEN];
	char *v, *nv_value, nv[100];
	char nv_name[] = "%swme_%s_%s";
	char *ac[] = {"be", "bk", "vi", "vo"};
	char *cwmin, *cwmax, *aifsn, *txop_b, *txop_ag, *admin_forced, *oldest_first;
	char **locals[] = { &cwmin, &cwmax, &aifsn, &txop_b, &txop_ag, &admin_forced,
	                    &oldest_first };
	struct {char *req; char *str;} mode[] = {{"wme_ac_sta", "sta"}, {"wme_ac_ap", "ap"},
	                                         {"wme_tx_params", "txp"}};

	/* query the phy type */
	WL_IOCTL(name, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));
	/* get gmode */
	gmode = atoi(nvram_safe_get(strcat_r(prefix, "gmode", nv)));

	/* WME sta setting first */
	for (i = 0; i < 2; i++) {
		/* build request block */
		memset(buf, 0, sizeof(buf));
		strcpy(buf, mode[i].req);
		/* put push wmeac params after "wme-ac" in buf */
		acparams = (edcf_acparam_t *)(buf + strlen(buf) + 1);
		dp[i] = 0;
		for (j = 0; j < AC_COUNT; j++) {
			/* get packed nvram parameter */
			snprintf(nv, sizeof(nv), nv_name, prefix, mode[i].str, ac[j]);
			nv_value = nvram_safe_get(nv);
			strcpy(nv, nv_value);
			/* unpack it */
			v = nv;
			for (k = 0; k < (sizeof(locals) / sizeof(locals[0])); k++) {
				*locals[k] = v;
				while (*v && *v != ' ')
					v++;
				if (*v) {
					*v = 0;
					v++;
				}
			}

			/* update CWmin */
			acparams->ECW &= ~EDCF_ECWMIN_MASK;
			val = atoi(cwmin);
			for (val++, k = 0; val; val >>= 1, k++);
			acparams->ECW |= (k ? k - 1 : 0) & EDCF_ECWMIN_MASK;
			/* update CWmax */
			acparams->ECW &= ~EDCF_ECWMAX_MASK;
			val = atoi(cwmax);
			for (val++, k = 0; val; val >>= 1, k++);
			acparams->ECW |= ((k ? k - 1 : 0) << EDCF_ECWMAX_SHIFT) & EDCF_ECWMAX_MASK;
			/* update AIFSN */
			acparams->ACI &= ~EDCF_AIFSN_MASK;
			acparams->ACI |= atoi(aifsn) & EDCF_AIFSN_MASK;
			/* update ac */
			acparams->ACI &= ~EDCF_ACI_MASK;
			acparams->ACI |= j << EDCF_ACI_SHIFT;
			/* update TXOP */
			if (phytype == PHY_TYPE_B || gmode == 0)
				val = atoi(txop_b);
			else
				val = atoi(txop_ag);
			acparams->TXOP = val / 32;
			/* update acm */
			acparams->ACI &= ~EDCF_ACM_MASK;
			val = strcmp(admin_forced, "on") ? 0 : 1;
			acparams->ACI |= val << 4;

			/* configure driver */
			WL_IOCTL(name, WLC_SET_VAR, buf, sizeof(buf));
		}
	}

	/* set no-ack */
	v = nvram_safe_get(strcat_r(prefix, "wme_no_ack", nv));
	no_ack = strcmp(v, "on") ? 0 : 1;
	WL_IOVAR_SETINT(name, "wme_noack", no_ack);

	/* set APSD */
	v = nvram_safe_get(strcat_r(prefix, "wme_apsd", nv));
	apsd = strcmp(v, "on") ? 0 : 1;
	WL_IOVAR_SETINT(name, "wme_apsd", apsd);

	/* set per-AC discard policy */
	strcpy(buf, "wme_dp");
	WL_IOVAR_SETINT(name, "wme_dp", dp[1]);

	/* WME Tx parameters setting */
	{
		wme_tx_params_t txparams[AC_COUNT];
		char *srl, *sfbl, *lrl, *lfbl, *maxrate;
		char **locals[] = { &srl, &sfbl, &lrl, &lfbl, &maxrate };

		/* build request block */
		memset(txparams, 0, sizeof(txparams));

		for (j = 0; j < AC_COUNT; j++) {
			/* get packed nvram parameter */
			snprintf(nv, sizeof(nv), nv_name, prefix, mode[2].str, ac[j]);
			nv_value = nvram_safe_get(nv);
			strcpy(nv, nv_value);
			/* unpack it */
			v = nv;
			for (k = 0; k < (sizeof(locals) / sizeof(locals[0])); k++) {
				*locals[k] = v;
				while (*v && *v != ' ')
					v++;
				if (*v) {
					*v = 0;
					v++;
				}
			}

			/* update short retry limit */
			txparams[j].short_retry = atoi(srl);

			/* update short fallback limit */
			txparams[j].short_fallback = atoi(sfbl);

			/* update long retry limit */
			txparams[j].long_retry = atoi(lrl);

			/* update long fallback limit */
			txparams[j].long_fallback = atoi(lfbl);

			/* update max rate */
			txparams[j].max_rate = atoi(maxrate);
		}

		/* set the WME tx parameters */
		WL_IOVAR_SET(name, mode[2].req, txparams, sizeof(txparams));
	}

	return;
}

#if defined(linux) || defined(__NetBSD__)
#include <unistd.h>
static void
sleep_ms(const unsigned int ms)
{
	usleep(1000*ms);
}
#elif defined(__ECOS)
static void
sleep_ms(const unsigned int ms)
{
	cyg_tick_count_t ostick;

	ostick = ms / 10;
	cyg_thread_delay(ostick);
}
#else
#error "sleep_ms() not defined for this OS!!!"
#endif /* defined(linux) */

/*
* The following condition(s) must be met when Auto Channel Selection
* is enabled.
*  - the I/F is up (change radio channel requires it is up?)
*  - the AP must not be associated (setting SSID to empty should
*    make sure it for us)
*/
static uint8
wlconf_auto_channel(char *name)
{
	int chosen = 0;
	wl_uint32_list_t request;
	int phytype;
	int ret;
	int i;

	/* query the phy type */
	WL_GETINT(name, WLC_GET_PHYTYPE, &phytype);

	request.count = 0;	/* let the ioctl decide */
	WL_IOCTL(name, WLC_START_CHANNEL_SEL, &request, sizeof(request));
	if (!ret) {
		sleep_ms(phytype == PHY_TYPE_A ? 1000 : 750);
		for (i = 0; i < 100; i++) {
			WL_GETINT(name, WLC_GET_CHANNEL_SEL, &chosen);
			if (!ret)
				break;
			sleep_ms(100);
		}
	}
	WLCONF_DBG("interface %s: channel selected %d\n", name, chosen);
	return chosen;
}

static chanspec_t
wlconf_auto_chanspec(char *name)
{
	chanspec_t chosen = 0;
	int temp = 0;
	wl_uint32_list_t request;
	int ret;
	int i;

	request.count = 0;	/* let the ioctl decide */
	WL_IOCTL(name, WLC_START_CHANNEL_SEL, &request, sizeof(request));
	if (!ret) {
		/* this time needs to be < 1000 to prevent mpc kicking in for 2nd radio */
		sleep_ms(500);
		for (i = 0; i < 100; i++) {
			WL_IOVAR_GETINT(name, "apcschspec", &temp);
			if (!ret)
				break;
			sleep_ms(100);
		}
	}

	chosen = (chanspec_t) temp;
	WLCONF_DBG("interface %s: chanspec selected %04x\n", name, chosen);
	return chosen;
}

/* PHY type/BAND conversion */
#define WLCONF_PHYTYPE2BAND(phy)	((phy) == PHY_TYPE_A ? WLC_BAND_5G : WLC_BAND_2G)
/* PHY type conversion */
#define WLCONF_PHYTYPE2STR(phy)	((phy) == PHY_TYPE_A ? "a" : \
				 (phy) == PHY_TYPE_B ? "b" : \
				 (phy) == PHY_TYPE_LP ? "l" : \
				 (phy) == PHY_TYPE_G ? "g" : \
				 (phy) == PHY_TYPE_SSN ? "s" : \
				 (phy) == PHY_TYPE_HT ? "h" : \
				 (phy) == PHY_TYPE_LCN ? "c" : "n")
#define WLCONF_STR2PHYTYPE(ch)	((ch) == 'a' ? PHY_TYPE_A : \
				 (ch) == 'b' ? PHY_TYPE_B : \
				 (ch) == 'l' ? PHY_TYPE_LP : \
				 (ch) == 'g' ? PHY_TYPE_G : \
				 (ch) == 's' ? PHY_TYPE_SSN : \
				 (ch) == 'h' ? PHY_TYPE_HT : \
				 (ch) == 'c' ? PHY_TYPE_LCN : PHY_TYPE_N)

#define WLCONF_PHYTYPE_11N(phy) ((phy) == PHY_TYPE_N 	|| (phy) == PHY_TYPE_SSN || \
				 (phy) == PHY_TYPE_LCN 	|| (phy) == PHY_TYPE_HT)


#define PREFIX_LEN 32			/* buffer size for wlXXX_ prefix */

struct bsscfg_info {
	int idx;			/* bsscfg index */
	char ifname[PREFIX_LEN];	/* OS name of interface (debug only) */
	char prefix[PREFIX_LEN];	/* prefix for nvram params (eg. "wl0.1_") */
};

struct bsscfg_list {
	int count;
	struct bsscfg_info bsscfgs[WL_MAXBSSCFG];
};

struct bsscfg_list *
wlconf_get_bsscfgs(char* ifname, char* prefix)
{
	char var[80];
	char tmp[100];
	char *next;

	struct bsscfg_list *bclist;
	struct bsscfg_info *bsscfg;

	bclist = (struct bsscfg_list*)malloc(sizeof(struct bsscfg_list));
	if (bclist == NULL)
		return NULL;
	memset(bclist, 0, sizeof(struct bsscfg_list));

	/* Set up Primary BSS Config information */
	bsscfg = &bclist->bsscfgs[0];
	bsscfg->idx = 0;
	strncpy(bsscfg->ifname, ifname, PREFIX_LEN-1);
	strcpy(bsscfg->prefix, prefix);
	bclist->count = 1;

	/* additional virtual BSS Configs from wlX_vifs */
	foreach(var, nvram_safe_get(strcat_r(prefix, "vifs", tmp)), next) {
		if (bclist->count == WL_MAXBSSCFG) {
			WLCONF_DBG("wlconf(%s): exceeded max number of BSS Configs (%d)"
			           "in nvram %s\n"
			           "while configuring interface \"%s\"\n",
			           ifname, WL_MAXBSSCFG, strcat_r(prefix, "vifs", tmp), var);
			continue;
		}
		bsscfg = &bclist->bsscfgs[bclist->count];
		if (get_ifname_unit(var, NULL, &bsscfg->idx) != 0) {
			WLCONF_DBG("wlconfg(%s): unable to parse unit.subunit in interface "
			           "name \"%s\"\n",
			           ifname, var);
			continue;
		}
		strncpy(bsscfg->ifname, var, PREFIX_LEN-1);
		snprintf(bsscfg->prefix, PREFIX_LEN, "%s_", bsscfg->ifname);
		bclist->count++;
	}

	return bclist;
}

static void
wlconf_config_join_pref(char *name, int auth_val)
{
	int ret = 0, i = 0;

	if ((auth_val & (WPA_AUTH_UNSPECIFIED | WPA2_AUTH_UNSPECIFIED)) || CHECK_PSK(auth_val)) {
		uchar pref[] = {
			/* WPA pref, 14 tuples */
			0x02, 0xaa, 0x00, 0x0e,
			/* WPA2                 AES  (unicast)          AES (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04,
			/* WPA                  AES  (unicast)          AES (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x50, 0xf2, 0x04,
			/* WPA2                 AES  (unicast)          TKIP (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x02,
			/* WPA                  AES  (unicast)          TKIP (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x50, 0xf2, 0x02,
			/* WPA2                 AES  (unicast)          WEP-40 (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x01,
			/* WPA                  AES  (unicast)          WEP-40 (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x50, 0xf2, 0x01,
			/* WPA2                 AES  (unicast)          WEP-128 (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x05,
			/* WPA                  AES  (unicast)          WEP-128 (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x04, 0x00, 0x50, 0xf2, 0x05,
			/* WPA2                 TKIP (unicast)          TKIP (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x0f, 0xac, 0x02,
			/* WPA                  TKIP (unicast)          TKIP (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x50, 0xf2, 0x02,
			/* WPA2                 TKIP (unicast)          WEP-40 (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x0f, 0xac, 0x01,
			/* WPA                  TKIP (unicast)          WEP-40 (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x50, 0xf2, 0x01,
			/* WPA2                 TKIP (unicast)          WEP-128 (multicast) */
			0x00, 0x0f, 0xac, 0x01, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x0f, 0xac, 0x05,
			/* WPA                  TKIP (unicast)          WEP-128 (multicast) */
			0x00, 0x50, 0xf2, 0x01, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x50, 0xf2, 0x05,
			/* RSSI pref */
			0x01, 0x02, 0x00, 0x00,
		};

		if (CHECK_PSK(auth_val)) {
			for (i = 0; i < pref[3]; i ++)
				pref[7 + i * 12] = 0x02;
		}

		WL_IOVAR_SET(name, "join_pref", pref, sizeof(pref));
	}

}

static void
wlconf_security_options(char *name, char *prefix, int bsscfg_idx, bool id_supp,
	bool check_join_pref)
{
	int i;
	int val;
	int ret;
	char tmp[100];
	bool need_id_supp = FALSE;
	bool need_join_pref = FALSE;

#define AUTOWPA(cfg) ((cfg) == (WPA_AUTH_PSK | WPA2_AUTH_PSK))

	/* Set WSEC */
	/*
	* Need to check errors (card may have changed) and change to
	* defaults since the new chip may not support the requested
	* encryptions after the card has been changed.
	*/
	if (wlconf_set_wsec(name, prefix, bsscfg_idx)) {
		/* change nvram only, code below will pass them on */
		wlconf_restore_var(prefix, "auth_mode");
		wlconf_restore_var(prefix, "auth");
		/* reset wep to default */
		wlconf_restore_var(prefix, "crypto");
		wlconf_restore_var(prefix, "wep");
		wlconf_set_wsec(name, prefix, bsscfg_idx);
	}

	val = wlconf_akm_options(prefix);
	if (bsscfg_idx == 0) {
		need_join_pref = ((check_join_pref || id_supp) && AUTOWPA(val));
		need_id_supp = (id_supp || need_join_pref);
	}

	if (need_join_pref)
		wlconf_config_join_pref(name, val);

	/* enable in-driver wpa supplicant? */
	if (need_id_supp && (CHECK_PSK(val))) {
		wsec_pmk_t psk;
		char *key;

		if (((key = nvram_get(strcat_r(prefix, "wpa_psk", tmp))) != NULL) &&
		    (strlen(key) < WSEC_MAX_PSK_LEN)) {
			psk.key_len = (ushort) strlen(key);
			psk.flags = WSEC_PASSPHRASE;
			strcpy((char *)psk.key, key);
			WL_IOCTL(name, WLC_SET_WSEC_PMK, &psk, sizeof(psk));
		}
		wl_iovar_setint(name, "sup_wpa", 1);
	}

	if (!need_join_pref)
		WL_BSSIOVAR_SETINT(name, "wpa_auth", bsscfg_idx, val);

	/* EAP Restrict if we have an AKM or radius authentication */
	val = ((val != 0) || (nvram_match(strcat_r(prefix, "auth_mode", tmp), "radius")));
	WL_BSSIOVAR_SETINT(name, "eap_restrict", bsscfg_idx, val);

	/* Set WEP keys */
	if (nvram_match(strcat_r(prefix, "wep", tmp), "enabled")) {
		for (i = 1; i <= DOT11_MAX_DEFAULT_KEYS; i++)
			wlconf_set_wep_key(name, prefix, bsscfg_idx, i);
	}

	/* Set 802.11 authentication mode - open/shared */
	val = atoi(nvram_safe_get(strcat_r(prefix, "auth", tmp)));
	WL_BSSIOVAR_SETINT(name, "auth", bsscfg_idx, val);
}


/*
 * When N-mode is ON, afterburner is disabled and AMPDU, AMSDU are enabled/disabled
 * based on the nvram setting. Only one of the AMPDU or AMSDU options is enabled any
 * time. When N-mode is OFF or the device is non N-phy, AMPDU and AMSDU are turned off,
 * afterburner is enabled/disabled based on the nvram settings.
 *
 * WME/WMM is also set in this procedure as it depends on N and afterburner.
 *     N ==> WMM is on by default
 *     N (or ampdu) ==> afterburner is off
 *     WMM ==> afterburner is off
 *
 * Returns whether afterburner is on in the system.
 */
static int
wlconf_aburn_ampdu_amsdu_set(char *name, char prefix[PREFIX_LEN], int nmode, int btc_mode)
{
	bool ampdu_valid_option = FALSE;
	bool amsdu_valid_option = FALSE;
	bool aburn_valid_option = FALSE;
	int  val, aburn_option_val = OFF, ampdu_option_val = OFF, amsdu_option_val = OFF;
	int wme_option_val = ON;  /* On by default */
	char tmp[CAP_STR_LEN], var[80], *next, *wme_val;
	char buf[WLC_IOCTL_SMLEN];
	int len = strlen("amsdu");
	int ret;

	/* First, clear WMM and afterburner settings to avoid conflicts */
	WL_IOVAR_SETINT(name, "wme", OFF);
	WL_IOVAR_SETINT(name, "afterburner_override", OFF);

	/* Get WME setting from NVRAM if present */
	wme_val = nvram_get(strcat_r(prefix, "wme", tmp));
	if (wme_val && !strcmp(wme_val, "off")) {
		wme_option_val = OFF;
	}

	/* Set options based on capability */
	wl_iovar_get(name, "cap", (void *)tmp, CAP_STR_LEN);
	foreach(var, tmp, next) {
		char *nvram_str;
		bool amsdu = 0;

		/* Check for the capabilitiy 'amsdutx' */
		if (strncmp(var, "amsdutx", sizeof(var)) == 0) {
			var[len] = '\0';
			amsdu = 1;
		}
		nvram_str = nvram_get(strcat_r(prefix, var, buf));
		if (!nvram_str)
			continue;

		if (!strcmp(nvram_str, "on"))
			val = ON;
		else if (!strcmp(nvram_str, "off"))
			val = OFF;
		else if (!strcmp(nvram_str, "auto"))
			val = AUTO;
		else
			continue;

		if (btc_mode != WL_BTC_PREMPT && strncmp(var, "afterburner", sizeof(var)) == 0) {
			aburn_valid_option = TRUE;
			aburn_option_val = val;
		}

		if (strncmp(var, "ampdu", sizeof(var)) == 0) {
			ampdu_valid_option = TRUE;
			ampdu_option_val = val;
		}

		if (amsdu) {
			amsdu_valid_option = TRUE;
			amsdu_option_val = val;
		}
	}

	if (nmode != OFF) { /* N-mode is ON/AUTO */
		if (ampdu_valid_option) {
			if (ampdu_option_val != OFF) {
				WL_IOVAR_SETINT(name, "amsdu", OFF);
				WL_IOVAR_SETINT(name, "ampdu", ampdu_option_val);
			} else {
				WL_IOVAR_SETINT(name, "ampdu", OFF);
			}
		}

		if (amsdu_valid_option) {
			if (amsdu_option_val != OFF) { /* AMPDU (above) has priority over AMSDU */
				if (ampdu_option_val == OFF) {
					WL_IOVAR_SETINT(name, "ampdu", OFF);
					WL_IOVAR_SETINT(name, "amsdu", amsdu_option_val);
				}
			} else
				WL_IOVAR_SETINT(name, "amsdu", OFF);
		}
		/* allow ab in N mode. Do this last: may defeat ampdu et al */
		if (aburn_valid_option) {
			WL_IOVAR_SETINT(name, "afterburner_override", aburn_option_val);

			/* Also turn off N reqd setting if ab is not OFF */
			if (aburn_option_val != 0)
				WL_IOVAR_SETINT(name, "nreqd", 0);
		}

	} else {
		/* When N-mode is off or for non N-phy device, turn off AMPDU, AMSDU;
		 * if WME is off, set the afterburner based on the configured nvram setting.
		 */
		wl_iovar_setint(name, "amsdu", OFF);
		wl_iovar_setint(name, "ampdu", OFF);
		if (wme_option_val != OFF) { /* Can't have afterburner with WMM */
			if (aburn_valid_option) {
				WL_IOVAR_SETINT(name, "afterburner_override", OFF);
			}
		} else if (aburn_valid_option) { /* Okay, use NVRam setting for afterburner */
			WL_IOVAR_SETINT(name, "afterburner_override", aburn_option_val);
		}
	}

	if (wme_option_val && aburn_option_val == 0) {
		WL_IOVAR_SETINT(name, "wme", wme_option_val);
		wlconf_set_wme(name, prefix);
	}

	return wme_option_val;
}

/* WLMEDIA_IPTV::WLMEDIA_IPTV_WET_TUNNEL */
static int
wlconf_del_wet_tunnel_vndr_ie(char *name, int bsscfg_idx, uchar *oui)
{
	int iebuf_len = 0;
	vndr_ie_setbuf_t *ie_setbuf;
	int iecount, i;

	char getbuf[2048] = {0};
	vndr_ie_buf_t *iebuf;
	vndr_ie_info_t *ieinfo;
	char *bufaddr;
	int buflen = 0;
	int found = 0;
	uint32 pktflag;
	uint32 frametype;
	int ret = 0;

	frametype = VNDR_IE_BEACON_FLAG;

	WL_BSSIOVAR_GET(name, "vndr_ie", bsscfg_idx, getbuf, 2048);
	iebuf = (vndr_ie_buf_t *)getbuf;

	bufaddr = (char*)iebuf->vndr_ie_list;

	for (i = 0; i < iebuf->iecount; i++) {
		ieinfo = (vndr_ie_info_t *)bufaddr;
		bcopy((char*)&ieinfo->pktflag, (char*)&pktflag, (int)sizeof(uint32));
		if (pktflag == frametype) {
			if (!memcmp(ieinfo->vndr_ie_data.oui, oui, DOT11_OUI_LEN)) {
				found = 1;
				bufaddr = (char*) &ieinfo->vndr_ie_data;
				buflen = (int)ieinfo->vndr_ie_data.len + VNDR_IE_HDR_LEN;
				break;
			}
		}
		bufaddr = (char *)ieinfo->vndr_ie_data.oui + ieinfo->vndr_ie_data.len;
	}

	if (!found)
		return -1;

	iebuf_len = buflen + sizeof(vndr_ie_setbuf_t) - sizeof(vndr_ie_t);
	ie_setbuf = (vndr_ie_setbuf_t *)malloc(iebuf_len);
	if (!ie_setbuf) {
		printf("memory alloc failure\n");
		return -1;
	}

	/* Copy the vndr_ie SET command ("add"/"del") to the buffer */
	strcpy(ie_setbuf->cmd, "del");

	/* Buffer contains only 1 IE */
	iecount = 1;
	memcpy(&ie_setbuf->vndr_ie_buffer.iecount, &iecount, sizeof(int));

	memcpy(&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].pktflag, &frametype, sizeof(uint32));

	memcpy(&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data, bufaddr, buflen);

	WL_BSSIOVAR_SET(name, "vndr_ie", bsscfg_idx, ie_setbuf, iebuf_len);

	free(ie_setbuf);

	return 0;
}

/* WLMEDIA_IPTV::WLMEDIA_IPTV_WET_TUNNEL */
static int
wlconf_set_wet_tunnel_vndr_ie(char *name, int bsscfg_idx, uchar *oui, uchar *data, int datalen)
{
	vndr_ie_setbuf_t *ie_setbuf;
	unsigned int pktflag;
	int buflen, iecount;
	int ret = 0;

	pktflag = VNDR_IE_BEACON_FLAG;

	buflen = sizeof(vndr_ie_setbuf_t) + datalen - 1;
	ie_setbuf = (vndr_ie_setbuf_t *)malloc(buflen);
	if (!ie_setbuf) {
		printf("memory alloc failure\n");
		return -1;
	}

	/* Copy the vndr_ie SET command ("add"/"del") to the buffer */
	strcpy(ie_setbuf->cmd, "add");

	/* Buffer contains only 1 IE */
	iecount = 1;
	memcpy(&ie_setbuf->vndr_ie_buffer.iecount, &iecount, sizeof(int));

	/* 
	 * The packet flag bit field indicates the packets that will
	 * contain this IE
	 */
	memcpy(&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].pktflag, &pktflag, sizeof(uint32));

	/* Now, add the IE to the buffer, +1: one byte OUI_TYPE */
	ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.len = DOT11_OUI_LEN + datalen;

	memcpy(&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.oui[0], oui, DOT11_OUI_LEN);
	if (datalen > 0)
		memcpy(&ie_setbuf->vndr_ie_buffer.vndr_ie_list[0].vndr_ie_data.data[0], data,
		       datalen);

	wlconf_del_wet_tunnel_vndr_ie(name, bsscfg_idx, oui);
	WL_BSSIOVAR_SET(name, "vndr_ie", (int)bsscfg_idx, ie_setbuf, buflen);
	free(ie_setbuf);

	return (ret);
}

#define VIFNAME_LEN 16

/* configure the specified wireless interface */
int
wlconf(char *name)
{
	int restore_defaults, val, unit, phytype, bandtype, gmode = 0, ret = 0;
	int bcmerr;
	int error_bg, error_a;
	struct bsscfg_list *bclist = NULL;
	struct bsscfg_info *bsscfg = NULL;
	char tmp[100], prefix[PREFIX_LEN];
	char var[80], *next, *str, *addr = NULL;
	/* Pay attention to buffer length requirements when using this */
	char buf[WLC_IOCTL_SMLEN*2]  __attribute__ ((aligned(4)));
	char *country;
	wlc_rev_info_t rev;
	channel_info_t ci;
	struct maclist *maclist;
	struct ether_addr *ea;
	wlc_ssid_t ssid;
	wl_rateset_t rs;
	unsigned int i;
	char eaddr[32];
	int ap, apsta, wds, sta = 0, wet = 0, mac_spoof = 0, wmf = 0;
	int radio_pwrsave = 0, rxchain_pwrsave = 0;
	char country_code[4];
	char *ba;
#ifdef BCMWPA2
	char *preauth;
	int set_preauth;
#endif
	int wlunit = -1;
	int wlsubunit = -1;
	int wl_ap_build = 0; /* wl compiled with AP capabilities */
	char cap[WLC_IOCTL_SMLEN];
	char caps[WLC_IOCTL_SMLEN];
	int btc_mode;
	uint32 leddc;
	uint nbw = WL_CHANSPEC_BW_20;
	int nmode = OFF; /* 802.11n support */
	char vif_addr[WLC_IOCTL_SMLEN];
	int max_no_vifs = 0;
	int wme_global;
	int max_assoc = -1;
	bool ure_enab = FALSE;
	bool radar_enab = FALSE;
	bool obss_coex = FALSE;

	/* wlconf doesn't work for virtual i/f, so if we are given a
	 * virtual i/f return 0 if that interface is in it's parent's "vifs"
	 * list otherwise return -1
	 */
	if (get_ifname_unit(name, &wlunit, &wlsubunit) == 0) {
		if (wlsubunit >= 0) {
			/* we have been given a virtual i/f,
			 * is it in it's parent i/f's virtual i/f list?
			 */
			sprintf(tmp, "wl%d_vifs", wlunit);

			if (strstr(nvram_safe_get(tmp), name) == NULL)
				return -1; /* config error */
			else
				return 0; /* okay */
		}
	} else {
		return -1;
	}

	/* clean up tmp */
	memset(tmp, 0, sizeof(tmp));

	/* because of ifdefs in wl driver,  when we don't have AP capabilities we
	 * can't use the same iovars to configure the wl.
	 * so we use "wl_ap_build" to help us know how to configure the driver
	 */
	if (wl_iovar_get(name, "cap", (void *)caps, WLC_IOCTL_SMLEN))
		return -1;

	foreach(cap, caps, next) {
		if (!strcmp(cap, "ap")) {
			wl_ap_build = 1;
		} else if (!strcmp(cap, "mbss16"))
			max_no_vifs = 16;
		else if (!strcmp(cap, "mbss8"))
			max_no_vifs = 8;
		else if (!strcmp(cap, "mbss4"))
			max_no_vifs = 4;
		else if (!strcmp(cap, "wmf"))
			wmf = 1;
		else if (!strcmp(cap, "rxchain_pwrsave"))
			rxchain_pwrsave = 1;
		else if (!strcmp(cap, "radio_pwrsave"))
			radio_pwrsave = 1;
	}

	/* Check interface (fail silently for non-wl interfaces) */
	if ((ret = wl_probe(name)))
		return ret;

	/* Get MAC address */
	(void) wl_hwaddr(name, (uchar *)buf);
	memcpy(vif_addr, buf, ETHER_ADDR_LEN);

	/* Get instance */
	WL_IOCTL(name, WLC_GET_INSTANCE, &unit, sizeof(unit));
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	/* Restore defaults if per-interface parameters do not exist */
	restore_defaults = !nvram_get(strcat_r(prefix, "ifname", tmp));
	wlconf_validate_all(prefix, restore_defaults);
	nvram_set(strcat_r(prefix, "ifname", tmp), name);
	nvram_set(strcat_r(prefix, "hwaddr", tmp), ether_etoa((uchar *)buf, eaddr));
	snprintf(buf, sizeof(buf), "%d", unit);
	nvram_set(strcat_r(prefix, "unit", tmp), buf);

#ifdef BCMDBG
	/* Apply message level */
	if (nvram_invmatch("wl_msglevel", "")) {
		val = (int)strtoul(nvram_get("wl_msglevel"), NULL, 0);
		WL_IOCTL(name, WLC_SET_MSGLEVEL, &val, sizeof(val));
	}
#endif

	/* Bring the interface down */
	WL_IOCTL(name, WLC_DOWN, NULL, sizeof(val));

	/* Disable all BSS Configs */
	for (i = 0; i < WL_MAXBSSCFG; i++) {
		struct {int bsscfg_idx; int enable;} setbuf;
		setbuf.bsscfg_idx = i;
		setbuf.enable = 0;

		ret = wl_iovar_set(name, "bss", &setbuf, sizeof(setbuf));
		if (ret) {
			wl_iovar_getint(name, "bcmerror", &bcmerr);
			/* fail quietly on a range error since the driver may
			 * support fewer bsscfgs than we are prepared to configure
			 */
			if (bcmerr == BCME_RANGE)
				break;
		}
		if (ret) {
			WLCONF_DBG("%d:(%s): setting bsscfg #%d iovar \"bss\" to 0"
			           " (down) failed, ret = %d, bcmerr = %d\n",
			           __LINE__, name, i, ret, bcmerr);
		}
	}

	/* Get the list of BSS Configs */
	bclist = wlconf_get_bsscfgs(name, prefix);
	if (bclist == NULL) {
		ret = -1;
		goto exit;
	}

#ifdef BCMDBG
	strcat_r(prefix, "vifs", tmp);
	printf("BSS Config summary: primary -> \"%s\", %s -> \"%s\"\n", name, tmp,
	       nvram_safe_get(tmp));
	for (i = 0; i < bclist->count; i++) {
		printf("BSS Config \"%s\": index %d\n",
		       bclist->bsscfgs[i].ifname, bclist->bsscfgs[i].idx);
	}
#endif

	/* create a wlX.Y_ifname nvram setting */
	for (i = 1; i < bclist->count; i++) {
		bsscfg = &bclist->bsscfgs[i];
#if defined(linux) || defined(__ECOS) || defined(__NetBSD__)
		strcpy(var, bsscfg->ifname);
#endif
		nvram_set(strcat_r(bsscfg->prefix, "ifname", tmp), var);
	}

	/* If ure_disable is not present or is 1, ure is not enabled;
	 * that is, if it is present and 0, ure is enabled.
	 */
	if (!strcmp(nvram_safe_get("ure_disable"), "0")) { /* URE is enabled */
		ure_enab = TRUE;
	}
	if (wl_ap_build) {
		/* Enable MBSS mode if appropriate */
		if (!ure_enab) {
			WL_IOVAR_SETINT(name, "mbss", (bclist->count > 1));
		}

		/*
		 * Set SSID for each BSS Config
		 */
		for (i = 0; i < bclist->count; i++) {
			bsscfg = &bclist->bsscfgs[i];
			strcat_r(bsscfg->prefix, "ssid", tmp);
			ssid.SSID_len = strlen(nvram_safe_get(tmp));
			if (ssid.SSID_len > sizeof(ssid.SSID))
				ssid.SSID_len = sizeof(ssid.SSID);
			strncpy((char *)ssid.SSID, nvram_safe_get(tmp), ssid.SSID_len);
			WLCONF_DBG("wlconfig(%s): configuring bsscfg #%d (%s) "
			           "with SSID \"%s\"\n", name, bsscfg->idx,
			           bsscfg->ifname, nvram_safe_get(tmp));
			WL_BSSIOVAR_SET(name, "ssid", bsscfg->idx, &ssid,
			                sizeof(ssid));
		}
	}

	/* Create addresses for VIFs */
	if (!ure_enab) {
		/* set local bit for our MBSS vif base */
		ETHER_SET_LOCALADDR(vif_addr);

		/* construct and set other wlX.Y_hwaddr */
		for (i = 1; i < max_no_vifs; i++) {
			snprintf(tmp, sizeof(tmp), "wl%d.%d_hwaddr", unit, i);
			addr = nvram_safe_get(tmp);
			if (!strcmp(addr, "")) {
				vif_addr[5] = (vif_addr[5] & ~(max_no_vifs-1))
				        | ((max_no_vifs-1) & (vif_addr[5]+1));

				nvram_set(tmp, ether_etoa((uchar *)vif_addr, eaddr));
			}
		}
		/* The addresses are available in NVRAM, so set them */
		for (i = 1; i < max_no_vifs; i++) {
			snprintf(tmp, sizeof(tmp), "wl%d.%d_bss_enabled", unit, i);
			if (!strcmp(nvram_safe_get(tmp), "1")) {
				snprintf(tmp, sizeof(tmp), "wl%d.%d_hwaddr", unit, i);
				ether_atoe(nvram_safe_get(tmp), (unsigned char *)eaddr);
				WL_BSSIOVAR_SET(name, "cur_etheraddr", i, eaddr, ETHER_ADDR_LEN);
			}
		}
	} else { /* URE is enabled */
		/* URE is on, so set wlX.1 hwaddr is same as that of primary interface */
		snprintf(tmp, sizeof(tmp), "wl%d.1_hwaddr", unit);
		WL_BSSIOVAR_SET(name, "cur_etheraddr", 1, vif_addr,
		                ETHER_ADDR_LEN);
		nvram_set(tmp, ether_etoa((uchar *)vif_addr, eaddr));
	}

	/* wlX_mode settings: AP, STA, WET, BSS/IBSS, APSTA */
	str = nvram_safe_get(strcat_r(prefix, "mode", tmp));
	ap = (!strcmp(str, "") || !strcmp(str, "ap"));
	apsta = (!strcmp(str, "apsta") ||
	         ((!strcmp(str, "sta") || !strcmp(str, "wet")) &&
	          bclist->count > 1));
	sta = (!strcmp(str, "sta") && bclist->count == 1);
	wds = !strcmp(str, "wds");
	wet = !strcmp(str, "wet");
	mac_spoof = !strcmp(str, "mac_spoof");

	/* set apsta var first, because APSTA mode takes precedence */
	WL_IOVAR_SETINT(name, "apsta", apsta);

	/* Set AP mode */
	val = (ap || apsta || wds) ? 1 : 0;
	WL_IOCTL(name, WLC_SET_AP, &val, sizeof(val));

	/* Set mode: WET */
	if (wet)
		WL_IOCTL(name, WLC_SET_WET, &wet, sizeof(wet));

	if (mac_spoof) {
		sta = 1;
		WL_IOVAR_SETINT(name, "mac_spoof", 1);
	}

	/* For STA configurations, configure association retry time.
	 * Use specified time (capped), or mode-specific defaults.
	 */
	if (sta || wet || apsta) {
		char *retry_time = nvram_safe_get(strcat_r(prefix, "sta_retry_time", tmp));
		val = atoi(retry_time);
		WL_IOVAR_SETINT(name, "sta_retry_time", val);
	}

	/* Retain remaining WET effects only if not APSTA */
	wet &= !apsta;

	/* Set infra: BSS/IBSS (IBSS only for WET or STA modes) */
	val = 1;
	if (wet || sta)
		val = atoi(nvram_safe_get(strcat_r(prefix, "infra", tmp)));
	WL_IOCTL(name, WLC_SET_INFRA, &val, sizeof(val));

	/* Set The AP MAX Associations Limit */
	if (ap || apsta) {
		max_assoc = val = atoi(nvram_safe_get(strcat_r(prefix, "maxassoc", tmp)));
		if (val > 0) {
			WL_IOVAR_SETINT(name, "maxassoc", val);
		} else { /* Get value from driver if not in nvram */
			WL_IOVAR_GETINT(name, "maxassoc", &max_assoc);
		}
	}
	if (!wet && !sta)
		WL_IOVAR_SETINT(name, "mpc", OFF);

	for (i = 0; i < bclist->count; i++) {
		char *subprefix;
		bsscfg = &bclist->bsscfgs[i];

#ifdef BCMWPA2
		/* XXXMSSID: The note about setting preauth now does not seem right.
		 * NAS brings the BSS up if it runs, so setting the preauth value
		 * will make it in the bcn/prb. If that is right, we can move this
		 * chunk out of wlconf.
		 */
		/*
		 * Set The WPA2 Pre auth cap. only reason we are doing it here is the driver is down
		 * if we do it in the NAS we need to bring down the interface and up to make
		 * it affect in the  beacons
		 */
		if (ap || (apsta && bsscfg->idx != 0)) {
			set_preauth = 1;
			preauth = nvram_safe_get(strcat_r(bsscfg->prefix, "preauth", tmp));
			if (strlen (preauth) != 0) {
				set_preauth = atoi(preauth);
			}
			wlconf_set_preauth(name, bsscfg->idx, set_preauth);
		}
#endif /* BCMWPA2 */

		/* WLMEDIA_IPTV::WLMEDIA_IPTV_WET_TUNNEL */
		/* Add BRCM proprietary IE for wet tunnel capability */
		if (ap) {
			if (atoi(nvram_safe_get("wet_tunnel")) == 1) {
				brcm_prop_ie_t wet_tunnel_ie;
				wet_tunnel_ie.type = WET_TUNNEL_IE_TYPE;
				wet_tunnel_ie.cap = htons(1);
				wlconf_set_wet_tunnel_vndr_ie(name,
				        bsscfg->idx, (uchar *)BRCM_PROP_OUI,
					(uchar *)&(wet_tunnel_ie.type),
					sizeof(wet_tunnel_ie.type)+sizeof(wet_tunnel_ie.cap));
				WL_IOVAR_SETINT(name, "ap_tunneling", 1);
			}
		}

		subprefix = apsta ? prefix : bsscfg->prefix;

		if (ap || (apsta && bsscfg->idx != 0)) {
			val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix, "bss_maxassoc", tmp)));
			if (val > 0) {
				WL_BSSIOVAR_SETINT(name, "bss_maxassoc", bsscfg->idx, val);
			} else if (max_assoc > 0) { /* Set maxassoc same as global if not set */
				snprintf(var, sizeof(var), "%d", max_assoc);
				nvram_set(tmp, var);
			}
		}

		/* Set network type */
		val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix, "closed", tmp)));
		WL_BSSIOVAR_SETINT(name, "closednet", bsscfg->idx, val);

		/* Set the ap isolate mode */
		val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix, "ap_isolate", tmp)));
		WL_BSSIOVAR_SETINT(name, "ap_isolate", bsscfg->idx, val);

		/* Set the WMF enable mode */
		if (wmf) {
			val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix, "wmf_bss_enable", tmp)));
			WL_BSSIOVAR_SETINT(name, "wmf_bss_enable", bsscfg->idx, val);
		}

		/* Set the Multicast Reverse Translation enable mode */
		if (wet) {
			val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix,
							 "mcast_regen_bss_enable", tmp)));
			WL_BSSIOVAR_SETINT(name, "mcast_regen_bss_enable", bsscfg->idx, val);
		}
	}

	if (rxchain_pwrsave) {
		val = atoi(nvram_safe_get(strcat_r(prefix, "rxchain_pwrsave_enable", tmp)));
		WL_BSSIOVAR_SETINT(name, "rxchain_pwrsave_enable", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(prefix, "rxchain_pwrsave_quiet_time", tmp)));
		WL_BSSIOVAR_SETINT(name, "rxchain_pwrsave_quiet_time", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(prefix, "rxchain_pwrsave_pps", tmp)));
		WL_BSSIOVAR_SETINT(name, "rxchain_pwrsave_pps", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix,
			"rxchain_pwrsave_stas_assoc_check", tmp)));
		WL_BSSIOVAR_SETINT(name, "rxchain_pwrsave_stas_assoc_check", bsscfg->idx,
			val);
	}

	if (radio_pwrsave) {
		val = atoi(nvram_safe_get(strcat_r(prefix, "radio_pwrsave_enable", tmp)));
		WL_BSSIOVAR_SETINT(name, "radio_pwrsave_enable", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(prefix, "radio_pwrsave_quiet_time", tmp)));
		WL_BSSIOVAR_SETINT(name, "radio_pwrsave_quiet_time", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(prefix, "radio_pwrsave_pps", tmp)));
		WL_BSSIOVAR_SETINT(name, "radio_pwrsave_pps", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(prefix, "radio_pwrsave_level", tmp)));
		WL_BSSIOVAR_SETINT(name, "radio_pwrsave_level", bsscfg->idx, val);

		val = atoi(nvram_safe_get(strcat_r(bsscfg->prefix,
			"radio_pwrsave_stas_assoc_check", tmp)));
		WL_BSSIOVAR_SETINT(name, "radio_pwrsave_stas_assoc_check", bsscfg->idx,
			val);
	}

	/* Set up the country code */
	(void) strcat_r(prefix, "country_code", tmp);
	country = nvram_get(tmp);
	if (country && country[0] != '\0') {
		strncpy(country_code, country, sizeof(country_code));
		WL_IOCTL(name, WLC_SET_COUNTRY, country_code, strlen(country_code) + 1);
	} else {
		/* Get the default country code if undefined */
		WL_IOCTL(name, WLC_GET_COUNTRY, country_code, sizeof(country_code));

		/* Add the new NVRAM variable */
		nvram_set("wl_country_code", country_code);
		(void) strcat_r(prefix, "country_code", tmp);
		nvram_set(tmp, country_code);
	}

	/* Change LED Duty Cycle */
	leddc = (uint32)strtoul(nvram_safe_get(strcat_r(prefix, "leddc", tmp)), NULL, 16);
	if (leddc)
		WL_IOVAR_SETINT(name, "leddc", leddc);

	/* Enable or disable the radio */
	val = nvram_match(strcat_r(prefix, "radio", tmp), "0");
	val += WL_RADIO_SW_DISABLE << 16;
	WL_IOCTL(name, WLC_SET_RADIO, &val, sizeof(val));

	/* Get supported phy types */
	WL_IOCTL(name, WLC_GET_PHYLIST, var, sizeof(var));
	nvram_set(strcat_r(prefix, "phytypes", tmp), var);

	/* Get radio IDs */
	*(next = buf) = '\0';
	for (i = 0; i < strlen(var); i++) {
		/* Switch to band */
		val = WLCONF_STR2PHYTYPE(var[i]);
		if (WLCONF_PHYTYPE_11N(val)) {
			WL_GETINT(name, WLC_GET_BAND, &val);
		} else
			val = WLCONF_PHYTYPE2BAND(val);
		WL_IOCTL(name, WLC_SET_BAND, &val, sizeof(val));
		/* Get radio ID on this band */
		WL_IOCTL(name, WLC_GET_REVINFO, &rev, sizeof(rev));
		next += sprintf(next, "%sBCM%X", i ? " " : "",
		                (rev.radiorev & IDCODE_ID_MASK) >> IDCODE_ID_SHIFT);
	}
	nvram_set(strcat_r(prefix, "radioids", tmp), buf);

	/* Set band */
	str = nvram_get(strcat_r(prefix, "phytype", tmp));
	val = str ? WLCONF_STR2PHYTYPE(str[0]) : PHY_TYPE_G;
	/* For NPHY use band value from NVRAM */
	if (WLCONF_PHYTYPE_11N(val)) {
		str = nvram_get(strcat_r(prefix, "nband", tmp));
		if (str)
			val = atoi(str);
		else {
			WL_GETINT(name, WLC_GET_BAND, &val);
		}
	} else
		val = WLCONF_PHYTYPE2BAND(val);

	WL_SETINT(name, WLC_SET_BAND, val);

	/* Check errors (card may have changed) */
	if (ret) {
		/* default band to the first band in band list */
		val = WLCONF_STR2PHYTYPE(var[0]);
		val = WLCONF_PHYTYPE2BAND(val);
		WL_SETINT(name, WLC_SET_BAND, val);
	}

	/* Store the resolved bandtype */
	bandtype = val;

	/* Get current core revision */
	WL_IOCTL(name, WLC_GET_REVINFO, &rev, sizeof(rev));
	snprintf(buf, sizeof(buf), "%d", rev.corerev);
	nvram_set(strcat_r(prefix, "corerev", tmp), buf);

	if ((rev.chipnum == BCM4716_CHIP_ID) || (rev.chipnum == BCM47162_CHIP_ID) ||
		(rev.chipnum == BCM4748_CHIP_ID) || (rev.chipnum == BCM4331_CHIP_ID) ||
		(rev.chipnum == BCM43431_CHIP_ID) || (rev.chipnum == BCM5357_CHIP_ID) ||
		(rev.chipnum == BCM53572_CHIP_ID) || (rev.chipnum == BCM43236_CHIP_ID)) {
		int pam_mode = WLC_N_PREAMBLE_GF_BRCM; /* default GF-BRCM */

		strcat_r(prefix, "mimo_preamble", tmp);
		if (nvram_match(tmp, "mm"))
			pam_mode = WLC_N_PREAMBLE_MIXEDMODE;
		else if (nvram_match(tmp, "gf"))
			pam_mode = WLC_N_PREAMBLE_GF;
		else if (nvram_match(tmp, "auto"))
			pam_mode = -1;
		WL_IOVAR_SETINT(name, "mimo_preamble", pam_mode);
	}

	if ((rev.chipnum == BCM5357_CHIP_ID) || (rev.chipnum == BCM53572_CHIP_ID)) {
		val = atoi(nvram_safe_get("coma_sleep"));
		if (val > 0) {
			struct {int sleep; int delay;} setbuf;
			nvram_unset("coma_sleep");
			nvram_commit();
			setbuf.sleep = val;
			setbuf.delay = 1;
			WL_IOVAR_SET(name, "coma", &setbuf, sizeof(setbuf));
		}
	}

	/* Get current phy type */
	WL_IOCTL(name, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));
	printf("%s: PHYTYPE: %d\n", __FUNCTION__, phytype);
	snprintf(buf, sizeof(buf), "%s", WLCONF_PHYTYPE2STR(phytype));
	nvram_set(strcat_r(prefix, "phytype", tmp), buf);

	/* Setup regulatory mode */
	strcat_r(prefix, "reg_mode", tmp);
	if (nvram_match(tmp, "off")) {
		val = 0;
		WL_IOCTL(name, WLC_SET_REGULATORY, &val, sizeof(val));
		WL_IOCTL(name, WLC_SET_RADAR, &val, sizeof(val));
		WL_IOCTL(name, WLC_SET_SPECT_MANAGMENT, &val, sizeof(val));
	} else if (nvram_match(tmp, "h") || nvram_match(tmp, "strict_h")) {
		val = 0;
		WL_IOCTL(name, WLC_SET_REGULATORY, &val, sizeof(val));
		val = 1;
		WL_IOCTL(name, WLC_SET_RADAR, &val, sizeof(val));
		radar_enab = TRUE;
		if (nvram_match(tmp, "h"))
			val = 1;
		else
			val = 2;
		WL_IOCTL(name, WLC_SET_SPECT_MANAGMENT, &val, sizeof(val));

		/* Set the CAC parameters */
		val = atoi(nvram_safe_get(strcat_r(prefix, "dfs_preism", tmp)));
		wl_iovar_setint(name, "dfs_preism", val);
		val = atoi(nvram_safe_get(strcat_r(prefix, "dfs_postism", tmp)));
		wl_iovar_setint(name, "dfs_postism", val);
		val = atoi(nvram_safe_get(strcat_r(prefix, "tpc_db", tmp)));
		WL_IOCTL(name, WLC_SEND_PWR_CONSTRAINT, &val, sizeof(val));

	} else if (nvram_match(tmp, "d")) {
		val = 0;
		WL_IOCTL(name, WLC_SET_RADAR, &val, sizeof(val));
		WL_IOCTL(name, WLC_SET_SPECT_MANAGMENT, &val, sizeof(val));
		val = 1;
		WL_IOCTL(name, WLC_SET_REGULATORY, &val, sizeof(val));
	}

	/* set bandwidth capability for nphy and calculate nbw */
	if (WLCONF_PHYTYPE_11N(phytype)) {
		/* Get the user nmode setting now */
		nmode = AUTO;	/* enable by default for NPHY */
		/* Set n mode */
		strcat_r(prefix, "nmode", tmp);
		if (nvram_match(tmp, "0"))
			nmode = OFF;

		val = (nmode != OFF) ? atoi(nvram_safe_get(strcat_r(prefix, "nbw_cap", tmp))) :
		        WLC_N_BW_20ALL;

		WL_IOVAR_SETINT(name, "nmode", (uint32)nmode);
		WL_IOVAR_SETINT(name, "mimo_bw_cap", val);

		if (((bandtype == WLC_BAND_2G) && (val == WLC_N_BW_40ALL)) ||
		    ((bandtype == WLC_BAND_5G) &&
		     (val == WLC_N_BW_40ALL || val == WLC_N_BW_20IN2G_40IN5G)))
			nbw = WL_CHANSPEC_BW_40;
		else
			nbw = WL_CHANSPEC_BW_20;
	} else {
		/* Save n mode to OFF */
		nvram_set(strcat_r(prefix, "nmode", tmp), "0");
	}

	/* Set channel before setting gmode or rateset */
	/* Manual Channel Selection - when channel # is not 0 */
	val = atoi(nvram_safe_get(strcat_r(prefix, "channel", tmp)));
	if (val && !WLCONF_PHYTYPE_11N(phytype)) {
		WL_SETINT(name, WLC_SET_CHANNEL, val);
		if (ret) {
			/* Use current channel (card may have changed) */
			WL_IOCTL(name, WLC_GET_CHANNEL, &ci, sizeof(ci));
			snprintf(buf, sizeof(buf), "%d", ci.target_channel);
			nvram_set(strcat_r(prefix, "channel", tmp), buf);
		}
	} else if (val && WLCONF_PHYTYPE_11N(phytype)) {
		chanspec_t chanspec = 0;
		uint channel;
		uint nctrlsb = WL_CHANSPEC_CTL_SB_NONE;

		channel = val;

		/* Get Ctrl SB for 40MHz channel */
		if (nbw == WL_CHANSPEC_BW_40) {
			str = nvram_safe_get(strcat_r(prefix, "nctrlsb", tmp));

			/* Adjust the channel to be center channel */
			if (!strcmp(str, "lower")) {
				nctrlsb = WL_CHANSPEC_CTL_SB_LOWER;
				channel = channel + 2;
			} else if (!strcmp(str, "upper")) {
				nctrlsb = WL_CHANSPEC_CTL_SB_UPPER;
				channel = channel - 2;
			}
		}

		/* band | BW | CTRL SB | Channel */
		chanspec |= ((bandtype << WL_CHANSPEC_BAND_SHIFT) |
		             (nbw | nctrlsb | channel));

		WL_IOVAR_SETINT(name, "chanspec", (uint32)chanspec);
	}

	/* Set up number of Tx and Rx streams */
	if (WLCONF_PHYTYPE_11N(phytype)) {
		int count;
		int streams;
		int policy;

		/* Get the number of tx chains supported by the hardware */
		wl_iovar_getint(name, "hw_txchain", &count);
		/* update NVRAM with capabilities */
		snprintf(var, sizeof(var), "%d", count);
		nvram_set(strcat_r(prefix, "hw_txchain", tmp), var);

		/* Verify that there is an NVRAM param for txstreams, if not create it and
		 * set it to hw_txchain
		 */
		streams = atoi(nvram_safe_get(strcat_r(prefix, "txchain", tmp)));
		if (streams == 0) {
			/* invalid - NVRAM needs to be fixed/initialized */
			nvram_set(strcat_r(prefix, "txchain", tmp), var);
			streams = count;
		}
		/* Apply user configured txstreams, use 1 if user disabled nmode */
		WL_IOVAR_SETINT(name, "txchain", streams);

		wl_iovar_getint(name, "hw_rxchain", &count);
		/* update NVRAM with capabilities */
		snprintf(var, sizeof(var), "%d", count);
		nvram_set(strcat_r(prefix, "hw_rxchain", tmp), var);

		/* Verify that there is an NVRAM param for rxstreams, if not create it and
		 * set it to hw_txchain
		 */
		streams = atoi(nvram_safe_get(strcat_r(prefix, "rxchain", tmp)));
		if (streams == 0) {
			/* invalid - NVRAM needs to be fixed/initialized */
			nvram_set(strcat_r(prefix, "rxchain", tmp), var);
			streams = count;
		}

		/* Apply user configured rxstreams, use 1 if user disabled nmode */
		WL_IOVAR_SETINT(name, "rxchain", streams);

		/* update the spatial policy to make chain changes effect */
		if (phytype == PHY_TYPE_HT) {
			wl_iovar_getint(name, "spatial_policy", &policy);
			WL_IOVAR_SETINT(name, "spatial_policy", policy);
		}
	}

	/* Reset to hardware rateset (band may have changed) */
	WL_IOCTL(name, WLC_GET_RATESET, &rs, sizeof(wl_rateset_t));
	WL_IOCTL(name, WLC_SET_RATESET, &rs, sizeof(wl_rateset_t));

	/* Set gmode */
	if (bandtype == WLC_BAND_2G) {
		int override = WLC_PROTECTION_OFF;
		int control = WLC_PROTECTION_CTL_OFF;

		/* Set gmode */
		gmode = atoi(nvram_safe_get(strcat_r(prefix, "gmode", tmp)));
		WL_IOCTL(name, WLC_SET_GMODE, &gmode, sizeof(gmode));

		/* Set gmode protection override and control algorithm */
		strcat_r(prefix, "gmode_protection", tmp);
		if (nvram_match(tmp, "auto")) {
			override = WLC_PROTECTION_AUTO;
			control = WLC_PROTECTION_CTL_OVERLAP;
		}
		WL_IOCTL(name, WLC_SET_GMODE_PROTECTION_OVERRIDE, &override, sizeof(override));
		WL_IOCTL(name, WLC_SET_PROTECTION_CONTROL, &control, sizeof(control));
	}

	/* Set nmode_protection */
	if (WLCONF_PHYTYPE_11N(phytype)) {
		int override = WLC_PROTECTION_OFF;
		int control = WLC_PROTECTION_CTL_OFF;

		/* Set n protection override and control algorithm */
		str = nvram_get(strcat_r(prefix, "nmode_protection", tmp));
		if (!str || !strcmp(str, "auto")) {
			override = WLC_PROTECTION_AUTO;
			control = WLC_PROTECTION_CTL_OVERLAP;
		}

		WL_IOVAR_SETINT(name, "nmode_protection_override",
		                (uint32)override);
		WL_IOCTL(name, WLC_SET_PROTECTION_CONTROL, &control, sizeof(control));
	}

	/* Set 802.11n required */
	if (nmode != OFF) {
		uint32 nreqd = OFF; /* default */

		strcat_r(prefix, "nreqd", tmp);

		if (nvram_match(tmp, "1"))
			nreqd = ON;

		WL_IOVAR_SETINT(name, "nreqd", nreqd);
	}

	/* Set vlan_prio_mode */
	{
		uint32 mode = OFF; /* default */

		strcat_r(prefix, "vlan_prio_mode", tmp);

		if (nvram_match(tmp, "on"))
			mode = ON;

		WL_IOVAR_SETINT(name, "vlan_mode", mode);
	}

	/* Get bluetooth coexistance(BTC) mode */
	btc_mode = atoi(nvram_safe_get(strcat_r(prefix, "btc_mode", tmp)));

	/* Set the afterburner, AMPDU and AMSDU options based on the N-mode */
	wme_global = wlconf_aburn_ampdu_amsdu_set(name, prefix, nmode, btc_mode);

	/* Now that wme_global is known, check per-BSS disable settings */
	for (i = 0; i < bclist->count; i++) {
		char *subprefix;
		bsscfg = &bclist->bsscfgs[i];

		subprefix = apsta ? prefix : bsscfg->prefix;

		/* For each BSS, check WME; make sure wme is set properly for this interface */
		strcat_r(subprefix, "wme", tmp);
		nvram_set(tmp, wme_global ? "on" : "off");

		str = nvram_safe_get(strcat_r(bsscfg->prefix, "wme_bss_disable", tmp));
		val = (str[0] == '1') ? 1 : 0;
		WL_BSSIOVAR_SETINT(name, "wme_bss_disable", bsscfg->idx, val);
	}

	/* Get current rateset (gmode may have changed) */
	WL_IOCTL(name, WLC_GET_CURR_RATESET, &rs, sizeof(wl_rateset_t));

	strcat_r(prefix, "rateset", tmp);
	if (nvram_match(tmp, "all")) {
		/* Make all rates basic */
		for (i = 0; i < rs.count; i++)
			rs.rates[i] |= 0x80;
	} else if (nvram_match(tmp, "12")) {
		/* Make 1 and 2 basic */
		for (i = 0; i < rs.count; i++) {
			if ((rs.rates[i] & 0x7f) == 2 || (rs.rates[i] & 0x7f) == 4)
				rs.rates[i] |= 0x80;
			else
				rs.rates[i] &= ~0x80;
		}
	}

	if (phytype != PHY_TYPE_SSN && phytype != PHY_TYPE_LCN) {
		/* Set BTC mode */
		if (!wl_iovar_setint(name, "btc_mode", btc_mode)) {
			if (btc_mode == WL_BTC_PREMPT) {
				wl_rateset_t rs_tmp = rs;
				/* remove 1Mbps and 2 Mbps from rateset */
				for (i = 0, rs.count = 0; i < rs_tmp.count; i++) {
					if ((rs_tmp.rates[i] & 0x7f) == 2 ||
					    (rs_tmp.rates[i] & 0x7f) == 4)
						continue;
					rs.rates[rs.count++] = rs_tmp.rates[i];
				}
			}
		}
	}

	/* Set rateset */
	WL_IOCTL(name, WLC_SET_RATESET, &rs, sizeof(wl_rateset_t));

	/* Allow short preamble settings for the following:
	 * 11b - short/long
	 * 11g - short /long in GMODE_LEGACY_B and GMODE_AUTO gmodes
	 *	 GMODE_PERFORMANCE and GMODE_LRS will use short and long
	 *	 preambles respectively, by default
	 * 11n - short/long applicable in 2.4G band only
	 */
	if (phytype == PHY_TYPE_B ||
	    (WLCONF_PHYTYPE_11N(phytype) && (bandtype == WLC_BAND_2G)) ||
	    ((phytype == PHY_TYPE_G || phytype == PHY_TYPE_LP) &&
	     (gmode == GMODE_LEGACY_B || gmode == GMODE_AUTO))) {
		strcat_r(prefix, "plcphdr", tmp);
		if (nvram_match(tmp, "long"))
			val = WLC_PLCP_AUTO;
		else
			val = WLC_PLCP_SHORT;
		WL_IOCTL(name, WLC_SET_PLCPHDR, &val, sizeof(val));
	}

	/* Set rate in 500 Kbps units */
	val = atoi(nvram_safe_get(strcat_r(prefix, "rate", tmp))) / 500000;

	/* Convert Auto mcsidx to Auto rate */
	if (WLCONF_PHYTYPE_11N(phytype)) {
		int mcsidx = atoi(nvram_safe_get(strcat_r(prefix, "nmcsidx", tmp)));

		/* -1 mcsidx used to designate AUTO rate */
		if (mcsidx == -1)
			val = 0;
	}

	/* 1Mbps and 2 Mbps are not allowed in BTC pre-emptive mode */
	if (btc_mode == WL_BTC_PREMPT && (val == 2 || val == 4))
		/* Must b/g band.  Set to 5.5Mbps */
		val = 11;

	/* it is band-blind. try both band */
	error_bg = wl_iovar_setint(name, "bg_rate", val);
	error_a = wl_iovar_setint(name, "a_rate", val);

	if (error_bg && error_a) {
		/* both failed. Try default rate (card may have changed) */
		val = 0;

		error_bg = wl_iovar_setint(name, "bg_rate", val);
		error_a = wl_iovar_setint(name, "a_rate", val);

		snprintf(buf, sizeof(buf), "%d", val);
		nvram_set(strcat_r(prefix, "rate", tmp), buf);
	}

	/* check if nrate needs to be applied */
	if (nmode != OFF) {
		uint32 nrate = 0;
		int mcsidx = atoi(nvram_safe_get(strcat_r(prefix, "nmcsidx", tmp)));
		bool ismcs = (mcsidx >= 0);

		/* mcsidx of 32 is valid only for 40 Mhz */
		if (mcsidx == 32 && nbw == WL_CHANSPEC_BW_20) {
			mcsidx = -1;
			ismcs = FALSE;
			nvram_set(strcat_r(prefix, "nmcsidx", tmp), "-1");
		}

		/* Use nrate iovar only for MCS rate. */
		if (ismcs) {
			nrate |= NRATE_MCS_INUSE;
			nrate |= mcsidx & NRATE_RATE_MASK;

			WL_IOVAR_SETINT(name, "nrate", nrate);
		}
	}

	/* Set multicast rate in 500 Kbps units */
	val = atoi(nvram_safe_get(strcat_r(prefix, "mrate", tmp))) / 500000;
	/* 1Mbps and 2 Mbps are not allowed in BTC pre-emptive mode */
	if (btc_mode == WL_BTC_PREMPT && (val == 2 || val == 4))
		/* Must b/g band.  Set to 5.5Mbps */
		val = 11;

	/* it is band-blind. try both band */
	error_bg = wl_iovar_setint(name, "bg_mrate", val);
	error_a = wl_iovar_setint(name, "a_mrate", val);

	if (error_bg && error_a) {
		/* Try default rate (card may have changed) */
		val = 0;

		wl_iovar_setint(name, "bg_mrate", val);
		wl_iovar_setint(name, "a_mrate", val);

		snprintf(buf, sizeof(buf), "%d", val);
		nvram_set(strcat_r(prefix, "mrate", tmp), buf);
	}

	/* Set fragmentation threshold */
	val = atoi(nvram_safe_get(strcat_r(prefix, "frag", tmp)));
	wl_iovar_setint(name, "fragthresh", val);

	/* Set RTS threshold */
	val = atoi(nvram_safe_get(strcat_r(prefix, "rts", tmp)));
	wl_iovar_setint(name, "rtsthresh", val);

	/* Set DTIM period */
	val = atoi(nvram_safe_get(strcat_r(prefix, "dtim", tmp)));
	WL_IOCTL(name, WLC_SET_DTIMPRD, &val, sizeof(val));

	/* Set beacon period */
	val = atoi(nvram_safe_get(strcat_r(prefix, "bcn", tmp)));
	WL_IOCTL(name, WLC_SET_BCNPRD, &val, sizeof(val));

	/* Set beacon rotation */
	str = nvram_get(strcat_r(prefix, "bcn_rotate", tmp));
	if (!str) {
		/* No nvram variable found, use the default */
		str = "1"; //nvram_default_get(strcat_r(prefix, "bcn_rotate", tmp));
	}
	val = atoi(str);
	wl_iovar_setint(name, "bcn_rotate", val);

	/* Set framebursting mode */
	if (btc_mode == WL_BTC_PREMPT)
		val = FALSE;
	else
		val = nvram_match(strcat_r(prefix, "frameburst", tmp), "on");
	WL_IOCTL(name, WLC_SET_FAKEFRAG, &val, sizeof(val));

	/* Enable or disable PLC failover */
	val = atoi(nvram_safe_get(strcat_r(prefix, "plc", tmp)));
	WL_IOVAR_SETINT(name, "plc", val);

	/* Set STBC tx and rx mode */
	if (phytype == PHY_TYPE_N || phytype == PHY_TYPE_HT) {
		char *nvram_str = nvram_safe_get(strcat_r(prefix, "stbc_tx", tmp));

		if (!strcmp(nvram_str, "auto")) {
			WL_IOVAR_SETINT(name, "stbc_tx", AUTO);
		} else if (!strcmp(nvram_str, "on")) {
			WL_IOVAR_SETINT(name, "stbc_tx", ON);
		} else if (!strcmp(nvram_str, "off")) {
			WL_IOVAR_SETINT(name, "stbc_tx", OFF);
		}
		val = atoi(nvram_safe_get(strcat_r(prefix, "stbc_rx", tmp)));
		WL_IOVAR_SETINT(name, "stbc_rx", val);
	}

	/* Set RIFS mode based on framebursting */
	if (WLCONF_PHYTYPE_11N(phytype)) {
		char *nvram_str = nvram_safe_get(strcat_r(prefix, "rifs", tmp));
		if (!strcmp(nvram_str, "on"))
			wl_iovar_setint(name, "rifs", ON);
		else if (!strcmp(nvram_str, "off"))
			wl_iovar_setint(name, "rifs", OFF);

		/* RIFS mode advertisement */
		nvram_str = nvram_safe_get(strcat_r(prefix, "rifs_advert", tmp));
		if (!strcmp(nvram_str, "auto"))
			wl_iovar_setint(name, "rifs_advert", AUTO);
		else if (!strcmp(nvram_str, "off"))
			wl_iovar_setint(name, "rifs_advert", OFF);
	}

	/* Override BA mode only if set to on/off */
	ba = nvram_safe_get(strcat_r(prefix, "ba", tmp));
	if (!strcmp(ba, "on"))
		wl_iovar_setint(name, "ba", ON);
	else if (!strcmp(ba, "off"))
		wl_iovar_setint(name, "ba", OFF);

	if (WLCONF_PHYTYPE_11N(phytype)) {
		val = AVG_DMA_XFER_RATE;
		wl_iovar_set(name, "avg_dma_xfer_rate", &val, sizeof(val));
	}

	/* Bring the interface back up */
	WL_IOCTL(name, WLC_UP, NULL, 0);

	/* Set antenna */
	val = atoi(nvram_safe_get(strcat_r(prefix, "antdiv", tmp)));
	WL_IOCTL(name, WLC_SET_ANTDIV, &val, sizeof(val));

	/* Set radar parameters if it is enabled */
	if (radar_enab) {
		wlconf_set_radarthrs(name, prefix);
	}

	/* Set channel interference threshold value if it is enabled */
	str = nvram_get(strcat_r(prefix, "glitchthres", tmp));

	if (str) {
		int glitch_thres = atoi(str);
		if (glitch_thres > 0)
			WL_IOVAR_SETINT(name, "chanim_glitchthres", glitch_thres);
	}

	str = nvram_get(strcat_r(prefix, "ccathres", tmp));

	if (str) {
		int cca_thres = atoi(str);
		if (cca_thres > 0)
			WL_IOVAR_SETINT(name, "chanim_ccathres", cca_thres);
	}

	str = nvram_get(strcat_r(prefix, "chanimmode", tmp));

	if (str) {
		int chanim_mode = atoi(str);
		if (chanim_mode >= 0)
			WL_IOVAR_SETINT(name, "chanim_mode", chanim_mode);
	}

	/* Overlapping BSS Coexistence aka 20/40 Coex. aka OBSS Coex.
	 * For an AP - Only use if 2G band AND user wants a 40Mhz chanspec.
	 * For a STA - Always
	 */
	if (WLCONF_PHYTYPE_11N(phytype)) {
		if (sta ||
		    ((ap || apsta) && (nbw == WL_CHANSPEC_BW_40) && (bandtype == WLC_BAND_2G))) {
			str = nvram_safe_get(strcat_r(prefix, "obss_coex", tmp));
			if (!str) {
				/* No nvram variable found, use the default */
				str = "0"; //nvram_default_get(strcat_r(prefix, "obss_coex", tmp));
			}
			obss_coex = atoi(str);
		} else {
			/* Need to disable obss coex in case of 20MHz and/or
			 * in case of 5G.
			 */
			obss_coex = 0;
		}
#ifdef WLTEST
		/* force coex off for msgtest build */
		obss_coex = 0;
#endif
		WL_IOVAR_SETINT(name, "obss_coex", obss_coex);
	}

	/* Auto Channel Selection:
	 * 1. When channel # is 0 in AP mode, this determines our channel and 20Mhz vs. 40Mhz
	 * 2. If we're running OBSS Coex and the user specified a channel, Autochannel runs to
	 *    do an initial scan to help us make decisions about whether we can create a 40Mhz AP
	 */
	/* The following condition(s) must be met in order for Auto Channel Selection to work.
	 *  - the I/F must be up for the channel scan
	 *  - the AP must not be supporting a BSS (all BSS Configs must be disabled)
	 */
	if (ap || apsta) {
		int channel = atoi(nvram_safe_get(strcat_r(prefix, "channel", tmp)));
#ifdef EXT_ACS
		char tmp[100];
		char *ptr;
		char * val;

		val = nvram_safe_get("acs_mode");

		if (!strcmp(val, "legacy") || (rev.chipnum == BCM4331_CHIP_ID) ||
			(rev.chipnum == BCM43431_CHIP_ID))
			goto legacy_mode;

		snprintf(tmp, sizeof(tmp), "acs_ifnames");
		ptr = nvram_get(tmp);
		if (ptr)
			snprintf(buf, sizeof(buf), "%s %s", ptr, name);
		else
			strncpy(buf, name, sizeof(buf));
		nvram_set(tmp, buf);
		goto legacy_end;

legacy_mode:
#endif /* EXT_ACS */
		if (obss_coex || channel == 0) {
			if (WLCONF_PHYTYPE_11N(phytype)) {
				chanspec_t chanspec;
				int pref_chspec;

				if (channel != 0) {
					/* assumes that initial chanspec has been set earlier */
					/* Maybe we expand scope of chanspec from above so
					 * that we don't have to do the iovar_get here?
					 */

					/* We're not doing auto-channel, give the driver
					 * the preferred chanspec.
					 */
					WL_IOVAR_GETINT(name, "chanspec", &pref_chspec);
					WL_IOVAR_SETINT(name, "pref_chanspec", pref_chspec);
				} else {
					WL_IOVAR_SETINT(name, "pref_chanspec", 0);
				}
				chanspec = wlconf_auto_chanspec(name);
				if (chanspec != 0)
					WL_IOVAR_SETINT(name, "chanspec", chanspec);
			} else {
				/* select a channel */
				val = wlconf_auto_channel(name);
				/* switch to the selected channel */
				if (val != 0)
					WL_IOCTL(name, WLC_SET_CHANNEL, &val, sizeof(val));
			}
			/* set the auto channel scan timer in the driver when in auto mode */
			if (channel == 0) {
				val = 15;	/* 15 minutes for now */
			} else {
				val = 0;
			}
		} else {
			/* reset the channel scan timer in the driver when not in auto mode */
			val = 0;
		}

		WL_IOCTL(name, WLC_SET_CS_SCAN_TIMER, &val, sizeof(val));
		WL_IOVAR_SETINT(name, "chanim_mode", CHANIM_ACT);
#ifdef EXT_ACS
legacy_end:
		;
#endif
	}

	/* Security settings for each BSS Configuration */
	for (i = 0; i < bclist->count; i++) {
		bsscfg = &bclist->bsscfgs[i];
		wlconf_security_options(name, bsscfg->prefix, bsscfg->idx, mac_spoof,
			(wet || sta || apsta));
	}

	/*
	 * Finally enable BSS Configs or Join BSS
	 *
	 * AP: Enable BSS Config to bring AP up only when nas will not run
	 * STA: Join the BSS regardless.
	 */
	for (i = 0; i < bclist->count; i++) {
		struct {int bsscfg_idx; int enable;} setbuf;
		char vifname[VIFNAME_LEN];
		char *name_ptr = name;

		setbuf.bsscfg_idx = bclist->bsscfgs[i].idx;
		setbuf.enable = 0;

		bsscfg = &bclist->bsscfgs[i];
		if (nvram_match(strcat_r(bsscfg->prefix, "bss_enabled", tmp), "1")) {
			setbuf.enable = 1;
		}

		/* Set the MAC list */
		maclist = (struct maclist *)buf;
		maclist->count = 0;
		if (!nvram_match(strcat_r(bsscfg->prefix, "macmode", tmp), "disabled")) {
			ea = maclist->ea;
			foreach(var, nvram_safe_get(strcat_r(bsscfg->prefix, "maclist", tmp)),
				next) {
				if (((char *)((&ea[1])->octet)) > ((char *)(&buf[sizeof(buf)])))
					break;
				if (ether_atoe(var, ea->octet)) {
					maclist->count++;
					ea++;
				}
			}
		}

		if (setbuf.bsscfg_idx == 0) {
			name_ptr = name;
		} else { /* Non-primary BSS; changes name syntax */
			char tmp[VIFNAME_LEN];
			int len;

			/* Remove trailing _ if present */
			memset(tmp, 0, sizeof(tmp));
			strncpy(tmp, bsscfg->prefix, VIFNAME_LEN - 1);
			if (((len = strlen(tmp)) > 0) && (tmp[len - 1] == '_')) {
				tmp[len - 1] = 0;
			}
			nvifname_to_osifname(tmp, vifname, VIFNAME_LEN);
			name_ptr = vifname;
		}

		WL_IOCTL(name_ptr, WLC_SET_MACLIST, buf, sizeof(buf));

		/* Set macmode for each VIF */
		(void) strcat_r(bsscfg->prefix, "macmode", tmp);

		if (nvram_match(tmp, "deny"))
			val = WLC_MACMODE_DENY;
		else if (nvram_match(tmp, "allow"))
			val = WLC_MACMODE_ALLOW;
		else
			val = WLC_MACMODE_DISABLED;

		WL_IOCTL(name_ptr, WLC_SET_MACMODE, &val, sizeof(val));
	}

	ret = 0;
exit:
	if (bclist != NULL)
		free(bclist);

	return ret;
}

int
wlconf_down(char *name)
{
	int val, ret = 0;
	int i;
	int wlsubunit;
	int bcmerr;
	struct {int bsscfg_idx; int enable;} setbuf;
	int wl_ap_build = 0; /* 1 = wl compiled with AP capabilities */
	char cap[WLC_IOCTL_SMLEN];
	char caps[WLC_IOCTL_SMLEN];
	char *next;
	wlc_ssid_t ssid;

	/* wlconf doesn't work for virtual i/f */
	if (get_ifname_unit(name, NULL, &wlsubunit) == 0 && wlsubunit >= 0) {
		WLCONF_DBG("wlconf: skipping virtual interface \"%s\"\n", name);
		return 0;
	}

	/* Check interface (fail silently for non-wl interfaces) */
	if ((ret = wl_probe(name)))
		return ret;

	/* because of ifdefs in wl driver,  when we don't have AP capabilities we
	 * can't use the same iovars to configure the wl.
	 * so we use "wl_ap_build" to help us know how to configure the driver
	 */
	if (wl_iovar_get(name, "cap", (void *)caps, WLC_IOCTL_SMLEN))
		return -1;

	foreach(cap, caps, next) {
		if (!strcmp(cap, "ap")) {
			wl_ap_build = 1;
		}
	}

	if (wl_ap_build) {
		/* Bring down the interface */
		WL_IOCTL(name, WLC_DOWN, NULL, sizeof(val));

		/* Disable all BSS Configs */
		for (i = 0; i < WL_MAXBSSCFG; i++) {
			setbuf.bsscfg_idx = i;
			setbuf.enable = 0;

			ret = wl_iovar_set(name, "bss", &setbuf, sizeof(setbuf));
			if (ret) {
				wl_iovar_getint(name, "bcmerror", &bcmerr);
				/* fail quietly on a range error since the driver may
				 * support fewer bsscfgs than we are prepared to configure
				 */
				if (bcmerr == BCME_RANGE)
					break;
			}
		}
	} else {
		WL_IOCTL(name, WLC_GET_UP, &val, sizeof(val));
		if (val) {
			/* Nuke SSID */
			ssid.SSID_len = 0;
			ssid.SSID[0] = '\0';
			WL_IOCTL(name, WLC_SET_SSID, &ssid, sizeof(ssid));

			/* Bring down the interface */
			WL_IOCTL(name, WLC_DOWN, NULL, sizeof(val));
		}
	}

	/* Nuke the WDS list */
	wlconf_wds_clear(name);

	return 0;
}

int
wlconf_start(char *name)
{
	int i, ii, unit, val, ret = 0;
	int wlunit = -1;
	int wlsubunit = -1;
	int ap, apsta, wds, sta = 0, wet = 0;
	int wl_ap_build = 0; /* wl compiled with AP capabilities */
	char buf[WLC_IOCTL_SMLEN];
	struct maclist *maclist;
	struct ether_addr *ea;
	struct bsscfg_list *bclist = NULL;
	struct bsscfg_info *bsscfg;
	wlc_ssid_t ssid;
	char cap[WLC_IOCTL_SMLEN], caps[WLC_IOCTL_SMLEN];
	char var[80], tmp[100], prefix[PREFIX_LEN], *str, *next;

	/* Check interface (fail silently for non-wl interfaces) */
	if ((ret = wl_probe(name)))
		return ret;

	/* wlconf doesn't work for virtual i/f, so if we are given a
	 * virtual i/f return 0 if that interface is in it's parent's "vifs"
	 * list otherwise return -1
	 */
	memset(tmp, 0, sizeof(tmp));
	if (get_ifname_unit(name, &wlunit, &wlsubunit) == 0) {
		if (wlsubunit >= 0) {
			/* we have been given a virtual i/f,
			 * is it in it's parent i/f's virtual i/f list?
			 */
			sprintf(tmp, "wl%d_vifs", wlunit);

			if (strstr(nvram_safe_get(tmp), name) == NULL)
				return -1; /* config error */
			else
				return 0; /* okay */
		}
	}
	else {
		return -1;
	}

	/* because of ifdefs in wl driver,  when we don't have AP capabilities we
	 * can't use the same iovars to configure the wl.
	 * so we use "wl_ap_build" to help us know how to configure the driver
	 */
	if (wl_iovar_get(name, "cap", (void *)caps, WLC_IOCTL_SMLEN))
		return -1;

	foreach(cap, caps, next) {
		if (!strcmp(cap, "ap"))
			wl_ap_build = 1;
	}

	/* Get instance */
	WL_IOCTL(name, WLC_GET_INSTANCE, &unit, sizeof(unit));
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);


	/* Get the list of BSS Configs */
	if (!(bclist = wlconf_get_bsscfgs(name, prefix)))
		return -1;

	/* wlX_mode settings: AP, STA, WET, BSS/IBSS, APSTA */
	str = nvram_safe_get(strcat_r(prefix, "mode", tmp));
	ap = (!strcmp(str, "") || !strcmp(str, "ap"));
	apsta = (!strcmp(str, "apsta") ||
	         ((!strcmp(str, "sta") || !strcmp(str, "wet")) &&
	          bclist->count > 1));
	sta = (!strcmp(str, "sta") && bclist->count == 1);
	wds = !strcmp(str, "wds");
	wet = !strcmp(str, "wet");
	if (!strcmp(str, "mac_spoof"))
		sta = 1;

	/* Retain remaining WET effects only if not APSTA */
	wet &= !apsta;

	/* AP only config, code copied as-is from wlconf function */
	if (ap || apsta || wds) {
		/* Set lazy WDS mode */
		val = atoi(nvram_safe_get(strcat_r(prefix, "lazywds", tmp)));
		WL_IOCTL(name, WLC_SET_LAZYWDS, &val, sizeof(val));

		/* Set the WDS list */
		maclist = (struct maclist *) buf;
		maclist->count = 0;
		ea = maclist->ea;
		foreach(var, nvram_safe_get(strcat_r(prefix, "wds", tmp)), next) {
			if (((char *)(ea->octet)) > ((char *)(&buf[sizeof(buf)])))
				break;
			ether_atoe(var, ea->octet);
			maclist->count++;
			ea++;
		}
		WL_IOCTL(name, WLC_SET_WDSLIST, buf, sizeof(buf));

		/* Set WDS link detection timeout */
		val = atoi(nvram_safe_get(strcat_r(prefix, "wds_timeout", tmp)));
		wl_iovar_setint(name, "wdstimeout", val);
	}

	/*
	 * Finally enable BSS Configs or Join BSS
	 * code copied as-is from wlconf function
	 */
	for (i = 0; i < bclist->count; i++) {
		struct {int bsscfg_idx; int enable;} setbuf;

		setbuf.bsscfg_idx = bclist->bsscfgs[i].idx;
		setbuf.enable = 0;

		bsscfg = &bclist->bsscfgs[i];
		if (nvram_match(strcat_r(bsscfg->prefix, "bss_enabled", tmp), "1")) {
			setbuf.enable = 1;
		}

		/*  bring up BSS  */
		if (ap || apsta || sta || wet) {
			for (ii = 0; ii < MAX_BSS_UP_RETRIES; ii++) {
				if (wl_ap_build) {
					WL_IOVAR_SET(name, "bss", &setbuf, sizeof(setbuf));
				}
				else {
					strcat_r(prefix, "ssid", tmp);
					ssid.SSID_len = strlen(nvram_safe_get(tmp));
					if (ssid.SSID_len > sizeof(ssid.SSID))
						ssid.SSID_len = sizeof(ssid.SSID);
					strncpy((char *)ssid.SSID, nvram_safe_get(tmp),
						ssid.SSID_len);
					WL_IOCTL(name, WLC_SET_SSID, &ssid, sizeof(ssid));
				}
				if (apsta && (ret != 0))
					sleep_ms(1000);
				else
					break;
			}
		}
	}

	if (bclist != NULL)
		free(bclist);

	return ret;
}

#if defined(linux)
int
main(int argc, char *argv[])
{
	/* Check parameters and branch based on action */
	if (argc == 3 && !strcmp(argv[2], "up"))
		return wlconf(argv[1]);
	else if (argc == 3 && !strcmp(argv[2], "down"))
		return wlconf_down(argv[1]);
	else if (argc == 3 && !strcmp(argv[2], "start"))
	  return wlconf_start(argv[1]);
	else {
		fprintf(stderr, "Usage: wlconf <ifname> up|down\n");
		return -1;
	}
}
#endif /*  defined(linux) */
