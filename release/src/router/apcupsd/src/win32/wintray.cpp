// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000-2005) Kern E. Sibbald

// Implementation of a system tray icon & menu for Apcupsd

#include "apc.h"
#include <windows.h>
#include "winups.h"
#include "resource.h"
#include "wintray.h"
#include "statmgr.h"
#include "balloonmgr.h"

// Implementation
upsMenu::upsMenu(HINSTANCE appinst, MonitorConfig &mcfg, BalloonMgr *balmgr,
                 InstanceManager *instmgr)
   : _statmgr(NULL),
     _about(appinst),
     _status(appinst, this),
     _events(appinst, this),
     _configdlg(appinst, instmgr),
     _wait(NULL),
     _thread(NULL),
     _hmenu(NULL),
     _hsubmenu(NULL),
     _upsname("<unknown>"),
     _balmgr(balmgr),
     _appinst(appinst),
     _hwnd(NULL),
     _config(mcfg),
     _runthread(true),
     _generation(0),
     _reconfig(true),
     _instmgr(instmgr)
{
   // Determine message id for "TaskbarCreate" message
   _tbcreated_msg = RegisterWindowMessage("TaskbarCreated");

   // Create a dummy window to handle tray icon messages
   WNDCLASSEX wndclass;
   wndclass.cbSize = sizeof(wndclass);
   wndclass.style = 0;
   wndclass.lpfnWndProc = upsMenu::WndProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = appinst;
   wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
   wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH);
   wndclass.lpszMenuName = (const char *)NULL;
   wndclass.lpszClassName = APCTRAY_WINDOW_CLASS;
   wndclass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
   RegisterClassEx(&wndclass);

   // Make unique window title as 'host:port'.
   char title[1024];
   asnprintf(title, sizeof(title), "%s:%d", mcfg.host.str(), mcfg.port);

   // Create System Tray menu window
   _hwnd = CreateWindow(APCTRAY_WINDOW_CLASS, title, WS_OVERLAPPEDWINDOW,
                         CW_USEDEFAULT, CW_USEDEFAULT, 200, 200, NULL, NULL,
                         appinst, NULL);
   if (_hwnd == NULL) {
      PostQuitMessage(0);
      return;
   }

   // record which client created this window
   SetWindowLong(_hwnd, GWL_USERDATA, (LONG)this);

   // Load the icons for the tray
   _online_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_ONLINE));
   _onbatt_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_ONBATT));
   _charging_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_CHARGING));
   _commlost_icon = LoadIcon(appinst, MAKEINTRESOURCE(IDI_COMMLOST));

   // Load the popup menu
   _hmenu = LoadMenu(appinst, MAKEINTRESOURCE(IDR_TRAYMENU));
   if (_hmenu == NULL) {
      PostQuitMessage(0);
      return;
   }
   _hsubmenu = GetSubMenu(_hmenu, 0);

   // Install the tray icon. Although it's tempting to let this happen
   // on the poll thread, we do it here so its synchronous and all icons
   // are consistently created in the same order.
   AddTrayIcon();

   // Create a semaphore to use for interruptible waiting
   _wait = CreateSemaphore(NULL, 0, 1, NULL);
   if (_wait == NULL) {
      PostQuitMessage(0);
      return;
   }

   // Thread to poll UPS status and update tray icon
   _thread = CreateThread(NULL, 0, &upsMenu::StatusPollThread, this, 0, NULL);
   if (_thread == NULL)
      PostQuitMessage(0);
}

upsMenu::~upsMenu()
{
   // Kill status polling thread
   if (WaitForSingleObject(_thread, 10000) == WAIT_TIMEOUT)
      TerminateThread(_thread, 0);
   CloseHandle(_thread);

   // Destroy the mutex
   CloseHandle(_wait);

   // Destroy the status manager
   delete _statmgr;

   // Remove the tray icon
   DelTrayIcon();

   // Destroy the window
   DestroyWindow(_hwnd);

   // Destroy the loaded menu
   DestroyMenu(_hmenu);

   // Unregister the window class
   UnregisterClass(APCTRAY_WINDOW_CLASS, _appinst);
}

void upsMenu::Destroy()
{
   // Trigger status poll thread to shut down. We will wait for
   // the thread to exit later in our destructor.
   _runthread = false;
   ReleaseSemaphore(_wait, 1, NULL);
}

void upsMenu::AddTrayIcon()
{
   SendTrayMsg(NIM_ADD);
}

void upsMenu::DelTrayIcon()
{
   SendTrayMsg(NIM_DELETE);
}

void upsMenu::UpdateTrayIcon()
{
   SendTrayMsg(NIM_MODIFY);
}

void upsMenu::SendTrayMsg(DWORD msg)
{
   // Create the tray icon message
   NOTIFYICONDATA nid;
   memset(&nid, 0, sizeof(nid));
   nid.hWnd = _hwnd;
   nid.cbSize = sizeof(nid);
   nid.uID = IDI_APCUPSD;
   nid.uFlags = NIF_ICON | NIF_MESSAGE;
   nid.uCallbackMessage = WM_APCTRAY_NOTIFY;

   int battstat = -1;
   astring statstr;

   // Get current status
   switch (msg) {
   case NIM_ADD:
   case NIM_DELETE:
      // Process these messages quickly without fetching new status
      break; 
   default:
      // Fetch current UPS status
      _statmgr->GetSummary(battstat, statstr, _upsname);
      break;
   }

   /* If battstat == 0 we are on batteries, otherwise we are online
    * and the value of battstat is the percent charge.
    */
   if (battstat == -1)
      nid.hIcon = _commlost_icon;
   else if (battstat == 0)
      nid.hIcon = _onbatt_icon;
   else if (battstat >= 100)
      nid.hIcon = _online_icon;
   else
      nid.hIcon = _charging_icon;

   // Use status as normal tooltip
   nid.uFlags |= NIF_TIP;
   if (_upsname == "UPS_IDEN" || _upsname == "<unknown>")
      asnprintf(nid.szTip, sizeof(nid.szTip), "%s", statstr.str());
   else
      asnprintf(nid.szTip, sizeof(nid.szTip), "%s: %s",
                _upsname.str(), statstr.str());

   // Display event in balloon tip
   if (_config.popups && !_laststatus.empty() && _laststatus != statstr)
      _balmgr->PostBalloon(_hwnd, _upsname, statstr);
   _laststatus = statstr;

   // Send the message
   if (!Shell_NotifyIcon(msg, &nid) && msg == NIM_ADD) {
      // The tray icon couldn't be created
      PostQuitMessage(0);
   }
}

// Process window messages (static springboard)
LRESULT CALLBACK upsMenu::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   // This is a static method, so we don't know which instantiation we're 
   // dealing with. We use Allen Hadden's (ahadden@taratec.com) suggestion 
   // from a newsgroup to get the pseudo-this.
   upsMenu *_this = (upsMenu *) GetWindowLong(hwnd, GWL_USERDATA);

   // During creation, we are called before the WindowLong has been set.
   // Just use default processing in that case since _this is not valid.   
   if (_this)
      return _this->WndProcess(hwnd, iMsg, wParam, lParam);
   else
      return DefWindowProc(hwnd, iMsg, wParam, lParam);
   
}

// Process window messages
LRESULT upsMenu::WndProcess(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
   switch (iMsg)
   {
   // User has clicked an item on the tray menu
   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case IDM_STATUS:
         // Show the status dialog
         _status.Show();
         break;

      case IDM_EVENTS:
         // Show the Events dialog
         _events.Show();
         break;

      case IDM_ABOUT:
         // Show the About box
         _about.Show();
         break;

      case IDM_EXIT:
         // User selected Exit from the tray menu
         PostMessage(hwnd, WM_CLOSE, 0, 0);
         break;

      case IDM_REMOVE:
         // User selected Remove from the tray menu
         PostMessage(hwnd, WM_APCTRAY_REMOVE, 0, (LPARAM)(_config.id.str()));
         break;

      case IDM_REMOVEALL:
         // User wants to remove all apctray instances from registry
         PostMessage(hwnd, WM_APCTRAY_REMOVEALL, 0, 0);
         break;

      case IDM_ADD:
         // User selected Add from the tray menu
         PostMessage(hwnd, WM_APCTRAY_ADD, 0, 0);
         break;

      case IDM_CONFIG:
         // User selected Config from the tray menu
         _configdlg.Show(_config);
         break;

      case IDM_AUTOSTART:
      {
         MENUITEMINFO mii;
         mii.cbSize = sizeof(MENUITEMINFO);
         mii.fMask = MIIM_STATE;
         GetMenuItemInfo(_hsubmenu, IDM_AUTOSTART, FALSE, &mii);
         mii.fState ^= (MFS_CHECKED | MFS_UNCHECKED);
         SetMenuItemInfo(_hsubmenu, IDM_AUTOSTART, FALSE, &mii);
         _instmgr->SetAutoStart(mii.fState & MFS_CHECKED);
         break;
      }

      }
      return 0;

   // User has clicked on the tray icon or the menu
   case WM_APCTRAY_NOTIFY:

      // What event are we responding to, RMB click?
      if (lParam == WM_RBUTTONUP)
      {
         // Make the Status menu item the default (bold font)
         SetMenuDefaultItem(_hsubmenu, IDM_STATUS, false);

         // Set UPS name field
         ModifyMenu(_hsubmenu, IDM_NAME, MF_BYCOMMAND|MF_STRING, IDM_NAME,
            ("UPS: " + _upsname).str());

         // Set HOST field
         char buf[100];
         asnprintf(buf, sizeof(buf), "HOST: %s:%d", _config.host.str(), _config.port);
         ModifyMenu(_hsubmenu, IDM_HOST, MF_BYCOMMAND|MF_STRING, IDM_HOST, buf);

         // Set autostart field
         MENUITEMINFO mii;
         mii.cbSize = sizeof(MENUITEMINFO);
         mii.fMask = MIIM_STATE;
         mii.fState = MFS_ENABLED | 
            _instmgr->IsAutoStart() ? MFS_CHECKED : MFS_UNCHECKED;
         SetMenuItemInfo(_hsubmenu, IDM_AUTOSTART, FALSE, &mii);

         // Get the current cursor position, to display the menu at
         POINT mouse;
         GetCursorPos(&mouse);

         // There's a "bug" (Microsoft calls it a feature) in Windows 95 that 
         // requires calling SetForegroundWindow. To find out more, search for 
         // Q135788 in MSDN.
         SetForegroundWindow(_hwnd);

         // Display the menu at the desired position
         TrackPopupMenu(_hsubmenu, 0, mouse.x, mouse.y, 0, _hwnd, NULL);
      }
      // Or was there a LMB double click?
      else if (lParam == WM_LBUTTONDBLCLK)
      {
         // double click: execute the default item
         SendMessage(_hwnd, WM_COMMAND, IDM_STATUS, 0);
      }

      return 0;

   // The user wants Apctray to quit cleanly...
   case WM_CLOSE:
      PostQuitMessage(0);
      return 0;

   default:
      if (iMsg == _tbcreated_msg)
      {
         // Explorer has restarted so we need to redraw the tray icon.
         // We purposely kick this out to the main loop instead of handling it
         // ourself so the icons are redrawn in a consistent order.
         PostMessage(hwnd, WM_APCTRAY_RESET, _generation++, 0);
      }
      break;
   }

   // Unknown message type
   return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void upsMenu::Redraw()
{
   AddTrayIcon();
}

void upsMenu::Reconfigure(const MonitorConfig &mcfg)
{
   // Indicate that a config change is pending
   _mutex.lock();
   _config = mcfg;
   _reconfig = true;
   _mutex.unlock();

   Refresh();
}

void upsMenu::Refresh()
{
   // Wake the poll thread to refresh the status ASAP
   ReleaseSemaphore(_wait, 1, NULL);
}

DWORD WINAPI upsMenu::StatusPollThread(LPVOID param)
{
   upsMenu* _this = (upsMenu*)param;

   while (_this->_runthread)
   {
      // Act on pending config change
      _this->_mutex.lock();
      if (_this->_reconfig)
      {
         // Recreate statmgr with new config
         delete _this->_statmgr;
         _this->_statmgr = new StatMgr(_this->_config.host, _this->_config.port);
         _this->_reconfig = false;
      }
      _this->_mutex.unlock();

      // Update the tray icon and status dialog
      _this->UpdateTrayIcon();
      _this->_status.Update(_this->_statmgr);
      _this->_events.Update(_this->_statmgr);

      // Delay for configured interval
      WaitForSingleObject(_this->_wait, _this->_config.refresh * 1000);
   }
}
