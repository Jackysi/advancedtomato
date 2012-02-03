/*
 * drivers.c
 *
 * UPS drivers middle (link) layer.
 */

/*
 * Copyright (C) 2001-2006 Kern Sibbald
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

#ifdef HAVE_DUMB_DRIVER
# include "dumb/dumb.h"
#endif

#ifdef HAVE_APCSMART_DRIVER
# include "apcsmart/apcsmart.h"
#endif

#ifdef HAVE_NET_DRIVER
# include "net/net.h"
#endif

#ifdef HAVE_USB_DRIVER
# include "usb/usb.h"
#endif

#ifdef HAVE_SNMP_DRIVER
# include "snmp/snmp.h"
#endif

#ifdef HAVE_SNMPLITE_DRIVER
# include "snmplite/snmplite.h"
#endif

#ifdef HAVE_TEST_DRIVER
# include "test/testdriver.h"
#endif

#ifdef HAVE_PCNET_DRIVER
# include "pcnet/pcnet.h"
#endif

static const UPSDRIVER drivers[] = {
#ifdef HAVE_DUMB_DRIVER
   { "dumb",
     dumb_ups_open,
     dumb_ups_setup,
     dumb_ups_close,
     dumb_ups_kill_power,
     NULL,
     dumb_ups_read_static_data,
     dumb_ups_read_volatile_data,
     dumb_ups_get_capabilities,
     dumb_ups_read_volatile_data,
     dumb_ups_program_eeprom,
     dumb_ups_entry_point },
#endif   /* HAVE_DUMB_DRIVER */

#ifdef HAVE_APCSMART_DRIVER
   { "apcsmart",
     apcsmart_ups_open,
     apcsmart_ups_setup,
     apcsmart_ups_close,
     apcsmart_ups_kill_power,
     apcsmart_ups_shutdown,
     apcsmart_ups_read_static_data,
     apcsmart_ups_read_volatile_data,
     apcsmart_ups_get_capabilities,
     apcsmart_ups_check_state,
     apcsmart_ups_program_eeprom,
     apcsmart_ups_entry_point },
#endif   /* HAVE_APCSMART_DRIVER */

#ifdef HAVE_NET_DRIVER
   { "net",
     net_ups_open,
     net_ups_setup,
     net_ups_close,
     net_ups_kill_power,
     NULL,
     net_ups_read_static_data,
     net_ups_read_volatile_data,
     net_ups_get_capabilities,
     net_ups_check_state,
     net_ups_program_eeprom,
     net_ups_entry_point },
#endif   /* HAVE_NET_DRIVER */

#ifdef HAVE_USB_DRIVER
   { "usb",
     usb_ups_open,
     usb_ups_setup,
     usb_ups_close,
     usb_ups_kill_power,
     usb_ups_shutdown,
     usb_ups_read_static_data,
     usb_ups_read_volatile_data,
     usb_ups_get_capabilities,
     usb_ups_check_state,
     usb_ups_program_eeprom,
     usb_ups_entry_point },
#endif   /* HAVE_USB_DRIVER */

#ifdef HAVE_SNMP_DRIVER
   { "snmp",
     snmp_ups_open,
     snmp_ups_setup,
     snmp_ups_close,
     snmp_ups_kill_power,
     NULL,
     snmp_ups_read_static_data,
     snmp_ups_read_volatile_data,
     snmp_ups_get_capabilities,
     snmp_ups_check_state,
     snmp_ups_program_eeprom,
     snmp_ups_entry_point },
#endif   /* HAVE_SNMP_DRIVER */

#ifdef HAVE_SNMPLITE_DRIVER
   { "snmplite",
     snmplite_ups_open,
     snmplite_ups_setup,
     snmplite_ups_close,
     snmplite_ups_kill_power,
     snmplite_ups_shutdown,
     snmplite_ups_read_static_data,
     snmplite_ups_read_volatile_data,
     snmplite_ups_get_capabilities,
     snmplite_ups_check_state,
     snmplite_ups_program_eeprom,
     snmplite_ups_entry_point },
#endif   /* HAVE_SNMPLITE_DRIVER */

#ifdef HAVE_TEST_DRIVER
   { "test",
     test_ups_open,
     test_ups_setup,
     test_ups_close,
     test_ups_kill_power,
     NULL,
     test_ups_read_static_data,
     test_ups_read_volatile_data,
     test_ups_get_capabilities,
     test_ups_check_state,
     test_ups_program_eeprom,
     test_ups_entry_point },
#endif   /* HAVE_TEST_DRIVER */

#ifdef HAVE_PCNET_DRIVER
   { "pcnet",
     pcnet_ups_open,
     pcnet_ups_setup,
     pcnet_ups_close,
     pcnet_ups_kill_power,
     NULL,
     pcnet_ups_read_static_data,
     pcnet_ups_read_volatile_data,
     pcnet_ups_get_capabilities,
     pcnet_ups_check_state,
     pcnet_ups_program_eeprom,
     pcnet_ups_entry_point },
#endif   /* HAVE_PCNET_DRIVER */

   /*
    * The NULL driver: closes the drivers list.
    */
   { NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL,
     NULL }
};

/*
 * This is the glue between UPSDRIVER and UPSINFO.
 * It returns an UPSDRIVER pointer that may be null if something
 * went wrong.
 */
static const UPSDRIVER *helper_attach_driver(UPSINFO *ups, const char *drvname)
{
   int i;

   write_lock(ups);

   Dmsg1(99, "Looking for driver: %s\n", drvname);
   ups->driver = NULL;

   for (i = 0; drivers[i].driver_name; i++) {
      Dmsg1(99, "Driver %s is configured.\n", drivers[i].driver_name);
      if (strcasecmp(drivers[i].driver_name, drvname) == 0) {
         ups->driver = &drivers[i];
         Dmsg1(20, "Driver %s found and attached.\n", drivers[i].driver_name);
         break;
      }
   }

   if (!ups->driver) {
      printf("\nApcupsd driver %s not found.\n"
             "The available apcupsd drivers are:\n", drvname);

      for (i = 0; drivers[i].driver_name; i++)
         printf("%s\n", drivers[i].driver_name);

      printf("\n");
      printf("Most likely, you need to add --enable-%s "
             "to your ./configure options.\n\n", drvname);
   }

   write_unlock(ups);

   Dmsg1(99, "Driver ptr=0x%x\n", ups->driver);
   return ups->driver;
}

const UPSDRIVER *attach_driver(UPSINFO *ups)
{
   const char *driver_name = NULL;

   /* Attach the correct driver. */
   switch (ups->mode.type) {
   case DUMB_UPS:
      driver_name = "dumb";
      break;

   case APCSMART_UPS:
      driver_name = "apcsmart";
      break;

   case USB_UPS:
      driver_name = "usb";
      break;

   case SNMP_UPS:
      driver_name = "snmp";
      break;

   case SNMPLITE_UPS:
      driver_name = "snmplite";
      break;

   case TEST_UPS:
      driver_name = "test";
      break;

   case NETWORK_UPS:
      driver_name = "net";
      break;

   case PCNET_UPS:
      driver_name = "pcnet";
      break;

   default:
   case NO_UPS:
      Dmsg1(000, "Warning: no UPS driver found (ups->mode.type=%d).\n",
         ups->mode.type);
      break;
   }

   return driver_name ? helper_attach_driver(ups, driver_name) : NULL;
}
