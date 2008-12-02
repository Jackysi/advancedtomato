/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 * Copyright (C) 2002 Ralf Baechle
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
#ifndef _LIB_HSSUBR_H
#define _LIB_HSSUBR_H

#include <linux/config.h>
#include <asm/addrspace.h>

typedef long hsaddr_t;

#ifdef CONFIG_MIPS64

#define PHYS_TO_XKSEG_UNCACHED(b) ((b) | K1BASE)

static inline void hs_write8(hsaddr_t a, uint8_t b)
{
	*(volatile uint8_t *) PHYS_TO_XKSEG_UNCACHED(a) = b;
}

static inline void hs_write16(hsaddr_t a, uint16_t b)
{
	*(volatile uint16_t *) PHYS_TO_XKSEG_UNCACHED(a) = b;
}

static inline void hs_write32(hsaddr_t a, uint32_t b)
{
	*(volatile uint32_t *) PHYS_TO_XKSEG_UNCACHED(a) = b;
}

static inline void hs_write64(hsaddr_t a, uint64_t b)
{
	*(volatile uint32_t *) PHYS_TO_XKSEG_UNCACHED(a) = b;
}

static inline uint8_t hs_read8(hsaddr_t a)
{
	return *(volatile uint8_t *) PHYS_TO_XKSEG_UNCACHED(a);
}

static inline uint16_t hs_read16(hsaddr_t a)
{
	return *(volatile uint16_t *) PHYS_TO_XKSEG_UNCACHED(a);
}

static inline uint32_t hs_read32(hsaddr_t a)
{
	return *(volatile uint32_t *) PHYS_TO_XKSEG_UNCACHED(a);
}

static inline uint64_t hs_read64(hsaddr_t a)
{
	return *(volatile uint64_t *) PHYS_TO_XKSEG_UNCACHED(a);
}

#else	/* just CONFIG_MIPS32 */

extern void hs_write8(hsaddr_t a, uint8_t b);
extern void hs_write16(hsaddr_t a, uint16_t b);
extern void hs_write32(hsaddr_t a, uint32_t b);
extern void hs_write64(hsaddr_t a, uint64_t b);
extern uint8_t hs_read8(hsaddr_t a);
extern uint16_t hs_read16(hsaddr_t a);
extern uint32_t hs_read32(hsaddr_t a);
extern uint64_t hs_read64(hsaddr_t a);
#endif

#endif /* _LIB_HSSUBR_H */
