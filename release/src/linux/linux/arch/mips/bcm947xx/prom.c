/*
 * Early initialization code for BCM94710 boards
 *
 * Copyright 2004, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: prom.c,v 1.1 2005/03/16 13:49:59 wbx Exp $
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <asm/bootinfo.h>

void __init prom_init(int argc, char **argv, char **envp, int *prom_vec)
{
	unsigned long mem, before, offset;

	mips_machgroup = MACH_GROUP_BRCM;
	mips_machtype  = MACH_BCM947XX;

	/* Figure out memory size by finding aliases.
	 *
	 * We assume that there will be no more than 128 MB of memory,
	 * and that the memory size will be a multiple of 1 MB.
	 *
	 * We set 'before' to be the amount of memory (in MB) before this
	 * function, i.e. one MB less than the number  of MB of memory that we
	 * *know* we have.  And we set 'offset' to be the address of 'prominit'
	 * minus 'before', so that KSEG0 or KSEG1 base + offset < 1 MB.
	 * This prevents us from overrunning 128 MB and causing a bus error.
	 */
	before = ((unsigned long) &prom_init) & (127 << 20);
	offset = ((unsigned long) &prom_init) - before;
	for (mem = before + (1 << 20); mem < (128 << 20); mem += (1 << 20))
		if (*(unsigned long *)(offset + mem) ==
		    *(unsigned long *)(prom_init)) {
			/*
			 * We may already be well past the end of memory at
			 * this point, so we'll have to compensate for it.
			 */
			mem -= before;
			break;
		}

	/* Ignoring the last page when ddr size is 128M. Cached
	 * accesses to last page is causing the processor to prefetch
	 * using address above 128M stepping out of the ddr address
	 * space.
	 */
	if (mem == 0x8000000)
		mem -= 0x1000;

	add_memory_region(0, mem, BOOT_MEM_RAM);
}

void __init prom_free_prom_memory(void)
{
}
