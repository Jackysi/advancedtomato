/*
 * sleep.c
 *
 * Implementations of sleep-related functions for platforms that
 * do not already have them.
 */

/*
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

#ifndef HAVE_NANOSLEEP
/*
 * This is a close approximation of nanosleep() for platforms that
 * do not have it.
 */
int nanosleep(const struct timespec *req, struct timespec *rem)
{
   static pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;
   static pthread_cond_t timer = PTHREAD_COND_INITIALIZER;
   struct timespec timeout;
   int stat;

   struct timeval tv;

   /* Copy relative exit time */
   timeout = *req;

   /* Compute absolute exit time */
   gettimeofday(&tv, NULL);
   timeout.tv_nsec += tv.tv_usec * 1000;
   timeout.tv_sec += tv.tv_sec;
   while (timeout.tv_nsec >= 1000000000) {
      timeout.tv_nsec -= 1000000000;
      timeout.tv_sec++;
   }

   Dmsg1(200, "pthread_cond_timedwait sec=%d\n", timeout.tv_sec);

   /* Mutex is unlocked during the timedwait */
   P(timer_mutex);

   stat = pthread_cond_timedwait(&timer, &timer_mutex, &timeout);
   Dmsg1(200, "pthread_cond_timedwait stat=%d\n", stat);

   V(timer_mutex);

   /* Assume no time leftover */
   if (rem) {
      rem->tv_nsec = 0;
      rem->tv_sec = 0;
   }

   return 0;
}
#endif /* HAVE_NANOSLEEP */
