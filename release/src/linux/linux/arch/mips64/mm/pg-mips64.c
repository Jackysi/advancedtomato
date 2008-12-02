/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2002 MIPS Technologies, Inc.  All rights reserved.
 *
 * This program is free software; you can distribute it and/or modify it
 * under the terms of the GNU General Public License (Version 2) as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * MIPS64 CPU variant specific Cache routines.
 * These routine are not optimized in any way, they are done in a generic way
 * so they can be used on all MIPS64 compliant CPUs, and also done in an
 * attempt not to break anything for the R4xx0 style CPUs.
 */
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>

extern int dc_lsize, ic_lsize, sc_lsize;

/*
 * Zero an entire page.
 */

void mips64_clear_page_dc(unsigned long page)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX)
	{
	        for (i=page; i<page+PAGE_SIZE; i+=dc_lsize)
		{
		        __asm__ __volatile__(
			        ".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_D));
		}
	}
	for (i=page; i<page+PAGE_SIZE; i+=sizeof(long))
	        *(unsigned long *)(i) = 0;
}

void mips64_clear_page_sc(unsigned long page)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX)
	{
	        for (i=page; i<page+PAGE_SIZE; i+=sc_lsize)
		{
		        __asm__ __volatile__(
				".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_SD));
		}
	}
	for (i=page; i<page+PAGE_SIZE; i+=sizeof(long))
	        *(unsigned long *)(i) = 0;
}

void mips64_copy_page_dc(unsigned long to, unsigned long from)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX)
	{
	        for (i=to; i<to+PAGE_SIZE; i+=dc_lsize)
		{
		        __asm__ __volatile__(
			        ".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_D));
		}
	}
	for (i=0; i<PAGE_SIZE; i+=sizeof(long))
	        *(unsigned long *)(to+i) = *(unsigned long *)(from+i);
}

void mips64_copy_page_sc(unsigned long to, unsigned long from)
{
	unsigned long i;

        if (mips_cpu.options & MIPS_CPU_CACHE_CDEX)
	{
	        for (i=to; i<to+PAGE_SIZE; i+=sc_lsize)
		{
		        __asm__ __volatile__(
				".set\tnoreorder\n\t"
				".set\tnoat\n\t"
				"cache\t%2,(%0)\n\t"
				".set\tat\n\t"
				".set\treorder"
				:"=r" (i)
				:"0" (i),
				"I" (Create_Dirty_Excl_SD));
		}
	}
	for (i=0; i<PAGE_SIZE; i+=sizeof(long))
	        *(unsigned long *)(to+i) = *(unsigned long *)(from+i);
}
