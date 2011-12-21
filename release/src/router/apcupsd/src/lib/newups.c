/*
 * newups.c 
 *
 * Create UPS structure and do locking/unlocking on it
 *
 */

/*
 * Copyright (C) 2000-2006 Kern Sibbald
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
 * NOTE!  P() locks a mutex, and V() unlocks it.  Both abort if there
 *   is a mutex error. So testing return codes is not necessary.
 */

UPSINFO *new_ups()
{
   int stat;
   UPSINFO *ups;

   ups = (UPSINFO *) malloc(sizeof(UPSINFO));
   if (!ups)
      Error_abort0("Could not allocate ups memory\n");

   memset(ups, 0, sizeof(UPSINFO));
   if ((stat = pthread_mutex_init(&ups->mutex, NULL)) != 0) {
      Error_abort1("Could not create pthread mutex. ERR=%s\n", strerror(stat));
      free(ups);
      return NULL;
   }

   /* Most drivers do not support this, so preset it to true */
   ups->set_battpresent();

   ups->refcnt = 1;
   return ups;
}

/* Attach to ups structure or create a new one */
UPSINFO *attach_ups(UPSINFO *ups)
{
   if (!ups)
      return new_ups();

   P(ups->mutex);
   ups->refcnt++;
   V(ups->mutex);

   return ups;
}

void detach_ups(UPSINFO *ups)
{
   P(ups->mutex);
   ups->refcnt--;
   if (ups->refcnt == 0) {
      destroy_ups(ups);
      return; // no unlock since mutex has been destroyed
   }
   V(ups->mutex);
}

void destroy_ups(UPSINFO *ups)
{
   pthread_mutex_destroy(&ups->mutex);
   if (ups->refcnt == 0) {
      free(ups);
   }
}

void _read_lock(const char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "read_lock at %s:%d\n", file, line);
   P(ups->mutex);
}

void _read_unlock(const char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "read_unlock at %s:%d\n", file, line);
   V(ups->mutex);
}

void _write_lock(const char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "write_lock at %s:%d\n", file, line);
   P(ups->mutex);
}

void _write_unlock(const char *file, int line, UPSINFO *ups)
{
   Dmsg2(100, "write_unlock at %s:%d\n", file, line);
   V(ups->mutex);
}
