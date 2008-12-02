/*
 * Hardware info about DECstation 5000/200 systems (otherwise known as
 * 3max or KN02).
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1995,1996 by Paul M. Antoine, some code and definitions
 * are by courtesy of Chris Fraser.
 * Copyright (C) 2002  Maciej W. Rozycki
 */
#ifndef __ASM_MIPS_DEC_KN02_H
#define __ASM_MIPS_DEC_KN02_H

#ifndef __ASSEMBLY__
#include <linux/spinlock.h>
#include <linux/types.h>
#endif

#include <asm/addrspace.h>


/*
 * Motherboard regs (kseg1 addresses)
 */
#define KN02_CSR_ADDR	KSEG1ADDR(0x1ff00000)	/* system control & status reg */

/*
 * Some port addresses...
 */
#define KN02_SLOT_SIZE	0x00080000

#define KN02_RTC_BASE	KSEG1ADDR(0x1fe80000)
#define KN02_DZ11_BASE	KSEG1ADDR(0x1fe00000)

#define KN02_CSR_BNK32M	(1<<10)			/* 32M stride */


/*
 * CPU interrupt bits.
 */
#define KN02_CPU_INR_RES_6	6	/* unused */
#define KN02_CPU_INR_MEMORY	5	/* memory, I/O bus write errors */
#define KN02_CPU_INR_RES_4	4	/* unused */
#define KN02_CPU_INR_RTC	3	/* DS1287 RTC */
#define KN02_CPU_INR_CASCADE	2	/* CSR cascade */

/*
 * CSR interrupt bits.
 */
#define KN02_CSR_INR_DZ11	7	/* DZ11 (DC7085) serial */
#define KN02_CSR_INR_LANCE	6	/* LANCE (Am7990) Ethernet */
#define KN02_CSR_INR_ASC	5	/* ASC (NCR53C94) SCSI */
#define KN02_CSR_INR_RES_4	4	/* unused */
#define KN02_CSR_INR_RES_3	3	/* unused */
#define KN02_CSR_INR_TC2	2	/* TURBOchannel slot #2 */
#define KN02_CSR_INR_TC1	1	/* TURBOchannel slot #1 */
#define KN02_CSR_INR_TC0	0	/* TURBOchannel slot #0 */


#define KN02_IRQ_BASE		8	/* first IRQ assigned to CSR */
#define KN02_IRQ_LINES		8	/* number of CSR interrupts */

#define KN02_IRQ_NR(n)		((n) + KN02_IRQ_BASE)
#define KN02_IRQ_MASK(n)	(1 << (n))
#define KN02_IRQ_ALL		0xff


#ifndef __ASSEMBLY__
extern u32 cached_kn02_csr;
extern spinlock_t kn02_lock;
extern void init_kn02_irqs(int base);
#endif

#endif /* __ASM_MIPS_DEC_KN02_H */
