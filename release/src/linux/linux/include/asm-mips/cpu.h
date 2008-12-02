/*
 * cpu.h: Values of the PRId register used to match up
 *        various MIPS cpu types.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 */
#ifndef _ASM_CPU_H
#define _ASM_CPU_H

#include <asm/cache.h>

/* Assigned Company values for bits 23:16 of the PRId Register
   (CP0 register 15, select 0).  As of the MIPS32 and MIPS64 specs from
   MTI, the PRId register is defined in this (backwards compatible)
   way:

  +----------------+----------------+----------------+----------------+
  | Company Options| Company ID     | Processor ID   | Revision       |
  +----------------+----------------+----------------+----------------+
   31            24 23            16 15             8 7

   I don't have docs for all the previous processors, but my impression is
   that bits 16-23 have been 0 for all MIPS processors before the MIPS32/64
   spec.
*/

#define PRID_COPT_MASK         0xff000000
#define PRID_COMP_MASK         0x00ff0000
#define PRID_IMP_MASK          0x0000ff00
#define PRID_REV_MASK          0x000000ff

#define PRID_COMP_LEGACY       0x000000
#define PRID_COMP_MIPS         0x010000
#define PRID_COMP_BROADCOM     0x020000
#define PRID_COMP_ALCHEMY      0x030000
#define PRID_COMP_SIBYTE       0x040000

/*
 * Assigned values for the product ID register.  In order to detect a
 * certain CPU type exactly eventually additional registers may need to
 * be examined.  These are valid when 23:16 == PRID_COMP_LEGACY
 */
#define PRID_IMP_R2000		0x0100
#define PRID_IMP_AU1_REV1	0x0100
#define PRID_IMP_AU1_REV2	0x0200
#define PRID_IMP_R3000		0x0200		/* Same as R2000A  */
#define PRID_IMP_R6000		0x0300		/* Same as R3000A  */
#define PRID_IMP_R4000		0x0400
#define PRID_IMP_R6000A		0x0600
#define PRID_IMP_R10000		0x0900
#define PRID_IMP_R4300		0x0b00
#define PRID_IMP_VR41XX		0x0c00
#define PRID_IMP_R12000		0x0e00
#define PRID_IMP_R8000		0x1000
#define PRID_IMP_R4600		0x2000
#define PRID_IMP_R4700		0x2100
#define PRID_IMP_TX39		0x2200
#define PRID_IMP_R4640		0x2200
#define PRID_IMP_R4650		0x2200		/* Same as R4640 */
#define PRID_IMP_R5000		0x2300
#define PRID_IMP_TX49		0x2d00
#define PRID_IMP_SONIC		0x2400
#define PRID_IMP_MAGIC		0x2500
#define PRID_IMP_RM7000		0x2700
#define PRID_IMP_NEVADA		0x2800		/* RM5260 ??? */
#define PRID_IMP_R5432		0x5400
#define PRID_IMP_R5500		0x5500
#define PRID_IMP_4KC		0x8000
#define PRID_IMP_5KC		0x8100
#define PRID_IMP_20KC		0x8200
#define PRID_IMP_4KEC		0x8400
#define PRID_IMP_4KSC		0x8600
#define PRID_IMP_BCM4710	0x4000
#define PRID_IMP_BCM3302	0x9000
#define PRID_IMP_BCM3303	0x9100
#define	PRID_IMP_BCM3303	0x9100

#define PRID_IMP_UNKNOWN	0xff00

#define       BCM330X(id) \
	(((id & (PRID_COMP_MASK | PRID_IMP_MASK)) == (PRID_COMP_BROADCOM | PRID_IMP_BCM3302)) \
	|| ((id & (PRID_COMP_MASK | PRID_IMP_MASK)) == (PRID_COMP_BROADCOM | PRID_IMP_BCM3303)))

/*
 * These are the PRID's for when 23:16 == PRID_COMP_SIBYTE
 */

#define PRID_IMP_SB1            0x0100

/*
 * Definitions for 7:0 on legacy processors
 */


#define PRID_REV_R4400		0x0040
#define PRID_REV_R3000A		0x0030
#define PRID_REV_R3000		0x0020
#define PRID_REV_R2000A		0x0010
#define PRID_REV_TX3912 	0x0010
#define PRID_REV_TX3922 	0x0030
#define PRID_REV_TX3927 	0x0040
#define PRID_REV_VR4111		0x0050
#define PRID_REV_VR4181		0x0050	/* Same as VR4111 */
#define PRID_REV_VR4121		0x0060
#define PRID_REV_VR4122		0x0070
#define PRID_REV_VR4181A	0x0070	/* Same as VR4122 */
#define PRID_REV_VR4131		0x0080

/*
 * FPU implementation/revision register (CP1 control register 0).
 *
 * +---------------------------------+----------------+----------------+
 * | 0                               | Implementation | Revision       |
 * +---------------------------------+----------------+----------------+
 *  31                             16 15             8 7              0
 */

#define FPIR_IMP_NONE		0x0000

#ifndef __ASSEMBLY__

extern void cpu_probe(void);
extern void cpu_report(void);

/*
 * Capability and feature descriptor structure for MIPS CPU
 */
struct mips_cpu {
	unsigned int processor_id;
	unsigned int fpu_id;
	unsigned int cputype;
	int isa_level;
	int options;
	int tlbsize;
	struct cache_desc icache;	/* Primary I-cache */
	struct cache_desc dcache;	/* Primary D or combined I/D cache */
	struct cache_desc scache;	/* Secondary cache */
	struct cache_desc tcache;	/* Tertiary/split secondary cache */
};

extern struct mips_cpu mips_cpu;

enum cputype {
	CPU_UNKNOWN,
	CPU_R2000, CPU_R3000, CPU_R3000A, CPU_R3041, CPU_R3051, CPU_R3052,
	CPU_R3081, CPU_R3081E, CPU_R4000PC, CPU_R4000SC, CPU_R4000MC,
	CPU_R4200, CPU_R4400PC, CPU_R4400SC, CPU_R4400MC, CPU_R4600,
	CPU_R6000, CPU_R6000A, CPU_R8000, CPU_R10000, CPU_R12000, CPU_R4300,
	CPU_R4650, CPU_R4700, CPU_R5000, CPU_R5000A, CPU_R4640, CPU_NEVADA,
	CPU_RM7000, CPU_R5432, CPU_4KC, CPU_5KC, CPU_R4310, CPU_SB1,
	CPU_TX3912, CPU_TX3922, CPU_TX3927, CPU_AU1000, CPU_4KEC, CPU_4KSC,
	CPU_VR41XX, CPU_R5500, CPU_TX49XX, CPU_TX39XX, CPU_AU1500, CPU_20KC,
	CPU_VR4111, CPU_VR4121, CPU_VR4122, CPU_VR4131, CPU_VR4181, CPU_VR4181A,
	CPU_AU1100, CPU_BCM4710, CPU_BCM3302, CPU_LAST
};

#endif /* !__ASSEMBLY__ */

/*
 * ISA Level encodings
 */
#define MIPS_CPU_ISA_I		0x00000001
#define MIPS_CPU_ISA_II		0x00000002
#define MIPS_CPU_ISA_III	0x00000003
#define MIPS_CPU_ISA_IV		0x00000004
#define MIPS_CPU_ISA_V		0x00000005
#define MIPS_CPU_ISA_M32	0x00000020
#define MIPS_CPU_ISA_M64	0x00000040

/*
 * CPU Option encodings
 */
#define MIPS_CPU_TLB		0x00000001 /* CPU has TLB */
/* Leave a spare bit for variant MMU types... */
#define MIPS_CPU_4KEX		0x00000004 /* "R4K" exception model */
#define MIPS_CPU_4KTLB		0x00000008 /* "R4K" TLB handler */
#define MIPS_CPU_FPU		0x00000010 /* CPU has FPU */
#define MIPS_CPU_32FPR		0x00000020 /* 32 dbl. prec. FP registers */
#define MIPS_CPU_COUNTER	0x00000040 /* Cycle count/compare */
#define MIPS_CPU_WATCH		0x00000080 /* watchpoint registers */
#define MIPS_CPU_MIPS16		0x00000100 /* code compression */
#define MIPS_CPU_DIVEC		0x00000200 /* dedicated interrupt vector */
#define MIPS_CPU_VCE		0x00000400 /* virt. coherence conflict possible */
#define MIPS_CPU_CACHE_CDEX	0x00000800 /* Create_Dirty_Exclusive CACHE op */
#define MIPS_CPU_MCHECK		0x00001000 /* Machine check exception */
#define MIPS_CPU_EJTAG		0x00002000 /* EJTAG exception */
#define MIPS_CPU_NOFPUEX	0x00004000 /* no FPU exception */

#endif /* _ASM_CPU_H */
