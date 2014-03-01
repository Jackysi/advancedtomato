/*                               -*- Mode: C -*-
 * compat.h --
 */
// Copyright transferred from Raider Solutions, Inc to
//   Kern Sibbald and John Walker by express permission.
//
// Copyright (C) 2004-2006 Kern Sibbald
//
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the GNU General Public License as
//   published by the Free Software Foundation; either version 2 of
//   the License, or (at your option) any later version.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//   General Public License for more details.
//
//   You should have received a copy of the GNU General Public
//   License along with this program; if not, write to the Free
//   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
//   MA 02111-1307, USA.
/*
 *
 * Author          : Christopher S. Hull
 * Created On      : Fri Jan 30 13:00:51 2004
 * Last Modified By: Thorsten Engel
 * Last Modified On: Fri Apr 22 19:30:00 2004
 * Update Count    : 218
 * $Id: compat.h,v 1.21.2.4 2009/08/01 12:01:59 adk0212 Exp $
 */


#ifndef __COMPAT_H_
#define __COMPAT_H_
#ifndef _STAT_H
#define _STAT_H       /* don't pull in MinGW stat.h */
#define _STAT_DEFINED /* don't pull in MinGW stat.h */
#endif

#include <stdio.h>
#include <basetsd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <process.h>
#include <direct.h>
#include <wchar.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <wincon.h>
#include <winbase.h>
#include <stdio.h>
#include <stdarg.h>
#include <conio.h>
#include <process.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <malloc.h>
#include <setjmp.h>
#include <direct.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <lmcons.h>
#include <dirent.h>
#include <winapi.h>

#define HAVE_WIN32 1

typedef UINT64 u_int64_t;
typedef UINT64 uint64_t;
typedef INT64 int64_t;
typedef UINT32 uint32_t;
typedef INT64 intmax_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef signed char int8_t;
typedef long time_t;

#if !__STDC__
typedef long _off_t;            /* must be same as sys/types.h */
#endif

typedef UINT32 u_int32_t;
typedef unsigned char u_int8_t;
typedef unsigned short u_int16_t;

#ifndef __cplusplus
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif
#endif

#ifndef ETIMEDOUT
#define ETIMEDOUT 55
#endif

#ifndef EAFNOSUPPORT
#define EAFNOSUPPORT WSAEAFNOSUPPORT
#endif

struct stat
{
    _dev_t      st_dev;
    uint64_t    st_ino;
    uint16_t    st_mode;
    int16_t     st_nlink;
    uint32_t    st_uid;
    uint32_t    st_gid;
    _dev_t      st_rdev;
    uint64_t    st_size;
    time_t      st_atime;
    time_t      st_mtime;
    time_t      st_ctime;
    uint32_t    st_blksize;
    uint64_t    st_blocks;
};

#undef  S_IFMT
#define S_IFMT         0170000         /* file type mask */
#undef  S_IFDIR
#define S_IFDIR        0040000         /* directory */
#define S_IFCHR        0020000         /* character special */
#define S_IFBLK        0060000         /* block special */
#define S_IFIFO        0010000         /* pipe */
#undef  S_IFREG
#define S_IFREG        0100000         /* regular */
#define S_IREAD        0000400         /* read permission, owner */
#define S_IWRITE       0000200         /* write permission, owner */
#define S_IEXEC        0000100         /* execute/search permission, owner */

#define S_IRUSR         S_IREAD
#define S_IWUSR         S_IWRITE
#define S_IXUSR         S_IEXEC
#define S_ISREG(x)  (((x) & S_IFMT) == S_IFREG)
#define S_ISDIR(x)  (((x) & S_IFMT) == S_IFDIR)
#define S_ISCHR(x) 0
#define S_ISBLK(x)  (((x) & S_IFMT) == S_IFBLK)
#define S_ISFIFO(x) 0

#define S_IRGRP         000040
#define S_IWGRP         000020
#define S_IXGRP         000010

#define S_IROTH         00004
#define S_IWOTH         00002
#define S_IXOTH         00001

#define S_IRWXO         000007
#define S_IRWXG         000070
#define S_ISUID         004000
#define S_ISGID         002000
#define S_ISVTX         001000
#define S_ISSOCK(x) 0
#define S_ISLNK(x)      0

#if __STDC__
#define O_RDONLY _O_RDONLY
#define O_WRONLY _O_WRONLY
#define O_RDWR   _O_RDWR
#define O_CREAT  _O_CREAT
#define O_TRUNC  _O_TRUNC
#define O_NOCTTY 0

#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym  __iscsym
#endif


//******************************************************************************
// Sockets
//******************************************************************************
#define WNOHANG 0
#define WIFEXITED(x) 0
#define WEXITSTATUS(x) x
#define WIFSIGNALED(x) 0
#define HAVE_OLD_SOCKOPT
int WSA_Init(void);
int inet_aton(const char *cp, struct in_addr *inp);

//******************************************************************************
// Time
//******************************************************************************
struct timespec;
void sleep(int);
struct timezone;
int strcasecmp(const char*, const char *);
int gettimeofday(struct timeval *, struct timezone *);
#define alarm(a) 0

//******************************************************************************
// User/Password
//******************************************************************************
int geteuid();

#ifndef uid_t
typedef UINT32 uid_t;
typedef UINT32 gid_t;
#endif

struct passwd {
    char *pw_name;
};

struct group {
    char *foo;
};

#define getpwuid(x) NULL
#define getgrgid(x) NULL
#define getuid() 0
#define getgid() 0

//******************************************************************************
// File/Path
//******************************************************************************
int lstat(const char *, struct stat *);
int stat(const char *file, struct stat *sb);
int readlink(const char *, char *, int);
int lchown(const char *, uid_t uid, gid_t gid);
int chown(const char *, uid_t uid, gid_t gid);
#define fcntl(a,b,c) 0

#define _PC_PATH_MAX 1
#define _PC_NAME_MAX 2
long pathconf(const char *, int);

#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#endif

//******************************************************************************
// Signals
//******************************************************************************
struct sigaction {
    int sa_flags;
    void (*sa_handler)(int);
};
#define sigfillset(x)
#define sigaction(a, b, c)
int kill(int pid, int signo);

#define SIGKILL 9
#define SIGUSR2 9999
#define SIGCHLD 0
#define SIGALRM 0
#define SIGHUP 0
#define SIGCHLD 0
#define SIGPIPE 0

//******************************************************************************
// Process
//******************************************************************************
#define getpid _getpid
#define getppid() 0
#define gethostid() 0
int fork();
int waitpid(int, int *, int);

//******************************************************************************
// Logging
//******************************************************************************
#ifndef LOG_DAEMON
#define LOG_DAEMON 0
#endif
#ifndef LOG_ERR
#define LOG_ERR 0
#endif

#define closelog()
#define openlog(a,b,c)
void syslog(int type, const char *fmt, ...);

//******************************************************************************
// Misc
//******************************************************************************
long int random(void);
void srandom(unsigned int seed);

/* Return the smaller of a or b */
#ifndef MIN
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )
#endif

// Parse windows-style command line into individual arguments
char *GetArg(char **cmdline);

#endif /* __COMPAT_H_ */
