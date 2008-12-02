/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1997, 1998, 1999 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999 Silicon Graphics, Inc.
 * Copyright (C) 2000 Kanoj Sarcar (kanoj@sgi.com)
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/r10kcache.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

static int scache_lsz64;

static void andes_flush_cache_all(void)
{
}

static void andes_flush_cache_mm(struct mm_struct *mm)
{
}

static void andes_flush_cache_range(struct mm_struct *mm, unsigned long start,
                                    unsigned long end)
{
}

static void andes_flush_cache_page(struct vm_area_struct *vma,
                                   unsigned long page)
{
}

static void andes_flush_page_to_ram(struct page *page)
{
}

/* Cache operations.  These are only used with the virtual memory system,
   not for non-coherent I/O so it's ok to ignore the secondary caches.  */
static void
andes_flush_cache_l1(void)
{
	blast_dcache32(); blast_icache64();
}

/*
 * This is only used during initialization time. vmalloc() also calls
 * this, but that will be changed pretty soon.
 */
static void
andes_flush_cache_l2(void)
{
	switch (sc_lsize()) {
		case 64:
			blast_scache64();
			break;
		case 128:
			blast_scache128();
			break;
		default:
			printk("Unknown L2 line size\n");
			while(1);
	}
}

static void andes___flush_cache_all(void)
{
	andes_flush_cache_l1();
	andes_flush_cache_l2();
}

void
andes_flush_icache_page(unsigned long page)
{
	if (scache_lsz64)
		blast_scache64_page(page);
	else
		blast_scache128_page(page);
}

static void
andes_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

void __init ld_mmu_andes(void)
{
	printk("Primary instruction cache %dkb, linesize %d bytes\n",
	       icache_size >> 10, ic_lsize);
	printk("Primary data cache %dkb, linesize %d bytes\n",
	       dcache_size >> 10, dc_lsize);
	printk("Secondary cache sized at %ldK, linesize %ld\n",
	       scache_size() >> 10, sc_lsize());

	_clear_page = andes_clear_page;
	_copy_page = andes_copy_page;

	_flush_cache_all = andes_flush_cache_all;
	___flush_cache_all = andes___flush_cache_all;
	_flush_cache_mm = andes_flush_cache_mm;
	_flush_cache_page = andes_flush_cache_page;
	_flush_page_to_ram = andes_flush_page_to_ram;
	_flush_cache_l1 = andes_flush_cache_l1;
	_flush_cache_l2 = andes_flush_cache_l2;
	_flush_cache_sigtramp = andes_flush_cache_sigtramp;

	switch (sc_lsize()) {
		case 64:
			scache_lsz64 = 1;
			break;
		case 128:
			scache_lsz64 = 0;
			break;
		default:
			printk("Unknown L2 line size\n");
			while(1);
	}

        flush_cache_l1();
}
