/*
 *  arch/mips/galileo/compressed/head.S
 *
 */

#include <asm/asm.h>
#include <asm/regdef.h>

	.text
NESTED(startup, 16, sp)
	.set	noreorder

	jal decompress_kernel
	nop

        jal kernel_location_start+0x584
        nop

	END(startup)
