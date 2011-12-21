/* upsfetch.h - prototypes for important functions used when linking

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

#ifndef __UPSFETCH_H
#define __UPSFETCH_H

#include <sys/types.h>

extern char statbuf[4096];
extern size_t statlen;
extern char errmsg[200];

/* Read data into memory buffer to be used by getupsvar() */
int fetch_events (const char *host);

/* get <varname> from <host> and put the reply in <buf> */
int getupsvar (const char *host, const char *varname, char *buf, size_t buflen);

#endif
