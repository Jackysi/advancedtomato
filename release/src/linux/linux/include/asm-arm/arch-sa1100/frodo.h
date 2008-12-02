#ifndef _INCLUDE_FRODO_H_
#define _INCLUDE_FRODO_H_

/*
 * linux/include/asm-arm/arch-sa1100/frodo.h
 *
 * Author: Abraham van der Merwe <abraham@2d3d.co.za>
 *
 * This file contains the hardware specific definitions for 2d3D, Inc.
 * SA-1110 Development Board.
 *
 * Only include this file from SA1100-specific files.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * History:
 *
 *   2002/02/28   Ethernet (cs89x0) support
 *
 *   2002/02/27   IDE support
 *
 *   2002/02/22   Added some CPLD registers to control backlight and
 *                general purpose LEDs
 *
 *   2002/01/31   Initial version
 */

/* CPLD registers */
#define FRODO_CPLD_UART				*((u16 *) 0xf000c000)
#define FRODO_CPLD_IDE				*((u16 *) 0xf0008000)
#define FRODO_CPLD_GENERAL			*((u16 *) 0xf0004004)
#define FRODO_CPLD_PCMCIA_STATUS	*((u16 *) 0xf0004000)
#define FRODO_CPLD_PCMCIA_COMMAND	*((u16 *) 0xf0000000)
#define FRODO_CPLD_SCRATCHPAD		*((u16 *) 0xf0000004)

/* general command/status register */
#define FRODO_LCD_BACKLIGHT			0x0400		/* R/W */
#define FRODO_LED1					0x0100		/* R/W */
#define FRODO_LED2					0x0200		/* R/W */
#define FRODO_PUSHBUTTON			0x8000		/* R/O */

/* IDE related definitions */
#define FRODO_IDE_GPIO			GPIO_GPIO23
#define FRODO_IDE_IRQ			IRQ_GPIO23
#define FRODO_IDE_DATA			0xf0020000
#define FRODO_IDE_CTRL			0xf0038004

/* Ethernet related definitions */
#define FRODO_ETH_GPIO			GPIO_GPIO20
#define FRODO_ETH_IRQ			IRQ_GPIO20
#define FRODO_ETH_MEMORY		0xf0060000
#define FRODO_ETH_IO			0xf0070000

#endif	/* _INCLUDE_FRODO_H_ */
