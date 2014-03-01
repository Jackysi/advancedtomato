/*
 * devicedbg.c
 *
 * A program to debug a driver: attaches a driver reading
 * a config file and then it opens, read static data and closes.
 * It is meant mainly for use with gdb.
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
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

#include "apc.h"

/*
 * myUPS is a structure that need to be defined in _all_ the forked processes.
 * The syncronization of data into this structure is done with the shared
 * memory area so this is made reentrant by the shm mechanics.
 */
UPSINFO myUPS;
UPSINFO *core_ups = &myUPS;        /* To understand the real need for it. */
UPSINFO *ups = &myUPS;             /* Added for clearness */

char argvalue[MAXSTRING];

/*
 * XXX --- Reminder !
 *
 * In principle I should not need to define dummy functions to link a
 * program against libapc.a and libdrivers.a
 * The fact that I need this means that we have forward references from
 * the libraries back into the main programs.
 *
 * The main programs are, by definition, non stand-alone programs,
 * that need libapc it is perfectly understandabe that to link apcupsd
 * we need libapc.a and libdrivers.a
 *
 * But it's much less understandable that to link
 *
 * main() {  dummy  }
 * 
 * against libapc.a and libdrivers.a I need to define the following dummy
 * functions.
 *
 * There is something that needs careful review in how we handle border-line
 * situations like shutdown of the daemon, cleanup of lock files, close
 * log files etc etc.
 *
 * To say the least, it's courios that to link the following simple program
 * against libapc.a and libdrivers.a I needed to define these dummies.
 *
 * To say the most, it's crazy :-)
 *
 * -RF
 */
void clear_files(void)
{                                  /* dummy */
}

void kill_power(UPSINFO *ups)
{                                  /* dummy */
}

/*********************************************************************/
/*			 Main program.					   */
/*********************************************************************/

int main(int argc, char *argv[])
{

   /*
    * Default config file. If we set a config file in startup switches, it
    * will be re-filled by parse_options()
    */
   cfgfile = APCCONF;

   init_ups_struct(ups);

   /* parse_options is self messaging on errors, so we need only to exit() */
   if (parse_options(argc, argv))
      exit(1);

   check_for_config(&myUPS, cfgfile);

   attach_driver(ups);

   if (ups->driver == NULL)
      Error_abort0("Apcupsd cannot continue without a valid driver.\n");

   printf("Attached to driver: %s\n", ups->driver->driver_name);

   device_open(ups);

   device_get_capabilities(ups);

   device_read_static_data(ups);

   device_read_volatile_data(ups);

   output_status(ups, 0, stat_open, stat_print, stat_close);

   device_close(ups);

   exit(0);
}
