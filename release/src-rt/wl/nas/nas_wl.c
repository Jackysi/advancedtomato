/*
 * Broadcom 802.11 device interface
 *
 * Copyright (C) 2010, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: nas_wl.c 248185 2011-03-23 06:37:49Z simonk $
 */

#include <typedefs.h>
#include <unistd.h>
#include <string.h>

#include <bcmutils.h>
#include <wlutils.h>
#include <wlioctl.h>

#include <nas.h>

int
nas_authorize(nas_t *nas, struct ether_addr *ea)
{
	return wl_ioctl(nas->interface, WLC_SCB_AUTHORIZE, ea, ETHER_ADDR_LEN);
}

int
nas_deauthorize(nas_t *nas, struct ether_addr *ea)
{
	return wl_ioctl(nas->interface, WLC_SCB_DEAUTHORIZE, ea, ETHER_ADDR_LEN);
}

int
nas_deauthenticate(nas_t *nas, struct ether_addr *ea, int reason)
{
	scb_val_t scb_val;

	/* remove the key if one is installed */
	nas_set_key(nas, ea, NULL, 0, 0, 0, 0, 0);
	scb_val.val = (uint32) reason;
	memcpy(&scb_val.ea, ea, ETHER_ADDR_LEN);
	return wl_ioctl(nas->interface, WLC_SCB_DEAUTHENTICATE_FOR_REASON,
	                &scb_val, sizeof(scb_val));
}

int
nas_get_group_rsc(nas_t *nas, uint8 *buf, int index)
{
	union {
		int index;
		uint8 rsc[EAPOL_WPA_KEY_RSC_LEN];
	} u;

	u.index = index;
	if (wl_ioctl(nas->interface, WLC_GET_KEY_SEQ, &u, sizeof(u)) != 0)
		return -1;

	bcopy(u.rsc, buf, EAPOL_WPA_KEY_RSC_LEN);

	return 0;
}

int
nas_set_key(nas_t *nas, struct ether_addr *ea, uint8 *key, int len, int index,
            int tx, uint32 hi, uint16 lo)
{
	wl_wsec_key_t wep;
#ifdef BCMDBG
	char eabuf[ETHER_ADDR_STR_LEN];
#endif
	char ki[] = "index XXXXXXXXXXX";

	memset(&wep, 0, sizeof(wep));
	wep.index = index;
	if (ea)
		memcpy(&wep.ea, ea, ETHER_ADDR_LEN);
	else {
		wep.flags = tx ? WL_PRIMARY_KEY : 0;
		snprintf(ki, sizeof(ki), "index %d", index);
	}

	wep.len = len;
	if (len)
		memcpy(wep.data, key, MIN(len, DOT11_MAX_KEY_SIZE));
	dbg(nas, "%s, flags %x, len %d",
		ea ? (char *)ether_etoa((uchar *)ea, eabuf) : ki,
		wep.flags, wep.len);
	if (lo || hi) {
		wep.rxiv.hi = hi;
		wep.rxiv.lo = lo;
		wep.iv_initialized = 1;
	}
	return wl_ioctl(nas->interface, WLC_SET_KEY, &wep, sizeof(wep));
}

int
nas_wl_tkip_countermeasures(nas_t *nas, int enable)
{
	return wl_ioctl(nas->interface, WLC_TKIP_COUNTERMEASURES, &enable, sizeof(int));
}

int
nas_set_ssid(nas_t *nas, char *ssid)
{
	wlc_ssid_t wl_ssid;

	strncpy((char *)wl_ssid.SSID, ssid, sizeof(wl_ssid.SSID));
	wl_ssid.SSID_len = strlen(ssid);
	return wl_ioctl(nas->interface, WLC_SET_SSID, &wl_ssid, sizeof(wl_ssid));
}

int
nas_disassoc(nas_t *nas)
{
	return wl_ioctl(nas->interface, WLC_DISASSOC, NULL, 0);
}

/* get WPA capabilities */
int
nas_get_wpacap(nas_t *nas, uint8 *cap)
{
	int err;
	int cap_val;

	err = wl_iovar_getint(nas->interface, "wpa_cap", &cap_val);
	if (!err) {
		cap[0] = (cap_val & 0xff);
		cap[1] = ((cap_val >> 8) & 0xff);
	}

	return err;
}

/* get STA info */
int
nas_get_stainfo(nas_t *nas, char *macaddr, int len, char *ret_buf, int ret_buf_len)
{
	char *tmp_ptr;

	tmp_ptr = ret_buf;
	strcpy(ret_buf, "sta_info");
	tmp_ptr += strlen(ret_buf);
	tmp_ptr++;
	memcpy(tmp_ptr, macaddr, len);

	return wl_ioctl(nas->interface, WLC_GET_VAR, ret_buf, ret_buf_len);
}

int
nas_get_wpa_ie(nas_t *nas, char *ret_buf, int ret_buf_len, uint32 sta_mode)
{
	int err, wpa_len = 0, ie_len = 0;
	char buf[WLC_IOCTL_SMLEN];
	bcm_tlv_t *ie_getbuf;
	char *buf_ptr;
	int i;

	if (ret_buf == NULL)
		return 0;

	memset(buf, 0, sizeof(buf));

	err = wl_iovar_getbuf(nas->interface, "wpaie", &nas->ea, 6, buf, sizeof(buf));
	if (err != 0)
		return 0;

	buf_ptr = buf;
	for (i = 0; i < 2; i++) {
		ie_getbuf = (bcm_tlv_t *)buf_ptr;
		wpa_len = ie_getbuf->len;
		if (wpa_len == 0)
			break;

		if (ie_getbuf->id == DOT11_MNG_RSN_ID) {
			dbg(nas, "found RSN IE of length %d\n", wpa_len);
			if (sta_mode == WPA || sta_mode == WPA_PSK) {
				buf_ptr += wpa_len + TLV_HDR_LEN;
				continue;
			}
		}
		else if (ie_getbuf->id != DOT11_MNG_RSN_ID) {
			dbg(nas, "found WPA IE of length %d\n", wpa_len);
			if (sta_mode == WPA2 || sta_mode == WPA2_PSK) {
				buf_ptr += wpa_len + TLV_HDR_LEN;
				continue;
			}
		}

		ie_len = wpa_len + TLV_HDR_LEN;
		/* Got WPA ie */
		if (ie_len <= ret_buf_len) {
			memcpy(ret_buf, buf_ptr, wpa_len + TLV_HDR_LEN);
			printf("return %d bytes ie\n", ie_len);
			return ie_len;
		}
		else {
			dbg(nas, "Not enough buffer (%d) to copy %d bytes wpa ie\n",
			    ret_buf_len, ie_len);
			return 0;
		}
	}

	return 0;
}
