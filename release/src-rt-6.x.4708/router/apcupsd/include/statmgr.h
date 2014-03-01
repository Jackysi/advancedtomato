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

#ifndef STATMGR_H
#define STATMGR_H

#include "apc.h"
#include "amutex.h"
#include "alist.h"
#include "astring.h"

#define MAX_STATS 256
#define MAX_DATA  100


class StatMgr
{
public:

   StatMgr(const char *host, unsigned short port);
   ~StatMgr();

   bool Update();
   astring Get(const char* key);
   bool GetAll(alist<astring> &keys, alist<astring> &values);
   bool GetEvents(alist<astring> &events);
   bool GetSummary(int &battstat, astring &statstr, astring &upsname);

private:

   bool open();
   void close();

   char *ltrim(char *str);
   void rtrim(char *str);
   char *trim(char *str);

   void lock();
   void unlock();

   struct keyval {
      const char *key;
      const char *value;
      char data[MAX_DATA];
   };

   keyval          m_stats[MAX_STATS];
   astring         m_host;
   unsigned short  m_port;
   int             m_socket;
   amutex          m_mutex;
};

#endif // STATMGR_H
