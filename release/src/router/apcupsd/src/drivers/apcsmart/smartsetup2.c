/*
 * smartsetup2.c
 *
 * UPS capability discovery for SmartUPS models.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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

/*********************************************************************/
const char *get_model_from_oldfwrev(const char *s)
{

   switch (s[0]) {
   case '0':
      return ("APC Matrix-UPS 3000");
   case '2':
      return ("APC Smart-UPS 250");
   case '3':
      return ("APC Smart-UPS 370ci");
   case '4':
      return ("APC Smart-UPS 400");
   case '5':
      return ("APC Matrix-UPS 5000");
   case '6':
      return ("APC Smart-UPS 600");
   case '7':
      return ("APC Smart-UPS 900");
   case '8':
      return ("APC Smart-UPS 1250");
   case '9':
      return ("APC Smart-UPS 2000");
   case 'A':
      return ("APC Smart-UPS 1400");
   case 'B':
      return ("APC Smart-UPS 1000");
   case 'C':
      return ("APC Smart-UPS 650");
   case 'D':
      return ("APC Smart-UPS 420");
   case 'E':
      return ("APC Smart-UPS 280");
   case 'F':
      return ("APC Smart-UPS 450");
   case 'G':
      return ("APC Smart-UPS 700");
   case 'H':
      return ("APC Smart-UPS 700 XL");
   case 'I':
      return ("APC Smart-UPS 1000");
   case 'J':
      return ("APC Smart-UPS 1000 XL");
   case 'K':
      return ("APC Smart-UPS 1400");
   case 'L':
      return ("APC Smart-UPS 1400 XL");
   case 'M':
      return ("APC Smart-UPS 2200");
   case 'N':
      return ("APC Smart-UPS 2200 XL");
   case 'O':
      return ("APC Smart-UPS 5000");
   case 'P':
      return ("APC Smart-UPS 3000");
   }

   return "Unknown";
}

/*
 * This subroutine polls the APC Smart UPS to find out 
 * what capabilities it has.          
 */
int apcsmart_ups_get_capabilities(UPSINFO *ups)
{
   char answer[1000];              /* keep this big to handle big string */
   char caps[1000], *cmds, *p;
   int i;

   write_lock(ups);

   /* Get UPS capabilities string */
   astrncpy(caps, smart_poll(ups->UPS_Cmd[CI_UPS_CAPS], ups), sizeof(caps));
   if (strlen(caps) && (strcmp(caps, "NA") != 0)) {
      ups->UPS_Cap[CI_UPS_CAPS] = TRUE;

      /* skip version */
      for (p = caps; *p && *p != '.'; p++)
         ;

      /* skip alert characters */
      for (; *p && *p != '.'; p++)
         ;

      cmds = p;                    /* point to commands */
      if (!*cmds) {                /* oops, none */
         cmds = NULL;
         ups->UPS_Cap[CI_UPS_CAPS] = FALSE;
      }
   } else {
      cmds = NULL;                 /* No commands string */
   }

   /*
    * Try all the possible UPS capabilities and mark the ones supported.
    * If we can get the eprom caps list, use them to jump over the
    * unsupported caps, if we can't get the cap list try every known
    * capability.
    */
   for (i = 0; i <= CI_MAX_CAPS; i++) {
      if (ups->UPS_Cmd[i] == 0)
         continue;
      if (!cmds || strchr(cmds, ups->UPS_Cmd[i]) != NULL) {
         astrncpy(answer, smart_poll(ups->UPS_Cmd[i], ups), sizeof(answer));
         if (*answer && (strcmp(answer, "NA") != 0)) {
            ups->UPS_Cap[i] = true;
         }
      }
   }

   /*
    * If UPS did not support APC_CMD_UPSMODEL (the default comand for 
    * CI_UPSMODEL), maybe it supports APC_CMD_OLDFWREV which can be used to 
    * construct the model number.
    */
   if (!ups->UPS_Cap[CI_UPSMODEL] && 
       (!cmds || strchr(cmds, APC_CMD_OLDFWREV) != NULL)) {
      astrncpy(answer, smart_poll(APC_CMD_OLDFWREV, ups), sizeof(answer));
      if (*answer && (strcmp(answer, "NA") != 0)) {
         ups->UPS_Cap[CI_UPSMODEL] = true;
         ups->UPS_Cmd[CI_UPSMODEL] = APC_CMD_OLDFWREV;
      }
   }

   /*
    * If UPS does not support APC_CMD_REVNO (the default command for CI_REVNO),
    * maybe it supports APC_CMD_OLDFWREV instead.
    */
   if (!ups->UPS_Cap[CI_REVNO] && 
       (!cmds || strchr(cmds, APC_CMD_OLDFWREV) != NULL)) {
      astrncpy(answer, smart_poll(APC_CMD_OLDFWREV, ups), sizeof(answer));
      if (*answer && (strcmp(answer, "NA") != 0)) {
         ups->UPS_Cap[CI_REVNO] = true;
         ups->UPS_Cmd[CI_REVNO] = APC_CMD_OLDFWREV;
      }
   }

   write_unlock(ups);

   return 1;
}
