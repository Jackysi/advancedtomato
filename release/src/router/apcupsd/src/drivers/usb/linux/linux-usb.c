/*
 * linux-usb.c
 *
 * Platform-specific interface to Linux hiddev USB HID driver.
 *
 * Parts of this code (especially the initialization and walking
 * the reports) were derived from a test program hid-ups.c by:    
 *    Vojtech Pavlik <vojtech@ucw.cz>
 *    Paul Stewart <hiddev@wetlogic.net>
 */

/*
 * Copyright (C) 2001-2004 Kern Sibbald
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

/*
 * The following is a work around for a problem in 2.6 kernel
 *  linux/hiddev.h file that is fixed in 2.6.9
 */
#define HID_MAX_USAGES 1024

#include "apc.h"
#include "../usb_common.h"
#include <asm/types.h>
#include <linux/hiddev.h>

/* RHEL has an out-of-date hiddev.h */
#ifndef HIDIOCGFLAG
# define HIDIOCSFLAG       _IOW('H', 0x0F, int)
#endif
#ifndef HIDDEV_FLAG_UREF
# define HIDDEV_FLAG_UREF  0x1
#endif

/* Enable this to force Linux 2.4 compatability mode */
#define FORCE_COMPAT24  false

/*
 * When we are traversing the USB reports given by the UPS and we
 * find an entry corresponding to an entry in the known_info table
 * above, we make the following USB_INFO entry in the info table
 * of our private data.
 */
typedef struct s_usb_info {
   unsigned physical;              /* physical value wanted */
   unsigned unit_exponent;         /* exponent */
   unsigned unit;                  /* units */
   int data_type;                  /* data type */
   int ci;                         /* which CI does this usage represent? */
   struct hiddev_usage_ref uref;   /* usage reference (read) */
   struct hiddev_usage_ref wuref;  /* usage reference (write) */
} USB_INFO;

/*
 * This "private" structure is returned to us in the driver private
 * field, and allows us to get to all the info we keep on each UPS.
 * The info field is malloced for each command we want and the UPS
 * has.
 */
typedef struct s_usb_data {
   int fd;                         /* Our UPS fd when open */
   bool compat24;                  /* Linux 2.4 compatibility mode */
   char orig_device[MAXSTRING];    /* Original port specification */
   USB_INFO *info[CI_MAXCI + 1];   /* Info pointers for each command */
} USB_DATA;


static void reinitialize_private_structure(UPSINFO *ups)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   int k;

   Dmsg0(200, "Reinitializing private structure.\n");
   /*
    * We are being reinitialized, so clear the Cap
    * array, and release previously allocated memory.
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
 * Internal routine to attempt device open.
 */
static int open_device(const char *dev, UPSINFO *ups)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   int flaguref = HIDDEV_FLAG_UREF;
   int fd, ret, i;

   Dmsg1(200, "Attempting to open \"%s\"\n", dev);

   /* Open the device port */
   fd = open(dev, O_RDWR | O_NOCTTY);
   if (fd >= 0) {
      /* Check for the UPS application HID usage */
      for (i = 0; (ret = ioctl(fd, HIDIOCAPPLICATION, i)) > 0; i++) {
         if ((ret & 0xffff000) == (UPS_USAGE & 0xffff0000)) {
            /* Request full uref reporting from read() */
            if (FORCE_COMPAT24 || ioctl(fd, HIDIOCSFLAG, &flaguref)) {
               Dmsg0(100, "HIDIOCSFLAG failed; enabling linux-2.4 "
                      "compatibility mode\n");
               my_data->compat24 = true;
            }
            /* Successfully opened the device */
            Dmsg1(200, "Successfully opened \"%s\"\n", dev);
            return fd;
         }
      }
      close(fd);
   }

   /* Failed to open the device */
   return -1;
}

/*
 * Internal routine to open the device and ensure that there is
 * a UPS application on the line.  This routine may be called
 * many times because the device may be unplugged and plugged
 * back in -- the joys of USB devices.
 */
static int open_usb_device(UPSINFO *ups)
{
   char devname[MAXSTRING];
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   const char *hiddev[] =
      { "/dev/usb/hiddev", "/dev/usb/hid/hiddev", "/dev/hiddev", NULL };
   int i, j, k;

   /*
    * Note, we set ups->fd here so the "core" of apcupsd doesn't
    * think we are a slave, which is what happens when it is -1.
    * (ADK: Actually this only appears to be true for apctest as
    * apcupsd proper uses the UPS_slave flag.)
    * Internally, we use the fd in our own private space   
    */
   ups->fd = 1;

   /*
    * If no device locating specified, we go autodetect it
    * by searching known places.
    */
   if (ups->device[0] == 0)
      goto auto_detect;

   /*
    * Also if specified device includes deprecated '[]' notation,
    * just use the automatic search.
    */
   if (strchr(ups->device, '[') &&
       strchr(ups->device, ']'))
   {
      my_data->orig_device[0] = 0;
      goto auto_detect;
   }

   /*
    * If we get here we know the user specified a device or we are
    * trying to re-open a device that previously was open.
    */
 
   /* Retry 10 times */
   for (i = 0; i < 10; i++) {
      my_data->fd = open_device(ups->device, ups);
      if (my_data->fd != -1)
         return 1;
      sleep(1);
   }

   /*
    * If user-specified device could not be opened, fail.
    */
   if (my_data->orig_device[0] != 0)
      return 0;

   /*
    * If we get here we failed to re-open a previously auto-detected
    * device. We will fall thru and restart autodetection...
    */

auto_detect:
 
   for (i = 0; i < 10; i++) {           /* try 10 times */
      for (j = 0; hiddev[j]; j++) {     /* loop over known device names */
         for (k = 0; k < 16; k++) {     /* loop over devices */
            asnprintf(devname, sizeof(devname), "%s%d", hiddev[j], k);
            my_data->fd = open_device(devname, ups);
            if (my_data->fd != -1) {
               /* Successful open, save device name and return */
               astrncpy(ups->device, devname, sizeof(ups->device));
               return 1;
            }
         }
      }
      sleep(1);
   }

   ups->device[0] = '\0';
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

static bool populate_uval(UPSINFO *ups, USB_INFO *info, USB_VALUE *uval)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   struct hiddev_string_descriptor sdesc;
   USB_VALUE val;
   int exponent;

   exponent = info->unit_exponent;
   if (exponent > 7)
      exponent = exponent - 16;

   if (info->data_type == T_INDEX) {    /* get string */
      if (info->uref.value == 0)
         return false;

      sdesc.index = info->uref.value;
      if (ioctl(my_data->fd, HIDIOCGSTRING, &sdesc) < 0)
         return false;

      astrncpy(val.sValue, sdesc.value, sizeof(val.sValue));
      val.value_type = V_STRING;

      Dmsg4(200, "Def val=%d exp=%d sVal=\"%s\" ci=%d\n", info->uref.value,
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
         break;
      default:
         val.UnitName = "";
         val.value_type = V_INTEGER;
         val.iValue = info->uref.value;
         break;
      }

      if (exponent == 0)
         val.dValue = info->uref.value;
      else
         val.dValue = ((double)info->uref.value) * pow_ten(exponent);

      // Store a (possibly truncated) copy of the floating point value in the
      // integer field as well.
      val.iValue = (int)val.dValue;

      Dmsg4(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->uref.value,
         exponent, val.dValue, info->ci);
   } else {                        /* should be T_NONE */
      val.UnitName = "";
      val.value_type = V_INTEGER;
      val.iValue = info->uref.value;

      if (exponent == 0)
         val.dValue = info->uref.value;
      else
         val.dValue = ((double)info->uref.value) * pow_ten(exponent);

      Dmsg4(200, "Def val=%d exp=%d dVal=%f ci=%d\n", info->uref.value,
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
   struct hiddev_report_info rinfo;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   USB_INFO *info;

   if (!ups->UPS_Cap[ci] || !my_data->info[ci])
      return false;                /* UPS does not have capability */

   /* Fetch the new value from the UPS */

   info = my_data->info[ci];       /* point to our info structure */
   rinfo.report_type = info->uref.report_type;
   rinfo.report_id = info->uref.report_id;
   if (ioctl(my_data->fd, HIDIOCGREPORT, &rinfo) < 0)   /* update Report */
      return false;

   if (ioctl(my_data->fd, HIDIOCGUSAGE, &info->uref) < 0)       /* update UPS value */
      return false;

   /* Process the updated value */
   return populate_uval(ups, info, uval);
}

/*
 * Find the USB_INFO structure used for tracking a given usage. Searching
 * by usage_code alone is insufficient since the same usage may appear in
 * multiple reports or even multiple times in the same report.
 */
static USB_INFO *find_info_by_uref(UPSINFO *ups, struct hiddev_usage_ref *uref)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   int i;

   for (i=0; i<CI_MAXCI; i++) {
      if (ups->UPS_Cap[i] && my_data->info[i] &&
          my_data->info[i]->uref.report_id == uref->report_id &&
          my_data->info[i]->uref.field_index == uref->field_index &&
          my_data->info[i]->uref.usage_index == uref->usage_index &&
          my_data->info[i]->uref.usage_code == uref->usage_code) {
            return my_data->info[i];
      }
   }

   return NULL;
}

/*
 * Same as find_info_by_uref() but only checks the usage code. This is
 * not entirely reliable, but it's the best we have on linux-2.4.
 */
static USB_INFO *find_info_by_ucode(UPSINFO *ups, unsigned int ucode)
{
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   int i;

   for (i=0; i<CI_MAXCI; i++) {
      if (ups->UPS_Cap[i] && my_data->info[i] &&
          my_data->info[i]->uref.usage_code == ucode) {
            return my_data->info[i];
      }
   }

   return NULL;
}

/*
 * Read UPS events. I.e. state changes.
 */
int pusb_ups_check_state(UPSINFO *ups)
{
   int retval;
   bool done = false;
   struct hiddev_usage_ref uref;
   struct hiddev_event hev;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   USB_INFO* info;
   USB_VALUE uval;

   struct timeval tv;
   tv.tv_sec = ups->wait_time;
   tv.tv_usec = 0;

   while (!done) {
      fd_set rfds;

      FD_ZERO(&rfds);
      FD_SET(my_data->fd, &rfds);

      retval = select((my_data->fd) + 1, &rfds, NULL, NULL, &tv);

      /*
       * Note: We are relying on select() adjusting tv to the amount
       * of time NOT waited. This is a Linux-specific feature but
       * shouldn't be a problem since the driver is only intended for
       * for Linux.
       */

      switch (retval) {
      case 0:                     /* No chars available in TIMER seconds. */
         return false;
      case -1:
         if (errno == EINTR || errno == EAGAIN)         /* assume SIGCHLD */
            continue;

         Dmsg1(200, "select error: ERR=%s\n", strerror(errno));
         usb_link_check(ups);      /* link is down, wait */
         return false;
      default:
         break;
      }

      if (!my_data->compat24) {
         /* This is >= linux-2.6, so we can read a uref directly */
         do {
            retval = read(my_data->fd, &uref, sizeof(uref));
         } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

         if (retval < 0) {            /* error */
            Dmsg1(200, "read error: ERR=%s\n", strerror(errno));
            usb_link_check(ups);      /* notify that link is down, wait */
            return false;
         }

         if (retval == 0 || retval < (int)sizeof(uref))
            return false;

         /* Ignore events we don't recognize */
         if ((info = find_info_by_uref(ups, &uref)) == NULL) {
            Dmsg3(200, "Unrecognized usage (rpt=%d, usg=0x%08x, val=%d)\n",
               uref.report_id, uref.usage_code, uref.value);
            continue;
         }
      } else {
         /*
          * We're in linux-2.4 compatibility mode, so we read a
          * hiddev_event and use it to construct a uref.
          */
         do {
            retval = read(my_data->fd, &hev, sizeof(hev));
         } while (retval == -1 && (errno == EAGAIN || errno == EINTR));

         if (retval < 0) {            /* error */
            Dmsg1(200, "read error: ERR=%s\n", strerror(errno));
            usb_link_check(ups);      /* notify that link is down, wait */
            return false;
         }

         if (retval == 0 || retval < (int)sizeof(hev))
            return false;

         /* Ignore events we don't recognize */
         if ((info = find_info_by_ucode(ups, hev.hid)) == NULL) {
            Dmsg2(200, "Unrecognized usage (usg=0x%08x, val=%d)\n",
               hev.hid, hev.value);
            continue;
         }

         /*
          * ADK FIXME: The info-> struct we have now is not guaranteed to
          * actually be the right one, because linux-2.4 does not give us
          * enough data in the event to make a positive match. We may need
          * to filter out ambiguous usages here or manually fetch each CI
          * that matches the given usage.
          */

         /* Copy the stored uref, replacing its value */
         uref = info->uref;
         uref.value = hev.value;
      }

      write_lock(ups);

      /* Ignore events whose value is unchanged */
      if (info->uref.value == uref.value) {
         Dmsg3(200, "Ignoring unchanged value (rpt=%d, usg=0x%08x, val=%d)\n",
            uref.report_id, uref.usage_code, uref.value);
         write_unlock(ups);
         continue;
      }

      /* Update tracked value */
      Dmsg3(200, "Processing changed value (rpt=%d, usg=0x%08x, val=%d)\n",
         uref.report_id, uref.usage_code, uref.value);
      info->uref.value = uref.value;

      /* Populate a uval and report it to the upper layer */
      populate_uval(ups, info, &uval);
      if (usb_report_event(ups, info->ci, &uval)) {
         /*
          * The upper layer considers this an important event,
          * so we will return immediately.
          */
         done = true;
      }

      write_unlock(ups);
   }

   return true;
}

/*
 * Open usb port
 * This is called once by the core code and is the first routine
 * called.
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

int pusb_ups_setup(UPSINFO *ups)
{
   /*
    * Seems that there is nothing to do.
    */
   return 1;
}

/*
 * This is the last routine called from apcupsd core code 
 */
int pusb_ups_close(UPSINFO *ups)
{
   write_lock(ups);

   if (ups->driver_internal_data) {
      free(ups->driver_internal_data);
      ups->driver_internal_data = NULL;
   }

   write_unlock(ups);
   return 1;
}

/*
 * Setup capabilities structure for UPS
 */
int pusb_ups_get_capabilities(UPSINFO *ups, const struct s_known_info *known_info)
{
   int rtype[2] = { HID_REPORT_TYPE_FEATURE, HID_REPORT_TYPE_INPUT };
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   struct hiddev_report_info rinfo;
   struct hiddev_field_info finfo;
   struct hiddev_usage_ref uref;
   unsigned int i, j, k, n;

   if (ioctl(my_data->fd, HIDIOCINITREPORT, 0) < 0)
      Error_abort1("Cannot init USB HID report. ERR=%s\n", strerror(errno));

   write_lock(ups);

   /*
    * Walk through all available reports and determine
    * what information we can use.
    */
   for (n = 0; n < sizeof(rtype)/sizeof(*rtype); n++) {
      rinfo.report_type = rtype[n];
      rinfo.report_id = HID_REPORT_ID_FIRST;

      while (ioctl(my_data->fd, HIDIOCGREPORTINFO, &rinfo) >= 0) {
         for (i = 0; i < rinfo.num_fields; i++) {
            memset(&finfo, 0, sizeof(finfo));
            finfo.report_type = rinfo.report_type;
            finfo.report_id = rinfo.report_id;
            finfo.field_index = i;

            if (ioctl(my_data->fd, HIDIOCGFIELDINFO, &finfo) < 0)
               continue;

            memset(&uref, 0, sizeof(uref));
            for (j = 0; j < finfo.maxusage; j++) {
               uref.report_type = finfo.report_type;
               uref.report_id = finfo.report_id;
               uref.field_index = i;
               uref.usage_index = j;

               if (ioctl(my_data->fd, HIDIOCGUCODE, &uref) < 0)
                  continue;

               ioctl(my_data->fd, HIDIOCGUSAGE, &uref);

               /*
                * We've got a UPS usage entry, now walk down our
                * know_info table and see if we have a match. If so,
                * allocate a new entry for it.
                */
               for (k = 0; known_info[k].usage_code; k++) {
                  USB_INFO *info;
                  int ci = known_info[k].ci;

                  if (ci != CI_NONE &&
                      uref.usage_code == known_info[k].usage_code &&
                      (known_info[k].physical == P_ANY ||
                         known_info[k].physical == finfo.physical) &&
                      (known_info[k].logical == P_ANY ||
                         known_info[k].logical == finfo.logical)) {

                     // If we do not have any data saved for this report yet,
                     // allocate an USB_INFO and populate the read uref.
                     info = my_data->info[ci];
                     if (!info) {
                        ups->UPS_Cap[ci] = true;
                        info = (USB_INFO *)malloc(sizeof(USB_INFO));

                        if (!info) {
                           write_unlock(ups);
                           Error_abort0("Out of memory.\n");
                        }

                        my_data->info[ci] = info;
                        memset(info, 0, sizeof(*info));
                        info->ci = ci;
                        info->physical = finfo.physical;
                        info->unit_exponent = finfo.unit_exponent;
                        info->unit = finfo.unit;
                        info->data_type = known_info[k].data_type;
                        memcpy(&info->uref, &uref, sizeof(uref));

                        Dmsg3(200, "Got READ ci=%d, usage=0x%x, rpt=%d\n",
                           ci, known_info[k].usage_code, uref.report_id);
                     }

                     // If this is a FEATURE report and we haven't set the
                     // write uref yet, place it in the write uref (possibly in
                     // addition to placing it in the read uref above).
                     if (rinfo.report_type == HID_REPORT_TYPE_FEATURE &&
                         info->wuref.report_id == 0) {
                        memcpy(&info->wuref, &uref, sizeof(uref));

                        Dmsg3(200, "Got WRITE ci=%d, usage=0x%x, rpt=%d\n",
                           ci, known_info[k].usage_code, uref.report_id);
                     }

                     break;
                  }
               }
            }
         }
         rinfo.report_id |= HID_REPORT_ID_NEXT;
      }
   }

   ups->UPS_Cap[CI_STATUS] = true; /* we have status flag */
   write_unlock(ups);
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
   struct hiddev_report_info rinfo;
   USB_DATA *my_data = (USB_DATA *)ups->driver_internal_data;
   USB_INFO *info;
   int old_value, new_value;

   // Make sure we have a writable uref for this CI
   if (ups->UPS_Cap[ci] && my_data->info[ci] &&
       my_data->info[ci]->wuref.report_id) {
      info = my_data->info[ci];    /* point to our info structure */
      rinfo.report_type = info->uref.report_type;
      rinfo.report_id = info->uref.report_id;

      /* Get report */
      if (ioctl(my_data->fd, HIDIOCGREPORT, &rinfo) < 0) {
         Dmsg2(000, "HIDIOCGREPORT for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /* Get UPS value */
      if (ioctl(my_data->fd, HIDIOCGUSAGE, &info->wuref) < 0) {
         Dmsg2(000, "HIDIOCGUSAGE for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }
      old_value = info->uref.value;

      info->wuref.value = value;
      rinfo.report_type = info->wuref.report_type;
      rinfo.report_id = info->wuref.report_id;
      Dmsg3(100, "SUSAGE type=%d id=%d index=%d\n", info->wuref.report_type,
         info->wuref.report_id, info->wuref.field_index);

      /* Update UPS value */
      if (ioctl(my_data->fd, HIDIOCSUSAGE, &info->wuref) < 0) {
         Dmsg2(000, "HIDIOCSUSAGE for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /* Update Report */
      if (ioctl(my_data->fd, HIDIOCSREPORT, &rinfo) < 0) {
         Dmsg2(000, "HIDIOCSREPORT for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /*
       * This readback of the report is NOT just for debugging. It
       * has the effect of flushing the above SET_REPORT to the 
       * device, which is important since we need to make sure it
       * happens before subsequent reports are sent.
       */

      rinfo.report_type = info->uref.report_type;
      rinfo.report_id = info->uref.report_id;

      /* Get report */
      if (ioctl(my_data->fd, HIDIOCGREPORT, &rinfo) < 0) {
         Dmsg2(000, "HIDIOCGREPORT for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      /* Get UPS value */
      if (ioctl(my_data->fd, HIDIOCGUSAGE, &info->uref) < 0) {
         Dmsg2(000, "HIDIOCGUSAGE for function %s failed. ERR=%s\n",
            name, strerror(errno));
         return false;
      }

      new_value = info->uref.value;
      Dmsg3(100, "function %s ci=%d value=%d OK.\n", name, ci, value);
      Dmsg4(100, "%s before=%d set=%d after=%d\n", name, old_value, value,
         new_value);

      return true;
   }

   Dmsg2(000, "function %s ci=%d not available in this UPS.\n", name, ci);
   return false;
}
