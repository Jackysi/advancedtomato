/*
 * Copyright (C) SGI 2000
 *
 * Written by Dimitris Michailidis (dimitris@engr.sgi.com)
 * Written by Goutham Rao <goutham.rao@intel.com>
 */

#ifndef _ASM_KERNPROF_H
#define _ASM_KERNPROF_H
#ifdef __KERNEL__

#include <asm/unwind.h>
#include <asm/system.h>
#include <asm/ptrace.h>

#ifdef FUNCTIONPC
#undef FUNCTIONPC
#endif
#define FUNCTIONPC(func)	(*(unsigned long *)&(func))

/* We can do 16-bit compare&swap */
#define __HAVE_ARCH_CMPXCHG16 1

typedef struct unw_frame_info frame_info_t;

/* 
 * default PC resolution for this platform 
 */
#define DFL_PC_RES 4

#define supports_call_graph prof_have_mcount

#define frame_get_pc(frame) ((frame)->ip)

#define get_prof_freq() HZ

extern void cg_record_arc(unsigned long, unsigned long);

static void do_cg_record_arc(struct unw_frame_info *info, void *arg)
{
	unsigned long callee_ip, caller_ip;

	/* First, get the frame for our backtrace_cg_record_arc() caller */
	unw_get_ip(info, &caller_ip);
	if (caller_ip == 0  ||  unw_unwind(info) < 0)
		return;

	/*
	 * Next, get the frame for the next higher caller -- this is the
	 * first interesting callee.
	 */
	unw_get_ip(info, &caller_ip);
	if (caller_ip == 0  ||  unw_unwind(info) < 0)
		return;

	/* Now begin the iteration of walking further up the call graph */
	do {
		callee_ip = caller_ip;
		unw_get_ip(info, &caller_ip);

		if (caller_ip == 0)
			break;

		if (pc_out_of_range(caller_ip))
			break;

		cg_record_arc(caller_ip, callee_ip);

	} while (unw_unwind(info) >= 0);
}

/* 
 *  Record the call graph, which is normally done by backtrace_cg_record_arc(),
 *  and return 1 to indicate success.  We expect that backtrace_cg_record_arc()
 *  will next call get_next_frame() and that will *fail*, leaving us with the
 *  backtrace that do_cg_record_arc() has recorded (above).
 *  Yes, this is a real hack.
 */
static __inline__ int build_fake_frame(frame_info_t *frame)
{
	unw_init_running(do_cg_record_arc, 0);
	return 1;
}

static __inline__ unsigned long instruction_pointer(struct pt_regs *regs)
{
	return regs->cr_iip + (ia64_psr(regs)->ri << 2);
}

static __inline__ int in_firmware(struct pt_regs *regs)
{
	return 0;
}

static __inline__ void get_top_frame(struct pt_regs *regs, frame_info_t *frame)
{
	struct switch_stack *sw = (struct switch_stack *) regs - 1;
	unw_init_frame_info(frame, current, sw);
	/* skip over interrupt frame */
	unw_unwind(frame);
}

static __inline__ int get_next_frame(frame_info_t *frame)
{
	return 0;
}

#define have_perfctr() 0
#define valid_perfctr_event(e) 0
#define valid_perfctr_freq(n) 0
#define perfctr_reload(x)
#define __perfctr_stop()
#define __perfctr_commence(x,y)
#define setup_profiling_timer(x) (-EINVAL)

#endif /* __KERNEL__ */
#endif /* !_ASM_KERNPROF_H */
