//                              -*- Mode: C++ -*-
// compat.cpp -- compatibilty layer to make bacula-fd run
//               natively under windows
//
// Copyright transferred from Raider Solutions, Inc to
//   Kern Sibbald and John Walker by express permission.
//
//  Copyright (C) 2004-2006 Kern Sibbald
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  version 2 as amended with additional clauses defined in the
//  file LICENSE in the main source directory.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  the file LICENSE for additional details.
//
// Author          : Christopher S. Hull
// Created On      : Sat Jan 31 15:55:00 2004
// $Id: compat.cpp,v 1.22.2.5 2010/09/10 14:50:12 adk0212 Exp $

#include "apc.h"
#include "compat.h"
#include "winapi.h"

#define b_errno_win32 (1<<29)

/* apcupsd doesn't need special allocators */
#define get_pool_memory(x) (char *)malloc(x)
#define free_pool_memory(x) free((char *)x)
#define check_pool_memory_size(x, y) x
#define PM_FNAME 2000
#define PM_MESSAGE 2000

/* No assertion checking */
#define ASSERT(x) 

/* to allow the usage of the original version in this file here */
#undef fputs

#define USE_WIN32_COMPAT_IO 1
#define USE_WIN32_32KPATHCONVERSION 1

extern void d_msg(const char *file, int line, int level, const char *fmt,...);

// from MicroSoft SDK (KES) is the diff between Jan 1 1601 and Jan 1 1970
#define WIN32_FILETIME_ADJUST 0x19DB1DED53E8000ULL 
#define WIN32_FILETIME_SCALE  10000000             // 100ns/second

void conv_unix_to_win32_path(const char *name, char *win32_name, DWORD dwSize)
{
    const char *fname = name;
    char *tname = win32_name;
    while (*name) {
        /* Check for Unix separator and convert to Win32 */
        if (name[0] == '/' && name[1] == '/') {  /* double slash? */
           name++;                               /* yes, skip first one */
        }
        if (*name == '/') {
            *win32_name++ = '\\';     /* convert char */
        /* If Win32 separated that is "quoted", remove quote */
        } else if (*name == '\\' && name[1] == '\\') {
            *win32_name++ = '\\';
            name++;                   /* skip first \ */
        } else {
            *win32_name++ = *name;    /* copy character */
        }
        name++;
    }
    /* Strip any trailing slash, if we stored something */
    /* but leave "c:\" with backslash (root directory case */
    if (*fname != 0 && win32_name[-1] == '\\' && strlen (fname) != 3) {
        win32_name[-1] = 0;
    } else {
        *win32_name = 0;
    }
}

int umask(int)
{
   return 0;
}

int chmod(const char *, mode_t)
{
   return 0;
}

int chown(const char *k, uid_t, gid_t)
{
   return 0;
}

int lchown(const char *k, uid_t, gid_t)
{
   return 0;
}

long int
random(void)
{
    return rand();
}

void
srandom(unsigned int seed)
{
   srand(seed);
}

// /////////////////////////////////////////////////////////////////
// convert from Windows concept of time to Unix concept of time
// /////////////////////////////////////////////////////////////////
void
cvt_utime_to_ftime(const time_t  &time, FILETIME &wintime)
{
    uint64_t mstime = time;
    mstime *= WIN32_FILETIME_SCALE;
    mstime += WIN32_FILETIME_ADJUST;

    #ifdef HAVE_MINGW
    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffUL);
    #else
    wintime.dwLowDateTime = (DWORD)(mstime & 0xffffffffI64);
    #endif
    wintime.dwHighDateTime = (DWORD) ((mstime>>32)& 0xffffffffUL);
}

time_t
cvt_ftime_to_utime(const FILETIME &time)
{
    uint64_t mstime = time.dwHighDateTime;
    mstime <<= 32;
    mstime |= time.dwLowDateTime;

    mstime -= WIN32_FILETIME_ADJUST;
    mstime /= WIN32_FILETIME_SCALE; // convert to seconds.

    return (time_t) (mstime & 0xffffffff);
}

static const char *
errorString(void)
{
   LPVOID lpMsgBuf;

   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL,
                 GetLastError(),
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default lang
                 (LPTSTR) &lpMsgBuf,
                 0,
                 NULL);

   /* Strip any \r or \n */
   char *rval = (char *) lpMsgBuf;
   char *cp = strchr(rval, '\r');
   if (cp != NULL) {
      *cp = 0;
   } else {
      cp = strchr(rval, '\n');
      if (cp != NULL)
         *cp = 0;
   }
   return rval;
}


static int
statDir(const char *file, struct stat *sb)
{
   WIN32_FIND_DATAW info_w;       // window's file info
   WIN32_FIND_DATAA info_a;       // window's file info

   // cache some common vars to make code more transparent
   DWORD* pdwFileAttributes;
   DWORD* pnFileSizeHigh;
   DWORD* pnFileSizeLow;
   FILETIME* pftLastAccessTime;
   FILETIME* pftLastWriteTime;
   FILETIME* pftCreationTime;

   if (file[1] == ':' && file[2] == 0) {
        d_msg(__FILE__, __LINE__, 99, "faking ROOT attrs(%s).\n", file);
        sb->st_mode = S_IFDIR;
        sb->st_mode |= S_IREAD|S_IEXEC|S_IWRITE;
        time(&sb->st_ctime);
        time(&sb->st_mtime);
        time(&sb->st_atime);
        return 0;
    }

   HANDLE h = INVALID_HANDLE_VALUE;

   // use unicode or ascii
   if (p_FindFirstFileA) {
      h = p_FindFirstFileA(file, &info_a);

      pdwFileAttributes = &info_a.dwFileAttributes;
      pnFileSizeHigh    = &info_a.nFileSizeHigh;
      pnFileSizeLow     = &info_a.nFileSizeLow;
      pftLastAccessTime = &info_a.ftLastAccessTime;
      pftLastWriteTime  = &info_a.ftLastWriteTime;
      pftCreationTime   = &info_a.ftCreationTime;
   }

    if (h == INVALID_HANDLE_VALUE) {
      const char *err = errorString();
      d_msg(__FILE__, __LINE__, 99, "FindFirstFile(%s):%s\n", file, err);
      LocalFree((void *)err);
      errno = b_errno_win32;
      return -1;
   }

   sb->st_mode = 0777;               /* start with everything */
   if (*pdwFileAttributes & FILE_ATTRIBUTE_READONLY)
      sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
   if (*pdwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
      sb->st_mode &= ~S_IRWXO; /* remove everything for other */
   if (*pdwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
      sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
   sb->st_mode |= S_IFDIR;

   sb->st_size = *pnFileSizeHigh;
   sb->st_size <<= 32;
   sb->st_size |= *pnFileSizeLow;
   sb->st_blksize = 4096;
   sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;

   sb->st_atime = cvt_ftime_to_utime(*pftLastAccessTime);
   sb->st_mtime = cvt_ftime_to_utime(*pftLastWriteTime);
   sb->st_ctime = cvt_ftime_to_utime(*pftCreationTime);
   FindClose(h);

   return 0;
}

static int
stat2(const char *file, struct stat *sb)
{
    BY_HANDLE_FILE_INFORMATION info;
    HANDLE h;
    int rval = 0;

    DWORD attr = (DWORD)-1;

    if (p_GetFileAttributesA) {
       attr = p_GetFileAttributesA(file);
    }

    if (attr == -1) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "GetFileAttributes(%s): %s\n", file, err);
        LocalFree((void *)err);
        errno = b_errno_win32;
        return -1;
    }

    if (attr & FILE_ATTRIBUTE_DIRECTORY)
        return statDir(file, sb);

    h = CreateFileA(file, GENERIC_READ,
                   FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "Cannot open file for stat (%s):%s\n", file, err);
        LocalFree((void *)err);
        rval = -1;
        errno = b_errno_win32;
        goto error;
    }

    if (!GetFileInformationByHandle(h, &info)) {
        const char *err = errorString();
        d_msg(__FILE__, __LINE__, 99,
              "GetfileInformationByHandle(%s): %s\n", file, err);
        LocalFree((void *)err);
        rval = -1;
        errno = b_errno_win32;
        goto error;
    }

    sb->st_dev = info.dwVolumeSerialNumber;
    sb->st_ino = info.nFileIndexHigh;
    sb->st_ino <<= 32;
    sb->st_ino |= info.nFileIndexLow;
    sb->st_nlink = (short)info.nNumberOfLinks;
    if (sb->st_nlink > 1) {
       d_msg(__FILE__, __LINE__, 99,  "st_nlink=%d\n", sb->st_nlink);
    }

    sb->st_mode = 0777;               /* start with everything */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
    if (info.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM)
        sb->st_mode &= ~S_IRWXO; /* remove everything for other */
    if (info.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
        sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
    sb->st_mode |= S_IFREG;

    sb->st_size = info.nFileSizeHigh;
    sb->st_size <<= 32;
    sb->st_size |= info.nFileSizeLow;
    sb->st_blksize = 4096;
    sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;
    sb->st_atime = cvt_ftime_to_utime(info.ftLastAccessTime);
    sb->st_mtime = cvt_ftime_to_utime(info.ftLastWriteTime);
    sb->st_ctime = cvt_ftime_to_utime(info.ftCreationTime);

error:
    CloseHandle(h);
    return rval;
}

int
stat(const char *file, struct stat *sb)
{
   WIN32_FILE_ATTRIBUTE_DATA data;
   errno = 0;

   memset(sb, 0, sizeof(*sb));

   if (p_GetFileAttributesExA) {
      if (!p_GetFileAttributesExA(file, GetFileExInfoStandard, &data)) {
         return stat2(file, sb);
       }
   } else {
      return stat2(file, sb);
   }

   sb->st_mode = 0777;               /* start with everything */
   if (data.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
      sb->st_mode &= ~(S_IRUSR|S_IRGRP|S_IROTH);
   }
   if (data.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
      sb->st_mode &= ~S_IRWXO; /* remove everything for other */
   }
   if (data.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
      sb->st_mode |= S_ISVTX; /* use sticky bit -> hidden */
   }
   if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      sb->st_mode |= S_IFDIR;
   } else {
      sb->st_mode |= S_IFREG;
   }

   sb->st_nlink = 1;
   sb->st_size = data.nFileSizeHigh;
   sb->st_size <<= 32;
   sb->st_size |= data.nFileSizeLow;
   sb->st_blksize = 4096;
   sb->st_blocks = (uint32_t)(sb->st_size + 4095)/4096;
   sb->st_atime = cvt_ftime_to_utime(data.ftLastAccessTime);
   sb->st_mtime = cvt_ftime_to_utime(data.ftLastWriteTime);
   sb->st_ctime = cvt_ftime_to_utime(data.ftCreationTime);
   return 0;
}

int
lstat(const char *file, struct stat *sb)
{
   return stat(file, sb);
}

void
sleep(int sec)
{
   Sleep(sec * 1000);
}

int
geteuid(void)
{
   return 0;
}

int
execvp(const char *, char *[]) {
   errno = ENOSYS;
   return -1;
}


int
fork(void)
{
   errno = ENOSYS;
   return -1;
}

int
waitpid(int, int*, int)
{
   errno = ENOSYS;
   return -1;
}

int
readlink(const char *, char *, int)
{
   errno = ENOSYS;
   return -1;
}

int
strncasecmp(const char *s1, const char *s2, int len)
{
    register int ch1, ch2;

    if (s1==s2)
        return 0;       /* strings are equal if same object. */
    else if (!s1)
        return -1;
    else if (!s2)
        return 1;
    while (len--) {
        ch1 = *s1;
        ch2 = *s2;
        s1++;
        s2++;
        if (ch1 == 0 || tolower(ch1) != tolower(ch2)) break;
    }

    return (ch1 - ch2);
}

int
gettimeofday(struct timeval *tv, struct timezone *)
{
    SYSTEMTIME now;
    FILETIME tmp;
    GetSystemTime(&now);

    if (tv == NULL) {
       errno = EINVAL;
       return -1;
    }
    if (!SystemTimeToFileTime(&now, &tmp)) {
       errno = b_errno_win32;
       return -1;
    }

    int64_t _100nsec = tmp.dwHighDateTime;
    _100nsec <<= 32;
    _100nsec |= tmp.dwLowDateTime;
    _100nsec -= WIN32_FILETIME_ADJUST;

    tv->tv_sec =(long) (_100nsec / 10000000);
    tv->tv_usec = (long) ((_100nsec % 10000000)/10);
    return 0;

}

void
init_stack_dump(void)
{

}


long
pathconf(const char *path, int name)
{
    switch(name) {
    case _PC_PATH_MAX :
        if (strncmp(path, "\\\\?\\", 4) == 0)
            return 32767;
    case _PC_NAME_MAX :
        return 255;
    }
    errno = ENOSYS;
    return -1;
}

int
kill(int pid, int signal)
{
   int rval = 0;
   DWORD exitcode = 0;

   switch (signal) {
   case SIGTERM:
      /* Terminate the process */
      if (!TerminateProcess((HANDLE)pid, (UINT) signal)) {
         rval = -1;
         errno = b_errno_win32;
      }
      CloseHandle((HANDLE)pid);
      break;
   case 0:
      /* Just check if process is still alive */
      if (GetExitCodeProcess((HANDLE)pid, &exitcode) &&
          exitcode != STILL_ACTIVE) {
         rval = -1;
      }
      break;
   default:
      /* Don't know what to do, so just fail */
      rval = -1;
      errno = EINVAL;
      break;   
   }

   return rval;
}

/* Implement syslog() using Win32 Event Service */
void syslog(int type, const char *fmt, ...)
{
   va_list arg_ptr;
   char message[MAXSTRING];
   HANDLE heventsrc;
   char* strings[32];
   WORD wtype;

   va_start(arg_ptr, fmt);
   message[0] = '\n';
   message[1] = '\n';
   avsnprintf(message+2, sizeof(message)-2, fmt, arg_ptr);
   va_end(arg_ptr);

   strings[0] = message;

   // Convert syslog type to Win32 type. This mapping is somewhat arbitrary
   // since there are many more LOG_* types than EVENTLOG_* types.
   switch (type) {
   case LOG_ERR:
      wtype = EVENTLOG_ERROR_TYPE;
      break;
   case LOG_CRIT:
   case LOG_ALERT:
   case LOG_WARNING:
      wtype = EVENTLOG_WARNING_TYPE;
      break;
   default:
      wtype = EVENTLOG_INFORMATION_TYPE;
      break;
   }

   // Use event logging to log the error
   heventsrc = RegisterEventSource(NULL, "Apcupsd");

   if (heventsrc != NULL) {
      ReportEvent(
              heventsrc,              // handle of event source
              wtype,                  // event type
              0,                      // event category
              0,                      // event ID
              NULL,                   // current user's SID
              1,                      // strings in 'strings'
              0,                      // no bytes of raw data
              (const char **)strings, // array of error strings
              NULL);                  // no raw data

      DeregisterEventSource(heventsrc);
   }
}

/* Convert Win32 baud constants to POSIX constants */
int posixbaud(DWORD baud)
{
   switch(baud) {
   case CBR_110:
      return B110;
   case CBR_300:
      return B300;
   case CBR_600:
      return B600;
   case CBR_1200:
      return B1200;
   case CBR_2400:
   default:
      return B2400;
   case CBR_4800:
      return B4800;
   case CBR_9600:
      return B9600;
   case CBR_19200:
      return B19200;
   case CBR_38400:
      return B38400;
   case CBR_57600:
      return B57600;
   case CBR_115200:
      return B115200;
   case CBR_128000:
      return B128000;
   case CBR_256000:
      return B256000;
   }
}

/* Convert POSIX baud constants to Win32 constants */
DWORD winbaud(int baud)
{
   switch(baud) {
   case B110:
      return CBR_110;
   case B300:
      return CBR_300;
   case B600:
      return CBR_600;
   case B1200:
      return CBR_1200;
   case B2400:
   default:
      return CBR_2400;
   case B4800:
      return CBR_4800;
   case B9600:
      return CBR_9600;
   case B19200:
      return CBR_19200;
   case B38400:
      return CBR_38400;
   case B57600:
      return CBR_57600;
   case B115200:
      return CBR_115200;
   case B128000:
      return CBR_128000;
   case B256000:
      return CBR_256000;
   }
}

/* Convert Win32 bytesize constants to POSIX constants */
int posixsize(BYTE size)
{
   switch(size) {
   case 5:
      return CS5;
   case 6:
      return CS6;
   case 7:
      return CS7;
   case 8:
   default:
      return CS8;
   }
}

/* Convert POSIX bytesize constants to Win32 constants */
BYTE winsize(int size)
{
   switch(size) {
   case CS5:
      return 5;
   case CS6:
      return 6;
   case CS7:
      return 7;
   case CS8:
   default:
      return 8;
   }
}

int tcgetattr (int fd, struct termios *out)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);

   memset(out, 0, sizeof(*out));
   
   out->c_cflag |= posixbaud(dcb.BaudRate);
   out->c_cflag |= posixsize(dcb.ByteSize);

   if (dcb.StopBits == TWOSTOPBITS)
      out->c_cflag |= CSTOPB;
   if (dcb.fParity) {
      out->c_cflag |= PARENB;
      if (dcb.Parity == ODDPARITY)
         out->c_cflag |= PARODD;
   }

   if (!dcb.fOutxCtsFlow && !dcb.fOutxDsrFlow && !dcb.fDsrSensitivity)
      out->c_cflag |= CLOCAL;
      
   if (dcb.fOutX)
      out->c_iflag |= IXON;
   if (dcb.fInX)
      out->c_iflag |= IXOFF;

   return 0;
}

int tcsetattr (int fd, int optional_actions, const struct termios *in)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);

   dcb.fBinary = 1;
   dcb.BaudRate = winbaud(in->c_cflag & CBAUD);
   dcb.ByteSize = winsize(in->c_cflag & CSIZE);
   dcb.StopBits = in->c_cflag & CSTOPB ? TWOSTOPBITS : ONESTOPBIT;

   if (in->c_cflag & PARENB) {
      dcb.fParity = 1;
      dcb.Parity = in->c_cflag & PARODD ? ODDPARITY : EVENPARITY;
   } else {
      dcb.fParity = 0;
      dcb.Parity = NOPARITY;
   }

   if (in->c_cflag & CLOCAL) {
      dcb.fOutxCtsFlow = 0;
      dcb.fOutxDsrFlow = 0;
      dcb.fDsrSensitivity = 0;
   }

   dcb.fOutX = !!(in->c_iflag & IXON);
   dcb.fInX = !!(in->c_iflag & IXOFF);

   SetCommState(h, &dcb);

   /* If caller wants a read() timeout, set that up */
   if (in->c_cc[VMIN] == 0 && in->c_cc[VTIME] != 0) {
      COMMTIMEOUTS ct;
      ct.ReadIntervalTimeout = MAXDWORD;
      ct.ReadTotalTimeoutMultiplier = MAXDWORD;
      ct.ReadTotalTimeoutConstant = in->c_cc[VTIME] * 100;
      ct.WriteTotalTimeoutMultiplier = 0;
      ct.WriteTotalTimeoutConstant = 0;
      SetCommTimeouts(h, &ct);
   }

   return 0;
}

int tcflush(int fd, int queue_selector)
{
   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   DWORD flags = 0;

   switch (queue_selector) {
   case TCIFLUSH:
      flags |= PURGE_RXCLEAR;
      break;
   case TCOFLUSH:
      flags |= PURGE_TXCLEAR;
      break;
   case TCIOFLUSH:
      flags |= PURGE_RXCLEAR;
      flags |= PURGE_TXCLEAR;
      break;
   }
   
   PurgeComm(h, flags);
   return 0;
}

int tiocmbic(int fd, int bits)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);
   
   if (bits & TIOCM_DTR)
      dcb.fDtrControl = DTR_CONTROL_DISABLE;
   if (bits & TIOCM_RTS)
      dcb.fRtsControl = RTS_CONTROL_DISABLE;
   if (bits & TIOCM_ST)
      d_msg(__FILE__, __LINE__, 99, "Win32 API does not allow clearing ST\n");

   SetCommState(h, &dcb);
   return 0;
}

int tiocmbis(int fd, int bits)
{
   DCB dcb;
   dcb.DCBlength = sizeof(DCB);

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }

   GetCommState(h, &dcb);
   
   if (bits & TIOCM_DTR)
      dcb.fDtrControl = DTR_CONTROL_ENABLE;
   if (bits & TIOCM_RTS)
      dcb.fRtsControl = RTS_CONTROL_ENABLE;
   if (bits & TIOCM_SR)
      d_msg(__FILE__, __LINE__, 99, "Win32 API does not allow setting ST\n");

   SetCommState(h, &dcb);
   return 0;
}

int tiocmget(int fd, int *bits)
{
   DWORD status;

   HANDLE h = (HANDLE)_get_osfhandle(fd);
   if (h == 0) {
      errno = EBADF;
      return -1;
   }
   
   GetCommModemStatus(h, &status);

   *bits = 0;

   if (status & MS_CTS_ON)
      *bits |= TIOCM_CTS;
   if (status & MS_DSR_ON)
      *bits |= TIOCM_DSR;
   if (status & MS_RING_ON)
      *bits |= TIOCM_RI;
   if (status & MS_RLSD_ON)
      *bits |= TIOCM_CD;
   
   return 0;
}

int ioctl(int fd, int request, ...)
{
   int rc;
   va_list list;
   va_start(list, request);

   /* We only know how to emulate a few ioctls */
   switch (request) {
   case TIOCMBIC:
      rc = tiocmbic(fd, *va_arg(list, int*));
      break;
   case TIOCMBIS:
      rc = tiocmbis(fd, *va_arg(list, int*));
      break;
   case TIOCMGET:
      rc = tiocmget(fd, va_arg(list, int*));
      break;
   default:
      rc = -1;
      errno = EINVAL;
      break;
   }

   va_end(list);
   return rc;
}

// Parse windows-style command line into individual arguments
char *GetArg(char **cmdline)
{
   // Skip leading whitespace
   while (isspace(**cmdline))
      (*cmdline)++;

   // Bail if there's nothing left
   if (**cmdline == '\0')
      return NULL;

   // Find end of this argument
   char *ret;
   if (**cmdline == '"') {
      // Find end of quoted argument
      ret = ++(*cmdline);
      while (**cmdline && **cmdline != '"')
         (*cmdline)++;
   } else {
      // Find end of non-quoted argument
      ret = *cmdline;
      while (**cmdline && !isspace(**cmdline))
         (*cmdline)++;
   }

   // NUL-terminate this argument
   if (**cmdline)
      *(*cmdline)++ = '\0';

   return ret;
}
