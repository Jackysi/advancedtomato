// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2009) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the Status dialog

#include <windows.h>
#include <commctrl.h>
#include "winstat.h"
#include "resource.h"
#include "statmgr.h"
#include "meter.h"
#include "listview.h"
#include "wintray.h"

// Constructor/destructor
upsStatus::upsStatus(HINSTANCE appinst, upsMenu *menu) :
   _hwnd(NULL),
   _appinst(appinst),
   _menu(menu)
{
}

upsStatus::~upsStatus()
{
}

// Dialog box handling functions
void upsStatus::Show()
{
   if (!_hwnd)
   {
      DialogBoxParam(_appinst,
                     MAKEINTRESOURCE(IDD_STATUS),
                     NULL,
                     (DLGPROC)DialogProc,
                     (LONG)this);
   }
}

BOOL CALLBACK upsStatus::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   upsStatus *_this;

   // Retrieve virtual 'this' pointer. When we come in here the first time for
   // the WM_INITDIALOG message, the pointer is in lParam. We then store it in
   // the user data so it can be retrieved on future calls.
   if (uMsg == WM_INITDIALOG)
   {
      // Set dialog user data to our "this" pointer which comes in via lParam.
      // On subsequent calls, this will be retrieved by the code below.
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsStatus *)lParam;
   }
   else
   {
      // We've previously been initialized, so retrieve pointer from user data
      _this = (upsStatus *)GetWindowLong(hwnd, GWL_USERDATA);
   }

   // Call thru to non-static member function
   return _this->DialogProcess(hwnd, uMsg, wParam, lParam);
}

BOOL upsStatus::DialogProcess(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   switch (uMsg) {
   case WM_INITDIALOG:
      // Silly: Save initial window size for use as minimum size. There's 
      // probably some programmatic way to fetch this from the resource when
      // we need it, but I can't find it. So we'll save it at runtime.
      GetWindowRect(hwnd, &_rect);

      // Initialize control wrappers
      _bmeter = new Meter(hwnd, IDC_BATTERY, 25, 15);
      _lmeter = new Meter(hwnd, IDC_LOAD, 75, 90);
      _grid = new ListView(hwnd, IDC_STATUSGRID, 2);

      // Save a copy of our window handle for later use.
      // Important to do this AFTER everything needed by FillStatusBox() is
      // initialized and ready to go since that function may be called at any
      // time from the wintray timer thread.
      _hwnd = hwnd;

      // Show the dialog
      _menu->Refresh();
      SetForegroundWindow(hwnd);
      return TRUE;

   case WM_GETMINMAXINFO:
      // Restrict minimum size to initial window size
      MINMAXINFO *mmi = (MINMAXINFO*)lParam;
      mmi->ptMinTrackSize.x = _rect.right - _rect.left;
      mmi->ptMinTrackSize.y = _rect.bottom - _rect.top;
      return TRUE;

   case WM_SIZE:
   {
      // Fetch new window size (esp client area size)
      WINDOWINFO wininfo;
      wininfo.cbSize = sizeof(wininfo);
      GetWindowInfo(hwnd, &wininfo);

      // Fetch current listview position
      HWND ctrl = GetDlgItem(hwnd, IDC_STATUSGRID);
      RECT gridrect;
      GetWindowRect(ctrl, &gridrect);

      // Calculate new position and size of listview
      int left = gridrect.left - wininfo.rcClient.left;
      int top = gridrect.top - wininfo.rcClient.top;
      int width = wininfo.rcClient.right - wininfo.rcClient.left - 2*left;
      int height = wininfo.rcClient.bottom - wininfo.rcClient.top - top - left;

      // Resize listview
      SetWindowPos(
         ctrl, NULL, left, top, width, height, SWP_NOZORDER | SWP_SHOWWINDOW);

      return TRUE;
   }

   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case IDCANCEL:
      case IDOK:
         EndDialog(hwnd, TRUE);
         return TRUE;
      }
      break;

   case WM_DESTROY:
      _mutex.lock();
      _hwnd = NULL;
      delete _bmeter;
      delete _lmeter;
      delete _grid;
      _mutex.unlock();
      return TRUE;
   }

   return FALSE;
}

void upsStatus::Update(StatMgr *statmgr)
{
   // Bail if window is not open
   _mutex.lock();
   if (!_hwnd)
   {
      _mutex.unlock();
      return;
   }

   // Fetch full status from apcupsd
   alist<astring> keys, values;
   if (!statmgr->GetAll(keys, values) || keys.empty())
   {
      _mutex.unlock();
      return;
   }

   // Update listview
   alist<astring>* data[] = {&keys, &values};
   _grid->UpdateAll(data);

   // Update battery
   _bmeter->Set(atoi(statmgr->Get("BCHARGE")));

   // Update load
   _lmeter->Set(atoi(statmgr->Get("LOADPCT")));

   // Update status
   char str[128];
   astring stat = statmgr->Get("STATUS");
   SendDlgItemMessage(_hwnd, IDC_STATUS, WM_GETTEXT, sizeof(str), (LONG)str);
   if (stat != str)
      SendDlgItemMessage(_hwnd, IDC_STATUS, WM_SETTEXT, 0, (LONG)stat.str());

   // Update runtime
   astring runtime = statmgr->Get("TIMELEFT");
   runtime = runtime.substr(0, runtime.strchr(' '));
   SendDlgItemMessage(_hwnd, IDC_RUNTIME, WM_GETTEXT, sizeof(str), (LONG)str);
   if (runtime != str)
      SendDlgItemMessage(_hwnd, IDC_RUNTIME, WM_SETTEXT, 0, (LONG)runtime.str());

   // Update title bar
   astring name;
   name.format("Status for UPS: %s", statmgr->Get("UPSNAME").str());
   SendMessage(_hwnd, WM_SETTEXT, 0, (LONG)name.str());

   _mutex.unlock();
}
