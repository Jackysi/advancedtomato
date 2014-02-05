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

#include "winapi.h"
#include <unistd.h>
#include <windows.h>

#include "apc.h"
#include "wintray.h"
#include "resource.h"
#include "winups.h"
#include "statmgr.h"
#include "astring.h"
#include "instmgr.h"

#define CMDOPT_INSTALL  "/install"
#define CMDOPT_REMOVE   "/remove"
#define CMDOPT_KILL     "/kill"
#define CMDOPT_QUIET    "/quiet"

#define USAGE_TEXT   "[" CMDOPT_INSTALL "] "            \
                     "[" CMDOPT_REMOVE  "] "            \
                     "[" CMDOPT_KILL    "] "            \
                     "[" CMDOPT_QUIET   "]"


// Global variables
static HINSTANCE appinst;                       // Application handle
static bool quiet = false;                      // Suppress user dialogs

void NotifyError(const char *format, ...)
{
   va_list args;
   char buf[2048];

   va_start(args, format);
   avsnprintf(buf, sizeof(buf), format, args);
   va_end(args);

   MessageBox(NULL, buf, "Apctray", MB_OK|MB_ICONEXCLAMATION);
}

void NotifyUser(const char *format, ...)
{
   va_list args;
   char buf[2048];

   if (!quiet) {
      va_start(args, format);
      avsnprintf(buf, sizeof(buf), format, args);
      va_end(args);

      MessageBox(NULL, buf, "Apctray", MB_OK|MB_ICONINFORMATION);
   }
}

void PostToApctray(DWORD msg)
{
   HWND wnd = FindWindow(APCTRAY_WINDOW_CLASS, NULL);
   if (wnd)
      PostMessage(wnd, msg, 0, 0);
   CloseHandle(wnd);
}

int Install()
{
   // Get the full path/filename of this executable
   char path[1024];
   GetModuleFileName(NULL, path, sizeof(path));

   // Add double quotes
   char cmd[1024];
   asnprintf(cmd, sizeof(cmd), "\"%s\"", path);

   // Open registry key for auto-run programs
   HKEY runkey;
   if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    &runkey) != ERROR_SUCCESS) {
      NotifyUser("The System Registry could not be updated.\n"
                 "Apctray was not installed.");
      return 1;
   }

   // Attempt to add Apctray key
   if (RegSetValueEx(runkey, "Apctray", 0, REG_SZ,
         (unsigned char *)cmd, strlen(cmd)+1) != ERROR_SUCCESS) {
      RegCloseKey(runkey);
      NotifyUser("The System Registry could not be updated.\n"
                 "Apctray was not installed.");
      return 1;
   }

   RegCloseKey(runkey);

   NotifyUser("Apctray was installed successfully and will\n"
              "automatically run when users log on.");
}

int Remove()
{
   // Open registry key apctray
   HKEY runkey;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    0, KEY_READ|KEY_WRITE, &runkey) == ERROR_SUCCESS) {
      RegDeleteValue(runkey, "Apctray");
      RegCloseKey(runkey);
   }

   NotifyUser("Apctray will no longer start automatically.");
   return 0;
}

int Kill()
{
   PostToApctray(WM_CLOSE);

   if (g_os_version >= WINDOWS_2000)
   {
      HANDLE evt = OpenEvent(EVENT_MODIFY_STATE, FALSE, APCTRAY_STOP_EVENT_NAME);
      if (evt != NULL)
      {
         SetEvent(evt);
         CloseHandle(evt);
      }
   }

   return 0;
}

void Usage(const char *text1, const char* text2)
{
   MessageBox(NULL, text1, text2, MB_OK);
   MessageBox(NULL, USAGE_TEXT, "Apctray Usage",
              MB_OK | MB_ICONINFORMATION);
}

// This thread runs on Windows 2000 and higher. It monitors for the
// global exit event to be signaled (/kill).
static bool runthread = false;
static HANDLE exitevt;
DWORD WINAPI EventThread(LPVOID param)
{
   // Create global exit event and allow Adminstrator access to it so any
   // member of the Administrators group can signal it.
   exitevt = CreateEvent(NULL, TRUE, FALSE, APCTRAY_STOP_EVENT_NAME);
   GrantAccess(exitevt, EVENT_MODIFY_STATE, TRUSTEE_IS_GROUP, "Administrators");

   // Wait for event to be signaled or for an error
   DWORD rc = WaitForSingleObject(exitevt, INFINITE);
   if (rc == WAIT_OBJECT_0)
      PostToApctray(WM_CLOSE);

   CloseHandle(exitevt);
   return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR CmdLine, int iCmdShow)
{
   InitWinAPIWrapper();

   // Publicize application handle
   appinst = hInstance;

   // Instance parameters
   char *host = NULL;
   unsigned short port = 0;
   int refresh = 0;

   // Check command line options
   char *arg;
   char *opt = CmdLine;
   while ((arg = GetArg(&opt))) {
      if (strcasecmp(arg, CMDOPT_INSTALL) == 0) {
         return Install();
      } else if (strcasecmp(arg, CMDOPT_REMOVE) == 0) {
         return Remove();
      } else if (strcasecmp(arg, CMDOPT_KILL) == 0) {
         return Kill();
      } else if (strcasecmp(arg, CMDOPT_QUIET) == 0) {
         quiet = true;
      } else {
         Usage(arg, "Unknown option");
         return 1;
      }
   }

   // Check to see if we're already running in this session
   const char *semname = g_os_version < WINDOWS_2000 ?
      "apctray" : "Local\\apctray";
   HANDLE sem = CreateSemaphore(NULL, 0, 1, semname);
   if (sem == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
      NotifyUser("Apctray is already running");
      WSACleanup();
      return 0;
   }

   // On Win2K and above we spawn a thread to watch for exit requests.
   HANDLE evtthread;
   if (g_os_version >= WINDOWS_2000) {
      runthread = true;
      evtthread = CreateThread(NULL, 0, EventThread, NULL, 0, NULL);
   }

   WPARAM generation = 0;
   InstanceManager instmgr(appinst);
   instmgr.CreateMonitors();

   // Enter the Windows message handling loop until told to quit
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0) > 0) {

      TranslateMessage(&msg);

      switch (LOWORD(msg.message)) {
      case WM_APCTRAY_REMOVEALL:
         // Remove all instances (and close)
         instmgr.RemoveAll();
         PostQuitMessage(0);
         break;

      case WM_APCTRAY_REMOVE:
         // Remove the given instance and exit if there are no more instances
         if (instmgr.RemoveInstance((const char *)msg.lParam) == 0)
            PostQuitMessage(0);
         break;

      case WM_APCTRAY_ADD:
         // Remove the given instance and exit if there are no more instances
         instmgr.AddInstance();
         break;

      case WM_APCTRAY_RESET:
         // Redraw tray icons due to explorer exit/restart
         // We'll get a RESET message from each tray icon but we only want
         // to act on the first one. Check the generation and only perform the
         // reset if the generation matches.
         if (msg.wParam == generation)
         {
            // Bump generation so we ignore redundant reset commands
            generation++;
            // Delay while other tray icons refresh. This prevents our icons
            // from being scattered across the tray.
            sleep(2);
            // Now command the instances to reset
            instmgr.ResetInstances();
         }
         break;

      default:
         DispatchMessage(&msg);
      }
   }

   // Wait for event thread to exit cleanly
   if (g_os_version >= WINDOWS_2000) {
      runthread = false;
      SetEvent(exitevt); // Kick exitevt to wake up thread
      if (WaitForSingleObject(evtthread, 5000) == WAIT_TIMEOUT)
         TerminateThread(evtthread, 0);
      CloseHandle(evtthread);
   }

   WSACleanup();
   return 0;
}
