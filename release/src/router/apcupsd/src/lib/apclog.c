/*
 * apclog.c
 *
 * Logging functions.
 */

/*
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 * Copyright (C) 2000-2005 Kern Sibbald <kern@sibbald.com>
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

int format_date(time_t timestamp, char *dest, size_t destlen)
{
   struct tm tm;
   localtime_r(&timestamp, &tm);

#ifdef HAVE_WIN32
   // Annoyingly, Windows does not properly implement %z (it always spells
   // out the timezone name) so we need to emulate it manually.
   int len = strftime(dest, destlen, "%Y-%m-%d %H:%M:%S ", &tm);
   tzset();
   unsigned int offset = abs(_timezone) / 60;
   len += snprintf(dest+len, destlen-len, "%c%02u%02u  ", 
      _timezone < 0 ? '+' : '-', // _timezone is UTC-local
      offset/60, offset%60);
   return len;
#else
   return strftime(dest, destlen, "%Y-%m-%d %H:%M:%S %z  ", &tm);
#endif
}

void log_event(const UPSINFO *ups, int level, const char *fmt, ...)
{
   va_list arg_ptr;
   char msg[2 *MAXSTRING];
   char datetime[100];
   int event_fd;

   event_fd = ups ? ups->event_fd : -1;

   va_start(arg_ptr, fmt);
   avsnprintf(msg, sizeof(msg), fmt, arg_ptr);
   va_end(arg_ptr);

   syslog(level, "%s", msg);       /* log the event */
   Dmsg1(100, "%s\n", msg);

   /* Write out to our temp file. LOG_INFO is DATA logging, so
    * do not write it to our temp events file. */
   if (event_fd >= 0 && level != LOG_INFO) {
      format_date(time(NULL), datetime, sizeof(datetime));
      write(event_fd, datetime, strlen(datetime));

      int lm = strlen(msg);
      if (msg[lm - 1] != '\n')
         msg[lm++] = '\n';

      write(event_fd, msg, lm);
   }
}

/*
 * Subroutine prints a debug message if the level number
 * is less than or equal the debug_level. File and line numbers
 * are included for more detail if desired, but not currently
 * printed.
 *  
 * If the level is negative, the details of file and line number
 * are not printed.
 */

int debug_level = 0;
FILE *trace_fd = NULL;
bool trace = false;

void logf(const char *fmt, ...)
{
   va_list arg_ptr;
   va_start(arg_ptr, fmt);

   if (trace) {
      if (!trace_fd) {
         char fn[200];
         asnprintf(fn, sizeof(fn), "./apcupsd.trace");
         trace_fd = fopen(fn, "a+");
      }
      if (trace_fd) {
         vfprintf(trace_fd, fmt, arg_ptr);
         fflush(trace_fd);
      } else {
         /* Some problem, turn off tracing */
         trace = false;
      }
   } else {   /* not tracing */
      vfprintf(stdout, fmt, arg_ptr);
      fflush(stdout);
   }

   va_end(arg_ptr);
}

#define FULL_LOCATION 1
void d_msg(const char *file, int line, int level, const char *fmt, ...)
{
#ifdef DEBUG
   char buf[4096];
   int i, diff;
   va_list arg_ptr;
   bool details = true;
   struct timeval now;
   const char *my_name = "apcupsd";
   static struct timeval start = {0,0};

   if (start.tv_sec == 0 && start.tv_usec == 0)
      gettimeofday(&start, NULL);

   if (level < 0) {
      details = false;
      level = -level;
   }

   if (level <= debug_level) {
#ifdef FULL_LOCATION
      if (details) {
         gettimeofday(&now, NULL);
         diff = TV_DIFF_MS(start, now);
         asnprintf(buf, sizeof(buf), "%lu.%03lu %s: %s:%d ",
            diff/1000, diff%1000, my_name, file, line);
         i = strlen(buf);
      } else {
         i = 0;
      }
#else
      i = 0;
#endif
      va_start(arg_ptr, fmt);
      avsnprintf(buf + i, sizeof(buf) - i, (char *)fmt, arg_ptr);
      va_end(arg_ptr);

      logf("%s", buf);
   }
#endif
}

void hex_dump(int level, void *data, unsigned int len)
{
   unsigned int pos = 0;
   unsigned char *dat = (unsigned char *)data;
   char temp[16*3+1];
   char temp2[8+2+16*3+1+16+1];
   char *ptr;

   if (debug_level < level)
      return;

   Dmsg2(level, "Dumping %d bytes @ 0x%08x\n", len, data);
   while (pos < len)
   {
      int num = MIN(16, len-pos);
      ptr = temp;
      for (int i=0; i < num; i++)
         ptr += sprintf(ptr, "%02x ", dat[pos+i]);

      ptr = temp2;
      ptr += sprintf(temp2, "%08x  %-48s ", pos, temp);

      for (int i=0; i < num; i++)
         ptr += sprintf(ptr, "%c", isgraph(dat[pos+i]) ? dat[pos+i] : '.');

      Dmsg1(level, "%s\n", temp2);
      pos += num;
   }
}
