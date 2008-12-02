/*
 * Copyright (C) 2000 Anton Blanchard (anton@linuxcare.com)
 */

#ifndef _ASM_KERNPROF_H
#define _ASM_KERNPROF_H

#ifdef __KERNEL__

#include <asm/system.h>
#include <asm/ptrace.h>

#define DFL_PC_RES 4		/* default PC resolution for this platform */

#define in_firmware(regs) 0	/* never in the PROM during normal execution */

struct frame_info {
	struct reg_window *rw;
	unsigned long pc;
};

typedef struct frame_info frame_info_t;

static __inline__ int build_fake_frame(frame_info_t *frame)
{
	flush_register_windows(); /* make sure reg windows have real data */

	/* flush_register_windows() does not flush the current window's registers,
	 * so we need this first frame to be "magic". rw is set to NULL.
	 * get_next_frame() will special case this and look at %i6/%i7 to make
	 * the next frame.
	 */

	frame->pc = (unsigned long)current_text_addr();
	frame->rw = NULL;

	return 1;
}

static __inline__ void get_top_frame(struct pt_regs *regs, frame_info_t *p)
{
	p->pc = instruction_pointer(regs);
	p->rw = (struct reg_window *)(regs->u_regs[UREG_FP] + STACK_BIAS);
}

/* This macro determines whether there are more frames to go on the stack */
#define last_frame(p) \
	(((char *)(p)->rw) < ((char *)current) || \
	((char *)(p)->rw) >= ((char *)current + (2 * PAGE_SIZE) - TRACEREG_SZ - REGWIN_SZ))

static __inline__ int get_next_frame(frame_info_t *p)
{
	if (p->rw == NULL) {
		unsigned long reg;

		__asm__ __volatile__("mov %%i6, %0" : "=r" (reg));
		p->rw = (struct reg_window *)(reg + STACK_BIAS);
		__asm__ __volatile__("mov %%i7, %0" : "=r" (reg));
		p->pc = reg;
		return 1;
	}

	if (last_frame(p)) {
		return 0;
	}

	p->pc = (p->rw)->ins[7];

	p->rw = (struct reg_window *)((p->rw)->ins[6] + STACK_BIAS);

	return 1;
}

#define frame_get_pc(p)	((p)->pc)

#define supports_call_graph prof_have_mcount

/* No performance counters for the time being */
#define have_perfctr() 0
#define valid_perfctr_event(e) 0
#define valid_perfctr_freq(n) 0
#define perfctr_reload(x)
#define __perfctr_stop()
#define __perfctr_commence(x,y)

#ifdef CONFIG_SMP
#define prof_multiplier(__cpu) cpu_data[(__cpu)].multiplier
#define get_prof_freq() (HZ * prof_multiplier(0))
extern int setup_profiling_timer(unsigned int);
#else
#define get_prof_freq() (HZ)
#define setup_profiling_timer(x) (-EINVAL)
#endif

#endif /* __KERNEL__ */

#endif /* !_ASM_KERNPROF_H */
