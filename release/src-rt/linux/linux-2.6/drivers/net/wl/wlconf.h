/*
 * Broadcom 802.11abg Networking Device Driver Configuration file
 *
 * Copyright (C) 2009, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wltunable_lx_router.h,v 1.3.98.3 2009/11/10 01:05:41 Exp $
 *
 * wl driver tunables
 */

#if 0
#define D11CONF		0x00a7bab0	/* D11 Core Rev 4, 5 (4306C0), 7 (4712), 9 (4318b0, 5352),
					 * 11 (4321a1), 12 (4321b/c), 13 (5354), 15(4312),
					 * 16 (4322), 17 (4716), 18 (43224a0), 21 (5356),
					 * 23 (43224b0).
					 */
#endif

#define NRXBUFPOST	56	/* # rx buffers posted */

#define RXBND		24	/* max # rx frames to process */

#define CTFPOOLSZ	64	/* max buffers in ctfpool */

#define WME_PER_AC_TX_PARAMS 1
#define WME_PER_AC_TUNING 1
