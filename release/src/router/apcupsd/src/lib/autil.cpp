/*
 * autil.cpp
 *
 * Common helper routines for a* classes.
 */

/*
 * Copyright (C) 2007 Adam Kropelin
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

#include "autil.h"
#include "apc.h"

void calc_abstimeout(int msec, struct timespec *abstime)
{
      // It would be best to use clock_gettime here, but it is not
      // widely available, so use gettimeofday() which we can count on
      // being available, if not quite as accurate.
      struct timeval now;
      gettimeofday(&now, NULL);
      abstime->tv_sec = now.tv_sec + msec/1000;
      abstime->tv_nsec = now.tv_usec*1000 + (msec % 1000) * 1000000;
      if (abstime->tv_nsec >= 1000000000) {
         abstime->tv_sec++;
         abstime->tv_nsec -= 1000000000;
      }
}
