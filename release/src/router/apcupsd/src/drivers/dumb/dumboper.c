/*
 * dumboper.c
 *
 * Functions for simple-signalling (dumb) UPS operations
 */

/*
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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
#include "dumb.h"

int dumb_ups_kill_power(UPSINFO *ups)
{
   int serial_bits = 0;

   switch (ups->cable.type) {
   case CUSTOM_SIMPLE:            /* killpwr_bit */
   case APC_940_0095A:
   case APC_940_0095B:
   case APC_940_0095C:            /* killpwr_bit */
      serial_bits = TIOCM_RTS;
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      sleep(10);                  /* hold for 10 seconds */
      serial_bits = TIOCM_ST;
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      break;

   case APC_940_0119A:
   case APC_940_0127A:
   case APC_940_0128A:
   case APC_940_0020B:            /* killpwr_bit */
   case APC_940_0020C:
      serial_bits = TIOCM_DTR;
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      sleep(10);                   /* hold for at least 10 seconds */
      break;

   case APC_940_0023A:            /* killpwr_bit */
      break;

   case MAM_CABLE:
      serial_bits = TIOCM_RTS;
      (void)ioctl(ups->fd, TIOCMBIC, &serial_bits);
      serial_bits = TIOCM_DTR;
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      sleep(10);                   /* hold */
      break;
   }
   return 1;
}

/*
 * Dumb UPSes don't have static UPS data.
 */
int dumb_ups_read_static_data(UPSINFO *ups)
{
   return 1;
}

/*
 * Set capabilities.
 */
int dumb_ups_get_capabilities(UPSINFO *ups)
{
   /* We create a Status word */
   ups->UPS_Cap[CI_STATUS] = TRUE;

   return 1;
}

/*
 * dumb_ups_check_state is the same as dumb_read_ups_volatile_data
 * because this is the only info we can get from UPS.
 *
 * This routine is polled. We should sleep at least 5 seconds
 * unless we are in a FastPoll situation, otherwise, we burn
 * too much CPU.
 */
int dumb_ups_read_volatile_data(UPSINFO *ups)
{
   SIMPLE_DATA *my_data = (SIMPLE_DATA *) ups->driver_internal_data;
   int stat = 1;

   /*
    * We generally poll a bit faster because we do 
    * not have interrupts like the smarter devices
    */
   if (ups->wait_time > TIMER_DUMB)
      ups->wait_time = TIMER_DUMB;

   sleep(ups->wait_time);

   write_lock(ups);

   ioctl(ups->fd, TIOCMGET, &my_data->sp_flags);

   switch (ups->cable.type) {
   case CUSTOM_SIMPLE:
      /*
       * This is the ONBATT signal sent by UPS.
       */
      if (my_data->sp_flags & TIOCM_CD) {
         ups->clear_online();
      } else {
         ups->set_online();
      }

      if (!(my_data->sp_flags & TIOCM_CTS)) {
         ups->set_battlow();
      } else {
         ups->clear_battlow();
      }

      break;

   case APC_940_0119A:
   case APC_940_0127A:
   case APC_940_0128A:
   case APC_940_0020B:
   case APC_940_0020C:
      if (my_data->sp_flags & TIOCM_CTS) {
         ups->clear_online();
      } else {
         ups->set_online();
      }

      if (my_data->sp_flags & TIOCM_CD) {
         ups->set_battlow();
      } else {
         ups->clear_battlow();
      }

      break;

   case APC_940_0023A:
      if (my_data->sp_flags & TIOCM_CD) {
         ups->clear_online();
      } else {
         ups->set_online();
      }

/*
 * Code block preserved for posterity in case I ever get a real
 * 940-0023A cable to test. According to schematic in the apcupsd
 * manual, SR is not connected at all. We used to treat it as a
 * battlow indicator, but I have no evidence that it works, and
 * some evidence that it does not.

      if (my_data->sp_flags & TIOCM_SR) {
         ups->set_battlow();
      } else {
         ups->clear_battlow();
      }
*/
      break;

   case APC_940_0095A:
   case APC_940_0095C:
      if (my_data->sp_flags & TIOCM_RNG) {
         ups->clear_online();
      } else {
         ups->set_online();
      }

      if (my_data->sp_flags & TIOCM_CD) {
         ups->set_battlow();
      } else {
         ups->clear_battlow();
      }

      break;

   case APC_940_0095B:
      if (my_data->sp_flags & TIOCM_RNG) {
         ups->clear_online();
      } else {
         ups->set_online();
      }

      break;

   case MAM_CABLE:
      if (!(my_data->sp_flags & TIOCM_CTS)) {
         ups->clear_online();
      } else {
         ups->set_online();
      }

      if (!(my_data->sp_flags & TIOCM_CD)) {
         ups->set_battlow();
      } else {
         ups->clear_battlow();
      }

      break;
   }

   ups->clear_replacebatt();

   write_unlock(ups);

   return stat;
}

int dumb_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
#if 0
   printf("This model cannot be configured.\n");
#endif
   return 0;
}

int dumb_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   int serial_bits = 0;

   switch (command) {
   case DEVICE_CMD_DTR_ENABLE:
      if (ups->cable.type == CUSTOM_SIMPLE) {
         /* 
          * A power failure just happened.
          *
          * Now enable the DTR for the CUSTOM_SIMPLE cable
          * Note: this enables the the CTS bit, which allows
          * us to detect the UPS_battlow condition!!!!
          */
         serial_bits = TIOCM_DTR;
         (void)ioctl(ups->fd, TIOCMBIS, &serial_bits);
      }
      break;

   case DEVICE_CMD_DTR_ST_DISABLE:
      if (ups->cable.type == CUSTOM_SIMPLE) {
         /* 
          * Mains power just returned.
          *
          * Clearing DTR and TxD (ST bit).
          */
         serial_bits = TIOCM_DTR | TIOCM_ST;

/* Leave it set */

/*              (void)ioctl(ups->fd, TIOCMBIC, &serial_bits);
 */
      }
      break;
   default:
      return 0;
      break;
   }
   return 1;
}
