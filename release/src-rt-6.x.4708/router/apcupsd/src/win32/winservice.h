// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//


// winservice.cpp

// SERVICE-MODE CODE

// This class provides access to service-oriented routines, under both
// Windows NT and Windows 95.  Some routines only operate under one
// OS, others operate under any OS.

class upsService;

#if (!defined(_win_upsSERVICE))
#define _win_upsSERVICE

// The NT-specific code wrapper class
class upsService
{
public:
   upsService();

   // INSTALL & START FUNCTIONS

   // Routine called by WinMain to cause Apcupsd to be installed
   // as a service.
   static int ApcupsdServiceMain();

   // Routine to install the Apcupsd service on the local machine
   static int InstallService(bool quiet);

   // Routine to remove the Apcupsd service from the local machine
   static int RemoveService(bool quiet);

   // Stop the service
   static void ServiceStop();


   // SERVICE OPERATION FUNCTIONS

   // SCM callbacks
   static void WINAPI ServiceMain(DWORD argc, char **argv);
   static void WINAPI ServiceCtrl(DWORD ctrlcode);

   // Thread on which service processing will take place
   static DWORD WINAPI ServiceWorkThread(LPVOID lpwThreadParam);


   // SUPPORT FUNCTIONS

   // Report status to the SCM
   static BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);

   // Set the service's description text
   static void SetServiceDescription(SC_HANDLE hService, LPSTR lpDesc);

   // Set registry value to indicate if we're installed to run as a service
   static void SetServiceFlag(DWORD flag);

   // Stop an NT service with the given handle
   static BOOL StopNTService(SC_HANDLE hservice);

   // Open the Apcupsd NT service
   static SC_HANDLE OpenNTService();

   
   // INTERNAL DATA

   static SERVICE_STATUS         m_srvstatus;
   static SERVICE_STATUS_HANDLE  m_hstatus;
   static DWORD                  m_servicethread;
};

#endif
