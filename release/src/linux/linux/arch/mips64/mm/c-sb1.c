/*
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 2001 Ralf Baechle (ralf@gnu.org)
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
#include <linux/config.h>
#include <linux/init.h>
#include <asm/mmu_context.h>
#include <asm/bootinfo.h>
#include <asm/cacheops.h>
#include <asm/cpu.h>
#include <asm/uaccess.h>

/* These are probed at ld_mmu time */
static unsigned long icache_size;
static unsigned long dcache_size;

static unsigned long icache_line_size;
static unsigned long dcache_line_size;

static unsigned int icache_index_mask;

static unsigned long icache_assoc;
static unsigned long dcache_assoc;

static unsigned int icache_sets;
static unsigned int dcache_sets;

/*
 * The dcache is fully coherent to the system, with one
 * big caveat:  the instruction stream.  In other words,
 * if we miss in the icache, and have dirty data in the
 * L1 dcache, then we'll go out to memory (or the L2) and
 * get the not-as-recent data.
 *
 * So the only time we have to flush the dcache is when
 * we're flushing the icache.  Since the L2 is fully
 * coherent to everything, including I/O, we never have
 * to flush it
 */

/*
 * Writeback and invalidate the entire dcache
 */
static void sb1_writeback_inv_dcache_all(void)
{
	/*
	 * Register usage:
	 *
	 * $1 - moving cache index
	 * $2 - set count
	 */
	__asm__ __volatile__ (
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     move   $1, %2         \n" /* Start at index 0 */
		"1:   cache  %3, 0($1)      \n" /* Invalidate this index */
		"     daddiu %1, %1, -1     \n" /* Decrement loop count */
		"     bnez   %1, 1b         \n" /* loop test */
		"      daddu   $1, $1, %0    \n" /* Next address */
		".set pop                   \n"
		:
		: "r" (dcache_line_size), "r" (dcache_sets * dcache_assoc),
		  "r" (KSEG0), "i" (Index_Writeback_Inv_D));
}


static inline void __sb1_writeback_inv_dcache_range(unsigned long start,
	unsigned long end)
{
	__asm__ __volatile__ (
	"	.set	push		\n"
	"	.set	noreorder	\n"
	"	.set	noat		\n"
	"	.set	mips4		\n"
	"1:	cache	%3, (0<<13)(%0)	\n" /* Index-WB-inval this address */
	"	cache	%3, (1<<13)(%0)	\n" /* Index-WB-inval this address */
	"	cache	%3, (2<<13)(%0)	\n" /* Index-WB-inval this address */
	"	cache	%3, (3<<13)(%0)	\n" /* Index-WB-inval this address */
	"	xori	$1, %0, 1<<12 	\n"
	"	cache	%3, (0<<13)($1)	\n" /* Index-WB-inval this address */
	"	cache	%3, (1<<13)($1)	\n" /* Index-WB-inval this address */
	"	cache	%3, (2<<13)($1)	\n" /* Index-WB-inval this address */
	"	cache	%3, (3<<13)($1)	\n" /* Index-WB-inval this address */
	"	bne	%0, %1, 1b	\n" /* loop test */
	"	 addu	%0, %0, %2	\n" /* next line */
	"	sync			\n"
	"	.set pop		\n"
	:
	: "r" (start  & ~(dcache_line_size - 1)),
	  "r" ((end - 1) & ~(dcache_line_size - 1)),
	  "r" (dcache_line_size),
	  "i" (Index_Writeback_Inv_D));
}


static inline void local_sb1___flush_icache_all(void)
{
	__asm__ __volatile__ (
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     move   $1, %2         \n" /* Start at index 0 */
		"1:   cache  %3, 0($1)       \n" /* Invalidate this index */
		"     daddiu  %1, %1, -1     \n" /* Decrement loop count */
		"     bnez   %1, 1b         \n" /* loop test */
		"      daddu   $1, $1, %0    \n" /* Next address */
		".set pop                   \n"
		:
		: "r" (icache_line_size), "r" (icache_sets * icache_assoc),
		  "r" (KSEG0), "i" (Index_Invalidate_I));
}

static void local_sb1___flush_cache_all(void)
{
	sb1_writeback_inv_dcache_all();
	local_sb1___flush_icache_all();
}

#ifdef CONFIG_SMP
extern void sb1___flush_cache_all_ipi(void *ignored);
asm("sb1___flush_cache_all_ipi = local_sb1___flush_cache_all");

static void sb1___flush_cache_all(void)
{
	smp_call_function(sb1___flush_cache_all_ipi, 0, 1, 1);
	local_sb1___flush_cache_all();
}
#else
extern void sb1___flush_cache_all(void);
asm("sb1___flush_cache_all = local_sb1___flush_cache_all");
#endif

static void sb1_flush_icache_all(void)
{
	sb1_writeback_inv_dcache_all();
	local_sb1___flush_icache_all();
}

/*
 * When flushing a range in the icache, we have to first writeback
 * the dcache for the same range, so new ifetches will see any
 * data that was dirty in the dcache.
 *
 * The start/end arguments are expected to be Kseg addresses.
 */

static void local_sb1_flush_icache_range(unsigned long start,
	unsigned long end)
{
	/*
	 * Don't do ridiculously large flushes; if it's more than 2x the cache
	 * size, it's probably going to be faster to just flush the whole thing.
	 *
	 * When time permits (Ha!) this x2 factor should be quantified more
	 * formally.
	 */
	if ((end - start) > (icache_size * 2)) {
		sb1_flush_icache_all();
		return;
	}

	__sb1_writeback_inv_dcache_range(start, end);

	__asm__ __volatile__ (
		".set push                  \n"
		".set noreorder             \n"
		".set noat                  \n"
		".set mips4                 \n"
		"     move   $1, %0         \n"
		".align 3                   \n"
		"1:   cache  %3, (0<<13)($1) \n" /* Index-inval this address */
		"     cache  %3, (1<<13)($1) \n" /* Index-inval this address */
		"     cache  %3, (2<<13)($1) \n" /* Index-inval this address */
		"     cache  %3, (3<<13)($1) \n" /* Index-inval this address */
		"     bne    $1, %1, 1b     \n" /* loop test */
		"      addu  $1, $1, %2     \n" /* next line */
		"     sync                  \n"
		".set pop                   \n"
		:
		: "r" (start & ~(icache_line_size - 1)),
		  "r" ((end - 1) & ~(dcache_line_size - 1)),
		  "r" (icache_line_size),
		  "i" (Index_Invalidate_I));
}

#ifdef CONFIG_SMP
struct flush_icache_range_args {
	unsigned long start;
	unsigned long end;
};

static void sb1_flush_icache_range_ipi(void *info)
{
	struct flush_icache_range_args *args = info;

	local_sb1_flush_icache_range(args->start, args->end);
}

void sb1_flush_icache_range(unsigned long start, unsigned long end)
{
	struct flush_icache_range_args args;

	args.start = start;
	args.end = end;
	smp_call_function(sb1_flush_icache_range_ipi, &args, 1, 1);
	local_sb1_flush_icache_range(start, end);
}
#else
void sb1_flush_icache_range(unsigned long start, unsigned long end);
asm("sb1_flush_icache_range = local_sb1_flush_icache_range");
#endif

/*
 * If the page isn't executable, no icache flush is needed
 */
static void sb1_flush_icache_page(struct vm_area_struct *vma,
	struct page *page)
{
	if (!(vma->vm_flags & VM_EXEC)) {
		return;
	}

	/*
	 * We're not sure of the virtual address(es) involved here, so
	 * conservatively flush the entire caches on all processors
	 * (ouch).
	 *
	 * Bumping the ASID may well be cheaper, need to experiment ...
	 */
	sb1___flush_cache_all();
}

/*
 * A signal trampoline must fit into a single cacheline.
 */
static void local_sb1_flush_cache_sigtramp(unsigned long addr)
{
	/*
	 * This routine is called on both cores.  We assume the ASID
	 * has been set up properly, and interrupts are off to prevent
	 * reschedule and TLB changes.
	 */
	__asm__ __volatile__ (
	"	.set	push		\n"
	"	.set	noreorder	\n"
	"	.set	noat		\n"
	"	.set	mips4		\n"
	"	cache	%2, (0<<13)(%0)	\n" /* Index-inval this address */
	"	cache	%2, (1<<13)(%0)	\n" /* Index-inval this address */
	"	cache	%2, (2<<13)(%0)	\n" /* Index-inval this address */
	"	cache	%2, (3<<13)(%0)	\n" /* Index-inval this address */
	"	xor	$1, %0, 1<<12	\n" /* Flip index bit 12	*/
	"	cache	%2, (0<<13)($1)	\n" /* Index-inval this address */
	"	cache	%2, (1<<13)($1)	\n" /* Index-inval this address */
	"	cache	%2, (2<<13)($1)	\n" /* Index-inval this address */
	"	cache	%2, (3<<13)($1)	\n" /* Index-inval this address */
	"	cache	%3, (0<<13)(%0)	\n" /* Index-inval this address */
	"	cache	%3, (1<<13)(%0)	\n" /* Index-inval this address */
	"	cache	%3, (2<<13)(%0)	\n" /* Index-inval this address */
	"	cache	%3, (3<<13)(%0)	\n" /* Index-inval this address */
	"	.set	pop		\n"
	: "=r" (addr)
	: "0" (addr), "i" (Index_Writeback_Inv_D), "i" (Index_Invalidate_I));
}

#ifdef CONFIG_SMP
static void sb1_flush_cache_sigtramp_ipi(void *info)
{
	unsigned long iaddr = (unsigned long) info;
	local_sb1_flush_cache_sigtramp(iaddr);
}

static void sb1_flush_cache_sigtramp(unsigned long addr)
{
	local_sb1_flush_cache_sigtramp(addr);
	smp_call_function(sb1_flush_cache_sigtramp_ipi, (void *) addr, 1, 1);
}
#else
void sb1_flush_cache_sigtramp(unsigned long addr);
asm("sb1_flush_cache_sigtramp = local_sb1_flush_cache_sigtramp");
#endif

/*
 * For executable pages, we need to writeback any local dirty data in
 * the dcache and invalidate the icache
 *
 * XXXKW the dcache flush on the remote core may be argued to
 * be unnecessary.
 */
static void local_sb1_flush_cache_page(struct vm_area_struct *vma,
	unsigned long addr)
{
	if (!(vma->vm_flags & VM_EXEC))
		return;

	addr &= PAGE_MASK;
	local_sb1_flush_icache_range(addr, addr + PAGE_SIZE);
}

#ifdef CONFIG_SMP
struct flush_cache_page_struct
{
	struct vm_area_struct *vma;
	unsigned long page;
};

static void sb1_flush_cache_page_ipi(void *info)
{
	struct flush_cache_page_struct *args =
		(struct flush_cache_page_struct *)info;
	local_sb1_flush_cache_page(args->vma, args->page);
}

static void sb1_flush_cache_page(struct vm_area_struct *vma, unsigned long addr)
{
	struct flush_cache_page_struct args;
	args.vma = vma;
	args.page = addr;
	local_sb1_flush_cache_page(vma, addr);
	smp_call_function(sb1_flush_cache_page_ipi, (void *) &args, 1, 1);
}
#else
void sb1_flush_cache_page(struct vm_area_struct *vma, unsigned long addr);
asm("sb1_flush_cache_page = local_sb1_flush_cache_page");
#endif

/*
 * Anything that just flushes dcache state can be ignored, as we're always
 * coherent in dcache space.  This is just a dummy function that all the
 * nop'ed routines point to
 */
static void sb1_nop(void)
{
}

/*
 *  Cache set values (from the mips64 spec)
 * 0 - 64
 * 1 - 128
 * 2 - 256
 * 3 - 512
 * 4 - 1024
 * 5 - 2048
 * 6 - 4096
 * 7 - Reserved
 */

static unsigned int decode_cache_sets(unsigned int config_field)
{
	if (config_field == 7) {
		/* JDCXXX - Find a graceful way to abort. */
		return 0;
	}
	return (1<<(config_field + 6));
}

/*
 *  Cache line size values (from the mips64 spec)
 * 0 - No cache present.
 * 1 - 4 bytes
 * 2 - 8 bytes
 * 3 - 16 bytes
 * 4 - 32 bytes
 * 5 - 64 bytes
 * 6 - 128 bytes
 * 7 - Reserved
 */

static unsigned int decode_cache_line_size(unsigned int config_field)
{
	if (config_field == 0) {
		return 0;
	} else if (config_field == 7) {
		/* JDCXXX - Find a graceful way to abort. */
		return 0;
	}
	return (1<<(config_field + 1));
}

/*
 * Relevant bits of the config1 register format (from the MIPS32/MIPS64 specs)
 *
 * 24:22 Icache sets per way
 * 21:19 Icache line size
 * 18:16 Icache Associativity
 * 15:13 Dcache sets per way
 * 12:10 Dcache line size
 * 9:7   Dcache Associativity
 */

static __init void probe_cache_sizes(void)
{
	u32 config1;

	config1 = read_c0_config1();
	icache_line_size = decode_cache_line_size((config1 >> 19) & 0x7);
	dcache_line_size = decode_cache_line_size((config1 >> 10) & 0x7);
	icache_sets = decode_cache_sets((config1 >> 22) & 0x7);
	dcache_sets = decode_cache_sets((config1 >> 13) & 0x7);
	icache_assoc = ((config1 >> 16) & 0x7) + 1;
	dcache_assoc = ((config1 >> 7) & 0x7) + 1;
	icache_size = icache_line_size * icache_sets * icache_assoc;
	dcache_size = dcache_line_size * dcache_sets * dcache_assoc;
	icache_index_mask = (icache_sets - 1) * icache_line_size;
}

/*
 * This is called from loadmmu.c.  We have to set up all the
 * memory management function pointers, as well as initialize
 * the caches and tlbs
 */
void ld_mmu_sb1(void)
{
	unsigned long temp;

#ifdef CONFIG_SB1_CACHE_ERROR
	/* Special cache error handler for SB1 */
	extern char except_vec2_sb1;

	memcpy((void *)(KSEG0 + 0x100), &except_vec2_sb1, 0x80);
	memcpy((void *)(KSEG1 + 0x100), &except_vec2_sb1, 0x80);
#endif

	probe_cache_sizes();

	_clear_page = sb1_clear_page;
	_copy_page = sb1_copy_page;

	/* None of these are needed for the sb1 */
	_flush_cache_mm = (void (*)(struct mm_struct *))sb1_nop;
	_flush_cache_range = (void *) sb1_nop;
	_flush_cache_all = sb1_nop;

	/*
	 * "flush_page_to_ram" is expected to prevent virtual aliasing
	 * in the Dcache, and is called before a new mapping for a
	 * page is about the be installed.  Since our Dcache is
	 * physically indexed and tagged, there can't be aliasing.  If
	 * coherence with I-stream is needed, an icache will be used
	 * -- so we don't have to do any flushing.
	 */
	_flush_page_to_ram = (void (*)(struct page *)) sb1_nop;

	___flush_cache_all = sb1___flush_cache_all;
	_flush_cache_l1 = sb1___flush_cache_all;
	_flush_icache_page = sb1_flush_icache_page;
	_flush_icache_range = sb1_flush_icache_range;
	_flush_cache_page = sb1_flush_cache_page;
	_flush_cache_sigtramp = sb1_flush_cache_sigtramp;
	_flush_icache_all = sb1_flush_icache_all;

	change_c0_config(CONF_CM_CMASK, CONF_CM_DEFAULT);
	/*
	 * This is the only way to force the update of K0 to complete
	 * before subsequent instruction fetch.
	 */
	__asm__ __volatile__ (
	"	.set	push		\n"
	"	.set	mips4		\n"
	"	dla	%0, 1f		\n"
	"	dmtc0	%0, $14		\n"
	"	eret			\n"
	"1:	.set	pop		\n"
	: "=r" (temp));
	flush_cache_all();
}
