/*
 * Copyright (C) 2000, 2001 Broadcom Corporation
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/kernel.h>
#include <linux/reboot.h>
#include <linux/string.h>

#include <asm/bootinfo.h>
#include <asm/mipsregs.h>
#include <asm/io.h>
#include <asm/sibyte/sb1250.h>
#include <asm/sibyte/sb1250_regs.h>
#include <asm/sibyte/sb1250_scd.h>
#include <asm/sibyte/64bit.h>

extern void prom_printf(char *fmt, ...);

/* Setup code likely to be common to all BCM1250 platforms */

static inline const char *soc_type_string(unsigned int soc_type)
{
	switch (soc_type) {
	case K_SYS_SOC_TYPE_BCM1250:
		return "BCM1250";
	case K_SYS_SOC_TYPE_BCM1120:
		return "BCM1120";
	case K_SYS_SOC_TYPE_BCM1125:
		return "BCM1125";
	case K_SYS_SOC_TYPE_BCM1125H:
		return "BCM1125H";
	default:
		return "unknown SOC";
	}
}

static inline const char *soc_pass_string(unsigned int soc_type, unsigned int soc_pass)
{
	switch (soc_type) {
	case K_SYS_SOC_TYPE_BCM1250:
		switch (soc_pass) {
		case K_SYS_REVISION_BCM1250_PASS1:
			return "Pass 1";
		case 11:
			return "A8/A10";
		case K_SYS_REVISION_BCM1250_PASS2_2:
			return "B1";
		default:
			if (soc_pass < K_SYS_REVISION_BCM1250_PASS2_2)
				return "pre-A8";
			else
				return "unknown rev";
		}
	case K_SYS_SOC_TYPE_BCM1120:
	case K_SYS_SOC_TYPE_BCM1125:
	case K_SYS_SOC_TYPE_BCM1125H:
		switch (soc_pass) {
		case K_SYS_REVISION_BCM112x_A1:
			return "A1";
		case K_SYS_REVISION_BCM112x_A2:
			return "A2";
		default:
			return "unknown rev";
		}
	default:
		return "";
	}
}

unsigned int sb1_pass;
unsigned int soc_pass;
unsigned int soc_type;

void sb1250_setup(void)
{
	uint64_t sys_rev;
	int bad_config = 0;
	unsigned int soc_war_pass;

	sys_rev = in64(IO_SPACE_BASE | A_SCD_SYSTEM_REVISION);
	soc_type = SYS_SOC_TYPE(sys_rev);
	soc_pass = G_SYS_REVISION(sys_rev);
	soc_war_pass = soc_pass;

	switch (soc_type) {
	case K_SYS_SOC_TYPE_BCM1250:
		/* Combine pass2 variants. */
		if ((soc_pass > K_SYS_REVISION_BCM1250_PASS1) &&
		    (soc_pass < K_SYS_REVISION_BCM1250_PASS2_2))
			soc_war_pass = K_SYS_REVISION_BCM1250_PASS2;
		break;

	case K_SYS_SOC_TYPE_BCM1120:
	case K_SYS_SOC_TYPE_BCM1125:
	case K_SYS_SOC_TYPE_BCM1125H:
		/* First silicon seems to not have the revid set */
		if (soc_pass == 0)
			soc_war_pass = K_SYS_REVISION_BCM112x_A1;
		break;
	}
	sb1_pass = read_c0_prid() & 0xff;

	/* XXXKW translate the soc_pass into "customer" terminology */
	prom_printf("SiByte %s %s (SB1 rev %d)\n",
		    soc_type_string(soc_type),
		    soc_pass_string(soc_type, soc_pass),
		    sb1_pass);
	prom_printf("Board type: %s\n", get_system_type());

	switch(soc_war_pass) {
	case K_SYS_REVISION_BCM1250_PASS1:
#ifndef CONFIG_SB1_PASS_1_WORKAROUNDS
		prom_printf("@@@@ This is a BCM1250 A0-A2 (Pass 1) board, and the kernel doesn't have the proper workarounds compiled in. @@@@\n");
		bad_config = 1;
#endif
		break;
	case K_SYS_REVISION_BCM1250_PASS2:
		/* Pass 2 - easiest as default for now - so many numbers */
#ifndef CONFIG_SB1_PASS_2_WORKAROUNDS
		prom_printf("@@@@ This is a BCM1250 A3-A10 board, and the kernel doesn't have the proper workarounds compiled in. @@@@\n");
		bad_config = 1;
#endif
#ifdef CONFIG_CPU_HAS_PREFETCH
		prom_printf("@@@@ Prefetches may be enabled in this kernel, but are buggy on this board.  @@@@\n");
		bad_config = 1;
#endif
		break;
	case K_SYS_REVISION_BCM1250_PASS2_2:
#ifndef CONFIG_SB1_PASS_2_WORKAROUNDS
		prom_printf("@@@@ This is a BCM1250 B1. board, and the kernel doesn't have the proper workarounds compiled in. @@@@\n");
		bad_config = 1;
#endif
		break;
	case K_SYS_REVISION_BCM112x_A1:
	case K_SYS_REVISION_BCM112x_A2:
		/* No workarounds yet */
		break;
	default:
		prom_printf("@@@ This is an unknown SOC pass. @@@@\n");
		bad_config = 1;
		break;
	}
	if (bad_config) {
		prom_printf("Invalid configuration for this chip.\n");
		machine_restart(NULL);
	}
}
