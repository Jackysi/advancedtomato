/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Time operations for IP22 machines. Original code may come from
 * Ralf Baechle or David S. Miller (sorry guys, i'm really not sure)
 *
 * Copyright (C) 2001 by Ladislav Michl
 */
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/kernel_stat.h>
#include <linux/time.h>

#include <asm/cpu.h>
#include <asm/mipsregs.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/ds1286.h>
#include <asm/sgialib.h>
#include <asm/sgi/sgint23.h>
#include <asm/sgi/sgigio.h>
#include <asm/time.h>

/*
 * note that mktime uses month from 1 to 12 while to_tm
 * uses 0 to 11.
 */
static unsigned long indy_rtc_get_time(void)
{
	unsigned char yrs, mon, day, hrs, min, sec;
	unsigned char save_control;

	save_control = CMOS_READ(RTC_CMD);
	CMOS_WRITE((save_control|RTC_TE), RTC_CMD);

	sec = CMOS_READ(RTC_SECONDS);
	min = CMOS_READ(RTC_MINUTES);
	hrs = CMOS_READ(RTC_HOURS) & 0x1f;
	day = CMOS_READ(RTC_DATE);
	mon = CMOS_READ(RTC_MONTH) & 0x1f;
	yrs = CMOS_READ(RTC_YEAR);

	CMOS_WRITE(save_control, RTC_CMD);

	BCD_TO_BIN(sec);
	BCD_TO_BIN(min);
	BCD_TO_BIN(hrs);
	BCD_TO_BIN(day);
	BCD_TO_BIN(mon);
	BCD_TO_BIN(yrs);

	if (yrs < 45)
		yrs += 30;
	if ((yrs += 40) < 70)
		yrs += 100;

	return mktime((int)yrs + 1900, mon, day, hrs, min, sec);
}

static int indy_rtc_set_time(unsigned long tim)
{
	struct rtc_time tm;
	unsigned char save_control;

	to_tm(tim, &tm);

	tm.tm_mon += 1;		/* tm_mon starts at zero */
	tm.tm_year -= 1940;
	if (tm.tm_year >= 100)
		tm.tm_year -= 100;

	BIN_TO_BCD(tm.tm_sec);
	BIN_TO_BCD(tm.tm_min);
	BIN_TO_BCD(tm.tm_hour);
	BIN_TO_BCD(tm.tm_mday);
	BIN_TO_BCD(tm.tm_mon);
	BIN_TO_BCD(tm.tm_year);

	save_control = CMOS_READ(RTC_CMD);
	CMOS_WRITE((save_control|RTC_TE), RTC_CMD);

	CMOS_WRITE(tm.tm_year, RTC_YEAR);
	CMOS_WRITE(tm.tm_mon, RTC_MONTH);
	CMOS_WRITE(tm.tm_mday, RTC_DATE);
	CMOS_WRITE(tm.tm_hour, RTC_HOURS);
	CMOS_WRITE(tm.tm_min, RTC_MINUTES);
	CMOS_WRITE(tm.tm_sec, RTC_SECONDS);
	CMOS_WRITE(0, RTC_HUNDREDTH_SECOND);

	CMOS_WRITE(save_control, RTC_CMD);

	return 0;
}

static unsigned long dosample(volatile unsigned char *tcwp,
			      volatile unsigned char *tc2p)
{
	u32 ct0, ct1;
	volatile u8 msb, lsb;

	/* Start the counter. */
	*tcwp = (SGINT_TCWORD_CNT2 | SGINT_TCWORD_CALL | SGINT_TCWORD_MRGEN);
	*tc2p = (SGINT_TCSAMP_COUNTER & 0xff);
	*tc2p = (SGINT_TCSAMP_COUNTER >> 8);

	/* Get initial counter invariant */
	ct0 = read_c0_count();

	/* Latch and spin until top byte of counter2 is zero */
	do {
		*tcwp = (SGINT_TCWORD_CNT2 | SGINT_TCWORD_CLAT);
		lsb = *tc2p;
		msb = *tc2p;
		ct1 = read_c0_count();
	} while(msb);

	/* Stop the counter. */
	*tcwp = (SGINT_TCWORD_CNT2 | SGINT_TCWORD_CALL | SGINT_TCWORD_MSWST);

	/*
	 * Return the difference, this is how far the r4k counter increments
	 * for every 1/HZ seconds. We round off the nearest 1 MHz of master
	 * clock (= 1000000 / 100 / 2 = 5000 count).
	 */
	return ((ct1 - ct0) / 5000) * 5000;
}

/*
 * Here we need to calibrate the cycle counter to at least be close.
 */
void indy_time_init(void)
{
	struct sgi_ioc_timers *p;
	volatile unsigned char *tcwp, *tc2p;
	unsigned long r4k_ticks[3];
	unsigned long r4k_tick;

	/* Figure out the r4k offset, the algorithm is very simple
	 * and works in _all_ cases as long as the 8254 counter
	 * register itself works ok (as an interrupt driving timer
	 * it does not because of bug, this is why we are using
	 * the onchip r4k counter/compare register to serve this
	 * purpose, but for r4k_offset calculation it will work
	 * ok for us).  There are other very complicated ways
	 * of performing this calculation but this one works just
	 * fine so I am not going to futz around. ;-)
	 */
	p = ioc_timers;
	tcwp = &p->tcword;
	tc2p = &p->tcnt2;

	printk(KERN_INFO "Calibrating system timer... ");
	dosample(tcwp, tc2p);                   /* Prime cache. */
	dosample(tcwp, tc2p);                   /* Prime cache. */
	/* Zero is NOT an option. */
	do {
		r4k_ticks[0] = dosample (tcwp, tc2p);
	} while (!r4k_ticks[0]);
	do {
		r4k_ticks[1] = dosample (tcwp, tc2p);
	} while (!r4k_ticks[1]);

	if (r4k_ticks[0] != r4k_ticks[1]) {
		printk ("warning: timer counts differ, retrying...");
		r4k_ticks[2] = dosample (tcwp, tc2p);
		if (r4k_ticks[2] == r4k_ticks[0]
		    || r4k_ticks[2] == r4k_ticks[1])
			r4k_tick = r4k_ticks[2];
		else {
			printk ("disagreement, using average...");
			r4k_tick = (r4k_ticks[0] + r4k_ticks[1]
				   + r4k_ticks[2]) / 3;
		}
	} else
		r4k_tick = r4k_ticks[0];

	printk("%d [%d.%02d MHz CPU]\n", (int) r4k_tick,
		(int) (r4k_tick / 5000), (int) (r4k_tick % 5000) / 50);

	mips_counter_frequency = r4k_tick * HZ;
}

/* Generic SGI handler for (spurious) 8254 interrupts */
void indy_8254timer_irq(struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	int irq = SGI_8254_0_IRQ;
	ULONG cnt;
	char c;

	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;
	printk("indy_8254timer_irq: Whoops, should not have gotten this IRQ\n");
	ArcRead(0, &c, 1, &cnt);
	ArcEnterInteractiveMode();
	irq_exit(cpu, irq);
}

void indy_r4k_timer_interrupt(struct pt_regs *regs)
{
	int cpu = smp_processor_id();
	int irq = SGI_TIMER_IRQ;

	irq_enter(cpu, irq);
	kstat.irqs[cpu][irq]++;
	timer_interrupt(irq, NULL, regs);
	irq_exit(cpu, irq);

	if (softirq_pending(cpu))
		do_softirq();
}

extern int setup_irq(unsigned int irq, struct irqaction *irqaction);

static void indy_timer_setup(struct irqaction *irq)
{
	unsigned long count;

	/* over-write the handler, we use our own way */
	irq->handler = no_action;

	/* set time for first interrupt */
	count = read_c0_count();
	count += mips_counter_frequency / HZ;
	write_c0_compare(count);

	/* setup irqaction */
	setup_irq(SGI_TIMER_IRQ, irq);
}

void sgitime_init(void)
{
	/* setup hookup functions */
	rtc_get_time = indy_rtc_get_time;
	rtc_set_time = indy_rtc_set_time;

	board_time_init = indy_time_init;
	board_timer_setup = indy_timer_setup;
}
