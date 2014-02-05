/* OV511 Decompression Support Module
 *
 * Copyright (c) 1999-2003 Mark W. McClelland. All rights reserved.
 * http://alpha.dyndns.org/ov511/
 *
 * Original decompression code Copyright 1998-2000 OmniVision Technologies
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; version 2 of the License.
 */

#include <linux/config.h>

#if defined(OUTSIDE_KERNEL)
	#if defined(CONFIG_MODVERSIONS) && !defined(MODVERSIONS)
		#define MODVERSIONS
	#endif

	#include <linux/version.h>

	#ifdef MODVERSIONS
		#include <linux/modversions.h>
	#endif
#else
	#include <linux/version.h>
#endif

#include <linux/module.h>
#include <linux/init.h>

#include "ov51x.h"

/******************************************************************************
 * Version Information
 ******************************************************************************/

#define DRIVER_VERSION "v1.6"
#define DRIVER_AUTHOR "Mark McClelland <mark@alpha.dyndns.org>, OmniVision \
Technologies <http://www.ovt.com/>"
#define DRIVER_DESC "OV511 Decompression Module"

/******************************************************************************
 * Prototypes
 ******************************************************************************/

extern int ov511_register_decomp_module(int ver, struct ov51x_decomp_ops *ops,
					int ov518, int mmx);
extern void ov511_deregister_decomp_module(int ov518, int mmx);

/******************************************************************************
 * Decompression Module Interface Constants
 ******************************************************************************/

static const int interface_ver = DECOMP_INTERFACE_VER;
static const int ov518 = 0;
static const int mmx = 0;

/******************************************************************************
 * Module Features
 ******************************************************************************/

static int debug;

MODULE_PARM(debug, "i");
MODULE_PARM_DESC(debug,
  "Debug level: 0=none, 1=inits, 2=warning, 3=config, 4=functions, 5=max");

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
#if defined(MODULE_LICENSE)	/* Introduced in ~2.4.10 */
MODULE_LICENSE("GPL");
#endif

/******************************************************************************
 * Decompression Functions
 ******************************************************************************/

static void
DecompressYHI(unsigned char *pIn,
	      unsigned char *pOut,
	      int           *iIn,	/* in/out */
	      int           *iOut,	/* in/out */
	      const int      w,
	      const int      YUVFlag)
{
	short ZigZag[64];
	int temp[64];
	int Zcnt_Flag = 0;
	int Num8_Flag = 0;
	int in_pos = *iIn;
	int out_pos = *iOut;
	int tmp, tmp1, tmp2, tmp3;
	unsigned char header, ZTable[64];
	short tmpl, tmph, half_byte, idx, count;
	unsigned long ZigZag_length = 0, ZT_length, i, j;
	short DeZigZag[64];

	const short a = 11584;
	const short b = 16068;
	const short c = 15136;
	const short d = 13624;
	const short e =  9104;
	const short f =  6270;
	const short g =  3196;

	int out_idx;

	/* Take off every 'Zig' */
	for (i = 0; i < 64; i++) {
		ZigZag[i] = 0;
	}

	/*****************************
	 * Read in the Y header byte *
	 *****************************/

	header = pIn[in_pos];
	in_pos++;

	ZigZag_length = header & 0x3f;
	ZigZag_length = ZigZag_length + 1;

	Num8_Flag = header & 0x40;
	Zcnt_Flag = header & 0x80;

	/*************************
	 * Read in the Y content *
	 *************************/

	if (Zcnt_Flag == 0) {    /* Without Zero Table read contents directly */
		/* Read in ZigZag[0] */
		ZigZag[0] = pIn[in_pos++];
		tmpl = pIn[in_pos++];
		tmph = tmpl<<8;
		ZigZag[0] = ZigZag[0] | tmph;
		ZigZag[0] = ZigZag[0]<<4;
		ZigZag[0] = ZigZag[0]>>4;

		if (Num8_Flag) { /* 8 Bits */
			for (i = 1; i < ZigZag_length; i++) {
				ZigZag[i] = pIn[in_pos++];
				ZigZag[i] = ZigZag[i]<<8;
				ZigZag[i] = ZigZag[i]>>8;
			}
		} else {   /* 12 bits and has no Zero Table */
			idx = 1;
			half_byte = 0;
			for (i = 1; i < ZigZag_length; i++) {
				if (half_byte == 0) {
					ZigZag[i] = pIn[in_pos++];
					tmpl = pIn[in_pos++];
					tmph = tmpl<<8;
					tmph = tmph&0x0f00;
					ZigZag[i] = ZigZag[i] | tmph;
					ZigZag[i] = ZigZag[i]<<4;
					ZigZag[i] = ZigZag[i]>>4;
					half_byte = 1;
				} else {
					ZigZag[i] = pIn[in_pos++];
					ZigZag[i] = ZigZag[i]<<8;
					tmpl = tmpl & 0x00f0;
					ZigZag[i] = ZigZag[i] | tmpl;
					ZigZag[i] = ZigZag[i]>>4;
					half_byte = 0;
				}
			}
		}
	} else {  /* Has Zero Table */
		/* Calculate Z-Table length */
		ZT_length = ZigZag_length/8;
		tmp = ZigZag_length%8;

		if (tmp > 0) {
			ZT_length = ZT_length + 1;
		}

		/* Read in Zero Table */
		for (j = 0; j < ZT_length; j++) {
			ZTable[j] = pIn[in_pos++];
		}

		/* Read in ZigZag[0] */
		ZigZag[0] = pIn[in_pos++];
		tmpl = pIn[in_pos++];
		tmph = tmpl<<8;
		ZigZag[0] = ZigZag[0] | tmph;
		ZigZag[0] = ZigZag[0]<<4;
		ZigZag[0] = ZigZag[0]>>4;

		/* Decode ZigZag */
		idx = 0;
		ZTable[idx] = ZTable[idx]<<1;
		count = 7;

		if (Num8_Flag) {	/* 8 Bits and has zero table */
			for (i = 1; i < ZigZag_length; i++) {
				if ((ZTable[idx]&0x80)) {
					ZigZag[i] = pIn[in_pos++];
					ZigZag[i] = ZigZag[i]<<8;
					ZigZag[i] = ZigZag[i]>>8;
				}

				ZTable[idx]=ZTable[idx]<<1;
				count--;
				if (count == 0)	{
					count = 8;
					idx++;
				}
			}
		} else {	/* 12 bits and has Zero Table */
			half_byte = 0;
			for (i = 1; i < ZigZag_length; i++) {
				if (ZTable[idx]&0x80) {
					if (half_byte == 0) {
						ZigZag[i] = pIn[in_pos++];
						tmpl = pIn[in_pos++];
						tmph = tmpl <<8;
						tmph = tmph & 0x0f00;
						ZigZag[i] = ZigZag[i] | tmph;
						ZigZag[i] = ZigZag[i]<<4;
						ZigZag[i] = ZigZag[i]>>4;
						half_byte = 1;
					} else {
						ZigZag[i] = pIn[in_pos++];
						ZigZag[i] = ZigZag[i]<<8;
						tmpl = tmpl & 0x00f0;
						ZigZag[i] = ZigZag[i] | tmpl;
						ZigZag[i] = ZigZag[i]>>4;
						half_byte = 0;
					}
				}

				ZTable[idx] = ZTable[idx]<<1;
				count--;
				if (count == 0)	{
					count = 8;
					idx++;
				}
			}
		}
	}

	/*************
	 * De-ZigZag *
	 *************/

	for (j = 0; j < 64; j++) {
		DeZigZag[j] = 0;
	}

	if (YUVFlag == 1) {
		DeZigZag[0] = ZigZag[0];
		DeZigZag[1] = ZigZag[1]<<1;
		DeZigZag[2] = ZigZag[5]<<1;
		DeZigZag[3] = ZigZag[6]<<2;

		DeZigZag[8] = ZigZag[2]<<1;
		DeZigZag[9] = ZigZag[4]<<1;
		DeZigZag[10] = ZigZag[7]<<1;
		DeZigZag[11] = ZigZag[13]<<2;

		DeZigZag[16] = ZigZag[3]<<1;
		DeZigZag[17] = ZigZag[8]<<1;
		DeZigZag[18] = ZigZag[12]<<2;
		DeZigZag[19] = ZigZag[17]<<2;

		DeZigZag[24] = ZigZag[9]<<2;
		DeZigZag[25] = ZigZag[11]<<2;
		DeZigZag[26] = ZigZag[18]<<2;
		DeZigZag[27] = ZigZag[24]<<3;
	} else {
		DeZigZag[0] = ZigZag[0];
		DeZigZag[1] = ZigZag[1]<<2;
		DeZigZag[2] = ZigZag[5]<<2;
		DeZigZag[3] = ZigZag[6]<<3;

		DeZigZag[8] = ZigZag[2]<<2;
		DeZigZag[9] = ZigZag[4]<<2;
		DeZigZag[10] = ZigZag[7]<<2;
		DeZigZag[11] = ZigZag[13]<<4;

		DeZigZag[16] = ZigZag[3]<<2;
		DeZigZag[17] = ZigZag[8]<<2;
		DeZigZag[18] = ZigZag[12]<<3;
		DeZigZag[19] = ZigZag[17]<<4;

		DeZigZag[24] = ZigZag[9]<<3;
		DeZigZag[25] = ZigZag[11]<<4;
		DeZigZag[26] = ZigZag[18]<<4;
		DeZigZag[27] = ZigZag[24]<<4;
	}

	/*****************
	 **** IDCT 1D ****
	 *****************/

#define IDCT_1D(c0, c1, c2, c3, in)					\
	do {								\
		tmp1=((c0)*DeZigZag[in])+((c2)*DeZigZag[(in)+2]);	\
		tmp2=(c1)*DeZigZag[(in)+1];				\
		tmp3=(c3)*DeZigZag[(in)+3];				\
	} while (0)

#define COMPOSE_1(out1, out2)		\
	do {				\
		tmp=tmp1+tmp2+tmp3;	\
		temp[out1] = tmp>>15;	\
		tmp=tmp1-tmp2-tmp3;	\
		temp[out2] = tmp>>15;	\
	} while (0)

#define COMPOSE_2(out1, out2)		\
	do {				\
		tmp=tmp1+tmp2-tmp3;	\
		temp[out1] = tmp>>15;	\
		tmp=tmp1-tmp2+tmp3;	\
		temp[out2] = tmp>>15;	\
	} while (0)

	/* j = 0 */
	IDCT_1D(a, b,  c, d,  0); COMPOSE_1( 0, 56);
	IDCT_1D(a, b,  c, d,  8); COMPOSE_1( 1, 57);
	IDCT_1D(a, b,  c, d, 16); COMPOSE_1( 2, 58);
	IDCT_1D(a, b,  c, d, 24); COMPOSE_1( 3, 59);

	/* j = 1 */
	IDCT_1D(a, d,  f, g,  0); COMPOSE_2( 8, 48);
	IDCT_1D(a, d,  f, g,  8); COMPOSE_2( 9, 49);
	IDCT_1D(a, d,  f, g, 16); COMPOSE_2(10, 50);
	IDCT_1D(a, d,  f, g, 24); COMPOSE_2(11, 51);

	/* j = 2 */
	IDCT_1D(a, e, -f, b,  0); COMPOSE_2(16, 40);
	IDCT_1D(a, e, -f, b,  8); COMPOSE_2(17, 41);
	IDCT_1D(a, e, -f, b, 16); COMPOSE_2(18, 42);
	IDCT_1D(a, e, -f, b, 24); COMPOSE_2(19, 43);

	/* j = 3 */
	IDCT_1D(a, g, -c, e,  0); COMPOSE_2(24, 32);
	IDCT_1D(a, g, -c, e,  8); COMPOSE_2(25, 33);
	IDCT_1D(a, g, -c, e, 16); COMPOSE_2(26, 34);
	IDCT_1D(a, g, -c, e, 24); COMPOSE_2(27, 35);

#undef IDCT_1D
#undef COMPOSE_1
#undef COMPOSE_2

	/*****************
	 **** IDCT 2D ****
	 *****************/

#define IDCT_2D(c0, c1, c2, c3, in)				\
	do {							\
		tmp = temp[in]*(c0) + temp[(in)+1]*(c1)		\
		    + temp[(in)+2]*(c2) + temp[(in)+3]*(c3);	\
	} while (0)

#define STORE(i)				\
	do {					\
		tmp = tmp >> 15;		\
		tmp = tmp + 128;		\
		if (tmp > 255) tmp = 255;	\
		if (tmp < 0)   tmp = 0;		\
		pOut[i] = (unsigned char) tmp;	\
	} while (0)

#define IDCT_2D_ROW(in)						\
	do {							\
		IDCT_2D(a,  b,  c,  d, in); STORE(0+out_idx);	\
		IDCT_2D(a,  d,  f, -g, in); STORE(1+out_idx);	\
		IDCT_2D(a,  e, -f, -b, in); STORE(2+out_idx);	\
		IDCT_2D(a,  g, -c, -e, in); STORE(3+out_idx);	\
		IDCT_2D(a, -g, -c,  e, in); STORE(4+out_idx);	\
		IDCT_2D(a, -e, -f,  b, in); STORE(5+out_idx);	\
		IDCT_2D(a, -d,  f,  g, in); STORE(6+out_idx);	\
		IDCT_2D(a, -b,  c, -d, in); STORE(7+out_idx);	\
	} while (0)


#define IDCT_2D_FAST(c0, c1, c2, c3, in)			\
	do {							\
		tmp1=((c0)*temp[in])+((c2)*temp[(in)+2]);	\
		tmp2=(c1)*temp[(in)+1];				\
		tmp3=(c3)*temp[(in)+3];				\
	} while (0)

#define STORE_FAST_1(out1, out2)				\
	do {							\
		tmp=tmp1+tmp2+tmp3;				\
		STORE((out1)+out_idx);				\
		tmp=tmp1-tmp2-tmp3;				\
		STORE((out2)+out_idx);				\
	} while (0)

#define STORE_FAST_2(out1, out2)				\
	do {							\
		tmp=tmp1+tmp2-tmp3;				\
		STORE((out1)+out_idx);				\
		tmp=tmp1-tmp2+tmp3;				\
		STORE((out2)+out_idx);				\
	} while (0)

#define IDCT_2D_FAST_ROW(in)						\
	do {								\
		IDCT_2D_FAST(a, b,  c, d, in);	STORE_FAST_1(0, 7);	\
		IDCT_2D_FAST(a, d,  f, g, in);	STORE_FAST_2(1, 6);	\
		IDCT_2D_FAST(a, e, -f, b, in);	STORE_FAST_2(2, 5);	\
		IDCT_2D_FAST(a, g, -c, e, in);	STORE_FAST_2(3, 4);	\
	} while (0)

	out_idx = out_pos;

	IDCT_2D_ROW(0);		out_idx += w;
	IDCT_2D_ROW(8);		out_idx += w;
	IDCT_2D_ROW(16);	out_idx += w;
	IDCT_2D_ROW(24);	out_idx += w;
	IDCT_2D_ROW(32);	out_idx += w;
	IDCT_2D_ROW(40);	out_idx += w;
	IDCT_2D_FAST_ROW(48);	out_idx += w;
	IDCT_2D_FAST_ROW(56);

	*iIn = in_pos;
	*iOut = out_pos + 8;
}

#define DECOMP_Y() DecompressYHI(pIn, pY, &iIn, &iY, w, 1)
#define DECOMP_U() DecompressYHI(pIn, pU, &iIn, &iU, w/2, 2)
#define DECOMP_V() DecompressYHI(pIn, pV, &iIn, &iV, w/2, 2)

inline static int
Decompress400HiNoMMX(unsigned char *pIn,
		     unsigned char *pOut,
		     const int      w,
		     const int      h,
		     const int      inSize)
{
	unsigned char *pY = pOut;
	int x, y, iIn, iY;

	iIn = 0;
	for (y = 0; y < h; y += 8) {
		iY = w*y;

		for (x = 0; x < w; x += 8)
			DECOMP_Y();
	}

	return 0;
}

inline static int
Decompress420HiNoMMX(unsigned char *pIn,
		     unsigned char *pOut,
		     const int      w,
		     const int      h,
		     const int      inSize)
{
	unsigned char *pY = pOut;
	unsigned char *pU = pY + w*h;
	unsigned char *pV = pU + w*h/4;
	int xY, xUV, iY, iU, iV, iIn, count;
	const int nBlocks = (w*h) / (32*8);

	iIn = 0;
	iY = iU = iV = 0;
	xY = xUV = 0;

	for (count = 0; count < nBlocks; count++) {
			DECOMP_U();
			DECOMP_V();	xUV += 16;
			if (xUV >= w) {
				iU += (w*7)/2;
				iV += (w*7)/2;
				xUV = 0;
			}

			DECOMP_Y();	xY += 8;
			DECOMP_Y();	xY += 8;
			if (xY >= w) {
				iY += w*7;
				xY = 0;
			}
			DECOMP_Y();	xY += 8;
			DECOMP_Y();	xY += 8;
			if (xY >= w) {
				iY += w*7;
				xY = 0;
			}
	}

	return 0;
}

/* Input format is raw isoc. data (with header and packet
 * number stripped, and all-zero blocks removed).
 * Output format is YUV400
 * Returns uncompressed data length if success, or zero if error
 */
static int
Decompress400(unsigned char *pIn,
	      unsigned char *pOut,
	      unsigned char *pTmp,
	      int	     w,
	      int	     h,
	      int	     inSize)
{
	int numpix = w * h;
	int rc;

	PDEBUG(4, "%dx%d pIn=%p pOut=%p inSize=%d", w, h, pIn, pOut, inSize);

	rc = Decompress400HiNoMMX(pIn, pOut, w, h, inSize);

	if (rc)
		return 0;

	return numpix;
}

/* Input format is raw isoc. data (with header and packet
 * number stripped, and all-zero blocks removed).
 * Output format is planar YUV420
 * Returns uncompressed data length if success, or zero if error
 */
static int
Decompress420(unsigned char *pIn,
	      unsigned char *pOut,
	      unsigned char *pTmp,
	      int	     w,
	      int	     h,
	      int	     inSize)
{
	int numpix = w * h;
	int rc;

	PDEBUG(4, "%dx%d pIn=%p pOut=%p inSize=%d", w, h, pIn, pOut, inSize);

	rc = Decompress420HiNoMMX(pIn, pOut, w, h, inSize);

	if (rc)
		return 0;

	return (numpix * 3 / 2);
}

/******************************************************************************
 * Module Functions
 ******************************************************************************/

static struct ov51x_decomp_ops decomp_ops = {
	.decomp_400 =	Decompress400,
	.decomp_420 =	Decompress420,
	.owner =	THIS_MODULE,
};

static int __init
decomp_init(void)
{
	int rc;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 5, 0)
	EXPORT_NO_SYMBOLS;
#endif

	rc = ov511_register_decomp_module(DECOMP_INTERFACE_VER, &decomp_ops,
					  ov518, mmx);
	if (rc) {
		err("Could not register with ov511 (rc=%d)", rc);
		return -1;
	}

	info(DRIVER_VERSION " : " DRIVER_DESC);

	return 0;
}

static void __exit
decomp_exit(void)
{
	ov511_deregister_decomp_module(ov518, mmx);
	info("deregistered\n");
}

module_init(decomp_init);
module_exit(decomp_exit);
