/*
 * Write-once support for IPX OTP wrapper.
 *
 * Copyright 2007, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */

#include <typedefs.h>
#include <bcmdefs.h>
#include <osl.h>
#include <bcmdevs.h>
#include <bcmutils.h>
#include <sbutils.h>
#include <bcmendian.h>
#include <sbconfig.h>
#include <sbchipc.h>
#include <bcmotp.h>

/* debug/trace */
#define OTP_MSG(x)


#if !defined(CONFIG_BCMHNDOTP)	    /* Newer IPX OTP wrapper */

/* OTP layout */
/* Subregion word offsets in General Use region */
#define OTPGU_HSB_OFF		12
#define OTPGU_SFB_OFF		13
#define OTPGU_CI_OFF		14
#define OTPGU_SROM_OFF		16

/* Fixed size subregions sizes in words */
#define OTPGU_CI_SZ		2

/* Flag bit offsets in General Use region  */
#define OTPGU_HWP_OFF		252
#define OTPGU_SWP_OFF		253
#define OTPGU_CIP_OFF		254
#define OTPGU_FUSEP_OFF		255

typedef struct {
	sb_t	*sbh;		/* Saved sb handle */
	osl_t	*osh;
	uint16	size;		/* Size of otp in words */
	uint16	rows;
	uint16	cols;
	uint16	hwprot;		/* Hardware protection bits */
	uint16	prog;		/* Subregion programmed bits */
	uint16	hwbase;		/* hardware subregion offset */
	uint16	hwlim;		/* hardware subregion boundary */
	uint16	swbase;		/* software subregion offset */
	uint16	swlim;		/* software subregion boundary */
	uint16	fbase;		/* fuse subregion offset */
	uint16	flim;		/* fuse subregion boundary */
} otpinfo_t;

static otpinfo_t otpinfo;

#define OTPP_TRIES	10000000	/* # of tries for OTPP */

static void
otp_rgn(otpinfo_t *oi, chipcregs_t *cc)
{
	/* Read OTP lock bits and subregion programmed indication bits */
	oi->hwprot = (uint16)(R_REG(oi->osh, &cc->otpstatus) & OTPS_OL_MASK);
	oi->prog = (uint16)(R_REG(oi->osh, &cc->otpstatus) & OTPS_GUP_MASK);
	OTP_MSG(("otp_rgn: hwprot %x prog %x\n", oi->hwprot, oi->prog));

	/*
	 * h/w region base and fuse region limit are fixed to the top and
	 * the bottom of the general use region. Everything else can be flexible.
	 */
	oi->hwbase = OTPGU_SROM_OFF;
	oi->hwlim = oi->size;
	if (oi->prog & OTPS_GUP_HW) {
		oi->hwlim = otpr(oi, cc, OTPGU_HSB_OFF) / 16;
		oi->swbase = oi->hwlim;
	}
	else
		oi->swbase = oi->hwbase;
	OTP_MSG(("otp_rgn: hwbase %x hwlim %x\n", oi->hwbase, oi->hwlim));
	oi->swlim = oi->size;
	if (oi->prog & OTPS_GUP_SW) {
		oi->swlim = otpr(oi, cc, OTPGU_SFB_OFF) / 16;
		oi->fbase = oi->swlim;
	}
	else
		oi->fbase = oi->swbase;
	OTP_MSG(("otp_rgn: swbase %x swlim %x\n", oi->swbase, oi->swlim));
	oi->flim = oi->size;
	OTP_MSG(("otp_rgn: fbase %x flim %x\n", oi->fbase, oi->flim));
}

void *
otp_init(sb_t *sbh)
{
	uint idx;
	chipcregs_t *cc;
	otpinfo_t *oi;

	/* chipc corerev must be >= 21 */
	if (sbh->ccrev < 21)
		return NULL;

	oi = &otpinfo;
	bzero(oi, sizeof(otpinfo_t));

	oi->sbh = sbh;
	oi->osh = sb_osh(sbh);

	/* Check for otp size */
	switch ((sbh->cccaps & CC_CAP_OTPSIZE) >> CC_CAP_OTPSIZE_SHIFT) {
	case 0:
		/* Nothing there */
		OTP_MSG(("%s: no OTP\n", __FUNCTION__));
		return NULL;
	case 1:	/* 32x64 */
		oi->rows = 32;
		oi->cols = 64;
		oi->size = 128;
		OTP_MSG(("otp_init: rows %u cols %u\n", oi->rows, oi->cols));
		break;
	default:
		/* Don't know the geometry */
		OTP_MSG(("%s: unknown OTP geometry\n", __FUNCTION__));
		return NULL;
	}

	/* Retrieve OTP region info */
	idx = sb_coreidx(sbh);
	cc = sb_setcore(sbh, SB_CC, 0);
	ASSERT(cc);

	switch (sbh->chip) {
#if defined(CONFIG_BCM4325)
	case BCM4325_CHIP_ID:
		if ((sbh->chipst & CST4325_SPROM_OTP_SEL_MASK) == CST4325_OTP_PWRDN) {
			OTP_MSG(("%s: OTP is strapped down\n", __FUNCTION__));
			oi = NULL;
			goto exit;
		}
		if (!(R_REG(oi->osh, &cc->min_res_mask) & PMURES_BIT(RES4325_LNLDO2_PU))) {
			OTP_MSG(("%s: OTP is powered down\n", __FUNCTION__));
			oi = NULL;
			goto exit;
		}
		break;
#endif	/* BCM4325 */
	default:
		break;
	}

	otp_rgn(oi, cc);

	goto exit;
exit:
	sb_setcoreidx(sbh, idx);

	return (void *)oi;
}

uint16
otpr(void *oh, chipcregs_t *cc, uint wn)
{
	otpinfo_t *oi;

	oi = (otpinfo_t *)oh;

	ASSERT(wn < oi->size);
	ASSERT(cc);

	return R_REG(oi->osh, &cc->otp[wn]);
}

int
otp_read_region(void *oh, int region, uint16 *data, uint wlen)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint idx;
	chipcregs_t *cc;
	uint base, i, sz;

	/* Validate region selection */
	switch (region) {
	case OTP_HW_RGN:
		if (!(oi->prog & OTPS_GUP_HW)) {
			OTP_MSG(("%s: h/w region not programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen < (sz = (uint)oi->hwlim - oi->hwbase)) {
			OTP_MSG(("%s: buffer too small, should be at least %u\n",
			         __FUNCTION__, oi->hwlim - oi->hwbase));
			return -1;
		}
		base = oi->hwbase;
		break;
	case OTP_SW_RGN:
		if (!(oi->prog & OTPS_GUP_SW)) {
			OTP_MSG(("%s: s/w region not programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen < (sz = (uint)oi->swlim - oi->swbase)) {
			OTP_MSG(("%s: buffer too small should be at least %u\n",
			         __FUNCTION__, oi->swlim - oi->swbase));
			return -1;
		}
		base = oi->swbase;
		break;
	case OTP_CI_RGN:
		if (!(oi->prog & OTPS_GUP_CI)) {
			OTP_MSG(("%s: chipid region not programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen < (sz = OTPGU_CI_SZ)) {
			OTP_MSG(("%s: buffer too small, should be at least %u\n",
			         __FUNCTION__, OTPGU_CI_SZ));
			return -1;
		}
		base = OTPGU_CI_OFF;
		break;
	case OTP_FUSE_RGN:
		if (!(oi->prog & OTPS_GUP_FUSE)) {
			OTP_MSG(("%s: fuse region not programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen < (sz = (uint)oi->flim - oi->fbase)) {
			OTP_MSG(("%s: buffer too small, should be at least %u\n",
			         __FUNCTION__, oi->flim - oi->fbase));
			return -1;
		}
		base = oi->fbase;
		break;
	default:
		OTP_MSG(("%s: reading region %d is not supported\n", __FUNCTION__, region));
		return -1;
	}

	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	/* Read the data */
	for (i = 0; i < sz; i ++)
		data[i] = otpr(oh, cc, base + i);

	sb_setcoreidx(oi->sbh, idx);
	return 0;
}

int
otp_status(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return (int)(oi->hwprot | oi->prog);
}

int
otp_size(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return (int)oi->size * 2;
}

int
otp_nvread(void *oh, char *data, uint *len)
{
	return -1;
}

#ifdef BCMNVRAMW
static int
otp_write_bit(otpinfo_t *oi, chipcregs_t *cc, uint idx)
{
	uint k, row, col;
	uint32 otpp, st;

	row = idx / oi->cols;
	col = idx % oi->cols;

	otpp = OTPP_START_BUSY |
	        ((1 << OTPP_VALUE_SHIFT) & OTPP_VALUE_MASK) |
	        ((OTPPOC_BIT_PROG << OTPP_OC_SHIFT) & OTPP_OC_MASK) |
	        ((row << OTPP_ROW_SHIFT) & OTPP_ROW_MASK) |
	        ((col << OTPP_COL_SHIFT) & OTPP_COL_MASK);
	OTP_MSG(("%s: idx = %d, row = %d, col = %d, otpp = 0x%x\n",
	         __FUNCTION__, idx, row, col, otpp));
	W_REG(oi->osh, &cc->otpprog, otpp);

	for (k = 0;
	     ((st = R_REG(oi->osh, &cc->otpprog)) & OTPP_START_BUSY) && (k < OTPP_TRIES);
	     k ++)
		;
	if (k >= OTPP_TRIES) {
		OTP_MSG(("\n%s: BUSY stuck: st=0x%x, count=%d\n", __FUNCTION__, st, k));
		return -1;
	}

	return 0;
}

static int
otpwb16(otpinfo_t *oi, chipcregs_t *cc, int wn, uint16 data)
{
	uint base, i;
	int rc;

	base = wn * 16;
	for (i = 0; i < 16; i++) {
		if (data & (1 << i)) {
			if ((rc = otp_write_bit(oi, cc, base + i)))
				return rc;
		}
	}

	return 0;
}

/* expects the caller to disable interrupts before calling this routine */
int
otp_write_region(void *oh, int region, uint16 *data, uint wlen)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint idx;
	chipcregs_t *cc;
	uint base, i;

	/* Validate region selection */
	switch (region) {
	case OTP_HW_RGN:
		if (oi->prog & OTPS_GUP_HW) {
			OTP_MSG(("%s: h/w region has been programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen > (uint)(oi->hwlim - oi->hwbase)) {
			OTP_MSG(("%s: wlen %u exceeds OTP h/w region limit %u\n",
			         __FUNCTION__, wlen, oi->hwlim - oi->hwbase));
			return -1;
		}
		base = oi->hwbase;
		break;
	case OTP_SW_RGN:
		if (oi->prog & OTPS_GUP_SW) {
			OTP_MSG(("%s: s/w region has been programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen > (uint)(oi->swlim - oi->swbase)) {
			OTP_MSG(("%s: wlen %u exceeds OTP s/w region limit %u\n",
			         __FUNCTION__, wlen, oi->swlim - oi->swbase));
			return -1;
		}
		base = oi->swbase;
		break;
	case OTP_CI_RGN:
		if (oi->prog & OTPS_GUP_CI) {
			OTP_MSG(("%s: chipid region has been programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen > OTPGU_CI_SZ) {
			OTP_MSG(("%s: wlen %u exceeds OTP ci region limit %u\n",
			         __FUNCTION__, wlen, OTPGU_CI_SZ));
			return -1;
		}
		base = OTPGU_CI_OFF;
		break;
	case OTP_FUSE_RGN:
		if (oi->prog & OTPS_GUP_FUSE) {
			OTP_MSG(("%s: fuse region has been programmed\n", __FUNCTION__));
			return -1;
		}
		if (wlen > (uint)(oi->flim - oi->fbase)) {
			OTP_MSG(("%s: wlen %u exceeds OTP ci region limit %u\n",
			         __FUNCTION__, wlen, oi->flim - oi->fbase));
			return -1;
		}
		base = oi->flim - wlen;
		break;
	default:
		OTP_MSG(("%s: writing region %d is not supported\n", __FUNCTION__, region));
		return -1;
	}

	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	/* Enable Write */
	OR_REG(oi->osh, &cc->otpcontrol, OTPC_PROGEN);

	/* Write the data */
	for (i = 0; i < wlen; i ++)
		otpwb16(oh, cc, base + i, data[i]);

	/* Update boundary/flag in memory and in OTP */
	switch (region) {
	case OTP_HW_RGN:
		otpwb16(oh, cc, OTPGU_HSB_OFF, (base + i) * 16);
		otp_write_bit(oh, cc, OTPGU_HWP_OFF);
		break;
	case OTP_SW_RGN:
		otpwb16(oh, cc, OTPGU_HSB_OFF, base * 16);
		otpwb16(oh, cc, OTPGU_SFB_OFF, (base + i) * 16);
		otp_write_bit(oh, cc, OTPGU_SWP_OFF);
		break;
	case OTP_CI_RGN:
		otp_write_bit(oh, cc, OTPGU_CIP_OFF);
		break;
	case OTP_FUSE_RGN:
		otpwb16(oh, cc, OTPGU_SFB_OFF, base * 16);
		otp_write_bit(oh, cc, OTPGU_FUSEP_OFF);
		break;
	}

	/* Disable Write */
	AND_REG(oi->osh, &cc->otpcontrol, ~OTPC_PROGEN);

	/* Sync region info by retrieving them again */
	otp_rgn(oi, cc);

	sb_setcoreidx(oi->sbh, idx);
	return 0;
}

/* expects the caller to disable interrupts before calling this routine */
int
otp_nvwrite(void *oh, uint16 *data, uint wlen)
{
	return -1;
}
#endif /* BCMNVRAMW */

#if defined(WLTEST)
static int
otp_read_bit(otpinfo_t *oi, chipcregs_t *cc, uint idx)
{
	uint k, row, col;
	uint32 otpp, st;

	row = idx / oi->cols;
	col = idx % oi->cols;

	otpp = OTPP_START_BUSY |
	        ((OTPPOC_READ << OTPP_OC_SHIFT) & OTPP_OC_MASK) |
	        ((row << OTPP_ROW_SHIFT) & OTPP_ROW_MASK) |
	        ((col << OTPP_COL_SHIFT) & OTPP_COL_MASK);
	OTP_MSG(("%s: idx = %d, row = %d, col = %d, otpp = 0x%x",
	         __FUNCTION__, idx, row, col, otpp));
	W_REG(oi->osh, &cc->otpprog, otpp);

	for (k = 0;
	     ((st = R_REG(oi->osh, &cc->otpprog)) & OTPP_START_BUSY) && (k < OTPP_TRIES);
	     k ++)
		;
	if (k >= OTPP_TRIES) {
		OTP_MSG(("\n%s: BUSY stuck: st=0x%x, count=%d\n", __FUNCTION__, st, k));
		return -1;
	}
	if (st & OTPP_READERR) {
		OTP_MSG(("\n%s: Could not read OTP bit %d\n", __FUNCTION__, idx));
		return -1;
	}
	st = (st & OTPP_VALUE_MASK) >> OTPP_VALUE_SHIFT;

	OTP_MSG((" => %d\n", st));
	return (int)st;
}

static uint16
otprb16(otpinfo_t *oi, chipcregs_t *cc, uint wn)
{
	uint base, i;
	uint16 val;
	int bit;

	base = wn * 16;

	val = 0;
	for (i = 0; i < 16; i++) {
		if ((bit = otp_read_bit(oi, cc, base + i)) == -1)
			break;
		val = val | (bit << i);
	}
	if (i < 16)
		val = 0xffff;

	return val;
}

int
otp_dump(void *oh, int arg, char *buf, uint size)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	chipcregs_t *cc;
	uint idx, i, count;
	uint16 val;
	struct bcmstrbuf b;

	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	count = otp_size(oh);

	bcm_binit(&b, buf, size);
	for (i = 0; i < count / 2; i++) {
		if (!(i % 4))
			bcm_bprintf(&b, "\n0x%04x:", 2 * i);
		if (arg == 0)
			val = otpr(oh, cc, i);
		else
			val = otprb16(oh, cc, i);
		bcm_bprintf(&b, " 0x%04x", val);
	}
	bcm_bprintf(&b, "\n");

	sb_setcoreidx(oi->sbh, idx);

	return ((int)(b.buf - b.origbuf));
}
#endif	

#else	/* BCMHNDOTP - Older HND OTP controller */

/* Fields in otpstatus */
#define	OTPS_PROGFAIL		0x80000000
#define	OTPS_PROTECT		0x00000007
#define	OTPS_HW_PROTECT		0x00000001
#define	OTPS_SW_PROTECT		0x00000002
#define	OTPS_CID_PROTECT	0x00000004

/* Fields in the otpcontrol register */
#define	OTPC_RECWAIT		0xff000000
#define	OTPC_PROGWAIT		0x00ffff00
#define	OTPC_PRW_SHIFT		8
#define	OTPC_MAXFAIL		0x00000038
#define	OTPC_VSEL		0x00000006
#define	OTPC_SELVL		0x00000001

/* Fields in otpprog */
#define	OTPP_COL_MASK		0x000000ff
#define	OTPP_ROW_MASK		0x0000ff00
#define	OTPP_ROW_SHIFT		8
#define	OTPP_READERR		0x10000000
#define	OTPP_VALUE		0x20000000
#define	OTPP_VALUE_SHIFT		29
#define	OTPP_READ		0x40000000
#define	OTPP_START		0x80000000
#define	OTPP_BUSY		0x80000000

/* OTP regions (Byte offsets from otp size) */
#define	OTP_SWLIM_OFF	(-8)
#define	OTP_CIDBASE_OFF	0
#define	OTP_CIDLIM_OFF	8

/* Predefined OTP words (Word offset from otp size) */
#define	OTP_BOUNDARY_OFF (-4)
#define	OTP_HWSIGN_OFF	(-3)
#define	OTP_SWSIGN_OFF	(-2)
#define	OTP_CIDSIGN_OFF	(-1)
#define	OTP_CID_OFF	0
#define	OTP_PKG_OFF	1
#define	OTP_FID_OFF	2
#define	OTP_RSV_OFF	3
#define	OTP_LIM_OFF	4

#define	OTP_HW_REGION	OTPS_HW_PROTECT
#define	OTP_SW_REGION	OTPS_SW_PROTECT
#define	OTP_CID_REGION	OTPS_CID_PROTECT

#if OTP_HW_REGION != OTP_HW_RGN
#error "incompatible OTP_HW_RGN"
#endif
#if OTP_SW_REGION != OTP_SW_RGN
#error "incompatible OTP_SW_RGN"
#endif
#if OTP_CID_REGION != OTP_CI_RGN
#error "incompatible OTP_CI_RGN"
#endif

#define	OTP_SIGNATURE	0x578a
#define	OTP_MAGIC	0x4e56

#define OTPP_TRIES	10000000	/* # of tries for OTPP */

typedef struct _otpinfo {
	sb_t	*sbh;		/* Saved sb handle */
	uint	ccrev;		/* chipc revision */
	uint	size;		/* Size of otp in bytes */
	uint	hwprot;		/* Hardware protection bits */
	uint	signvalid;	/* Signature valid bits */
	int	boundary;	/* hw/sw boundary */
} otpinfo_t;

static otpinfo_t otpinfo;

static uint16 otproff(void *oh, chipcregs_t *cc, int woff);
#ifdef BCMNVRAMW
static int otp_write_word(void *oh, chipcregs_t *cc, int wn, uint16 data);
#endif /* BCMNVRAMW */

uint16
otpr(void *oh, chipcregs_t *cc, uint wn)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	osl_t *osh;
	uint16 *ptr;

	ASSERT(wn < ((((otpinfo_t *)oh)->size / 2) + OTP_LIM_OFF));
	ASSERT(cc);

	osh = sb_osh(oi->sbh);

	ptr = (uint16 *)((uchar *)cc + CC_OTP);
	return (R_REG(osh, &ptr[wn]));
}

static uint16
otproff(void *oh, chipcregs_t *cc, int woff)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	osl_t *osh;
	uint16 *ptr;

	ASSERT(woff >= (-((int)oi->size / 2)));
	ASSERT(woff < OTP_LIM_OFF);
	ASSERT(cc);

	osh = sb_osh(oi->sbh);

	ptr = (uint16 *)((uchar *)cc + CC_OTP);

	return (R_REG(osh, &ptr[(oi->size / 2) + woff]));
}

void *
otp_init(sb_t *sbh)
{
	uint idx;
	chipcregs_t *cc;
	otpinfo_t *oi;
	uint32 cap = 0;
	void *ret = NULL;
	osl_t *osh;

	oi = &otpinfo;
	bzero(oi, sizeof(otpinfo_t));

	idx = sb_coreidx(sbh);

	oi->sbh = sbh;
	osh = sb_osh(oi->sbh);

	/* Check for otp */
	if ((cc = sb_setcore(sbh, SB_CC, 0)) != NULL) {
		cap = R_REG(osh, &cc->capabilities);
		if ((cap & CC_CAP_OTPSIZE) == 0) {
			/* Nothing there */
			goto out;
		}

		oi->sbh = sbh;
		oi->ccrev = sb_chipcrev(sbh);

		/* As of right now, support only 4320a2 and 4311a1 */
		if ((oi->ccrev != 12) && (oi->ccrev != 17)) {
			goto out;
		}

		oi->size = 1 << (((cap & CC_CAP_OTPSIZE) >> CC_CAP_OTPSIZE_SHIFT)
			+ CC_CAP_OTPSIZE_BASE);

		oi->hwprot = (int)(R_REG(osh, &cc->otpstatus) & OTPS_PROTECT);
		oi->boundary = -1;

		if (oi->ccrev != 17) {
			if (otproff(oi, cc, OTP_HWSIGN_OFF) == OTP_SIGNATURE) {
				oi->signvalid |= OTP_HW_REGION;
				oi->boundary = otproff(oi, cc, OTP_BOUNDARY_OFF);
			}

			if (otproff(oi, cc, OTP_SWSIGN_OFF) == OTP_SIGNATURE)
				oi->signvalid |= OTP_SW_REGION;

			if (otproff(oi, cc, OTP_CIDSIGN_OFF) == OTP_SIGNATURE)
				oi->signvalid |= OTP_CID_REGION;
		}

		ret = (void *)oi;
	}

out:	/* All done */
	sb_setcoreidx(sbh, idx);

	return ret;
}

int
otp_read_region(void *oh, int region, uint16 *data, uint wlen)
{
	return -1;
}

int
otp_status(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return ((int)(oi->hwprot | oi->signvalid));
}

int
otp_size(void *oh)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	return ((int)(oi->size));
}

int
otp_nvread(void *oh, char *data, uint *len)
{
	int rc = 0;
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint32 base, bound, lim = 0, st;
	int i, chunk, gchunks, tsz = 0;
	uint32 idx;
	chipcregs_t *cc;
	uint offset;
	uint16 *rawotp = NULL;

	/* save the orig core */
	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	st = otp_status(oh);
	if (!(st & (OTP_HW_REGION | OTP_SW_REGION))) {
		OTP_MSG(("OTP not programmed\n"));
		rc = -1;
		goto out;
	}

	/* Read the whole otp so we can easily manipulate it */
	lim = otp_size(oh);
	if ((rawotp = MALLOC(sb_osh(oi->sbh), lim)) == NULL) {
		OTP_MSG(("Out of memory for rawotp\n"));
		rc = -2;
		goto out;
	}
	for (i = 0; i < (lim / 2); i++)
		rawotp[i] = otpr(oh, cc,  i);

	if ((st & OTP_HW_REGION) == 0) {
		OTP_MSG(("otp: hw region not written (0x%x)\n", st));

		/* This could be a programming failure in the first
		 * chunk followed by one or more good chunks
		 */
		for (i = 0; i < (lim / 2); i++)
			if (rawotp[i] == OTP_MAGIC)
				break;

		if (i < (lim / 2)) {
			base = i;
			bound = (i * 2) + rawotp[i + 1];
			OTP_MSG(("otp: trying chunk at 0x%x-0x%x\n", i * 2, bound));
		} else {
			OTP_MSG(("otp: unprogrammed\n"));
			rc = -3;
			goto out;
		}
	} else {
		bound = rawotp[(lim / 2) + OTP_BOUNDARY_OFF];

		/* There are two cases: 1) The whole otp is used as nvram
		 * and 2) There is a hardware header followed by nvram.
		 */
		if (rawotp[0] == OTP_MAGIC) {
			base = 0;
			if (bound != rawotp[1])
				OTP_MSG(("otp: Bound 0x%x != chunk0 len 0x%x\n", bound,
				         rawotp[1]));
		} else
			base = bound;
	}

	/* Find and copy the data */

	chunk = 0;
	gchunks = 0;
	i = base / 2;
	offset = 0;
	while ((i < (lim / 2)) && (rawotp[i] == OTP_MAGIC)) {
		int dsz, rsz = rawotp[i + 1];

		if (((i * 2) + rsz) >= lim) {
			OTP_MSG(("  bad chunk size, chunk %d, base 0x%x, size 0x%x\n",
			         chunk, i * 2, rsz));
			/* Bad length, try to find another chunk anyway */
			rsz = 6;
		}
		if (hndcrc16((uint8 *)&rawotp[i], rsz,
		             CRC16_INIT_VALUE) == CRC16_GOOD_VALUE) {
			/* Good crc, copy the vars */
			OTP_MSG(("  good chunk %d, base 0x%x, size 0x%x\n",
			         chunk, i * 2, rsz));
			gchunks++;
			dsz = rsz - 6;
			tsz += dsz;
			if (offset + dsz >= *len) {
				OTP_MSG(("Out of memory for otp\n"));
				goto out;
			}
			bcopy((char *)&rawotp[i + 2], &data[offset], dsz);
			offset += dsz;
			/* Remove extra null characters at the end */
			while (offset > 1 &&
			       data[offset - 1] == 0 && data[offset - 2] == 0)
				offset --;
			i += rsz / 2;
		} else {
			/* bad length or crc didn't check, try to find the next set */
			OTP_MSG(("  chunk %d @ 0x%x size 0x%x: bad crc, ",
			         chunk, i * 2, rsz));
			if (rawotp[i + (rsz / 2)] == OTP_MAGIC) {
				/* Assume length is good */
				i += rsz / 2;
			} else {
				while (++i < (lim / 2))
					if (rawotp[i] == OTP_MAGIC)
						break;
			}
			if (i < (lim / 2))
				OTP_MSG(("trying next base 0x%x\n", i * 2));
			else
				OTP_MSG(("no more chunks\n"));
		}
		chunk++;
	}

	OTP_MSG(("  otp size = %d, boundary = 0x%x, nv base = 0x%x\n",
	         lim, bound, base));
	if (tsz != 0)
		OTP_MSG(("  Found %d bytes in %d good chunks out of %d\n",
		         tsz, gchunks, chunk));
	else
		OTP_MSG(("  No good chunks found out of %d\n", chunk));

	*len = offset;

out:
	if (rawotp)
		MFREE(sb_osh(oi->sbh), rawotp, lim);
	sb_setcoreidx(oi->sbh, idx);

	return rc;
}

#ifdef BCMNVRAMW

static int
otp_write_word(void *oh, chipcregs_t *cc, int wn, uint16 data)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint base, row, col, bit, i, j, k;
	uint32 pwait, init_pwait, otpc, otpp, pst, st;

#ifdef	OTP_FORCEFAIL
	OTP_MSG(("%s: [0x%x] = 0x%x\n", __FUNCTION__, wn * 2, data));
#endif /* OTP_FORCEFAIL */

	/* This is bit-at-a-time writing, future cores may do word-at-a-time */
	base = (wn * 16) + (wn / 4);
	if (oi->ccrev == 12) {
		otpc = 0x20000001;
		init_pwait = 0x00000200;
	} else {
		otpc = 0x20000000;
		init_pwait = 0x00004000;
	}
	for (i = 0; i < 16; i++) {
		pwait = init_pwait;
		bit = data & 1;
		row = (base + i) / 65;
		col = (base + i) % 65;
		otpp = OTPP_START |
			((bit << OTPP_VALUE_SHIFT) & OTPP_VALUE) |
			((row << OTPP_ROW_SHIFT) & OTPP_ROW_MASK) |
			(col & OTPP_COL_MASK);
		OTP_MSG(("row %d, col %d, val %d, otpc 0x%x, otpp 0x%x\n", row, col, bit,
		         otpc, otpp));
		j = 0;
		while (1) {
			j++;
			OTP_MSG(("  %d: pwait %d\n", j, (pwait >> 8)));
			W_REG(osh, &cc->otpcontrol, otpc | pwait);
			W_REG(osh, &cc->otpprog, otpp);
			pst = R_REG(osh, &cc->otpprog);
			for (k = 0; ((pst & OTPP_BUSY) == OTPP_BUSY) && (k < OTPP_TRIES); k++)
				pst = R_REG(osh, &cc->otpprog);
			if (k >= OTPP_TRIES) {
				OTP_MSG(("BUSY stuck: pst=0x%x, count=%d\n", pst, k));
				st = OTPS_PROGFAIL;
				break;
			}
			st = R_REG(osh, &cc->otpstatus);
			if (((st & OTPS_PROGFAIL) == 0) || (pwait == OTPC_PROGWAIT)) {
				break;
			} else {
				if ((oi->ccrev == 12) && (pwait >= 0x1000))
					pwait = (pwait << 3) & OTPC_PROGWAIT;
				else
					pwait = (pwait << 1) & OTPC_PROGWAIT;
				if (pwait == 0)
					pwait = OTPC_PROGWAIT;
			}
		}
		if (st & OTPS_PROGFAIL) {
			OTP_MSG(("After %d tries: otpc = 0x%x, otpp = 0x%x/0x%x, otps = 0x%x\n",
			       j, otpc | pwait, otpp, pst, st));
			OTP_MSG(("otp prog failed. wn=%d, bit=%d, ppret=%d, ret=%d\n",
			       wn, i, k, j));
			return 1;
		}
		data >>= 1;
	}
	return 0;
}

/* expects the caller to disable interrupts before calling this routine */
int
otp_write_region(void *oh, int region, uint16 *data, uint wlen)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint32 st;
	uint wn, base = 0, lim;
	int ret;
	uint idx;
	chipcregs_t *cc;

	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	/* Run bist on chipc to get any unprogrammed bits into a known state */
	if (sb_corebist(oi->sbh) == 0)
		OTP_MSG(("%s: bist passed, otp is blank\n", __FUNCTION__));

	if (oi->ccrev != 17) {
		/* Check valid region */
		if ((region != OTP_HW_REGION) &&
		    (region != OTP_SW_REGION) &&
		    (region != OTP_CID_REGION)) {
			ret = -2;
			goto out;
		}

		/* Region already written? */
		st = oi->hwprot | oi-> signvalid;
		if ((st & region) != 0) {
			ret = -3;
			goto out;
		}

		/* HW and CID have to be written before SW */
		if ((st & OTP_SW_REGION) != 0) {
			ret = -4;
			goto out;
		}

		/* Bounds for the region */
		lim = (oi->size / 2) + OTP_SWLIM_OFF;
		if (region == OTP_HW_REGION) {
			base = 0;
		} else if (region == OTP_SW_REGION) {
			base = oi->boundary / 2;
		} else if (region == OTP_CID_REGION) {
			base = (oi->size / 2) + OTP_CID_OFF;
			lim = (oi->size / 2) + OTP_LIM_OFF;
		}
	} else {
		base = 0;
		lim = oi->size / 4;
	}
	if (wlen > (lim - base)) {
		ret = -5;
		goto out;
	}
	lim = base + wlen;


	/* Write the data */
	ret = -7;
	for (wn = base; wn < lim; wn++)
		if (oi->ccrev == 17) {
			uint werrs, rwn;

			rwn = 4 * wn;
			werrs = (otp_write_word(oh, cc, rwn++, *data) != 0) ? 1 : 0;
			werrs += (otp_write_word(oh, cc, rwn++, *data) != 0) ? 1 : 0;
			werrs += (otp_write_word(oh, cc, rwn, *data++) != 0) ? 1 : 0;
			if (werrs > 2)
				goto out;
		} else
			if (otp_write_word(oh, cc, wn, *data++) != 0)
				goto out;

	if (oi->ccrev != 17) {
		/* Done with the data, write the signature & boundary if needed */
		if (region == OTP_HW_REGION) {
			ret = -8;
			if (otp_write_word(oh, cc, (oi->size / 2) + OTP_BOUNDARY_OFF,
			                   lim * 2) != 0)
				goto out;
			ret = -9;
			if (otp_write_word(oh, cc, (oi->size / 2) + OTP_HWSIGN_OFF,
			                   OTP_SIGNATURE) != 0)
				goto out;
			oi->boundary = lim * 2;
			oi->signvalid |= OTP_HW_REGION;
		} else if (region == OTP_SW_REGION) {
			ret = -10;
			if (otp_write_word(oh, cc, (oi->size / 2) + OTP_SWSIGN_OFF,
			                   OTP_SIGNATURE) != 0)
				goto out;
			oi->signvalid |= OTP_SW_REGION;
		} else if (region == OTP_CID_REGION) {
			ret = -11;
			if (otp_write_word(oh, cc, (oi->size / 2) + OTP_CIDSIGN_OFF,
			                   OTP_SIGNATURE) != 0)
				goto out;
			oi->signvalid |= OTP_CID_REGION;
		}
	}
	ret = 0;
out:
	OTP_MSG(("bits written: %d, average (%d/%d): %d, max retry: %d, pp max: %d\n",
		st_n, st_s, st_n, st_n?(st_s / st_n):0, st_hwm, pp_hwm));

	sb_setcoreidx(oi->sbh, idx);

	return ret;
}

/* expects the caller to disable interrupts before calling this routine */
int
otp_nvwrite(void *oh, uint16 *data, uint wlen)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	uint32 st;
	uint16 crc, clen, *p, hdr[2];
	uint wn, base = 0, lim;
	int err, gerr = 0;
	uint idx;
	chipcregs_t *cc;


	/* Run bist on chipc to get any unprogrammed bits into a known state */
	if (sb_corebist(oi->sbh) == 0)
		OTP_MSG(("%s: bist passed, otp is blank\n", __FUNCTION__));

	/* otp already written? */
	st = oi->hwprot | oi-> signvalid;
	if ((st & (OTP_HW_REGION | OTP_SW_REGION)) == (OTP_HW_REGION | OTP_SW_REGION))
		return BCME_EPERM;

	/* save the orig core */
	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	/* Bounds for the region */
	lim = (oi->size / 2) + OTP_SWLIM_OFF;
	base = 0;

	/* Look for possible chunks from the end down */
	wn = lim;
	while (wn > 0) {
		wn--;
		if (otpr(oh, cc, wn) == OTP_MAGIC) {
			base = wn + (otpr(oh, cc, wn + 1) / 2);
			break;
		}
	}
	if (base == 0) {
		OTP_MSG(("Unprogrammed otp\n"));
	} else {
		OTP_MSG(("Found some chunks, skipping to 0x%x\n", base * 2));
	}
	if ((wlen + 3) > (lim - base)) {
		err =  BCME_NORESOURCE;
		goto out;
	}


	/* Prepare the header and crc */
	hdr[0] = OTP_MAGIC;
	hdr[1] = (wlen + 3) * 2;
	crc = hndcrc16((uint8 *)hdr, sizeof(hdr), CRC16_INIT_VALUE);
	crc = hndcrc16((uint8 *)data, wlen * 2, crc);
	crc = ~crc;

	do {
		p = data;
		wn = base + 2;
		lim = base + wlen + 2;

		OTP_MSG(("writing chunk, 0x%x bytes @ 0x%x-0x%x\n", wlen * 2,
		         base * 2, (lim + 1) * 2));

		/* Write the header */
		err = otp_write_word(oh, cc, base, hdr[0]);

		/* Write the data */
		while (wn < lim) {
			err += otp_write_word(oh, cc, wn++, *p++);

			/* If there has been an error, close this chunk */
			if (err != 0) {
				OTP_MSG(("closing early @ 0x%x\n", wn * 2));
				break;
			}
		}

		/* If we wrote the whole chunk, write the crc */
		if (wn == lim) {
			OTP_MSG(("  whole chunk written, crc = 0x%x\n", crc));
			err += otp_write_word(oh, cc, wn++, crc);
			clen = hdr[1];
		} else {
			/* If there was an error adjust the count to point to
			 * the word after the error so we can start the next
			 * chunk there.
			 */
			clen = (wn - base) * 2;
			OTP_MSG(("  partial chunk written, chunk len = 0x%x\n", clen));
		}
		/* And now write the chunk length */
		err += otp_write_word(oh, cc, base + 1, clen);

		if (base == 0) {
			/* Write the signature and boundary if this is the HW region,
			 * but don't report failure if either of these 2 writes fail.
			 */
			if (otp_write_word(oh, cc, (oi->size / 2) + OTP_BOUNDARY_OFF, wn * 2) == 0)
				gerr += otp_write_word(oh, cc, (oi->size / 2) + OTP_HWSIGN_OFF,
				                       OTP_SIGNATURE);
			else
				gerr++;
			oi->boundary = wn * 2;
			oi->signvalid |= OTP_HW_REGION;
		}

		if (err != 0) {
			gerr += err;
			/* Errors, do it all over again if there is space left */
			if ((wlen + 3) <= ((oi->size / 2) + OTP_SWLIM_OFF - wn)) {
				base = wn;
				lim = base + wlen + 2;
				OTP_MSG(("Programming errors, retry @ 0x%x\n", wn * 2));
			} else {
				OTP_MSG(("Programming errors, no space left ( 0x%x)\n", wn * 2));
				break;
			}
		}
	} while (err != 0);

	OTP_MSG(("bits written: %d, average (%d/%d): %d, max retry: %d, pp max: %d\n",
	       st_n, st_s, st_n, st_s / st_n, st_hwm, pp_hwm));

	if (gerr != 0)
		OTP_MSG(("programming %s after %d errors\n", (err == 0) ? "succedded" : "failed",
		         gerr));
out:
	/* done */
	sb_setcoreidx(oi->sbh, idx);

	if (err)
		return BCME_ERROR;
	else
		return 0;
}
#endif /* BCMNVRAMW */

#if	defined(WLTEST)
static uint16
otp_read_bit(void *oh, chipcregs_t *cc, uint idx)
{
	uint k, row, col;
	uint32 otpp, st;
	osl_t *osh;
	otpinfo_t *oi = (otpinfo_t *)oh;

	osh = sb_osh(oi->sbh);
	row = idx / 65;
	col = idx % 65;

	otpp = OTPP_START | OTPP_READ |
	        ((row << OTPP_ROW_SHIFT) & OTPP_ROW_MASK) |
	        (col & OTPP_COL_MASK);

	OTP_MSG(("%s: idx = %d, row = %d, col = %d, otpp = 0x%x", __FUNCTION__,
	         idx, row, col, otpp));

	W_REG(osh, &cc->otpprog, otpp);
	st = R_REG(osh, &cc->otpprog);
	for (k = 0; ((st & OTPP_BUSY) == OTPP_BUSY) && (k < OTPP_TRIES); k++)
		st = R_REG(osh, &cc->otpprog);

	if (k >= OTPP_TRIES) {
		OTP_MSG(("\n%s: BUSY stuck: st=0x%x, count=%d\n", __FUNCTION__, st, k));
		return 0xffff;
	}
	if (st & OTPP_READERR) {
		OTP_MSG(("\n%s: Could not read OTP bit %d\n", __FUNCTION__, idx));
		return 0xffff;
	}
	st = (st & OTPP_VALUE) >> OTPP_VALUE_SHIFT;
	OTP_MSG((" => %d\n", st));
	return (uint16)st;
}

static uint16
otprb16(void *oh, chipcregs_t *cc, uint wn)
{
	uint base, i;
	uint16 val, bit;

	base = (wn * 16) + (wn / 4);
	val = 0;
	for (i = 0; i < 16; i++) {
		if ((bit = otp_read_bit(oh, cc, base + i)) == 0xffff)
			break;
		val = val | (bit << i);
	}
	if (i < 16)
		val = 0xaaaa;
	return val;
}

int
otp_dump(void *oh, int arg, char *buf, uint size)
{
	otpinfo_t *oi = (otpinfo_t *)oh;
	chipcregs_t *cc;
	uint idx, i, count, lil;
	uint16 val;
	struct bcmstrbuf b;

	idx = sb_coreidx(oi->sbh);
	cc = sb_setcore(oi->sbh, SB_CC, 0);

	if (arg >= 16) {
		arg -= 16;
	} else {
		/* Run bist on chipc to get any unprogrammed bits into a known state */
		if (sb_corebist(oi->sbh) == 0)
			OTP_MSG(("%s: bist passed, otp is blank\n", __FUNCTION__));
	}

	if (arg == 2) {
		count = 66 * 4;
		lil = 3;
	} else {
		count = (oi->size / 2) + OTP_LIM_OFF;
		lil = 7;
	}

	OTP_MSG(("%s: arg %d, size %d, words %d\n", __FUNCTION__, arg, size, count));
	bcm_binit(&b, buf, size);
	for (i = 0; i < count; i++) {
		if ((i & lil) == 0)
			bcm_bprintf(&b, "0x%04x:", 2 * i);

		if (arg == 0)
			val = otpr(oh, cc, i);
		else
			val = otprb16(oh, cc, i);
		bcm_bprintf(&b, " 0x%04x", val);
		if ((i & lil) == lil) {
			if (arg == 2) {
				bcm_bprintf(&b, " %d\n",
				              otp_read_bit(oh, cc, ((i / 4) * 65) + 64) & 1);
			} else {
				bcm_bprintf(&b, "\n");
			}
		}
	}
	if ((i & lil) != lil)
		bcm_bprintf(&b, "\n");

	OTP_MSG(("%s: returning %d, left %d, wn %d\n",
		__FUNCTION__, (int)(b.buf - b.origbuf), b.size, i));

	sb_setcoreidx(oi->sbh, idx);

	return ((int)(b.buf - b.origbuf));
}
#endif	

#endif	/* BCMHNDOTP */
