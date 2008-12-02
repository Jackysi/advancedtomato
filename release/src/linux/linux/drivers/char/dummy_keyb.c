/*
 * linux/drivers/char/dummy_keyb.c
 *
 * Allows CONFIG_VT on hardware without keyboards.
 *
 * Copyright (C) 1999, 2001 Bradley D. LaRonde
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * What is this for?
 *
 * Not all systems have keyboards.  Some don't even have a keyboard
 * port.  However, some of those systems have video support and can
 * use the virtual terminal support for display.  However, the virtual
 * terminal code expects a keyboard of some kind.  This driver keeps
 * the virtual terminal code happy by providing it a "keyboard", albeit
 * a very quiet one.
 *
 * If you want to use the virtual terminal support but your system
 * does not support a keyboard, define CONFIG_DUMMY_KEYB along with
 * CONFIG_VT.
 *
 */
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/init.h>

void kbd_leds(unsigned char leds)
{
}

int kbd_setkeycode(unsigned int scancode, unsigned int keycode)
{
	return (scancode == keycode) ? 0 : -EINVAL;
}

int kbd_getkeycode(unsigned int scancode)
{
	return scancode;
}

int kbd_translate(unsigned char scancode, unsigned char *keycode,
	char raw_mode)
{
	*keycode = scancode;

	return 1;
}

char kbd_unexpected_up(unsigned char keycode)
{
	return 0x80;
}

void __init kbd_init_hw(void)
{
	printk("Dummy keyboard driver installed.\n");
}
