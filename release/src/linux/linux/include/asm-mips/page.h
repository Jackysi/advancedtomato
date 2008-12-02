/*
 * Definitions for page handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 1999 by Ralf Baechle
 */
#ifndef __ASM_PAGE_H
#define __ASM_PAGE_H

#include <linux/config.h>

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12
#define PAGE_SIZE	(1L << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE-1))

#ifdef __KERNEL__

#ifndef __ASSEMBLY__

#define BUG() do { printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__); *(int *)0=0; } while (0)
#define PAGE_BUG(page) do {  BUG(); } while (0)

/*
 * Prototypes for clear_page / copy_page variants with processor dependant
 * optimizations.
 */
void andes_clear_page(void * page);
void mips32_clear_page_dc(unsigned long page);
void mips32_clear_page_sc(unsigned long page);
void r3k_clear_page(void * page);
void r4k_clear_page_d16(void * page);
void r4k_clear_page_d32(void * page);
void r4k_clear_page_r4600_v1(void * page);
void r4k_clear_page_r4600_v2(void * page);
void r4k_clear_page_s16(void * page);
void r4k_clear_page_s32(void * page);
void r4k_clear_page_s64(void * page);
void r4k_clear_page_s128(void * page);
void r5432_clear_page_d32(void * page);
void rm7k_clear_page(void * page);
void sb1_clear_page(void * page);
void andes_copy_page(void * to, void * from);
void mips32_copy_page_dc(unsigned long to, unsigned long from);
void mips32_copy_page_sc(unsigned long to, unsigned long from);
void r3k_copy_page(void * to, void * from);
void r4k_copy_page_d16(void * to, void * from);
void r4k_copy_page_d32(void * to, void * from);
void r4k_copy_page_r4600_v1(void * to, void * from);
void r4k_copy_page_r4600_v2(void * to, void * from);
void r4k_copy_page_s16(void * to, void * from);
void r4k_copy_page_s32(void * to, void * from);
void r4k_copy_page_s64(void * to, void * from);
void r4k_copy_page_s128(void * to, void * from);
void r5432_copy_page_d32(void * to, void * from);
void rm7k_copy_page(void * to, void * from);
void sb1_copy_page(void * to, void * from);

extern void (*_clear_page)(void * page);
extern void (*_copy_page)(void * to, void * from);

#define clear_page(page)	_clear_page(page)
#define copy_page(to, from)	_copy_page(to, from)
#define clear_user_page(page, vaddr)	clear_page(page)
#define copy_user_page(to, from, vaddr)	copy_page(to, from)

/*
 * These are used to make use of C type-checking..
 */
#ifdef CONFIG_64BIT_PHYS_ADDR
typedef struct { unsigned long long pte; } pte_t;
#else
typedef struct { unsigned long pte; } pte_t;
#endif
typedef struct { unsigned long pmd; } pmd_t;
typedef struct { unsigned long pgd; } pgd_t;
typedef struct { unsigned long pgprot; } pgprot_t;

#define pte_val(x)	((x).pte)
#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

#define ptep_buddy(x)	((pte_t *)((unsigned long)(x) ^ sizeof(pte_t)))

#define __pte(x)	((pte_t) { (x) } )
#define __pmd(x)	((pmd_t) { (x) } )
#define __pgd(x)	((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

/* Pure 2^n version of get_order */
extern __inline__ int get_order(unsigned long size)
{
	int order;

	size = (size-1) >> (PAGE_SHIFT-1);
	order = -1;
	do {
		size >>= 1;
		order++;
	} while (size);
	return order;
}

#endif /* !__ASSEMBLY__ */

/* to align the pointer to the (next) page boundary */
#define PAGE_ALIGN(addr)	(((addr) + PAGE_SIZE - 1) & PAGE_MASK)

/*
 * This handles the memory map.
 * We handle pages at KSEG0 for kernels with 32 bit address space.
 */
#define PAGE_OFFSET	0x80000000UL
#define UNCAC_BASE	0xa0000000UL

#define __pa(x)		((unsigned long) (x) - PAGE_OFFSET)
#define __va(x)		((void *)((unsigned long) (x) + PAGE_OFFSET))
#define virt_to_page(kaddr)	(mem_map + (__pa(kaddr) >> PAGE_SHIFT))
#define VALID_PAGE(page)	((page - mem_map) < max_mapnr)

#define VM_DATA_DEFAULT_FLAGS  (VM_READ | VM_WRITE | VM_EXEC | \
				VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC)

#define UNCAC_ADDR(addr)	((addr) - PAGE_OFFSET + UNCAC_BASE)
#define CAC_ADDR(addr)		((addr) - UNCAC_BASE + PAGE_OFFSET)

/*
 * Memory above this physical address will be considered highmem.
 */
#define HIGHMEM_START	0x20000000UL

#endif /* defined (__KERNEL__) */

#endif /* __ASM_PAGE_H */
