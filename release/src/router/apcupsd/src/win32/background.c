#include "windows.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
         PSTR szCmdLine, int iCmdShow)
{
   PROCESS_INFORMATION procinfo;
   STARTUPINFOA startinfo;
   BOOL rc;

   if (*szCmdLine == '\0')
   {
      MessageBox(NULL, "Usage: background.exe <command>", "Invalid usage!", MB_ICONSTOP);
      return 1;
   }

   /* Init the STARTUPINFOA struct. */
   memset(&startinfo, 0, sizeof(startinfo));
   startinfo.cb = sizeof(startinfo);

   rc = CreateProcess(NULL,
                      szCmdLine,        // command line
                      NULL,             // process security attributes
                      NULL,             // primary thread security attributes
                      TRUE,             // handles are inherited
                      DETACHED_PROCESS, // creation flags
                      NULL,             // use parent's environment
                      NULL,             // use parent's current directory
                      &startinfo,       // STARTUPINFO pointer
                      &procinfo);       // receives PROCESS_INFORMATION
   if (!rc)
   {
      MessageBox(NULL, szCmdLine, "Failed to launch command!", MB_ICONSTOP);
      return 1;
   }

   // Cleanup handles
   CloseHandle(procinfo.hProcess);
   CloseHandle(procinfo.hThread);

   ExitProcess(0);
}
