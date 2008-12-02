/*
 * This code fix the stack pointer so that the dunamic linker
 * can find argc, argv and auxvt (Auxillary Vector Table).
 */
asm("\
	.text
	.globl _dl_boot
	.type _dl_boot,@function
_dl_boot:
	move.d $sp,$r10
	move.d $pc,$r9
	add.d _dl_boot2 - ., $r9
	jsr $r9
");

#define _dl_boot _dl_boot2
#define LD_BOOT(X) static void __attribute__ ((unused)) _dl_boot(X)
