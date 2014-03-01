// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2009) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

#ifndef WINEVENTS_H
#define WINEVENTS_H

#include <windows.h>
#include "amutex.h"

// Forward declarations
class StatMgr;
class ListView;
class upsMenu;

// Object implementing the Events dialogue box for apcupsd
class upsEvents
{
public:
   // Constructor/destructor
   upsEvents(HINSTANCE appinst, upsMenu *menu);
   ~upsEvents();

   // General
   void Show();
   void Update(StatMgr *statmgr);

private:
   // The dialog box window proc
   static BOOL CALLBACK DialogProc(
      HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
   BOOL CALLBACK DialogProcess(
      HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

   // Private data
   HWND _hwnd;
   HINSTANCE _appinst;
   ListView *_events;
   amutex _mutex;
   RECT _rect;
   upsMenu *_menu;
};

#endif // WINEVENTS_H
