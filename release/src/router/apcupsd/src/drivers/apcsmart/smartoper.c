/*
 * smartoper.c
 *
 * Functions for SmartUPS operations
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
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
#include "apcsmart.h"

int apcsmart_ups_kill_power(UPSINFO *ups)
{
   char response[32] = {0};
   int shutdown_delay = apcsmart_ups_get_shutdown_delay(ups);

   // Ask for soft shutdown
   writechar('S', ups);

   // Check whether the UPS has acknowledged the power-off command.
   // The command only succeeds if UPS was on battery.
   sleep(5);
   getline(response, sizeof response, ups);
   if (strcmp(response, "OK") == 0 || (strcmp(response, "*") == 0))
      goto acked;

   // 'S' command failed, most likely because utility power was restored.
   // We still need to power cycle the UPS however since the OS has been
   // shut down already. We will issue the shutdown-and-return command
   // '@000' which works even if UPS is online.

   // Experiments show that the UPS needs delays between chars
   // to accept this command. Old code is written to send 2 zeros
   // first, check for a response, and then send a third zero if
   // command needs it. I've never seen an UPS that needs only 2 zeros
   // but apparently someone did, so code is preserved.

   writechar('@', ups);    /* Shutdown now, try two 0s first */
   sleep(2);
   writechar('0', ups);
   sleep(2);
   writechar('0', ups);
   sleep(2);

   getline(response, sizeof(response), ups);
   if ((strcmp(response, "OK") == 0) || (strcmp(response, "*") == 0))
      goto acked;

   writechar('0', ups);
   sleep(2);
 
   getline(response, sizeof(response), ups);
   if ((strcmp(response, "OK") == 0) || (strcmp(response, "*") == 0))
      goto acked;

   // Both shutdown techniques failed. We have one more we can try, but
   // UPS will power off and stay off in this case.
   return apcsmart_ups_shutdown_with_delay(ups, shutdown_delay);

acked:
   // Shutdown command was accepted
   apcsmart_ups_warn_shutdown(ups, shutdown_delay);
   return 1;
}

int apcsmart_ups_shutdown(UPSINFO *ups)
{
   return apcsmart_ups_shutdown_with_delay(ups, apcsmart_ups_get_shutdown_delay(ups));
}

int apcsmart_ups_shutdown_with_delay(UPSINFO *ups, int shutdown_delay)
{
   char response[32];

   /*
    * K K command
    *
    * This method should turn the UPS off completely according to this article:
    * http://nam-en.apc.com/cgi-bin/nam_en.cfg/php/enduser/std_adp.php?p_faqid=604
    */

   writechar('K', ups);
   sleep(2);
   writechar('K', ups);
   getline(response, sizeof response, ups);
   if (strcmp(response, "*") != 0 && strcmp(response, "OK") != 0) {
      log_event(ups, LOG_WARNING, "Failed to issue shutdown command!\n");
      return 0;
   }

   apcsmart_ups_warn_shutdown(ups, shutdown_delay);
   return 1;
}

void apcsmart_ups_warn_shutdown(UPSINFO *ups, int shutdown_delay)
{
   if (shutdown_delay > 0) {
      log_event(ups, LOG_WARNING,
         "UPS will power off after %d seconds ...\n", shutdown_delay);
   } else {
      log_event(ups, LOG_WARNING,
         "UPS will power off after the configured delay  ...\n");
   }
   log_event(ups, LOG_WARNING,
      "Please power off your UPS before rebooting your computer ...\n");
}

int apcsmart_ups_get_shutdown_delay(UPSINFO *ups)
{
   char response[32];

   writechar(ups->UPS_Cmd[CI_DSHUTD], ups);
   getline(response, sizeof(response), ups);
   return (int)atof(response);
}

int apcsmart_ups_check_state(UPSINFO *ups)
{
   return getline(NULL, 0, ups) == SUCCESS ? 1 : 0;
}
