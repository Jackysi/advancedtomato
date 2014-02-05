/*
 * Windows APIs that are different for each system.
 *   We use pointers to the entry points so that a
 *   single binary will run on all Windows systems.
 *
 *     Kern Sibbald MMIII
 */
/*
   Copyright (C) 2003-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "apc.h"
#include "winapi.h"

/* Platform version info */
OSVERSIONINFO g_os_version_info;
OSVERSION g_os_version;

/* API Pointers */
t_OpenProcessToken      p_OpenProcessToken = NULL;
t_AdjustTokenPrivileges p_AdjustTokenPrivileges = NULL;
t_LookupPrivilegeValue  p_LookupPrivilegeValue = NULL;
t_SetProcessShutdownParameters p_SetProcessShutdownParameters = NULL;
t_CreateFileA   p_CreateFileA = NULL;
t_CreateDirectoryA   p_CreateDirectoryA;
t_GetFileAttributesA    p_GetFileAttributesA = NULL;
t_GetFileAttributesExA  p_GetFileAttributesExA = NULL;
t_SetFileAttributesA    p_SetFileAttributesA = NULL;
t_FindFirstFileA p_FindFirstFileA = NULL;
t_FindNextFileA p_FindNextFileA = NULL;
t_SetCurrentDirectoryA p_SetCurrentDirectoryA = NULL;
t_GetCurrentDirectoryA p_GetCurrentDirectoryA = NULL;

void 
InitWinAPIWrapper() 
{
   HMODULE hLib = LoadLibraryA("KERNEL32.DLL");
   if (hLib) {
      /* create file calls */
      p_CreateFileA = (t_CreateFileA)
          GetProcAddress(hLib, "CreateFileA");

      p_CreateDirectoryA = (t_CreateDirectoryA)
          GetProcAddress(hLib, "CreateDirectoryA");

      /* attribute calls */
      p_GetFileAttributesA = (t_GetFileAttributesA)
          GetProcAddress(hLib, "GetFileAttributesA");
      p_GetFileAttributesExA = (t_GetFileAttributesExA)
          GetProcAddress(hLib, "GetFileAttributesExA");
      p_SetFileAttributesA = (t_SetFileAttributesA)
          GetProcAddress(hLib, "SetFileAttributesA");
      /* process calls */
      p_SetProcessShutdownParameters = (t_SetProcessShutdownParameters)
          GetProcAddress(hLib, "SetProcessShutdownParameters");

      /* find files */
      p_FindFirstFileA = (t_FindFirstFileA)
          GetProcAddress(hLib, "FindFirstFileA"); 
      p_FindNextFileA = (t_FindNextFileA)
          GetProcAddress(hLib, "FindNextFileA");
      /* set and get directory */
      p_SetCurrentDirectoryA = (t_SetCurrentDirectoryA)
          GetProcAddress(hLib, "SetCurrentDirectoryA");
      p_GetCurrentDirectoryA = (t_GetCurrentDirectoryA)
          GetProcAddress(hLib, "GetCurrentDirectoryA");

      FreeLibrary(hLib);
   }
   
   hLib = LoadLibraryA("ADVAPI32.DLL");
   if (hLib) {
      p_OpenProcessToken = (t_OpenProcessToken)
         GetProcAddress(hLib, "OpenProcessToken");
      p_AdjustTokenPrivileges = (t_AdjustTokenPrivileges)
         GetProcAddress(hLib, "AdjustTokenPrivileges");
      p_LookupPrivilegeValue = (t_LookupPrivilegeValue)
         GetProcAddress(hLib, "LookupPrivilegeValueA");
      FreeLibrary(hLib);
   }

   // Get the current OS version
   memset(&g_os_version_info, 0, sizeof(g_os_version_info));
   g_os_version_info.dwOSVersionInfoSize = sizeof(g_os_version_info);
   GetVersionEx(&g_os_version_info);

   // Convert OS version to ordered enumeration
   if (g_os_version_info.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
   {
      // VER_PLATFORM_WIN32_WINDOWS...
      //
      //   WINDOWS 95: 4.0
      //   WINDOWS 98: 4.10
      //   WINDOWS ME: 4.90
      //
      switch (g_os_version_info.dwMinorVersion)
      {
      case 0:
         g_os_version = WINDOWS_95;
         break;
      case 10:
         g_os_version = WINDOWS_98;
         break;
      default:
      case 90:
         g_os_version = WINDOWS_ME;
         break;
      }
   }
   else // if (g_os_version_info.dwPlatformId == VER_PLATFORM_WIN32_NT)
   {
      // VER_PLATFORM_WIN32_NT...
      //
      //   WINDOWS NT:    4.0
      //   WINDOWS 2000:  5.0
      //   WINDOWS XP:    5.1
      //   WINDOWS 2003:  5.2
      //   WINDOWS VISTA: 6.0
      //
      switch (g_os_version_info.dwMajorVersion)
      {
      case 4:
         g_os_version = WINDOWS_NT;
         break;
      case 5:
         switch (g_os_version_info.dwMinorVersion)
         {
         case 0:
            g_os_version = WINDOWS_2000;
            break;
         case 1:
            g_os_version = WINDOWS_XP;
            break;
         default:
         case 2:
            g_os_version = WINDOWS_2003;
            break;
         }
         break;
      default:
      case 6:
         g_os_version = WINDOWS_VISTA;
         break;
      }
   }
}

// Add the requested access to the given kernel object handle
bool GrantAccess(HANDLE h, ACCESS_MASK access, TRUSTEE_TYPE type, LPTSTR name)
{
   DWORD rc;

   // Obtain current DACL from object
   ACL *dacl;
   SECURITY_DESCRIPTOR *sd;
   rc = GetSecurityInfo(h, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 
                        NULL, NULL, &dacl, NULL, &sd);
   if (rc != ERROR_SUCCESS)
      return false;

   // Add requested access to DACL
   EXPLICIT_ACCESS ea;
   ea.grfAccessPermissions = access;
   ea.grfAccessMode = GRANT_ACCESS;
   ea.grfInheritance = NO_INHERITANCE;
   ea.Trustee.pMultipleTrustee = FALSE;
   ea.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
   ea.Trustee.TrusteeForm = TRUSTEE_IS_NAME;
   ea.Trustee.TrusteeType = type;
   ea.Trustee.ptstrName = name;
   ACL *newdacl;
   rc = SetEntriesInAcl(1, &ea, dacl, &newdacl);
   if (rc != ERROR_SUCCESS) {
      LocalFree(sd);
      return false;
   }

   // Set new DACL on object
   rc = SetSecurityInfo(h, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 
                        NULL, NULL, newdacl, NULL);

   // Done with structs
   LocalFree(newdacl);
   LocalFree(sd);

   return rc == ERROR_SUCCESS;
}
