// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

#ifndef WINUPS_H
#define WINUPS_H

// WinUPS header file

#include <windows.h>

// Application specific messages
enum {
   // Message used for system tray notifications
   WM_APCTRAY_NOTIFY = WM_USER+1,

   // Message used to remove all apctray instances from the registry
   WM_APCTRAY_REMOVEALL,

   // Message used to remove specified apctray instance from the registry
   WM_APCTRAY_REMOVE,

   // Messages used to trigger redraw of tray icons
   WM_APCTRAY_RESET,

   // Message used to add a new apctray instance
   WM_APCTRAY_ADD
};

// Apcupsd application window constants
#define APCUPSD_WINDOW_CLASS		"apcupsd"
#define APCUPSD_WINDOW_NAME		"apcupsd"

// apctray window constants
#define APCTRAY_WINDOW_CLASS		"apctray"
#define APCTRAY_WINDOW_NAME		"apctray"

// Command line option to start in service mode
#define ApcupsdRunService        "/service"

// Names of various global events
#define APCUPSD_STOP_EVENT_NAME  "Global\\ApcupsdStopEvent"
#define APCTRAY_STOP_EVENT_NAME  "Global\\ApctrayStopEvent"

// Main UPS server routine - Exported by winmain for use by winservice
extern int ApcupsdAppMain(int service);

// Stop apcupsd - Exported by winmain for use by winservice
extern void ApcupsdTerminate();

#endif // WINUPS_H
