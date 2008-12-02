/*
 * Kevin D. Kissell, kevink@mips.com and Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
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
 * MIPS32 CPU variant specific MMU/Cache routines.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/bcache.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/mmu_context.h>

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

/* Primary cache parameters. */
extern int icache_size, dcache_size; 			/* Size in bytes */
extern int ic_lsize, dc_lsize;				/* LineSize in bytes */

#include <asm/cacheops.h>
#include <asm/bcm4710_cache.h>

#undef DEBUG_CACHE

static inline void mips32_flush_cache_all_pc(void)
{
	unsigned long flags;

	local_irq_save(flags);
	blast_dcache(); blast_icache();
	local_irq_restore(flags);
}

static void mips32_flush_cache_range_pc(struct mm_struct *mm,
				     unsigned long start,
				     unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		local_irq_save(flags);
		blast_dcache(); blast_icache();
		local_irq_restore(flags);
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void mips32_flush_cache_mm_pc(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		mips32_flush_cache_all_pc();
	}
}

static void mips32_flush_cache_page_pc(struct vm_area_struct *vma,
				    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
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
	if (!(pte_val(*ptep) & _PAGE_VALID))
		return;

	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since Mips32 caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if (mm == current->active_mm) {
		blast_dcache_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache_page_indexed(page);
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
static void mips32_flush_page_to_ram_pc(struct page *page)
{
	blast_dcache_page((unsigned long)page_address(page));
}

static void
mips32_flush_icache_range(unsigned long start, unsigned long end)
{
	flush_cache_all();
}

static void
mips32_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	/*
	 * If there's no context yet, or the page isn't executable, no icache 
	 * flush is needed.
	 */
	if (!(vma->vm_flags & VM_EXEC))
		return;

	/*
	 * We're not sure of the virtual address(es) involved here, so
	 * conservatively flush the entire caches.
	 */
	flush_cache_all();
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 */
static void
mips32_dma_cache_wback_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned long flags;

	if (size >= dcache_size) {
		blast_dcache();
	} else if (size) {
	        local_irq_save(flags);
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		BCM4710_FILL_TLB(a);
		BCM4710_FILL_TLB(end);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
		local_irq_restore(flags);
	}
	bc_wback_inv(addr, size);
}

static void
mips32_dma_cache_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned long flags;

	if (size >= dcache_size) {
		blast_dcache();
	} else if (size) {
	        local_irq_save(flags);
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		BCM4710_FILL_TLB(a);
		BCM4710_FILL_TLB(end);
		while (1) {
			invalidate_dcache_line(a); /* Hit_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
		local_irq_restore(flags);
	}

	bc_inv(addr, size);
}

static void
mips32_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("mips32_dma_cache called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void mips32_flush_cache_sigtramp(unsigned long addr)
{
	BCM4710_PROTECTED_FILL_TLB(addr);
	BCM4710_PROTECTED_FILL_TLB(addr + 4);
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

static void mips32_flush_icache_all(void)
{
	if (mips_cpu.icache.flags | MIPS_CACHE_VTAG_CACHE) {
		blast_icache();
	}
}

/* Detect and size the various caches. */
static void __init probe_icache(unsigned long config)
{
        unsigned long config1;
	unsigned int lsize;

	mips_cpu.icache.flags = 0;
        if (!(config & (1 << 31))) {
	        /*
		 * Not a MIPS32 complainant CPU.
		 * Config 1 register not supported, we assume R4k style.
		 */
	        icache_size = 1 << (12 + ((config >> 9) & 7));
		ic_lsize = 16 << ((config >> 5) & 1);
		mips_cpu.icache.linesz = ic_lsize;

		/*
		 * We cannot infer associativity - assume direct map
		 * unless probe template indicates otherwise
		 */
		if(!mips_cpu.icache.ways) mips_cpu.icache.ways = 1;
		mips_cpu.icache.sets =
			(icache_size / ic_lsize) / mips_cpu.icache.ways;
	} else {
	       config1 = read_c0_config1();

	       if ((lsize = ((config1 >> 19) & 7)))
		       mips_cpu.icache.linesz = 2 << lsize;
	       else
		       mips_cpu.icache.linesz = lsize;
	       mips_cpu.icache.sets = 64 << ((config1 >> 22) & 7);
	       mips_cpu.icache.ways = 1 + ((config1 >> 16) & 7);

	       ic_lsize = mips_cpu.icache.linesz;
	       icache_size = mips_cpu.icache.sets * mips_cpu.icache.ways *
		             ic_lsize;

	       if ((config & 0x8) || (mips_cpu.cputype == CPU_20KC)) {
		       /* 
			* The CPU has a virtually tagged I-cache.
			* Some older 20Kc chips doesn't have the 'VI' bit in
			* the config register, so we also check for 20Kc.
			*/
		       mips_cpu.icache.flags = MIPS_CACHE_VTAG_CACHE;
		       printk("Virtually tagged I-cache detected\n");
	       }
	}
	printk("Primary instruction cache %dkb, linesize %d bytes (%d ways)\n",
	       icache_size >> 10, ic_lsize, mips_cpu.icache.ways);
}

static void __init probe_dcache(unsigned long config)
{
        unsigned long config1;
	unsigned int lsize;

	mips_cpu.dcache.flags = 0;
        if (!(config & (1 << 31))) {
	        /*
		 * Not a MIPS32 complainant CPU.
		 * Config 1 register not supported, we assume R4k style.
		 */
		dcache_size = 1 << (12 + ((config >> 6) & 7));
		dc_lsize = 16 << ((config >> 4) & 1);
		mips_cpu.dcache.linesz = dc_lsize;
		/*
		 * We cannot infer associativity - assume direct map
		 * unless probe template indicates otherwise
		 */
		if(!mips_cpu.dcache.ways) mips_cpu.dcache.ways = 1;
		mips_cpu.dcache.sets =
			(dcache_size / dc_lsize) / mips_cpu.dcache.ways;
	} else {
	        config1 = read_c0_config1();

		if ((lsize = ((config1 >> 10) & 7)))
		        mips_cpu.dcache.linesz = 2 << lsize;
		else
		        mips_cpu.dcache.linesz= lsize;
		mips_cpu.dcache.sets = 64 << ((config1 >> 13) & 7);
		mips_cpu.dcache.ways = 1 + ((config1 >> 7) & 7);

		dc_lsize = mips_cpu.dcache.linesz;
		dcache_size =
			mips_cpu.dcache.sets * mips_cpu.dcache.ways
			* dc_lsize;
	}
	printk("Primary data cache %dkb, linesize %d bytes (%d ways)\n",
	       dcache_size >> 10, dc_lsize, mips_cpu.dcache.ways);
}

static void __init setup_noscache_funcs(void)
{
	_clear_page = (void *)mips32_clear_page_dc;
	_copy_page = (void *)mips32_copy_page_dc;
	_flush_cache_all = mips32_flush_cache_all_pc;
	___flush_cache_all = mips32_flush_cache_all_pc;
	_flush_cache_mm = mips32_flush_cache_mm_pc;
	_flush_cache_range = mips32_flush_cache_range_pc;
	_flush_cache_page = mips32_flush_cache_page_pc;
	_flush_page_to_ram = mips32_flush_page_to_ram_pc;

	_flush_icache_page = mips32_flush_icache_page;

	_dma_cache_wback_inv = mips32_dma_cache_wback_inv_pc;
	_dma_cache_wback = mips32_dma_cache_wback;
	_dma_cache_inv = mips32_dma_cache_inv_pc;
}

static void __init _change_cachability(u32 cm)
{
	change_c0_config(CONF_CM_CMASK, cm);

	if ((mips_cpu.processor_id & (PRID_COMP_MASK | PRID_IMP_MASK)) ==
	    (PRID_COMP_BROADCOM | PRID_IMP_BCM3302)) {
		cm = read_c0_diag();
		/* Enable icache */
		cm |= (1 << 31);
		/* Enable dcache */
		cm |= (1 << 30);
		write_c0_diag(cm);
	}
}	
static void (*change_cachability)(u32);

void __init ld_mmu_bcm4710(void)
{
	unsigned long config = read_c0_config();

	change_cachability = (void (*)(u32)) KSEG1ADDR((unsigned long)(_change_cachability));
	change_cachability(CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);
	setup_noscache_funcs();

	_flush_cache_sigtramp = mips32_flush_cache_sigtramp;
	_flush_icache_range = mips32_flush_icache_range;	/* Ouch */
	_flush_icache_all = mips32_flush_icache_all;

	__flush_cache_all();
}
