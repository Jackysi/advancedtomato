#ifndef _LINUX_THREADS_H
#define _LINUX_THREADS_H

#include <linux/config.h>

/*
 * The default limit for the nr of threads is now in
 * /proc/sys/kernel/threads-max.
 */
 
#ifdef CONFIG_SMP
#ifdef __mips__
#define NR_CPUS _MIPS_SZLONG
#else
#define NR_CPUS	64		/* Max processors that can be running in SMP */
#endif
#else
#define NR_CPUS 1
#endif

#define MIN_THREADS_LEFT_FOR_ROOT 4

/*
 * This controls the maximum pid allocated to a process
 */
#define PID_MAX 0x8000

#endif
