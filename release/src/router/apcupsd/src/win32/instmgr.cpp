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

#include "instmgr.h"
#include "wintray.h"
#include <stdio.h>

const char *InstanceManager::INSTANCES_KEY =
   "Software\\Apcupsd\\Apctray\\instances";

const MonitorConfig InstanceManager::DEFAULT_CONFIG =
{
   "",            // id
   "127.0.0.1",   // host
   3551,          // port
   5,             // refresh
   true           // popups
};

InstanceManager::InstanceManager(HINSTANCE appinst) :
   _appinst(appinst)
{
}

InstanceManager::~InstanceManager()
{
   // Destroy all instances
   while (!_instances.empty())
   {
      alist<InstanceConfig>::iterator inst = _instances.begin();
      inst->menu->Destroy();
      delete inst->menu;
      _instances.remove(inst);
   }
}

void InstanceManager::CreateMonitors()
{
   alist<InstanceConfig> unsorted;
   InstanceConfig config;

   // Open registry key for instance configs
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, INSTANCES_KEY, 0, KEY_READ, &apctray)
         == ERROR_SUCCESS)
   {
      // Iterate though all instance keys, reading the config for each 
      // instance into a list.
      int i = 0;
      char name[128];
      DWORD len = sizeof(name);
      while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS)
      {
         unsorted.append(ReadConfig(apctray, name));
         len = sizeof(name);
      }
      RegCloseKey(apctray);

      // Now that we've read all instance configs, place them in a sorted list.
      // This is a really inefficient algorithm, but even O(N^2) is tolerable
      // for small N.
      alist<InstanceConfig>::iterator iter, lowest;
      while (!unsorted.empty())
      {
         lowest = unsorted.begin();
         for (iter = unsorted.begin(); iter != unsorted.end(); ++iter)
         {
            if (iter->order < lowest->order)
               lowest = iter;
         }

         _instances.append(*lowest);
         unsorted.remove(lowest);
      }
   }

   // If no instances were found, create a default one
   if (_instances.empty())
   {
      InstanceConfig config;
      config.mcfg = DEFAULT_CONFIG;
      config.mcfg.id = CreateId();
      _instances.append(config);
      Write();
   }

   // Loop thru sorted instance list and create an upsMenu for each
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
      iter->menu = new upsMenu(_appinst, iter->mcfg, &_balmgr, this);
}

InstanceManager::InstanceConfig InstanceManager::ReadConfig(HKEY key, const char *id)
{
   InstanceConfig config;
   config.mcfg = DEFAULT_CONFIG;
   config.mcfg.id = id;

   // Read instance config from registry
   HKEY subkey;
   if (RegOpenKeyEx(key, id, 0, KEY_READ, &subkey) == ERROR_SUCCESS)
   {
      RegQueryString(subkey, "host", config.mcfg.host);
      RegQueryDWORD(subkey, "port", config.mcfg.port);
      RegQueryDWORD(subkey, "refresh", config.mcfg.refresh);
      RegQueryDWORD(subkey, "popups", config.mcfg.popups);
      RegQueryDWORD(subkey, "order", config.order);
      RegCloseKey(subkey);
   }

   return config;
}

void InstanceManager::Write()
{
   // Remove all existing instances from the registry
   HKEY apctray;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, INSTANCES_KEY,
                    0, KEY_READ|KEY_WRITE, &apctray) == ERROR_SUCCESS)
   {
      // Iterate though all apctray keys locating instance keys of the form
      // "instanceN". Add the id of each key to a list. We don't want to
      // remove keys while we're enumerating them since Win32 docs say to not
      // change the key while enumerating it.
      int i = 0;
      char name[128];
      DWORD len = sizeof(name);
      alist<astring> ids;
      while (RegEnumKeyEx(apctray, i++, name, &len, NULL, NULL,
                          NULL, NULL) == ERROR_SUCCESS)
      {
         ids.append(name);
         len = sizeof(name);
      }

      // Now that we have a list of all ids, loop thru and delete each one
      alist<astring>::iterator iter;
      for (iter = ids.begin(); iter != ids.end(); ++iter)
         RegDeleteKey(apctray, *iter);
   }
   else
   {
      // No apctray\instances key (and therefore no instances) yet
      if (RegCreateKey(HKEY_LOCAL_MACHINE, INSTANCES_KEY, &apctray) 
            != ERROR_SUCCESS)
         return;
   }

   // Write out instances. Our instance list is in sorted order but may
   // have gaps in the numbering, so regenerate 'order' value, ignoring what's
   // in the InstanceConfig.order field.
   int count = 0;
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
   {
      // Create the instance and populate it
      HKEY instkey;
      if (RegCreateKey(apctray, iter->mcfg.id, &instkey) == ERROR_SUCCESS)
      {
         RegSetString(instkey, "host", iter->mcfg.host);
         RegSetDWORD(instkey, "port", iter->mcfg.port);
         RegSetDWORD(instkey, "refresh", iter->mcfg.refresh);
         RegSetDWORD(instkey, "popups", iter->mcfg.popups);
         RegSetDWORD(instkey, "order", count);
         RegCloseKey(instkey);
         count++;
      }
   }
   RegCloseKey(apctray);
}

alist<InstanceManager::InstanceConfig>::iterator 
   InstanceManager::FindInstance(const char *id)
{
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
   {
      if (iter->mcfg.id == id)
         return iter;
   }
   return _instances.end();
}

int InstanceManager::RemoveInstance(const char *id)
{
   alist<InstanceConfig>::iterator inst = FindInstance(id);
   if (inst != _instances.end())
   {
      inst->menu->Destroy();
      delete inst->menu;
      _instances.remove(inst);
      Write();
   }

   return _instances.size();
}

void InstanceManager::AddInstance()
{
   InstanceConfig config;
   config.mcfg = DEFAULT_CONFIG;
   config.mcfg.id = CreateId();
   config.menu = new upsMenu(_appinst, config.mcfg, &_balmgr, this);
   _instances.append(config);
   Write();
}

void InstanceManager::UpdateInstance(const MonitorConfig &mcfg)
{
   alist<InstanceConfig>::iterator inst = FindInstance(mcfg.id);
   if (inst != _instances.end())
   {
      inst->mcfg = mcfg;
      inst->menu->Reconfigure(mcfg);
      Write();
   }
}

void InstanceManager::RemoveAll()
{
   while (!_instances.empty())
      RemoveInstance(_instances.begin()->mcfg.id);
}

void InstanceManager::ResetInstances()
{
   alist<InstanceConfig>::iterator iter;
   for (iter = _instances.begin(); iter != _instances.end(); ++iter)
      iter->menu->Redraw();
}

astring InstanceManager::CreateId()
{
   // Create binary UUID
   UUID uuid;
   UuidCreate(&uuid);

   // Convert binary UUID to RPC string
   unsigned char *tmpstr;
   UuidToString(&uuid, &tmpstr);   

   // Copy string UUID to astring and free RPC version
   astring uuidstr((char*)tmpstr);
   RpcStringFree(&tmpstr);

   return uuidstr;
}

bool InstanceManager::IsAutoStart()
{
   HKEY hkey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    0, KEY_READ, &hkey) != ERROR_SUCCESS)
      return false;

   astring path;
   RegQueryString(hkey, "Apctray", path);
   RegCloseKey(hkey);

   return !path.empty();
}

void InstanceManager::SetAutoStart(bool start)
{
   HKEY runkey;
   if (start)
   {
      // Get the full path/filename of this executable
      char path[1024];
      GetModuleFileName(NULL, path, sizeof(path));

      // Add double quotes
      char cmd[1024];
      snprintf(cmd, sizeof(cmd), "\"%s\"", path);

      // Open registry key for auto-run programs
      if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                       "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       &runkey) != ERROR_SUCCESS)
      {
         return;
      }

      // Add apctray key
      RegSetString(runkey, "Apctray", cmd);
   }
   else
   {
      // Open registry key apctray
      HKEY runkey;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                       "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                       0, KEY_READ|KEY_WRITE, &runkey) == ERROR_SUCCESS)
      {
         RegDeleteValue(runkey, "Apctray");
      }
   }

   RegCloseKey(runkey);
}

bool InstanceManager::RegQueryDWORD(HKEY hkey, const char *name, DWORD &result)
{
   DWORD data;
   DWORD len = sizeof(data);
   DWORD type;

   // Retrieve DWORD
   if (RegQueryValueEx(hkey, name, NULL, &type, (BYTE*)&data, &len)
         == ERROR_SUCCESS && type == REG_DWORD)
   {
      result = data;
      return true;
   }

   return false;
}

bool InstanceManager::RegQueryString(HKEY hkey, const char *name, astring &result)
{
   char data[512]; // Arbitrary max
   DWORD len = sizeof(data);
   DWORD type;

   // Retrieve string
   if (RegQueryValueEx(hkey, name, NULL, &type, (BYTE*)data, &len)
         == ERROR_SUCCESS && type == REG_SZ)
   {
      result = data;
      return true;
   }

   return false;
}

void InstanceManager::RegSetDWORD(HKEY hkey, const char *name, DWORD value)
{
   RegSetValueEx(hkey, name, 0, REG_DWORD, (BYTE*)&value, sizeof(value));
}

void InstanceManager::RegSetString(HKEY hkey, const char *name, const char *value)
{
   RegSetValueEx(hkey, name, 0, REG_SZ, (BYTE*)value, strlen(value)+1);
}
