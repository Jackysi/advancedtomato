/*
 * June 27, 2001             Manuel Novoa III
 *
 * Modified to (hopefully) be PIC and REENTRANT safe.
 *
 */
	
.text
	.align 4
.globl vfork
	.type	 vfork,@function
vfork:
	popl %ecx
	movl $190,%eax
#ifdef PIC
	pushl %ebx
#endif
#APP
	int $0x80
#NO_APP
#ifdef PIC
	popl %ebx
#endif
	cmpl $-4095,%eax
	jae .Lerror
	jmp *%ecx
	.p2align 4,,7
.Lerror:
	pushl %ecx

#ifdef PIC
	pushl %ebx
	call .Lhere
.Lhere:
	popl %ebx
	addl $_GLOBAL_OFFSET_TABLE_+[.-.Lhere],%ebx
	negl %eax
	movl %eax,%ecx
#ifdef __UCLIBC_HAS_THREADS__
	call __errno_location@PLT
#else
	movl errno@GOT(%ebx),%eax
#endif /* __UCLIBC_HAS_THREADS__ */
	movl %ecx,(%eax)
	popl %ebx
#else
	negl %eax
#ifdef __UCLIBC_HAS_THREADS__
	movl %eax,%ecx
	call __errno_location
	movl %ecx,(%eax)
#else
	movl %eax,errno
#endif /* __UCLIBC_HAS_THREADS__ */

#endif /* PIC */
	
	movl $-1,%eax
	ret
.Lsize:
	.size	 vfork,.Lsize-vfork
