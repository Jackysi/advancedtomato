/*
 * BRIEF MODULE DESCRIPTION
 * Code to handle irqs on GT64120A boards
 *  Derived from mips/orion and Cort <cort@fsmlabs.com>
 *
 * Copyright (C) 2000 RidgeRun, Inc.
 * Author: RidgeRun, Inc.
 *   glonnon@ridgerun.com, skranz@ridgerun.com, stevej@ridgerun.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/config.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <asm/bitops.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/galileo-boards/ev64120int.h>

#undef IRQ_DEBUG

#ifdef IRQ_DEBUG
#define DBG(x...) printk(x)
#else
#define DBG(x...)
#endif


asmlinkage void do_IRQ(int irq, struct pt_regs *regs);

#define MAX_AGENTS_PER_INT 21	/*  Random number  */
unsigned char pci_int_irq[MAX_AGENTS_PER_INT];
static int max_interrupts = 0;

/*  Duplicate interrupt handlers.  */
/********************************************************************
 *pci_int(A/B/C/D) -
 *
 *Calls all the handlers connected to PCI interrupt A/B/C/D
 *
 *Inputs :
 *
 *Outpus :
 *
 *********************************************************************/
asmlinkage __inline__ void pci_intA(struct pt_regs *regs)
{
	unsigned int count = 0;
	DBG(KERN_INFO "pci_intA, max_interrupts %d\n", max_interrupts);
	for (count = 0; count < max_interrupts; count++) {
		do_IRQ(pci_int_irq[count], regs);
	}
}

asmlinkage __inline__ void pci_intD(struct pt_regs *regs)
{
	unsigned int count = 0;
	DBG(KERN_INFO "pci_intD, max_interrupts %d\n", max_interrupts);
	for (count = 0; count < max_interrupts; count++) {
		do_IRQ(pci_int_irq[count], regs);
	}
}


/* Function for careful CP0 interrupt mask access */
static inline void modify_cp0_intmask(unsigned clr_mask, unsigned set_mask)
{
	unsigned long status = read_c0_status();
	DBG(KERN_INFO "modify_cp0_intmask clr %x, set %x\n", clr_mask,
	    set_mask);
	DBG(KERN_INFO "modify_cp0_intmask status %x\n", status);
	status &= ~((clr_mask & 0xFF) << 8);
	status |= (set_mask & 0xFF) << 8;
	DBG(KERN_INFO "modify_cp0_intmask status %x\n", status);
	write_c0_status(status);
}

static inline void mask_irq(unsigned int irq_nr)
{
	modify_cp0_intmask(irq_nr, 0);
}

static inline void unmask_irq(unsigned int irq_nr)
{
	modify_cp0_intmask(0, irq_nr);
}

void disable_irq(unsigned int irq_nr)
{
	unsigned long flags;

	DBG(KERN_INFO "disable_irq, irq %d\n", irq_nr);
	save_and_cli(flags);
	if (irq_nr >= 8) {	// All PCI interrupts are on line 5 or 2
		mask_irq(9 << 2);
	} else {
		mask_irq(1 << irq_nr);
	}
	restore_flags(flags);
}

void enable_irq(unsigned int irq_nr)
{
	unsigned long flags;

	DBG(KERN_INFO "enable_irq, irq %d\n", irq_nr);
	save_and_cli(flags);
	if (irq_nr >= 8) {	// All PCI interrupts are on line 5 or 2
		DBG(KERN_INFO __FUNCTION__ " pci interrupt %d\n", irq_nr);
		unmask_irq(9 << 2);
	} else {
		DBG(KERN_INFO __FUNCTION__ " interrupt set mask %d\n",
		    1 << irq_nr);
		unmask_irq(1 << irq_nr);
	}
	restore_flags(flags);
}

/*
 * Generic no controller code
 */

static void no_irq_enable_disable(unsigned int irq)
{
}
static unsigned int no_irq_startup(unsigned int irq)
{
	return 0;
}


struct hw_interrupt_type no_irq_type = {
	.typename= "none",
	.startup= no_irq_startup,
	.shutdown= no_irq_enable_disable,
	.enable= no_irq_enable_disable,
	.disable= no_irq_enable_disable,
	.ack= NULL,
	.end= no_irq_enable_disable,
};

//      ack:            no_irq_ack,                re-enable later -- SKJ


/*
 * Controller mappings for all interrupt sources:
 */
irq_desc_t irq_desc[NR_IRQS];

atomic_t irq_err_count;

int get_irq_list(char *buf)
{
	int i, len = 0, j;
	struct irqaction *action;

	len += sprintf(buf + len, "           ");
	for (j = 0; j < smp_num_cpus; j++)
		len += sprintf(buf + len, "CPU%d       ", j);
	*(char *) (buf + len++) = '\n';

	for (i = 0; i < NR_IRQS; i++) {
		action = irq_desc[i].action;
		if (!action || !action->handler)
			continue;
		len += sprintf(buf + len, "%3d: ", i);
		len += sprintf(buf + len, "%10u ", kstat_irqs(i));
		if (irq_desc[i].handler)
			len +=
			    sprintf(buf + len, " %s ",
				    irq_desc[i].handler->typename);
		else
			len += sprintf(buf + len, "  None      ");
		len += sprintf(buf + len, "    %s", action->name);
		for (action = action->next; action; action = action->next) {
			len += sprintf(buf + len, ", %s", action->name);
		}
		len += sprintf(buf + len, "\n");
	}
	len += sprintf(buf + len, "BAD: %10lu\n", atomic_read(&irq_err_count));
	return len;
}

asmlinkage void do_IRQ(int irq, struct pt_regs *regs)
{
	struct irqaction *action;
	int cpu;

#ifdef IRQ_DEBUG
	if (irq != TIMER)
		DBG(KERN_INFO __FUNCTION__ " irq = %d\n", irq);
	if (irq != TIMER)
		DBG(KERN_INFO "cause register = %x\n",
		    read_c0_cause());
	if (irq != TIMER)
		DBG(KERN_INFO "status register = %x\n",
		    read_c0_status());
#endif

	cpu = smp_processor_id();
	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;

	if (irq_desc[irq].handler->ack) {
		irq_desc[irq].handler->ack(irq);
	}

	disable_irq(irq);

	action = irq_desc[irq].action;
	if (action && action->handler) {
#ifdef IRQ_DEBUG
		if (irq != TIMER)
			DBG(KERN_INFO
			    "rr: irq %d action %p and handler %p\n", irq,
			    action, action->handler);
#endif
		if (!(action->flags & SA_INTERRUPT))
			__sti();
		do {
			action->handler(irq, action->dev_id, regs);
			action = action->next;
		} while (action);
		__cli();
		if (irq_desc[irq].handler) {
			if (irq_desc[irq].handler->end)
				irq_desc[irq].handler->end(irq);
			else if (irq_desc[irq].handler->enable)
				irq_desc[irq].handler->enable(irq);
		}
	}

	enable_irq(irq);
	irq_exit(cpu, irq);

	if (softirq_pending(cpu))
		do_softirq();

	/* unmasking and bottom half handling is done magically for us. */
}

int request_irq(unsigned int irq,
		void (*handler) (int, void *, struct pt_regs *),
		unsigned long irqflags, const char *devname, void *dev_id)
{
	struct irqaction *old, **p, *action;
	unsigned long flags;

	DBG(KERN_INFO "rr:dev %s irq %d handler %x\n", devname, irq,
	    handler);
	if (irq >= NR_IRQS)
		return -EINVAL;

	action = (struct irqaction *)
	    kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->dev_id = dev_id;
	action->next = NULL;

	save_flags(flags);
	cli();

	p = &irq_desc[irq].action;

	if ((old = *p) != NULL) {
		/* Can't share interrupts unless both agree to */
		if (!(old->flags & action->flags & SA_SHIRQ))
			return -EBUSY;
		/* add new interrupt at end of irq queue */
		do {
			p = &old->next;
			old = *p;
		} while (old);
	}
	*p = action;

	restore_flags(flags);
	if (irq >= 8) {
		DBG(KERN_INFO "request_irq, max_interrupts %d\n",
		    max_interrupts);
		pci_int_irq[max_interrupts++] = irq;	// NOTE:  Add error-handling if > max
	}
	enable_irq(irq);
	return 0;
}


void free_irq(unsigned int irq, void *dev_id)
{
	struct irqaction *p, *old = NULL;
	unsigned long flags;
	int count, tmp, removed = 0;

	for (p = irq_desc[irq].action; p != NULL; old = p, p = p->next) {
		/* Found the IRQ, is it the correct dev_id?  */
		if (dev_id == p->dev_id) {
			save_flags(flags);
			cli();

			// remove link from list
			if (old)
				old->next = p->next;
			else
				irq_desc[irq].action = p->next;

			restore_flags(flags);
			kfree(p);
			removed = 1;
			break;
		}
	}

	/*
	   Remove PCI interrupts from the pci_int_irq list.  Make sure
	   that some handler was removed before decrementing max_interrupts.
	 */
	if ((irq >= 8) && (removed)) {
		for (count = 0; count < max_interrupts; count++) {
			if (pci_int_irq[count] == irq) {
				for (tmp = count; tmp < max_interrupts;
				     tmp++) {
					pci_int_irq[tmp] =
					    pci_int_irq[tmp + 1];
				}
			}
		}
		max_interrupts--;
		DBG(KERN_INFO "free_irq, max_interrupts %d\n",
		    max_interrupts);
	}
}

unsigned long probe_irq_on(void)
{
	printk(KERN_INFO "probe_irq_on\n");
	return 0;
}

int probe_irq_off(unsigned long irqs)
{
	printk(KERN_INFO "probe_irq_off\n");
	return 0;
}

/********************************************************************
 *galileo_irq_setup -
 *
 *Initializes CPU interrupts
 *
 *
 *Inputs :
 *
 *Outpus :
 *
 *********************************************************************/
void galileo_irq_setup(void)
{
	extern asmlinkage void galileo_handle_int(void);
	extern void galileo_irq_init(void);

	DBG(KERN_INFO "rr: galileo_irq_setup entry\n");

	galileo_irq_init();

	/*
	 * Clear all of the interrupts while we change the able around a bit.
	 */
	clear_c0_status(ST0_IM);

	/* Sets the exception_handler array. */
	set_except_vector(0, galileo_handle_int);

	cli();

	/*
	 * Enable timer.  Other interrupts will be enabled as they are
	 * registered.
	 */
	set_c0_status(IE_IRQ2);


#ifdef CONFIG_REMOTE_DEBUG
	{
		extern int DEBUG_CHANNEL;
		serial_init(DEBUG_CHANNEL);
		serial_set(DEBUG_CHANNEL, 115200);
		set_debug_traps();
		breakpoint();	/* you may move this line to whereever you want :-) */
	}
#endif
}

void init_irq_proc(void)
{
	/* Nothing, for now.  */
}

void __init init_IRQ(void)
{
	int i;

	DBG(KERN_INFO "rr:init_IRQ\n");

	/*  Let's initialize our IRQ descriptors  */
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i].status = 0;
		irq_desc[i].handler = &no_irq_type;
		irq_desc[i].action = NULL;
		irq_desc[i].depth = 0;
		irq_desc[i].lock = SPIN_LOCK_UNLOCKED;
	}

	galileo_irq_setup();
}
