/*
 *  arch/s390x/lib/uaccess.S
 *    fixup routines for copy_{from|to}_user functions.
 *
 *  S390
 *    Copyright (C) 2000 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Authors(s): Martin Schwidefsky (schwidefsky@de.ibm.com)
 *
 *  These functions have standard call interface
 */

#include <asm/lowcore.h>

        .text
        .align 4
        .globl __copy_from_user_asm
__copy_from_user_asm:
	lgr	%r5,%r3
	sacf	512
0:	mvcle	%r2,%r4,0
	jo	0b
	sacf	0
	lgr	%r2,%r5
	br	%r14
1:	lg	%r1,__LC_PGM_OLD_PSW+8
2:	lghi	%r3,-4096
        ng	%r3,__LC_TRANS_EXC_ADDR
        sgr	%r3,%r4
        bm	4(%r1)
3:      mvcle	%r2,%r4,0
        b	4(%r1)
        .section __ex_table,"a"
        .align	8
	.quad	0b,1b
	.quad	3b,2b
        .previous

        .align 4
        .text
        .globl __copy_to_user_asm
__copy_to_user_asm:
	lgr	%r5,%r3
	sacf	512
0:	mvcle	%r4,%r2,0
	jo	0b
	sacf	0
	lgr	%r2,%r3
	br	%r14
1:	lg	%r1,__LC_PGM_OLD_PSW+8
2:      lghi	%r5,-4096
        ng	%r5,__LC_TRANS_EXC_ADDR
        sgr	%r5,%r4
        bm	4(%r1)
3:      mvcle	%r4,%r2,0
        b	4(%r1)
        .section __ex_table,"a"
        .align	8
        .quad	0b,1b
        .quad	3b,2b
        .previous

        .align 4
        .text
        .globl __clear_user_asm
__clear_user_asm:
	lgr	%r4,%r2
	lgr	%r5,%r3
	sgr	%r2,%r2
	sgr	%r3,%r3
	sacf	512
0:	mvcle	%r4,%r2,0
	jo	0b
	sacf	0
	lgr	%r2,%r5
	br	%r14
1:	lg	%r1,__LC_PGM_OLD_PSW+8
2:      lghi	%r5,-4096
        ng	%r5,__LC_TRANS_EXC_ADDR
        sgr	%r5,%r4
        bm	4(%r1)
3:      mvcle	%r4,%r2,0
        b	4(%r1)
        .section __ex_table,"a"
        .align	8
        .quad	0b,1b
        .quad	3b,2b
        .previous
