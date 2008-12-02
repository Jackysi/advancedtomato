/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * r49xx.c: TX49 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998, 1999, 2000 Ralf Baechle ralf@gnu.org
 *
 * Modified for R4300/TX49xx (Jun/2001)
 * Copyright (C) 1999-2001 Toshiba Corporation
 *
 * To do:
 *
 *  - this code is a overbloated pig
 *  - many of the bug workarounds are not efficient at all, but at
 *    least they are functional ...
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */
#define ic_lsize	mips_cpu.icache.linesz
#define dc_lsize	mips_cpu.dcache.linesz
static unsigned long scache_size;

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

#undef DEBUG_CACHE

/* TX49 does can not flush the line contains the CACHE insn itself... */
/* r4k_xxx routines are completely same as those in r4xx0.c */

/*
 * If you think for one second that this stuff coming up is a lot
 * of bulky code eating too many kernel cache lines.  Think _again_.
 *
 * Consider:
 * 1) Taken branches have a 3 cycle penalty on R4k
 * 2) The branch itself is a real dead cycle on even R4600/R5000.
 * 3) Only one of the following variants of each type is even used by
 *    the kernel based upon the cache parameters we detect at boot time.
 *
 * QED.
 */

static inline void r49_flush_cache_all_d16i32(void)
{
	unsigned long flags, config;

	local_irq_save(flags);
	blast_dcache16_wayLSB();
	/* disable icache (set ICE#) */
	config = read_c0_config();
	write_c0_config(config | TX49_CONF_IC);
	blast_icache32_wayLSB();
	write_c0_config(config);
	local_irq_restore(flags);
}

static inline void r49_flush_cache_all_d32i32(void)
{
	unsigned long flags, config;

	local_irq_save(flags);
	blast_dcache32_wayLSB();
	/* disable icache (set ICE#) */
	config = read_c0_config();
	write_c0_config(config | TX49_CONF_IC);
	blast_icache32_wayLSB();
	write_c0_config(config);
	local_irq_restore(flags);
}

static void r49_flush_cache_range_d16i32(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	if (mm->context != 0) {
		unsigned long flags, config;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		local_irq_save(flags);
		blast_dcache16_wayLSB();
		/* disable icache (set ICE#) */
		config = read_c0_config();
		write_c0_config(config | TX49_CONF_IC);
		blast_icache32_wayLSB();
		write_c0_config(config);
		local_irq_restore(flags);
	}
}

static void r49_flush_cache_range_d32i32(struct mm_struct *mm,
					       unsigned long start,
					       unsigned long end)
{
	if (mm->context != 0) {
		unsigned long flags, config;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		local_irq_save(flags);
		blast_dcache32_wayLSB();
		/* disable icache (set ICE#) */
		config = read_c0_config();
		write_c0_config(config | TX49_CONF_IC);
		blast_icache32_wayLSB();
		write_c0_config(config);
		local_irq_restore(flags);
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void r49_flush_cache_mm_d16i32(struct mm_struct *mm)
{
	if (mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r49_flush_cache_all_d16i32();
	}
}

static void r49_flush_cache_mm_d32i32(struct mm_struct *mm)
{
	if (mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r49_flush_cache_all_d32i32();
	}
}

static void r49_flush_cache_page_d16i32(struct vm_area_struct *vma,
					unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if (mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if (!(pte_val(*ptep) & _PAGE_PRESENT))
		return;

	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if ((mm == current->active_mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache16_page(page);
	} else {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache16_page_indexed_wayLSB(page);
	}
}

static void r49_flush_cache_page_d32i32(struct vm_area_struct *vma,
					      unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if (mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if (!(pte_val(*ptep) & _PAGE_PRESENT))
		return;

	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if ((mm == current->active_mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache32_page(page);
	} else {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache32_page_indexed_wayLSB(page);
	}
}

/* If the addresses passed to these routines are valid, they are
 * either:
 *
 * 1) In KSEG0, so we can do a direct flush of the page.
 * 2) In KSEG2, and since every process can translate those
 *    addresses all the time in kernel mode we can do a direct
 *    flush.
 * 3) In KSEG1, no flush necessary.
 */
static void r4k_flush_page_to_ram_d16(struct page *page)
{
	blast_dcache16_page((unsigned long)page_address(page));
}

static void r4k_flush_page_to_ram_d32(struct page *page)
{
	blast_dcache32_page((unsigned long)page_address(page));
}

static void
r4k_flush_icache_range(unsigned long start, unsigned long end)
{
	flush_cache_all();
}

/*
 * Ok, this seriously sucks.  We use them to flush a user page but don't
 * know the virtual address, so we have to blast away the whole icache
 * which is significantly more expensive than the real thing.
 */
static void
r4k_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	if (!(vma->vm_flags & VM_EXEC))
		return;

	flush_cache_all();
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 */
static void
r4k_dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned int flags;

	if (size >= dcache_size) {
		flush_cache_all();
	} else {
		local_irq_save(flags);

		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
		local_irq_restore(flags);
	}
}

static void
r4k_dma_cache_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned int flags;

	if (size >= dcache_size) {
		flush_cache_all();
	} else {
		local_irq_save(flags);

		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
		local_irq_restore(flags);
	}
}

static void
r4k_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("r4k_dma_cache called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void r4k_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

/* Detect and size the various r4k caches. */
static void __init probe_icache(unsigned long config)
{
	icache_size = 1 << (12 + ((config >> 9) & 7));
	ic_lsize = 16 << ((config >> 5) & 1);

	printk("Primary instruction cache %dkb, linesize %d bytes.\n",
	       icache_size >> 10, ic_lsize);
}

static void __init probe_dcache(unsigned long config)
{
	dcache_size = 1 << (12 + ((config >> 6) & 7));
	dc_lsize = 16 << ((config >> 4) & 1);

	printk("Primary data cache %dkb, linesize %d bytes.\n",
	       dcache_size >> 10, dc_lsize);
}

int mips_configk0 = -1;	/* board-specific setup routine can override this */
void __init ld_mmu_tx49(void)
{
	unsigned long config = read_c0_config();

	if (mips_configk0 != -1)
		change_c0_config(CONF_CM_CMASK, mips_configk0);
	else
		change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);
	if (mips_cpu.icache.ways == 0)
		mips_cpu.icache.ways = 1;
	if (mips_cpu.dcache.ways == 0)
		mips_cpu.dcache.ways = 1;
	mips_cpu.icache.sets =
		icache_size / mips_cpu.icache.ways / mips_cpu.icache.linesz;
	mips_cpu.dcache.sets =
		dcache_size / mips_cpu.dcache.ways / mips_cpu.dcache.linesz;

	switch(dc_lsize) {
	case 16:
		_clear_page = r4k_clear_page_d16;
		_copy_page = r4k_copy_page_d16;
		_flush_page_to_ram = r4k_flush_page_to_ram_d16;
		_flush_cache_all = r49_flush_cache_all_d16i32;
		_flush_cache_mm = r49_flush_cache_mm_d16i32;
		_flush_cache_range = r49_flush_cache_range_d16i32;
		_flush_cache_page = r49_flush_cache_page_d16i32;
		break;
	case 32:
		_clear_page = r4k_clear_page_d32;
		_copy_page = r4k_copy_page_d32;
		_flush_page_to_ram = r4k_flush_page_to_ram_d32;
		_flush_cache_all = r49_flush_cache_all_d32i32;
		_flush_cache_mm = r49_flush_cache_mm_d32i32;
		_flush_cache_range = r49_flush_cache_range_d32i32;
		_flush_cache_page = r49_flush_cache_page_d32i32;
		break;
	}
	___flush_cache_all = _flush_cache_all;

	_flush_icache_page = r4k_flush_icache_page;

	_dma_cache_wback_inv = r4k_dma_cache_wback_inv;
	_dma_cache_wback = r4k_dma_cache_wback;
	_dma_cache_inv = r4k_dma_cache_inv;

	_flush_cache_sigtramp = r4k_flush_cache_sigtramp;
	_flush_icache_range = r4k_flush_icache_range;	/* Ouch */

	__flush_cache_all();
}
