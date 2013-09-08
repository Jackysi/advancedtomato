/*
 *   wep.c - WEP functions
 *
<<<<<<< HEAD
 * Copyright (C) 2010, Broadcom Corporation
=======
 * Copyright (C) 2011, Broadcom Corporation
>>>>>>> 055422e... import shared dir, include, emf, bcm57xx and bcmcrypto
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
<<<<<<< HEAD
 * $Id: wep.c,v 1.4 2007-04-29 04:36:19 Exp $
=======
 * $Id: wep.c 281526 2011-09-02 17:10:12Z $
>>>>>>> 055422e... import shared dir, include, emf, bcm57xx and bcmcrypto
 */

#include <typedefs.h>

/* include wl driver config file if this file is compiled for driver */
#ifdef BCMDRIVER
#include <osl.h>
#else
<<<<<<< HEAD
#if defined(__GNUC__)
extern void bcopy(const void *src, void *dst, int len);
extern int bcmp(const void *b1, const void *b2, int len);
extern void bzero(void *b, int len);
#else
#define	bcopy(src, dst, len)	memcpy((dst), (src), (len))
#define	bcmp(b1, b2, len)		memcmp((b1), (b2), (len))
#define	bzero(b, len)			memset((b), 0, (len))
#endif /* defined(__GNUC__) */
=======
#include <string.h>
>>>>>>> 055422e... import shared dir, include, emf, bcm57xx and bcmcrypto
#endif /* BCMDRIVER */

#include <bcmutils.h>
#include <bcmcrypto/rc4.h>
#include <bcmcrypto/wep.h>
#include <proto/802.11.h>

/* WEP-encrypt a buffer */
/* assumes a contiguous buffer, with IV prepended, and with enough space at
 * the end for the ICV
 */
void
BCMROMFN(wep_encrypt)(uint buf_len, uint8 *buf, uint sec_len, uint8 *sec_data)
{
	uint8 key_data[16];
	uint32 ICV;
	rc4_ks_t ks;
	uint8 *body = buf + DOT11_IV_LEN;
	uint body_len = buf_len - (DOT11_IV_LEN + DOT11_ICV_LEN);
	uint8 *picv = body + body_len;

<<<<<<< HEAD
	bcopy(buf, key_data, 3);
	bcopy(sec_data, &key_data[3], sec_len);
=======
	memcpy(key_data, buf, 3);
	memcpy(&key_data[3], sec_data, sec_len);
>>>>>>> 055422e... import shared dir, include, emf, bcm57xx and bcmcrypto

	prepare_key(key_data, sec_len + 3, &ks);

	/* append ICV */
	ICV = ~hndcrc32(body, body_len, CRC32_INIT_VALUE);
	picv[0] = ICV & 0xff;
	picv[1] = (ICV >> 8) & 0xff;
	picv[2] = (ICV >> 16) & 0xff;
	picv[3] = (ICV >> 24) & 0xff;

	rc4(body, body_len + DOT11_ICV_LEN, &ks);
}

<<<<<<< HEAD
/* WEP-decrypt 
=======
/* WEP-decrypt
>>>>>>> 055422e... import shared dir, include, emf, bcm57xx and bcmcrypto
 * Assumes a contigous buffer, with IV prepended.
 * Returns TRUE if ICV check passes, FALSE otherwise
 *
 */
bool
BCMROMFN(wep_decrypt)(uint buf_len, uint8 *buf, uint sec_len, uint8 *sec_data)
{
	uint8 key_data[16];
	rc4_ks_t ks;

<<<<<<< HEAD
	bcopy(buf, key_data, 3);
	bcopy(sec_data, &key_data[3], sec_len);
=======
	memcpy(key_data, buf, 3);
	memcpy(&key_data[3], sec_data, sec_len);
>>>>>>> 055422e... import shared dir, include, emf, bcm57xx and bcmcrypto

	prepare_key(key_data, sec_len + 3, &ks);

	rc4(buf + DOT11_IV_LEN, buf_len - DOT11_IV_LEN, &ks);

	return (hndcrc32(buf + DOT11_IV_LEN, buf_len - DOT11_IV_LEN, CRC32_INIT_VALUE) ==
		CRC32_GOOD_VALUE);
}
