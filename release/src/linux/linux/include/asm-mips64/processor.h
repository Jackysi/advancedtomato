/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 Waldorf GMBH
 * Copyright (C) 1995, 1996, 1997, 1998, 1999, 2000 Ralf Baechle
 * Modified further for R[236]000 compatibility by Paul M. Antoine
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 */
#ifndef _ASM_PROCESSOR_H
#define _ASM_PROCESSOR_H

#include <linux/config.h>

/*
 * Return current * instruction pointer ("program counter").
 */
#define current_text_addr()						\
({									\
	void *_a;							\
									\
	__asm__ ("bal\t1f\t\t\t# current_text_addr\n"			\
		"1:\tmove\t%0, $31"					\
		: "=r" (_a)						\
		:							\
		: "$31");						\
									\
	_a;								\
})

#ifndef __ASSEMBLY__
#include <asm/cachectl.h>
#include <asm/mipsregs.h>
#include <asm/reg.h>
#include <asm/system.h>

#if defined(CONFIG_SGI_IP27)
#include <asm/sn/types.h>
#include <asm/sn/intr_public.h>
#endif

struct cpuinfo_mips {
	unsigned long udelay_val;
	unsigned long *pgd_quick;
	unsigned long *pmd_quick;
	unsigned long *pte_quick;
	unsigned long pgtable_cache_sz;
	unsigned long last_asn;
	unsigned long asid_cache;
#if defined(CONFIG_SGI_IP27)
	cpuid_t		p_cpuid;	/* PROM assigned cpuid */
	cnodeid_t	p_nodeid;	/* my node ID in compact-id-space */
	nasid_t		p_nasid;	/* my node ID in numa-as-id-space */
	unsigned char	p_slice;	/* Physical position on node board */
	hub_intmasks_t	p_intmasks;	/* SN0 per-CPU interrupt masks */
#endif
} __attribute__((aligned(128)));

extern void (*cpu_wait)(void);

extern unsigned int vced_count, vcei_count;
extern struct cpuinfo_mips cpu_data[];

#ifdef CONFIG_SMP
#define current_cpu_data cpu_data[smp_processor_id()]
#else
#define current_cpu_data cpu_data[0]
#endif

/*
 * Bus types (default is ISA, but people can check others with these..)
 */
#ifdef CONFIG_EISA
extern int EISA_bus;
#else
#define EISA_bus (0)
#endif

#define MCA_bus 0
#define MCA_bus__is_a_macro /* for versions in ksyms.c */

/*
 * MIPS has no problems with write protection
 */
#define wp_works_ok 1
#define wp_works_ok__is_a_macro /* for versions in ksyms.c */

/*
 * User space process size: 1TB. This is hardcoded into a few places,
 * so don't change it unless you know what you are doing.  TASK_SIZE
 * is limited to 1TB by the R4000 architecture; R10000 and better can
 * support 16TB.
 */
#define TASK_SIZE32	   0x7fff8000UL
#define TASK_SIZE	0x10000000000UL

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define TASK_UNMAPPED_BASE	((current->thread.mflags & MF_32BIT) ? \
	(TASK_SIZE32 / 3) : (TASK_SIZE / 3))

/*
 * Size of io_bitmap in longwords: 32 is ports 0-0x3ff.
 */
#define IO_BITMAP_SIZE	32

#define NUM_FPU_REGS	32

struct mips_fpu_hard_struct {
	unsigned long fp_regs[NUM_FPU_REGS];
	unsigned int control;
};

/*
 * It would be nice to add some more fields for emulator statistics, but there
 * are a number of fixed offsets in offset.h and elsewhere that would have to
 * be recalculated by hand.  So the additional information will be private to
 * the FPU emulator for now.  See asm-mips/fpu_emulator.h.
 */
typedef u64 fpureg_t;
struct mips_fpu_soft_struct {
	fpureg_t	regs[NUM_FPU_REGS];
	unsigned int	sr;
};


union mips_fpu_union {
        struct mips_fpu_hard_struct hard;
        struct mips_fpu_soft_struct soft;
};

#define INIT_FPU { \
	{{0,},} \
}

typedef struct {
	unsigned long seg;
} mm_segment_t;

/*
 * If you change thread_struct remember to change the #defines below too!
 */
struct thread_struct {
        /* Saved main processor registers. */
        unsigned long reg16;
	unsigned long reg17, reg18, reg19, reg20, reg21, reg22, reg23;
        unsigned long reg29, reg30, reg31;

	/* Saved cp0 stuff. */
	unsigned long cp0_status;

	/* Saved fpu/fpu emulator stuff. */
	union mips_fpu_union fpu;

	/* Other stuff associated with the thread. */
	unsigned long cp0_badvaddr;	/* Last user fault */
	unsigned long cp0_baduaddr;	/* Last kernel fault accessing USEG */
	unsigned long error_code;
	unsigned long trap_no;
#define MF_FIXADE 1			/* Fix address errors in software */
#define MF_LOGADE 2			/* Log address errors to syslog */
#define MF_32BIT  4			/* Process is in 32-bit compat mode */
	unsigned long mflags;
	mm_segment_t current_ds;
	unsigned long irix_trampoline;  /* Wheee... */
	unsigned long irix_oldctx;
};

#endif /* !__ASSEMBLY__ */

#define INIT_THREAD  { \
        /* \
         * saved main processor registers \
         */ \
	0, 0, 0, 0, 0, 0, 0, 0, \
	               0, 0, 0, \
	/* \
	 * saved cp0 stuff \
	 */ \
	0, \
	/* \
	 * saved fpu/fpu emulator stuff \
	 */ \
	INIT_FPU, \
	/* \
	 * Other stuff associated with the process \
	 */ \
	0, 0, 0, 0, \
	/* \
	 * For now the default is to fix address errors \
	 */ \
	MF_FIXADE, KERNEL_DS, 0, 0 \
}

#ifdef __KERNEL__

#define KERNEL_STACK_SIZE 0x4000

#ifndef __ASSEMBLY__

/* Free all resources held by a thread. */
#define release_thread(thread) do { } while(0)

extern int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);

/* Copy and release all segment info associated with a VM */
#define copy_segments(p, mm) do { } while(0)
#define release_segments(mm) do { } while(0)

/*
 * Return saved PC of a blocked thread.
 */
static inline unsigned long thread_saved_pc(struct thread_struct *t)
{
	extern void ret_from_sys_call(void);

	/* New born processes are a special case */
	if (t->reg31 == (unsigned long) ret_from_sys_call)
		return t->reg31;

	return ((unsigned long*)t->reg29)[11];
}

#define user_mode(regs)	(((regs)->cp0_status & ST0_KSU) == KSU_USER)

/*
 * Do necessary setup to start up a newly executed thread.
 */
extern void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long sp);

struct task_struct;
unsigned long get_wchan(struct task_struct *p);

#define __PT_REG(reg) ((long)&((struct pt_regs *)0)->reg - sizeof(struct pt_regs))
#define __KSTK_TOS(tsk) ((unsigned long)(tsk) + KERNEL_STACK_SIZE - 32)
#define KSTK_EIP(tsk) (*(unsigned long *)(__KSTK_TOS(tsk) + __PT_REG(cp0_epc)))
#define KSTK_ESP(tsk) (*(unsigned long *)(__KSTK_TOS(tsk) + __PT_REG(regs[29])))
#define KSTK_STATUS(tsk) (*(unsigned long *)(__KSTK_TOS(tsk) + __PT_REG(cp0_status)))

/* Allocation and freeing of basic task resources. */
/*
 * NOTE! The task struct and the stack go together
 */
#define THREAD_SIZE (2*PAGE_SIZE)
#define alloc_task_struct() \
	((struct task_struct *) __get_free_pages(GFP_KERNEL, 2))
#define free_task_struct(p)	free_pages((unsigned long)(p), 2)
#define get_task_struct(tsk)	atomic_inc(&virt_to_page(tsk)->count)

#define init_task	(init_task_union.task)
#define init_stack	(init_task_union.stack)

#define cpu_relax()	do { } while (0)

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */

/*
 * Return_address is a replacement for __builtin_return_address(count)
 * which on certain architectures cannot reasonably be implemented in GCC
 * (MIPS, Alpha) or is unuseable with -fomit-frame-pointer (i386).
 * Note that __builtin_return_address(x>=1) is forbidden because GCC
 * aborts compilation on some CPUs.  It's simply not possible to unwind
 * some CPU's stackframes.
 *
 * In gcc 2.8 and newer  __builtin_return_address works only for non-leaf
 * functions.  We avoid the overhead of a function call by forcing the
 * compiler to save the return address register on the stack.
 */
#define return_address() ({__asm__ __volatile__("":::"$31");__builtin_return_address(0);})

#endif /* _ASM_PROCESSOR_H */
