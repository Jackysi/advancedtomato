/*
 *  linux/arch/x86-64/kernel/time.c
 *
 *  "High Precision Event Timer" based timekeeping.
 *
 *  Copyright (c) 1991,1992,1995  Linus Torvalds
 *  Copyright (c) 1994  Alan Modra
 *  Copyright (c) 1995  Markus Kuhn
 *  Copyright (c) 1996  Ingo Molnar
 *  Copyright (c) 1998  Andrea Arcangeli
 *  Copyright (c) 2002  Vojtech Pavlik
 *
 */

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/mc146818rtc.h>
#include <linux/irq.h>
#include <linux/ioport.h>
#include <asm/vsyscall.h>
#include <asm/timex.h>

extern rwlock_t xtime_lock;
spinlock_t rtc_lock = SPIN_LOCK_UNLOCKED;
spinlock_t i8253_lock = SPIN_LOCK_UNLOCKED;

unsigned int cpu_khz;					/* TSC clocks / usec, not used here */
unsigned long hpet_address;
unsigned long hpet_period;				/* fsecs / HPET clock */
unsigned int hpet_tick;					/* HPET clocks / interrupt */
unsigned long vxtime_hz = 1193182;
int report_lost_ticks;					/* command line option */

struct vxtime_data __vxtime __section_vxtime;		/* data for vsyscall */

volatile unsigned long __jiffies __section_jiffies;
unsigned long __wall_jiffies __section_wall_jiffies;
struct timeval __xtime __section_xtime;
struct timezone __sys_tz __section_sys_tz;

static inline void rdtscll_sync(unsigned long *tsc)
{
	sync_core();
	rdtscll(*tsc);
}

/*
 * do_gettimeoffset() returns microseconds since last timer interrupt was
 * triggered by hardware. 
 */

extern spinlock_t i8259A_lock;

/* This function must be called with interrupts disabled
 * It was inspired by Steve McCanne's microtime-i386 for BSD.  -- jrs
 *
 * However, the pc-audio speaker driver changes the divisor so that
 * it gets interrupted rather more often - it loads 64 into the
 * counter rather than 11932! This has an adverse impact on
 * do_gettimeoffset() -- it stops working! What is also not
 * good is that the interval that our timer function gets called
 * is no longer 10.0002 ms, but 9.9767 ms. To get around this
 * would require using a different timing source. Maybe someone
 * could use the RTC - I know that this can interrupt at frequencies
 * ranging from 8192Hz to 2Hz. If I had the energy, I'd somehow fix
 * it so that at startup, the timer code in sched.c would select
 * using either the RTC or the 8253 timer. The decision would be
 * based on whether there was any other device around that needed
 * to trample on the 8253. I'd set up the RTC to interrupt at 1024 Hz,
 * and then do some jiggery to have a version of do_timer that
 * advanced the clock by 1/1024 s. Every time that reached over 1/100
 * of a second, then do all the old code. If the time was kept correct
 * then do_gettimeoffset could just return 0 - there is no low order
 * divider that can be accessed.
 *
 * Ideally, you would be able to use the RTC for the speaker driver,
 * but it appears that the speaker driver really needs interrupt more
 * often than every 120 us or so.
 *
 * Anyway, this needs more thought....		pjsg (1993-08-28)
 *
 * If you are really that interested, you should be reading
 * comp.protocols.time.ntp!
 *
 * X86_64 port from arch/i386/kernel/time.c - KS (2007-03-12)
 */

static unsigned long do_slow_gettimeoffset(void)
{
	int count;

	static int count_p = -1;    /* for the first call after boot */
	static unsigned long jiffies_p = 0;

	/*
	 * cache volatile jiffies temporarily; we have IRQs turned off.
	 */
	unsigned long jiffies_t;

	if (count_p < 0)
		count_p = LATCH;    /* LATCH is not a constant on x86_64 */

	/* gets recalled with irq locally disabled */
	spin_lock(&i8253_lock);
	/* timer count may underflow right here */
	outb_p(0x00, 0x43);	/* latch the count ASAP */

	count = inb_p(0x40);	/* read the latched count */

	/*
	 * We do this guaranteed double memory access instead of a _p
	 * postfix in the previous port access. Wheee, hackady hack
	 */
	jiffies_t = jiffies;

	count |= inb_p(0x40) << 8;

        /* VIA686a test code... reset the latch if count > max + 1 */
        if (count > LATCH) {
                outb_p(0x34, 0x43);
                outb_p(LATCH & 0xff, 0x40);
                outb(LATCH >> 8, 0x40);
                count = LATCH - 1;
        }

	spin_unlock(&i8253_lock);

	/*
	 * avoiding timer inconsistencies (they are rare, but they happen)...
	 * there are two kinds of problems that must be avoided here:
	 *  1. the timer counter underflows
	 *  2. hardware problem with the timer, not giving us continuous time,
	 *     the counter does small "jumps" upwards on some Pentium systems,
	 *     (see c't 95/10 page 335 for Neptun bug.)
	 */

	if( jiffies_t == jiffies_p ) {
		if( count > count_p ) {
			/* the nutcase */

			int i;

			spin_lock(&i8259A_lock);
			/*
			 * This is tricky when I/O APICs are used;
			 * see do_timer_interrupt().
			 */
			i = inb(0x20);
			spin_unlock(&i8259A_lock);

			/* assumption about timer being IRQ0 */
			if (i & 0x01) {
				/*
				 * We cannot detect lost timer interrupts ...
				 * well, that's why we call them lost, don't we? :)
				 * [hmm, on the Pentium and Alpha we can ... sort of]
				 */
				count -= LATCH;
			} else
				printk("do_slow_gettimeoffset(): hardware timer problem?\n");
		}
	} else
		jiffies_p = jiffies_t;

	count_p = count;

	count = ((LATCH-1) - count) * tick;
	count = (count + LATCH/2) / LATCH;

	return count;
}

static unsigned int do_gettimeoffset_tsc(void)
{
	unsigned long t;
	rdtscll_sync(&t);	
	return ((t  - vxtime.last_tsc) * vxtime.tsc_quot) >> 32;
}

static unsigned int do_gettimeoffset_hpet(void)
{
	return ((hpet_readl(HPET_COUNTER) - vxtime.last) * vxtime.quot) >> 32;
}

static unsigned int do_gettimeoffset_pit(void)
{
	unsigned long usec, flags;
	read_lock_irqsave(&xtime_lock, flags);
	usec = do_slow_gettimeoffset();
	read_unlock_irqrestore(&xtime_lock, flags);
	return usec;
}

unsigned int (*do_gettimeoffset)(void) = do_gettimeoffset_tsc;

/*
 * This version of gettimeofday() has microsecond resolution and better than
 * microsecond precision, as we're using at least a 10 MHz (usually 14.31818
 * MHz) HPET timer.
 */

void do_gettimeofday(struct timeval *tv)
{
	unsigned long sequence;
 	unsigned int sec, usec;

	do { 
		sequence = __vxtime_sequence[1];
		rmb();

	sec = xtime.tv_sec;
	usec = xtime.tv_usec
		+ (jiffies - wall_jiffies) * tick
			+ do_gettimeoffset();

		rmb(); 
	} while (sequence != __vxtime_sequence[0]);

	tv->tv_sec = sec + usec / 1000000;
	tv->tv_usec = usec % 1000000;
}

/*
 * settimeofday() first undoes the correction that gettimeofday would do
 * on the time, and then saves it. This is ugly, but has been like this for
 * ages already.
 */

void do_settimeofday(struct timeval *tv)
{
	write_lock_irq(&xtime_lock);
	vxtime_lock();

	tv->tv_usec -= (jiffies - wall_jiffies) * tick
			+ do_gettimeoffset();

	while (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	vxtime_unlock();

	time_adjust = 0;		/* stop active adjtime() */
	time_status |= STA_UNSYNC;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;

	write_unlock_irq(&xtime_lock);
}

/*
 * In order to set the CMOS clock precisely, set_rtc_mmss has to be called 500
 * ms after the second nowtime has started, because when nowtime is written
 * into the registers of the CMOS clock, it will jump to the next second
 * precisely 500 ms later. Check the Motorola MC146818A or Dallas DS12887 data
 * sheet for details.
 */

static void set_rtc_mmss(unsigned long nowtime)
{
	int real_seconds, real_minutes, cmos_minutes;
	unsigned char control, freq_select;

/*
 * IRQs are disabled when we're called from the timer interrupt,
 * no need for spin_lock_irqsave()
 */

	spin_lock(&rtc_lock);

/*
 * Tell the clock it's being set and stop it.
 */

	control = CMOS_READ(RTC_CONTROL);
	CMOS_WRITE(control | RTC_SET, RTC_CONTROL);

	freq_select = CMOS_READ(RTC_FREQ_SELECT);
	CMOS_WRITE(freq_select | RTC_DIV_RESET2, RTC_FREQ_SELECT);

	cmos_minutes = CMOS_READ(RTC_MINUTES);
	BCD_TO_BIN(cmos_minutes);

/*
 * since we're only adjusting minutes and seconds, don't interfere with hour
 * overflow. This avoids messing with unknown time zones but requires your RTC
 * not to be off by more than 15 minutes. Since we're calling it only when
 * our clock is externally synchronized using NTP, this shouldn't be a problem.
 */

	real_seconds = nowtime % 60;
	real_minutes = nowtime / 60;
	if (((abs(real_minutes - cmos_minutes) + 15) / 30) & 1)
		real_minutes += 30;	/* correct for half hour time zone */
	real_minutes %= 60;

	if (abs(real_minutes - cmos_minutes) < 30) {
		BIN_TO_BCD(real_seconds);
		BIN_TO_BCD(real_minutes);
		CMOS_WRITE(real_seconds, RTC_SECONDS);
		CMOS_WRITE(real_minutes, RTC_MINUTES);
	} else
		printk(KERN_WARNING "time.c: can't update CMOS clock from %d to %d\n",
			cmos_minutes, real_minutes);

/*
 * The following flags have to be released exactly in this order, otherwise the
 * DS12887 (popular MC146818A clone with integrated battery and quartz) will
 * not reset the oscillator and will not update precisely 500 ms later. You
 * won't find this mentioned in the Dallas Semiconductor data sheets, but who
 * believes data sheets anyway ... -- Markus Kuhn
 */

	CMOS_WRITE(control, RTC_CONTROL);
	CMOS_WRITE(freq_select, RTC_FREQ_SELECT);

	spin_unlock(&rtc_lock);
}

static void timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	static unsigned long rtc_update = 0;

/*
 * Here we are in the timer irq handler. We have irqs locally disabled (so we
 * don't need spin_lock_irqsave()) but we don't know if the timer_bh is running
 * on the other CPU, so we need a lock. We also need to lock the vsyscall
 * variables, because both do_timer() and us change them -arca+vojtech
 */

	write_lock(&xtime_lock);
	vxtime_lock();

	{
		long tsc;
		int delay, offset = 0;

		if (hpet_address) {

			offset = hpet_readl(HPET_T0_CMP) - hpet_tick;
			delay = hpet_readl(HPET_COUNTER) - offset;

		} else {

			spin_lock(&i8253_lock);
			outb_p(0x00, 0x43);
			delay = inb_p(0x40);
			delay |= inb(0x40) << 8;
			spin_unlock(&i8253_lock);
			delay = LATCH - 1 - delay;
		}

		rdtscll_sync(&tsc);

		if (vxtime.mode == VXTIME_HPET) {

			if (offset - vxtime.last > hpet_tick) {
				if (report_lost_ticks)
					printk(KERN_WARNING "time.c: Lost %d timer tick(s)! (rip %016lx)\n",
						(offset - vxtime.last) / hpet_tick - 1, regs->rip);
				jiffies += (offset - vxtime.last) / hpet_tick - 1;
			}

			vxtime.last = offset;

		} else {

			offset = (((tsc - vxtime.last_tsc) * vxtime.tsc_quot) >> 32) - tick;

			if (offset > tick) {
				if (report_lost_ticks)
					printk(KERN_WARNING "time.c: lost %ld tick(s) (rip %016lx)\n",
						 offset / tick, regs->rip);
				jiffies += offset / tick;
				offset %= tick;
			}

			vxtime.last_tsc = tsc - vxtime.quot * delay / vxtime.tsc_quot;

			if ((((tsc - vxtime.last_tsc) * vxtime.tsc_quot) >> 32) < offset)
				vxtime.last_tsc = tsc - (((long)offset << 32) / vxtime.tsc_quot) - 1;

		}
	}

/*
 * Do the timer stuff.
 */

	do_timer(regs);

/*
 * If we have an externally synchronized Linux clock, then update CMOS clock
 * accordingly every ~11 minutes. set_rtc_mmss() will be called in the jiffy
 * closest to exactly 500 ms before the next second. If the update fails, we
 * don'tcare, as it'll be updated on the next turn, and the problem (time way
 * off) isn't likely to go away much sooner anyway.
 */

	if ((~time_status & STA_UNSYNC) && xtime.tv_sec > rtc_update &&
		abs(xtime.tv_usec - 500000) <= tick / 2) {
		set_rtc_mmss(xtime.tv_sec);
		rtc_update = xtime.tv_sec + 660;
	}

	vxtime_unlock();
	write_unlock(&xtime_lock);
}

static unsigned long get_cmos_time(void)
{
	unsigned int timeout, year, mon, day, hour, min, sec;
	unsigned char last, this;

/*
 * The Linux interpretation of the CMOS clock register contents: When the
 * Update-In-Progress (UIP) flag goes from 1 to 0, the RTC registers show the
 * second which has precisely just started. Waiting for this can take up to 1
 * second, we timeout approximately after 2.4 seconds on a machine with
 * standard 8.3 MHz ISA bus.
 */

	spin_lock(&rtc_lock);

	timeout = 1000000;
	last = this = 0;

	while (timeout && last && !this) {
		last = this;
		this = CMOS_READ(RTC_FREQ_SELECT) & RTC_UIP;
		timeout--;
	}

/*
 * Here we are safe to assume the registers won't change for a whole second, so
 * we just go ahead and read them.
 */

	sec = CMOS_READ(RTC_SECONDS);
	min = CMOS_READ(RTC_MINUTES);
	hour = CMOS_READ(RTC_HOURS);
	day = CMOS_READ(RTC_DAY_OF_MONTH);
	mon = CMOS_READ(RTC_MONTH);
	year = CMOS_READ(RTC_YEAR);

	spin_unlock(&rtc_lock);

/*
 * We know that x86-64 always uses BCD format, no need to check the config
 * register.
 */

	BCD_TO_BIN(sec);
	BCD_TO_BIN(min);
	BCD_TO_BIN(hour);
	BCD_TO_BIN(day);
	BCD_TO_BIN(mon);
	BCD_TO_BIN(year);

/*
 * This will work up to Dec 31, 2069.
 */

	if ((year += 1900) < 1970)
		year += 100;

	return mktime(year, mon, day, hour, min, sec);
}

/*
 * calibrate_tsc() calibrates the processor TSC in a very simple way, comparing
 * it to the HPET timer of known frequency.
 */

#define TICK_COUNT 100000000

static unsigned int __init hpet_calibrate_tsc(void)
{
	int tsc_start, hpet_start;
	int tsc_now, hpet_now;
	unsigned long flags;

	__save_flags(flags);
	__cli();

	hpet_start = hpet_readl(HPET_COUNTER);
	rdtscl(tsc_start);

	do {
		__cli();
		hpet_now = hpet_readl(HPET_COUNTER);
		sync_core();
		rdtscl(tsc_now);
		__restore_flags(flags);
	} while ((tsc_now - tsc_start) < TICK_COUNT && (hpet_now - hpet_start) < TICK_COUNT);

	return (tsc_now - tsc_start) * 1000000000L
		/ ((hpet_now - hpet_start) * hpet_period / 1000);
}

/*
 * pit_calibrate_tsc() uses the speaker output (channel 2) of
 * the PIT. This is better than using the timer interrupt output,
 * because we can read the value of the speaker with just one inb(),
 * where we need three i/o operations for the interrupt channel.
 * We count how many ticks the TSC does in 50 ms.
 */

static unsigned int __init pit_calibrate_tsc(void)
{
	unsigned long start, end;

	outb((inb(0x61) & ~0x02) | 0x01, 0x61);

	spin_lock_irq(&i8253_lock);

	outb(0xb0, 0x43);
	outb((1193182 / (1000 / 50)) & 0xff, 0x42);
	outb((1193182 / (1000 / 50)) >> 8, 0x42);
	rdtscll(start);

	while ((inb(0x61) & 0x20) == 0);
	rdtscll(end);

	spin_unlock_irq(&i8253_lock);

	return (end - start) / 50;
}

static int hpet_init(void)
{
	unsigned int cfg, id;

	if (!hpet_address)
		return -1;
	set_fixmap_nocache(FIX_HPET_BASE, hpet_address);

/*
 * Read the period, compute tick and quotient.
 */

	id = hpet_readl(HPET_ID);

	if (!(id & HPET_ID_VENDOR) || !(id & HPET_ID_NUMBER) || !(id & HPET_ID_LEGSUP))
		return -1;

	hpet_period = hpet_readl(HPET_PERIOD);
	if (hpet_period < 100000 || hpet_period > 100000000)
		return -1;

	hpet_tick = (1000000000L * tick + hpet_period / 2) / hpet_period;

/*
 * Stop the timers and reset the main counter.
 */

	cfg = hpet_readl(HPET_CFG);
	cfg &= ~(HPET_CFG_ENABLE | HPET_CFG_LEGACY);
	hpet_writel(cfg, HPET_CFG);
	hpet_writel(0, HPET_COUNTER);
	hpet_writel(0, HPET_COUNTER + 4);

/*
 * Set up timer 0, as periodic with first interrupt to happen at hpet_tick,
 * and period also hpet_tick.
 */

	hpet_writel(HPET_T0_ENABLE | HPET_T0_PERIODIC | HPET_T0_SETVAL | HPET_T0_32BIT, HPET_T0_CFG);
	hpet_writel(hpet_tick, HPET_T0_CMP);
	hpet_writel(hpet_tick, HPET_T0_CMP);

/*
 * Go!
 */

	cfg |= HPET_CFG_ENABLE | HPET_CFG_LEGACY;
	hpet_writel(cfg, HPET_CFG);

	return 0;
}

void __init pit_init(void)
{
	spin_lock_irq(&i8253_lock);
	outb_p(0x34, 0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff, 0x40);	/* LSB */
	outb_p(LATCH >> 8, 0x40);	/* MSB */
	spin_unlock_irq(&i8253_lock);
}

static int __init time_setup(char *str)
{
	report_lost_ticks = 1;
	return 1;
}

/* Only used on SMP */
static int notsc __initdata = 0; 

static int __init notsc_setup(char *str)
{ 
#ifdef CONFIG_SMP
	printk(KERN_INFO "notsc ignored on non SMP kernel\n"); 
#endif
	notsc = 1;
	return 1;
} 

static struct irqaction irq0 = { timer_interrupt, SA_INTERRUPT, 0, "timer", NULL, NULL};

void __init time_init(void)
{
	char *timename;

#ifdef HPET_HACK_ENABLE_DANGEROUS
        if (!hpet_address) {
		printk(KERN_WARNING "time.c: WARNING: Enabling HPET base manually!\n");
                outl(0x800038a0, 0xcf8);
                outl(0xff000001, 0xcfc);
                outl(0x800038a0, 0xcf8);
                hpet_address = inl(0xcfc) & 0xfffffffe;
		printk(KERN_WARNING "time.c: WARNING: Enabled HPET at at %#lx.\n", hpet_address);
        }
#endif

#ifndef CONFIG_HPET_TIMER
        hpet_address = 0;
#endif

	write_lock(&xtime_lock);
	xtime.tv_sec = get_cmos_time();
	xtime.tv_usec = 0;
	write_unlock(&xtime_lock);

	if (!hpet_init()) {
                vxtime_hz = (1000000000000000L + hpet_period / 2) / hpet_period;
                cpu_khz = hpet_calibrate_tsc();
		timename = "HPET";
	} else {
		pit_init();
		cpu_khz = pit_calibrate_tsc();
		timename = "PIT";
	}

	vxtime.mode = VXTIME_TSC;
	vxtime.quot = (1000000L << 32) / vxtime_hz;
	vxtime.tsc_quot = (1000L << 32) / cpu_khz;
	rdtscll_sync(&vxtime.last_tsc);

	setup_irq(0, &irq0);

        printk(KERN_INFO "time.c: Detected %ld.%06ld MHz %s timer.\n",
		vxtime_hz / 1000000, vxtime_hz % 1000000, timename);
	printk(KERN_INFO "time.c: Detected %d.%03d MHz TSC timer.\n",
			cpu_khz / 1000, cpu_khz % 1000);
}

void __init time_init_smp(void)
{
	char *timetype;

	if (hpet_address) {
		if (notsc) { 
			timetype = "HPET";
			vxtime.last = hpet_readl(HPET_T0_CMP) - hpet_tick;
			vxtime.mode = VXTIME_HPET;
			do_gettimeoffset = do_gettimeoffset_hpet;
		} else {
			timetype = "HPET/TSC";
			vxtime.mode = VXTIME_TSC;
		}		
	} else {
		if (notsc) { 
			timetype = "PIT"; 
			vxtime.mode = VXTIME_STUPID; 
			do_gettimeoffset = do_gettimeoffset_pit;
		} else { 
			timetype = "PIT/TSC";
			vxtime.mode = VXTIME_TSC;
	}
	}
	printk(KERN_INFO "time.c: Using %s based timekeeping.\n", timetype);
}

__setup("notsc", notsc_setup);
__setup("report_lost_ticks", time_setup);

