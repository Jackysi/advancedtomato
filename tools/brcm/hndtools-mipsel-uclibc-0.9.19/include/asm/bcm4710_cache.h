/*
 * mips32_cache.h
 *
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Inline assembly cache operations.
 *
 * This file is the original r4cache.c file with modification that makes the
 * cache handling more generic.
 *
 * FIXME: Handle split L2 caches.
 *
 */
#ifndef _MIPS_R4KCACHE_H
#define _MIPS_R4KCACHE_H

#include <asm/asm.h>
#include <asm/cacheops.h>

#include <typedefs.h>
#include <sbconfig.h>
#include <bcm4710.h>
#include <asm/paccess.h>
/* HNBU PR2795: out-of-order write workaround */
#define BCM4710_DUMMY_RREG() (((sbconfig_t *)(KSEG1ADDR(BCM4710_REG_SDRAM + SBCONFIGOFF)))->sbimstate)
/* HNBU PR2610: cache op causing TLB miss still updates cache */
#define BCM4710_FILL_TLB(addr) (*(volatile unsigned long *)(addr))
#define BCM4710_PROTECTED_FILL_TLB(addr) ({ unsigned long x; get_dbe(x, (volatile unsigned long *)(addr)); })

static inline void flush_icache_line_indexed(unsigned long addr)
{
	unsigned long waystep = icache_size/mips_cpu.icache.ways;
	unsigned int way;

	for (way = 0; way < mips_cpu.icache.ways; way++)
	{
		__asm__ __volatile__(
			".set noreorder\n\t"
			".set mips3\n\t"
			"cache %1, (%0)\n\t"
			".set mips0\n\t"
			".set reorder"
			:
			: "r" (addr),
			"i" (Index_Invalidate_I));
		
		addr += waystep;
	}
}

static inline void flush_dcache_line_indexed(unsigned long addr)
{
	unsigned long waystep = dcache_size/mips_cpu.dcache.ways;
	unsigned int way;

	for (way = 0; way < mips_cpu.dcache.ways; way++)
	{
		BCM4710_DUMMY_RREG();
		__asm__ __volatile__(
			".set noreorder\n\t"
			".set mips3\n\t"
			"cache %1, (%0)\n\t"
			".set mips0\n\t"
			".set reorder"
			:
			: "r" (addr),
			"i" (Index_Writeback_Inv_D));

		addr += waystep;
	}
}

static inline void flush_icache_line(unsigned long addr)
{

	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n\t"
		"cache %1, (%0)\n\t"
		".set mips0\n\t"
		".set reorder"
		:
		: "r" (addr),
		  "i" (Hit_Invalidate_I));
}

static inline void flush_dcache_line(unsigned long addr)
{
	BCM4710_DUMMY_RREG();
	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n\t"
		"cache %1, (%0)\n\t"
		".set mips0\n\t"
		".set reorder"
		:
		: "r" (addr),
		  "i" (Hit_Writeback_Inv_D));
}

static inline void invalidate_dcache_line(unsigned long addr)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n\t"
		"cache %1, (%0)\n\t"
		".set mips0\n\t"
		".set reorder"
		:
		: "r" (addr),
		  "i" (Hit_Invalidate_D));
}

/*
 * The next two are for badland addresses like signal trampolines.
 */
static inline void protected_flush_icache_line(unsigned long addr)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n"
		"1:\tcache %1,(%0)\n"
		"2:\t.set mips0\n\t"
		".set reorder\n\t"
		".section\t__ex_table,\"a\"\n\t"
		STR(PTR)"\t1b,2b\n\t"
		".previous"
		:
		: "r" (addr),
		  "i" (Hit_Invalidate_I));
}

static inline void protected_writeback_dcache_line(unsigned long addr)
{
	BCM4710_DUMMY_RREG();
	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n"
		"1:\tcache %1,(%0)\n"
		"2:\t.set mips0\n\t"
		".set reorder\n\t"
		".section\t__ex_table,\"a\"\n\t"
		STR(PTR)"\t1b,2b\n\t"
		".previous"
		:
		: "r" (addr),
		  "i" (Hit_Writeback_D));
}

#define cache_unroll(base,op)	        	\
	__asm__ __volatile__("	         	\
		.set noreorder;		        \
		.set mips3;		        \
                cache %1, (%0);	                \
		.set mips0;			\
		.set reorder"			\
		:				\
		: "r" (base),			\
		  "i" (op));


static inline void blast_dcache(void)
{
	unsigned long start = KSEG0;
	unsigned long end = (start + dcache_size);

	while(start < end) {
		BCM4710_DUMMY_RREG();
		cache_unroll(start,Index_Writeback_Inv_D);
		start += dc_lsize;
	}
}

static inline void blast_dcache_page(unsigned long page)
{
	unsigned long start = page;
	unsigned long end = (start + PAGE_SIZE);

	BCM4710_FILL_TLB(start);
	while(start < end) {
		BCM4710_DUMMY_RREG();
		cache_unroll(start,Hit_Writeback_Inv_D);
		start += dc_lsize;
	}
}

static inline void blast_dcache_page_indexed(unsigned long page)
{
	unsigned long start;
	unsigned long end = (page + PAGE_SIZE);
	unsigned long waystep = dcache_size/mips_cpu.dcache.ways;
	unsigned int way;

	for (way = 0; way < mips_cpu.dcache.ways; way++) {
		start = page + way*waystep;
		while(start < end) {
			BCM4710_DUMMY_RREG();
			cache_unroll(start,Index_Writeback_Inv_D);
			start += dc_lsize;
		}
	}
}

static inline void blast_icache(void)
{
	unsigned long start = KSEG0;
	unsigned long end = (start + icache_size);

	while(start < end) {
		cache_unroll(start,Index_Invalidate_I);
		start += ic_lsize;
	}
}

static inline void blast_icache_page(unsigned long page)
{
	unsigned long start = page;
	unsigned long end = (start + PAGE_SIZE);

	BCM4710_FILL_TLB(start);
	while(start < end) {
		cache_unroll(start,Hit_Invalidate_I);
		start += ic_lsize;
	}
}

static inline void blast_icache_page_indexed(unsigned long page)
{
	unsigned long start;
	unsigned long end = (page + PAGE_SIZE);
	unsigned long waystep = icache_size/mips_cpu.icache.ways;
	unsigned int way;

	for (way = 0; way < mips_cpu.icache.ways; way++) {
		start = page + way*waystep;
		while(start < end) {
			cache_unroll(start,Index_Invalidate_I);
			start += ic_lsize;
		}
	}
}

#endif /* !(_MIPS_R4KCACHE_H) */
