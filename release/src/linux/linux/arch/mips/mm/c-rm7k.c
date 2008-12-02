/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * r4xx0.c: R4000 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1998 Ralf Baechle ralf@gnu.org
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

#include <asm/io.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <asm/mmu_context.h>

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */

#define ic_lsize	32		/* Fixed to 32 byte on RM7000  */
#define dc_lsize	32		/* Fixed to 32 byte on RM7000  */
#define sc_lsize	32		/* Fixed to 32 byte on RM7000  */
#define tc_pagesize	(32*128)

/* Secondary cache parameters. */
#define scache_size	(256*1024)	/* Fixed to 256KiB on RM7000 */

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

int rm7k_tcache_enabled = 0;

/*
 * Not added to asm/r4kcache.h because it seems to be RM7000-specific.
 */
#define Page_Invalidate_T 0x16

static inline void invalidate_tcache_page(unsigned long addr)
{
	__asm__ __volatile__(
		".set\tnoreorder\t\t\t# invalidate_tcache_page\n\t"
		".set\tmips3\n\t"
		"cache\t%1, (%0)\n\t"
		".set\tmips0\n\t"
		".set\treorder"
		:
		: "r" (addr),
		  "i" (Page_Invalidate_T));
}

static void __flush_cache_all_d32i32(void)
{
	blast_dcache32();
	blast_icache32();
}

static inline void rm7k_flush_cache_all_d32i32(void)
{
	/* Yes! Caches that don't suck ...  */
}

static void rm7k_flush_cache_range_d32i32(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	/* RM7000 caches are sane ...  */
}

static void rm7k_flush_cache_mm_d32i32(struct mm_struct *mm)
{
	/* RM7000 caches are sane ...  */
}

static void rm7k_flush_cache_page_d32i32(struct vm_area_struct *vma,
					unsigned long page)
{
	/* RM7000 caches are sane ...  */
}

static void rm7k_flush_page_to_ram_d32i32(struct page * page)
{
	/* Yes!  Caches that don't suck!  */
}

static void rm7k_flush_icache_range(unsigned long start, unsigned long end)
{
	__flush_cache_all_d32i32();
}

static void rm7k_flush_icache_page(struct vm_area_struct *vma,
                                   struct page *page)
{
	__flush_cache_all_d32i32();
}


static void
rm7k_dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	a = addr & ~(sc_lsize - 1);
	end = (addr + size - 1) & ~(sc_lsize - 1);
	while (1) {
		flush_dcache_line(a);	/* Hit_Writeback_Inv_D */
		flush_scache_line(a);	/* Hit_Writeback_Inv_SD */
		if (a == end) break;
		a += sc_lsize;
	}

	if (!rm7k_tcache_enabled)
		return;

	a = addr & ~(tc_pagesize - 1);
	end = (addr + size - 1) & ~(tc_pagesize - 1);
	while(1) {
		invalidate_tcache_page(a);	/* Page_Invalidate_T */
		if (a == end) break;
		a += tc_pagesize;
	}
}

static void
rm7k_dma_cache_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	a = addr & ~(sc_lsize - 1);
	end = (addr + size - 1) & ~(sc_lsize - 1);
	while (1) {
		invalidate_dcache_line(a);	/* Hit_Invalidate_D */
		invalidate_scache_line(a);	/* Hit_Invalidate_SD */
		if (a == end) break;
		a += sc_lsize;
	}

	if (!rm7k_tcache_enabled)
		return;

	a = addr & ~(tc_pagesize - 1);
	end = (addr + size - 1) & ~(tc_pagesize - 1);
	while(1) {
		invalidate_tcache_page(a);	/* Page_Invalidate_T */
		if (a == end) break;
		a += tc_pagesize;
	}
}

static void
rm7k_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("rm7k_dma_cache_wback called - should not happen.");
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void rm7k_flush_cache_sigtramp(unsigned long addr)
{
	protected_writeback_dcache_line(addr & ~(dc_lsize - 1));
	protected_flush_icache_line(addr & ~(ic_lsize - 1));
}

/* Detect and size the caches. */
static inline void probe_icache(unsigned long config)
{
	icache_size = 1 << (12 + ((config >> 9) & 7));

	printk(KERN_INFO "Primary instruction cache %dKiB.\n", icache_size >> 10);
}

static inline void probe_dcache(unsigned long config)
{
	dcache_size = 1 << (12 + ((config >> 6) & 7));

	printk(KERN_INFO "Primary data cache %dKiB.\n", dcache_size >> 10);
}


/*
 * This function is executed in the uncached segment KSEG1.
 * It must not touch the stack, because the stack pointer still points
 * into KSEG0.
 *
 * Three options:
 *	- Write it in assembly and guarantee that we don't use the stack.
 *	- Disable caching for KSEG0 before calling it.
 *	- Pray that GCC doesn't randomly start using the stack.
 *
 * This being Linux, we obviously take the least sane of those options -
 * following DaveM's lead in r4xx0.c
 *
 * It seems we get our kicks from relying on unguaranteed behaviour in GCC
 */
static __init void setup_scache(void)
{
	int register i;

	set_c0_config(1<<3 /* CONF_SE */);

	write_c0_taglo(0);
	write_c0_taghi(0);

	for (i=0; i<scache_size; i+=sc_lsize) {
		__asm__ __volatile__ (
		      ".set noreorder\n\t"
		      ".set mips3\n\t"
		      "cache %1, (%0)\n\t"
		      ".set mips0\n\t"
		      ".set reorder"
		      :
		      : "r" (KSEG0ADDR(i)),
		        "i" (Index_Store_Tag_SD));
	}

}

static inline void probe_scache(unsigned long config)
{
	void (*func)(void) = KSEG1ADDR(&setup_scache);

	if ((config >> 31) & 1)
		return;

	printk(KERN_INFO "Secondary cache %dKiB, linesize %d bytes.\n",
	       (scache_size >> 10), sc_lsize);

	if ((config >> 3) & 1)
		return;

	printk(KERN_INFO "Enabling secondary cache...");
	func();
	printk("Done\n");
}

static inline void probe_tcache(unsigned long config)
{
	if ((config >> 17) & 1)
		return;

	/* We can't enable the L3 cache yet. There may be board-specific
	 * magic necessary to turn it on, and blindly asking the CPU to
	 * start using it would may give cache errors.
	 *
	 * Also, board-specific knowledge may allow us to use the
	 * CACHE Flash_Invalidate_T instruction if the tag RAM supports
	 * it, and may specify the size of the L3 cache so we don't have
	 * to probe it.
	 */
	printk(KERN_INFO "Tertiary cache present, %s enabled\n",
	       config&(1<<12) ? "already" : "not (yet)");

	if ((config >> 12) & 1)
		rm7k_tcache_enabled = 1;
}

void __init ld_mmu_rm7k(void)
{
	unsigned long config = read_c0_config();
	unsigned long addr;

        change_c0_config(CONF_CM_CMASK, CONF_CM_UNCACHED);

	/* RM7000 erratum #31. The icache is screwed at startup. */
	write_c0_taglo(0);
	write_c0_taghi(0);
	for (addr = KSEG0; addr <= KSEG0 + 4096; addr += ic_lsize) {
		__asm__ __volatile__ (
			".set noreorder\n\t"
			".set mips3\n\t"
			"cache\t%1, 0(%0)\n\t"
			"cache\t%1, 0x1000(%0)\n\t"
			"cache\t%1, 0x2000(%0)\n\t"
			"cache\t%1, 0x3000(%0)\n\t"
			"cache\t%2, 0(%0)\n\t"
			"cache\t%2, 0x1000(%0)\n\t"
			"cache\t%2, 0x2000(%0)\n\t"
			"cache\t%2, 0x3000(%0)\n\t"
			"cache\t%1, 0(%0)\n\t"
			"cache\t%1, 0x1000(%0)\n\t"
			"cache\t%1, 0x2000(%0)\n\t"
			"cache\t%1, 0x3000(%0)\n\t"
			".set\tmips0\n\t"
			".set\treorder\n\t"
			:
			: "r" (addr), "i" (Index_Store_Tag_I), "i" (Fill));
	}

	change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);

	probe_icache(config);
	probe_dcache(config);
	probe_scache(config);
	probe_tcache(config);

	_clear_page = rm7k_clear_page;
	_copy_page = rm7k_copy_page;

	_flush_cache_all = rm7k_flush_cache_all_d32i32;
	___flush_cache_all = __flush_cache_all_d32i32;
	_flush_cache_mm = rm7k_flush_cache_mm_d32i32;
	_flush_cache_range = rm7k_flush_cache_range_d32i32;
	_flush_cache_page = rm7k_flush_cache_page_d32i32;
	_flush_page_to_ram = rm7k_flush_page_to_ram_d32i32;
	_flush_cache_sigtramp = rm7k_flush_cache_sigtramp;
	_flush_icache_range = rm7k_flush_icache_range;
	_flush_icache_page = rm7k_flush_icache_page;

	_dma_cache_wback_inv = rm7k_dma_cache_wback_inv;
	_dma_cache_wback = rm7k_dma_cache_wback;
	_dma_cache_inv = rm7k_dma_cache_inv;

	__flush_cache_all_d32i32();
}
