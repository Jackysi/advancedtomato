/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2002 by Ralf Baechle
 */
#ifndef _ASM_WAR_H
#define _ASM_WAR_H

#include <linux/config.h>

/*
 * Pleassures of the R4600 V1.x.  Cite from the IDT R4600 V1.7 errata:
 *
 *  18. The CACHE instructions Hit_Writeback_Invalidate_D, Hit_Writeback_D,
 *      Hit_Invalidate_D and Create_Dirty_Excl_D should only be
 *      executed if there is no other dcache activity. If the dcache is
 *      accessed for another instruction immeidately preceding when these
 *      cache instructions are executing, it is possible that the dcache
 *      tag match outputs used by these cache instructions will be
 *      incorrect. These cache instructions should be preceded by at least
 *      four instructions that are not any kind of load or store
 *      instruction.
 *
 *      This is not allowed:    lw
 *                              nop
 *                              nop
 *                              nop
 *                              cache       Hit_Writeback_Invalidate_D
 *
 *      This is allowed:        lw
 *                              nop
 *                              nop
 *                              nop
 *                              nop
 *                              cache       Hit_Writeback_Invalidate_D
 */
#define R4600_V1_HIT_DCACHE_WAR


/*
 * Writeback and invalidate the primary cache dcache before DMA.
 *
 * R4600 v2.0 bug: "The CACHE instructions Hit_Writeback_Inv_D,
 * Hit_Writeback_D, Hit_Invalidate_D and Create_Dirty_Exclusive_D will only
 * operate correctly if the internal data cache refill buffer is empty.  These
 * CACHE instructions should be separated from any potential data cache miss
 * by a load instruction to an uncached address to empty the response buffer."
 * (Revision 2.0 device errata from IDT available on http://www.idt.com/
 * in .pdf format.)
 */
#define R4600_V2_HIT_CACHEOP_WAR

#ifdef CONFIG_CPU_R5432

/*
 * When an interrupt happens on a CP0 register read instruction, CPU may
 * lock up or read corrupted values of CP0 registers after it enters
 * the exception handler.
 *
 * This workaround makes sure that we read a "safe" CP0 register as the
 * first thing in the exception handler, which breaks one of the
 * pre-conditions for this problem.
 */
#define	R5432_CP0_INTERRUPT_WAR

#endif

#if defined(CONFIG_SB1_PASS_1_WORKAROUNDS) || \
    defined(CONFIG_SB1_PASS_2_WORKAROUNDS)

/*
 * Workaround for the Sibyte M3 errata the text of which can be found at
 *
 *   http://sibyte.broadcom.com/hw/bcm1250/docs/pass2errata.txt
 *
 * This will enable the use of a special TLB refill handler which does a
 * consistency check on the information in c0_badvaddr and c0_entryhi and
 * will just return and take the exception again if the information was
 * found to be inconsistent.
 */
#define BCM1250_M3_WAR

#endif

#endif /* _ASM_WAR_H */
