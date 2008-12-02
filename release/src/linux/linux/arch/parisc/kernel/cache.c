/* $Id: cache.c,v 1.1.1.4 2003/10/14 08:07:37 sparq Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1999 Helge Deller (07-13-1999)
 * Copyright (C) 1999 SuSE GmbH Nuernberg
 * Copyright (C) 2000 Philipp Rumpf (prumpf@tux.org)
 *
 * Cache and TLB management
 *
 */
 
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/seq_file.h>

#include <asm/pdc.h>
#include <asm/cache.h>
#include <asm/system.h>
#include <asm/page.h>
#include <asm/pgalloc.h>
#include <asm/processor.h>

int split_tlb;
int dcache_stride;
int icache_stride;

struct pdc_cache_info cache_info;
#ifndef CONFIG_PA20
static struct pdc_btlb_info btlb_info;
#endif

#ifdef CONFIG_SMP
void
flush_data_cache(void)
{
	smp_call_function((void (*)(void *))flush_data_cache_local, NULL, 1, 1);
	flush_data_cache_local();
}
#endif

void
flush_cache_all_local(void)
{
	flush_instruction_cache_local();
	flush_data_cache_local();
}

/* flushes EVERYTHING (tlb & cache) */

void
flush_all_caches(void)
{
	flush_cache_all();
	flush_tlb_all();
}

void
update_mmu_cache(struct vm_area_struct *vma, unsigned long address, pte_t pte)
{
	struct page *page = pte_page(pte);

	if (VALID_PAGE(page) && page->mapping &&
	    test_bit(PG_dcache_dirty, &page->flags)) {

		flush_kernel_dcache_page(page_address(page));
		clear_bit(PG_dcache_dirty, &page->flags);
	}
}

void
show_cache_info(struct seq_file *m)
{
	seq_printf(m, "I-cache\t\t: %ld KB\n", 
		cache_info.ic_size/1024 );
	seq_printf(m, "D-cache\t\t: %ld KB (%s)%s\n", 
		cache_info.dc_size/1024,
		(cache_info.dc_conf.cc_wt ? "WT":"WB"),
		(cache_info.dc_conf.cc_sh ? " - shared I/D":"")
	);

	seq_printf(m, "ITLB entries\t: %ld\n" "DTLB entries\t: %ld%s\n",
		cache_info.it_size,
		cache_info.dt_size,
		cache_info.dt_conf.tc_sh ? " - shared with ITLB":""
	);
		
#ifndef CONFIG_PA20
	/* BTLB - Block TLB */
	if (btlb_info.max_size==0) {
		seq_printf(m, "BTLB\t\t: not supported\n" );
	} else {
		seq_printf(m, 
		"BTLB fixed\t: max. %d pages, pagesize=%d (%dMB)\n"
		"BTLB fix-entr.\t: %d instruction, %d data (%d combined)\n"
		"BTLB var-entr.\t: %d instruction, %d data (%d combined)\n",
		btlb_info.max_size, (int)4096,
		btlb_info.max_size>>8,
		btlb_info.fixed_range_info.num_i,
		btlb_info.fixed_range_info.num_d,
		btlb_info.fixed_range_info.num_comb, 
		btlb_info.variable_range_info.num_i,
		btlb_info.variable_range_info.num_d,
		btlb_info.variable_range_info.num_comb
		);
	}
#endif
}

void __init 
cache_init(void)
{
	if(pdc_cache_info(&cache_info)<0)
		panic("cache_init: pdc_cache_info failed");


	split_tlb = 0;
	if (cache_info.dt_conf.tc_sh == 0 || cache_info.dt_conf.tc_sh == 2) {

	    if (cache_info.dt_conf.tc_sh == 2)
		printk(KERN_WARNING "Unexpected TLB configuration. "
			"Will flush I/D separately (could be optimized).\n");

	    split_tlb = 1;
	}

	dcache_stride = ( (1<<(cache_info.dc_conf.cc_block+3)) *
			 cache_info.dc_conf.cc_line );
	icache_stride = ( (1<<(cache_info.ic_conf.cc_block+3)) *
			 cache_info.ic_conf.cc_line );
#ifndef CONFIG_PA20
	if(pdc_btlb_info(&btlb_info)<0) {
		memset(&btlb_info, 0, sizeof btlb_info);
	}
#endif

	if ((boot_cpu_data.pdc.capabilities & PDC_MODEL_NVA_MASK) == PDC_MODEL_NVA_UNSUPPORTED) {
		printk(KERN_WARNING "Only equivalent aliasing supported\n");
#ifndef CONFIG_SMP
		panic("SMP kernel required to avoid non-equivalent aliasing");
#endif
	}
}

void disable_sr_hashing(void)
{
    int srhash_type;

    if (boot_cpu_data.cpu_type == pcxl2)
	return; /* pcxl2 doesn't support space register hashing */

    switch (boot_cpu_data.cpu_type) {

    case pcx:
	BUG(); /* We shouldn't get here. code in setup.c should prevent it */
	return;

    case pcxs:
    case pcxt:
    case pcxt_:
	srhash_type = SRHASH_PCXST;
	break;

    case pcxl:
	srhash_type = SRHASH_PCXL;
	break;

    default: /* Currently all PA2.0 machines use the same ins. sequence */
	srhash_type = SRHASH_PA20;
	break;
    }

    disable_sr_hashing_asm(srhash_type);
}
