/*
 * apceeprom.c
 *
 * Do APC EEPROM changes.
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
#include "apcsmart.h"

static void change_ups_battery_date(UPSINFO *ups, const char *newdate);
static void change_ups_name(UPSINFO *ups, const char *newname);
static void change_extended(UPSINFO *ups);
static int change_ups_eeprom_item(UPSINFO *ups, const char *title, const char cmd,
   const char *setting);


/*********************************************************************/
int apcsmart_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
   char setting[20];

   setup_device(ups);
   apcsmart_ups_get_capabilities(ups);

   switch (command) {
   case CI_BATTDAT:               /* change battery date */
      if (ups->UPS_Cap[CI_BATTDAT]) {
         printf("Attempting to update UPS battery date ...\n");
         change_ups_battery_date(ups, data);
      } else {
         printf("UPS battery date configuration not supported by this UPS.\n");
         return 0;
      }
      break;

   case CI_IDEN:
      if (ups->UPS_Cap[CI_IDEN]) {
         printf("Attempting to rename UPS ...\n");
         change_ups_name(ups, data);
      } else {
         printf("UPS name configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* SENSITIVITY */
   case CI_SENS:
      if (ups->UPS_Cap[CI_SENS]) {
         asnprintf(setting, sizeof(setting), "%.1s", data);
         change_ups_eeprom_item(ups, "sensitivity", ups->UPS_Cmd[CI_SENS], setting);
      } else {
         printf("UPS sensitivity configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* ALARM_STATUS */
   case CI_DALARM:
      if (ups->UPS_Cap[CI_DALARM]) {
         asnprintf(setting, sizeof(setting), "%.1s", data);
         change_ups_eeprom_item(ups, "alarm status", ups->UPS_Cmd[CI_DALARM],
            setting);
      } else {
         printf("UPS alarm status configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* LOWBATT_SHUTDOWN_LEVEL */
   case CI_DLBATT:
      if (ups->UPS_Cap[CI_DLBATT]) {
         asnprintf(setting, sizeof(setting), "%02d", (int)atoi(data));
         change_ups_eeprom_item(ups, "low battery warning delay",
            ups->UPS_Cmd[CI_DLBATT], setting);
      } else {
         printf(
            "UPS low battery warning configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* WAKEUP_DELAY */
   case CI_DWAKE:
      if (ups->UPS_Cap[CI_DWAKE]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item(ups, "wakeup delay", ups->UPS_Cmd[CI_DWAKE],
            setting);
      } else {
         printf("UPS wakeup delay configuration not supported by this UPS.\n");
         return 0;
      }
      break;


      /* SLEEP_DELAY */
   case CI_DSHUTD:
      if (ups->UPS_Cap[CI_DSHUTD]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item(ups, "shutdown delay", ups->UPS_Cmd[CI_DSHUTD],
            setting);
      } else {
         printf("UPS shutdown delay configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* LOW_TRANSFER_LEVEL */
   case CI_LTRANS:
      if (ups->UPS_Cap[CI_LTRANS]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item(ups, "lower transfer voltage",
            ups->UPS_Cmd[CI_LTRANS], setting);
      } else {
         printf(
            "UPS low transfer voltage configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* HIGH_TRANSFER_LEVEL */
   case CI_HTRANS:
      if (ups->UPS_Cap[CI_HTRANS]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item(ups, "high transfer voltage",
            ups->UPS_Cmd[CI_HTRANS], setting);
      } else {
         printf(
            "UPS high transfer voltage configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* UPS_BATT_CAP_RETURN */
   case CI_RETPCT:
      if (ups->UPS_Cap[CI_RETPCT]) {
         asnprintf(setting, sizeof(setting), "%02d", (int)atoi(data));
         change_ups_eeprom_item(ups, "return threshold percent",
            ups->UPS_Cmd[CI_RETPCT], setting);
      } else {
         printf(
            "UPS return threshold configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* UPS_SELFTEST */
   case CI_STESTI:
      if (ups->UPS_Cap[CI_STESTI]) {
         asnprintf(setting, sizeof(setting), "%.3s", data);
         /* Make sure "ON" is 3 characters */
         if (setting[2] == 0) {
            setting[2] = ' ';
            setting[3] = 0;
         }
         change_ups_eeprom_item(ups, "self test interval", ups->UPS_Cmd[CI_STESTI],
            setting);
      } else {
         printf(
            "UPS self test interval configuration not supported by this UPS.\n");
         return 0;
      }
      break;

      /* OUTPUT_VOLTAGE */
   case CI_NOMOUTV:
      if (ups->UPS_Cap[CI_NOMOUTV]) {
         asnprintf(setting, sizeof(setting), "%03d", (int)atoi(data));
         change_ups_eeprom_item(ups, "output voltage on batteries",
            ups->UPS_Cmd[CI_NOMOUTV], setting);
      } else {
         printf(
            "UPS output voltage on batteries configuration not supported by this UPS.\n");
         return 0;
      }
      break;


   case -1:                       /* old style from .conf file */

      printf("Attempting to configure UPS ...\n");
      change_extended(ups);        /* set new values in UPS */

      printf("\nReading updated UPS configuration ...\n\n");
      device_read_volatile_data(ups);
      device_read_static_data(ups);

      /* Print report of status */
      output_status(ups, 0, stat_open, stat_print, stat_close);
      break;

   default:
      printf("Ignoring unknown config request command=%d\n", command);
      return 0;
      break;
   }

   return 1;
}

/*********************************************************************/
static void change_ups_name(UPSINFO *ups, const char *newname)
{
   char *n;
   char response[32];
   char name[10];
   char a = ups->UPS_Cmd[CI_CYCLE_EPROM];
   char c = ups->UPS_Cmd[CI_IDEN];
   int i;
   int j = strlen(newname);

   name[0] = '\0';

   if (j == 0) {
      fprintf(stderr, "Error, new name of zero length.\n");
      return;
   } else if (j > 8) {
      j = 8;                       /* maximum size */
   }

   strncpy(name, newname, 9);

   /* blank fill to 8 chars */
   while (j < 8) {
      name[j] = ' ';
      j++;
   }

   /* Ask for name */
   write(ups->fd, &c, 1);          /* c = 'c' */
   getline(response, sizeof(response), ups);
   fprintf(stderr, "The old UPS name is: %s\n", response);

   /* Tell UPS we will change name */
   write(ups->fd, &a, 1);          /* a = '-' */
   sleep(1);

   n = name;
   for (i = 0; i < 8; i++) {
      write(ups->fd, n++, 1);
      sleep(1);
   }

   /* Expect OK after successful name change */
   *response = 0;
   getline(response, sizeof(response), ups);
   if (strcmp(response, "OK") != 0) {
      fprintf(stderr, "\nError changing UPS name\n");
   }

   ups->upsname[0] = '\0';
   smart_poll(ups->UPS_Cmd[CI_IDEN], ups);
   astrncpy(ups->upsname, smart_poll(ups->UPS_Cmd[CI_IDEN], ups),
      sizeof(ups->upsname));

   fprintf(stderr, "The new UPS name is: %s\n", ups->upsname);
}

/*
 * Update date battery replaced
 */
static void change_ups_battery_date(UPSINFO *ups, const char *newdate)
{
   char *n;
   char response[32];
   char battdat[9];
   char a = ups->UPS_Cmd[CI_CYCLE_EPROM];
   char c = ups->UPS_Cmd[CI_BATTDAT];
   int i;
   int j = strlen(newdate);

   battdat[0] = '\0';

   if (j != 8) {
      fprintf(stderr, "Error, new battery date must be 8 characters long.\n");
      return;
   }

   astrncpy(battdat, newdate, sizeof(battdat));

   /* Ask for battdat */
   write(ups->fd, &c, 1);          /* c = 'x' */
   getline(response, sizeof(response), ups);
   fprintf(stderr, "The old UPS battery date is: %s\n", response);

   /* Tell UPS we will change battdat */
   write(ups->fd, &a, 1);          /* a = '-' */
   sleep(1);

   n = battdat;
   for (i = 0; i < 8; i++) {
      write(ups->fd, n++, 1);
      sleep(1);
   }

   /* Expect OK after successful battdat change */
   *response = 0;
   getline(response, sizeof(response), ups);
   if (strcmp(response, "OK") != 0) {
      fprintf(stderr, "\nError changing UPS battery date\n");
   }

   ups->battdat[0] = '\0';
   smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups);
   astrncpy(ups->battdat, smart_poll(ups->UPS_Cmd[CI_BATTDAT], ups),
      sizeof(ups->battdat));

   fprintf(stderr, "The new UPS battery date is: %s\n", ups->battdat);
}

/*********************************************************************/
static int change_ups_eeprom_item(UPSINFO *ups, const char *title, const char cmd,
   const char *setting)
{
   char response[32];
   char response1[32];
   char oldvalue[32];
   char lastvalue[32];
   char allvalues[256];
   char a = ups->UPS_Cmd[CI_CYCLE_EPROM];
   int i;

   /* Ask for old value */
   write(ups->fd, &cmd, 1);
   if (getline(oldvalue, sizeof(oldvalue), ups) == FAILURE) {
      fprintf(stderr, "Could not get old value of %s.\n", title);
      return FAILURE;
   }

   if (strcmp(oldvalue, setting) == 0) {
      fprintf(stderr, "The UPS %s remains unchanged as: %s\n", title, oldvalue);
      return SUCCESS;
   }

   fprintf(stderr, "The old UPS %s is: %s\n", title, oldvalue);
   astrncpy(allvalues, oldvalue, sizeof(allvalues));
   astrncat(allvalues, " ", sizeof(allvalues));
   astrncpy(lastvalue, oldvalue, sizeof(lastvalue));

   /* Try a second time to ensure that it is a stable value */
   write(ups->fd, &cmd, 1);
   *response = 0;
   getline(response, sizeof(response), ups);
   if (strcmp(oldvalue, response) != 0) {
      fprintf(stderr, "\nEEPROM value of %s is not stable\n", title);
      return FAILURE;
   }

   /*
    * Just before entering this loop, the last command sent
    * to the UPS MUST be to query the old value.   
    */
   for (i = 0; i < 10; i++) {
      write(ups->fd, &cmd, 1);
      getline(response1, sizeof(response1), ups);

      /* Tell UPS to cycle to next value */
      write(ups->fd, &a, 1);       /* a = '-' */

      /* Expect OK after successful change */
      *response = 0;
      getline(response, sizeof(response), ups);
      if (strcmp(response, "OK") != 0) {
         fprintf(stderr, "\nError changing UPS %s\n", title);
         fprintf(stderr, "Got %s instead of OK\n\n", response);
         sleep(10);
         return FAILURE;
      }

      /* get cycled value */
      write(ups->fd, &cmd, 1);
      getline(response1, sizeof(response1), ups);

      /* get cycled value again */
      write(ups->fd, &cmd, 1);
      if (getline(response, sizeof(response), ups) == FAILURE ||
         strcmp(response1, response) != 0) {
         fprintf(stderr, "Error cycling values.\n");
         getline(response, sizeof(response), ups);      /* eat any garbage */
         return FAILURE;
      }
      if (strcmp(setting, response) == 0) {
         fprintf(stderr, "The new UPS %s is: %s\n", title, response);
         sleep(10);                /* allow things to settle down */
         return SUCCESS;
      }

      /*
       * Check if we cycled back to the same value, but permit
       * a duplicate because the L for sensitivy appears
       * twice in a row, i.e. H M L L.
       */
      if (strcmp(oldvalue, response) == 0 && i > 0)
         break;
      if (strcmp(lastvalue, response) != 0) {
         astrncat(allvalues, response, sizeof(allvalues));
         astrncat(allvalues, " ", sizeof(allvalues));
         astrncpy(lastvalue, response, sizeof(lastvalue));
      }
      sleep(5);                    /* don't cycle too fast */
   }

   fprintf(stderr, "Unable to change %s to: %s\n", title, setting);
   fprintf(stderr, "Permitted values are: %s\n", allvalues);
   getline(response, sizeof(response), ups);    /* eat any garbage */

   return FAILURE;
}


/*
 * Set new values in EEPROM memmory.  Change the UPS EEPROM.
 */
static void change_extended(UPSINFO *ups)
{
   char setting[20];

   apcsmart_ups_get_capabilities(ups);

   /*
    * Note, a value of -1 in the variable at the beginning
    * means that the user did not put a configuration directive
    * in /etc/apcupsd/apcupsd.conf. Consequently, if no value
    * was given, we won't attept to change it.
    */

   /* SENSITIVITY */
   if (ups->UPS_Cap[CI_SENS] && strcmp(ups->sensitivity, "-1") != 0) {
      asnprintf(setting, sizeof(setting), "%.1s", ups->sensitivity);
      change_ups_eeprom_item(ups, "sensitivity", ups->UPS_Cmd[CI_SENS], setting);
   }

   /* WAKEUP_DELAY */
   if (ups->UPS_Cap[CI_DWAKE] && ups->dwake != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)ups->dwake);
      change_ups_eeprom_item(ups, "wakeup delay", ups->UPS_Cmd[CI_DWAKE], setting);
   }

   /* SLEEP_DELAY */
   if (ups->UPS_Cap[CI_DSHUTD] && ups->dshutd != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)ups->dshutd);
      change_ups_eeprom_item(ups, "shutdown delay", ups->UPS_Cmd[CI_DSHUTD],
         setting);
   }

   /* LOW_TRANSFER_LEVEL */
   if (ups->UPS_Cap[CI_LTRANS] && ups->lotrans != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)ups->lotrans);
      change_ups_eeprom_item(ups, "lower transfer voltage",
         ups->UPS_Cmd[CI_LTRANS], setting);
   }

   /* HIGH_TRANSFER_LEVEL */
   if (ups->UPS_Cap[CI_HTRANS] && ups->hitrans != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)ups->hitrans);
      change_ups_eeprom_item(ups, "upper transfer voltage",
         ups->UPS_Cmd[CI_HTRANS], setting);
   }

   /* UPS_BATT_CAP_RETURN */
   if (ups->UPS_Cap[CI_RETPCT] && ups->rtnpct != -1) {
      asnprintf(setting, sizeof(setting), "%02d", (int)ups->rtnpct);
      change_ups_eeprom_item(ups, "return threshold percent",
         ups->UPS_Cmd[CI_RETPCT], setting);
   }

   /* ALARM_STATUS */
   if (ups->UPS_Cap[CI_DALARM] && strcmp(ups->beepstate, "-1") != 0) {
      asnprintf(setting, sizeof(setting), "%.1s", ups->beepstate);
      change_ups_eeprom_item(ups, "alarm delay", ups->UPS_Cmd[CI_DALARM], setting);
   }

   /* LOWBATT_SHUTDOWN_LEVEL */
   if (ups->UPS_Cap[CI_DLBATT] && ups->dlowbatt != -1) {
      asnprintf(setting, sizeof(setting), "%02d", (int)ups->dlowbatt);
      change_ups_eeprom_item(ups, "low battery warning delay",
         ups->UPS_Cmd[CI_DLBATT], setting);
   }

   /* UPS_SELFTEST */
   if (ups->UPS_Cap[CI_STESTI] && strcmp(ups->selftest, "-1") != 0) {
      asnprintf(setting, sizeof(setting), "%.3s", ups->selftest);
      /* Make sure "ON" is 3 characters */
      if (setting[2] == 0) {
         setting[2] = ' ';
         setting[3] = 0;
      }
      change_ups_eeprom_item(
         ups, "self test interval", ups->UPS_Cmd[CI_STESTI], setting);
   }

   /* OUTPUT_VOLTAGE */
   if (ups->UPS_Cap[CI_NOMOUTV] && ups->NomOutputVoltage != -1) {
      asnprintf(setting, sizeof(setting), "%03d", (int)ups->NomOutputVoltage);
      change_ups_eeprom_item(ups, "output voltage on batteries",
         ups->UPS_Cmd[CI_NOMOUTV], setting);
   }
}
