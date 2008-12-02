

#include <linux/config.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ptrace.h>
#include <linux/mman.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/smp.h>
#include <linux/bootmem.h>

#include <asm/system.h>
#include <asm/segment.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/dma.h>
#include <asm/svinto.h>
#include <asm/io.h>
#include <asm/mmu_context.h>

static unsigned long totalram_pages;

struct pgtable_cache_struct quicklists;  /* see asm/pgalloc.h */

const char bad_pmd_string[] = "Bad pmd in pte_alloc: %08lx\n";

extern void die_if_kernel(char *,struct pt_regs *,long);
extern void show_net_buffers(void);
extern void tlb_init(void);


unsigned long empty_zero_page;

/* trim the page-table cache if necessary */

int 
do_check_pgt_cache(int low, int high)
{
        int freed = 0;

        if(pgtable_cache_size > high) {
                do {
                        if(pgd_quicklist) {
                                free_pgd_slow(get_pgd_fast());
                                freed++;
                        }
                        if(pmd_quicklist) {
                                pmd_free_slow(pmd_alloc_one_fast(NULL, 0));
                                freed++;
                        }
                        if(pte_quicklist) {
                                pte_free_slow(pte_alloc_one_fast(NULL, 0));
				freed++;
                        }
                } while(pgtable_cache_size > low);
        }
        return freed;
}

void 
show_mem(void)
{
	int i,free = 0,total = 0,cached = 0, reserved = 0, nonshared = 0;
	int shared = 0;

	printk("\nMem-info:\n");
	show_free_areas();
	printk("Free swap:       %6dkB\n",nr_swap_pages<<(PAGE_SHIFT-10));
	i = max_mapnr;
	while (i-- > 0) {
		total++;
		if (PageReserved(mem_map+i))
			reserved++;
		else if (PageSwapCache(mem_map+i))
			cached++;
		else if (!page_count(mem_map+i))
			free++;
		else if (page_count(mem_map+i) == 1)
			nonshared++;
		else
			shared += page_count(mem_map+i) - 1;
	}
	printk("%d pages of RAM\n",total);
	printk("%d free pages\n",free);
	printk("%d reserved pages\n",reserved);
	printk("%d pages nonshared\n",nonshared);
	printk("%d pages shared\n",shared);
	printk("%d pages swap cached\n",cached);
	printk("%ld pages in page table cache\n",pgtable_cache_size);
	show_buffers();
}

/*
 * The kernel is already mapped with a kernel segment at kseg_c so 
 * we don't need to map it with a page table. However head.S also
 * temporarily mapped it at kseg_4 so we should set up the ksegs again,
 * clear the TLB and do some other paging setup stuff.
 */

void __init 
paging_init(void)
{
	int i;
	unsigned long zones_size[MAX_NR_ZONES];

	printk("Setting up paging and the MMU.\n");
	
	/* clear out the init_mm.pgd that will contain the kernel's mappings */

	for(i = 0; i < PTRS_PER_PGD; i++)
		swapper_pg_dir[i] = __pgd(0);
	
	/* make sure the current pgd table points to something sane
	 * (even if it is most probably not used until the next 
	 *  switch_mm)
	 */

	current_pgd = init_mm.pgd;

	/* initialise the TLB (tlb.c) */

	tlb_init();

	/* see README.mm for details on the KSEG setup */

#ifndef CONFIG_CRIS_LOW_MAP
	/* This code is for the corrected Etrax-100 LX version 2... */

#define CACHED_BOOTROM (KSEG_A | 0x08000000UL)

	*R_MMU_KSEG = ( IO_STATE(R_MMU_KSEG, seg_f, seg  ) | /* cached flash */
			IO_STATE(R_MMU_KSEG, seg_e, seg  ) | /* uncached flash */
			IO_STATE(R_MMU_KSEG, seg_d, page ) | /* vmalloc area */
			IO_STATE(R_MMU_KSEG, seg_c, seg  ) | /* kernel area */
			IO_STATE(R_MMU_KSEG, seg_b, seg  ) | /* kernel reg area */
			IO_STATE(R_MMU_KSEG, seg_a, seg  ) | /* bootrom/regs cached */ 
			IO_STATE(R_MMU_KSEG, seg_9, page ) | /* user area */
			IO_STATE(R_MMU_KSEG, seg_8, page ) |
			IO_STATE(R_MMU_KSEG, seg_7, page ) |
			IO_STATE(R_MMU_KSEG, seg_6, page ) |
			IO_STATE(R_MMU_KSEG, seg_5, page ) |
			IO_STATE(R_MMU_KSEG, seg_4, page ) |
			IO_STATE(R_MMU_KSEG, seg_3, page ) |
			IO_STATE(R_MMU_KSEG, seg_2, page ) |
			IO_STATE(R_MMU_KSEG, seg_1, page ) |
			IO_STATE(R_MMU_KSEG, seg_0, page ) );

	*R_MMU_KBASE_HI = ( IO_FIELD(R_MMU_KBASE_HI, base_f, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_e, 0x8 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_d, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_c, 0x4 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_b, 0xb ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_a, 0x3 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_9, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_8, 0x0 ) );
	
	*R_MMU_KBASE_LO = ( IO_FIELD(R_MMU_KBASE_LO, base_7, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_6, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_5, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_4, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_3, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_2, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_1, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_0, 0x0 ) );
#else
	/* Etrax-100 LX version 1 has a bug so that we cannot map anything
	 * across the 0x80000000 boundary, so we need to shrink the user-virtual
	 * area to 0x50000000 instead of 0xb0000000 and map things slightly
	 * different. The unused areas are marked as paged so that we can catch
	 * freak kernel accesses there.
	 *
	 * The ARTPEC chip is mapped at 0xa so we pass that segment straight
	 * through. We cannot vremap it because the vmalloc area is below 0x8
	 * and Juliette needs an uncached area above 0x8.
	 *
	 * Same thing with 0xc and 0x9, which is memory-mapped I/O on some boards.
	 * We map them straight over in LOW_MAP, but use vremap in LX version 2.
	 */

#define CACHED_BOOTROM (KSEG_F | 0x08000000UL)

	*R_MMU_KSEG = ( IO_STATE(R_MMU_KSEG, seg_f, seg  ) |  /* cached bootrom/regs */ 
			IO_STATE(R_MMU_KSEG, seg_e, page ) |
			IO_STATE(R_MMU_KSEG, seg_d, page ) | 
			IO_STATE(R_MMU_KSEG, seg_c, page ) |   
			IO_STATE(R_MMU_KSEG, seg_b, seg  ) |  /* kernel reg area */
#ifdef CONFIG_JULIETTE
			IO_STATE(R_MMU_KSEG, seg_a, seg  ) |  /* ARTPEC etc. */
#else
			IO_STATE(R_MMU_KSEG, seg_a, page ) |
#endif
			IO_STATE(R_MMU_KSEG, seg_9, seg  ) |  /* LED's on some boards */
			IO_STATE(R_MMU_KSEG, seg_8, seg  ) |  /* CSE0/1, flash and I/O */
			IO_STATE(R_MMU_KSEG, seg_7, page ) |  /* kernel vmalloc area */
			IO_STATE(R_MMU_KSEG, seg_6, seg  ) |  /* kernel DRAM area */
			IO_STATE(R_MMU_KSEG, seg_5, seg  ) |  /* cached flash */
			IO_STATE(R_MMU_KSEG, seg_4, page ) |  /* user area */
			IO_STATE(R_MMU_KSEG, seg_3, page ) |  /* user area */
			IO_STATE(R_MMU_KSEG, seg_2, page ) |  /* user area */
			IO_STATE(R_MMU_KSEG, seg_1, page ) |  /* user area */
			IO_STATE(R_MMU_KSEG, seg_0, page ) ); /* user area */

	*R_MMU_KBASE_HI = ( IO_FIELD(R_MMU_KBASE_HI, base_f, 0x3 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_e, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_d, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_c, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_b, 0xb ) |
#ifdef CONFIG_JULIETTE
			    IO_FIELD(R_MMU_KBASE_HI, base_a, 0xa ) |
#else
			    IO_FIELD(R_MMU_KBASE_HI, base_a, 0x0 ) |
#endif
			    IO_FIELD(R_MMU_KBASE_HI, base_9, 0x9 ) |
			    IO_FIELD(R_MMU_KBASE_HI, base_8, 0x8 ) );
	
	*R_MMU_KBASE_LO = ( IO_FIELD(R_MMU_KBASE_LO, base_7, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_6, 0x4 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_5, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_4, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_3, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_2, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_1, 0x0 ) |
			    IO_FIELD(R_MMU_KBASE_LO, base_0, 0x0 ) );
#endif

	*R_MMU_CONTEXT = ( IO_FIELD(R_MMU_CONTEXT, page_id, 0 ) );
	
	/* The MMU has been enabled ever since head.S but just to make
	 * it totally obvious we do it here as well.
	 */

	*R_MMU_CTRL = ( IO_STATE(R_MMU_CTRL, inv_excp, enable ) |
			IO_STATE(R_MMU_CTRL, acc_excp, enable ) |
			IO_STATE(R_MMU_CTRL, we_excp,  enable ) );
	
	*R_MMU_ENABLE = IO_STATE(R_MMU_ENABLE, mmu_enable, enable);

	/*
	 * initialize the bad page table and bad page to point
	 * to a couple of allocated pages
	 */

	empty_zero_page = (unsigned long)alloc_bootmem_pages(PAGE_SIZE);
	memset((void *)empty_zero_page, 0, PAGE_SIZE);

	/* All pages are DMA'able in Etrax, so put all in the DMA'able zone */

	zones_size[0] = ((unsigned long)high_memory - PAGE_OFFSET) >> PAGE_SHIFT;

	for (i = 1; i < MAX_NR_ZONES; i++)
		zones_size[i] = 0;

	/* Use free_area_init_node instead of free_area_init, because the former
	 * is designed for systems where the DRAM starts at an address substantially
	 * higher than 0, like us (we start at PAGE_OFFSET). This saves space in the
	 * mem_map page array.
	 */

	free_area_init_node(0, 0, 0, zones_size, PAGE_OFFSET, 0);

}

extern unsigned long loops_per_jiffy; /* init/main.c */
unsigned long loops_per_usec;

extern char _stext, _edata, _etext;
extern char __init_begin, __init_end;

void __init
mem_init(void)
{
	int codesize, reservedpages, datasize, initsize;
	unsigned long tmp;

	if(!mem_map)
		BUG();

	/* max/min_low_pfn was set by setup.c
	 * now we just copy it to some other necessary places...
	 *
	 * high_memory was also set in setup.c
	 */

	max_mapnr = num_physpages = max_low_pfn - min_low_pfn;
 
	/* this will put all memory onto the freelists */
        totalram_pages = free_all_bootmem();

	reservedpages = 0;
	for (tmp = 0; tmp < max_mapnr; tmp++) {
		/*
                 * Only count reserved RAM pages
                 */
		if (PageReserved(mem_map + tmp))
			reservedpages++;
	}

	codesize =  (unsigned long) &_etext - (unsigned long) &_stext;
        datasize =  (unsigned long) &_edata - (unsigned long) &_etext;
        initsize =  (unsigned long) &__init_end - (unsigned long) &__init_begin;
	
        printk("Memory: %luk/%luk available (%dk kernel code, %dk reserved, %dk data, "
	       "%dk init)\n" ,
	       (unsigned long) nr_free_pages() << (PAGE_SHIFT-10),
	       max_mapnr << (PAGE_SHIFT-10),
	       codesize >> 10,
	       reservedpages << (PAGE_SHIFT-10),
	       datasize >> 10,
	       initsize >> 10
               );

	/* HACK alert - calculate a loops_per_usec for asm/delay.h here
	 * since this is called just after calibrate_delay in init/main.c
	 * but before places which use udelay. cannot be in time.c since
	 * that is called _before_ calibrate_delay
	 */

	loops_per_usec = (loops_per_jiffy * HZ) / 1000000;

	return;
}

/* Initialize remaps of some I/O-ports. This is designed to be callable
 * multiple times from the drivers init-sections, because we don't know
 * beforehand which driver will get initialized first.
 */

void 
init_ioremap(void)
{
  
	/* Give the external I/O-port addresses their values */

        static int initialized = 0;
  
        if( !initialized ) {
                initialized++;
            
#ifdef CONFIG_CRIS_LOW_MAP
               /* Simply a linear map (see the KSEG map above in paging_init) */
               port_cse1_addr = (volatile unsigned long *)(MEM_CSE1_START | 
                                                           MEM_NON_CACHEABLE);
               port_csp0_addr = (volatile unsigned long *)(MEM_CSP0_START |
                                                           MEM_NON_CACHEABLE);
               port_csp4_addr = (volatile unsigned long *)(MEM_CSP4_START |
                                                           MEM_NON_CACHEABLE);
#else
               /* Note that nothing blows up just because we do this remapping 
                * it's ok even if the ports are not used or connected 
                * to anything (or connected to a non-I/O thing) */        
               port_cse1_addr = (volatile unsigned long *)
                 ioremap((unsigned long)(MEM_CSE1_START | 
                                         MEM_NON_CACHEABLE), 16);
               port_csp0_addr = (volatile unsigned long *)
                 ioremap((unsigned long)(MEM_CSP0_START |
                                         MEM_NON_CACHEABLE), 16);
               port_csp4_addr = (volatile unsigned long *)
                 ioremap((unsigned long)(MEM_CSP4_START |
                                         MEM_NON_CACHEABLE), 16);
#endif	
        }
}

/* Helper function for the two below */

static inline void
flush_etrax_cacherange(void *startadr, int length)
{
	/* CACHED_BOOTROM is mapped to the boot-rom area (cached) which
	 * we can use to get fast dummy-reads of cachelines
	 */

	volatile short *flushadr = (volatile short *)(((unsigned long)startadr & ~PAGE_MASK) |
						      CACHED_BOOTROM);

	length = length > 8192 ? 8192 : length;  /* No need to flush more than cache size */

	while(length > 0) {
		short tmp = *flushadr;           /* dummy read to flush */
		flushadr += (32/sizeof(short));  /* a cacheline is 32 bytes */
		length -= 32;
	}
}


void
prepare_rx_descriptor(struct etrax_dma_descr *desc)
{
	flush_etrax_cacherange((void *)desc->buf, desc->sw_len ? desc->sw_len : 65536);
}

/* Do the same thing but flush the entire cache */

void
flush_etrax_cache(void)
{
	flush_etrax_cacherange(0, 8192);
}

/* free the pages occupied by initialization code */

void 
free_initmem(void)
{
        unsigned long addr;

        addr = (unsigned long)(&__init_begin);
        for (; addr < (unsigned long)(&__init_end); addr += PAGE_SIZE) {
                ClearPageReserved(virt_to_page(addr));
                set_page_count(virt_to_page(addr), 1);
                free_page(addr);
                totalram_pages++;
        }
        printk (KERN_INFO "Freeing unused kernel memory: %luk freed\n", 
		(&__init_end - &__init_begin) >> 10);
}

void 
si_meminfo(struct sysinfo *val)
{
        val->totalram = totalram_pages;
        val->sharedram = 0;
        val->freeram = nr_free_pages();
        val->bufferram = atomic_read(&buffermem_pages);
        val->totalhigh = 0;
        val->freehigh = 0;
        val->mem_unit = PAGE_SIZE;
}
