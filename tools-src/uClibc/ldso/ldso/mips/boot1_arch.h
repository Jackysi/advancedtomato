/* Any assmbly language/system dependent hacks needed to setup boot1.c so it
 * will work as expected and cope with whatever platform specific wierdness is
 * needed for this architecture.
 */

asm("
	.text
	.globl _dl_boot
_dl_boot:
	.set noreorder
	bltzal $0, 0f
	nop
0:	.cpload $31
	.set reorder
	la $4, _DYNAMIC
	sw $4, -0x7ff0($28)
	move $4, $29
	la $8, coff
	.set noreorder
	bltzal $0, coff
	nop
coff:	subu $8, $31, $8
	.set reorder
	la $25, _dl_boot2
	addu $25, $8
	jalr $25
	lw $4, 0($29)
	la $5, 4($29)
	sll $6, $4, 2
	addu $6, $6, $5
	addu $6, $6, 4
	la $7, _dl_elf_main
	lw $25, 0($7)
	jr $25
");

#define _dl_boot _dl_boot2
#define LD_BOOT(X)   static void __attribute__ ((unused)) _dl_boot (X)
