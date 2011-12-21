// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2009) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the Events dialogue 

#include <windows.h>
#include "winevents.h"
#include "resource.h"
#include "statmgr.h"
#include "listview.h"
#include "wintray.h"

// Constructor/destructor
upsEvents::upsEvents(HINSTANCE appinst, upsMenu *menu) :
   _appinst(appinst),
   _hwnd(NULL),
   _menu(menu)
{
}

upsEvents::~upsEvents()
{
}

// Dialog box handling functions
void upsEvents::Show()
{
   if (!_hwnd)
   {
      DialogBoxParam(_appinst,
                     MAKEINTRESOURCE(IDD_EVENTS),
                     NULL,
                     (DLGPROC)DialogProc,
                     (LONG)this);
   }
}

BOOL CALLBACK upsEvents::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   upsEvents *_this;

   // Retrieve virtual 'this' pointer. When we come in here the first time for
   // the WM_INITDIALOG message, the pointer is in lParam. We then store it in
   // the user data so it can be retrieved on future calls.
   if (uMsg == WM_INITDIALOG)
   {
      // Set dialog user data to our "this" pointer which comes in via lParam.
      // On subsequent calls, this will be retrieved by the code below.
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsEvents *)lParam;
   }
   else
   {
      // We've previously been initialized, so retrieve pointer from user data
      _this = (upsEvents *)GetWindowLong(hwnd, GWL_USERDATA);
   }

   // Call thru to non-static member function
   return _this->DialogProcess(hwnd, uMsg, wParam, lParam);
}

BOOL upsEvents::DialogProcess(
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
      _events = new ListView(hwnd, IDC_LIST, 1);

      // Save a copy of our window handle for later use.
      // Important to do this AFTER everything needed by Update() is
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
      HWND ctrl = GetDlgItem(hwnd, IDC_LIST);
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
      switch (LOWORD(wParam)) {
      case IDCANCEL:
      case IDOK:
         // Close the dialog
         EndDialog(hwnd, TRUE);
         return TRUE;
      }
      break;

   case WM_DESTROY:
      _mutex.lock();
      _hwnd = NULL;
      delete _events;
      _mutex.unlock();
      return TRUE;
   }

   return 0;
}

void upsEvents::Update(StatMgr *statmgr)
{
   // Bail if window is not open
   _mutex.lock();
   if (!_hwnd)
   {
      _mutex.unlock();
      return;
   }

   // Fetch events from apcupsd
   alist<astring> events;
   if (!statmgr->GetEvents(events) || events.empty())
   {
      _mutex.unlock();
      return;
   }

   // Update listview
   alist<astring> *data[] = {&events};
   _events->UpdateAll(data);

   _mutex.unlock();
}
