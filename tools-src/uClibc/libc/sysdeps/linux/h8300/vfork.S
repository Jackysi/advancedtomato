
#include <asm/unistd.h>

#ifndef __NR_vfork
#define __NR_vfork __NR_fork /* uClinux-2.0 only has fork which is vfork */
#endif

	.h8300h
	.text
	.align 2
	.globl _errno
	.globl _vfork
#if defined __HAVE_ELF__
	.type	 vfork,@function
#endif
_vfork:
	mov.l	@sp+, er1
	sub.l	er0,er0
	mov.b	#__NR_vfork,r0l
	trapa	#0
	mov.l	#-4096, er2
	cmp.l 	er0,er2
	bcs     fix_errno
	jmp	@er1                     /* don't return,  just jmp directly */
fix_errno:
	neg.l	er0
	mov.l	er0,@_errno
	sub.l	er0,er0
	dec.l	#1,er0
	jmp	@er1                     /* don't return,  just jmp directly */

