/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * r5432.c: NEC Vr5432 processor.  We cannot use r4xx0.c because of
 *      its unique way-selection method for indexed operations.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998, 1999, 2000 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 2000 Jun Sun (jsun@mvista.com)
 *
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/bcache.h>
#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/mmu_context.h>

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

#include <asm/asm.h>
#include <asm/cacheops.h>

#undef DEBUG_CACHE

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */
static int ic_lsize, dc_lsize;       /* LineSize in bytes */


/* -------------------------------------------------------------------- */
/* #include <asm/r4kcache.h> */

static inline void flush_icache_line_indexed(unsigned long addr)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n\t"
		"cache %1, (%0)\n\t"
		"cache %1, 1(%0)\n\t"
		".set mips0\n\t"
		".set reorder"
		:
		: "r" (addr),
		  "i" (Index_Invalidate_I));
}

static inline void flush_dcache_line_indexed(unsigned long addr)
{
	__asm__ __volatile__(
		".set noreorder\n\t"
		".set mips3\n\t"
		"cache %1, (%0)\n\t"
		"cache %1, 1(%0)\n\t"
		".set mips0\n\t"
		".set reorder"
		:
		: "r" (addr),
		  "i" (Index_Writeback_Inv_D));
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


#define cache32_unroll32(base,op)				\
	__asm__ __volatile__("					\
		.set noreorder;					\
		.set mips3;					\
		cache %1, 0x000(%0); cache %1, 0x020(%0);	\
		cache %1, 0x040(%0); cache %1, 0x060(%0);	\
		cache %1, 0x080(%0); cache %1, 0x0a0(%0);	\
		cache %1, 0x0c0(%0); cache %1, 0x0e0(%0);	\
		cache %1, 0x100(%0); cache %1, 0x120(%0);	\
		cache %1, 0x140(%0); cache %1, 0x160(%0);	\
		cache %1, 0x180(%0); cache %1, 0x1a0(%0);	\
		cache %1, 0x1c0(%0); cache %1, 0x1e0(%0);	\
		cache %1, 0x200(%0); cache %1, 0x220(%0);	\
		cache %1, 0x240(%0); cache %1, 0x260(%0);	\
		cache %1, 0x280(%0); cache %1, 0x2a0(%0);	\
		cache %1, 0x2c0(%0); cache %1, 0x2e0(%0);	\
		cache %1, 0x300(%0); cache %1, 0x320(%0);	\
		cache %1, 0x340(%0); cache %1, 0x360(%0);	\
		cache %1, 0x380(%0); cache %1, 0x3a0(%0);	\
		cache %1, 0x3c0(%0); cache %1, 0x3e0(%0);	\
		.set mips0;					\
		.set reorder"					\
		:						\
		: "r" (base),					\
		  "i" (op));

static inline void blast_dcache32(void)
{
	unsigned long start = KSEG0;
	unsigned long end = (start + dcache_size/2);

	while(start < end) {
		cache32_unroll32(start,Index_Writeback_Inv_D);
		cache32_unroll32(start+1,Index_Writeback_Inv_D);
		start += 0x400;
	}
}

static inline void blast_dcache32_page(unsigned long page)
{
	unsigned long start = page;
	unsigned long end = (start + PAGE_SIZE);

	while(start < end) {
		cache32_unroll32(start,Hit_Writeback_Inv_D);
		start += 0x400;
	}
}

static inline void blast_dcache32_page_indexed(unsigned long page)
{
	unsigned long start = page;
	unsigned long end = (start + PAGE_SIZE);

	while(start < end) {
		cache32_unroll32(start,Index_Writeback_Inv_D);
		cache32_unroll32(start+1,Index_Writeback_Inv_D);
		start += 0x400;
	}
}

static inline void blast_icache32(void)
{
	unsigned long start = KSEG0;
	unsigned long end = (start + icache_size/2);

	while(start < end) {
		cache32_unroll32(start,Index_Invalidate_I);
		cache32_unroll32(start+1,Index_Invalidate_I);
		start += 0x400;
	}
}

static inline void blast_icache32_page(unsigned long page)
{
	unsigned long start = page;
	unsigned long end = (start + PAGE_SIZE);

	while(start < end) {
		cache32_unroll32(start,Hit_Invalidate_I);
		start += 0x400;
	}
}

static inline void blast_icache32_page_indexed(unsigned long page)
{
	unsigned long start = page;
	unsigned long end = (start + PAGE_SIZE);

	while(start < end) {
		cache32_unroll32(start,Index_Invalidate_I);
		cache32_unroll32(start+1,Index_Invalidate_I);
		start += 0x400;
	}
}

/* -------------------------------------------------------------------- */

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

static inline void r5432_flush_cache_all_d32i32(void)
{
	blast_dcache32(); blast_icache32();
}

static void r5432_flush_cache_range_d32i32(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	if (mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		blast_dcache32(); blast_icache32();
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void r5432_flush_cache_mm_d32i32(struct mm_struct *mm)
{
	if (mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r5432_flush_cache_all_d32i32();
	}
}

static void r5432_flush_cache_page_d32i32(struct vm_area_struct *vma,
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
		blast_dcache32_page_indexed(page);
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
static void r5432_flush_page_to_ram_d32(struct page *page)
{
	blast_dcache32_page((unsigned long)page_address(page));
}

static void
r5432_flush_icache_range(unsigned long start, unsigned long end)
{
	r5432_flush_cache_all_d32i32();
}

/*
 * Ok, this seriously sucks.  We use them to flush a user page but don't
 * know the virtual address, so we have to blast away the whole icache
 * which is significantly more expensive than the real thing.
 */
static void
r5432_flush_icache_page_i32(struct vm_area_struct *vma, struct page *page)
{
	if (!(vma->vm_flags & VM_EXEC))
		return;

	r5432_flush_cache_all_d32i32();
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 */
static void
r5432_dma_cache_wback_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		flush_cache_all();
	} else {
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
	}
	bc_wback_inv(addr, size);
}

static void
r5432_dma_cache_inv_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (size >= dcache_size) {
		flush_cache_all();
	} else {
		a = addr & ~(dc_lsize - 1);
		end = (addr + size - 1) & ~(dc_lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += dc_lsize;
		}
	}

	bc_inv(addr, size);
}

static void
r5432_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("r5432_dma_cache called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void r5432_flush_cache_sigtramp(unsigned long addr)
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


void __init ld_mmu_r5432(void)
{
	unsigned long config = read_c0_config();

	change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);

	_clear_page = r5432_clear_page_d32;
	_copy_page = r5432_copy_page_d32;
	_flush_cache_all = r5432_flush_cache_all_d32i32;
	___flush_cache_all = r5432_flush_cache_all_d32i32;
	_flush_page_to_ram = r5432_flush_page_to_ram_d32;
	_flush_cache_mm = r5432_flush_cache_mm_d32i32;
	_flush_cache_range = r5432_flush_cache_range_d32i32;
	_flush_cache_page = r5432_flush_cache_page_d32i32;
	_flush_icache_page = r5432_flush_icache_page_i32;
	_dma_cache_wback_inv = r5432_dma_cache_wback_inv_pc;
	_dma_cache_wback = r5432_dma_cache_wback;
	_dma_cache_inv = r5432_dma_cache_inv_pc;

	_flush_cache_sigtramp = r5432_flush_cache_sigtramp;
	_flush_icache_range = r5432_flush_icache_range;	/* Ouch */

	__flush_cache_all();
}
