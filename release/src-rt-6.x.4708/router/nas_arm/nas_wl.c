/*
 * Broadcom 802.11 device interface
 *
 * Copyright (C) 2013, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: nas_wl.c 394138 2013-04-01 05:24:26Z $
 */

#include <typedefs.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <bcmutils.h>
#include <wlutils.h>
#include <wlioctl.h>
#include <bcmendian.h>

#include <nas.h>

#define BUFFER_SIZE		256
#define HSPOT_WNM_TYPE		1
#define ACTION_FRAME_SIZE 1800
#define HSPOT_WNM_OUI_TYPE		0x00
#define WL_WIFI_AF_PARAMS_SIZE sizeof(struct wl_af_params)

static bool g_swap = FALSE;
#define htod32(i) (g_swap?bcmswap32(i):(uint32)(i))
#define htod16(i) (g_swap?bcmswap16(i):(uint16)(i))

typedef struct
{
	int maxLength;
	int length;
	uint8 *buf;
} wnm_encode_t;

#define bcm_encode_length(pkt)	\
	((pkt)->length)

#define bcm_encode_buf(pkt)		\
	((pkt)->buf)

static int isLengthValid(wnm_encode_t *pkt, int length)
{
	/* assert(pkt != 0); */
	if (pkt == 0)
		return FALSE;

	if (pkt->buf == 0)
		return FALSE;

	if (pkt->length + length > pkt->maxLength) {
		printf("length %d exceeds remaining buffer %d\n",
			length, pkt->maxLength - pkt->length);
		return 0;
	}

	return 1;
}

int bcm_encode_byte(wnm_encode_t *pkt, uint8 byte)
{
	/* assert(pkt != 0); */
	if (!isLengthValid(pkt, 1))
		return 0;

	pkt->buf[pkt->length++] = byte;
	return 1;
}

int bcm_encode_bytes(wnm_encode_t *pkt, int length, uint8 *bytes)
{
	/* assert(pkt != 0); */
	/* assert(bytes != 0); */
	if (!isLengthValid(pkt, length))
		return 0;

	memcpy(&pkt->buf[pkt->length], bytes, length);
	pkt->length += length;
	return length;
}

int bcm_encode_init(wnm_encode_t *pkt, int maxLength, uint8 *buf)
{
	/* assert(pkt != 0); */
	if (buf == 0)
		return FALSE;

	pkt->maxLength = maxLength;
	pkt->length = 0;
	pkt->buf = buf;

	return TRUE;
}

int bcm_encode_wnm_subscription_remediation(wnm_encode_t *pkt,
	uint8 dialogToken, uint16 urlLen, char *url)
{
	int initLen = bcm_encode_length(pkt);

	bcm_encode_byte(pkt, DOT11_ACTION_CAT_WNM);
	bcm_encode_byte(pkt, DOT11_WNM_ACTION_NOTFCTN_REQ);
	bcm_encode_byte(pkt, dialogToken);
	bcm_encode_byte(pkt, HSPOT_WNM_TYPE);
	bcm_encode_byte(pkt, DOT11_MNG_VS_ID);
	bcm_encode_byte(pkt, 5 + urlLen);
	bcm_encode_bytes(pkt, WFA_OUI_LEN, (uint8 *)WFA_OUI);
	bcm_encode_byte(pkt, HSPOT_WNM_OUI_TYPE);
	bcm_encode_byte(pkt, urlLen);
	if (urlLen > 0) {
		bcm_encode_bytes(pkt, urlLen, (uint8 *)url);
	}

	return bcm_encode_length(pkt) - initLen;
}

int wl_actframe(nas_t *nas, int bsscfg_idx, uint32 packet_id,
	uint32 channel, int32 dwell_time,
	struct ether_addr *BSSID, struct ether_addr *da,
	uint16 len, uint8 *data)
{
	wl_action_frame_t * action_frame;
	wl_af_params_t * af_params;
	struct ether_addr *bssid;
	int err = 0;

	if (len > ACTION_FRAME_SIZE)
		return -1;

	if ((af_params = (wl_af_params_t *) malloc(WL_WIFI_AF_PARAMS_SIZE)) == NULL) {
		return -1;
	}
	action_frame = &af_params->action_frame;

	/* Add the packet Id */
	action_frame->packetId = packet_id;

	memcpy(&action_frame->da, (char*)da, ETHER_ADDR_LEN);

	/* set default BSSID */
	bssid = da;
	if (BSSID != 0)
		bssid = BSSID;
	memcpy(&af_params->BSSID, (char*)bssid, ETHER_ADDR_LEN);

	action_frame->len = htod16(len);
	af_params->channel = htod32(channel);
	af_params->dwell_time = htod32(dwell_time);
	memcpy(action_frame->data, data, len);

	/* Calculate Lengths of Action Frame and af_param */
	int act_frame_len = sizeof(action_frame->da) + sizeof(action_frame->len)
		+ sizeof(action_frame->packetId) +  action_frame->len;
	int af_param_len = act_frame_len + sizeof(af_params->channel)
		+ sizeof(af_params->dwell_time)	+ sizeof(af_params->BSSID) + 2;

	char smbuf[WLC_IOCTL_MEDLEN] = {0};
	err =  wl_iovar_setbuf(nas->interface, "actframe", af_params,
		af_param_len, smbuf, sizeof(smbuf));

	free(af_params);
	return (err);
}

int
nas_send_wnm_on_radius_access_accept(nas_t *nas, char* url, struct ether_addr *ea)
{
	/* struct ether_addr bssid; */
	int urlLength;
	wnm_encode_t enc;
	uint8 buffer[WLC_IOCTL_SMLEN];
	uint8 dialogtoken = 1;
	urlLength = strlen(url);
	if (urlLength > WLC_IOCTL_SMLEN) {
		printf("<url> too long");
		return 0;
	}

	bcm_encode_init(&enc, sizeof(buffer), buffer);

	bcm_encode_wnm_subscription_remediation(&enc,
		dialogtoken, urlLength, url);

	/* send action frame */
	wl_actframe(nas, -1,
		(uint32)bcm_encode_buf(&enc), 0, 250, &nas->ea, ea,
		bcm_encode_length(&enc), bcm_encode_buf(&enc));

	return 1;
}

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

/* get current encryption algo bitvec */
int
nas_get_wpawsec(nas_t *nas, uint32 *wsec)
{
	int err;
	int32 val;

	err = wl_iovar_getint(nas->interface, "wsec", &val);
	if (!err)
		*wsec = val;

	return err;
}

/* get current WPA auth mode */
int
nas_get_wpaauth(nas_t *nas, uint32 *wpa_auth)
{
	int err;
	int32 val;

	err = wl_iovar_getint(nas->interface, "wpa_auth", &val);
	if (!err)
		*wpa_auth = val;

	return err;
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

int
nas_send_brcm_event(nas_t *nas, uint8* mac, int reason)
{
	int err;
	scb_val_t scb_val;
	scb_val.val = (uint32) reason;
	memcpy(&scb_val.ea, mac, ETHER_ADDR_LEN);
	err = wl_iovar_set(nas->interface, "nas_notify_event", &scb_val, sizeof(scb_val));
	if (err) {
		printf("set nas_notify_event iovar error = %d, name %s\n", err, nas->interface);
	}
	return err;
}
