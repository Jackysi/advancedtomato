/*
 * Copyright (C) 2000 Silicon Graphics, Inc.
 *
 * Written by Ulf Carlsson (ulfc@engr.sgi.com)
 */

#ifndef _ASM_KERNPROF_H
#define _ASM_KERNPROF_H

#ifdef __KERNEL__

#include <asm/system.h>
#include <asm/ptrace.h>
#include <asm/processor.h>

#define DFL_PC_RES 4		/* default PC resolution for this platform */

#define in_firmware(regs) 0	/* never in the PROM during normal execution */

extern char stext;
extern char _etext;
extern int prof_freq[];

extern int setup_profiling_timer(unsigned int);

typedef struct frame_info frame_info_t;

struct frame_info {
	unsigned long ra;
	unsigned long pc;
	unsigned long sp;
	unsigned long top;
};

#define frame_get_pc(p)		((p)->pc)

/*
 * A function tests up its stack frame with the instructions
 *
 *	addiu	$sp,$sp,-stacksize
 *	sw	$ra,stacksize-8($sp)
 *
 * The timer interrupt may arrive at any time including right at the moment
 * that the new frame is being set up, so we need to distinguish a few cases.
 */
static __inline__ void get_top_frame(struct pt_regs *regs, frame_info_t *p)
{
	unsigned long pc = regs->cp0_epc;

	pc = regs->cp0_epc;

#ifndef CONFIG_SMP
	{
		extern unsigned long kernelsp;
		p->top = kernelsp;
	}
#else
	{
		unsigned int lo, hi;
		lo = read_32bit_cp0_register(CP0_WATCHLO);
		hi = read_32bit_cp0_register(CP0_WATCHHI);
		p->top = ((unsigned long) hi << 32) | lo;
	}
#endif

	do {
		unsigned int inst = *(unsigned int *)pc;
		/* First we look for a ``addiu $sp,$sp,...'' and then we look
		   for a ``jr $ra'' in case this is a leaf function without
		   stack frame.  */
		if ((inst & 0xffff0000) == 0x27bd0000) {
			p->sp = regs->regs[29] - (short) (inst & 0xffff);
			p->ra = *((unsigned long *)p->sp - 1);
			p->pc = regs->cp0_epc;
			return;
		} else if (inst == 0x03e00008) { 
			/* N32 says that routines aren't restricted to a single
			   exit block.  In that case we lose.  The thing is
			   that the .mdebug format doesn't handle that either
			   so we should be pretty safe.  */
			p->sp = regs->regs[29];
			p->ra = regs->regs[31];
			p->pc = regs->cp0_epc;
		}
	} while (--pc > (unsigned long) &stext);

	BUG();
}

static unsigned long this_pc(void)
{
	return (unsigned long)return_address();
}

/* Fabricate a stack frame that is sufficient to begin walking up the stack */
static __inline__ int build_fake_frame(frame_info_t *p)
{
#ifndef CONFIG_SMP
	{
		extern unsigned long kernelsp;
		p->top = kernelsp;
	}
#else
	{
		unsigned int lo, hi;
		lo = read_32bit_cp0_register(CP0_WATCHLO);
		hi = read_32bit_cp0_register(CP0_WATCHHI);
		p->top = ((unsigned long) hi << 32) | lo;
	}
#endif
	__asm__ __volatile__("sw\t$29,%0\t\n" : "=m" (p->sp));
	p->pc = this_pc();
	return 1;
}

static __inline__ int last_frame(frame_info_t *p)
{
	if (p->sp < (unsigned long) current + sizeof(*current))
		BUG();
	
	return (p->sp < p->top);
}

static __inline__ int get_next_frame(frame_info_t *p)
{
	unsigned int *sp = (unsigned int *)p->sp;
	unsigned int *pc = (unsigned int *)p->pc;

	if (last_frame(p))
		return 0;

	/*
	 * First, scan backwards to find the stack-decrement that signals the
	 * beginning of this routine in which we're inlined.  That tells us
	 * how to roll back the stack.
	 */
	do {
		unsigned int inst = *pc;
		/* Look for a ``addiu $sp,$sp,...'' */
		if ((inst & 0xffff0000) == 0x27bd0000) {
			p->sp = (unsigned long)sp - (short) (inst & 0xffff);
			break;
		}
	} while (--pc > (unsigned int *)&stext);

	if (pc == (unsigned int *)&stext)
		return 0;

	/*
	 * Now scan forwards to find the $ra-save, so we can decode the
	 * instruction and retrieve the return address from the stack.
	 */
	pc++;
	do {
		unsigned int inst = *pc;
		/* Look for a ``sw $ra,NN($sp)'' */
		if ((inst & 0xffff0000) == 0xafbf0000) {
			p->pc = *(unsigned long *)((unsigned long)sp + (short)(inst & 0xffff));
			return 1;
		}
	} while (++pc <= (unsigned int *)&_etext);

	return 0;
}

#define supports_call_graph prof_have_mcount

#define get_prof_freq() prof_freq[0]

/* No performance counters for the time being */

#define have_perfctr() 0
#define valid_perfctr_event(e) 0
#define valid_perfctr_freq(n) 0
#define perfctr_reload(x)
#define __perfctr_stop()
#define __perfctr_commence(x,y)

#endif /* __KERNEL__ */

#endif /* !_ASM_KERNPROF_H */
