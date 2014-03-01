/*
 * asys.c
 *
 * Miscellaneous apcupsd memory and thread safe routines
 * Generally, these are interfaces to system or standard
 * library routines. 
 *
 * Adapted from Bacula source code
 */

/*
 * Copyright (C) 2004 Kern Sibbald
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

/* Guarantee that the string is properly terminated */
char *astrncpy(char *dest, const char *src, int maxlen)
{
   strncpy(dest, src, maxlen - 1);
   dest[maxlen - 1] = 0;
   return dest;
}

char *astrncat(char *dest, const char *src, int maxlen)
{
   strncat(dest, src, maxlen - 1);
   dest[maxlen - 1] = 0;
   return dest;
}

#ifndef DEBUG
void *amalloc(size_t size)
{
   void *buf;

   buf = malloc(size);
   if (buf == NULL)
      Error_abort1("Out of memory: ERR=%s\n", strerror(errno));

   return buf;
}
#endif

void *arealloc(void *buf, size_t size)
{
   buf = realloc(buf, size);
   if (buf == NULL)
      Error_abort1("Out of memory: ERR=%s\n", strerror(errno));

   return buf;
}


void *acalloc(size_t size1, size_t size2)
{
   void *buf;

   buf = calloc(size1, size2);
   if (buf == NULL)
      Error_abort1("Out of memory: ERR=%s\n", strerror(errno));

   return buf;
}


#define BIG_BUF 5000

/* Implement snprintf */
int asnprintf(char *str, size_t size, const char *fmt, ...)
{
#ifdef HAVE_VSNPRINTF
   va_list arg_ptr;
   int len;

   va_start(arg_ptr, fmt);
   len = vsnprintf(str, size, fmt, arg_ptr);
   va_end(arg_ptr);

   str[size - 1] = 0;
   return len;

#else

   va_list arg_ptr;
   int len;
   char *buf;

   buf = (char *)malloc(BIG_BUF);

   va_start(arg_ptr, fmt);
   len = vsprintf(buf, fmt, arg_ptr);
   va_end(arg_ptr);

   if (len >= BIG_BUF)
      Error_abort0("Buffer overflow.\n");

   memcpy(str, buf, size);
   str[size - 1] = 0;

   free(buf);
   return len;
#endif
}

/* Implement vsnprintf() */
int avsnprintf(char *str, size_t size, const char *format, va_list ap)
{
#ifdef HAVE_VSNPRINTF
   int len;

   len = vsnprintf(str, size, format, ap);
   str[size - 1] = 0;

   return len;

#else

   int len;
   char *buf;

   buf = (char *)malloc(BIG_BUF);

   len = vsprintf(buf, format, ap);
   if (len >= BIG_BUF)
      Error_abort0("Buffer overflow.\n");

   memcpy(str, buf, size);
   str[size - 1] = 0;

   free(buf);
   return len;
#endif
}

#ifndef HAVE_LOCALTIME_R

struct tm *localtime_r(const time_t *timep, struct tm *tm)
{
   struct tm *ltm;

   static pthread_mutex_t mutex;
   static int first = 1;

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
#endif   /* HAVE_LOCALTIME_R */

/*
 * These are mutex routines that do error checking
 * for deadlock and such.  Normally not turned on.
 */
#ifdef DEBUG_MUTEX

void _p(char *file, int line, pthread_mutex_t *m)
{
   int errstat;

   if ((errstat = pthread_mutex_trylock(m))) {
      e_msg(file, line, M_ERROR, 0, "Possible mutex deadlock.\n");

      /* We didn't get the lock, so do it definitely now */
      if ((errstat = pthread_mutex_lock(m))) {
         e_msg(file, line, M_ABORT, 0,
            "Mutex lock failure. ERR=%s\n", strerror(errstat));
      } else {
         e_msg(file, line, M_ERROR, 0,
            "Possible mutex deadlock resolved.\n");
      }
   }
}

void _v(char *file, int line, pthread_mutex_t *m)
{
   int errstat;

   if ((errstat = pthread_mutex_trylock(m)) == 0) {
      e_msg(file, line, M_ERROR, 0,
         "Mutex unlock not locked. ERR=%s\n", strerror(errstat));
   }
   if ((errstat = pthread_mutex_unlock(m))) {
      e_msg(file, line, M_ABORT, 0,
         "Mutex unlock failure. ERR=%s\n", strerror(errstat));
   }
}
#endif   /* DEBUG_MUTEX */


