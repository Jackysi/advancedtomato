/*
 * This file contains the code to configure and utilize the ppc64 pmc hardware
 * Copyright (C) 2002 David Engebretsen <engebret@us.ibm.com>
 */

#ifndef __KERNEL__
#define INLINE_SYSCALL(arg1, arg2)       \
  ({                                            \
    register long r0 __asm__ ("r0");     \
    register long r3 __asm__ ("r3"); \
    register long r4 __asm__ ("r4"); \
    long ret, err;                              \
    r0 = 208; \
    r3 = (long) (arg1); \
    r4 = (long) (arg2); \
    __asm__ ("sc\n\t"                           \
             "mfcr      %1\n\t"                 \
             : "=r" (r3), "=r" (err)            \
             : "r" (r0), "r" (r3), "r" (r4) \
             : "cc", "memory");                 \
    ret = r3;                                   \
  })
#endif

#ifndef __ASSEMBLY__
struct perfmon_base_struct {
	u64 profile_buffer;
	u64 profile_length;
	u64 trace_buffer;
	u64 trace_length;
	u64 trace_end;
	u64 state;
};

struct pmc_header {
	int type;
	int pid;
	int resv[30];
};

struct pmc_struct {
        int pmc[11];
};

struct pmc_info_struct {
	unsigned int mode, cpu;

	unsigned int  pmc_base[11];
	unsigned long pmc_cumulative[8];
};

struct perfmon_struct {
	struct pmc_header header;

	union {
		struct pmc_struct      pmc;
		struct pmc_info_struct pmc_info;
 	} vdata;
};

enum {
	PMC_OP_ALLOC         = 1,
	PMC_OP_FREE          = 2,
	PMC_OP_CLEAR         = 4,
	PMC_OP_DUMP          = 5,
	PMC_OP_DUMP_HARDWARE = 6,
	PMC_OP_DECR_PROFILE  = 20,
	PMC_OP_PMC_PROFILE   = 21,
	PMC_OP_SET           = 30,
	PMC_OP_SET_USER      = 31,
	PMC_OP_END           = 30
};


#define	PMC_TRACE_CMD 0xFF

enum {
	PMC_TYPE_DERC_PROFILE  = 1,
	PMC_TYPE_CYCLE         = 2,
	PMC_TYPE_PROFILE       = 3,
	PMC_TYPE_DCACHE        = 4,
	PMC_TYPE_L2_MISS       = 5,
	PMC_TYPE_LWARCX        = 6,
	PMC_TYPE_END           = 6
};
#endif

#define	PMC_STATE_INITIAL         0x00
#define	PMC_STATE_READY           0x01
#define	PMC_STATE_DECR_PROFILE    0x10
#define	PMC_STATE_PROFILE_KERN    0x11
#define	PMC_STATE_TRACE_KERN      0x20
#define	PMC_STATE_TRACE_USER      0x21

