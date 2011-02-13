/*
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/serial_reg.h>
#include <linux/interrupt.h>
#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/time.h>

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <bcmnvram.h>
#include <sbconfig.h>
#include <sbextif.h>
#include <sbchipc.h>
#include <sbutils.h>
#include <hndmips.h>
#include <mipsinc.h>
#include <hndcpu.h>
#include <bcmdevs.h>

/* Global SB handle */
extern void *bcm947xx_sbh;
extern spinlock_t bcm947xx_sbh_lock;

/* Convenience */
#define sbh bcm947xx_sbh
#define sbh_lock bcm947xx_sbh_lock

extern int panic_timeout;
static int watchdog = 0;
static u8 *mcr = NULL;

void __init
bcm947xx_time_init(void)
{
	unsigned int hz;
	extifregs_t *eir;

	/*
	 * Use deterministic values for initial counter interrupt
	 * so that calibrate delay avoids encountering a counter wrap.
	 */
	write_c0_count(0);
	write_c0_compare(0xffff);

	if (!(hz = sb_cpu_clock(sbh)))
		hz = 100000000;

	printk("CPU: BCM%04x rev %d at %d MHz\n", sb_chip(sbh), sb_chiprev(sbh),
	       (hz + 500000) / 1000000);

	/* Set MIPS counter frequency for fixed_rate_gettimeoffset() */

   // !!TB - fix for WL-520GU clock rate
   if (sb_chip(sbh) == BCM5354_CHIP_ID && nvram_match("t_fix1", "WL-520GU"))
		   mips_counter_frequency = 100000000; // Fix WL520GUGC clock
   else
		   mips_counter_frequency = hz / 2;

	/* Set watchdog interval in ms */
	watchdog = simple_strtoul(nvram_safe_get("watchdog"), NULL, 0);

	/* Please set the watchdog to 3 sec if it is less than 3 but not equal to 0 */
	if (watchdog > 0) {
		if (watchdog < 3000)
			watchdog = 3000;
	}

	/* Set panic timeout in seconds */
	panic_timeout = watchdog / 1000;

	/* Setup blink */
	if ((eir = sb_setcore(sbh, SB_EXTIF, 0))) {
		sbconfig_t *sb = (sbconfig_t *)((unsigned int) eir + SBCONFIGOFF);
		unsigned long base = EXTIF_CFGIF_BASE(sb_base(readl(&sb->sbadmatch1)));
		mcr = (u8 *) ioremap_nocache(base + UART_MCR, 1);
	}
}

#ifdef CONFIG_HND_BMIPS3300_PROF
extern bool hndprofiling;
#ifdef CONFIG_MIPS64
typedef u_int64_t sbprof_pc;
#else
typedef u_int32_t sbprof_pc;
#endif
extern void sbprof_cpu_intr(sbprof_pc restartpc);
#endif	/* CONFIG_HND_BMIPS3300_PROF */

static void
bcm947xx_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
#ifdef CONFIG_HND_BMIPS3300_PROF
	/*
	 * Are there any ExcCode or other mean(s) to determine what has caused
	 * the timer interrupt? For now simply stop the normal timer proc if
	 * count register is less than compare register.
	 */
	if (hndprofiling) {
		sbprof_cpu_intr(regs->cp0_epc +
		                ((regs->cp0_cause >> (CAUSEB_BD - 2)) & 4));
		if (read_c0_count() < read_c0_compare())
			return;
	}
#endif	/* CONFIG_HND_BMIPS3300_PROF */

	/* Generic MIPS timer code */
	timer_interrupt(irq, dev_id, regs);

	/* Set the watchdog timer to reset after the specified number of ms */
	if (watchdog > 0) {
		if (sb_chip(sbh) == BCM5354_CHIP_ID)
			sb_watchdog(sbh, WATCHDOG_CLOCK_5354 / 1000 * watchdog);
		else
			sb_watchdog(sbh, WATCHDOG_CLOCK / 1000 * watchdog);
	}

#ifdef	CONFIG_HWSIM
	(*((int *)0xa0000f1c))++;
#else
	/* Blink one of the LEDs in the external UART */
	if (mcr && !(jiffies % (HZ/2)))
		writeb(readb(mcr) ^ UART_MCR_OUT2, mcr);
#endif
}

static struct irqaction bcm947xx_timer_irqaction = {
	bcm947xx_timer_interrupt,
	SA_INTERRUPT,
	0,
	"timer",
	NULL,
	NULL
};

void __init
bcm947xx_timer_setup(struct irqaction *irq)
{
	/* Enable the timer interrupt */
	setup_irq(7, &bcm947xx_timer_irqaction);
}
