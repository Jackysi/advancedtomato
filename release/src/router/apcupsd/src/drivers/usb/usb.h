/*
 * usb.h
 *
 * Public USB driver interface exposed to the driver management layer.
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 2004-2005 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _USB_H
#define _USB_H

/*********************************************************************/

/* Function ProtoTypes                                               */

/*********************************************************************/

extern int usb_ups_get_capabilities(UPSINFO *ups);
extern int usb_ups_read_volatile_data(UPSINFO *ups);
extern int usb_ups_read_static_data(UPSINFO *ups);
extern int usb_ups_kill_power(UPSINFO *ups);
extern int usb_ups_shutdown(UPSINFO *ups);
extern int usb_ups_check_state(UPSINFO *ups);
extern int usb_ups_open(UPSINFO *ups);
extern int usb_ups_close(UPSINFO *ups);
extern int usb_ups_setup(UPSINFO *ups);
extern int usb_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int usb_ups_entry_point(UPSINFO *ups, int command, void *data);

/* Extra functions exported for use by apctest */
extern int usb_write_int_to_ups(UPSINFO *ups, int ci, int value, char const* name);
extern int usb_read_int_from_ups(UPSINFO *ups, int ci, int *value);

#endif  /* _USB_H */
