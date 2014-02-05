/*
 * apclock.c
 *
 * Lock file managing functions.
 */

/*
 * Copyright (C) 2000-2006 Kern Sibbald
 * Copyright (C) 1998-99 Brian Schau <bsc@fleggaard.dk>
 * Copyright (C) 1998-99-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
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

/* 
 * For Windows, we do NOT create a lock file. Even if we did
 * no other program would respect it since this is not a
 * Unix system.
 */

/* Check to see if a serial port lockfile exists.
 * If so, and the process is no longer running,
 * blow away the lockfile.  
 */
#if  !defined(HAVE_WIN32)
static int check_stale_lockfile(UPSINFO *ups)
{
   char pidbuffer[12];
   int size;
   int stalepid;
   int error;

   /*
    * If the daemon is talking with APC_NET, the lock file is not
    * needed.
    */
   if (ups->cable.type == APC_NET)
      return LCKNOLOCK;

   if (ups->lockpath[0] == '\0') {
      /*
       * If there's no lockfile configured, return error.
       * This is a _must_. See my comment in apcconfig.c
       */
      log_event(ups, LOG_ERR, "No lock path configured.\n");
      return LCKERROR;
   }

   errno = 0;
   if ((ups->lockfile = open(ups->lockpath, O_RDONLY)) < 0) {
      /*
       * Cannot open the file (may be it doesn't exist and that's okay
       * for us so return success).
       */
      if (errno == ENOENT)
         return LCKSUCCESS;

      /* On any other error, return error. */
      log_event(ups, LOG_ERR, "Lock file open error. ERR=%s\n", strerror(errno));
      return LCKERROR;
   }

   if ((size = read(ups->lockfile, &pidbuffer, 11)) == -1) {
      /*
       * If we can not read from file, close it and return error:
       * the file exist but we can not check for stale.
       */
      error = LCKERROR;
      log_event(ups, LOG_ERR, "Lock file read error. ERR=%s\n", strerror(errno));
      goto out;
   }

   if (size == 0 || (sscanf(pidbuffer, "%d", &stalepid) != 1)) {
      /*
       * If there's no data in the file or the data written is wrong
       * we have a process that:
       * 1 - running but failed to write the lock file
       * 2 - not running and failed to write the lock file
       *
       * Anyway we assume the worst case (1) and return error.
       */
      error = LCKERROR;
      log_event(ups, LOG_ERR, "Lock file data error: %s\n", pidbuffer);
      goto out;
   }

   /* Check if it is our current pid or the pid of our parent */
   if (stalepid == getpid() || stalepid == getppid()) {
      /*
       * We are us (may be a crash of the machine ... same pid
       * because same boot sequence ... leave it alone and go run)
       */
      error = LCKEXIST;
      goto out;
   }

   /*
    * Okay, now we have a stalepid to check.
    * kill(pid,0) checks to see if the process is still running.
    */
   if (kill(stalepid, 0) == -1 && errno == ESRCH) {
      /*
       * Okay this is a stale lock:
       * we can unlink even before closing it.
       */
      if (unlink(ups->lockpath) < 0) {
         log_event(ups, LOG_ERR,
            "Unable to unlink old lock file %s because %s\n",
            ups->lockpath, strerror(errno));
         error = LCKERROR;
      } else {
         error = LCKSUCCESS;
      }
      goto out;
   }

   /*
    * We have unfortunately found a perfectly valid lock file.
    * Don't touch it.
    */
   log_event(ups, LOG_ERR, "Valid lock file for pid=%d, but not ours pid=%d\n",
      stalepid, getpid());

   error = LCKERROR;

 out:
   close(ups->lockfile);
   ups->lockfile = -1;

   return error;
}
#endif

/* 
 * Create serial port lock file   
 */
int hibernate_ups = FALSE;
int shutdown_ups = FALSE;
int create_lockfile(UPSINFO *ups)
{
#if !defined(HAVE_WIN32)
   char pidbuffer[12];
   int error;

   /*
    * If this is a hibernate or shutdown execution, we are
    * probably running with the filesystems read-only, so
    * don't try to create the lock file. 
    */
   if (hibernate_ups || shutdown_ups)
      return LCKSUCCESS;

   switch (error = check_stale_lockfile(ups)) {
   case LCKNOLOCK:
      /* Lockfile not needed: return success. */
   case LCKEXIST:
      /* Lockfile exists and contains our pid. */
      return LCKSUCCESS;

   case LCKERROR:
      /* Lockfile exists and is not stale. */
      return LCKERROR;

   case LCKSUCCESS:
      /* Lockfile does not exist _now_. */
      break;
   }


   /*
    * Now the file does not exist any more.
    * Open it for creation and don't accept any kind of error.
    */
   errno = 0;
   if ((ups->lockfile = open(ups->lockpath, O_CREAT | O_EXCL | O_RDWR, 0644)) < 0) {
      /*
       * Okay there is some problem with the lock path or
       * something like that.
       */
      log_event(ups, LOG_ERR,
         "Cannot create %s serial port lock file: ERR=%s\n",
         ups->lockpath, strerror(errno));
      return LCKERROR;
   }

   if (asnprintf(pidbuffer, sizeof(pidbuffer), "%010ld", (long)getpid()) <= 0) {
      /* Problems with sprintf */
      error = LCKERROR;
      log_event(ups, LOG_ERR, "Lock file sprintf error.\n");
      goto out;
   }

   if (write(ups->lockfile, pidbuffer,
         strlen(pidbuffer) + 1) != (int)strlen(pidbuffer) + 1) {
      /* Problems with write. */
      error = LCKERROR;
      log_event(ups, LOG_ERR, "Lock file %s write failure. ERR=%s\n",
         ups->lockpath, strerror(errno));
      goto out;
   }

   /* Done it. */
   error = LCKSUCCESS;

 out:

   close(ups->lockfile);
   ups->lockfile = -1;
   return error;

#else
   return LCKSUCCESS;
#endif
}

void delete_lockfile(UPSINFO *ups)
{
#if !defined(HAVE_WIN32)
   if (ups->lockpath[0] != '\0') {
      /*
       * If lockfile is ours, close it and delete it,
       * otherwise do nothing.
       */
      if (check_stale_lockfile(ups) == LCKEXIST) {
         if (ups->lockfile != -1) {
            close(ups->lockfile);
            ups->lockfile = -1;
         }
         unlink(ups->lockpath);
      }
      /*
       * Now ups->lockfile is == -1 so there's no need to
       * blank ups->lockfile too.
       */
   }
#endif
}
