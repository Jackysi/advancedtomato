/*
 * apcsmart.h
 *
 * Public header file for the APC Smart protocol driver
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

#ifndef _APCSMART_H
#define _APCSMART_H

/* Private data structure */
typedef struct s_smart_data {
   struct termios oldtio;
   struct termios newtio;
} SMART_DATA;


/*
 * Function Prototypes
 */
 
extern int apcsmart_ups_get_capabilities(UPSINFO *ups);
extern int apcsmart_ups_read_volatile_data(UPSINFO *ups);
extern int apcsmart_ups_read_static_data(UPSINFO *ups);
extern int apcsmart_ups_kill_power(UPSINFO *ups);
extern int apcsmart_ups_shutdown(UPSINFO *ups);
extern int apcsmart_ups_check_state(UPSINFO *ups);
extern int apcsmart_ups_open(UPSINFO *ups);
extern int apcsmart_ups_close(UPSINFO *ups);
extern int apcsmart_ups_setup(UPSINFO *ups);
extern int apcsmart_ups_program_eeprom(UPSINFO *ups, int command, const char *data);
extern int apcsmart_ups_entry_point(UPSINFO *ups, int command, void *data);

extern int apcsmart_ups_shutdown_with_delay(UPSINFO *ups, int shutdown_delay);
extern int apcsmart_ups_get_shutdown_delay(UPSINFO *ups);
extern void apcsmart_ups_warn_shutdown(UPSINFO *ups, int shutdown_delay);

#endif   /* _APCSMART_H */
