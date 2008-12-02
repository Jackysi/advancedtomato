/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * r4xx0.c: R4000 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997,  1998,  1999,  2000 Ralf Baechle ralf@gnu.org
 */
#include <asm/asm.h>
#include <asm/regdef.h>
#include <asm/offset.h>

	.text
	.set	mips4
	.set	noat

/*
 * This is suboptimal for 32-bit kernels; we assume that R10000 is only used
 * with 64-bit kernels.  The prefetch offsets have been experimentally tuned
 * an Origin 200.
 */

LEAF(andes_clear_page)
	LONG_ADDIU	AT, a0, _PAGE_SIZE
1:	pref		7, 512(a0)
	LONG_S		zero, 0*SZREG(a0)
	LONG_S		zero, 1*SZREG(a0)
	LONG_S		zero, 2*SZREG(a0)
	LONG_S		zero, 3*SZREG(a0)
	LONG_ADDIU	a0, a0, 8*SZREG
	LONG_S		zero, -4*SZREG(a0)
	LONG_S		zero, -3*SZREG(a0)
	LONG_S		zero, -2*SZREG(a0)
	LONG_S		zero, -1*SZREG(a0)
	bne		AT, a0, 1b
	j		ra
	.end		andes_clear_page
	END(andes_clear_page)

LEAF(andes_copy_page)
	LONG_ADDIU	AT, a0, _PAGE_SIZE
1:	pref		0, 2*128(a1)
	pref		1, 2*128(a0)
	LONG_L		a3, 0*SZREG(a1)
	LONG_L		a2, 1*SZREG8(a1)
	LONG_L		v1, 2*SZREG(a1)
	LONG_L		v0, 3*SZREG(a1)
	LONG_S		a3, 0*SZREG(a0)
	LONG_S		a2, 1*SZREG(a0)
	LONG_S		v1, 2*SZREG(a0)
	LONG_S		v0, 3*SZREG(a0)
	LONG_ADDIU	a0, a0, 8*SZREG
	LONG_ADDIU	a1, a1, 8*SZREG
	LONG_L		a3, -4*SZREG(a1)
	LONG_L		a2, -3*SZREG(a1)
	LONG_L		v1, -2*SZREG(a1)
	LONG_L		v0, -1*SZREG(a1)
	LONG_S		a3, -4*SZREG(a0)
	LONG_S		a2, -3*SZREG(a0)
	LONG_S		v1, -2*SZREG(a0)
	LONG_S		v0, -1*SZREG(a0)
	bne		AT, a0,1b
	j		ra
	END(andes_copy_page)
