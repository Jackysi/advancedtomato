/*
 * smartrsetup.c
 *
 * Functions to open/setup/close the device
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

/* Win32 needs O_BINARY; sane platforms have never heard of it */
#ifndef O_BINARY
#define O_BINARY 0
#endif

/*
 * This is the first routine in the driver that is called.
 */
int apcsmart_ups_open(UPSINFO *ups)
{
   int cmd;
   SMART_DATA *my_data = (SMART_DATA *) ups->driver_internal_data;

   if (my_data == NULL) {
      my_data = (SMART_DATA *) malloc(sizeof(SMART_DATA));
      if (my_data == NULL) {
         log_event(ups, LOG_ERR, "Out of memory.");
         exit(1);
      }

      memset(my_data, 0, sizeof(SMART_DATA));
      ups->driver_internal_data = my_data;
   } else {
      log_event(ups, LOG_ERR,
         "apcsmart_ups_open called twice. This shouldn't happen.");
   }

   char *opendev = ups->device;

#ifdef HAVE_MINGW
   // On Win32 add \\.\ UNC prefix to COMx in order to correctly address
   // ports >= COM10.
   char device[MAXSTRING];
   if (!strnicmp(ups->device, "COM", 3)) {
      snprintf(device, sizeof(device), "\\\\.\\%s", ups->device);
      opendev = device;
   }
#endif

   if ((ups->fd = open(opendev, O_RDWR | O_NOCTTY | O_NDELAY | O_BINARY)) < 0)
      Error_abort2("Cannot open UPS port %s: %s\n", opendev, strerror(errno));

   /* Cancel the no delay we just set */
   cmd = fcntl(ups->fd, F_GETFL, 0);
   fcntl(ups->fd, F_SETFL, cmd & ~O_NDELAY);

   /* Save old settings */
   tcgetattr(ups->fd, &my_data->oldtio);

   my_data->newtio.c_cflag = DEFAULT_SPEED | CS8 | CLOCAL | CREAD;
   my_data->newtio.c_iflag = IGNPAR;    /* Ignore errors, raw input */
   my_data->newtio.c_oflag = 0;         /* Raw output */
   my_data->newtio.c_lflag = 0;         /* No local echo */

#if defined(HAVE_OPENBSD_OS) || \
    defined(HAVE_FREEBSD_OS) || \
    defined(HAVE_NETBSD_OS) || \
    defined(HAVE_QNX_OS)
   my_data->newtio.c_ispeed = DEFAULT_SPEED;    /* Set input speed */
   my_data->newtio.c_ospeed = DEFAULT_SPEED;    /* Set output speed */
#endif   /* __openbsd__ || __freebsd__ || __netbsd__  */

   /* This makes a non.blocking read() with TIMER_READ (10) sec. timeout */
   my_data->newtio.c_cc[VMIN] = 0;
   my_data->newtio.c_cc[VTIME] = TIMER_READ * 10;

#if defined(HAVE_OSF1_OS) || \
    defined(HAVE_LINUX_OS) || defined(HAVE_DARWIN_OS)
   (void)cfsetospeed(&my_data->newtio, DEFAULT_SPEED);
   (void)cfsetispeed(&my_data->newtio, DEFAULT_SPEED);
#endif  /* do it the POSIX way */

   tcflush(ups->fd, TCIFLUSH);
   tcsetattr(ups->fd, TCSANOW, &my_data->newtio);
   tcflush(ups->fd, TCIFLUSH);

   return 1;
}

/*
 * This routine is the last one called before apcupsd
 * terminates.
 */
int apcsmart_ups_close(UPSINFO *ups)
{
   SMART_DATA *my_data = (SMART_DATA *) ups->driver_internal_data;

   if (my_data == NULL)
      return SUCCESS;              /* shouldn't happen */

   /* Reset serial line to old values */
   if (ups->fd >= 0) {
      tcflush(ups->fd, TCIFLUSH);
      tcsetattr(ups->fd, TCSANOW, &my_data->oldtio);
      tcflush(ups->fd, TCIFLUSH);

      close(ups->fd);
   }

   ups->fd = -1;
   free(ups->driver_internal_data);
   ups->driver_internal_data = NULL;

   return 1;
}

int apcsmart_ups_setup(UPSINFO *ups)
{
   int attempts;
   int rts_bit = TIOCM_RTS;
   char a = 'Y';

   if (ups->fd == -1)
      return 1;                    /* we must be a slave */

   /* Have to clear RTS line to access the serial cable mode PnP on BKPro */
   /* Shouldn't hurt on other cables, so just do it all the time. */
   ioctl(ups->fd, TIOCMBIC, &rts_bit);

   write(ups->fd, &a, 1);          /* This one might not work, if UPS is */
   sleep(1);                       /* in an unstable communication state */
   tcflush(ups->fd, TCIOFLUSH);    /* Discard UPS's response, if any */

   /*
    * Don't use smart_poll here because it may loop waiting
    * on the serial port, and here we may be called before
    * we are a deamon, so we want to error after a reasonable
    * time.
    */
   for (attempts = 0; attempts < 5; attempts++) {
      char answer[10];

      *answer = 0;
      write(ups->fd, &a, 1);       /* enter smart mode */
      getline(answer, sizeof(answer), ups);
      if (strcmp("SM", answer) == 0)
         goto out;
      sleep(1);
   }
   Error_abort0(
      "PANIC! Cannot communicate with UPS via serial port.\n"
      "Please make sure the port specified on the DEVICE directive is correct,\n"
      "and that your cable specification on the UPSCABLE directive is correct.\n");

 out:
   return 1;
}
