/*
 * VIA chipset irq handling
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996, 1997 by Ralf Baechle
 * Copyright (C) 2001 by Liam Davies (ldavies@agile.tv)
 *
 */

#include <linux/kernel.h>
#include <asm/cobalt/cobalt.h>
#include <asm/ptrace.h>
#include <asm/io.h>
#include <asm/mipsregs.h>

extern void do_IRQ(int irq, struct pt_regs * regs);

asmlinkage void via_irq(struct pt_regs *regs)
{
	char mstat, sstat;

	/* Read Master Status */
	outb(0x0C, 0x20);
	mstat = inb(0x20);

	if (mstat < 0) {
		mstat &= 0x7f;
		if (mstat != 2) {
			do_IRQ(mstat, regs);
			outb(mstat | 0x20, 0x20);
		} else {
			sstat = inb(0xA0);

			/* Slave interrupt */
			outb(0x0C, 0xA0);
			sstat = inb(0xA0);

			if (sstat < 0) {
				do_IRQ((sstat + 8) & 0x7f, regs);
				outb(0x22, 0x20);
				outb((sstat & 0x7f) | 0x20, 0xA0);
			} else {
				printk("Spurious slave interrupt...\n");
			}
		}
	} else
		printk("Spurious master interrupt...");
}

#define GALILEO_INTCAUSE	0xb4000c18
#define GALILEO_T0EXP		0x00000100

asmlinkage void galileo_irq(struct pt_regs *regs)
{
	unsigned long irq_src = *((unsigned long *) GALILEO_INTCAUSE);

	/* Check for timer irq ... */
	if (irq_src & GALILEO_T0EXP) {
		/* Clear the int line */
		*((volatile unsigned long *) GALILEO_INTCAUSE) = 0;
		do_IRQ(COBALT_TIMER_IRQ, regs);
	} else
		printk("Spurious Galileo interrupt...\n");
}
