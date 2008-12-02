/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Setting up the clock on the MIPS boards.
 *
 */
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/mc146818rtc.h>
#include <linux/timex.h>

#include <asm/mipsregs.h>
#include <asm/ptrace.h>
#include <asm/it8172/it8172_int.h>
#include <asm/debug.h>

static unsigned long r4k_offset; /* Amount to increment compare reg each time */
static unsigned long r4k_cur;    /* What counter should be at next timer irq */
extern unsigned int mips_counter_frequency;
extern asmlinkage unsigned int do_IRQ(int irq, struct pt_regs *regs);

/*
 * Figure out the r4k offset, the amount to increment the compare
 * register for each time tick.
 * Use the RTC to calculate offset.
 */
static unsigned long __init cal_r4koff(void)
{
	unsigned int flags;

	__save_and_cli(flags);

	/* Start counter exactly on falling edge of update flag */
	while (CMOS_READ(RTC_REG_A) & RTC_UIP);
	while (!(CMOS_READ(RTC_REG_A) & RTC_UIP));

	/* Start r4k counter. */
	write_c0_count(0);

	/* Read counter exactly on falling edge of update flag */
	while (CMOS_READ(RTC_REG_A) & RTC_UIP);
	while (!(CMOS_READ(RTC_REG_A) & RTC_UIP));

	mips_counter_frequency = read_c0_count();

	/* restore interrupts */
	__restore_flags(flags);

	return (mips_counter_frequency / HZ);
}

unsigned long it8172_rtc_get_time(void)
{
	unsigned int year, mon, day, hour, min, sec;
	unsigned char save_control;

	save_control = CMOS_READ(RTC_CONTROL);

	/* Freeze it. */
	CMOS_WRITE(save_control | RTC_SET, RTC_CONTROL);

	/* Read regs. */
	sec = CMOS_READ(RTC_SECONDS);
	min = CMOS_READ(RTC_MINUTES);
	hour = CMOS_READ(RTC_HOURS);

	if (!(save_control & RTC_24H))
	{
		if ((hour & 0xf) == 0xc)
		        hour &= 0x80;
	        if (hour & 0x80)
		        hour = (hour & 0xf) + 12;
	}
	day = CMOS_READ(RTC_DAY_OF_MONTH);
	mon = CMOS_READ(RTC_MONTH);
	year = CMOS_READ(RTC_YEAR);

	/* Unfreeze clock. */
	CMOS_WRITE(save_control, RTC_CONTROL);

	if ((year += 1900) < 1970)
	        year += 100;

	return mktime(year, mon, day, hour, min, sec);
}

void __init it8172_time_init(void)
{
        unsigned int est_freq, flags;

	__save_and_cli(flags);
        /* Set Data mode - binary. */
        CMOS_WRITE(CMOS_READ(RTC_CONTROL) | RTC_DM_BINARY, RTC_CONTROL);

	printk("calculating r4koff... ");
	r4k_offset = cal_r4koff();
	printk("%08lx(%d)\n", r4k_offset, (int) r4k_offset);

	est_freq = 2*r4k_offset*HZ;
	est_freq += 5000;    /* round */
	est_freq -= est_freq%10000;
	printk("CPU frequency %d.%02d MHz\n", est_freq/1000000,
	       (est_freq%1000000)*100/1000000);
	__restore_flags(flags);
}

#define ALLINTS (IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ4 | IE_IRQ5)
void __init it8172_timer_setup(struct irqaction *irq)
{
	puts("timer_setup\n");
	put32(NR_IRQS);
	puts("");
        /* we are using the cpu counter for timer interrupts */
	setup_irq(MIPS_CPU_TIMER_IRQ, irq);

        /* to generate the first timer interrupt */
	r4k_cur = (read_c0_count() + r4k_offset);
	write_c0_compare(r4k_cur);
	set_c0_status(ALLINTS);
}
