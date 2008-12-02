/*
 * 
 * Copyright (C) SGI 1999, 2000
 *
 * Written by Dimitris Michailidis (dimitris@engr.sgi.com)
 */

#ifndef _ASM_KERNPROF_H
#define _ASM_KERNPROF_H

#ifdef __KERNEL__

#include <asm/system.h>
#include <asm/ptrace.h>
#include <asm/msr.h>
#include <asm/processor.h>

#define DFL_PC_RES 4		/* default PC resolution for this platform */

/*
 * When executing in the BIOS or PROM (happens with APM) we get kernel-mode
 * addresses below &_stext.
 */ 
#define in_firmware(regs) ((regs)->eip < (unsigned long) &_stext)

struct st_limits {              /* valid kernel stack is between bot & top */
	unsigned long *top;
	unsigned long *bot;
};

struct frame_info {
	struct st_limits limits;
	unsigned long *frame_ptr; /* saved ebp */
	unsigned long pc;         /* saved eip */
};

typedef struct frame_info frame_info_t;

#define frame_get_pc(p) ((p)->pc)

static __inline__ void get_stack_limits(struct pt_regs *regs,
					struct st_limits *p)
{
	p->top = &regs->esp;
	p->bot = (unsigned long *)((unsigned long) current + THREAD_SIZE);
}

/*
 * A function sets up its stack frame with the instructions
 *
 * 	pushl %ebp
 *	movl %esp, %ebp
 *
 * The timer interrupt may arrive at any time including right at the moment
 * that the new frame is being set up, so we need to distinguish a few cases.
 */
static __inline__ void get_top_frame(struct pt_regs *regs, frame_info_t *p)
{
	unsigned long pc = regs->eip;

	get_stack_limits(regs, &p->limits);
	if (*p->limits.top == regs->ebp) {         /* between pushl and movl */
		p->frame_ptr = p->limits.top;
		p->pc = pc;
	} else {
		p->frame_ptr = (unsigned long *) regs->ebp;
		if (*(unsigned char *)pc == 0x55)  /* right at pushl %ebp */
			p->pc = *p->limits.top;
		else
			p->pc = pc;
	}
}

/* Fabricate a stack frame that is sufficient to begin walking up the stack */ 
static __inline__ int build_fake_frame(frame_info_t *p)
{
	__asm__ __volatile__("movl %%esp,%0" : "=m" (p->limits.top));
	p->limits.bot = (unsigned long *)((unsigned long)current + THREAD_SIZE);
	__asm__ __volatile__("movl %%ebp,%0" : "=m" (p->frame_ptr));
	p->pc = (unsigned long)current_text_addr();
	return 1;
}

/* This macro determines whether there are more frames to go on the stack */
#define last_frame(p) \
	((p)->frame_ptr < (p)->limits.top || (p)->frame_ptr >= (p)->limits.bot)

static __inline__ int get_next_frame(frame_info_t *p)
{
	if (last_frame(p))
		return 0;
	p->pc = p->frame_ptr[1];
	p->frame_ptr = (unsigned long *) *p->frame_ptr;
	return 1;
}

/* These are called by mcount() so we want them to be fast. */
void cg_record_arc(unsigned long, unsigned long) __attribute__((regparm(2)));
void record_fn_call(unsigned long, unsigned long) __attribute__((regparm(2)));

#define supports_call_graph (prof_have_mcount && prof_have_frameptr)

#if defined(CONFIG_MCOUNT)
#define MCOUNT_STEXT_LOCK "call mcount_stext_lock"
#define MCOUNT_ASM        "call mcount_asm"
#else
#define MCOUNT_STEXT_LOCK
#define MCOUNT_ASM
#endif

/* We can do 16-bit compare&swap */
#define __HAVE_ARCH_CMPXCHG16 1

/*
 * Performance counters are supported only on P6-family systems with local APIC
 * since we rely on the overflow interrupts.
 */
#ifdef CONFIG_X86_LOCAL_APIC
#define have_perfctr() (cpu_has_msr && boot_cpu_data.x86 == 6)

#define valid_perfctr_event(e) ((unsigned long)(e) <= 0xFFFFF)
#define valid_perfctr_freq(n)  ((long)(n) >= 0)

#define get_prof_freq()		(HZ * prof_multiplier[0])

#define EVENTSEL0_ENABLE_MASK  0x00500000

#define perfctr_reload(n)	wrmsr(MSR_P6_PERFCTR0, -(int)(n), 0)
#define __perfctr_stop()	wrmsr(MSR_P6_EVNTSEL0, 0, 0)

static __inline__ void __perfctr_commence(unsigned int freq, int evt)
{
	perfctr_reload(freq);
	wrmsr(MSR_P6_EVNTSEL1, 0, 0);
	wrmsr(MSR_P6_EVNTSEL0, EVENTSEL0_ENABLE_MASK | (evt), 0);
}
#else
#define have_perfctr()		0
#define valid_perfctr_event(e)	0
#define valid_perfctr_freq(n)	0
#define perfctr_reload(x)
#define __perfctr_stop()
#define __perfctr_commence(x,y)
#define get_prof_freq()		HZ
#define setup_profiling_timer(x) (-EINVAL)
#endif

#endif /* __KERNEL__ */

#endif /* !_ASM_KERNPROF_H */
