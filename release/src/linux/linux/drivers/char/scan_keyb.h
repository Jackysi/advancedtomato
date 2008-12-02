#ifndef	__DRIVER_CHAR_SCAN_KEYB_H
#define	__DRIVER_CHAR_SCAN_KEYB_H
/*
 *	$Id: scan_keyb.h,v 1.1.1.4 2003/10/14 08:08:02 sparq Exp $
 *	Copyright (C) 2000 YAEGASHI Takeshi
 *	Generic scan keyboard driver
 */

int register_scan_keyboard(int (*scan)(unsigned char *buffer),
			   const unsigned char *table,
			   int length);

void __init scan_kbd_init(void);

#endif
