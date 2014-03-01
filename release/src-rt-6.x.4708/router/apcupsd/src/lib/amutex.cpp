/*
 * amutex.cpp
 *
 * Simple mutex wrapper class.
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

#include "amutex.h"
#include "autil.h"

const char *amutex::DEFAULT_NAME = "unnamed_mutex";

amutex::amutex(const char *name, bool recursive)
   : _name(name)
{
   pthread_mutexattr_t attr;
   pthread_mutexattr_init(&attr);
   pthread_mutexattr_settype(&attr,
      recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_NORMAL);
   pthread_mutex_init(&_mutex, &attr);
   pthread_mutexattr_destroy(&attr);
}

amutex::~amutex()
{
   pthread_mutex_destroy(&_mutex);
}
