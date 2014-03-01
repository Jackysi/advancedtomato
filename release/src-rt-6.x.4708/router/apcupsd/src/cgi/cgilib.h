/* cgilib.h - headers for cgilib.c

   Copyright (C) 1999  Russell Kroll <rkroll@exploits.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              
*/

/* other programs that link to this should provide parsearg() ... */
void parsearg (const char *var, const char *value);

/* actually extract the values from QUERY_STRING */
int extractcgiargs(void);

/* see if a host is allowed per the hosts.conf */
int checkhost(const char *check);

/*
 * Output a string taking care to assure that any html meta characters
 * are output properly.
 */
void html_puts(const char *s);

/*
 * Print the standard http header, html header which is common to all
 * html pages.
 */
void html_begin(const char *title, int refresh);

/*
 * Print the standard footer which appears on every html page and close out
 * the html document.
 */
void html_finish(void);
