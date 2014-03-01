/*
 * amutex.h
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

#ifndef __AMUTEX_H
#define __AMUTEX_H

#include <pthread.h>
#include "astring.h"

// Wrappers for debug trace
#define LOCK(m)                                                  \
   do {                                                          \
      Dmsg3(500, "lock 0x%p @ %s:%d\n", &m, __FILE__, __LINE__); \
      m.lock();                                                  \
   } while(0)

#define UNLOCK(m)                                                  \
   do {                                                            \
      Dmsg3(500, "unlock 0x%p @ %s:%d\n", &m, __FILE__, __LINE__); \
      m.unlock();                                                  \
   } while(0)

class amutex
{
public:

   amutex(const char *name = DEFAULT_NAME, bool recursive = false);
   ~amutex();

   // Basic lock/unlock are inlined for efficiency
   inline void lock()   const { pthread_mutex_lock(&_mutex); }
   inline void unlock() const { pthread_mutex_unlock(&_mutex); }

protected:

   mutable pthread_mutex_t _mutex;
   astring _name;

private:

   static const char *DEFAULT_NAME;

   // Prevent use
   amutex(const amutex &rhs);
   amutex &operator=(const amutex &rhs);
};

#endif
