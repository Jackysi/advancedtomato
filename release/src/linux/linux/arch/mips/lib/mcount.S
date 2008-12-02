/*
 * Copyright (C) 2000 Silicon Graphics, Inc.
 *
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 *
 * This file implements mcount(), which is used to collect profiling data.
 * We provide several variants to accomodate different types of callers at
 * the lowest possible overhead.
 */

#include <asm/regdef.h>
#include <asm/asm.h>
#include <linux/config.h>

.macro MCOUNT
	.set	push
	.set	noat
	.set	noreorder
#if defined(CONFIG_KERNPROF) || defined(CONFIG_KERNPROF_MODULE)
	lw	t9, (mcount_hook)
	beqz	t9, 1f
	 nop
	subu	sp, 16
	sw	a0, 0(sp) 
	sw	a1, 4(sp)
	sw	a2, 8(sp)
	sw	a3, 12(sp)
	sw	ra, 16(sp)
	sw	AT, 20(sp)
	move	a1, ra		# mcount's parent (another gas bug -> ')
	jalr	t9
	 move	a0, AT		# mcount's parent's parent
	lw	ra, 20(sp)
	lw	AT, 16(sp)
	lw	a3, 12(sp)
	lw	a2, 8(sp)
	lw	a1, 4(sp)
	lw	a0, 0(sp)
	jr	AT	
	 addu	sp, 24
#endif
1:	move	t0, AT
	move	AT, ra
	move	ra, t0
	jr	AT
	 addu	sp, 8		# sp is adjusted by -8 when we are called
	.set	pop
.endm


/*
 * This is the main variant and is called by C code.  GCC's -pg option
 * automatically instruments every C function with a call to this.
 */
FEXPORT(_mcount)
	MCOUNT
