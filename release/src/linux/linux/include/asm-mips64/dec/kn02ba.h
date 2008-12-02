/*
 *	include/asm-mips/dec/kn02ba.h
 *
 *	DECstation 5000/1xx (3min or KN02-BA) definitions.
 *
 *	Copyright (C) 2002  Maciej W. Rozycki
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#ifndef __ASM_MIPS_DEC_KN02BA_H
#define __ASM_MIPS_DEC_KN02BA_H

#include <asm/dec/kn02xa.h>		/* For common definitions. */

/*
 * Some port addresses...
 */
#define KN02BA_IOASIC_BASE	KN02XA_IOASIC_BASE	/* I/O ASIC */
#define KN02BA_RTC_BASE		KN02XA_RTC_BASE		/* RTC */

/*
 * CPU interrupt bits.
 */
#define KN02BA_CPU_INR_HALT	6	/* HALT button */
#define KN02BA_CPU_INR_CASCADE	5	/* I/O ASIC cascade */
#define KN02BA_CPU_INR_TC2	4	/* TURBOchannel slot #2 */
#define KN02BA_CPU_INR_TC1	3	/* TURBOchannel slot #1 */
#define KN02BA_CPU_INR_TC0	2	/* TURBOchannel slot #0 */

/*
 * I/O ASIC interrupt bits.  Star marks denote non-IRQ status bits.
 */
#define KN02BA_IO_INR_RES_15	15	/* unused */
#define KN02BA_IO_INR_NVRAM	14	/* (*) NVRAM clear jumper */
#define KN02BA_IO_INR_RES_13	13	/* unused */
#define KN02BA_IO_INR_MEMORY	12	/* memory, I/O bus write errors */
#define KN02BA_IO_INR_RES_11	11	/* unused */
#define KN02BA_IO_INR_NRMOD	10	/* (*) NRMOD manufacturing jumper */
#define KN02BA_IO_INR_ASC	9	/* ASC (NCR53C94) SCSI */
#define KN02BA_IO_INR_LANCE	8	/* LANCE (Am7990) Ethernet */
#define KN02BA_IO_INR_SCC1	7	/* SCC (Z85C30) serial #1 */
#define KN02BA_IO_INR_SCC0	6	/* SCC (Z85C30) serial #0 */
#define KN02BA_IO_INR_RTC	5	/* DS1287 RTC */
#define KN02BA_IO_INR_PSU	4	/* power supply unit warning */
#define KN02BA_IO_INR_RES_3	3	/* unused */
#define KN02BA_IO_INR_ASC_DATA	2	/* SCSI data ready (discouraged?) */
#define KN02BA_IO_INR_PBNC	1	/* HALT button debouncer */
#define KN02BA_IO_INR_PBNO	0	/* ~HALT button debouncer */

#endif /* __ASM_MIPS_DEC_KN02BA_H */
