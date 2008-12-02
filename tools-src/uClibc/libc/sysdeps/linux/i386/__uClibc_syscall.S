/*
 * June 27, 2001             Manuel Novoa III
 *
 * This is a heavily modified version of gcc's output for the syscall5 macro.
 * The idea (originally from dietlibc) is that all syscall functions simply
 * set the syscall number in %al (since <= 255) and then jump here.  All the
 * common work is done by __uClibc_syscall, saving a fair amount of generated
 * code where a number of syscalls are used.  The (potential) cost is some 
 * unnecessary pushes, pops, and movs but the execution time penalty should
 * be relatively small compared to the cost of the syscall itself.
 *
 * WARNING: If the startup code for uClibc changes, I suppose it is possible
 *          that this code might try to access memory under the bottom of
 *          the stack.
 * WARNING: This will need to be modified if the number of syscalls ever
 *          exceeds 255.  So will the associated syscall macros.
 */
	
.text
	.align 4
.globl __uClibc_syscall
	.type	 __uClibc_syscall,@function
__uClibc_syscall:
	pushl %edi
	pushl %esi
	pushl %ebx
	and $0xff,%eax
	movl 16(%esp),%ebx
	movl 20(%esp),%ecx
	movl 24(%esp),%edx
	movl 28(%esp),%esi
	movl 32(%esp),%edi
#APP
	int $0x80
#NO_APP
	cmpl $-4095,%eax
	jbe .Ldone

#ifdef PIC
	call Lhere
Lhere:
	popl %ebx
	addl $_GLOBAL_OFFSET_TABLE_+[.-Lhere],%ebx
	negl %eax
	movl %eax,%ecx
#ifdef _LIBC_REENTRANT
	call __errno_location@PLT
#else
	movl errno@GOT(%ebx),%eax
#endif /* _LIBC_REENTRANT */
	movl %ecx,(%eax)
#else
	negl %eax
#ifdef _LIBC_REENTRANT
	movl %eax,%ecx
	call __errno_location
	movl %ecx,(%eax)
#else
	movl %eax,errno
#endif /* _LIBC_REENTRANT */

#endif /* PIC */

	movl $-1,%eax
	.p2align 4,,7
.Ldone:
	popl %ebx
	popl %esi
	popl %edi
	ret
.Lsize:
	.size	 __uClibc_syscall,.Lsize-__uClibc_syscall
