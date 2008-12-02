/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2001 Keith M Wesolowski
 * Copyright (C) 2001 Paul Mundt
 */
#include <linux/init.h>

#include <asm/reboot.h>
#include <asm/sgialib.h>

static void ip32_machine_restart(char *cmd)
{
	ArcReboot();
}

static inline void ip32_machine_halt(void)
{
	ArcEnterInteractiveMode();
}

static void ip32_machine_power_off(void)
{
	ip32_machine_halt();
}

void __init ip32_reboot_setup(void)
{
	_machine_restart = ip32_machine_restart;
	_machine_halt = ip32_machine_halt;
	_machine_power_off = ip32_machine_power_off;
}
