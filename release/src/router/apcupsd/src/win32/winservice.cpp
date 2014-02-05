// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

// Implementation of service-oriented functionality of Apcupsd

#include "winapi.h"
#include "compat.h"
#include "winups.h"
#include "winservice.h"

// Error message logging
void LogErrorMsg(char *msg, char *fname, int lineno);
#define log_error_message(msg) LogErrorMsg((msg), __FILE__, __LINE__)

// No internationalization support
#define _(x) x

// Internal service state (static)
SERVICE_STATUS         upsService::m_srvstatus;
SERVICE_STATUS_HANDLE  upsService::m_hstatus;
DWORD                  upsService::m_servicethread = 0;

// Typedefs for dynamically loaded functions
typedef BOOL (WINAPI * ChangeServiceConfig2Func)(SC_HANDLE, DWORD, LPVOID);
typedef DWORD (* RegisterServiceProcessFunc)(DWORD, DWORD);

// Internal service name
#define SERVICE_NAME         "Apcupsd"

// Displayed service name
#define SERVICE_DISPLAYNAME  "Apcupsd UPS Monitor"

// List other required serves 
#define SERVICE_DEPENDENCIES __TEXT("tcpip\0afd\0+File System\0") 

// SERVICE MAIN ROUTINE
int upsService::ApcupsdServiceMain()
{
   // How to run as a service depends upon the OS being used
   switch (g_os_version_info.dwPlatformId) {

   // Windows 95/98/Me
   case VER_PLATFORM_WIN32_WINDOWS:
      // Obtain a handle to the kernel library
      HINSTANCE kerneldll = LoadLibrary("KERNEL32.DLL");
      if (kerneldll == NULL) {
         MessageBox(NULL,
                    "KERNEL32.DLL not found: Apcupsd service not started", 
                    "Apcupsd Service", MB_OK);
         break;
      }

      // And find the RegisterServiceProcess function
      RegisterServiceProcessFunc RegisterServiceProcess = 
         (RegisterServiceProcessFunc)GetProcAddress(
            kerneldll, "RegisterServiceProcess");
      if (RegisterServiceProcess == NULL) {
         MessageBox(NULL,
                    "Registry service not found: Apcupsd service not started",
                    "Apcupsd Service", MB_OK);
         log_error_message("Registry service not found"); 
         FreeLibrary(kerneldll);
         break;
      }

      // Register this process with the OS as a service!
      RegisterServiceProcess(0, 1);

      // Run the main program as a service
      ApcupsdAppMain(1);

      // Then remove the service from the system service table
      RegisterServiceProcess(0, 0);

      // Free the kernel library
      FreeLibrary(kerneldll);
      break;

   // Windows NT, Win2K, WinXP 
   case VER_PLATFORM_WIN32_NT:
      // Create a service entry table
      SERVICE_TABLE_ENTRY dispatchTable[] = {
         {SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
         {NULL, NULL}
      };

     // Call the service control dispatcher with our entry table
      if (!StartServiceCtrlDispatcher(dispatchTable)) {
         log_error_message("StartServiceCtrlDispatcher failed.");
      }
      break;

   } /* end switch */

   return 0;
}

// SERVICE MAIN ROUTINE
// NT/Win2K/WinXP ONLY !!!
void WINAPI upsService::ServiceMain(DWORD argc, char **argv)
{
    // Register the service control handler
    m_hstatus = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrl);
    if (m_hstatus == 0) {
       log_error_message("RegisterServiceCtlHandler failed"); 
       MessageBox(NULL, "Contact Register Service Handler failure",
                  "Apcupsd service", MB_OK);
       return;
    }

    // Set up some standard service state values
    m_srvstatus.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;
    m_srvstatus.dwServiceSpecificExitCode = 0;

    // Give this status to the SCM
    if (!ReportStatus(
            SERVICE_START_PENDING,    // service state
            NO_ERROR,                 // exit code
            45000)) {                 // wait hint
        log_error_message("ReportStatus STOPPED failed 1"); 
        return;
    }

    // Now start the service working thread
    CreateThread(NULL, 0, ServiceWorkThread, NULL, 0, NULL);
}

// SERVICE START ROUTINE - thread that calls ApcupsdAppMain
// NT/Win2K/WinXP ONLY !!!
DWORD WINAPI upsService::ServiceWorkThread(LPVOID lpwThreadParam)
{
    // report the status to the service control manager.
    if (!ReportStatus(
          SERVICE_RUNNING,       // service state
          NO_ERROR,              // exit code
          0)) {                  // wait hint
       MessageBox(NULL, "Report Service failure", "Apcupsd Service", MB_OK);
       log_error_message("ReportStatus RUNNING failed"); 
       return 0;
    }

    // Save the current thread identifier
    m_servicethread = GetCurrentThreadId();

    /* Call Apcupsd main code */
    ApcupsdAppMain(1);

    /* Mark that we're no longer running */
    m_servicethread = 0;

    /* Tell the service manager that we've stopped */
    ReportStatus(SERVICE_STOPPED, 0, 0);
    return 0;
}

// SERVICE STOP ROUTINE - NT/Win2K/WinXP ONLY !!!
void upsService::ServiceStop()
{
   ApcupsdTerminate();
}

// SERVICE INSTALL ROUTINE
int upsService::InstallService(bool quiet)
{
   const int MAXPATH = 2048;

   // Get the filename of this executable
   char path[MAXPATH];
   if (GetModuleFileName(NULL, path, MAXPATH) == 0) {
      if (!quiet) {
         MessageBox(NULL,
                    "Unable to install Apcupsd service", SERVICE_NAME,
                    MB_ICONEXCLAMATION | MB_OK);
      }
      return 0;
   }

   // Append the service-start flag to the end of the path
   // Length is path len plus quotes, space, start flag, and NUL terminator
   char servicecmd[MAXPATH];
   if (strlen(path) + 4 + strlen(ApcupsdRunService) < MAXPATH) {
      sprintf(servicecmd, "\"%s\" %s", path, ApcupsdRunService);
   } else {
      if (!quiet) {
         MessageBox(NULL,
                    "Service command length too long. Service not registered.",
                    SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
      }
      return 0;
   }

   // How to add the Apcupsd service depends upon the OS
   switch (g_os_version_info.dwPlatformId) {

   // Windows 95/98/Me
   case VER_PLATFORM_WIN32_WINDOWS:
      // Locate the RunService registry entry
      HKEY runservices;
      if (RegCreateKey(HKEY_LOCAL_MACHINE, 
              "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
              &runservices) != ERROR_SUCCESS) {
         log_error_message("Cannot write System Registry"); 
         MessageBox(NULL, _("The System Registry could not be updated - "
                            "the Apcupsd service was not installed"),
                    SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
         break;
      }

      // Attempt to add a Apcupsd key
      if (RegSetValueEx(runservices, SERVICE_NAME, 0, REG_SZ,
            (unsigned char *)servicecmd, strlen(servicecmd)+1) != ERROR_SUCCESS) {
         RegCloseKey(runservices);
         MessageBox(NULL, "The Apcupsd service could not be installed",
                    SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
         break;
      }

      RegCloseKey(runservices);

      // Indicate that we're installed to run as a service
      SetServiceFlag(1);

      // We have successfully installed the service!
      if (!quiet) {
         MessageBox(NULL,
                    _("The Apcupsd UPS service was successfully installed.\n"
                      "The service may be started by double clicking on the\n"
                      "Apcupsd \"Start\" icon and will automatically\n"
                      "be run the next time this machine is rebooted. "),
                    SERVICE_NAME, MB_ICONINFORMATION | MB_OK);
      }
      break;

   // Windows NT, Win2K, WinXP
   case VER_PLATFORM_WIN32_NT:
      // Open the default, local Service Control Manager database
      SC_HANDLE hsrvmanager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
      if (hsrvmanager == NULL) {
         MessageBox(NULL,
            _("The Service Control Manager could not be contacted - "
              "the Apcupsd service was not installed"),
            SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
         break;
      }

      // Create an entry for the Apcupsd service
      SC_HANDLE hservice = CreateService(
              hsrvmanager,                    // SCManager database
              SERVICE_NAME,                   // name of service
              SERVICE_DISPLAYNAME,            // name to display
              SERVICE_ALL_ACCESS,             // desired access
              SERVICE_WIN32_OWN_PROCESS |     // service type
                 SERVICE_INTERACTIVE_PROCESS,
              SERVICE_AUTO_START,             // start type
              SERVICE_ERROR_NORMAL,           // error control type
              servicecmd,                     // service's binary
              NULL,                           // no load ordering group
              NULL,                           // no tag identifier
              SERVICE_DEPENDENCIES,           // dependencies
              NULL,                           // LocalSystem account
              NULL);                          // no password
      if (hservice == NULL) {
         if (!quiet || GetLastError() != ERROR_SERVICE_EXISTS) {
            MessageBox(NULL,
                       "The Apcupsd service could not be installed",
                       SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
         }
         CloseServiceHandle(hsrvmanager);
         break;
      }

      SetServiceDescription(hservice,
         _("Apcupsd provides shutdown of your computer in the "
           "event of a power failure."));

      CloseServiceHandle(hservice);
      CloseServiceHandle(hsrvmanager);

      // Indicate that we're installed to run as a service
      SetServiceFlag(1);

      // Everything went fine
      if (!quiet) {
         MessageBox(NULL,
              _("The Apcupsd UPS service was successfully installed.\n"
                "The service may be started from the Control Panel and will\n"
                "automatically be run the next time this machine is rebooted."),
              SERVICE_NAME,
              MB_ICONINFORMATION | MB_OK);
      }
      break;

   default:
      MessageBox(NULL, 
                 _("Unknown Windows operating system.\n"     
                 "Cannot install Apcupsd service.\n"),
                 SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
       break;     
   }

   return 0;
}


// SERVICE REMOVE ROUTINE
int upsService::RemoveService(bool quiet)
{
   // How to remove the Apcupsd service depends upon the OS
   switch (g_os_version_info.dwPlatformId) {

   // Windows 95/98/Me
   case VER_PLATFORM_WIN32_WINDOWS:
      // Locate the RunService registry entry
      HKEY runservices;
      if (RegOpenKey(HKEY_LOCAL_MACHINE, 
              "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
              &runservices) != ERROR_SUCCESS) {
         if (!quiet) {
            MessageBox(NULL, 
                       _("Could not find registry entry.\n"
                         "Service probably not registerd - "
                         "the Apcupsd service was not removed"),
                       SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
         }
      } else {
         // Attempt to delete the Apcupsd key
         if (RegDeleteValue(runservices, SERVICE_NAME) != ERROR_SUCCESS) {
            if (!quiet) {
               MessageBox(NULL, _("Could not delete Registry key.\n"
                                  "The Apcupsd service could not be removed"),
                          SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
            }
         }

         RegCloseKey(runservices);
         break;
      }

      // Try to kill any running copy of Apcupsd
      ApcupsdTerminate();

      // Indicate that we're no longer installed to run as a service
      SetServiceFlag(0);

      // We have successfully removed the service!
      if (!quiet) {
         MessageBox(NULL, "The Apcupsd service has been removed",
                    SERVICE_NAME, MB_ICONINFORMATION | MB_OK);
      }
      break;

   // Windows NT, Win2K, WinXP
   case VER_PLATFORM_WIN32_NT:
      SC_HANDLE hservice = OpenNTService();
      if (!StopNTService(hservice)) {
         // Service could not be stopped
         MessageBox(NULL, "The Apcupsd service could not be stopped",
                    SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
      }

      if(DeleteService(hservice)) {
         // Indicate that we're no longer installed to run as a service
         SetServiceFlag(0);

         // Service successfully removed
         if (!quiet) {
            MessageBox(NULL, "The Apcupsd service has been removed",
                       SERVICE_NAME, MB_ICONINFORMATION | MB_OK);
         }
      } else {
         // Failed to remove
         MessageBox(NULL, "The Apcupsd service could not be removed",
                    SERVICE_NAME, MB_ICONEXCLAMATION | MB_OK);
      }

      CloseServiceHandle(hservice);
      break;
   }

   return 0;
}

// USEFUL SERVICE SUPPORT ROUTINES

// Service control routine
void WINAPI upsService::ServiceCtrl(DWORD ctrlcode)
{
    // What control code have we been sent?
    switch(ctrlcode) {
    case SERVICE_CONTROL_STOP:
        // STOP : The service must stop
        m_srvstatus.dwCurrentState = SERVICE_STOP_PENDING;
        ServiceStop();
        break;

    case SERVICE_CONTROL_INTERROGATE:
        // QUERY : Service control manager just wants to know our state
        break;

     default:
        // Control code not recognised
        break;
    }

    // Tell the control manager what we're up to.
    ReportStatus(m_srvstatus.dwCurrentState, NO_ERROR, 0);
}

// Service manager status reporting
BOOL upsService::ReportStatus(DWORD state,
                              DWORD exitcode,
                              DWORD waithint)
{
    static DWORD checkpoint = 1;

    // If we're in the start state then we don't want the control manager
    // sending us control messages because they'll confuse us.
    if (state == SERVICE_START_PENDING)
       m_srvstatus.dwControlsAccepted = 0;
    else
       m_srvstatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    // Save the new status we've been given
    m_srvstatus.dwCurrentState = state;
    m_srvstatus.dwWin32ExitCode = exitcode;
    m_srvstatus.dwWaitHint = waithint;

    // Update the checkpoint variable to let the SCM know that we
    // haven't died if requests take a long time
    if ((state == SERVICE_RUNNING) || (state == SERVICE_STOPPED))
       m_srvstatus.dwCheckPoint = 0;
    else
       m_srvstatus.dwCheckPoint = checkpoint++;

    // Tell the SCM our new status
    BOOL result = SetServiceStatus(m_hstatus, &m_srvstatus);
    if (!result)
       log_error_message("SetServiceStatus failed");

    return result;
}

// Error reporting
void LogErrorMsg(char *message, char *fname, int lineno)
{
   // Get the error code
   LPTSTR msg;
   DWORD error = GetLastError();
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL,
                 error,
                 0,
                 (LPTSTR)&msg,
                 0,
                 NULL);

   // Use event logging to log the error
   HANDLE heventsrc = RegisterEventSource(NULL, SERVICE_NAME);
   if (heventsrc == NULL)
      return;

   char msgbuff[256];
   snprintf(msgbuff, sizeof(msgbuff), "\n\n%s error: %ld at %s:%d", 
      SERVICE_NAME, error, fname, lineno);

   char *strings[3];
   strings[0] = msgbuff;
   strings[1] = message;
   strings[2] = msg;

   ReportEvent(heventsrc,              // handle of event source
               EVENTLOG_ERROR_TYPE,    // event type
               0,                      // event category
               0,                      // event ID
               NULL,                   // current user's SID
               3,                      // strings in 'strings'
               0,                      // no bytes of raw data
               (const char **)strings, // array of error strings
               NULL);                  // no raw data

   DeregisterEventSource(heventsrc);
   LocalFree(msg);
}

void upsService::SetServiceDescription(SC_HANDLE hService, LPSTR lpDesc) 
{ 
   HINSTANCE hLib = LoadLibrary("ADVAPI32.DLL");
   if (!hLib)
      return;

   ChangeServiceConfig2Func ChangeServiceConfig2 =
      (ChangeServiceConfig2Func)GetProcAddress(hLib, "ChangeServiceConfig2A");
   if (!ChangeServiceConfig2) {
      FreeLibrary(hLib);
      return;
   }

   SERVICE_DESCRIPTION sdBuf;
   sdBuf.lpDescription = lpDesc;

   ChangeServiceConfig2(
      hService,                   // handle to service
      SERVICE_CONFIG_DESCRIPTION, // change: description
      &sdBuf);                    // value: new description

   FreeLibrary(hLib);
}

void upsService::SetServiceFlag(DWORD flag)
{
   // Create or open HKLM\Software\Apcupsd key
   HKEY apcupsd;
   RegCreateKey(HKEY_LOCAL_MACHINE, "Software\\Apcupsd", &apcupsd);

   // Add InstalledService value
   RegSetValueEx(
      apcupsd, "InstalledService", 0, REG_DWORD, (BYTE*)&flag, sizeof(flag));
}

BOOL upsService::StopNTService(SC_HANDLE hservice)
{
   // Try to stop the Apcupsd service
   SERVICE_STATUS status;
   status.dwCurrentState = SERVICE_RUNNING;
   if (ControlService(hservice, SERVICE_CONTROL_STOP, &status)) {
      while(QueryServiceStatus(hservice, &status)) {
         if (status.dwCurrentState == SERVICE_STOP_PENDING) {
            Sleep(1000);
         } else {
            break;
         }
      }
   }

   return status.dwCurrentState == SERVICE_STOPPED;
}

SC_HANDLE upsService::OpenNTService()
{
   // Open the SCM
   SC_HANDLE hscm = OpenSCManager(
      NULL,                   // machine (NULL == local)
      NULL,                   // database (NULL == default)
      SC_MANAGER_ALL_ACCESS); // access required
   if (hscm == NULL) {
      return NULL;
   }

   // Open the service
   SC_HANDLE hservice = OpenService(hscm, SERVICE_NAME, SERVICE_ALL_ACCESS);

   // Close SCM and return service handle
   CloseServiceHandle(hscm);
   return hservice;
}
