/*
 * Simple kernel trace
 *
 * To enable ktrace() points:
 *   echo x > /proc/sys/ktracectl
 * To dump trace events:
 *   cat /proc/ktrace
 *
 * $Id: ktrace.c,v 1.1.1.4 2003/10/14 08:09:30 sparq Exp $
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/time.h>
#include <linux/ktrace.h>

struct ktentry {
	uint	timestamp;	/* ticks (really any metric) */
	uint	pid;		/* process id */
	char	*fmt;		/* printf() format string */
	ulong	a1;		/* first arg to fmt string */
	ulong	a2;		/* second arg to fmt string */
};

#define	KTNR	4096
static struct ktentry ktbuf[KTNR];

int ktracectl = 0;
static struct ctl_table_header *ktracectl_header;
static spinlock_t ktlock;
static uint ktwrite = 0;
static uint ktread = 0;
static uint ktdrop = 0;

#define	KTNEXT(i)	((i+1) % KTNR)

/* arch-specific fast timestamp */
#if defined(__i386__)
#include <asm/msr.h>
#define	TS(ts)		rdtscl(ts)
#elif defined(__arch_um__)
static struct timeval ktracetv;
#define TS(ts)		({ do_gettimeofday(&ktracetv); (ts) = ktracetv.tv_usec; })
#elif defined(mips)
#include <asm/mipsregs.h>
#define	TS(ts)		((ts) = read_c0_count())
#else
#warning "ktrace.c: arch-specific fast timestamp not supported"
#define	TS(ts)		((ts) = 0)
#endif

static int ktrace_read_proc(char *buf, char **start, off_t offset, int length, int *eof, void *data);

#define	KT_CTL	1
static ctl_table kt_sys_table[] = {
	{KT_CTL, "ktracectl", &ktracectl, sizeof (int), 0644, NULL, &proc_dointvec},
	{0}
};

int __init
ktrace_init(void)
{
	/* init private lock */
	spin_lock_init(&ktlock);

	/* create /proc/sys/ktracectl */
	if (!(ktracectl_header = 
	      register_sysctl_table(kt_sys_table, 1)))
		return -ENOMEM;

	/* create /proc/ktrace */
	if (!(create_proc_read_entry("ktrace", S_IFREG | 0444, 0, ktrace_read_proc, NULL))) {
		unregister_sysctl_table(ktracectl_header);
		return -ENOMEM;
	}		

	return 0;
}

void __exit
ktrace_exit(void)
{
	unregister_sysctl_table(ktracectl_header);
	remove_proc_entry("ktrace", NULL);
}

void
_ktrace(char *fmt, ulong a1, ulong a2)
{
	static uint lastts = 0;
	ulong flags;
	uint ts;
	uint next;

	spin_lock_irqsave(&ktlock, flags);

	/* if full, drop */
	if ((next = KTNEXT(ktwrite)) == ktread) {
		ktdrop++;
		goto done;
	}

	TS(ts);

	ktbuf[ktwrite].timestamp = ts - lastts;	/* delta time */
	ktbuf[ktwrite].pid = current->pid;
	ktbuf[ktwrite].fmt = fmt;
	ktbuf[ktwrite].a1 = a1;
	ktbuf[ktwrite].a2 = a2;
	ktwrite = next;

	lastts = ts;

done:
	spin_unlock_irqrestore(&ktlock, flags);
}

static int
ktrace_read_proc(char *buf, char **start, off_t offset, int length, int *eof, void *data)
{
	ulong flags;
	int i;
	int len;

	len = 0;

	for (i = ktread; (i != ktwrite) && (len < (length - 128)); i = KTNEXT(i)) {
		len += sprintf(buf+len, "%10u\t(%u)\t", ktbuf[i].timestamp, ktbuf[i].pid);
		len += sprintf(buf+len, ktbuf[i].fmt, ktbuf[i].a1, ktbuf[i].a2);
		len += sprintf(buf+len, "\n");
	}

	ktread = i;

	if (ktdrop) {
		_ktrace("ktrace: drop %d", ktdrop, 0);

		spin_lock_irqsave(&ktlock, flags);
		ktdrop = 0;
		spin_unlock_irqrestore(&ktlock, flags);
	}

	*start = buf;

	if (ktread == ktwrite)
		*eof = 1;

	return (len);
}

__initcall(ktrace_init);
__exitcall(ktrace_exit);

EXPORT_SYMBOL(ktracectl);
EXPORT_SYMBOL(_ktrace);
