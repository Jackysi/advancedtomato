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

/*
 * This version has been tuned on an Origin.  For other machines the arguments
 * of the pref instructin may have to be tuned differently.
 */
void andes_clear_page(void * page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tpref 7,512(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		: "=r" (page)
		: "0" (page), "I" (PAGE_SIZE)
		: "memory");
}

/* R10000 has no Create_Dirty type cacheops.  */
void andes_copy_page(void * to, void * from)
{
	unsigned long dummy1, dummy2, reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tpref\t0,2*128(%1)\n\t"
		"pref\t1,2*128(%0)\n\t"
		"ld\t%2,(%1)\n\t"
		"ld\t%3,8(%1)\n\t"
		"ld\t%4,16(%1)\n\t"
		"ld\t%5,24(%1)\n\t"
		"sd\t%2,(%0)\n\t"
		"sd\t%3,8(%0)\n\t"
		"sd\t%4,16(%0)\n\t"
		"sd\t%5,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"ld\t%2,-32(%1)\n\t"
		"ld\t%3,-24(%1)\n\t"
		"ld\t%4,-16(%1)\n\t"
		"ld\t%5,-8(%1)\n\t"
		"sd\t%2,-32(%0)\n\t"
		"sd\t%3,-24(%0)\n\t"
		"sd\t%4,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		" sd\t%5,-8(%0)\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2), "=&r" (reg1), "=&r" (reg2),
		 "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from), "I" (PAGE_SIZE));
}
