/*
 * snmp.c
 *
 * SNMP UPS driver
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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
#include "snmp.h"
#include "snmp_private.h"

static int initialize_device_data(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   char *port_num = NULL;
   char *cp;

   if (ups->device == NULL || *ups->device == '\0') {
      log_event(ups, LOG_ERR, "Wrong device for SNMP driver.");
      exit(1);
   }

   astrncpy(Sid->device, ups->device, sizeof(Sid->device));

   /*
    * Split the DEVICE statement and assign pointers to the various parts.
    * The DEVICE statement syntax in apcupsd.conf is:
    *
    *    DEVICE address:port:vendor:community
    *
    * vendor can be "APC" or "RFC".
    */

   Sid->peername = Sid->device;

   cp = strchr(Sid->device, ':');
   if (cp == NULL) {
      log_event(ups, LOG_ERR, "Wrong port for SNMP driver.");
      exit(1);
   }

   /*
    * Note that we purposely keep :port as part of peername. Newer
    * versions of Net-SNMP appear to ignore sess->remote_port and you
    * can only specify a port number by including it in the peername.
    * Older versions also appear to be able to cope with :port
    * appended to peername.
    */

   port_num = cp + 1;
   cp = strchr(port_num, ':');
   if (cp == NULL) {
      log_event(ups, LOG_ERR, "Wrong vendor for SNMP driver.");
      exit(1);
   }
   *cp = '\0';
   Sid->remote_port = atoi(port_num);

   Sid->DeviceVendor = cp + 1;

   cp = strchr(Sid->DeviceVendor, ':');
   if (cp == NULL) {
      log_event(ups, LOG_ERR, "Wrong community for SNMP driver.");
      exit(1);
   }
   *cp = '\0';

   /*
    * Convert DeviceVendor to upper case.
    * Reuse cp and in the end of while() it will point to the end
    * of Sid->DeviceVendor in anyway.
    */
   cp = Sid->DeviceVendor;
   while (*cp != '\0') {
      *cp = toupper(*cp);
      cp++;
   }

   Sid->community = cp + 1;

   return 1;
}

int snmp_ups_open(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid;

   /* Allocate the internal data structure and link to UPSINFO. */
   Sid = (struct snmp_ups_internal_data *)
      malloc(sizeof(struct snmp_ups_internal_data));
   if (Sid == NULL) {
      log_event(ups, LOG_ERR, "Out of memory.");
      exit(1);
   }
   memset(Sid, 0, sizeof(struct snmp_ups_internal_data));

   write_lock(ups);

   ups->driver_internal_data = Sid;
   initialize_device_data(ups);

   write_unlock(ups);

   memset(&Sid->session, 0, sizeof(struct snmp_session));
   Sid->session.peername = Sid->peername;
   Sid->session.remote_port = Sid->remote_port;

   /*
    * We will use Version 1 of SNMP protocol as it is the most widely
    * used.
    */
   Sid->session.version = SNMP_VERSION_1;
   Sid->session.community = (u_char *)Sid->community;
   Sid->session.community_len = strlen((const char *)Sid->session.community);

   /* Set a maximum of 5 retries before giving up. */
   Sid->session.retries = 5;
   Sid->session.timeout = SNMP_DEFAULT_TIMEOUT;
   Sid->session.authenticator = NULL;

   if (!strcmp(Sid->DeviceVendor, "APC") ||
       !strcmp(Sid->DeviceVendor, "APC_NOTRAP")) {
      Sid->MIB = malloc(sizeof(powernet_mib_t));
      if (Sid->MIB == NULL) {
         log_event(ups, LOG_ERR, "Out of memory.");
         exit(1);
      }

      memset(Sid->MIB, 0, sizeof(powernet_mib_t));

      /* Run powernet specific init */
      return powernet_snmp_ups_open(ups);
   }

   if (!strcmp(Sid->DeviceVendor, "RFC")) {
      Sid->MIB = malloc(sizeof(ups_mib_t));
      if (Sid->MIB == NULL) {
         log_event(ups, LOG_ERR, "Out of memory.");
         exit(1);
      }

      memset(Sid->MIB, 0, sizeof(ups_mib_t));
      return 1;
   }

   /* No mib for this vendor. */
   Dmsg1(0, "No MIB defined for vendor %s\n", Sid->DeviceVendor);

   return 0;
}

int snmp_ups_close(UPSINFO *ups)
{
   write_lock(ups);
   free(ups->driver_internal_data);
   ups->driver_internal_data = NULL;
   write_unlock(ups);
   return 1;
}

int snmp_ups_setup(UPSINFO *ups)
{
   /* No need to setup anything. */
   return 1;
}

int snmp_ups_get_capabilities(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   int ret = 0;

   write_lock(ups);

   if (!strcmp(Sid->DeviceVendor, "APC"))
      ret = powernet_snmp_ups_get_capabilities(ups);

   if (!strcmp(Sid->DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_get_capabilities(ups);

   write_unlock(ups);

   return ret;
}

int snmp_ups_program_eeprom(UPSINFO *ups, int command, const char *data)
{
   return 0;
}

int snmp_ups_kill_power(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   int ret = 0;

   if (!strcmp(Sid->DeviceVendor, "APC"))
      ret = powernet_snmp_kill_ups_power(ups);

   if (!strcmp(Sid->DeviceVendor, "RFC"))
      ret = rfc1628_snmp_kill_ups_power(ups);

   return ret;
}

int snmp_ups_check_state(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   int ret = 0;

   if (!strcmp(Sid->DeviceVendor, "APC"))
      ret = powernet_snmp_ups_check_state(ups);

   if (!strcmp(Sid->DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_check_state(ups);

   return ret;
}

int snmp_ups_read_volatile_data(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   int ret = 0;

   write_lock(ups);

   ups->poll_time = time(NULL);    /* save time stamp */
   if (!strcmp(Sid->DeviceVendor, "APC"))
      ret = powernet_snmp_ups_read_volatile_data(ups);

   if (!strcmp(Sid->DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_read_volatile_data(ups);

   write_unlock(ups);

   return ret;
}

int snmp_ups_read_static_data(UPSINFO *ups)
{
   struct snmp_ups_internal_data *Sid =
      (struct snmp_ups_internal_data *)ups->driver_internal_data;
   int ret = 0;

   write_lock(ups);

   if (!strcmp(Sid->DeviceVendor, "APC"))
      ret = powernet_snmp_ups_read_static_data(ups);

   if (!strcmp(Sid->DeviceVendor, "RFC"))
      ret = rfc1628_snmp_ups_read_static_data(ups);

   write_unlock(ups);

   return ret;
}

int snmp_ups_entry_point(UPSINFO *ups, int command, void *data)
{
   return 0;
}
