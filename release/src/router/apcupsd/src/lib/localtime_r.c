/*
 * localtime_r.c
 *
 * Implementation of reentrant localtime(), for platforms don't
 * already have it.
 */

/*
 * Copyright (C) 1999-2005 Kern Sibbald <kern@sibbald.com>
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

#if !defined(HAVE_LOCALTIME_R)

struct tm *localtime_r(const time_t *timep, struct tm *tm)
{
   static pthread_mutex_t mutex;
   static int first = 1;
   struct tm *ltm;

   if (first) {
      pthread_mutex_init(&mutex, NULL);
      first = 0;
   }

   P(mutex);

   ltm = localtime(timep);
   if (ltm)
      memcpy(tm, ltm, sizeof(struct tm));

   V(mutex);

   return ltm ? tm : NULL;
}

#endif   /* !defined(HAVE_LOCALTIME_R) */
