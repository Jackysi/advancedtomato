/*
 * hidutils.c
 *
 * Platform-specific USB module for *BSD ugen USB driver.
 *
 * Based on linux-usb.c by Kerb Sibbald.
 */

/*
 * Copyright (C) 2004-2005 Adam Kropelin
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
#include "hidutils.h"
#include "../usb_common.h"
#include <dev/usb/usb.h>
#include <dev/usb/usbhid.h>

/* Compatibility cruft for FreeBSD <= 4.7 */
#ifndef USB_MAX_DEVNAMES
#define USB_MAX_DEVNAMES MAXDEVNAMES
#endif
#ifndef USB_MAX_DEVNAMELEN
#define USB_MAX_DEVNAMELEN MAXDEVNAMELEN
#endif

/*
 * When we are traversing the USB reports given by the UPS and we find
 * an entry corresponding to an entry in the known_info table above,
 * we make the following USB_INFO entry in the info table of our
 * private data.
 */
typedef struct s_usb_info {
   unsigned usage_code;            /* usage code wanted */
   unsigned unit_exponent;         /* exponent */
   unsigned unit;                  /* units */
   int data_type;                  /* data type */
   hid_item_t item;                /* HID item (read) */
   hid_item_t witem;               /* HID item (write) */
   int report_len;                 /* Length of containing report */
   int ci;                         /* which CI does this usage represent? */
   int value;                      /* Previous value of this item */
} USB_INFO;

/*
 * This "private" structure is returned to us in the driver private
 * field, and allows us to get to all the info we keep on each UPS.
 * The info field is malloced for each command we want and the UPS
 * has.
 */
typedef struct s_usb_data {
   int fd;                         /* Our UPS control pipe fd when open */
   int intfd;                      /* Interrupt pipe fd */
   char orig_device[MAXSTRING];    /* Original port specification */
   short vendor;                   /* UPS vendor id */
   report_desc_t rdesc;            /* Device's report descrptor */
   USB_INFO *info[CI_MAXCI + 1];   /* Info pointers for each command */
} USB_DATA;


static void reinitialize_private_structure(UPSINFO *ups)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   int k;

   Dmsg0(200, "Reinitializing private structure.\n");
   /*
    * We are being reinitialized, so clear the Cap
    *   array, and release previously allocated memory.
    */
   for (k = 0; k <= CI_MAXCI; k++) {
      ups->UPS_Cap[k] = false;
      if (my_data->info[k] != NULL) {
         free(my_data->info[k]);
         my_data->info[k] = NULL;
      }
   }
}

/*
 * Initializes the USB device by fetching its report descriptor
 * and making sure we can drive the device.
 */
static int init_device(UPSINFO *ups, const char *devname)
{
   int fd, rc, rdesclen;
   struct usb_device_info devinfo;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   unsigned char *rdesc;
   char intdevname[USB_MAX_DEVNAMELEN + 5 + 3 + 1];

   fd = open(devname, O_RDWR | O_NOCTTY);
   if (fd == -1)
      return 0;

   memset(&devinfo, 0, sizeof(devinfo));
   rc = ioctl(fd, USB_GET_DEVICEINFO, &devinfo);
   if (rc) {
      close(fd);
      Dmsg0(100, "Unable to get device info.\n");
      return 0;
   }

   /* Only interested if APC or MGE is the vendor */
   if (devinfo.udi_vendorNo != VENDOR_APC &&
       devinfo.udi_vendorNo != VENDOR_MGE) {
      close(fd);
      Dmsg1(100, "Unsupported vendor (%04x).\n", devinfo.udi_vendorNo);
      return 0;
   }
   my_data->vendor = devinfo.udi_vendorNo;

   /* Fetch the report descritor */
   rdesc = hidu_fetch_report_descriptor(fd, &rdesclen);
   if (!rdesc) {
      close(fd);
      Dmsg0(100, "Unable to fetch report descriptor.\n");
      return 0;
   }

   /* Initialize hid parser with this descriptor */
   my_data->rdesc = hid_use_report_desc(rdesc, rdesclen);
   if (!my_data->rdesc) {
      free(rdesc);
      close(fd);
      Dmsg0(100, "Unable to init parser with report descriptor.\n");
      return 0;
   }
   free(rdesc);

   /* Does this device have an UPS application collection? */
   if (!hidu_locate_item(
         my_data->rdesc,
         UPS_USAGE,             /* Match usage code */
         -1,                    /* Don't care about application */
         -1,                    /* Don't care about physical usage */
         -1,                    /* Don't care about logical */
         HID_KIND_COLLECTION,   /* Match collection type */
         NULL)) {
      hid_dispose_report_desc(my_data->rdesc);
      close(fd);
      Dmsg0(100, "Device does not have an UPS application collection.\n");
      return 0;
   }

   my_data->fd = fd;

   /* Open the interrupt pipe */
   astrncpy(intdevname, devname, sizeof(intdevname));

#ifdef HAVE_FREEBSD_OS
   /* ugen0 -> ugen0.1 */
   astrncat(intdevname, ".1", sizeof(intdevname));
#else
   /* ugen0.00 -> ugen0.01 */
   intdevname[strlen(intdevname) - 1] = '1';
#endif

   fd = open(intdevname, O_RDONLY | O_NOCTTY);
   if (fd == -1) {
      Dmsg2(100, "Unable to open interrupt pipe %s: %s\n", intdevname,
         strerror(errno));
      hid_dispose_report_desc(my_data->rdesc);
      close(my_data->fd);
      my_data->fd = -1;
      return 0;
   }
   my_data->intfd = fd;

   return 1;
}

/*
 * Internal routine to open the device and ensure that there is a UPS
 * application on the line.  This routine may be called many times 
 * because the device may be unplugged and plugged back in -- the 
 * joys of USB devices.
 */
static int open_usb_device(UPSINFO *ups)
{
   int i, j, k, fd, rc;
   char busname[] = "/dev/usbN";
   char devname[USB_MAX_DEVNAMELEN + 5 + 1];
   struct usb_device_info devinfo;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

   /*
    * Note, we set ups->fd here so the "core" of apcupsd doesn't
    * think we are a slave, which is what happens when it is -1.
    * (ADK: Actually this only appears to be true for apctest as
    * apcupsd proper uses the UPS_slave flag.)
    * Internally, we use the fd in our own private space   
    */
   ups->fd = 1;

   /*
    * If no device location specified, we go autodetect it
    * by searching known places.
    */
   if (ups->device[0] == 0)
      goto auto_detect;

   if (my_data->orig_device[0] == 0)
      astrncpy(my_data->orig_device, ups->device, sizeof(my_data->orig_device));

   /*
    * No range support yet... Name the device specifically or we will
    * search them all.
    */

   for (i = 0; i < 10; i++) {
      if (init_device(ups, ups->device))
         return 1;

      sleep(1);
   }

   /*
    * If the above device specified by the user fails,
    * fall through here and look in predefined places
    * for the device.
    */

 auto_detect:

   /*
    * We could just start trying to open the /dev/ugenN devices,
    * one after another, but BSD gives us a decent way to enumerate
    * them. We might as well be polite and use it.
    */

   /* Max of 10 USB busses */
   for (i = 0; i < 10; i++) {
      busname[8] = '0' + i;
      fd = open(busname, O_RDWR | O_NOCTTY);
      if (fd == -1)
         continue;

      Dmsg1(200, "Found bus %s.\n", busname);

      /* Max 127 devices per bus */
      for (j = 0; j < 127; j++) {
         memset(&devinfo, 0, sizeof(devinfo));
         devinfo.udi_addr = j;
         rc = ioctl(fd, USB_DEVICEINFO, &devinfo);
         if (rc)
            continue;

         /* See if this device is bound to ugen driver */
         for (k = 0; k < USB_MAX_DEVNAMES; k++)
            if (strncmp(devinfo.udi_devnames[k], "ugen", 4) == 0)
               break;

         if (k < USB_MAX_DEVNAMES) {
            astrncpy(devname, "/dev/", sizeof(devname));
            astrncat(devname, devinfo.udi_devnames[k], sizeof(devname));
#if defined(HAVE_OPENBSD_OS) || defined(HAVE_NETBSD_OS)
            astrncat(devname, ".00", sizeof(devname));
#endif
            Dmsg1(200, "Trying device %s.\n", devname);
            if (init_device(ups, devname)) {
               astrncpy(ups->device, devname, sizeof(ups->device));
               return 1;
            }
         }
      }

      close(fd);
   }

   ups->device[0] = 0;
   return 0;
}

/* 
 * Called if there is an ioctl() or read() error, we close() and
 * re open() the port since the device was probably unplugged.
 */
static int usb_link_check(UPSINFO *ups)
{
   bool comm_err = true;
   int tlog;
   bool once = true;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   static bool linkcheck = false;

   if (linkcheck)
      return 0;

   linkcheck = true;               /* prevent recursion */

   ups->set_commlost();
   Dmsg0(200, "link_check comm lost\n");

   /* Don't warn until we try to get it at least 2 times and fail */
   for (tlog = LINK_RETRY_INTERVAL * 2; comm_err; tlog -= (LINK_RETRY_INTERVAL)) {

      if (tlog <= 0) {
         tlog = 10 * 60;           /* notify every 10 minutes */
         log_event(ups, event_msg[CMDCOMMFAILURE].level,
                   event_msg[CMDCOMMFAILURE].msg);
         if (once) {               /* execute script once */
            execute_command(ups, ups_event[CMDCOMMFAILURE]);
            once = false;
         }
      }

      /* Retry every LINK_RETRY_INTERVAL seconds */
      sleep(LINK_RETRY_INTERVAL);

      if (my_data->fd >= 0) {
         close(my_data->fd);
         my_data->fd = -1;
         close(my_data->intfd);
         my_data->intfd = -1;
         hid_dispose_report_desc(my_data->rdesc);
         reinitialize_private_structure(ups);
      }

      if (open_usb_device(ups) && usb_ups_get_capabilities(ups) &&
         usb_ups_read_static_data(ups)) {
         comm_err = false;
      } else {
         continue;
      }
   }

   if (!comm_err) {
      generate_event(ups, CMDCOMMOK);
      ups->clear_commlost();
      Dmsg0(200, "link check comm OK.\n");
   }

   linkcheck = false;
   return 1;
}

int pusb_ups_get_capabilities(UPSINFO *ups, const struct s_known_info *known_info)
{
   int i, input, feature, ci, phys, logi;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   hid_item_t input_item, feature_item, item;
   USB_INFO *info;

   write_lock(ups);

   for (i = 0; known_info[i].usage_code; i++) {
      ci = known_info[i].ci;
      phys = known_info[i].physical;
      logi = known_info[i].logical;

      if (ci != CI_NONE && !my_data->info[ci]) {
         /* Try to find an INPUT report containing this usage */
         input = hidu_locate_item(
            my_data->rdesc,
            known_info[i].usage_code,     /* Match usage code */
            -1,                           /* Don't care about application */
            (phys == P_ANY) ? -1 : phys,  /* Match physical usage */
            (logi == P_ANY) ? -1 : logi,  /* Match logical usage */
            HID_KIND_INPUT,               /* Match feature type */
            &input_item);

         /* Try to find a FEATURE report containing this usage */
         feature = hidu_locate_item(
            my_data->rdesc,
            known_info[i].usage_code,     /* Match usage code */
            -1,                           /* Don't care about application */
            (phys == P_ANY) ? -1 : phys,  /* Match physical usage */
            (logi == P_ANY) ? -1 : logi,  /* Match logical usage */
            HID_KIND_FEATURE,             /* Match feature type */
            &feature_item);

         /*
          * Choose which report to use. We prefer FEATURE since some UPSes
          * have broken INPUT reports, but we will fall back on INPUT if
          * FEATURE is not available.
          */
         if (feature)
            item = feature_item;
         else if (input)
            item = input_item;
         else
            continue; // No valid report, bail

         ups->UPS_Cap[ci] = true;
         ups->UPS_Cmd[ci] = known_info[i].usage_code;

         info = (USB_INFO *)malloc(sizeof(USB_INFO));
         if (!info) {
            write_unlock(ups);
            Error_abort0("Out of memory.\n");
         }

         // Populate READ report data
         my_data->info[ci] = info;
         memset(info, 0, sizeof(*info));
         info->ci = ci;
         info->usage_code = item.usage;
         info->unit_exponent = item.unit_exponent;
         info->unit = item.unit;
         info->data_type = known_info[i].data_type;
         info->item = item;
         info->report_len = hid_report_size( /* +1 for report id */
            my_data->rdesc, item.kind, item.report_ID) + 1;
         Dmsg6(200, "Got READ ci=%d, rpt=%d (len=%d), usage=0x%x (len=%d), kind=0x%02x\n",
            ci, item.report_ID, info->report_len,
            known_info[i].usage_code, item.report_size, item.kind);

         // If we have a FEATURE report, use that as the writable report
         if (feature) {
            info->witem = item;
            Dmsg6(200, "Got WRITE ci=%d, rpt=%d (len=%d), usage=0x%x (len=%d), kind=0x%02x\n",
               ci, item.report_ID, info->report_len,
               known_info[i].usage_code, item.report_size, item.kind);               
         }
      }
   }

   ups->UPS_Cap[CI_STATUS] = true; /* we always have status flag */
   write_unlock(ups);
   return 1;
}

static bool populate_uval(UPSINFO *ups, USB_INFO *info, unsigned char *data, USB_VALUE *uval)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   const char *str;
   int exponent;
   USB_VALUE val;

   /* data+1 skips the report tag byte */
   info->value = hid_get_data(data+1, &info->item);

   exponent = info->unit_exponent;
   if (exponent > 7)
      exponent = exponent - 16;

   if (info->data_type == T_INDEX) {    /* get string */
      if (info->value == 0)
         return false;

      str = hidu_get_string(my_data->fd, info->value);
      if (!str)
         return false;

      astrncpy(val.sValue, str, sizeof(val.sValue));
      val.value_type = V_STRING;

      Dmsg4(200, "Def val=%d exp=%d sVal=\"%s\" ci=%d\n", info->value,
         exponent, val.sValue, info->ci);
   } else if (info->data_type == T_UNITS) {
      val.value_type = V_DOUBLE;

      switch (info->unit) {
      case 0x00F0D121:
         val.UnitName = "Volts";
         exponent -= 7;            /* remove bias */
         break;
      case 0x00100001:
         exponent += 2;            /* remove bias */
         val.UnitName = "Amps";
         break;
      case 0xF001:
         val.UnitName = "Hertz";
         break;
      case 0x1001:
         val.UnitName = "Seconds";
         break;
      case 0xD121:
         exponent -= 7;            /* remove bias */
         val.UnitName = "Watts";
         break;
      case 0x010001:
         val.UnitName = "Degrees K";
         break;
      case 0x0101001:
         val.UnitName = "AmpSecs";
         if (exponent == 0)
            val.dValue = info->value;
         else
            val.dValue = ((double)info->value) * pow_ten(exponent);
         break;
      default:
         val.UnitName = "";
         val.value_type = V_INTEGER;
         val.iValue = info->value;
         break;
      }

      if (exponent == 0)
         val.dValue = info->value;
      else
         val.dValue = ((double)info->value) * pow_ten(exponent);

      // Store a (possibly truncated) copy of the floating point value in the
      // integer field as well.
      val.iValue = (int)val.dValue;

      Dmsg4(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->value,
         exponent, val.dValue, info->ci);
   } else {                        /* should be T_NONE */

      val.UnitName = "";
      val.value_type = V_INTEGER;
      val.iValue = info->value;

      if (exponent == 0)
         val.dValue = info->value;
      else
         val.dValue = ((double)info->value) * pow_ten(exponent);

      Dmsg4(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->value,
         exponent, val.dValue, info->ci);
   }

   memcpy(uval, &val, sizeof(*uval));
   return true;   
}

/*
 * Get a field value
 */
int pusb_get_value(UPSINFO *ups, int ci, USB_VALUE *uval)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   USB_INFO *info = my_data->info[ci];
   unsigned char data[20];
   int len;

   /*
    * Note we need to check info since CI_STATUS is always true
    * even when the UPS doesn't directly support that CI.
    */
   if (!UPS_HAS_CAP(ci) || !info)
      return false;                /* UPS does not have capability */

   /*
    * Clear the destination buffer. In the case of a short transfer (see
    * below) this will increase the likelihood of extracting the correct
    * value in spite of the missing data.
    */
   memset(data, 0, sizeof(data));

   /* Fetch the proper report */
   len = hidu_get_report(my_data->fd, &info->item, data, info->report_len);
   if (len == -1)
      return false;

   /*
    * Some UPSes seem to have broken firmware that sends a different number
    * of bytes (usually fewer) than the report descriptor specifies. On
    * UHCI controllers under *BSD, this can lead to random lockups. To
    * reduce the likelihood of a lockup, we adjust our expected length to
    * match the actual as soon as a mismatch is detected, so future
    * transfers will have the proper lengths from the outset. NOTE that
    * the data returned may not be parsed properly (since the parsing is
    * necessarily based on the report descriptor) but given that HID
    * reports are in little endian byte order and we cleared the buffer
    * above, chances are good that we will actually extract the right
    * value in spite of the UPS's brokenness.
    */
   if (info->report_len != len) {
      Dmsg4(100, "Report length mismatch, fixing "
         "(id=%d, ci=%d, expected=%d, actual=%d)\n",
         info->item.report_ID, ci, info->report_len, len);
      info->report_len = len;
   }

   /* Populate a uval struct using the raw report data */
   return populate_uval(ups, info, data, uval);
}

/*
 * Read UPS events. I.e. state changes.
 */
int pusb_ups_check_state(UPSINFO *ups)
{
   int i, ci;
   int retval, value;
   unsigned char buf[100];
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   struct timespec now, exit;
   struct timeval tv;
   fd_set rfds;
   USB_VALUE uval;
   bool done = false;

   /* Figure out when we need to exit by */
   clock_gettime(CLOCK_REALTIME, &exit);
   exit.tv_sec += ups->wait_time;

   while (!done) {

      /* Figure out how long until we have to exit */
      clock_gettime(CLOCK_REALTIME, &now);

      if (now.tv_sec > exit.tv_sec ||
         (now.tv_sec == exit.tv_sec &&
            now.tv_nsec / 1000 >= exit.tv_nsec / 1000)) {
         /* Done already? How time flies... */
         return 0;
      }

      tv.tv_sec = exit.tv_sec - now.tv_sec;
      tv.tv_usec = (exit.tv_nsec - now.tv_nsec) / 1000;
      if (tv.tv_usec < 0) {
         tv.tv_sec--;              /* Normalize */
         tv.tv_usec += 1000000;
      }

      FD_ZERO(&rfds);
      FD_SET(my_data->intfd, &rfds);

      retval = select((my_data->intfd) + 1, &rfds, NULL, NULL, &tv);

      switch (retval) {
      case 0:                     /* No chars available in TIMER seconds. */
         return 0;
      case -1:
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;
         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
         usb_link_check(ups);      /* link is down, wait */
         return 0;
      default:
         break;
      }

      do {
         retval = read(my_data->intfd, buf, sizeof(buf));
      } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

      if (retval < 0) {            /* error */
         Dmsg1(200, "read error: ERR=%s\n", strerror(errno));
         usb_link_check(ups);      /* notify that link is down, wait */
         return 0;
      }

      if (debug_level >= 300) {
         logf("Interrupt data: ");
         for (i = 0; i < retval; i++)
            logf("%02x, ", buf[i]);
         logf("\n");
      }

      write_lock(ups);

      /*
       * Iterate over all CIs, firing off events for any that are
       * affected by this report.
       */
      for (ci=0; ci<CI_MAXCI; ci++) {
         if (ups->UPS_Cap[ci] && my_data->info[ci] &&
             my_data->info[ci]->item.report_ID == buf[0]) {

            /*
             * Check if we received fewer bytes of data from the UPS than we
             * should have. If so, ignore the report since we can't process it
             * reliably. If we go ahead and try to process it we may get 
             * sporradic bad readings. UPSes we've seen this issue on so far 
             * include:
             *
             *    "Back-UPS CS 650 FW:817.v7 .I USB FW:v7"
             *    "Back-UPS CS 500 FW:808.q8.I USB FW:q8"
             */
            if (my_data->info[ci]->report_len != retval) {
               Dmsg4(100, "Report length mismatch, ignoring "
                  "(id=%d, ci=%d, expected=%d, actual=%d)\n",
                  my_data->info[ci]->item.report_ID, ci, 
                  my_data->info[ci]->report_len, retval);
               break; /* don't continue since other CIs will be just as wrong */
            }

            /* Ignore this event if the value has not changed */
            value = hid_get_data(buf+1, &my_data->info[ci]->item);
            if (my_data->info[ci]->value == value) {
               Dmsg3(200, "Ignoring unchanged value (ci=%d, rpt=%d, val=%d)\n",
                  ci, buf[0], value);
               continue;
            }

            Dmsg3(200, "Processing changed value (ci=%d, rpt=%d, val=%d)\n",
               ci, buf[0], value);

            /* Populate a uval and report it to the upper layer */
            populate_uval(ups, my_data->info[ci], buf, &uval);
            if (usb_report_event(ups, ci, &uval)) {
               /*
                * The upper layer considers this an important event,
                * so we will return after processing any remaining
                * CIs for this report.
                */
               done = true;
            }
         }
      }

      write_unlock(ups);
   }
   
   return true;
}

/*
 * Open usb port
 *
 * This is called once by the core code and is the first 
 * routine called.
 */
int pusb_ups_open(UPSINFO *ups)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;

   write_lock(ups);
   if (my_data == NULL) {
      my_data = (USB_DATA *)malloc(sizeof(USB_DATA));
      if (my_data == NULL) {
         log_event(ups, LOG_ERR, "Out of memory.");
         write_unlock(ups);
         exit(1);
      }

      memset(my_data, 0, sizeof(USB_DATA));
      ups->driver_internal_data = my_data;
   } else {
      reinitialize_private_structure(ups);
   }

   if (my_data->orig_device[0] == 0)
      astrncpy(my_data->orig_device, ups->device, sizeof(my_data->orig_device));

   if (!open_usb_device(ups)) {
      write_unlock(ups);
      if (ups->device[0]) {
         Error_abort1("Cannot open UPS device: \"%s\" --\n"
               "For a link to detailed USB trouble shooting information,\n"
               "please see <http://www.apcupsd.com/support.html>.\n", ups->device);
      } else {
         Error_abort0("Cannot find UPS device --\n"
               "For a link to detailed USB trouble shooting information,\n"
               "please see <http://www.apcupsd.com/support.html>.\n");
      }
   }

   ups->clear_slave();
   write_unlock(ups);
   return 1;
}

/*
 * This is the last routine called from apcupsd core code 
 */
int pusb_ups_close(UPSINFO *ups)
{
   /* Should we be politely closing fds here or anything? */
   write_lock(ups);

   if (ups->driver_internal_data) {
      free(ups->driver_internal_data);
      ups->driver_internal_data = NULL;
   }

   write_unlock(ups);
   return 1;
}

int pusb_ups_setup(UPSINFO *ups)
{
   /* Nothing to do */
   return 1;
}

int pusb_read_int_from_ups(UPSINFO *ups, int ci, int *value)
{
   USB_VALUE val;

   if (!pusb_get_value(ups, ci, &val))
      return false;

   *value = val.iValue;
   return true;
}

int pusb_write_int_to_ups(UPSINFO *ups, int ci, int value, const char *name)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   USB_INFO *info;
   int old_value, new_value;
   unsigned char rpt[20];

   if (ups->UPS_Cap[ci] && my_data->info[ci] && my_data->info[ci]->witem.report_ID) {
      info = my_data->info[ci];    /* point to our info structure */

      if (hidu_get_report(my_data->fd, &info->item, rpt, info->report_len) < 1) {
         Dmsg1(000, "get_report for kill power function %s failed.\n", name);
         return false;
      }

      old_value = hid_get_data(rpt + 1, &info->item);

      hid_set_data(rpt + 1, &info->witem, value);

      if (!hidu_set_report(my_data->fd, &info->witem, rpt, info->report_len)) {
         Dmsg1(000, "set_report for kill power function %s failed.\n", name);
         return false;
      }

      if (hidu_get_report(my_data->fd, &info->item, rpt, info->report_len) < 1) {
         Dmsg1(000, "get_report for kill power function %s failed.\n", name);
         return false;
      }

      new_value = hid_get_data(rpt + 1, &info->item);

      Dmsg3(100, "function %s ci=%d value=%d OK.\n", name, ci, value);
      Dmsg4(100, "%s before=%d set=%d after=%d\n", name, old_value, value, new_value);
      return true;
   }

   Dmsg2(000, "function %s ci=%d not available in this UPS.\n", name, ci);
   return false;
}
