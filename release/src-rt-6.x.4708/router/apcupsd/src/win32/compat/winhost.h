/*
 * Define Host machine
 *
 *  Version $Id: winhost.h,v 1.2 2006/04/28 16:34:53 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#ifdef HAVE_MINGW

#define HOST_OS  "Linux"
#define DISTNAME "Cross-compile"
#define DISTVER  "1"

#else
extern char WIN_VERSION_LONG[];
extern char WIN_VERSION[];

#define HOST_OS  WIN_VERSION_LONG
#define DISTNAME "MVS"
#define DISTVER  WIN_VERSION

#endif
