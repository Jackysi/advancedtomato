/* $Id: timer.h,v 1.1.1.4 2003/10/14 08:09:23 sparq Exp $
 * timer.h: System timer definitions for sun5.
 *
 * Copyright (C) 1997 David S. Miller (davem@caip.rutgers.edu)
 */

#ifndef _SPARC64_TIMER_H
#define _SPARC64_TIMER_H

/* How timers work:
 *
 * On uniprocessors we just use counter zero for the system wide
 * ticker, this performs thread scheduling, clock book keeping,
 * and runs timer based events.  Previously we used the Ultra
 * %tick interrupt for this purpose.
 *
 * On multiprocessors we pick one cpu as the master level 10 tick
 * processor.  Here this counter zero tick handles clock book
 * keeping and timer events only.  Each Ultra has it's level
 * 14 %tick interrupt set to fire off as well, even the master
 * tick cpu runs this locally.  This ticker performs thread
 * scheduling, system/user tick counting for the current thread,
 * and also profiling if enabled.
 */

#include <linux/config.h>

struct sun5_timer {
	u64	count0;
	u64	limit0;
	u64	count1;
	u64	limit1;
};

#define SUN5_LIMIT_ENABLE	0x80000000
#define SUN5_LIMIT_TOZERO	0x40000000
#define SUN5_LIMIT_ZRESTART	0x20000000
#define SUN5_LIMIT_CMASK	0x1fffffff

/* Given a HZ value, set the limit register to so that the timer IRQ
 * gets delivered that often.
 */
#define SUN5_HZ_TO_LIMIT(__hz)  (1000000/(__hz))

#ifdef CONFIG_SMP
extern unsigned long timer_tick_offset;
extern void timer_tick_interrupt(struct pt_regs *);
#endif

#endif /* _SPARC64_TIMER_H */
