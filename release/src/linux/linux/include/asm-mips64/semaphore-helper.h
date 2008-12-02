/*
 * SMP- and interrupt-safe semaphores helper functions.
 *
 * (C) Copyright 1996 Linus Torvalds
 * (C) Copyright 1999 Andrea Arcangeli
 * (C) Copyright 1999, 2001, 2002 Ralf Baechle
 * (C) Copyright 1999, 2001 Silicon Graphics, Inc.
 */
#ifndef _ASM_SEMAPHORE_HELPER_H
#define _ASM_SEMAPHORE_HELPER_H

#include <linux/errno.h>

/*
 * These two _must_ execute atomically wrt each other.
 */
static inline void wake_one_more(struct semaphore * sem)
{
	atomic_inc(&sem->waking);
}

static inline int waking_non_zero(struct semaphore *sem)
{
	int ret, tmp;

	__asm__ __volatile__(
	"1:\tll\t%1, %2\t\t\t# waking_non_zero\n\t"
	"blez\t%1, 2f\n\t"
	"subu\t%0, %1, 1\n\t"
	"sc\t%0, %2\n\t"
	"beqz\t%0, 1b\n"
	"2:"
	: "=r" (ret), "=r" (tmp), "+m" (sem->waking)
	: "0" (0));

	return ret;
}

/*
 * waking_non_zero_interruptible:
 *	1	got the lock
 *	0	go to sleep
 *	-EINTR	interrupted
 *
 * We must undo the sem->count down_interruptible decrement
 * simultaneously and atomically with the sem->waking adjustment,
 * otherwise we can race with wake_one_more.
 *
 * This is accomplished by doing a 64-bit lld/scd on the 2 32-bit words.
 *
 * Pseudocode:
 *
 * If(sem->waking > 0) {
 *	Decrement(sem->waking)
 *	Return(SUCCESS)
 * } else If(signal_pending(tsk)) {
 *	Increment(sem->count)
 *	Return(-EINTR)
 * } else {
 *	Return(SLEEP)
 * }
 */

static inline int
waking_non_zero_interruptible(struct semaphore *sem, struct task_struct *tsk)
{
	long ret, tmp;

        __asm__ __volatile__(
	".set\tpush\t\t\t# waking_non_zero_interruptible\n\t"
	".set\tnoat\n\t"
	"0:\tlld\t%1, %2\n\t"
	"li\t%0, 0\n\t"
	"sll\t$1, %1, 0\n\t"
	"blez\t$1, 1f\n\t"
	"daddiu\t%1, %1, -1\n\t"
	"li\t%0, 1\n\t"
	"b\t2f\n\t"
	"1:\tbeqz\t%3, 2f\n\t"
	"li\t%0, %4\n\t"
	"dli\t$1, 0x0000000100000000\n\t"
	"daddu\t%1, %1, $1\n\t"
	"2:\tscd\t%1, %2\n\t"
	"beqz\t%1, 0b\n\t"
	".set\tpop"
	: "=&r" (ret), "=&r" (tmp), "=m" (*sem)
	: "r" (signal_pending(tsk)), "i" (-EINTR));

	return ret;
}

/*
 * waking_non_zero_trylock is unused.  we do everything in
 * down_trylock and let non-ll/sc hosts bounce around.
 */

static inline int waking_non_zero_trylock(struct semaphore *sem)
{
#if WAITQUEUE_DEBUG
	CHECK_MAGIC(sem->__magic);
#endif

	return 0;
}

#endif /* _ASM_SEMAPHORE_HELPER_H */
