/*
 *	include/asm-mips/dec/kn02ca.h
 *
 *	Personal DECstation 5000/xx (Maxine or KN02-CA) definitions.
 *
 *	Copyright (C) 2002  Maciej W. Rozycki
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#ifndef __ASM_MIPS_DEC_KN02CA_H
#define __ASM_MIPS_DEC_KN02CA_H

#include <asm/dec/kn02xa.h>		/* For common definitions. */

/*
 * Some port addresses...
 */
#define KN02CA_IOASIC_BASE	KN02XA_IOASIC_BASE	/* I/O ASIC */
#define KN02CA_RTC_BASE		KN02XA_RTC_BASE		/* RTC */

/*
 * CPU interrupt bits.
 */
#define KN02CA_CPU_INR_HALT	6	/* HALT from ACCESS.Bus */
#define KN02CA_CPU_INR_CASCADE	5	/* I/O ASIC cascade */
#define KN02CA_CPU_INR_MEMORY	4	/* memory, I/O bus write errors */
#define KN02CA_CPU_INR_RTC	3	/* DS1287 RTC */
#define KN02CA_CPU_INR_TIMER	2	/* ARC periodic timer */

/*
 * I/O ASIC interrupt bits.  Star marks denote non-IRQ status bits.
 */
#define KN02CA_IO_INR_FLOPPY	15	/* 82077 FDC */
#define KN02CA_IO_INR_NVRAM	14	/* (*) NVRAM clear jumper */
#define KN02CA_IO_INR_POWERON	13	/* (*) power-on reset */
#define KN02CA_IO_INR_TC0	12	/* TURBOchannel slot #0 */
#define KN02CA_IO_INR_ISDN	11	/* Am79C30A ISDN */
#define KN02CA_IO_INR_NRMOD	10	/* (*) NRMOD manufacturing jumper */
#define KN02CA_IO_INR_ASC	9	/* ASC (NCR53C94) SCSI */
#define KN02CA_IO_INR_LANCE	8	/* LANCE (Am7990) Ethernet */
#define KN02CA_IO_INR_HDFLOPPY	7	/* (*) HD (1.44MB) floppy status */
#define KN02CA_IO_INR_SCC0	6	/* SCC (Z85C30) serial #0 */
#define KN02CA_IO_INR_TC1	5	/* TURBOchannel slot #1 */
#define KN02CA_IO_INR_XDFLOPPY	4	/* (*) XD (2.88MB) floppy status */
#define KN02CA_IO_INR_VIDEO	3	/* framebuffer */
#define KN02CA_IO_INR_XVIDEO	2	/* ~framebuffer */
#define KN02CA_IO_INR_AB_XMIT	1	/* ACCESS.bus transmit */
#define KN02CA_IO_INR_AB_RECV	0	/* ACCESS.bus receive */

#endif /* __ASM_MIPS_DEC_KN02CA_H */
