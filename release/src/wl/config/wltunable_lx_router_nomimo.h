/*
 * Broadcom 802.11abg Networking Device Driver Configuration file
 *
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *                                     
 * $Id$
 *
 * wl driver tunables without nphy support
 */

#define NCONF	0	/* no nphy */

#define NRXBUFPOST	32	/* # rx buffers posted */
#define ACONF 0x0000    /* No support for A-Phy operation */
