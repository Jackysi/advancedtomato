#ifndef _BITS_SYSCALLS_H
#define _BITS_SYSCALLS_H

#ifndef _SYSCALL_H
# error "Never use <bits/syscalls.h> directly; include <sys/syscall.h> instead."
#endif

/* This includes the `__NR_<name>' syscall numbers taken from the Linux kernel
 * header files.  It also defines the traditional `SYS_<name>' macros for older
 * programs.  */
#include <bits/sysnum.h>


#define STRINGIFY(s) STRINGIFY2 (s)
#define STRINGIFY2(s) #s

#ifdef __PIC__
#define JUMPTARGET(name) STRINGIFY(name##@plt)
#else
#define JUMPTARGET(name) STRINGIFY(name)
#endif

#define unified_syscall_body(name)			\
	__asm__ (					\
	".section \".text\"\n\t"			\
	".align 2\n\t"					\
	".globl " STRINGIFY(name) "\n\t"				\
	".type " STRINGIFY(name) ",@function\n"			\
	#name":\n\tli 0," STRINGIFY(__NR_##name) "\n\t"	\
	"b " JUMPTARGET(__uClibc_syscall) "\n"		\
	".Lfe1" STRINGIFY(name) ":\n\t"				\
	".size\t" STRINGIFY(name) ",.Lfe1" STRINGIFY(name) "-" STRINGIFY(name) "\n"	\
	)

#undef _syscall0
#define _syscall0(type,name)						\
type name(void);							\
unified_syscall_body(name)

#undef _syscall1
#define _syscall1(type,name,type1,arg1) \
type name(type1 arg1);  \
unified_syscall_body(name)

#undef _syscall2
#define _syscall2(type,name,type1,arg1,type2,arg2) \
type name(type1 arg1, type2 arg2);      \
unified_syscall_body(name)

#undef _syscall3
#define _syscall3(type,name,type1,arg1,type2,arg2,type3,arg3) \
type name(type1 arg1, type2 arg2, type3 arg3);  \
unified_syscall_body(name)

#undef _syscall4
#define _syscall4(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4);      \
unified_syscall_body(name)

#undef _syscall5
#define _syscall5(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5);  \
unified_syscall_body(name)

#undef _syscall6
#define _syscall6(type,name,type1,arg1,type2,arg2,type3,arg3,type4,arg4,type5,arg5,type6,arg6) \
type name(type1 arg1, type2 arg2, type3 arg3, type4 arg4, type5 arg5, type6 arg6);      \
unified_syscall_body(name)

#endif /* _BITS_SYSCALLS_H */

