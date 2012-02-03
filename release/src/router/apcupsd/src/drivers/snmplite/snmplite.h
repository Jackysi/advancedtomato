/*
 * snmplite.h
 *
 * Public header for the SNMP Lite UPS driver
 */

/*
 * Copyright (C) 2009 Adam Kropelin
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

#ifndef _SNMPLITE_H
#define _SNMPLITE_H

/*********************************************************************/
/* Public Function ProtoTypes                                        */
/*********************************************************************/

extern int snmplite_ups_get_capabilities(UPSINFO *ups);
extern int snmplite_ups_read_volatile_data(UPSINFO *ups);
extern int snmplite_ups_read_static_data(UPSINFO *ups);
extern int snmplite_ups_kill_power(UPSINFO *ups);
extern int snmplite_ups_shutdown(UPSINFO *ups);
extern int snmplite_ups_check_state(UPSINFO *ups);
extern int snmplite_ups_open(UPSINFO *ups);
extern int snmplite_ups_close(UPSINFO *ups);
extern int snmplite_ups_setup(UPSINFO *ups);
extern int snmplite_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int snmplite_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _SNMPLITE_H */
