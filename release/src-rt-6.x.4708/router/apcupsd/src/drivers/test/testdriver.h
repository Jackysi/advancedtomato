/*
 * testdriver.h
 *
 * Public header file for the test driver.
 */

/*
 * Copyright (C) 2001-2006 Kern Sibbald
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

#ifndef _TESTDRIVER_H
#define _TESTDRIVER_H

/*********************************************************************/
/* Function ProtoTypes                                               */
/*********************************************************************/

extern int test_ups_get_capabilities(UPSINFO *ups);
extern int test_ups_read_volatile_data(UPSINFO *ups);
extern int test_ups_read_static_data(UPSINFO *ups);
extern int test_ups_kill_power(UPSINFO *ups);
extern int test_ups_check_state(UPSINFO *ups);
extern int test_ups_open(UPSINFO *ups);
extern int test_ups_close(UPSINFO *ups);
extern int test_ups_setup(UPSINFO *ups);
extern int test_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int test_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _TEST_DRIVER_H */
