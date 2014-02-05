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

#ifndef __WINAPI_H
#define __WINAPI_H

#include <windows.h>
#include <windef.h>
#include <accctrl.h>
#include <aclapi.h>

// OS version enumeration
// Keep these in order so >= comparisons work
typedef enum
{
   WINDOWS_95,
   WINDOWS_98,
   WINDOWS_ME,
   WINDOWS_NT,
   WINDOWS_2000,
   WINDOWS_XP,
   WINDOWS_2003,
   WINDOWS_VISTA
} OSVERSION;

/* Platform version info */
extern OSVERSIONINFO g_os_version_info;
extern OSVERSION g_os_version;

// unicode enabling of win 32 needs some defines and functions

// using an average of 3 bytes per character is probably fine in
// practice but I believe that Windows actually uses UTF-16 encoding
// as opposed to UCS2 which means characters 0x10000-0x10ffff are
// valid and result in 4 byte UTF-8 encodings.
#define MAX_PATH_UTF8    MAX_PATH*4  // strict upper bound on UTF-16 to UTF-8 conversion
// from
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/fileio/fs/getfileattributesex.asp
// In the ANSI version of this function, the name is limited to
// MAX_PATH characters. To extend this limit to 32,767 wide
// characters, call the Unicode version of the function and prepend
// "\\?\" to the path. For more information, see Naming a File.
#define MAX_PATH_W 32767

/* In ADVAPI32.DLL */

typedef BOOL (WINAPI * t_OpenProcessToken)(HANDLE, DWORD, PHANDLE);
typedef BOOL (WINAPI * t_AdjustTokenPrivileges)(HANDLE, BOOL,
          PTOKEN_PRIVILEGES, DWORD, PTOKEN_PRIVILEGES, PDWORD);
typedef BOOL (WINAPI * t_LookupPrivilegeValue)(LPCTSTR, LPCTSTR, PLUID);

extern t_OpenProcessToken      p_OpenProcessToken;
extern t_AdjustTokenPrivileges p_AdjustTokenPrivileges;
extern t_LookupPrivilegeValue  p_LookupPrivilegeValue;

/* In KERNEL32.DLL */
typedef BOOL (WINAPI * t_GetFileAttributesExA)(LPCSTR, GET_FILEEX_INFO_LEVELS,
       LPVOID);
typedef DWORD (WINAPI * t_GetFileAttributesA)(LPCSTR);
typedef BOOL (WINAPI * t_SetFileAttributesA)(LPCSTR, DWORD);
typedef HANDLE (WINAPI * t_CreateFileA) (LPCSTR, DWORD ,DWORD, LPSECURITY_ATTRIBUTES,
        DWORD , DWORD, HANDLE);
typedef BOOL (WINAPI * t_CreateDirectoryA) (LPCSTR, LPSECURITY_ATTRIBUTES);
typedef BOOL (WINAPI * t_SetProcessShutdownParameters)(DWORD, DWORD);
typedef HANDLE (WINAPI * t_FindFirstFileA) (LPCSTR, LPWIN32_FIND_DATAA);
typedef BOOL (WINAPI * t_FindNextFileA) (HANDLE, LPWIN32_FIND_DATAA);
typedef BOOL (WINAPI * t_SetCurrentDirectoryA) (LPCSTR);
typedef DWORD (WINAPI * t_GetCurrentDirectoryA) (DWORD, LPSTR);
  
extern t_GetFileAttributesA   p_GetFileAttributesA;
extern t_GetFileAttributesExA   p_GetFileAttributesExA;
extern t_SetFileAttributesA   p_SetFileAttributesA;
extern t_CreateFileA   p_CreateFileA;
extern t_CreateDirectoryA   p_CreateDirectoryA;
extern t_SetProcessShutdownParameters p_SetProcessShutdownParameters;
extern t_FindFirstFileA p_FindFirstFileA;
extern t_FindNextFileA p_FindNextFileA;
extern t_SetCurrentDirectoryA p_SetCurrentDirectoryA;
extern t_GetCurrentDirectoryA p_GetCurrentDirectoryA;

void InitWinAPIWrapper();

bool GrantAccess(HANDLE h, ACCESS_MASK access, TRUSTEE_TYPE type, LPTSTR name);

#endif /* __WINAPI_H */
