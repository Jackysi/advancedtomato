/*
 * Copyright (C) 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#include <linux/config.h>

#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/sibyte/board.h>

	.text
	/* Special Cache Error handler for SB1 for now */
	LEAF(except_vec2_sb1)
	.set	noat
	.set	mips0
	/*
	 * This is a very bad place to be.  Our cache error detection has
	 * triggered.  If we have write-back data in the cache, we may not be
	 * able to recover.  As a first-order desperate measure, turn off KSEG0
	 * cacheing.
	 */
	.set	push
	#.set	mips64
	.set	mips4
	.set	reorder
	mfc0	k1, $26				# mfc0	k1, $26, 0
	# check if error was recoverable
	# XXXKW - count them
	bltz	k1, leave_cerr
#ifdef CONFIG_SB1_PASS_1_WORKAROUNDS
	# look for signature of spurious CErr
	lui	k0, 0x4000
	bne	k0, k1, real_cerr
	.word	0x401Bd801			# mfc0	k1, $27, 1
	lui	k0, 0xffe0
	and	k1, k0, k1
	lui	k0, 0x0200
	beq	k0, k1, leave_cerr
	# XXXKW - count them
#endif

real_cerr:
	mfc0	k0,CP0_CONFIG
	li	k1,~CONF_CM_CMASK
	and	k0,k0,k1
	ori	k0,k0,CONF_CM_UNCACHED
	mtc0	k0,CP0_CONFIG

	SSNOP
	SSNOP
	SSNOP
	SSNOP
	bnezl	$0, 1f
1:
	mfc0	k0, CP0_STATUS
	sll	k0, k0, 3			# check CU0 (kernel?)
	bltz	k0, 2f
	get_saved_sp
	move	sp, k1				# want Kseg SP (so uncached)
2:
	j	sb1_cache_error

leave_cerr:
	# clear/unlock the registers
	mtc0	zero, $26			# mtc0	zero, $26, 0
	mtc0	zero, $27			# mtc0	zero, $27, 0
	.word	0x4080d801			# mtc0	zero, $27, 1
	.word	0x4080d803			# mtc0	zero, $27, 3
	eret

	.set	pop
	END(except_vec2_sb1)
