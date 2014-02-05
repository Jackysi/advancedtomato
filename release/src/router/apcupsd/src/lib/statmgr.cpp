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

#include "apc.h"
#include "statmgr.h"
#include <stdarg.h>

StatMgr::StatMgr(const char *host, unsigned short port)
   : m_host(host),
     m_port(port),
     m_socket(-1)
{
   memset(m_stats, 0, sizeof(m_stats));
}

StatMgr::~StatMgr()
{
   lock();
   close();
}

bool StatMgr::Update()
{
   lock();

   int tries;
   for (tries = 2; tries; tries--)
   {
      if (m_socket == -1 && !open()) {
         // Hard failure: bail immediately
         unlock();
         return false;
      }

      if (net_send(m_socket, "status", 6) != 6) {
         // Soft failure: close and try again
         close();
         continue;
      }

      memset(m_stats, 0, sizeof(m_stats));

      int len;
      int i = 0;
      while (i < MAX_STATS &&
             (len = net_recv(m_socket, m_stats[i].data, sizeof(m_stats[i].data)-1)) > 0)
      {
         char *key, *value;

         // NUL-terminate the string
         m_stats[i].data[len] = '\0';

         // Find separator
         value = strchr(m_stats[i].data, ':');

         // Trim whitespace from value
         if (value) {
            *value++ = '\0';
            value = trim(value);
         }

         // Trim whitespace from key;
         key = trim(m_stats[i].data);

         m_stats[i].key = key;
         m_stats[i].value = value;
         i++;
      }

      // Good update, bail now
      if (i > 0 && len == 0)
         break;

      // Soft failure: close and try again
      close();
   }

   unlock();
   return tries > 0;
}

astring StatMgr::Get(const char* key)
{
   astring ret;

   lock();
   for (int idx=0; idx < MAX_STATS && m_stats[idx].key; idx++) {
      if (strcmp(key, m_stats[idx].key) == 0) {
         if (m_stats[idx].value)
            ret = m_stats[idx].value;
         break;
      }
   }
   unlock();

   return ret;
}

bool StatMgr::GetAll(alist<astring> &keys, alist<astring> &values)
{
   keys.clear();
   values.clear();

   lock();
   for (int idx=0; idx < MAX_STATS && m_stats[idx].key; idx++)
   {
      keys.append(m_stats[idx].key);
      values.append(m_stats[idx].value);
   }
   unlock();

   return true;
}

bool StatMgr::GetEvents(alist<astring> &events)
{
   lock();

   if (m_socket == -1 && !open()) {
      unlock();
      return false;
   }

   if (net_send(m_socket, "events", 6) != 6) {
      close();
      unlock();
      return false;
   }

   events.clear();

   char temp[1024];
   int len;
   while ((len = net_recv(m_socket, temp, sizeof(temp)-1)) > 0)
   {
      temp[len] = '\0';
      rtrim(temp);
      events.append(temp);
   }

   if (len == -1)
      close();

   unlock();
   return true;
}

char *StatMgr::ltrim(char *str)
{
   while(isspace(*str))
      *str++ = '\0';

   return str;
}

void StatMgr::rtrim(char *str)
{
   char *tmp = str + strlen(str) - 1;

   while (tmp >= str && isspace(*tmp))
      *tmp-- = '\0';
}

char *StatMgr::trim(char *str)
{
   str = ltrim(str);
   rtrim(str);
   return str;
}

void StatMgr::lock()
{
   m_mutex.lock();
}

void StatMgr::unlock()
{
   m_mutex.unlock();
}

bool StatMgr::open()
{
   if (m_socket != -1)
      close();

   m_socket = net_open(m_host, NULL, m_port);
   return m_socket != -1;
}

void StatMgr::close()
{
   if (m_socket != -1) {
      net_close(m_socket);
      m_socket = -1;
   }
}

bool StatMgr::GetSummary(int &battstat, astring &statstr, astring &upsname)
{
   // Fetch data from the UPS
   if (!Update()) {
      battstat = -1;
      statstr = "NETWORK ERROR";
      return false;
   }

   // Lookup the STATFLAG key
   astring statflag = Get("STATFLAG");
   if (statflag.empty()) {
      battstat = -1;
      statstr = "ERROR";
      return false;
   }
   unsigned long status = strtoul(statflag, NULL, 0);

   // Lookup BCHARGE key
   astring bcharge = Get("BCHARGE");

   // Determine battery charge percent
   if (status & UPS_onbatt)
      battstat = 0;
   else if (!bcharge.empty())
      battstat = (int)atof(bcharge);
   else
      battstat = 100;

   // Fetch UPSNAME
   astring uname = Get("UPSNAME");
   if (!uname.empty())
      upsname = uname;

   // Now output status in human readable form
   statstr = "";
   if (status & UPS_calibration)
      statstr += "CAL ";
   if (status & UPS_trim)
      statstr += "TRIM ";
   if (status & UPS_boost)
      statstr += "BOOST ";
   if (status & UPS_online)
      statstr += "ONLINE ";
   if (status & UPS_onbatt)
      statstr += "ONBATT ";
   if (status & UPS_overload)
      statstr += "OVERLOAD ";
   if (status & UPS_battlow)
      statstr += "LOWBATT ";
   if (status & UPS_replacebatt)
      statstr += "REPLACEBATT ";
   if (!(status & UPS_battpresent))
      statstr += "NOBATT ";

   // This overrides the above
   if (status & UPS_commlost) {
      statstr = "COMMLOST";
      battstat = -1;
   }

   // This overrides the above
   if (status & UPS_shutdown)
      statstr = "SHUTTING DOWN";

   // This overrides the above
   if (status & UPS_onbatt) {
      astring reason = Get("LASTXFER");
      if (strstr(reason, "self test"))
         statstr = "SELFTEST";
   }

   // Remove trailing space, if present
   statstr.rtrim();
   return true;
}
