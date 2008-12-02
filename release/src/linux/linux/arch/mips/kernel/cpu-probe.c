#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <asm/bugs.h>
#include <asm/cpu.h>
#include <asm/fpu.h>
#include <asm/mipsregs.h>

/*
 * Not all of the MIPS CPUs have the "wait" instruction available. Moreover,
 * the implementation of the "wait" feature differs between CPU families. This
 * points to the function that implements CPU specific wait.
 * The wait instruction stops the pipeline and reduces the power consumption of
 * the CPU very much.
 */
void (*cpu_wait)(void) = NULL;

static void r3081_wait(void)
{
	unsigned long cfg = read_c0_conf();
	write_c0_conf(cfg | CONF_HALT);
}

static void r39xx_wait(void)
{
	unsigned long cfg = read_c0_conf();
	write_c0_conf(cfg | TX39_CONF_HALT);
}

static void r4k_wait(void)
{
	__asm__(".set\tmips3\n\t"
		"wait\n\t"
		".set\tmips0");
}

void au1k_wait(void)
{
#ifdef CONFIG_PM
	/* using the wait instruction makes CP0 counter unusable */
	__asm__(".set\tmips3\n\t"
		"wait\n\t"
		"nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t" ".set\tmips0");
#else
	__asm__("nop\n\t" "nop\n\t");
#endif
}

static inline void check_wait(void)
{
	printk("Checking for 'wait' instruction... ");
	switch(mips_cpu.cputype) {
	case CPU_R3081:
	case CPU_R3081E:
		cpu_wait = r3081_wait;
		printk(" available.\n");
		break;
	case CPU_TX3927:
	case CPU_TX39XX:
		cpu_wait = r39xx_wait;
		printk(" available.\n");
		break;
	case CPU_R4200:
/*	case CPU_R4300: */
	case CPU_R4600:
	case CPU_R4640:
	case CPU_R4650:
	case CPU_R4700:
	case CPU_R5000:
	case CPU_NEVADA:
	case CPU_RM7000:
	case CPU_TX49XX:
	case CPU_4KC:
	case CPU_4KEC:
	case CPU_4KSC:
	case CPU_5KC:
/*	case CPU_20KC:*/
		cpu_wait = r4k_wait;
		printk(" available.\n");
		break;
	case CPU_AU1000:
	case CPU_AU1100:
	case CPU_AU1500:
		cpu_wait = au1k_wait;
		printk(" available.\n");
		break;
	default:
		printk(" unavailable.\n");
		break;
	}
}

void __init check_bugs(void)
{
	check_wait();
}

/*
 * Probe whether cpu has config register by trying to play with
 * alternate cache bit and see whether it matters.
 * It's used by cpu_probe to distinguish between R3000A and R3081.
 */
static inline int cpu_has_confreg(void)
{
#ifdef CONFIG_CPU_R3000
	extern unsigned long r3k_cache_size(unsigned long);
	unsigned long size1, size2;
	unsigned long cfg = read_c0_conf();

	size1 = r3k_cache_size(ST0_ISC);
	write_c0_conf(cfg ^ CONF_AC);
	size2 = r3k_cache_size(ST0_ISC);
	write_c0_conf(cfg);
	return size1 != size2;
#else
	return 0;
#endif
}

/*
 * Get the FPU Implementation/Revision.
 */
static inline unsigned long cpu_get_fpu_id(void)
{
	unsigned long tmp, fpu_id;

	tmp = read_c0_status();
	__enable_fpu();
	fpu_id = read_32bit_cp1_register(CP1_REVISION);
	write_c0_status(tmp);
	return fpu_id;
}

/*
 * Check the CPU has an FPU the official way.
 */
static inline int cpu_has_fpu(void)
{
	return ((cpu_get_fpu_id() & 0xff00) != FPIR_IMP_NONE);
}

/* declaration of the global struct */
struct mips_cpu mips_cpu = {
    .processor_id	= PRID_IMP_UNKNOWN,
    .fpu_id		= FPIR_IMP_NONE,
    .cputype		= CPU_UNKNOWN
};

/* Shortcut for assembler access to mips_cpu.options */
int *cpuoptions = &mips_cpu.options;

#define R4K_OPTS (MIPS_CPU_TLB | MIPS_CPU_4KEX | MIPS_CPU_4KTLB \
		| MIPS_CPU_COUNTER | MIPS_CPU_CACHE_CDEX)

__init void cpu_probe(void)
{
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
	unsigned long config0 = read_c0_config();
	unsigned long config1;

	if ((config0 & CONF_BE) !=
#ifdef	CONFIG_CPU_LITTLE_ENDIAN
				0
#else
				CONF_BE
#endif
		) {
		panic("Kernel compiled little-endian, but running on a big-endian cpu");
	}

        if (config0 & (1 << 31)) {
		/* MIPS32 or MIPS64 compliant CPU. Read Config 1 register. */
		mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
			MIPS_CPU_4KTLB | MIPS_CPU_COUNTER | MIPS_CPU_DIVEC;
		config1 = read_c0_config1();
		if (config1 & (1 << 3))
			mips_cpu.options |= MIPS_CPU_WATCH;
		if (config1 & (1 << 2))
			mips_cpu.options |= MIPS_CPU_MIPS16;
		if (config1 & (1 << 1))
			mips_cpu.options |= MIPS_CPU_EJTAG;
		if (config1 & 1) {
			mips_cpu.options |= MIPS_CPU_FPU;
#if defined(CONFIG_CPU_MIPS64)
			mips_cpu.options |= MIPS_CPU_32FPR;
#endif
		}
		mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
	}
#endif
	mips_cpu.processor_id = read_c0_prid();
	switch (mips_cpu.processor_id & PRID_COMP_MASK) {
	case PRID_COMP_LEGACY:
		switch (mips_cpu.processor_id & PRID_IMP_MASK) {
		case PRID_IMP_R2000:
			mips_cpu.cputype = CPU_R2000;
			mips_cpu.isa_level = MIPS_CPU_ISA_I;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_NOFPUEX;
			if (cpu_has_fpu())
				mips_cpu.options |= MIPS_CPU_FPU;
			mips_cpu.tlbsize = 64;
			break;
		case PRID_IMP_R3000:
			if ((mips_cpu.processor_id & PRID_REV_MASK) == PRID_REV_R3000A)
				if (cpu_has_confreg())
					mips_cpu.cputype = CPU_R3081E;
				else
					mips_cpu.cputype = CPU_R3000A;
			else
				mips_cpu.cputype = CPU_R3000;
			mips_cpu.isa_level = MIPS_CPU_ISA_I;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_NOFPUEX;
			if (cpu_has_fpu())
				mips_cpu.options |= MIPS_CPU_FPU;
			mips_cpu.tlbsize = 64;
			break;
		case PRID_IMP_R4000:
			if ((mips_cpu.processor_id & PRID_REV_MASK) >= PRID_REV_R4400)
				mips_cpu.cputype = CPU_R4400SC;
			else
				mips_cpu.cputype = CPU_R4000SC;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH |
			                   MIPS_CPU_VCE;
			mips_cpu.tlbsize = 48;
			break;
                case PRID_IMP_VR41XX:
			switch (mips_cpu.processor_id & 0xf0) {
#ifndef CONFIG_VR4181
			case PRID_REV_VR4111:
				mips_cpu.cputype = CPU_VR4111;
				break;
#else
			case PRID_REV_VR4181:
				mips_cpu.cputype = CPU_VR4181;
				break;
#endif
			case PRID_REV_VR4121:
				mips_cpu.cputype = CPU_VR4121;
				break;
			case PRID_REV_VR4122:
				if ((mips_cpu.processor_id & 0xf) < 0x3)
					mips_cpu.cputype = CPU_VR4122;
				else
					mips_cpu.cputype = CPU_VR4181A;
				break;
			case PRID_REV_VR4131:
				mips_cpu.cputype = CPU_VR4131;
				mips_cpu.icache.ways = 2;
				mips_cpu.dcache.ways = 2;
				break;
			default:
				printk(KERN_INFO "Unexpected CPU of NEC VR4100 series\n");
				mips_cpu.cputype = CPU_VR41XX;
				break;
			}
                        mips_cpu.isa_level = MIPS_CPU_ISA_III;
                        mips_cpu.options = R4K_OPTS;
                        mips_cpu.tlbsize = 32;
                        break;
		case PRID_IMP_R4300:
			mips_cpu.cputype = CPU_R4300;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
					   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 32;
			break;
		case PRID_IMP_R4600:
			mips_cpu.cputype = CPU_R4600;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU;
			mips_cpu.tlbsize = 48;
			break;
		#if 0
 		case PRID_IMP_R4650:
			/*
			 * This processor doesn't have an MMU, so it's not
			 * "real easy" to run Linux on it. It is left purely
			 * for documentation.  Commented out because it shares
			 * it's c0_prid id number with the TX3900.
			 */
	 		mips_cpu.cputype = CPU_R4650;
		 	mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU;
		        mips_cpu.tlbsize = 48;
			break;
		#endif
		case PRID_IMP_TX39:
			mips_cpu.isa_level = MIPS_CPU_ISA_I;
			mips_cpu.options = MIPS_CPU_TLB;

			if ((mips_cpu.processor_id & 0xf0) ==
			    (PRID_REV_TX3927 & 0xf0)) {
				mips_cpu.cputype = CPU_TX3927;
				mips_cpu.tlbsize = 64;
				mips_cpu.icache.ways = 2;
				mips_cpu.dcache.ways = 2;
			} else {
				switch (mips_cpu.processor_id & PRID_REV_MASK) {
				case PRID_REV_TX3912:
					mips_cpu.cputype = CPU_TX3912;
					mips_cpu.tlbsize = 32;
					break;
				case PRID_REV_TX3922:
					mips_cpu.cputype = CPU_TX3922;
					mips_cpu.tlbsize = 64;
					break;
				default:
					mips_cpu.cputype = CPU_UNKNOWN;
					break;
				}
			}
			break;
		case PRID_IMP_R4700:
			mips_cpu.cputype = CPU_R4700;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_TX49:
			mips_cpu.cputype = CPU_TX49XX;
			mips_cpu.isa_level = MIPS_CPU_ISA_III;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			mips_cpu.icache.ways = 4;
			mips_cpu.dcache.ways = 4;
			break;
		case PRID_IMP_R5000:
			mips_cpu.cputype = CPU_R5000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_R5432:
			mips_cpu.cputype = CPU_R5432;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_R5500:
			mips_cpu.cputype = CPU_R5500;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_WATCH;
			mips_cpu.tlbsize = 48;
			break;
		case PRID_IMP_NEVADA:
			mips_cpu.cputype = CPU_NEVADA;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR | MIPS_CPU_DIVEC;
			mips_cpu.tlbsize = 48;
			mips_cpu.icache.ways = 2;
			mips_cpu.dcache.ways = 2;
			break;
		case PRID_IMP_R6000:
			mips_cpu.cputype = CPU_R6000;
			mips_cpu.isa_level = MIPS_CPU_ISA_II;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_FPU;
			mips_cpu.tlbsize = 32;
			break;
		case PRID_IMP_R6000A:
			mips_cpu.cputype = CPU_R6000A;
			mips_cpu.isa_level = MIPS_CPU_ISA_II;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_FPU;
			mips_cpu.tlbsize = 32;
			break;
		case PRID_IMP_RM7000:
			mips_cpu.cputype = CPU_RM7000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = R4K_OPTS | MIPS_CPU_FPU |
			                   MIPS_CPU_32FPR;
			/*
			 * Undocumented RM7000:  Bit 29 in the info register of
			 * the RM7000 v2.0 indicates if the TLB has 48 or 64
			 * entries.
			 *
			 * 29      1 =>    64 entry JTLB
			 *         0 =>    48 entry JTLB
			 */
			mips_cpu.tlbsize = (read_c0_info() & (1 << 29)) ? 64 : 48;
			break;
		case PRID_IMP_R8000:
			mips_cpu.cputype = CPU_R8000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
				           MIPS_CPU_FPU | MIPS_CPU_32FPR;
			mips_cpu.tlbsize = 384;      /* has weird TLB: 3-way x 128 */
			break;
		case PRID_IMP_R10000:
			mips_cpu.cputype = CPU_R10000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
				           MIPS_CPU_FPU | MIPS_CPU_32FPR |
				           MIPS_CPU_COUNTER | MIPS_CPU_WATCH;
			mips_cpu.tlbsize = 64;
			break;
		case PRID_IMP_R12000:
			mips_cpu.cputype = CPU_R12000;
			mips_cpu.isa_level = MIPS_CPU_ISA_IV;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
				           MIPS_CPU_FPU | MIPS_CPU_32FPR |
				           MIPS_CPU_COUNTER | MIPS_CPU_WATCH;
			mips_cpu.tlbsize = 64;
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
#if defined(CONFIG_CPU_MIPS32) || defined(CONFIG_CPU_MIPS64)
	case PRID_COMP_MIPS:
		switch (mips_cpu.processor_id & PRID_IMP_MASK) {
		case PRID_IMP_4KC:
			mips_cpu.cputype = CPU_4KC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M32;
			break;
		case PRID_IMP_4KEC:
			mips_cpu.cputype = CPU_4KEC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M32;
			break;
		case PRID_IMP_4KSC:
			mips_cpu.cputype = CPU_4KSC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M32;
			break;
		case PRID_IMP_5KC:
			mips_cpu.cputype = CPU_5KC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
			break;
		case PRID_IMP_20KC:
			mips_cpu.cputype = CPU_20KC;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
	case PRID_COMP_ALCHEMY:
		switch (mips_cpu.processor_id & PRID_IMP_MASK) {
		case PRID_IMP_AU1_REV1:
		case PRID_IMP_AU1_REV2:
			switch ((mips_cpu.processor_id >> 24) & PRID_REV_MASK) {
			case 0:
 				mips_cpu.cputype = CPU_AU1000;
				break;
			case 1:
				mips_cpu.cputype = CPU_AU1500;
				break;
			case 2:
				mips_cpu.cputype = CPU_AU1100;
				break;
			default:
				panic("Unknown Au Core!");
				break;
			}
			mips_cpu.isa_level = MIPS_CPU_ISA_M32;
 			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
        case PRID_COMP_BROADCOM:
                switch (mips_cpu.processor_id & PRID_IMP_MASK) {
                case PRID_IMP_BCM4710:          
                        mips_cpu.cputype = CPU_BCM4710;
                        mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
                                           MIPS_CPU_4KTLB | MIPS_CPU_COUNTER;
                        config1 = read_c0_config1();
                        if (config1 & (1 << 3))
                                mips_cpu.options |= MIPS_CPU_WATCH;
                        if (config1 & (1 << 2))
                                mips_cpu.options |= MIPS_CPU_MIPS16;
                        if (config1 & 1)
                                mips_cpu.options |= MIPS_CPU_FPU;
                        mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
                        break;
		case PRID_IMP_4KC:		
		case PRID_IMP_BCM3302:		
			mips_cpu.cputype = CPU_BCM3302;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX | 
					   MIPS_CPU_4KTLB | MIPS_CPU_COUNTER;
			config1 = read_c0_config1();
			if (config1 & (1 << 3))
				mips_cpu.options |= MIPS_CPU_WATCH;
			if (config1 & (1 << 2))
				mips_cpu.options |= MIPS_CPU_MIPS16;
			if (config1 & 1)
				mips_cpu.options |= MIPS_CPU_FPU;
			mips_cpu.scache.flags = MIPS_CACHE_NOT_PRESENT;
			break;
                default:
                        mips_cpu.cputype = CPU_UNKNOWN;
                        break;
                }
                break;
#endif /* CONFIG_CPU_MIPS32 */
	case PRID_COMP_SIBYTE:
		switch (mips_cpu.processor_id & PRID_IMP_MASK) {
		case PRID_IMP_SB1:
			mips_cpu.cputype = CPU_SB1;
			mips_cpu.isa_level = MIPS_CPU_ISA_M64;
			mips_cpu.options = MIPS_CPU_TLB | MIPS_CPU_4KEX |
			                   MIPS_CPU_COUNTER | MIPS_CPU_DIVEC |
			                   MIPS_CPU_MCHECK | MIPS_CPU_EJTAG;
#ifndef CONFIG_SB1_PASS_1_WORKAROUNDS
			/* FPU in pass1 is known to have issues. */
			mips_cpu.options |= MIPS_CPU_FPU;
#endif
			break;
		default:
			mips_cpu.cputype = CPU_UNKNOWN;
			break;
		}
		break;
	default:
		mips_cpu.cputype = CPU_UNKNOWN;
	}
	if (mips_cpu.options & MIPS_CPU_FPU)
		mips_cpu.fpu_id = cpu_get_fpu_id();
}

__init void cpu_report(void)
{
	printk("CPU revision is: %08x\n", mips_cpu.processor_id);
	if (mips_cpu.options & MIPS_CPU_FPU)
		printk("FPU revision is: %08x\n", mips_cpu.fpu_id);
}
