
/*
 * linux/arch/arm/mach-sa1100/frodo.c
 *
 * Author: Abraham van der Merwe <abraham@2d3d.co.za>
 *
 * This file contains the 2d3D, Inc. SA-1110 Development Board tweaks.
 *
 * This source code is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * History:
 *
 *   2002/01/31   Initial version
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/tty.h>

#include <asm/hardware.h>
#include <asm/setup.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/serial_sa1100.h>

#include "generic.h"

static struct map_desc frodo_io_desc[] __initdata =
{
  /* virtual     physical    length      domain     r  w  c  b */
   { 0xe8000000, 0x00000000, 0x04000000, DOMAIN_IO, 1, 1, 0, 0 },	/* flash memory */
   { 0xf0000000, 0x40000000, 0x00100000, DOMAIN_IO, 1, 1, 0, 0 },	/* 16-bit on-board devices (including CPLDs) */
   { 0xf1000000, 0x18000000, 0x04000000, DOMAIN_IO, 1, 1, 0, 0 },	/* 32-bit daughter card */
   LAST_DESC
};

static void __init frodo_map_io (void)
{
   sa1100_map_io ();
   iotable_init (frodo_io_desc);

   sa1100_register_uart (0,2);	/* UART2 (serial console) */
   sa1100_register_uart (1,1);	/* UART1 (big kahuna flow control serial port) */

   /*
	* Set SUS bit in SDCR0 so serial port 1 acts as a UART.
	* See Intel SA-1110 Developers Manual Section 11.9.2.1 (GPCLK/UART Select)
	*/
   Ser1SDCR0 |= SDCR0_SUS;
}

MACHINE_START (FRODO,"2d3D, Inc. SA-1110 Development Board")
	BOOT_MEM (0xc0000000,0x80000000,0xf8000000)
	BOOT_PARAMS (0xc0000100)
	MAPIO (frodo_map_io)
	INITIRQ (sa1100_init_irq)
MACHINE_END

