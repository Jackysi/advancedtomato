/*
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * Broadcom HND BCM47xx chips interrupt dispatcher.
 * Derived from ../generic/irq.c
 *
 *	MIPS IRQ	Source
 *      --------        ------------------
 *             0	Software
 *             1        Software
 *             2        Hardware (shared)
 *             3        Hardware
 *             4        Hardware
 *             5        Hardware
 *             6        Hardware
 *             7        Hardware (r4k timer)
 *
 *      MIPS IRQ        Linux IRQ
 *      --------        -----------
 *         0 - 1        0 - 1
 *             2        8 and above
 *         3 - 7        3 - 7
 *
 * MIPS has 8 IRQs as indicated and assigned above. SB cores
 * that use dedicated MIPS IRQ3 to IRQ6 are 1-to-1 mapped to
 * linux IRQ3 to IRQ6. SB cores sharing MIPS IRQ2 are mapped
 * to linux IRQ8 and above as virtual IRQs using the following
 * mapping:
 *
 *   <linux-IRQ#> = <SB-core-flag> + <base-IRQ> + 2
 *
 * where <base-IRQ> is specified in setup.c when calling
 * sb_mips_init(), 2 is to offset the two software IRQs.
 *
 * $Id: irq.c,v 1.1.1.1 2007/03/20 12:20:31 roly Exp $
 */

#include <linux/config.h>

#ifdef CONFIG_SMP
/*
 * This code is designed to work on Uniprocessor only.
 *
 * To support SMP we must know:
 *   - interrupt architecture
 *   - interrupt distribution machenism
 */
#error "This implementation does not support SMP"
#endif

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>

#include <asm/mipsregs.h>

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <sbconfig.h>
#include <sbutils.h>
#include <hndcpu.h>
#include "bcm947xx.h"

/* cp0 SR register IM field */
#define SR_IM(irq)	(1 << ((irq) + STATUSB_IP0))

/* cp0 CR register IP field */
#define CR_IP(irq)	(1 << ((irq) + CAUSEB_IP0))

/* other local constants */
#define NUM_IRQS	32

/* local variables and functions */
static sbconfig_t *ccsbr = NULL;	/* Chipc core SB config regs */
static sbconfig_t *mipssbr = NULL;	/* MIPS core SB config regs */
static int mipsirq = -1;		/* MIPS core virtual IRQ number */
static uint32 sbintvec = 0;		/* MIPS core sbintvec reg val */
static int irq2en = 0;			/* MIPS IRQ2 enable count */

/* global variables and functions */
extern sb_t *bcm947xx_sbh;		/* defined in setup.c */

extern asmlinkage void brcmIRQ(void);

/* Control functions for MIPS IRQ0 to IRQ7 */
static INLINE void
enable_brcm_irq(unsigned int irq)
{
	set_c0_status(SR_IM(irq));
}

static INLINE void
disable_brcm_irq(unsigned int irq)
{
	clear_c0_status(SR_IM(irq));
}

static unsigned int
startup_brcm_irq(unsigned int irq)
{ 
	enable_brcm_irq(irq);
	return 0;
}

static void
shutdown_brcm_irq(unsigned int irq)
{
	disable_brcm_irq(irq);
}

static void
ack_brcm_irq(unsigned int irq)
{
	/* Done in brcm_irq_dispatch()! */
}

static void
end_brcm_irq(unsigned int irq)
{
	/* Done in brcm_irq_dispatch()! */
}

/* Control functions for linux IRQ8 and above */
static INLINE void
enable_brcm_irq2(unsigned int irq)
{
	ASSERT(irq2en >= 0);
	if (irq2en++)
		return;
	enable_brcm_irq(2);
}

static INLINE void
disable_brcm_irq2(unsigned int irq)
{
	ASSERT(irq2en > 0);
	if (--irq2en)
		return;
	disable_brcm_irq(2);
}

static unsigned int
startup_brcm_irq2(unsigned int irq)
{
	enable_brcm_irq2(irq);
	return 0;
}

static void
shutdown_brcm_irq2(unsigned int irq)
{
	disable_brcm_irq2(irq);
}

static void
ack_brcm_irq2(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch()! */
}

static void
end_brcm_irq2(unsigned int irq)
{
	/* Already done in brcm_irq_dispatch()! */
}

/*
 * Route interrupts to ISR(s).
 *
 * This function is entered with the IE disabled. It can be
 * re-entered as soon as the IE is re-enabled in function
 * handle_IRQ_envet().
 */
void
brcm_irq_dispatch(struct pt_regs *regs)
{
	u32 pending, ipvec;
	uint32 sbflagst = 0;
	struct irqaction *action;
	int cpu = smp_processor_id();
	int irq;

	/* Disable MIPS IRQs with pending interrupts */
	pending = regs->cp0_cause & CAUSEF_IP;
	pending &= regs->cp0_status;
	clear_c0_status(pending);

	/*
	 * Build bitvec for pending interrupts. Start with
	 * MIPS IRQ2 and add linux IRQs to higher bits to
	 * make the interrupt processing uniform.
	 */
	ipvec = pending >> CAUSEB_IP2;
	if (pending & CAUSEF_IP2) {
		if (ccsbr && mipssbr) {
			/* Make sure no one has changed IRQ assignments */
			ASSERT(R_REG(NULL, &mipssbr->sbintvec) == sbintvec);
			sbflagst = R_REG(NULL, &ccsbr->sbflagst);
			sbflagst &= sbintvec;
			ipvec += sbflagst << SBMIPS_VIRTIRQ_BASE;
		}
	}

	/*
	 * Handle MIPS timer interrupt. Re-enable MIPS IRQ7
	 * immediately after servicing the interrupt so that
	 * we can take this kind of interrupt again later
	 * while servicing other interrupts.
	 */
	if (pending & CAUSEF_IP7) {
		kstat.irqs[cpu][7]++;
		action = irq_desc[7].action;
		if (action)
			handle_IRQ_event(7, regs, action);
		ipvec &= ~(1 << 5);
		pending &= ~CAUSEF_IP7;
		set_c0_status(STATUSF_IP7);
	}

#ifdef CONFIG_HND_BMIPS3300_PROF
	/*
	 * Handle MIPS core interrupt. Re-enable the MIPS IRQ that
	 * MIPS core is assigned to immediately after servicing the
	 * interrupt so that we can take this kind of interrupt again
	 * later while servicing other interrupts.
	 *
	 * mipsirq < 0 indicates MIPS core IRQ # is unknown.
	 */
	if (mipsirq >= 0 && (ipvec & (1 << mipsirq))) {
		/*
		 * MIPS core raised the interrupt on the shared MIPS IRQ2.
		 * Make sure MIPS core is the only interrupt source before
		 * re-enabling the IRQ.
		 */
		if (mipsirq >= SBMIPS_VIRTIRQ_BASE) {
			if (sbflagst == (1 << (mipsirq-SBMIPS_VIRTIRQ_BASE))) {
				irq = mipsirq + 2;
				kstat.irqs[cpu][irq]++;
				action = irq_desc[irq].action;
				if (action)
					handle_IRQ_event(irq, regs, action);
				ipvec &= ~(1 << mipsirq);
				pending &= ~CAUSEF_IP2;
				set_c0_status(STATUSF_IP2);
			}
		}
		/*
		 * MIPS core raised the interrupt on a dedicated MIPS IRQ.
		 * Re-enable the IRQ immediately.
		 */
		else {
			irq = mipsirq + 2;
			kstat.irqs[cpu][irq]++;
			action = irq_desc[irq].action;
			if (action)
				handle_IRQ_event(irq, regs, action);
			ipvec &= ~(1 << mipsirq);
			pending &= ~CR_IP(irq);
			set_c0_status(SR_IM(irq));
		}
	}
#endif	/* CONFIG_HND_BMIPS3300_PROF */

	/*
	 * Handle all other interrupts. Re-enable disabled MIPS IRQs
	 * after processing all pending interrupts.
	 */
	for (irq = 2; irq < NUM_IRQS && ipvec != 0; irq ++) {
		if (ipvec & 1) {
		kstat.irqs[cpu][irq]++;
			if ((action = irq_desc[irq].action))
			handle_IRQ_event(irq, regs, action);
		}
		ipvec >>= 1;
	}
	set_c0_status(pending);

	/* Run pending bottom halves */
	if (softirq_pending(cpu))
		do_softirq();
}

/* MIPS IRQ0 to IRQ7 interrupt controller */
static struct hw_interrupt_type brcm_irq_type = {
	typename: "MIPS",
	startup: startup_brcm_irq,
	shutdown: shutdown_brcm_irq,
	enable: enable_brcm_irq,
	disable: disable_brcm_irq,
	ack: ack_brcm_irq,
	end: end_brcm_irq,
	NULL
};

/* linux IRQ8 and above interrupt controller */
static struct hw_interrupt_type brcm_irq2_type = {
	typename: "IRQ2",
	startup: startup_brcm_irq2,
	shutdown: shutdown_brcm_irq2,
	enable: enable_brcm_irq2,
	disable: disable_brcm_irq2,
	ack: ack_brcm_irq2,
	end: end_brcm_irq2,
	NULL
};

/*
 * We utilize chipcommon configuration register SBFlagSt to implement a
 * smart shared IRQ handling machenism through which only ISRs registered
 * for the SB cores that raised the interrupt are invoked. This machenism
 * relies on the SBFlagSt register's reliable recording of the SB cores
 * that raised the interrupt.
 */
void __init
init_IRQ(void)
{
	int i;
	uint32 coreidx;
	void *regs;

	/* Cache chipc and mips33 config registers */
	ASSERT(bcm947xx_sbh);
	coreidx = sb_coreidx(bcm947xx_sbh);
	if ((regs = sb_setcore(bcm947xx_sbh, SB_CC, 0)))
		ccsbr = (sbconfig_t *)((ulong)regs + SBCONFIGOFF);
	if ((regs = sb_setcore(bcm947xx_sbh, SB_MIPS33, 0))) {
		mipssbr = (sbconfig_t *)((ulong)regs + SBCONFIGOFF);
		mipsirq = sb_irq(bcm947xx_sbh);
	}
	sb_setcoreidx(bcm947xx_sbh, coreidx);

	/* Cache mips33 sbintvec register */
	if (mipssbr)
		sbintvec = R_REG(NULL, &mipssbr->sbintvec);

	/* Install interrupt controllers */
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = 0;
		irq_desc[i].depth = 1;
		irq_desc[i].handler = i < SBMIPS_NUMIRQS ?
		        &brcm_irq_type :
		        &brcm_irq2_type;
	}
    	set_except_vector(0, brcmIRQ);
}
