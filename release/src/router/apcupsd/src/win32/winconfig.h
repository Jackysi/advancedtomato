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

#ifndef __WINCONFIG_H
#define __WINCONFIG_H

#include "astring.h"
#include "instmgr.h"
#include <windows.h>

// Object implementing the Status dialogue for apcupsd
class upsConfig
{
public:
   // Constructor/destructor
   upsConfig(HINSTANCE appinst, InstanceManager *instmgr);
   ~upsConfig();

   // General
   void Show(MonitorConfig &mcfg);

private:
   // The dialog box window proc
   static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
   BOOL DialogProcess(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   // Private data
   HWND _hwnd;
   HINSTANCE _appinst;
   InstanceManager *_instmgr;
   MonitorConfig _config;
   HWND _hhost, _hport, _hrefresh, _hpopups;
   bool _hostvalid, _portvalid, _refreshvalid;
};

#endif // WINCONFIG_H
