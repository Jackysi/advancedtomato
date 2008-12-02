#ifndef __ASM_SMPBOOT_H
#define __ASM_SMPBOOT_H

/*emum for clustered_apic_mode values*/
enum{
	CLUSTERED_APIC_NONE = 0,
	CLUSTERED_APIC_XAPIC,
	CLUSTERED_APIC_NUMAQ
};

#ifdef CONFIG_MULTIQUAD
 #define clustered_apic_mode (CLUSTERED_APIC_NUMAQ)
#else /* !CONFIG_MULTIQUAD */
 #define clustered_apic_mode (CLUSTERED_APIC_NONE)
#endif /* CONFIG_MULTIQUAD */

#ifdef CONFIG_X86_LOCAL_APIC
extern unsigned char esr_disable;
static inline int target_cpus(void)
{
	switch(clustered_apic_mode){
		case CLUSTERED_APIC_NUMAQ:
			/* Broadcast intrs to local quad only. */
			return APIC_BROADCAST_ID_APIC;
		default:
	}
	return cpu_online_map;
}
#ifdef CONFIG_X86_IO_APIC
extern unsigned char int_delivery_mode;
extern unsigned int int_dest_addr_mode;
#define	INT_DEST_ADDR_MODE (int_dest_addr_mode)
#define	INT_DELIVERY_MODE (int_delivery_mode)
#endif /* CONFIG_X86_IO_APIC */
#else /* CONFIG_X86_LOCAL_APIC */
#define esr_disable (0)
#define target_cpus() (0x01)
#ifdef CONFIG_X86_IO_APIC
#define INT_DEST_ADDR_MODE (APIC_DEST_LOGICAL)	/* logical delivery */
#define INT_DELIVERY_MODE (dest_LowestPrio)
#endif /* CONFIG_X86_IO_APIC */
#endif /* CONFIG_X86_LOCAL_APIC */

#define TRAMPOLINE_LOW phys_to_virt((clustered_apic_mode == CLUSTERED_APIC_NUMAQ)?0x8:0x467)
#define TRAMPOLINE_HIGH phys_to_virt((clustered_apic_mode == CLUSTERED_APIC_NUMAQ)?0xa:0x469)

#define boot_cpu_apicid ((clustered_apic_mode == CLUSTERED_APIC_NUMAQ)?boot_cpu_logical_apicid:boot_cpu_physical_apicid)

/*
 * How to map from the cpu_present_map
 */
static inline int cpu_present_to_apicid(int mps_cpu)
{
	if(clustered_apic_mode == CLUSTERED_APIC_NUMAQ)
		return (mps_cpu/4)*16 + (1<<(mps_cpu%4));
	return mps_cpu;
}

#define physical_to_logical_apicid(phys_apic) ( (1ul << (phys_apic & 0x3)) | (phys_apic & 0xF0u) )

/*
 * Mappings between logical cpu number and logical / physical apicid
 * The first four macros are trivial, but it keeps the abstraction consistent
 */
extern volatile int logical_apicid_2_cpu[];
extern volatile int cpu_2_logical_apicid[];
extern volatile int physical_apicid_2_cpu[];
extern volatile int cpu_2_physical_apicid[];

#define logical_apicid_to_cpu(apicid) logical_apicid_2_cpu[apicid]
#define cpu_to_logical_apicid(cpu) cpu_2_logical_apicid[cpu]
#define physical_apicid_to_cpu(apicid) physical_apicid_2_cpu[apicid]
#define cpu_to_physical_apicid(cpu) cpu_2_physical_apicid[cpu]
#ifdef CONFIG_MULTIQUAD			    /* use logical IDs to bootstrap */
#define boot_apicid_to_cpu(apicid) logical_apicid_2_cpu[apicid]
#define cpu_to_boot_apicid(cpu) cpu_2_logical_apicid[cpu]
#else /* !CONFIG_MULTIQUAD */		/* use physical IDs to bootstrap */
#define boot_apicid_to_cpu(apicid) physical_apicid_2_cpu[apicid]
#define cpu_to_boot_apicid(cpu) cpu_2_physical_apicid[cpu]
#endif /* CONFIG_MULTIQUAD */

#endif
