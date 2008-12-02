/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994, 95, 96, 97, 98, 99, 2000 by Ralf Baechle at alii
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#ifndef _ASM_PGTABLE_H
#define _ASM_PGTABLE_H

#include <linux/config.h>
#include <asm/addrspace.h>
#include <asm/page.h>

#include <linux/linkage.h>
#include <asm/cachectl.h>
#include <asm/fixmap.h>

/* Cache flushing:
 *
 *  - flush_cache_all() flushes entire cache
 *  - flush_cache_mm(mm) flushes the specified mm context's cache lines
 *  - flush_cache_page(mm, vmaddr) flushes a single page
 *  - flush_cache_range(mm, start, end) flushes a range of pages
 *  - flush_page_to_ram(page) write back kernel page to ram
 *  - flush_icache_range(start, end) flush a range of instructions
 *
 *  - flush_cache_sigtramp() flush signal trampoline
 *  - flush_icache_all() flush the entire instruction cache
 */
extern void (*_flush_cache_all)(void);
extern void (*___flush_cache_all)(void);
extern void (*_flush_cache_mm)(struct mm_struct *mm);
extern void (*_flush_cache_range)(struct mm_struct *mm, unsigned long start,
	unsigned long end);
extern void (*_flush_cache_page)(struct vm_area_struct *vma,
	unsigned long page);
extern void (*_flush_page_to_ram)(struct page * page);
extern void (*_flush_icache_range)(unsigned long start, unsigned long end);
extern void (*_flush_icache_page)(struct vm_area_struct *vma,
	struct page *page);
extern void (*_flush_cache_sigtramp)(unsigned long addr);
extern void (*_flush_icache_all)(void);

#define flush_dcache_page(page)			do { } while (0)

#define flush_cache_all()		_flush_cache_all()
#define __flush_cache_all()		___flush_cache_all()
#define flush_cache_mm(mm)		_flush_cache_mm(mm)
#define flush_cache_range(mm,start,end)	_flush_cache_range(mm,start,end)
#define flush_cache_page(vma,page)	_flush_cache_page(vma, page)
#define flush_page_to_ram(page)		_flush_page_to_ram(page)

#define flush_icache_range(start, end)	_flush_icache_range(start,end)
#define flush_icache_user_range(vma, page, addr, len) \
					_flush_icache_page((vma), (page))
#define flush_icache_page(vma, page) 	_flush_icache_page(vma, page)

#define flush_cache_sigtramp(addr)	_flush_cache_sigtramp(addr)
#ifdef CONFIG_VTAG_ICACHE
#define flush_icache_all()		_flush_icache_all()
#else
#define flush_icache_all()		do { } while(0)
#endif

/*
 * - add_wired_entry() add a fixed TLB entry, and move wired register
 */
extern void add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
			       unsigned long entryhi, unsigned long pagemask);

/*
 * - add_temporary_entry() add a temporary TLB entry. We use TLB entries
 *	starting at the top and working down. This is for populating the
 *	TLB before trap_init() puts the TLB miss handler in place. It
 *	should be used only for entries matching the actual page tables,
 *	to prevent inconsistencies.
 */
extern int add_temporary_entry(unsigned long entrylo0, unsigned long entrylo1,
			       unsigned long entryhi, unsigned long pagemask);


/* Basically we have the same two-level (which is the logical three level
 * Linux page table layout folded) page tables as the i386.  Some day
 * when we have proper page coloring support we can have a 1% quicker
 * tlb refill handling mechanism, but for now it is a bit slower but
 * works even with the cache aliasing problem the R4k and above have.
 */

/* PMD_SHIFT determines the size of the area a second-level page table can map */
#ifdef CONFIG_64BIT_PHYS_ADDR
#define PMD_SHIFT	21
#else
#define PMD_SHIFT	22
#endif
#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))

/* PGDIR_SHIFT determines what a third-level page table entry can map */
#define PGDIR_SHIFT	PMD_SHIFT
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

/*
 * Entries per page directory level: we use two-level, so
 * we don't really have any PMD directory physically.
 */
#ifdef CONFIG_64BIT_PHYS_ADDR
#define PTRS_PER_PTE	512
#define PTRS_PER_PMD	1
#define PTRS_PER_PGD	2048
#define PGD_ORDER	1
#else
#define PTRS_PER_PTE	1024
#define PTRS_PER_PMD	1
#define PTRS_PER_PGD	1024
#define PGD_ORDER	0
#endif

#define USER_PTRS_PER_PGD	(0x80000000UL/PGDIR_SIZE)
#define FIRST_USER_PGD_NR	0

#define VMALLOC_START     KSEG2
#define VMALLOC_VMADDR(x) ((unsigned long)(x))

#if CONFIG_HIGHMEM
# define VMALLOC_END	(PKMAP_BASE-2*PAGE_SIZE)
#else
# define VMALLOC_END	(FIXADDR_START-2*PAGE_SIZE)
#endif

#include <asm/pgtable-bits.h>

#define PAGE_NONE	__pgprot(_PAGE_PRESENT | _CACHE_CACHABLE_NONCOHERENT)
#define PAGE_SHARED     __pgprot(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_COPY       __pgprot(_PAGE_PRESENT | _PAGE_READ | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_READONLY   __pgprot(_PAGE_PRESENT | _PAGE_READ | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_KERNEL	__pgprot(_PAGE_PRESENT | __READABLE | __WRITEABLE | \
			_PAGE_GLOBAL | PAGE_CACHABLE_DEFAULT)
#define PAGE_USERIO     __pgprot(_PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | \
			PAGE_CACHABLE_DEFAULT)
#define PAGE_KERNEL_UNCACHED __pgprot(_PAGE_PRESENT | __READABLE | \
			__WRITEABLE | _PAGE_GLOBAL | _CACHE_UNCACHED)

/*
 * MIPS can't do page protection for execute, and considers that the same like
 * read. Also, write permissions imply read permissions. This is the closest
 * we can get by reasonable means..
 */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY
#define __P101	PAGE_READONLY
#define __P110	PAGE_COPY
#define __P111	PAGE_COPY

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY
#define __S101	PAGE_READONLY
#define __S110	PAGE_SHARED
#define __S111	PAGE_SHARED

#ifdef CONFIG_64BIT_PHYS_ADDR
#define pte_ERROR(e) \
	printk("%s:%d: bad pte %016Lx.\n", __FILE__, __LINE__, pte_val(e))
#else
#define pte_ERROR(e) \
	printk("%s:%d: bad pte %08lx.\n", __FILE__, __LINE__, pte_val(e))
#endif
#define pmd_ERROR(e) \
	printk("%s:%d: bad pmd %08lx.\n", __FILE__, __LINE__, pmd_val(e))
#define pgd_ERROR(e) \
	printk("%s:%d: bad pgd %08lx.\n", __FILE__, __LINE__, pgd_val(e))

extern unsigned long empty_zero_page;
extern unsigned long zero_page_mask;

#define ZERO_PAGE(vaddr) \
	(virt_to_page(empty_zero_page + (((unsigned long)(vaddr)) & zero_page_mask)))

extern void load_pgd(unsigned long pg_dir);

extern pmd_t invalid_pte_table[PAGE_SIZE/sizeof(pmd_t)];

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */
static inline unsigned long pmd_page(pmd_t pmd)
{
	return pmd_val(pmd);
}

static inline void pmd_set(pmd_t * pmdp, pte_t * ptep)
{
	pmd_val(*pmdp) = (((unsigned long) ptep) & PAGE_MASK);
}

static inline int pte_none(pte_t pte)    { return !(pte_val(pte) & ~_PAGE_GLOBAL); }
static inline int pte_present(pte_t pte) { return pte_val(pte) & _PAGE_PRESENT; }

/* Certain architectures need to do special things when pte's
 * within a page table are directly modified.  Thus, the following
 * hook is made available.
 */
static inline void set_pte(pte_t *ptep, pte_t pteval)
{
	*ptep = pteval;
#if !defined(CONFIG_CPU_R3000) && !defined(CONFIG_CPU_TX39XX)
	if (pte_val(pteval) & _PAGE_GLOBAL) {
		pte_t *buddy = ptep_buddy(ptep);
		/*
		 * Make sure the buddy is global too (if it's !none,
		 * it better already be global)
		 */
		if (pte_none(*buddy))
			pte_val(*buddy) = pte_val(*buddy) | _PAGE_GLOBAL;
	}
#endif
}

static inline void pte_clear(pte_t *ptep)
{
#if !defined(CONFIG_CPU_R3000) && !defined(CONFIG_CPU_TX39XX)
	/* Preserve global status for the pair */
	if (pte_val(*ptep_buddy(ptep)) & _PAGE_GLOBAL)
		set_pte(ptep, __pte(_PAGE_GLOBAL));
	else
#endif
		set_pte(ptep, __pte(0));
}

/*
 * (pmds are folded into pgds so this doesn't get actually called,
 * but the define is needed for a generic inline function.)
 */
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)
#define set_pgd(pgdptr, pgdval) (*(pgdptr) = pgdval)

/*
 * Empty pgd/pmd entries point to the invalid_pte_table.
 */
static inline int pmd_none(pmd_t pmd)
{
	return pmd_val(pmd) == (unsigned long) invalid_pte_table;
}

static inline int pmd_bad(pmd_t pmd)
{
	return ((pmd_page(pmd) > (unsigned long) high_memory) ||
	        (pmd_page(pmd) < PAGE_OFFSET));
}

static inline int pmd_present(pmd_t pmd)
{
	return (pmd_val(pmd) != (unsigned long) invalid_pte_table);
}

static inline void pmd_clear(pmd_t *pmdp)
{
	pmd_val(*pmdp) = ((unsigned long) invalid_pte_table);
}

/*
 * The "pgd_xxx()" functions here are trivial for a folded two-level
 * setup: the pgd is never bad, and a pmd always exists (as it's folded
 * into the pgd entry)
 */
static inline int pgd_none(pgd_t pgd)		{ return 0; }
static inline int pgd_bad(pgd_t pgd)		{ return 0; }
static inline int pgd_present(pgd_t pgd)	{ return 1; }
static inline void pgd_clear(pgd_t *pgdp)	{ }

/*
 * Permanent address of a page.  Obviously must never be called on a highmem
 * page.
 */
#ifdef CONFIG_CPU_VR41XX
#define pte_page(x)             (mem_map+(unsigned long)((pte_val(x) >> (PAGE_SHIFT + 2))))
#else
#define pte_page(x)		(mem_map+(unsigned long)((pte_val(x) >> PAGE_SHIFT)))
#endif

/*
 * The following only work if pte_present() is true.
 * Undefined behaviour if not..
 */
static inline int pte_read(pte_t pte)	{ return pte_val(pte) & _PAGE_READ; }
static inline int pte_write(pte_t pte)	{ return pte_val(pte) & _PAGE_WRITE; }
static inline int pte_dirty(pte_t pte)	{ return pte_val(pte) & _PAGE_MODIFIED; }
static inline int pte_young(pte_t pte)	{ return pte_val(pte) & _PAGE_ACCESSED; }

static inline pte_t pte_wrprotect(pte_t pte)
{
	pte_val(pte) &= ~(_PAGE_WRITE | _PAGE_SILENT_WRITE);
	return pte;
}

static inline pte_t pte_rdprotect(pte_t pte)
{
	pte_val(pte) &= ~(_PAGE_READ | _PAGE_SILENT_READ);
	return pte;
}

static inline pte_t pte_mkclean(pte_t pte)
{
	pte_val(pte) &= ~(_PAGE_MODIFIED|_PAGE_SILENT_WRITE);
	return pte;
}

static inline pte_t pte_mkold(pte_t pte)
{
	pte_val(pte) &= ~(_PAGE_ACCESSED|_PAGE_SILENT_READ);
	return pte;
}

static inline pte_t pte_mkwrite(pte_t pte)
{
	pte_val(pte) |= _PAGE_WRITE;
	if (pte_val(pte) & _PAGE_MODIFIED)
		pte_val(pte) |= _PAGE_SILENT_WRITE;
	return pte;
}

static inline pte_t pte_mkread(pte_t pte)
{
	pte_val(pte) |= _PAGE_READ;
	if (pte_val(pte) & _PAGE_ACCESSED)
		pte_val(pte) |= _PAGE_SILENT_READ;
	return pte;
}

static inline pte_t pte_mkdirty(pte_t pte)
{
	pte_val(pte) |= _PAGE_MODIFIED;
	if (pte_val(pte) & _PAGE_WRITE)
		pte_val(pte) |= _PAGE_SILENT_WRITE;
	return pte;
}

/*
 * Macro to make mark a page protection value as "uncacheable".  Note
 * that "protection" is really a misnomer here as the protection value
 * contains the memory attribute bits, dirty bits, and various other
 * bits as well.
 */
#define pgprot_noncached pgprot_noncached

static inline pgprot_t pgprot_noncached(pgprot_t _prot)
{
	unsigned long prot = pgprot_val(_prot);

	prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;

	return __pgprot(prot);
}

static inline pte_t pte_mkyoung(pte_t pte)
{
	pte_val(pte) |= _PAGE_ACCESSED;
	if (pte_val(pte) & _PAGE_READ)
		pte_val(pte) |= _PAGE_SILENT_READ;
	return pte;
}

/*
 * Conversion functions: convert a page and protection to a page entry,
 * and a page entry and page directory to the page they refer to.
 */

#ifdef CONFIG_CPU_VR41XX
#define mk_pte(page, pgprot)                                            \
({                                                                      \
        pte_t   __pte;                                                  \
                                                                        \
        pte_val(__pte) = ((phys_t)(page - mem_map) << (PAGE_SHIFT + 2)) | \
                         pgprot_val(pgprot);                            \
                                                                        \
        __pte;                                                          \
})
#else
#define mk_pte(page, pgprot)						\
({									\
	pte_t   __pte;							\
									\
	pte_val(__pte) = ((phys_t)(page - mem_map) << PAGE_SHIFT) | \
	                 pgprot_val(pgprot);				\
									\
	__pte;								\
})
#endif

static inline pte_t mk_pte_phys(phys_t physpage, pgprot_t pgprot)
{
#ifdef CONFIG_CPU_VR41XX
        return __pte((physpage << 2) | pgprot_val(pgprot));
#else
	return __pte(physpage | pgprot_val(pgprot));
#endif
}

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	return __pte((pte_val(pte) & _PAGE_CHG_MASK) | pgprot_val(newprot));
}

#define page_pte(page) page_pte_prot(page, __pgprot(0))

#define __pgd_offset(address)	pgd_index(address)
#define __pmd_offset(address) \
	(((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

#define pgd_index(address)	((address) >> PGDIR_SHIFT)

/* to find an entry in a page-table-directory */
static inline pgd_t *pgd_offset(struct mm_struct *mm, unsigned long address)
{
	return mm->pgd + pgd_index(address);
}

/* Find an entry in the second-level page table.. */
static inline pmd_t *pmd_offset(pgd_t *dir, unsigned long address)
{
	return (pmd_t *) dir;
}

/* Find an entry in the third-level page table.. */
static inline pte_t *pte_offset(pmd_t * dir, unsigned long address)
{
	return (pte_t *) (pmd_page(*dir)) +
	       ((address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1));
}

extern int do_check_pgt_cache(int, int);

extern pgd_t swapper_pg_dir[1024];
extern void paging_init(void);

extern void update_mmu_cache(struct vm_area_struct *vma,
				unsigned long address, pte_t pte);

/* Swap entries must have VALID and GLOBAL bits cleared. */
#if defined(CONFIG_CPU_R3000) || defined(CONFIG_CPU_TX39XX)

#define SWP_TYPE(x)		(((x).val >> 1) & 0x7f)
#define SWP_OFFSET(x)		((x).val >> 10)
#define SWP_ENTRY(type,offset)	((swp_entry_t) { ((type) << 1) | ((offset) << 10) })
#else

#define SWP_TYPE(x)		(((x).val >> 1) & 0x1f)
#define SWP_OFFSET(x)		((x).val >> 8)
#define SWP_ENTRY(type,offset)	((swp_entry_t) { ((type) << 1) | ((offset) << 8) })
#endif

#define pte_to_swp_entry(pte)	((swp_entry_t) { pte_val(pte) })
#define swp_entry_to_pte(x)	((pte_t) { (x).val })


/* Needs to be defined here and not in linux/mm.h, as it is arch dependent */
#define PageSkip(page)		(0)
#define kern_addr_valid(addr)	(1)

#include <asm-generic/pgtable.h>

/*
 * We provide our own get_unmapped area to cope with the virtual aliasing
 * constraints placed on us by the cache architecture.
 */
#define HAVE_ARCH_UNMAPPED_AREA

#define io_remap_page_range remap_page_range

/*
 * No page table caches to initialise
 */
#define pgtable_cache_init()	do { } while (0)

#endif /* _ASM_PGTABLE_H */
