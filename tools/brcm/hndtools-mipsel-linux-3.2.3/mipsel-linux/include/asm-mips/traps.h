/*
 *	include/asm-mips/traps.h
 *
 *	Trap handling definitions.
 *
 *	Copyright (C) 2002  Maciej W. Rozycki
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 */
#ifndef __ASM_MIPS_TRAPS_H
#define __ASM_MIPS_TRAPS_H

/*
 * Possible status responses for a be_board_handler backend.
 */
#define MIPS_BE_DISCARD	0		/* return with no action */
#define MIPS_BE_FIXUP	1		/* return to the fixup code */
#define MIPS_BE_FATAL	2		/* treat as an unrecoverable error */

extern int (*be_board_handler)(struct pt_regs *regs, int is_fixup);

extern void bus_error_init(void);

#endif /* __ASM_MIPS_TRAPS_H */
