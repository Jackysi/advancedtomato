// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald


// This class handles creation of a system-tray icon & menu

#ifndef WINTRAY_H
#define WINTRAY_H

#include <windows.h>
#include "winabout.h"
#include "winstat.h"
#include "winevents.h"
#include "winconfig.h"
#include "astring.h"
#include "instmgr.h"
#include "amutex.h"

// Forward declarations
class StatMgr;
class BalloonMgr;

// The tray menu class itself
class upsMenu
{
public:
   upsMenu(HINSTANCE appinst, MonitorConfig &mcfg, BalloonMgr *balmgr,
           InstanceManager *instmgr);
   ~upsMenu();
   void Destroy();
   void Redraw();
   void Reconfigure(const MonitorConfig &mcfg);
   void Refresh();

protected:
   // Tray icon handling
   void AddTrayIcon();
   void DelTrayIcon();
   void UpdateTrayIcon();
   void SendTrayMsg(DWORD msg);

   // Message handler for the tray window
   static LRESULT CALLBACK WndProc(
      HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
   LRESULT WndProcess(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

   // Fetch UPS status info
   bool FetchStatus(int &battstat, astring &statstr, astring &upsname);

   // Thread to poll for UPS status changes
   static DWORD WINAPI StatusPollThread(LPVOID param);

   HWND                    _hwnd;           // Window handle
   HMENU                   _hmenu;          // Menu handle
   HMENU                   _hsubmenu;       // Submenu handle
   StatMgr                *_statmgr;        // Manager for UPS stats
   HANDLE                  _thread;         // Handle to status polling thread
   HANDLE                  _wait;           // Handle to wait mutex
   astring                 _upsname;        // Cache UPS name
   astring                 _laststatus;     // Cache previous status string
   BalloonMgr             *_balmgr;         // Balloon tip manager
   UINT                    _tbcreated_msg;  // Id of TaskbarCreated message
   HINSTANCE               _appinst;        // Application instance handle
   MonitorConfig           _config;         // Configuration (host, port, etc.)
   bool                    _runthread;      // Run the poll thread?
   amutex                  _mutex;          // Lock to protect statmgr
   WPARAM                  _generation;
   bool                    _reconfig;
   InstanceManager        *_instmgr;

   // Dialogs for About, Status, Config, and Events
   upsAbout                _about;
   upsStatus               _status;
   upsConfig               _configdlg;
   upsEvents               _events;

   // The icon handles
   HICON                   _online_icon;
   HICON                   _onbatt_icon;
   HICON                   _charging_icon;
   HICON                   _commlost_icon;
};

#endif // WINTRAY_H
