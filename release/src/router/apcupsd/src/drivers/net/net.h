/*
 * net.h
 *
 * Public header file for the net client driver.
 */

/*
 * Copyright (C) 2000-2006 Kern Sibbald
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

#ifndef _NET_H
#define _NET_H

#define BIGBUF 4096

struct driver_data {
   char device[MAXSTRING];
   char *hostname;
   int port;
   int sockfd;
   int got_caps;
   int got_static_data;
   time_t last_fill_time;
   char statbuf[BIGBUF];
   int statlen;
};

/*********************************************************************/
/* Function ProtoTypes                                               */
/*********************************************************************/

extern int net_ups_get_capabilities(UPSINFO *ups);
extern int net_ups_read_volatile_data(UPSINFO *ups);
extern int net_ups_read_static_data(UPSINFO *ups);
extern int net_ups_kill_power(UPSINFO *ups);
extern int net_ups_check_state(UPSINFO *ups);
extern int net_ups_open(UPSINFO *ups);
extern int net_ups_close(UPSINFO *ups);
extern int net_ups_setup(UPSINFO *ups);
extern int net_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int net_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _NET_H */
