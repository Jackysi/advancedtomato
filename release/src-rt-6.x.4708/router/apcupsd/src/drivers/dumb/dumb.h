/*
 * dumb.h
 *
 * Public header file for the simple-signalling (aka "dumb") driver
 */

/*
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

#ifndef _DUMB_H
#define _DUMB_H

/* Private dumb driver data structure */

typedef struct s_dumb_data {
   int sp_flags;                   /* Serial port flags */
   time_t debounce;                /* last event time for debounce */
   struct termios oldtio;
   struct termios newtio;
} SIMPLE_DATA;

/*********************************************************************/
/* Function ProtoTypes                                               */
/*********************************************************************/

extern int dumb_ups_get_capabilities(UPSINFO *ups);
extern int dumb_ups_read_volatile_data(UPSINFO *ups);
extern int dumb_ups_read_static_data(UPSINFO *ups);
extern int dumb_ups_kill_power(UPSINFO *ups);
extern int dumb_ups_check_state(UPSINFO *ups);
extern int dumb_ups_open(UPSINFO *ups);
extern int dumb_ups_close(UPSINFO *ups);
extern int dumb_ups_setup(UPSINFO *ups);
extern int dumb_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int dumb_ups_entry_point(UPSINFO *ups, int command, void *data);

#endif   /* _DUMB_H */
