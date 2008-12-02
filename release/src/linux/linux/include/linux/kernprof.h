#ifndef _LINUX_KERNPROF_H
#define _LINUX_KERNPROF_H

#include <linux/ioctl.h>

#define CG_MAX_ARCS (1 << (8 * sizeof(short)))

#define PROF_BACKTRACE_BUFSIZE	4096  /* must be a power of 2 */
#define PROF_BACKTRACE_MAX_LEN	24

typedef unsigned int PC_sample_count_t;

/* profiling ioctl requests */
#define PROF_START		_IO(0xAF, 0)
#define PROF_STOP		_IO(0xAF, 1)
#define PROF_RESET		_IO(0xAF, 2)
#define PROF_SET_SAMPLE_FREQ	_IOW(0xAF, 3, int)
#define PROF_GET_SAMPLE_FREQ	_IOR(0xAF, 4, int)
#define PROF_GET_PC_RES		_IOR(0xAF, 5, int)
#define PROF_GET_ON_OFF_STATE	_IOR(0xAF, 6, int)
#define PROF_SET_DOMAIN		_IOW(0xAF, 7, int)
#define PROF_GET_DOMAIN		_IOR(0xAF, 8, int)
#define PROF_SET_MODE		_IOW(0xAF, 9, int)
#define PROF_GET_MODE		_IOR(0xAF, 10, int)
#define PROF_SET_PERFCTR_EVENT	_IOW(0xAF, 11, int)
#define PROF_GET_PERFCTR_EVENT	_IOR(0xAF, 12, int)
/* PROF_*_ENABLE_MAP and PROF_GET_MAPPING ioctl requests are defined below */
#define PROF_SET_PID		_IOW(0xAF, 16, int)
#define PROF_GET_PID		_IOR(0xAF, 17, int)

enum {
	PROF_MODE_PC_SAMPLING = 1,
	PROF_MODE_CALL_GRAPH = 2,
	PROF_MODE_BACKTRACE = 4,
	PROF_MODE_CALL_COUNT = 8,
	PROF_MODE_SCHEDULER_CALL_GRAPH = 16
};

enum {
	PROF_DOMAIN_TIME,
	PROF_DOMAIN_PERFCTR
};

#if defined(CONFIG_KERNPROF) || defined(CONFIG_MCOUNT)
/*
 * To allow for profiling of loaded modules, this structure
 * describes the layout of the buckets used to collect samples.
 */

typedef struct prof_mem_map
{
   unsigned long     kernel_buckets;   /* number of kernel buckets */
   unsigned long     module_buckets;   /* number of module buckets */
   unsigned long     nr_cpus;          /* number of processors whether profiled or not */
   unsigned long     cg_from_size;     /* size of one cg_from array */
   unsigned long     cg_to_size;       /* size of one cg_to array */
   unsigned long     cg_to_offset;     /* offset of cg_to array */
   unsigned long     kernel_start;     /* lowest text address in kernel */
   unsigned long     kernel_end;       /* highest text address in kernel */
   unsigned long     module_start;     /* lowest text address in all modules */
   unsigned long     module_end;       /* highest text address in all modules */
} prof_mem_map_t;
#endif /* CONFIG_KERNPROF or CONFIG_MCOUNT */

#ifdef __KERNEL__

#include <asm/atomic.h>
#include <asm/ptrace.h>

/*
 * We don't export this to user space because its pointers may be of different
 * size.  If user space needs this it should define its own version making sure
 * that individual fields are of the same size as in the kernel definition.
 */
struct cg_arc_dest {
	unsigned long address;
	atomic_t count;
	unsigned short link;
	unsigned short pad;
};

/*
 * We do not export these ioctl requests to user space because it may have
 * longs of different size.
 */
#define PROF_SET_ENABLE_MAP	_IOW(0xAF, 13, long)
#define PROF_GET_ENABLE_MAP	_IOR(0xAF, 14, long)
#define PROF_GET_MAPPING	_IOR(0xAF, 15, long)


typedef void (*prof_hook_p)(struct pt_regs *);
typedef void (*mcount_hook_p)(unsigned long, unsigned long);
typedef void (*wakeup_hook_p)(unsigned long, unsigned long, unsigned long);

extern char _stext, _etext;
extern prof_hook_p prof_timer_hook;
extern prof_hook_p prof_perfctr_hook;
extern mcount_hook_p prof_scheduler_hook;
extern wakeup_hook_p prof_wakeup_hook;
extern mcount_hook_p mcount_hook;

extern int prof_have_frameptr, prof_have_mcount;

extern void USER(void);            /* these can not be in a module */
extern void UNKNOWN_KERNEL(void);
extern void FIRMWARE(void);
extern void STALLED(void);
extern void SLEEPING(void);
extern void MODULE(void);

#define pc_out_of_range(pc)	\
	((pc) < (unsigned long) &_stext || (pc) >= (unsigned long) &_etext)

/* might be overridden by arch-specific redefinition */
#define FUNCTIONPC(func)	(unsigned long) &(func)

#endif /* __KERNEL__ */

#endif /* !_LINUX_KERNPROF_H */
