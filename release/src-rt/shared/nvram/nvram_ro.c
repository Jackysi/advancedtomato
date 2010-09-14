/*
 * Read-only support for NVRAM on flash and otp.
 *
 * Copyright 2004, Broadcom Corporation
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
#include <osl.h>
#include <bcmnvram.h>
#include <bcmendian.h>
#include <bcmutils.h>
#include <sflash.h>
#include <sbconfig.h>
#include <sbchipc.h>
#include <sbutils.h>


static char *fnv = NULL;
static char *fnvlim = NULL;
static char *otp = NULL;
static char *otplim = NULL;
static char *otp2 = NULL;
static char *otp2lim = NULL;


static struct nvram_header *
find_flash_nvram(uint32 base, uint32 lim)
{
	struct nvram_header *nvh;
	uint32 off = FLASH_MIN;

	nvh = NULL;
	while (off <= lim) {
		nvh = (struct nvram_header *) (base + off - NVRAM_SPACE);
		if (nvh->magic == NVRAM_MAGIC)
			return nvh;
		off <<= 1;
	};

	/* Try embedded NVRAM at 4 KB and 1 KB as last resorts */
	nvh = (struct nvram_header *) (base + 4096);
	if (nvh->magic == NVRAM_MAGIC)
		return nvh;

	nvh = (struct nvram_header *) (base + 1024);
	if (nvh->magic == NVRAM_MAGIC)
		return nvh;

	return NULL;
}

int
BCMINITFN(nvram_init)(void *sbh)
{
	uint idx;
	chipcregs_t *cc;
	struct sflash *info;
	struct nvram_header *nvh = NULL;
	uint32 cap = 0, base, lim;


	/* Make sure */
	fnv = NULL;
	otp = NULL;
	otp2 = NULL;

	/* Check for flash */
	idx = sb_coreidx(sbh);
	if ((cc = sb_setcore(sbh, SB_CC, 0)) != NULL) {
		base = KSEG1ADDR(SB_FLASH2);
		lim = 0;
		cap = R_REG(&cc->capabilities);
		switch (cap & CAP_FLASH_MASK) {
		case PFLASH:
			lim = SB_FLASH2_SZ;
			break;

		case SFLASH_ST:
		case SFLASH_AT:
			if ((info = sflash_init(cc)) == NULL)
				break;
			lim = info->size;
			break;

		case FLASH_NONE:
		default:
			break;
		}
	} else {
		base = KSEG1ADDR(SB_FLASH1);
		lim = SB_FLASH1_SZ;
	}

	if (lim != 0)
		nvh = find_flash_nvram(base, lim);

	if (nvh != NULL) {
		fnv = (char *)&nvh[1];
		fnvlim = (char *)((uint32)nvh + NVRAM_SPACE);
	}

	/* Check for otp */
	if ((cc != NULL) && ((lim = cap & CAP_OTPSIZE) != 0)) {
		uint32 bound;
		uint16 *otpw, *d16;
		int i;

		otpw = (uint16 *)((uint32)cc + CC_OTP);
		lim = 1 << ((lim >> CAP_OTPSIZE_SHIFT) + 5);
		if ((otpw[OTP_HWSIGN] == OTP_SIGNATURE) &&
		    (otpw[0] == OTP_MAGIC)) {
			bound = otpw[OTP_BOUNDARY];
			if (bound >= lim) {
				printf("Bad boundary value in otp (%d >= %d)\n", bound, lim);
				goto out;
			}
			/* OK, we like otp, copy to ram, skipping the magic word */
			if ((otp = MALLOC(NULL,bound - 2)) == NULL) {
				printf("Out of memory for otp\n");
				goto out;
			}
			d16 = (uint16 *)otp;
			for (i = 1; i < (bound / 2); i++)
				*d16++ = otpw[i];
			otplim = otp + bound - 2;
			printf ("otp size = %d, hwsign = 0x%x, magic = 0x%x,  boundary = 0x%x\n",
				lim, otpw[OTP_HWSIGN], otpw[0], bound);

			/* Now do it again for the "second" part of the otp */
			if (otpw[OTP_SWSIGN] == OTP_SIGNATURE) {
				if ((otp2 = MALLOC(NULL, lim - bound)) == NULL) {
					printf("Out of memory for otp2\n");
					goto out;
				}
				d16 = (uint16 *)otp2;
				while (i < (lim / 2))
					*d16++ = otpw[i++];
				otp2lim = otp2 + lim - bound;
			}
		}
	}

out:	/* All done */
	sb_setcoreidx(sbh, idx);

	return 0;
}

void
BCMINITFN(nvram_exit)(void)
{
	if (otp) {
		MFREE(NULL, otp, otplim - otp);
		otp = NULL;
	}

	if (otp2) {
		MFREE(NULL, otp2, otp2lim - otp2);
		otp2 = NULL;
	}
}

static char *
findvar(char *vars, char *lim, const char *name)
{
	char *s;
	int len;

	len = strlen(name);

	for (s = vars; (s < lim) && *s; ) {
		if ((bcmp(s, name, len) == 0) && (s[len] == '='))
			return (&s[len+1]);

		while (*s++)
			;
	}

	return NULL;
}

char *
BCMINITFN(nvram_get)(const char *name)
{
	char *v = NULL;

	if ((fnv != NULL) && ((v = findvar(fnv, fnvlim, name)) != NULL))
		return v;

	if ((otp2 != NULL) && ((v = findvar(otp2, otp2lim, name)) != NULL))
		return v;

	if (otp != NULL)
		v = findvar(otp, otplim, name);

	return v;
}

int
BCMINITFN(nvram_set)(const char *name, const char *value)
{
	return 0;
}

int
BCMINITFN(nvram_unset)(const char *name)
{
	return 0;
}

int
BCMINITFN(nvram_commit)(void)
{
	return 0;
}

int
BCMINITFN(nvram_getall)(char *buf, int count)
{
	return 0;
}
