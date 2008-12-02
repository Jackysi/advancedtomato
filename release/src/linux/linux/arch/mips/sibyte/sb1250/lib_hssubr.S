/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
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
#include <asm/asm.h>
#include <asm/addrspace.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>

		 .set mips64

#define HAZARD SSNOP ; SSNOP ; SSNOP ; SSNOP ; SSNOP ; SSNOP ; SSNOP


/*  *********************************************************************
    *  hs_read8 - read 8-bit bytes
    ********************************************************************* */


LEAF(hs_read8)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0,v0
		lb	v0, (a0)
		and	v0, 0xFF

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_read8)

/*  *********************************************************************
    *  hs_read16 - read 16-bit shorts
    ********************************************************************* */

LEAF(hs_read16)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		lh	v0, (a0)
		and	v0, 0xFFFF

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_read16)

/*  *********************************************************************
    *  hs_read32 - read 32-bit ints
    ********************************************************************* */

LEAF(hs_read32)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		lw	v0, (a0)
		and	v0, 0xFFFFFFFF

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_read32)

/*  *********************************************************************
    *  hs_read64 - read 64-bit longs
    ********************************************************************* */

LEAF(hs_read64)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		ld	v0, (a0)

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_read64)

/*  *********************************************************************
    *  hs_write8 - write 8-bit bytes
    ********************************************************************* */

LEAF(hs_write8)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		sb	a1, (a0)

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_write8)

/*  *********************************************************************
    *  hs_write16 - write 16-bit shorts
    ********************************************************************* */

LEAF(hs_write16)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		sh	a1, (a0)

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_write16)

/*  *********************************************************************
    *  hs_write32 - write 32-bit longs
    ********************************************************************* */

LEAF(hs_write32)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		sw	a1, (a0)

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_write32)

/*  *********************************************************************
    *  hs_write64 - write 64-bit longs
    ********************************************************************* */

LEAF(hs_write64)
		mfc0	t2, CP0_STATUS
		or	t1, t2, ST0_KX
		mtc0	t1, CP0_STATUS
		HAZARD

		dli	v0, PHYS_TO_XKSEG_UNCACHED(0)
		dsll	a0, a0, 32
		dsrl	a0, a0, 32
		or	a0, a0, v0
		sd	a1, (a0)

		mtc0	t2, CP0_STATUS
		HAZARD
		j	ra
END(hs_write64)
