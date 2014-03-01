/*
 * apcfile.c
 *
 * Files management.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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
 * If this is a Windows machine, we do NOT create a pid file.
 * We prevent multiple copies of apcupsd from running by 
 * ensuring that there is only one system tray entry for
 * apcupsd.
 */

/*
 * Create a file in `path'.  Used for nologin and powerfail
 * files.
 */
int make_file(UPSINFO *ups, const char *path)
{
   int makefd;

   if ((makefd = open(path, O_CREAT | O_WRONLY, 0644)) >= 0) {
      close(makefd);
   } else {
      log_event(ups, LOG_ERR, "Unable to create %s: ERR=%s\n",
         path, strerror(errno));
   }

   return makefd;
}

/* Create the pid lock file. */
const char *pidfile = APCPID;
void make_pid_file(void)
{
#if !defined(HAVE_WIN32)
   pid_t pid = getpid();
   int pfd, len;
   char buf[100];

   unlink(pidfile);
   if ((pfd = open(pidfile, O_CREAT | O_TRUNC | O_WRONLY, 0644)) >= 0) {
      len = asnprintf(buf, sizeof(buf), "%ld\n", (long)pid);
      write(pfd, buf, len);
      close(pfd);
   }
#endif
}
