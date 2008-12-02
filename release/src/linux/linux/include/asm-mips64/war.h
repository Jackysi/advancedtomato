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
#undef R4600_V1_HIT_DCACHE_WAR	/* Not used yet in 64-bit kernel */


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
#undef R4600_V2_HIT_CACHEOP_WAR	/* Not used yet in 64-bit kernel */

#ifdef CONFIG_CPU_R5432

#define	R5432_CP0_INTERRUPT_WAR

#endif

#if defined(CONFIG_SB1_PASS_1_WORKAROUNDS) || \
    defined(CONFIG_SB1_PASS_2_WORKAROUNDS)

#define BCM1250_M3_WAR

#endif

#endif /* _ASM_WAR_H */
