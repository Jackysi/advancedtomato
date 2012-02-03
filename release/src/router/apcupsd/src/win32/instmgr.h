/*
 * Copyright (C) 2009 Adam Kropelin
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

#include "astring.h"
#include "alist.h"
#include <windows.h>

#ifndef __INSTMGR_H
#define __INSTMGR_H

#include "balloonmgr.h"

class upsMenu;

struct MonitorConfig
{
   astring id;
   astring host;
   DWORD port;
   DWORD refresh;
   DWORD popups;
};

class InstanceManager
{
public:
   InstanceManager(HINSTANCE appinst);
   ~InstanceManager();

   void CreateMonitors();
   int RemoveInstance(const char *id);
   void AddInstance();
   void UpdateInstance(const MonitorConfig &mcfg);
   void RemoveAll();
   void ResetInstances();
   bool IsAutoStart();
   void SetAutoStart(bool start);

private:

   struct InstanceConfig
   {
      InstanceConfig() : menu(NULL), order(0) {}
      ~InstanceConfig() {}

      MonitorConfig mcfg;
      DWORD order;
      upsMenu *menu;
   };

   void Write();
   InstanceConfig ReadConfig(HKEY key, const char *id);
   alist<InstanceConfig>::iterator FindInstance(const char *id);
   astring CreateId();

   bool RegQueryDWORD(HKEY hkey, const char *name, DWORD &result);
   bool RegQueryString(HKEY key, const char *name, astring &result);
   void RegSetDWORD(HKEY key, const char *name, DWORD value);
   void RegSetString(HKEY key, const char *name, const char *value);

   static const char *INSTANCES_KEY;
   static const MonitorConfig DEFAULT_CONFIG;

   alist<InstanceConfig> _instances;
   HINSTANCE _appinst;
   BalloonMgr _balmgr;
};

#endif
