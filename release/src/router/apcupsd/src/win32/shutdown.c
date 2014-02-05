/*
 * Dumb program to shutdown Windows
 * This code implements a very limited set of the
 * standard Unix shutdown program.
 *
 *   Kern E. Sibbald, July MM
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#ifndef HAVE_MINGW
extern void mainCRTStartup();
void WinMainCRTStartup() { mainCRTStartup(); }
#endif

enum {
   MODE_CANCEL,
   MODE_HALT,
   MODE_REBOOT
};

int main(int argc, char **argv)
{
   char ch;
   int mode = MODE_HALT;
   int timeout = 0;
   int force = 1;
   char* message;

   // Process command line args
   while ((ch = getopt(argc, argv, "+chrf")) != -1) {
      switch (ch) {
      case 'c':
         mode = MODE_CANCEL;
         break;
      case 'h':
         mode = MODE_HALT;
         break;
      case 'r':
         mode = MODE_REBOOT;
         break;
      case 'f':
         force = 0;
         break;
      }
   }

   if (optind < argc) {
      if (stricmp(argv[optind], "now") == 0)
         timeout = 0;
      else
         timeout = strtoul(argv[optind], NULL, 0);
      optind++;
   }

   if (optind < argc) {
      message = argv[optind];
   } else {
      message = "Power failure system going down!";
   }

   // Get the current OS version
   OSVERSIONINFO ver;
   ver.dwOSVersionInfoSize = sizeof(ver);
   GetVersionEx(&ver);

   /* For WinNT and above, we must get permission */
   if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) { 

      HANDLE hToken; 
      TOKEN_PRIVILEGES tkp; 

      // Get a token for this process. 
      OpenProcessToken(GetCurrentProcess(), 
         TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);

      // Get the LUID for the shutdown privilege. 

      LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
	      &tkp.Privileges[0].Luid); 

      tkp.PrivilegeCount = 1;  // one privilege to set	  
      tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

      // Get the shutdown privilege for this process. 

      AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
	      (PTOKEN_PRIVILEGES)NULL, 0);
   }

   if (mode == MODE_CANCEL) {
      AbortSystemShutdown(NULL);
   } else {
      if (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) {
         InitiateSystemShutdown(
            NULL,                                  /* Local machine */
            message,                               /* Message */
            timeout,                               /* Timeout (secs) */
            force,                                 /* Force */
            mode == MODE_REBOOT);                  /* Reboot */
      } else {
         Sleep(timeout*1000);

         int action = (mode == MODE_REBOOT) ? EWX_REBOOT : EWX_SHUTDOWN;

         if (force)
            action |= EWX_FORCE;

         ExitWindowsEx(
            action, /* Action */
            0);     /* Reason */
      }
   }

   exit(0);
}
