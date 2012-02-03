// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of the About dialog

#include <windows.h>
#include <stdio.h>
#include "winabout.h"
#include "resource.h"
#include "version.h"

// Constructor/destructor
upsAbout::upsAbout(HINSTANCE appinst)
{
   _dlgvisible = FALSE;
   _appinst = appinst;
}

upsAbout::~upsAbout()
{
}

// Dialog box handling functions
void upsAbout::Show()
{
   if (!_dlgvisible)
   {
      DialogBoxParam(_appinst,
                     MAKEINTRESOURCE(IDD_ABOUT), 
                     NULL,
                    (DLGPROC) DialogProc,
                    (LONG) this);
   }
}

BOOL CALLBACK upsAbout::DialogProc(
   HWND hwnd,
   UINT uMsg,
   WPARAM wParam,
   LPARAM lParam)
{
   // We use the dialog-box's USERDATA to store a _this pointer
   // This is set only once WM_INITDIALOG has been recieved, though!
   upsAbout *_this = (upsAbout *)GetWindowLong(hwnd, GWL_USERDATA);

   switch (uMsg)
   {
   case WM_INITDIALOG:
      // Retrieve the Dialog box parameter and use it as a pointer
      // to the calling vncProperties object
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      _this = (upsAbout *)lParam;

      // Show the dialog
      char tmp[128];
      snprintf(tmp, sizeof(tmp), "Apctray %s (%s)", VERSION, ADATE);
      SendDlgItemMessage(hwnd, IDC_VERSION, WM_SETTEXT, 0, (LONG)tmp);
      SetForegroundWindow(hwnd);
      _this->_dlgvisible = TRUE;
      return TRUE;

   case WM_COMMAND:
      switch (LOWORD(wParam))
      {
      case IDCANCEL:
      case IDOK:
         // Close the dialog
         EndDialog(hwnd, TRUE);
         _this->_dlgvisible = FALSE;
         return TRUE;
      }
      break;

   case WM_DESTROY:
       EndDialog(hwnd, FALSE);
       _this->_dlgvisible = FALSE;
       return TRUE;
   }

   return 0;
}
