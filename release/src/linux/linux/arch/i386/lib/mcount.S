/*
 * Copyright (C) 2000 SGI
 *
 * Written by Dimitris Michailidis dimitris@sgi.com
 *
 * This file implements mcount(), which is used to collect profiling data.
 * We provide several variants to accomodate different types of callers at
 * the lowest possible overhead.
 */

#include <linux/config.h>
#include <linux/linkage.h>

#define MCOUNT_HEAD  \
	pushl %ecx          /* We must protect the arguments of FASTCALLs */; \
	movl mcount_hook, %ecx;  \
	testl %ecx, %ecx;  \
	jz 1f;  \
	pushl %eax;  \
	pushl %edx;  \
        movl 12(%esp), %edx  /* mcount()'s parent */

#define MCOUNT_TAIL \
	call *%ecx;  \
	popl %edx;  \
	popl %eax;  \
1:	popl %ecx

/*
 * This is the main variant and is called by C code.  GCC's -pg option
 * automatically instruments every C function with a call to this.
 */
ENTRY(mcount)
#if defined(CONFIG_KERNPROF) || defined(CONFIG_KERNPROF_MODULE)
	MCOUNT_HEAD
#ifdef CONFIG_FRAME_POINTER
        movl 4(%ebp), %eax  /* mcount()'s parent's parent */
#endif
	MCOUNT_TAIL
#endif
	ret

/*
 * This variant is used by assembly functions.  Must be inserted by hand.
 */
ENTRY(mcount_asm)
#if defined(CONFIG_KERNPROF) || defined(CONFIG_KERNPROF_MODULE)
	MCOUNT_HEAD
        movl 16(%esp), %eax  /* mcount()'s parent's parent */
	MCOUNT_TAIL
#endif
	ret
/*
 * This variant is used by assembly functions in section .stext.lock.
 * Must be inserted by hand.
 */
ENTRY(mcount_stext_lock)
#if defined(CONFIG_KERNPROF) || defined(CONFIG_KERNPROF_MODULE)
	MCOUNT_HEAD
        movl 16(%esp), %eax  /* mcount()'s parent's parent */
	addl 1(%eax), %eax   /* this and the next lines are magic */
	leal 5(%eax), %eax
	MCOUNT_TAIL
#endif
	ret

