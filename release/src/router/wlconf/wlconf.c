/*
 * Wireless Network Adapter Configuration Utility
 *
 * Copyright 2008, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * $Id$
 */

#include <typedefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <bcmparams.h>
#include <shutils.h>
#include <wlutils.h>

/* phy types */
#define	PHY_TYPE_A		0
#define	PHY_TYPE_B		1
#define	PHY_TYPE_G		2
#define	PHY_TYPE_N		4
#define	PHY_TYPE_LP		5
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

/*
 * Debugging Macros
 */
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
#endif
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

#if defined(linux)
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
		sleep_ms(1000);
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
				 (phy) == PHY_TYPE_G ? "g" : "n")
#define WLCONF_STR2PHYTYPE(phy)	((phy) && (phy)[0] == 'a' ? PHY_TYPE_A : \
				 (phy) && (phy)[0] == 'b' ? PHY_TYPE_B : \
				 (phy) && (phy)[0] == 'l' ? PHY_TYPE_LP : \
				 (phy) && (phy)[0] == 'g' ? PHY_TYPE_G : PHY_TYPE_N)

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
wlconf_security_options(char *name, char *prefix, int bsscfg_idx, bool id_supp)
{
	int i;
	int val;
	int ret;
	char tmp[100];

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
	/* enable in-driver wpa supplicant? */
	if (id_supp && !nvram_get_int(strcat_r(prefix, "disable_wpa_supp", tmp)) && (CHECK_PSK(val))) {
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
	char tmp[100], var[80], *next, *wme_val;
	char buf[WLC_IOCTL_SMLEN];
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
	wl_iovar_get(name, "cap", (void *)tmp, 100);
	foreach(var, tmp, next) {
		char *nvram_str = nvram_get(strcat_r(prefix, var, buf));

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

		if (strncmp(var, "amsdu", sizeof(var)) == 0) {
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

#define VIFNAME_LEN 16

/* configure the specified wireless interface */
int
wlconf(char *name)
{
	int restore_defaults, val, unit, phytype, bandtype, gmode = 0, ret = 0;
	int bcmerr;
	int error_bg, error_a;
	struct bsscfg_list *bclist = NULL;
	struct bsscfg_info *bsscfg;
	char tmp[100], prefix[PREFIX_LEN];
	char var[80], *next, phy[] = "a", *str, *addr = NULL;
	/* Pay attention to buffer length requirements when using this */
	char buf[WLC_IOCTL_SMLEN];
	char *country;
	wlc_rev_info_t rev;
	channel_info_t ci;
	struct maclist *maclist;
	struct ether_addr *ea;
	wlc_ssid_t ssid;
	wl_rateset_t rs;
	unsigned int i;
	char eaddr[32];
	int ap, apsta, wds, sta = 0, wet = 0, mac_spoof = 0;
	char country_code[4];
	int nas_will_run = 0;
	char *ba;
#ifdef BCMWPA2
	char *preauth;
	int set_preauth;
#endif
	int ii;
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

	/* wlconf doesn't work for virtual i/f, so if we are given a
	 * virtual i/f return 0 if that interface is in it's parent's "vifs"
	 * list otherwise return -1
	 */
	if (get_ifname_unit(name, &wlunit, &wlsubunit) == 0)
	{
		if (wlsubunit >= 0)
		{
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
	else
	{
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
		}
		else if (!strcmp(cap, "mbss16"))
			max_no_vifs = 16;
		else if (!strcmp(cap, "mbss4"))
			max_no_vifs = 4;
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
		if (ret)
			WLCONF_DBG("%d:(%s): setting bsscfg #%d iovar \"bss\" to 0"
			           " (down) failed, ret = %d, bcmerr = %d\n",
			           __LINE__, name, i, ret, bcmerr);
	}

	/* Get the list of BSS Configs */
	bclist = wlconf_get_bsscfgs(name, prefix);
	if (bclist == NULL) {
		ret = -1;
		goto exit;
	}


	/* create a wlX.Y_ifname nvram setting */
	for (i = 1; i < bclist->count; i++) {
		bsscfg = &bclist->bsscfgs[i];
#if defined(linux) || defined(__ECOS)
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

				nvram_set(tmp, ether_etoa((uchar *)vif_addr,
				                          eaddr));
			}
		}
		/* The addresses are available in NVRAM, so set them */
		for (i = 1; i < max_no_vifs; i++) {
			snprintf(tmp, sizeof(tmp), "wl%d.%d_bss_enabled",
			         unit, i);
			if (!strcmp(nvram_safe_get(tmp), "1")) {
				snprintf(tmp, sizeof(tmp), "wl%d.%d_hwaddr",
				         unit, i);
				ether_atoe(nvram_safe_get(tmp), (unsigned char *)eaddr);
				WL_BSSIOVAR_SET(name, "cur_etheraddr", i,
				                eaddr, ETHER_ADDR_LEN);
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
	if (ap | apsta) {
		max_assoc = val = atoi(nvram_safe_get(strcat_r(prefix, "maxassoc", tmp)));
		if (val > 0) {
			WL_IOVAR_SETINT(name, "maxassoc", val);
		} else { /* Get value from driver if not in nvram */
			WL_IOVAR_GETINT(name, "maxassoc", &max_assoc);
		}
	}

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
	}

	/* Set up the country code */
	(void) strcat_r(prefix, "country_code", tmp);
	country = nvram_get(tmp);
	if (country && country[0] != '\0') {
		strncpy(country_code, country, sizeof(country_code));
		WL_IOCTL(name, WLC_SET_COUNTRY, country_code, strlen(country_code)+1);
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
		phy[0] = var[i];
		val = WLCONF_STR2PHYTYPE(phy);
		if (val == PHY_TYPE_N) {
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
	val = WLCONF_STR2PHYTYPE(str);
	/* For NPHY use band value from NVRAM */
	if (val == PHY_TYPE_N) {
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
		phy[0] = var[0];
		val = WLCONF_STR2PHYTYPE(phy);
		val = WLCONF_PHYTYPE2BAND(val);
		WL_SETINT(name, WLC_SET_BAND, val);
	}

	/* Store the resolved bandtype */
	bandtype = val;

	/* Get current core revision */
	WL_IOCTL(name, WLC_GET_REVINFO, &rev, sizeof(rev));
	snprintf(buf, sizeof(buf), "%d", rev.corerev);
	nvram_set(strcat_r(prefix, "corerev", tmp), buf);

	/* Get current phy type */
	WL_IOCTL(name, WLC_GET_PHYTYPE, &phytype, sizeof(phytype));
	snprintf(buf, sizeof(buf), "%s", WLCONF_PHYTYPE2STR(phytype));
	nvram_set(strcat_r(prefix, "phytype", tmp), buf);

	/* Setup regulatory mode */
	strcat_r(prefix, "reg_mode", tmp);
	if (nvram_match(tmp, "off"))  {
		val = 0;
		WL_IOCTL(name, WLC_SET_REGULATORY, &val, sizeof(val));
		WL_IOCTL(name, WLC_SET_RADAR, &val, sizeof(val));
		WL_IOCTL(name, WLC_SET_SPECT_MANAGMENT, &val, sizeof(val));
	} else if (nvram_match(tmp, "h")) {
		val = 0;
		WL_IOCTL(name, WLC_SET_REGULATORY, &val, sizeof(val));
		val = 1;
		WL_IOCTL(name, WLC_SET_RADAR, &val, sizeof(val));
		radar_enab = TRUE;
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

	/* Reset to hardware rateset (band may have changed) */
	WL_IOCTL(name, WLC_GET_RATESET, &rs, sizeof(wl_rateset_t));
	WL_IOCTL(name, WLC_SET_RATESET, &rs, sizeof(wl_rateset_t));

	/* set bandwidth capability for nphy and calculate nbw */
	if (phytype == PHY_TYPE_N) {
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
	}

	/* Set channel before setting gmode or rateset */
	/* Manual Channel Selection - when channel # is not 0 */
	val = atoi(nvram_safe_get(strcat_r(prefix, "channel", tmp)));
	if (val && phytype != PHY_TYPE_N) {
		WL_SETINT(name, WLC_SET_CHANNEL, val);
		if (ret) {
			/* Use current channel (card may have changed) */
			WL_IOCTL(name, WLC_GET_CHANNEL, &ci, sizeof(ci));
			snprintf(buf, sizeof(buf), "%d", ci.target_channel);
			nvram_set(strcat_r(prefix, "channel", tmp), buf);
		}
	} else if (val && phytype == PHY_TYPE_N) {
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
	if (phytype == PHY_TYPE_N) {
		int count;
		int streams;

		wl_iovar_getint(name, "txchain_cnt", &count);
		/* update NVRAM with capabilities */
		snprintf(var, sizeof(var), "%d", count);
		nvram_set(strcat_r(prefix, "txchain_cnt", tmp), var);

		/* Verify that there is an NVRAM param for txstreams, if not create it and
		 * set it to txchain_cnt
		 */
		streams = atoi(nvram_safe_get(strcat_r(prefix, "txstreams", tmp)));
		if (streams == 0) {
			/* invalid - NVRAM needs to be fixed/initialized */
			nvram_set(strcat_r(prefix, "txstreams", tmp), var);
			streams = count;
		}
		/* Apply user configured txstreams, use 1 if user disabled nmode */
		if (nmode == OFF)
			streams = 1;
		WL_IOVAR_SETINT(name, "txstreams", streams);

		wl_iovar_getint(name, "rxchain_cnt", &count);
		/* update NVRAM with capabilities */
		snprintf(var, sizeof(var), "%d", count);
		nvram_set(strcat_r(prefix, "rxchain_cnt", tmp), var);

		/* Verify that there is an NVRAM param for rxstreams, if not create it and
		 * set it to txchain_cnt
		 */
		streams = atoi(nvram_safe_get(strcat_r(prefix, "rxstreams", tmp)));
		if (streams == 0) {
			/* invalid - NVRAM needs to be fixed/initialized */
			nvram_set(strcat_r(prefix, "rxstreams", tmp), var);
			streams = count;
		}

		/* Apply user configured rxstreams, use 1 if user disabled nmode */
		if (nmode == OFF)
			streams = 1;
		WL_IOVAR_SETINT(name, "rxstreams", streams);
	}

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
	if (phytype == PHY_TYPE_N) {
		int override = WLC_PROTECTION_OFF;
		int control = WLC_PROTECTION_CTL_OFF;

		/* Set n protection override and control algorithm */
		strcat_r(prefix, "nmode_protection", tmp);

		if (nvram_match(tmp, "auto")) {
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

	/* Set BTC mode */
	if (!wl_iovar_setint(name, "btc_mode", btc_mode)) {
		if (btc_mode == WL_BTC_PREMPT) {
			wl_rateset_t rs_tmp = rs;
			/* remove 1Mbps and 2 Mbps from rateset */
			for (i = 0, rs.count = 0; i < rs_tmp.count; i++) {
				if ((rs_tmp.rates[i] & 0x7f) == 2 || (rs_tmp.rates[i] & 0x7f) == 4)
					continue;
				rs.rates[rs.count++] = rs_tmp.rates[i];
			}
		}
	}

	/* Allow short preamble override for b cards */
	if (phytype == PHY_TYPE_B ||
	    (phytype == PHY_TYPE_G && (gmode == GMODE_LEGACY_B || gmode == GMODE_AUTO))) {
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
	if (phytype == PHY_TYPE_N) {
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
			mcsidx =  -1;
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

	/* Set framebursting mode */
	if (btc_mode == WL_BTC_PREMPT)
		val = FALSE;
	else
		val = nvram_match(strcat_r(prefix, "frameburst", tmp), "on");
	WL_IOCTL(name, WLC_SET_FAKEFRAG, &val, sizeof(val));

	/* Set RIFS mode based on framebursting */
	if (phytype == PHY_TYPE_N) {
		char *nvram_str = nvram_safe_get(strcat_r(prefix, "rifs", tmp));
		if (!strcmp(nvram_str, "on"))
			wl_iovar_setint(name, "rifs", ON);
		else if (!strcmp(nvram_str, "off"))
			wl_iovar_setint(name, "rifs", OFF);
	}

	/* Override BA mode only if set to on/off */
	ba = nvram_safe_get(strcat_r(prefix, "ba", tmp));
	if (!strcmp(ba, "on"))
		wl_iovar_setint(name, "ba", ON);
	else if (!strcmp(ba, "off"))
		wl_iovar_setint(name, "ba", OFF);

	if (phytype == PHY_TYPE_N) {
		val = AVG_DMA_XFER_RATE;
		wl_iovar_set(name, "avg_dma_xfer_rate", &val, sizeof(val));
	}

	/* Bring the interface back up */
	WL_IOCTL(name, WLC_UP, NULL, 0);

	/* Set antenna */
	val = atoi(nvram_safe_get(strcat_r(prefix, "antdiv", tmp)));
	WL_IOCTL(name, WLC_SET_ANTDIV, &val, sizeof(val));


	/* Get current rateset (gmode may have changed) */
	WL_IOCTL(name, WLC_GET_RATESET, &rs, sizeof(wl_rateset_t));

	strcat_r(prefix, "rateset", tmp);
	if (nvram_match(tmp, "all"))  {
		/* Make all rates basic */
		for (i = 0; i < rs.count; i++)
			rs.rates[i] |= 0x80;
	} else if (nvram_match(tmp, "12") && bandtype == WLC_BAND_2G) {
		/* Make 1 and 2 basic */
		for (i = 0; i < rs.count; i++) {
			if ((rs.rates[i] & 0x7f) == 2 || (rs.rates[i] & 0x7f) == 4)
				rs.rates[i] |= 0x80;
			else
				rs.rates[i] &= ~0x80;
		}
	}

	/* Set rateset. */
	WL_IOCTL(name, WLC_SET_RATESET, &rs, sizeof(wl_rateset_t));

	/* Set radar parameters if it is enabled */
	if (radar_enab) {
		wlconf_set_radarthrs(name, prefix);
	}

	/* The following condition(s) must be met in order for Auto Channel Selection to work.
	 *  - the I/F must be up for the channel scan
	 *  - the AP must not be supporting a BSS (all BSS Configs must be disabled)
	 */
	if (ap || apsta) {
		int channel = atoi(nvram_safe_get(strcat_r(prefix, "channel", tmp)));

		if (channel == 0) {
			if (phytype == PHY_TYPE_N) {
				chanspec_t chanspec;
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
		}
		else {
			/* reset the channel scan timer in the driver when not in auto mode */
			val = 0;
		}

		WL_IOCTL(name, WLC_SET_CS_SCAN_TIMER, &val, sizeof(val));
	}

	/* Security settings for each BSS Configuration */
	for (i = 0; i < bclist->count; i++) {
		bsscfg = &bclist->bsscfgs[i];
		wlconf_security_options(name, bsscfg->prefix, bsscfg->idx, wet | sta);
	}

	/* AP only config */
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

		/* NAS runs if we have an AKM or radius authentication */
		nas_will_run = wlconf_akm_options(bclist->bsscfgs[i].prefix) ||
		        nvram_match(strcat_r(bclist->bsscfgs[i].prefix, "auth_mode", tmp),
		                    "radius");

		if (((ap || apsta) && !nas_will_run) || sta || wet) {
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
	}
	else {
		WL_IOCTL(name, WLC_GET_UP, &val, sizeof(val));
		if (val) {
			/* Nuke SSID  */
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

#if defined(linux)
int
main(int argc, char *argv[])
{
	/* Check parameters and branch based on action */
	if (argc == 3 && !strcmp(argv[2], "up"))
		return wlconf(argv[1]);
	else if (argc == 3 && !strcmp(argv[2], "down"))
		return wlconf_down(argv[1]);
	else {
		fprintf(stderr, "Usage: wlconf <ifname> up|down\n");
		return -1;
	}
}
#endif
