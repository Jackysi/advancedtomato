/*
 * apc.h
 *
 * Main header file for apcupsd package
 */

/*
 * Copyright (C) 1999-2005 Kern Sibbald
 * Copyright (C) 1999 Brian Schau <Brian@Schau.dk>
 * Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
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

#ifndef APC_H
#define APC_H 1

#ifdef HAVE_WIN32
# include "winconfig.h"
#else
# include "config.h"
#endif

/*
 * Solaris needs BSD_COMP set in order to get FIONBIO
 * For simplicity, we set it across the board.
 */
#define BSD_COMP

/*
 * Note, on the Alpha, we must include stdarg to get
 * the GNU version before stdio or we get multiple
 * definitions. This order could probably be used
 * on all systems, but is untested so I #ifdef it.
 * KES 9/2000
 */
#ifdef HAVE_OSF1_OS
# include <stdarg.h>
# include <stdio.h>
# include <stdlib.h>
#else
# include <stdio.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_GETOPTLONG
# include <getopt.h>
#else
# include "getopt.h"
#endif

#define _THREAD_SAFE 1
#define _REENTRANT   1
#include <pthread.h>
#ifdef HAVE_SUN_OS
#  include <thread.h>
#  define set_thread_concurrency() thr_setconcurrency(4)
#else
#  define set_thread_concurrency()
#endif

#include <string.h>
#include <strings.h>
#include <signal.h>
#include <ctype.h>
#include <syslog.h>
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <errno.h>
#include <fcntl.h>
#include <setjmp.h>
#include <termios.h>
#include <netdb.h>
#include <sys/ioctl.h>

# ifdef HAVE_SYS_IPC_H
#  include <sys/ipc.h>
# endif
# ifdef HAVE_SYS_SEM_H
#  include <sys/sem.h>
# endif
# ifdef HAVE_SYS_SHM_H
#  include <sys/shm.h>
# endif

#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
# include <sys/types.h>
#endif

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_HPUX_OS
# include <sys/modem.h>
#endif

#ifdef HAVE_QNX_OS
# include <spawn.h>
# include <sys/procmgr.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>


/* Include apcupsd stuff */

#include "apc_config.h"
#include "version.h"
#include "defines.h"
#include "struct.h"
#include "drivers.h"
#include "nis.h"
#include "extern.h"

/* System includes conditionally included */

/* Pull in our local copy because the library does not have correct protos */
#ifdef HAVE_LIBWRAP
# include "tcpd.h"
#endif

/* Solaris doesn't define this */
#ifndef INADDR_NONE
#define INADDR_NONE ((in_addr_t)-1)
#endif

#endif
