/*
 *  linux/arch/arm/mach-mx1ads/cpu.c
 *
 *  Copyright (C) 2001 Deep Blue Solutions Ltd.
 *
 *  $Id: cpu.c,v 1.1.1.4 2003/10/14 08:07:15 sparq Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * CPU support functions
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/cpufreq.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>


static int __init cpu_init(void)
{
	return 0;
}

__initcall(cpu_init);
