/*
 * This file contains the code to configure and utilize the ppc64 pmc hardware
 * Copyright (C) 2002 David Engebretsen <engebret@us.ibm.com>
 */

#include <asm/proc_fs.h>
#include <asm/paca.h>
#include <asm/iSeries/ItLpPaca.h>
#include <asm/iSeries/ItLpQueue.h>
#include <asm/processor.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <asm/pmc.h>
#include <asm/uaccess.h>
#include <asm/naca.h>
#include <asm/perfmon.h>

extern char _stext[], _etext[], _end[];
struct perfmon_base_struct perfmon_base = {0, 0, 0, 0, 0, PMC_STATE_INITIAL};

int alloc_perf_buffer(int size);
int free_perf_buffer(void);
int clear_buffers(void);
void pmc_stop(void *data);
void pmc_start(void *data);
void pmc_touch_bolted(void *data);
void dump_pmc_struct(struct perfmon_struct *perfdata);
void dump_hardware_pmc_struct(void *perfdata);
int  decr_profile(struct perfmon_struct *perfdata);
int  pmc_profile(struct perfmon_struct *perfdata);
int  pmc_set_general(struct perfmon_struct *perfdata);
int  pmc_set_user_general(struct perfmon_struct *perfdata);
void pmc_configure(void *data);

asmlinkage int
sys_perfmonctl (int cmd, void *data) 
{ 
	struct perfmon_struct *pdata;
	int err;

	printk("sys_perfmonctl: cmd = 0x%x\n", cmd); 
	pdata = kmalloc(sizeof(struct perfmon_struct), GFP_USER);
	err = __copy_from_user(pdata, data, sizeof(struct perfmon_struct));
	switch(cmd) {
	case PMC_OP_ALLOC:
		alloc_perf_buffer(0); 
		break;
	case PMC_OP_FREE:
		free_perf_buffer(); 
		break;
	case PMC_OP_CLEAR:
		clear_buffers();
		break;
	case PMC_OP_DUMP:
		dump_pmc_struct(pdata);
		copy_to_user(data, pdata, sizeof(struct perfmon_struct));
		break;
	case PMC_OP_DUMP_HARDWARE:
		dump_hardware_pmc_struct(pdata);
		smp_call_function(dump_hardware_pmc_struct, (void *)pdata, 0, 1);
		break;
	case PMC_OP_DECR_PROFILE: /* NIA time sampling */
		decr_profile(pdata); 
		break;
	case PMC_OP_PMC_PROFILE:
		pmc_profile(pdata); 
		break;
	case PMC_OP_SET:
		pmc_set_general(pdata); 
		break;
	case PMC_OP_SET_USER:
		pmc_set_user_general(pdata); 
		break;
	default:
		printk("Perfmon: Unknown operation\n");
		break;
	}

	kfree(pdata); 
	return 0;
}

int alloc_perf_buffer(int size) 
{
	int i;

	printk("Perfmon: allocate buffer\n");
	if(perfmon_base.state == PMC_STATE_INITIAL) {
		perfmon_base.profile_length = (((unsigned long) &_etext - 
				   (unsigned long) &_stext) >> 2) * sizeof(int);
		perfmon_base.profile_buffer = (unsigned long)btmalloc(perfmon_base.profile_length);
		perfmon_base.trace_length = 1024*1024*16;
		perfmon_base.trace_buffer = (unsigned long)btmalloc(perfmon_base.trace_length);

		if(perfmon_base.profile_buffer && perfmon_base.trace_buffer) {
			memset((char *)perfmon_base.profile_buffer, 0, perfmon_base.profile_length);
			printk("Profile buffer created at address 0x%lx of length 0x%lx\n",
			       perfmon_base.profile_buffer, perfmon_base.profile_length); 
		} else {
			printk("Profile buffer creation failed\n");
			return 0;
		}

		/* Fault in the first bolted segment - it then remains in the stab for all time */
		pmc_touch_bolted(NULL); 
		smp_call_function(pmc_touch_bolted, (void *)NULL, 0, 1);

		for (i=0; i<MAX_PACAS; ++i) {
			paca[i].prof_shift = 2;
			paca[i].prof_len = perfmon_base.profile_length;
			paca[i].prof_buffer = (unsigned *)(perfmon_base.profile_buffer);
			paca[i].prof_stext = (unsigned *)&_stext;

			paca[i].prof_etext = (unsigned *)&_etext;
			mb();
		} 

		perfmon_base.state = PMC_STATE_READY; 
	}

	return 0;
}

int free_perf_buffer() 
{
	printk("Perfmon: free buffer\n");

	if(perfmon_base.state == PMC_STATE_INITIAL) {
		printk("Perfmon: free buffer failed - no buffer was allocated.\n"); 
		return -1;
	}

	btfree((void *)perfmon_base.profile_buffer); 
	btfree((void *)perfmon_base.trace_buffer); 

	perfmon_base.profile_length = 0;
	perfmon_base.profile_buffer = 0;
	perfmon_base.trace_buffer   = 0;
	perfmon_base.trace_length   = 0;
	perfmon_base.trace_end      = 0;
	perfmon_base.state = PMC_STATE_INITIAL; 

	return(0); 
}

int clear_buffers() 
{
	if(perfmon_base.state == PMC_STATE_INITIAL) {
		printk("Perfmon: clear buffer failed - no buffer was allocated.\n"); 
		return -1;
	}

	printk("Perfmon: clear buffer\n");
	
	/* Stop counters on all processors -- blocking */
	pmc_stop(NULL); 
	smp_call_function(pmc_stop, (void *)NULL, 0, 1);
	
	/* Clear the buffers */
	memset((char *)perfmon_base.profile_buffer, 0, perfmon_base.profile_length);
	memset((char *)perfmon_base.trace_buffer, 0, perfmon_base.trace_length);
	
	/* Reset the trace buffer point */
	perfmon_base.trace_end = 0;
	
	/* Restart counters on all processors -- blocking */
	pmc_start(NULL); 
	smp_call_function(pmc_start, (void *)NULL, 0, 1);

	return(0); 
}

void pmc_stop(void *data) 
{
	/* Freeze all counters, leave everything else alone */
	mtspr( MMCR0, mfspr( MMCR0 ) | 0x80000000 );
}

void pmc_start(void *data) 
{
	/* Free all counters, leave everything else alone */
	mtspr( MMCR0, mfspr( MMCR0 ) & 0x7fffffff );
}

void pmc_touch_bolted(void *data) 
{
	volatile int touch;

	/* Hack to fault the buffer into the segment table */
	touch = *((int *)(perfmon_base.profile_buffer));
}

void dump_pmc_struct(struct perfmon_struct *perfdata) 
{
	unsigned int cpu = perfdata->vdata.pmc_info.cpu, i;

	if(cpu > MAX_PACAS) return;

	printk("PMC Control Mode: 0x%lx\n", perfmon_base.state);
	printk("PMC[1 - 2] = 0x%16.16lx 0x%16.16lx\n",
	       paca[cpu].pmcc[0], paca[cpu].pmcc[1]);
	printk("PMC[3 - 4] = 0x%16.16lx 0x%16.16lx\n",
	       paca[cpu].pmcc[2], paca[cpu].pmcc[3]);
	printk("PMC[5 - 6] = 0x%16.16lx 0x%16.16lx\n",
	       paca[cpu].pmcc[4], paca[cpu].pmcc[5]);
	printk("PMC[7 - 8] = 0x%16.16lx 0x%16.16lx\n",
	       paca[cpu].pmcc[6], paca[cpu].pmcc[7]);

	perfdata->vdata.pmc_info.mode = perfmon_base.state;
	for(i = 0; i < 11; i++) 
		perfdata->vdata.pmc_info.pmc_base[i]  = paca[cpu].pmc[i];

	for(i = 0; i < 8; i++) 
		perfdata->vdata.pmc_info.pmc_cumulative[i]  = paca[cpu].pmcc[i];
}

void dump_hardware_pmc_struct(void *perfdata) 
{
	unsigned int cpu = smp_processor_id();

	printk("PMC[%2.2d][1 - 4]  = 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
	       cpu, (u32) mfspr(PMC1),(u32) mfspr(PMC2),(u32) mfspr(PMC3),(u32) mfspr(PMC4));
	printk("PMC[%2.2d][5 - 8]  = 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
	       cpu, (u32) mfspr(PMC5),(u32) mfspr(PMC6),(u32) mfspr(PMC7),(u32) mfspr(PMC8));
	printk("MMCR[%2.2d][0,1,A] = 0x%8.8x 0x%8.8x 0x%8.8x\n",
	       cpu, (u32) mfspr(MMCR0),(u32) mfspr(MMCR1),(u32) mfspr(MMCRA));
}

int decr_profile(struct perfmon_struct *perfdata) 
{
	int i;

	printk("Perfmon: NIA decrementer profile\n");

	if(perfmon_base.state == PMC_STATE_INITIAL) {
		printk("Perfmon: failed - no buffer was allocated.\n"); 
		return -1;
	}
	
	/* Stop counters on all processors -- blocking */
	pmc_stop(NULL); 
	smp_call_function(pmc_stop, (void *)NULL, 0, 1);
	
	for (i=0; i<MAX_PACAS; ++i) {
		paca[i].prof_mode = PMC_STATE_DECR_PROFILE;
	}
	
	perfmon_base.state = PMC_STATE_DECR_PROFILE; 
	mb(); 

	return 0;
}

int pmc_profile(struct perfmon_struct *perfdata) 
{
	struct pmc_struct *pdata = &(perfdata->vdata.pmc);
	int i;

	printk("Perfmon: NIA PMC profile and CPI\n");

	if(perfmon_base.state == PMC_STATE_INITIAL) {
		printk("Perfmon: failed - no buffer was allocated.\n"); 
		return -1;
	}

	/* Stop counters on all processors -- blocking */
	pmc_stop(NULL); 
	smp_call_function(pmc_stop, (void *)NULL, 0, 1);
	
	for (i=0; i<MAX_PACAS; ++i) {
		paca[i].prof_mode = PMC_STATE_PROFILE_KERN;
	}
	perfmon_base.state = PMC_STATE_PROFILE_KERN; 

	pdata->pmc[0] = 0x7f000000;
	for(i = 1; i < 8; i++) 
		pdata->pmc[i] = 0x0;
	pdata->pmc[8] = 0x26000000 | (0x01 << (31 - 25) | (0x1));
	pdata->pmc[9] = (0x3 << (31-4)); /* Instr completed */
	pdata->pmc[10] = 0x00000000 | (0x1 << (31 - 30));

	mb();

	pmc_configure((void *)perfdata);
	smp_call_function(pmc_configure, (void *)perfdata, 0, 0);

	return 0;
}

int pmc_set_general(struct perfmon_struct *perfdata) 
{
	int i;

	printk("Perfmon: PMC sampling - General\n");

	if(perfmon_base.state == PMC_STATE_INITIAL) {
		printk("Perfmon: failed - no buffer was allocated.\n"); 
		return -1;
	}

	/* Stop counters on all processors -- blocking */
	pmc_stop(NULL); 
	smp_call_function(pmc_stop, (void *)NULL, 0, 1);
	
	for (i=0; i<MAX_PACAS; ++i) {
		paca[i].prof_mode = PMC_STATE_TRACE_KERN;
	}
	perfmon_base.state = PMC_STATE_TRACE_KERN; 
	mb();

	pmc_configure((void *)perfdata);
	smp_call_function(pmc_configure, (void *)perfdata, 0, 0);

	return 0;
}

int pmc_set_user_general(struct perfmon_struct *perfdata) 
{
	struct pmc_struct *pdata = &(perfdata->vdata.pmc);
	int pid = perfdata->header.pid;
	struct task_struct *task;
	int i;

	printk("Perfmon: PMC sampling - general user\n");

	if(perfmon_base.state == PMC_STATE_INITIAL) {
		printk("Perfmon: failed - no buffer was allocated.\n"); 
		return -1;
	}

	if(pid) {
		printk("Perfmon: pid = 0x%x\n", pid);
		read_lock(&tasklist_lock);
		task = find_task_by_pid(pid);
		if (task) {
			printk("Perfmon: task = 0x%lx\n", (u64) task);
			task->thread.regs->msr |= 0x4;
		} else {
			printk("Perfmon: task not found\n");
			read_unlock(&tasklist_lock);
			return -1;
		}
	}
	read_unlock(&tasklist_lock);

	/* Stop counters on all processors -- blocking */
	pmc_stop(NULL); 
	smp_call_function(pmc_stop, (void *)NULL, 0, 1);
	
	for (i=0; i<MAX_PACAS; ++i) {
		paca[i].prof_mode = PMC_STATE_TRACE_USER;
	}
	perfmon_base.state = PMC_STATE_TRACE_USER; 
	mb();

	pmc_configure((void *)perfdata);
	smp_call_function(pmc_configure, (void *)perfdata, 0, 0);

	return 0;
}

void pmc_configure(void *data)
{
	struct paca_struct *lpaca = get_paca();
	struct perfmon_struct *perfdata = (struct perfmon_struct *)data;
	struct pmc_struct *pdata = &(perfdata->vdata.pmc);
	unsigned long cmd_rec, i;

	/* Indicate to hypervisor that we are using the PMCs */
	if(naca->platform == PLATFORM_ISERIES_LPAR)
		lpaca->xLpPacaPtr->xPMCRegsInUse = 1;

	/* Freeze all counters */
	mtspr( MMCR0, 0x80000000 ); mtspr( MMCR1, 0x00000000 );

	cmd_rec = 0xFFUL << 56;
	cmd_rec |= perfdata->header.type;
	*((unsigned long *)(perfmon_base.trace_buffer + perfmon_base.trace_end)) = cmd_rec;
	perfmon_base.trace_end += 8;

	/* Clear all the PMCs */
	mtspr( PMC1, 0 ); mtspr( PMC2, 0 ); mtspr( PMC3, 0 );
	mtspr( PMC4, 0 ); mtspr( PMC5, 0 ); mtspr( PMC6, 0 );
	mtspr( PMC7, 0 ); mtspr( PMC8, 0 );

	for(i = 0; i < 11; i++)
		lpaca->pmc[i]  = pdata->pmc[i];

	mtspr(PMC1, lpaca->pmc[0]); mtspr(PMC2, lpaca->pmc[1]);
	mtspr(PMC3, lpaca->pmc[2]); mtspr(PMC4, lpaca->pmc[3]);
	mtspr(PMC5, lpaca->pmc[4]); mtspr(PMC6, lpaca->pmc[5]);
	mtspr(PMC7, lpaca->pmc[6]); mtspr(PMC8, lpaca->pmc[7]);
	mtspr(MMCR1, lpaca->pmc[9]); mtspr(MMCRA, lpaca->pmc[10]);

	mb();
	
	/* Start all counters */
	mtspr( MMCR0, lpaca->pmc[8]);
}

