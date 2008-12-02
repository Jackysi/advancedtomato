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
	ld	t9, (mcount_hook)
	beqz	t9, 1f
	 nop
	dsubu	sp, 64
	sd	a0, 0(sp) 
	sd	a1, 8(sp)
	sd	a2, 16(sp)
	sd	a3, 24(sp)
	sd	a4, 32(sp)
	sd	a5, 40(sp)
	sd	a6, 48(sp)
	sd	a7, 56(sp)
	sd	ra, 64(sp)
	sd	AT, 72(sp)
	move	a1, ra		# mcount's parent (another gas bug -> ')
	jalr	t9
	 move	a0, AT		# mcount's parent's parent
	ld	ra, 72(sp)
	ld	AT, 64(sp)
	ld	a7, 56(sp)
	ld	a6, 48(sp)
	ld	a5, 40(sp)
	ld	a4, 32(sp)
	ld	a3, 24(sp)
	ld	a2, 16(sp)
	ld	a1, 8(sp)
	ld	a0, 0(sp)
	jr	AT	
	 daddu	sp, 80
#endif
1:	move	t0, AT
	move	AT, ra
	move	ra, t0
	jr	AT
	 daddu	sp, 16		# sp is adjusted by -16 when we are called
	.set	pop
.endm


/*
 * This is the main variant and is called by C code.  GCC's -pg option
 * automatically instruments every C function with a call to this.
 */
FEXPORT(_mcount)
	MCOUNT
