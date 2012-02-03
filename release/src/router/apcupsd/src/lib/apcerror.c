/*
 * apcerror.c
 *
 * Error functions.
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
 * Subroutine normally called by macro error_abort() to print
 * FATAL ERROR message and supplied error message
 */
void generic_error_out(const char *file, int line, const char *fmt, ...)
{
   char buf[256];
   va_list arg_ptr;
   int i;

   asnprintf(buf, sizeof(buf), "FATAL ERROR in %s at line %d\n", file, line);
   i = strlen(buf);
   va_start(arg_ptr, fmt);
   avsnprintf((char *)&buf[i], sizeof(buf) - i, (char *)fmt, arg_ptr);
   va_end(arg_ptr);
   fprintf(stdout, "%s", buf);

   if (error_cleanup)
      error_cleanup();

   exit(1);
}

/* simply print the message and exit */
void generic_error_exit(const char *fmt, ...)
{
   va_list arg_ptr;
   char buf[256];

   va_start(arg_ptr, fmt);
   avsnprintf(buf, sizeof(buf), (char *)fmt, arg_ptr);
   va_end(arg_ptr);
   fprintf(stdout, "%s", buf);

   if (error_cleanup)
      error_cleanup();

   exit(1);
}

void (*error_out) (const char *file, int line, const char *fmt, ...) = generic_error_out;
void (*error_exit) (const char *fmt, ...) = generic_error_exit;
void (*error_cleanup) (void) = NULL;
