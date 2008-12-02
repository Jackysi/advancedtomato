/*
 * linux/drivers/char/kernprof.c
 *
 * Implementation of profiling devices.  We reserve minor number 255 for a 
 * control interface.  ioctl()s on this device control various profiling
 * settings. 
 * 
 * Copyright (C) SGI 1999, 2000, 2001
 *
 * Written by Dimitris Michailidis (dimitris@engr.sgi.com)
 * Modified by John Hawkes (hawkes@engr.sgi.com)
 * Contributions from Niels Christiansen (nchr@us.ibm.com)
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernprof.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/smp.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/module.h>
#include <linux/compiler.h>

#include <asm/uaccess.h>
#include <asm/kernprof.h>

#define PROF_CNTRL_MINOR 0

int prof_enabled = 0; /* any profiling active */
int prof_domain = PROF_DOMAIN_TIME, prof_mode = PROF_MODE_PC_SAMPLING;
int prof_pid = 0;
int perfctr_event = 0;
unsigned int prof_shift, PC_resolution = DFL_PC_RES;
unsigned int perfctr_freq = 1000;
unsigned long unload_timeout = 0;

prof_hook_p *prof_intr_hook = &prof_timer_hook;
prof_hook_p prof_perfctr_aux_hook = NULL;

/* This buffer holds PC samples */
PC_sample_count_t *PC_sample_buf = NULL;
size_t PC_buf_sz;

/* Switch for /proc files created */
int proc_created = 0;

int proc_handle;

/*
 * These variables deal with the call graph.  The call graph records arcs
 * linking the location of each function call to the address of the called
 * function.  It is maintained as a hash table indexed by a call site's
 * location.  The bucket associated with each hash table entry records the
 * targets of the calls.
 */
unsigned short *cg_from_base = NULL;
struct cg_arc_dest *cg_to_base = NULL;
size_t cg_from_sz, cg_to_sz;
int cg_arc_overflow; /* set when no new arcs can be added to the call graph */
int n_buckets = 0;

size_t mem_needed;   /* space needed for the call graph and the PC samples */

/* And these hold backtrace samples */
struct trace_ring_buf {
	unsigned long *data;
	int start;
	int end;
	int active;
};

struct trace_ring_buf trace_bufs[NR_CPUS];

prof_mem_map_t memory_map;

unsigned char cpu_prof_enabled[NR_CPUS];
unsigned long cpu_prof_enable_map = ~0UL;

#define DEBUG_RECUR_COUNT_MAX 4
static union {
	struct percpu_data {
		unsigned long lost_ones;
		unsigned long total_mcount;
		unsigned long debug_recurse_count[DEBUG_RECUR_COUNT_MAX];
		unsigned int  amhere;
	} d;
	char __pad [SMP_CACHE_BYTES];
} kernprof_cpu_data [NR_CPUS] __cacheline_aligned;

MODULE_AUTHOR("Dimitris Michailidis");
MODULE_DESCRIPTION("Kernel profile driver");

MODULE_PARM(PC_resolution, "i");
MODULE_PARM_DESC(PC_resolution, "resolution of PC samples "
		                "(rounded down to a power of 2)");

/* round x up to a multiple of n.  n must be a power of 2 */
static inline size_t roundup(size_t x, int n)
{
	return (x + n - 1) & ~(n - 1);
}

/* The next few definitions deal with procfs */
static ssize_t read_prof_buf(char *prof_buf, size_t prof_buf_sz,
			     char *user_buf, size_t count, loff_t *ppos)
{
	if (!prof_buf)
		return -EIO;
	if (*ppos >= prof_buf_sz)
		return 0;
	if (count > prof_buf_sz - *ppos)
		count = prof_buf_sz - *ppos;
	copy_to_user(user_buf, prof_buf + *ppos, count);
	*ppos += count;
	return count;
}

static ssize_t read_PC_samples(struct file *file, char *user_buf,
			       size_t count, loff_t *ppos)
{
	return read_prof_buf((char *)PC_sample_buf, PC_buf_sz, user_buf,
			     count, ppos);
}

static struct file_operations proc_PC_sample_operations = {
	read: read_PC_samples,
};

static ssize_t read_call_graph(struct file *file, char *user_buf,
			       size_t count, loff_t *ppos)
{
	return read_prof_buf((char *)cg_from_base, (cg_from_sz + cg_to_sz) * smp_num_cpus,
			     user_buf, count, ppos);
}

static struct file_operations proc_call_graph_operations = {
	read: read_call_graph,
};

static void expand_enable_map(void)
{
	int i;

	for (i = 0; i < NR_CPUS; ++i)
		cpu_prof_enabled[i] = (cpu_prof_enable_map & (1L << i)) != 0;
}

static void prof_reset(void)
{
	int i;
	if (PC_sample_buf)
		memset(PC_sample_buf, 0, mem_needed);
	cg_arc_overflow = 0;
	prof_pid = 0;
	for (i = 0; i < smp_num_cpus; i++) {
#ifdef CONFIG_LIMIT_RECURS
		int c;
		for (c = 0; c < DEBUG_RECUR_COUNT_MAX; c++) {
			kernprof_cpu_data[i].d.debug_recurse_count[c] = 0L;
		}
#endif
		kernprof_cpu_data[i].d.total_mcount = 0L;
		kernprof_cpu_data[i].d.lost_ones    = 0L;
		trace_bufs[i].start = 0;
		trace_bufs[i].end   = PROF_BACKTRACE_BUFSIZE - 1;
	}
}

/* Deallocate profiling buffers */
static void prof_free_mem(void)
{
	int i;
	
	/* vfree() and kfree() handle NULL pointers */
	vfree(PC_sample_buf);
	PC_sample_buf = NULL;
	for (i = 0; i < smp_num_cpus; ++i)
		kfree(trace_bufs[cpu_logical_map(i)].data);
}

/*
 * Allocate memory for the various profiling buffers. We are lazy and only do
 * this if we really try to use the profiling facilities.
 */
static int prof_alloc_mem(void)
{
	char *p;
	int i;

	if ((p = vmalloc(mem_needed)) == NULL)
		return -ENOMEM;
	PC_sample_buf = (PC_sample_count_t *) p;
	memory_map.nr_cpus = smp_num_cpus;
	if (supports_call_graph)
	{
		cg_from_base = (unsigned short *) (p + PC_buf_sz);
		cg_to_base = (struct cg_arc_dest *) (p + PC_buf_sz + cg_from_sz * smp_num_cpus);
		memory_map.cg_from_size = cg_from_sz;
		memory_map.cg_to_size = cg_to_sz;
		memory_map.cg_to_offset = cg_from_sz * smp_num_cpus;
	}
	else
	{
		memory_map.cg_from_size = 0L;
		memory_map.cg_to_size = 0L;
		memory_map.cg_to_offset = 0L;
	}
	if (prof_have_frameptr)  /* allocate ring buffers for present CPUs */
		for (i = 0; i < smp_num_cpus; ++i) {
			int cpu = cpu_logical_map(i);

			trace_bufs[cpu].data = (unsigned long *)kmalloc(
				PROF_BACKTRACE_BUFSIZE * sizeof(unsigned long),
				GFP_KERNEL);
		}
	prof_reset();
	return 0;
}

/* Record a PC sample.  Called from interrupt handlers.  SMP safe. */
static void PC_sample(struct pt_regs *regs)
{
	unsigned long pc;

	if (!cpu_prof_enabled[smp_processor_id()]) return;
	if (prof_pid && (!current || current->pid != prof_pid)) return;

	pc = instruction_pointer(regs);
	if (user_mode(regs))
		pc = FUNCTIONPC(USER);
	else if (in_firmware(regs))
		pc = FUNCTIONPC(FIRMWARE);
	else if (pc >= memory_map.module_start && pc < memory_map.module_end)
		pc = FUNCTIONPC(MODULE);
	else if (pc_out_of_range(pc))
		pc = FUNCTIONPC(UNKNOWN_KERNEL);

	pc -= (unsigned long) &_stext;
	atomic_inc((atomic_t *) &PC_sample_buf[pc >> prof_shift]);
}

/* Record PC samples when woken up, called from schedule()
 * blocked --> time spent sleeping on a wait queue
 * stalled --> time spent runnable yet not running
 */
static void PC_wakeup_sample(unsigned long frompc, unsigned long blocked,
			     unsigned long stalled)
{
	if (!cpu_prof_enabled[smp_processor_id()]) return;
	if (prof_pid && (!current || current->pid != prof_pid)) return;
	
	if (blocked == 0)
		goto stalled;

	frompc = FUNCTIONPC(SLEEPING) - (unsigned long) &_stext;
	atomic_add(blocked * (get_prof_freq() / HZ),
		   (atomic_t *) &PC_sample_buf[frompc >> prof_shift]);

 stalled:
	if (!stalled)
		return;

	frompc = FUNCTIONPC(STALLED) - (unsigned long) &_stext;
	atomic_add(stalled * (get_prof_freq() / HZ),
		   (atomic_t *) &PC_sample_buf[frompc >> prof_shift]);
}

/* Maintain function call counts. Called by mcount().  SMP safe. */
void record_fn_call(unsigned long not_used, unsigned long pc)
{
	if (prof_pid && (!current || current->pid != prof_pid)) return;
	if (pc_out_of_range(pc))
	{
		if (pc >= memory_map.module_start && pc < memory_map.module_end)
			pc = FUNCTIONPC(MODULE);
		else
			pc = FUNCTIONPC(UNKNOWN_KERNEL);
	}
	pc -= (unsigned long) &_stext;
	atomic_inc((atomic_t *) &PC_sample_buf[pc >> prof_shift]);
}

/* Record an arc traversal in the call graph.  Called by mcount().  SMP safe */
void cg_record_arc(unsigned long frompc, unsigned long selfpc)
{
#ifndef __HAVE_ARCH_CMPXCHG16
	static spinlock_t cg_record_lock = SPIN_LOCK_UNLOCKED;
	unsigned long flags;
#endif
	int toindex;
	int fromindex;
	int cpu;
	unsigned short *q;
	struct cg_arc_dest *p;
        unsigned short *cg_from;
        struct cg_arc_dest *cg_to;
#ifdef CONFIG_LIMIT_RECURS
        uint *ishere;
#endif /* CONFIG_LIMIT_RECURS */

	cpu = smp_processor_id();
        if (!cpu_prof_enabled[cpu])
		return;
	kernprof_cpu_data[cpu].d.total_mcount++;
#ifdef CONFIG_LIMIT_RECURS
	ishere = &kernprof_cpu_data[cpu].d.amhere;
	toindex = atomic_add_return(1, (atomic_t *)ishere) - 2;
	if (unlikely(toindex >= 0)) {
		/* Ongoing decrements (see below) should keep index in range */
		if (toindex >= DEBUG_RECUR_COUNT_MAX)   BUG();
        	kernprof_cpu_data[cpu].d.debug_recurse_count[toindex]++;
		/* If we're at the highest recursion count, then bail out! */
        	if (toindex == DEBUG_RECUR_COUNT_MAX-1) {
			atomic_dec((atomic_t *)ishere);
			return;
		}
	}
#endif /* CONFIG_LIMIT_RECURS */
	cg_from = (u_short *)(((char *)cg_from_base) + cg_from_sz * cpu);
	cg_to = &cg_to_base[CG_MAX_ARCS * cpu];
	if (pc_out_of_range(frompc))
	{
	   if (frompc >= memory_map.module_start && frompc < memory_map.module_end)
	      fromindex = (FUNCTIONPC(MODULE) - (unsigned long)&_stext) >> prof_shift;
	   else
	      fromindex = (FUNCTIONPC(UNKNOWN_KERNEL) - (unsigned long)&_stext) >> prof_shift;
	}
	else
		fromindex = (frompc - (unsigned long) &_stext) >> prof_shift;
	q = &cg_from[fromindex];
	
	/* Easy case: the arc is already in the call graph */
	for (toindex = *q; toindex != 0; ) {
		p = &cg_to[toindex];
		if (p->address == selfpc) {
			atomic_inc(&p->count);
#ifdef CONFIG_LIMIT_RECURS
			atomic_dec((atomic_t *)ishere);
#endif /* CONFIG_LIMIT_RECURS */
			return;
		}
		toindex = p->link;
	}

	/*
	 * No luck.  We need to add a new arc.  Since cg_to[0] is unused,
	 * we use cg_to[0].count to keep track of the next available arc.
	 */
	if (cg_arc_overflow)
	{
		kernprof_cpu_data[cpu].d.lost_ones++;
#ifdef CONFIG_LIMIT_RECURS
		atomic_dec((atomic_t *)ishere);
#endif /* CONFIG_LIMIT_RECURS */
		return;
	}
	toindex = atomic_add_return(1, &cg_to->count);
	if (toindex >= CG_MAX_ARCS) {
		/*
		 * We have run out of space for arcs.  We'll keep incrementing
		 * the existing ones but we won't try to add any more.
		 */
		kernprof_cpu_data[cpu].d.lost_ones++;
		cg_arc_overflow = 1;
		atomic_set(&cg_to->count, CG_MAX_ARCS - 1);
#ifdef CONFIG_LIMIT_RECURS
		atomic_dec((atomic_t *)ishere);
#endif /* CONFIG_LIMIT_RECURS */
		return;
	}

	/*
	 * We have a secured slot for a new arc and all we need to do is
	 * initialize it and add it to a hash bucket.  We use compare&swap, if
	 * possible, to avoid any spinlocks whatsoever.
	 */
	p = &cg_to[toindex];
	p->address = selfpc;
	atomic_set(&p->count, 1);
#ifdef __HAVE_ARCH_CMPXCHG16
	do {
		p->link = *q;
	} while (cmpxchg(q, p->link, toindex) != p->link);
#else
	spin_lock_irqsave(&cg_record_lock, flags);
	p->link = *q;
	*q = toindex;
	spin_unlock_irqrestore(&cg_record_lock, flags);
#endif
#ifdef CONFIG_LIMIT_RECURS
	atomic_dec((atomic_t *)ishere);
#endif /* CONFIG_LIMIT_RECURS */
	return;
}

/*
 * Record an arc traversal in the call graph, and walk up the stack to
 * find and record all the call graph arcs.  Called by schedule() (and
 * potentially others).  SMP safe.
 */
void backtrace_cg_record_arc(unsigned long frompc, unsigned long selfpc)
{
	int backtrace_count = PROF_BACKTRACE_MAX_LEN;	/* for safety */
	frame_info_t frame;
	unsigned long caller_pc, callee_pc;

	if (prof_pid && (!current || current->pid != prof_pid))
		return;

	/* If can't build fake frame, then record what info we have and leave */
	if (!build_fake_frame(&frame)) {
#ifndef CONFIG_IA64
		caller_pc = frompc;
		callee_pc = (selfpc) ? selfpc
				: (unsigned long)__builtin_return_address(0);
		cg_record_arc(caller_pc, callee_pc);
#endif
		return;
	}

	/* Walk back to who called us */
	if (!get_next_frame(&frame)) {
		return;
	}
	callee_pc = frame_get_pc(&frame);
	if (pc_out_of_range(callee_pc)) {
		return;
	}

	/* Now walk back to who called our caller, giving us the 1st cg arc */
	if (!get_next_frame(&frame)) {
		printk("  computed callee_pc:0x%lx\n", callee_pc & 0xffffffffL);
		printk("  caller-supplied caller:0x%lx callee:0x%lx\n",
			frompc & 0xffffffffL, selfpc & 0xffffffffL);
		BUG();	/* debug */
		return;
	}
	caller_pc = frame_get_pc(&frame);
	if (pc_out_of_range(caller_pc)) {
		return;
	}
	/* Now record this cg arc and keep walking back the stack for more */
	while (backtrace_count--) {
		cg_record_arc(caller_pc, callee_pc);
		callee_pc = caller_pc;
		if (!get_next_frame(&frame))
			break;		/* quit! */
		caller_pc = frame_get_pc(&frame);
		if (pc_out_of_range(caller_pc))
			break;		/* quit! */
		backtrace_count--;
	}
}

#define PROF_TRACE_MASK (PROF_BACKTRACE_BUFSIZE - 1)

/* circularly increment i to point to the next entry in a trace ring buffer */
#define CIRC_INC(i)     (((i) + 1) & PROF_TRACE_MASK)

/*
 * In backtrace mode, add a sample to the per-processor trace bufs.
 *
 * If frame is NULL, there is no backtrace. Just record a length 1
 * backtrace at alt_pc.
 *
 * If frame is non-NULL, use it to perform a backtrace, generating a
 * list of PCs to add onto the trace bufs.
 *
 * If frame is non-NULL, and alt_pc is non-NULL, same as before, except
 * force alt_pc to be at the head of the backtrace, and pretend that the
 * first function on the frame called alt_pc.
 */

static void do_backtrace_sample(frame_info_t *frame, unsigned long alt_pc,
				unsigned long count)
{
	int free_slots, j, n_entries;
	struct trace_ring_buf *p;

	p = &trace_bufs[smp_processor_id()];
	if (!p->active ||
	    ((free_slots = ((p->end - p->start) & PROF_TRACE_MASK)) < 3))
		goto out;
	j = CIRC_INC(p->start);
	n_entries = 1;

	if (!frame) {
		p->data[j] = alt_pc;
		goto end_trace;
	}

	/* We set aside one slot for the trace length */
	if (--free_slots > PROF_BACKTRACE_MAX_LEN)
		free_slots = PROF_BACKTRACE_MAX_LEN;

	n_entries = 0;
	if (alt_pc) {
		p->data[j] = alt_pc;
		if (++n_entries == free_slots)
			goto end_trace;
		j = CIRC_INC(j);
	}
	while (1) {
		p->data[j] = frame_get_pc(frame);
		if (pc_out_of_range(p->data[j])) {
	   		if (p->data[j] >= memory_map.module_start &&
			    p->data[j] < memory_map.module_end)
				p->data[j] = FUNCTIONPC(MODULE);
			else
				p->data[j] = FUNCTIONPC(UNKNOWN_KERNEL);
		}
		if (++n_entries == free_slots || !get_next_frame(frame))
			break;
		j = CIRC_INC(j);
	}
end_trace:
	/* count goes in upper half of data value. 0 is interpreted as a 1 */
	p->data[p->start] = (count << ((sizeof count) * 4)) | n_entries;
	p->start = CIRC_INC(j);
out:    return;
}

/* Record a stack backtrace.  Called from interrupt handlers. No MP issues. */
static void backtrace_sample(struct pt_regs *regs)
{
	frame_info_t frame;
	u_long pc;

	if (!cpu_prof_enabled[smp_processor_id()])
		return;
	if (prof_pid && (!current || current->pid != prof_pid))
		return;

	/* Check for corner cases, otherwise generate frame from regs */

	if (user_mode(regs)) {
		pc = FUNCTIONPC(USER);
		do_backtrace_sample(NULL, pc, 0);
	} else if (in_firmware(regs)) {
		pc = FUNCTIONPC(FIRMWARE);
		do_backtrace_sample(NULL, pc, 0);
	} else if (pc_out_of_range(instruction_pointer(regs))) {
		if (instruction_pointer(regs) >= memory_map.module_start &&
			instruction_pointer(regs) < memory_map.module_end)
			{
				pc = FUNCTIONPC(MODULE);
				do_backtrace_sample(NULL, pc, 0);
			} else {
				pc = FUNCTIONPC(UNKNOWN_KERNEL);
				do_backtrace_sample(NULL, pc, 0);
			}
	} else {
		/* We have a pc value within the static kernel text area */
		get_top_frame(regs, &frame);
		pc = instruction_pointer(regs);
		do_backtrace_sample(&frame, 0, 0);
	}

	pc -= (u_long) &_stext;
	atomic_inc((atomic_t *) &PC_sample_buf[pc >> prof_shift]);
}

static void backtrace_wakeup_sample(unsigned long frompc, unsigned long blocked,
				    unsigned long stalled)
{
	frame_info_t frame;
	u_long pc;

	if (!cpu_prof_enabled[smp_processor_id()])
		return;

	if (prof_pid == 0)
		printk("kernprof error: backtrace_wakeup_sample but prof_pid == 0\n");

	if (!current || current->pid != prof_pid)
		return;

	if (!build_fake_frame(&frame))
		return;

	if (!get_next_frame(&frame))
		return;

	if (blocked) {
		pc = FUNCTIONPC(SLEEPING);
		do_backtrace_sample(&frame, pc,
				    blocked * (get_prof_freq() / HZ));

		pc -= (u_long) &_stext;
		atomic_add(blocked * (get_prof_freq() / HZ),
			   (atomic_t *) &PC_sample_buf[pc >> prof_shift]);
	}

	if (stalled) {
		pc = FUNCTIONPC(STALLED);
		do_backtrace_sample(NULL, pc,
				    stalled * (get_prof_freq() / HZ));
		pc -= (u_long) &_stext;
		atomic_add(stalled * (get_prof_freq() / HZ),
			   (atomic_t *) &PC_sample_buf[pc >> prof_shift]);
	}
}

static ssize_t trace_read(struct file *file, char *buf,
			  size_t count, loff_t *ppos)
{
	struct trace_ring_buf *p;
	size_t avail, entries_to_write;

	p = &trace_bufs[minor(file->f_dentry->d_inode->i_rdev) - 1];
	avail = (PROF_BACKTRACE_BUFSIZE - 1) + p->start - p->end;
	avail &= PROF_TRACE_MASK;

	entries_to_write = count / sizeof(*p->data);
	if (entries_to_write > avail)
		entries_to_write = avail;
	if (entries_to_write == 0)
		return 0;
	count = entries_to_write * sizeof(*p->data);
	if (p->end + entries_to_write < PROF_BACKTRACE_BUFSIZE) {
		copy_to_user(buf, (void *)&p->data[p->end + 1], count);
		p->end += entries_to_write;
	} else {
		size_t first_part;

		avail = (PROF_BACKTRACE_BUFSIZE - 1) - p->end;
		first_part = avail * sizeof(*p->data);

		if (avail)
			copy_to_user(buf, (void *)&p->data[p->end + 1],
				     first_part);
		copy_to_user(buf + first_part, (void *)&p->data[0],
			     count - first_part);
		p->end = entries_to_write - avail - 1;
	}
	return count;
}

static int trace_release(struct inode *inode, struct file *filp)
{
	trace_bufs[minor(inode->i_rdev) - 1].active = 0;
        return 0;
}

static struct file_operations prof_trace_fops = {
	owner: THIS_MODULE,
	read: trace_read,
	release: trace_release,
};

/*
 * The perf counter interrupt handler calls this function which then calls the
 * appropriate sampling function.  We do this because we may need to reload the
 * perf counter after it overflows.
 */
void perfctr_aux_intr_handler(struct pt_regs *regs)
{
	prof_perfctr_aux_hook(regs);
	perfctr_reload(perfctr_freq);
}

/* Start the performance monitoring counters */
static void perfctr_commence(void *dummy)
{
	__perfctr_commence(perfctr_freq, perfctr_event);
}

/* Stop the performance monitoring counters */
static void perfctr_stop(void *dummy)
{
	__perfctr_stop();
}

/* Open a profiling device */
static int prof_open(struct inode *inode, struct file *filp)
{
	int minor = minor(inode->i_rdev);

	if (minor != PROF_CNTRL_MINOR) {
		--minor;
		if (minor >= NR_CPUS || trace_bufs[minor].data == NULL)
			return -ENODEV;

		filp->f_op = &prof_trace_fops;
		trace_bufs[minor].start = 0;
		trace_bufs[minor].end = PROF_BACKTRACE_BUFSIZE - 1;
		trace_bufs[minor].active = 1;
	}

	return 0;
}

static void prof_stop(void)
{
	if (prof_mode & PROF_MODE_CALL_GRAPH) {
		/* Aggregate per-cpu counts into all-cpu counts to display */
		unsigned long total_mcount = 0L;
		unsigned long lost_ones = 0L;
		int i;
#ifdef CONFIG_LIMIT_RECURS
		int ii;
		unsigned long recur_counts[DEBUG_RECUR_COUNT_MAX];
		for (i = 0; i < DEBUG_RECUR_COUNT_MAX; i++)
			recur_counts[i] = 0L;
#endif
		for (i = 0; i < smp_num_cpus; i++) {
			total_mcount += kernprof_cpu_data[i].d.total_mcount;
			lost_ones    += kernprof_cpu_data[i].d.lost_ones;
#ifdef CONFIG_LIMIT_RECURS
			for (ii = 0; ii < DEBUG_RECUR_COUNT_MAX; ii++)
				recur_counts[ii] += kernprof_cpu_data[i].d.debug_recurse_count[ii];
#endif
		}
#ifdef CONFIG_LIMIT_RECURS
		if (lost_ones || recur_counts[DEBUG_RECUR_COUNT_MAX-1]) {
#else
		if (lost_ones) {
#endif
			printk("Total mcount invocations: %12lu\n",
				total_mcount);
			printk("Lost to table overflow:   %12lu\n",
				lost_ones);
#ifdef CONFIG_LIMIT_RECURS
			printk("Lost to recursive invoc:  %12lu\n",
				recur_counts[DEBUG_RECUR_COUNT_MAX-1]);
			printk("Recursion depth:counts: ");
			for (ii = 0; ii < DEBUG_RECUR_COUNT_MAX-1; ii++)
				printk(" %d:%lu ", ii+1, recur_counts[ii]);
			printk("\n");
#endif /* CONFIG_LIMIT_RECURS */
		}
	}
	if (prof_perfctr_hook) {
		smp_call_function(perfctr_stop, NULL, 1, 0);
		perfctr_stop(NULL);
	}
	prof_timer_hook = prof_perfctr_hook = NULL;
	mcount_hook = NULL;
	prof_scheduler_hook = NULL;
	prof_wakeup_hook = NULL;
	if (prof_enabled) {
		unload_timeout = jiffies + HZ;
		prof_enabled = 0;
		MOD_DEC_USE_COUNT;
	}
}

extern struct module *module_list;
extern struct module *static_module_list;

int prof_get_module_map(prof_mem_map_t *map)
{
   struct module        *mod;
   struct module_symbol *s;
   char                 *t;
   u_long                low = (u_long)-1L;
   u_long                high = 0L;
   u_long                end;
   int                   i;

   for (mod = module_list; mod != static_module_list; mod = mod->next)
   {
      if (mod->flags & MOD_RUNNING)
      {
         for (i = 0, s = mod->syms; i < mod->nsyms; i++, s++)
         {
            if ((t = strstr(s->name, "_S.text_L")))
            {
               if (s->value < low)
                  low = s->value;
               end = mod->size + s->value;
               if (end > high)
                  high = end;
            }
         }
      }
   }
   if (high)
   {
      map->module_start = low;
      map->module_end = high;
      map->module_buckets = 0;
      return(0);
   }
   return(-1);
}

int create_proc_files(void)
{
   struct proc_dir_entry *ent;
   prof_mem_map_t m_map;

   if (prof_get_module_map(&m_map))
   {
      m_map.module_start = m_map.module_end = 0L;
      m_map.module_buckets = 0;
   }
   if (n_buckets != memory_map.kernel_buckets + m_map.module_buckets)
   {
      if (proc_created)
      {
         remove_proc_entry("profile/PC_samples", 0);
         if (supports_call_graph)
            remove_proc_entry("profile/call_graph", 0);
         remove_proc_entry("profile", 0);
         prof_free_mem();
         proc_created = 0;
      }
      memory_map.module_buckets = 0;
      memory_map.module_start = m_map.module_start;
      memory_map.module_end = m_map.module_end;
      n_buckets = memory_map.kernel_buckets;
   }

   if (proc_created)
      return(0);

   PC_buf_sz = n_buckets * sizeof(PC_sample_count_t);

   if (!proc_mkdir("profile", 0))
   {
      printk(KERN_ERR "kernprof: unable to create /proc entries\n");
      return -ENODEV;
   }
   if ((ent = create_proc_entry("profile/PC_samples", 0, 0)) != NULL)
   {
      ent->size = PC_buf_sz;
      ent->proc_fops = &proc_PC_sample_operations;
   }
   else
      printk("Unable to do create_proc_entry for PC_samples\n");

   if (supports_call_graph)
   {
      /*
       * Calculate size of call graph structures.  The round-ups
       * ensure that pointers to these structures are properly
       * aligned.
       */
      cg_from_sz = n_buckets * sizeof(short);
      cg_to_sz = CG_MAX_ARCS * sizeof(struct cg_arc_dest);
      
      PC_buf_sz = roundup(PC_buf_sz, sizeof(unsigned long));
      cg_from_sz = roundup(cg_from_sz, sizeof(unsigned long));
      mem_needed = PC_buf_sz + cg_from_sz * smp_num_cpus + cg_to_sz * smp_num_cpus ;
      
      if ((ent = create_proc_entry("profile/call_graph", 0, 0)))
      {
         ent->size = cg_to_sz * smp_num_cpus  + cg_from_sz * smp_num_cpus;
         ent->proc_fops = &proc_call_graph_operations;
      }
      else
         printk("Unable to do create_proc_entry for call_graph\n");
   }
   else
      mem_needed = PC_buf_sz;

   proc_created = 1;
   return(0);
}

/*
 * ioctl handler for the kernprof control device.
 */
int prof_ctl_ioctl(struct inode *inode, struct file *filp,
		   unsigned int command, unsigned long arg)
{
	int err = 0;

	switch (command) {
	case PROF_START:
		if (prof_enabled)
			return 0;
		if (create_proc_files())
		{
			err = -EINVAL;
			return err;
		}
		if (PC_sample_buf == NULL && (err = prof_alloc_mem()))
			return err;
		MOD_INC_USE_COUNT;
		prof_enabled = 1;
		if (prof_mode & PROF_MODE_CALL_GRAPH)
		{
		   mcount_hook = cg_record_arc;
		}
		else if (prof_mode & PROF_MODE_CALL_COUNT)
		{
		   mcount_hook = record_fn_call;
		}
		else if (prof_mode & PROF_MODE_SCHEDULER_CALL_GRAPH)
			prof_scheduler_hook = backtrace_cg_record_arc;
		if (prof_mode & PROF_MODE_PC_SAMPLING) {
			*prof_intr_hook = PC_sample;
			if (prof_pid)
				prof_wakeup_hook = PC_wakeup_sample;
		} else if (prof_mode & PROF_MODE_BACKTRACE) {
			*prof_intr_hook = backtrace_sample;
			if (prof_pid)
				prof_wakeup_hook = backtrace_wakeup_sample;
		}
		if (prof_domain == PROF_DOMAIN_PERFCTR) {
			if (!(prof_mode & PROF_MODE_PC_SAMPLING) &&
			    !(prof_mode & PROF_MODE_BACKTRACE))
			{
				err = -EINVAL;
				return err;
			}
			prof_perfctr_hook = perfctr_aux_intr_handler;
			smp_call_function(perfctr_commence, NULL, 1, 0);
			perfctr_commence(NULL);
		}
		break;
	case PROF_STOP:
		prof_stop();
		break;
	case PROF_RESET:
		prof_stop();         /* resetting also stops profiling */
		prof_reset();
		break;
	case PROF_SET_SAMPLE_FREQ:
		if (prof_domain == PROF_DOMAIN_TIME)
			err = setup_profiling_timer(arg);
		else if (prof_domain == PROF_DOMAIN_PERFCTR) {
			if (valid_perfctr_freq(arg))
				perfctr_freq = arg;
			else
				err = -EINVAL;
		} else
			err = EINVAL;
		break;
	case PROF_GET_SAMPLE_FREQ:
		if (prof_domain == PROF_DOMAIN_TIME) {
			unsigned int freq = get_prof_freq();
			err = copy_to_user((void *)arg, &freq, sizeof freq) ?
				-EFAULT : 0;
		} else
			err = copy_to_user((void *)arg, &perfctr_freq,
					   sizeof perfctr_freq) ? -EFAULT : 0;
		break;
	case PROF_GET_PC_RES:
		err = copy_to_user((void *)arg, &PC_resolution,
				   sizeof PC_resolution) ? -EFAULT : 0;
		break;
	case PROF_GET_ON_OFF_STATE:
		err = copy_to_user((void *)arg, &prof_enabled,
				   sizeof prof_enabled) ? -EFAULT : 0;
		break;
	case PROF_SET_DOMAIN:
		if (arg != prof_domain)  /* changing domains stops profiling */
			prof_stop();
		if (arg == PROF_DOMAIN_TIME) {
			prof_domain = arg;
			prof_intr_hook = &prof_timer_hook;
		} else if (arg == PROF_DOMAIN_PERFCTR && have_perfctr()) {
			prof_domain = arg;
			prof_intr_hook = &prof_perfctr_aux_hook;
		} else
			err = -EINVAL;
		break;
	case PROF_GET_DOMAIN:
		err = copy_to_user((void *)arg, &prof_domain,
				   sizeof prof_domain) ? -EFAULT : 0;
		break;
	case PROF_SET_MODE:
		if (arg != prof_mode) /* changing modes also stops profiling */
			prof_stop();
		if (arg == PROF_MODE_PC_SAMPLING)
			prof_mode = arg;
		else if (arg == PROF_MODE_BACKTRACE && prof_have_frameptr)
			prof_mode = arg;
		else if (arg == PROF_MODE_CALL_COUNT && prof_have_mcount)
			prof_mode = arg;
		else if (supports_call_graph &&
			  (arg == PROF_MODE_SCHEDULER_CALL_GRAPH ||
			   arg == PROF_MODE_CALL_GRAPH ||
			   arg == (PROF_MODE_CALL_GRAPH|PROF_MODE_PC_SAMPLING)))
			prof_mode = arg;
		else
			err = -EINVAL;
		break;
	case PROF_GET_MODE:
		err = copy_to_user((void *)arg, &prof_mode, sizeof prof_mode) ?
			-EFAULT : 0;
		break;
	case PROF_SET_PID:
		if (prof_enabled) /* don't change PID while profiling */
			err = -EINVAL;
		else {
			prof_reset();
			prof_pid = arg;
		}
		break;
	case PROF_GET_PID:
		err = copy_to_user((void *)arg, &prof_pid, sizeof prof_pid) ?
 			-EFAULT : 0;
 		break;
	case PROF_SET_PERFCTR_EVENT:
		if (have_perfctr() && valid_perfctr_event(arg))
			perfctr_event = arg;
		else
			err = -EINVAL;
		break;
	case PROF_GET_PERFCTR_EVENT:
		if (have_perfctr())
			err = copy_to_user((void *)arg, &perfctr_event,
					   sizeof perfctr_event) ? -EFAULT : 0;
		else
			err = -EINVAL;
		break;
	case PROF_SET_ENABLE_MAP:
		if (get_user(cpu_prof_enable_map, (u_long *)arg))
			err = -EFAULT;
		else {
			cpu_prof_enable_map &= cpu_online_map;
			expand_enable_map();
		}
		break;
	case PROF_GET_ENABLE_MAP:
		err = copy_to_user((void *)arg, &cpu_prof_enable_map,
				   sizeof cpu_prof_enable_map) ? -EFAULT : 0;
		break;
	case PROF_GET_MAPPING:
		err = copy_to_user((void *)arg, &memory_map,
				   sizeof memory_map) ? -EFAULT : 0;
		break;
	default:
		err = -EINVAL;
	}

	return err;
}

static struct file_operations prof_ctl_fops = {
	owner: THIS_MODULE,
	ioctl: prof_ctl_ioctl,
	open: prof_open,
};

#ifndef MODULE
static int __init kernprof_setup(char *str)
{
	int res;

	if (get_option(&str, &res)) PC_resolution = res;
	return 1;
}

__setup("kernprof=", kernprof_setup);
#else
static int can_unload(void)
{
	int ret = atomic_read(&__this_module.uc.usecount);

	/*
	 * It is conceivable that we may try to delete this module just as 
	 * an interrupt handler is trying to write into a profile buffer.
	 * Since unloading the module frees the buffers that would be
	 * unfortunate.  To avoid such races this module may not be unloaded 
	 * within one second after profiling is turned off.
	 */
	if (time_before(jiffies, unload_timeout))
		ret = 1;

	return ret;
}
#endif

int __init kernprof_init(void)
{
	size_t text_size = (unsigned long) &_etext - (unsigned long) &_stext;
	int ret;
	
	/* Round PC_resolution down to a power of 2 and compute its log */
	if (PC_resolution == 0)
		PC_resolution = DFL_PC_RES;
	while ((PC_resolution & (PC_resolution - 1)) != 0)
		PC_resolution &= PC_resolution - 1;
	for (prof_shift = 0; (1 << prof_shift) < PC_resolution; prof_shift++);

	/* Calculate size of PC-sample buffer. */
	memory_map.kernel_buckets = n_buckets = text_size >> prof_shift;
	memory_map.kernel_start = (u_long)&_stext;
	memory_map.kernel_end = (u_long)&_etext;

#ifdef MODULE
	__this_module.can_unload = can_unload;
#endif
	memset(trace_bufs, 0, sizeof trace_bufs);

	cpu_prof_enable_map = cpu_online_map;
	expand_enable_map();

	ret = devfs_register_chrdev(KERNPROF_MAJOR, "profile", &prof_ctl_fops);
	if (ret < 0)
		return ret;
	proc_handle = devfs_register(NULL, "profile",
				     DEVFS_FL_NONE, KERNPROF_MAJOR, 0,
				     S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP,
				     &prof_ctl_fops, NULL);
	return 0;
}

/* This must be static for some reason */
static void __exit kernprof_exit(void)
{
	devfs_unregister(proc_handle);
	devfs_unregister_chrdev(KERNPROF_MAJOR, "profile");
	remove_proc_entry("profile/PC_samples", 0);
	if (supports_call_graph)
		remove_proc_entry("profile/call_graph", 0);
	remove_proc_entry("profile", 0);
	prof_free_mem();
}

module_init(kernprof_init);
module_exit(kernprof_exit);
