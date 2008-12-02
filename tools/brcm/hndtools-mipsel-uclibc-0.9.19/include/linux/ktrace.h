/*
 * Simple kernel trace
 * $Id$
 */

#ifndef _LINUX_KTRACE_H
#define _LINUX_KTRACE_H

#include <linux/config.h>

#ifdef CONFIG_KTRACE

/* kernel subsystems */
#define	KT_SYSCALL	0x1
#define	KT_TRAP		0x2
#define	KT_PROC		0x4
#define	KT_IRQ		0x8
#define	KT_MM		0x10
#define	KT_SOCK		0x20
#define	KT_NET		0x40
#define	KT_FS		0x80

extern int ktracectl;
extern void _ktrace(char *fmt, unsigned long a1, unsigned long a2);

#define	ktrace(subsys, fmt, a1, a2)	if ((subsys) & ktracectl) _ktrace(fmt, (unsigned long)a1, (unsigned long)a2)

#else
#define	ktrace(subsys, fmt, a1, a2)
#endif	/* CONFIG_KTRACE */

#endif	/* _LINUX_KTRACE_H */
