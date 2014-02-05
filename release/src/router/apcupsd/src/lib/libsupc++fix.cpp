/*
 * libsupc++fix.cpp
 *
 * Workaround for broken libsupc++ on FreeBSD 5.x and early 6.x.
 * FreeBSD 5.x and early 6.x inadvertantly left out some critical
 * files from libsupc++.a. See FreeBSD PR 99702:
 * <http://www.freebsd.org/cgi/query-pr.cgi?pr=99702>
 *
 * This file is a hack-around I came up with that seems to work.
 */

/*
 * Copyright (C) 2010 Adam Kropelin
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

#ifdef FREEBSD_SUPCPP_FIX

#include <new>
#include <stdlib.h>

void* operator new[] (std::size_t size)
{
   return malloc(size); 
}

void* operator new (std::size_t size)
{
   return malloc(size); 
}

void operator delete (void *p)
{
   free(p); 
}

void operator delete[] (void *p)
{
   free(p); 
}

namespace __cxxabiv1
{
   std::terminate_handler __terminate_handler;
   std::unexpected_handler __unexpected_handler;
};

#endif
