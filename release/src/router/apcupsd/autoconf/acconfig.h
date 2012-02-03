/* acconfig.h
   This file is in the public domain.

   Descriptive text for the C preprocessor macros that
   the distributed Autoconf macros can define.
   No software package will use all of them; autoheader copies the ones
   your configure.in uses into your configuration header file templates.

   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  Although this order
   can split up related entries, it makes it easier to check whether
   a given entry is in the file.

   Leave the following blank line there!!  Autoheader needs it.  */


/* Define to 1 if NLS is requested.  */
#undef ENABLE_NLS

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
#undef HAVE_CATGETS

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
#undef HAVE_GETTEXT

/* Define if you have herror available in your bind library */
#undef HAVE_HERROR

/* Define if your locale.h file contains LC_MESSAGES.  */
#undef HAVE_LC_MESSAGES

/* Define if you have res_search available in your bind library */
#undef HAVE_RES_SEARCH

/* Define if you have strftime() */
#undef HAVE_STRFTIME

/* Define as 1 if you have the stpcpy function.  */
#undef HAVE_STPCPY

/* Define if your C compiler allows void * as a function result */
#undef HAVE_VOIDPOINTER

/* Define if your C compiler allows ANSI volatile */
#undef HAVE_VOLATILE

/* Define if `union wait' is the type of the first arg to wait functions.  */
#undef HAVE_UNION_WAIT

/* Define if you have the strcasecmp function.  */
#undef HAVE_STRCASECMP

/* Define if you have the memmove function.  */
#undef HAVE_MEMMOVE

/* Define if you have GNU's getopt family of functions.  */
#undef HAVE_GETOPTLONG

/* Define if you have strstr */
#undef HAVE_STRSTR

/* Define if you have localtime_r */
#undef HAVE_LOCALTIME_R

/* Define if you have inet_pton */
#undef HAVE_INETPTON

/* Define if you have vsyslog */
#undef HAVE_VSYSLOG

/* Define if you have atexit */
#undef HAVE_ATEXIT

/* Define if you have on_exit */
#undef HAVE_ON_EXIT

/* Define if you have setrlimit */
#undef HAVE_SETRLIMIT

/* Define if you have setproctitle */
#undef HAVE_SETPROCTITLE

/* Define to the name of the distribution.  */
#undef PACKAGE

/* Define to help us deduce a 32-bit type (unneeded) */
#undef SIZEOF_INT
#undef SIZEOF_SHORT
#undef SIZEOF_LONG

/* Define to the host we are compiling for.  */
#undef HOST

/* Define the default pid files directory. */
#undef PIDDIR

/* Define the default "log" files directory. */
#undef LOGDIR

/* Define the default serial port lock directory */
#undef LOCKDIR

/* Define the default serial port device */
#undef SERIALDEV

/* Network Information port */
#undef NISPORT

/* Master/slave port */
#undef NETPORT

/* UPSTYPE */
#undef UPSTYPE

/* UPSCABLE */
#undef UPSCABLE

/* tcp wrappers */
#undef HAVE_LIBWRAP

/* ncurses */
#undef HAVE_MENU_H
#undef HAVE_NCURSES_MENU_H

/* OSes */
#undef HAVE_LINUX_OS
#undef HAVE_FREEBSD_OS
#undef HAVE_NETBSD_OS
#undef HAVE_OPENBSD_OS
#undef HAVE_BSDI_OS
#undef HAVE_HPUX_OS
#undef HAVE_SUN_OS
#undef HAVE_AIX_OS
#undef HAVE_SGI_OS
#undef HAVE_CYGWIN
#undef HAVE_OSF1_OS
#undef HAVE_OPENSERVER_OS
#undef HAVE_DARWIN_OS
#undef HAVE_QNX_OS

/* Compiler */
#undef HAVE_GCC

/* sysconfdir */
#undef SYSCONFDIR

/* Power fail dir */
#undef PWRFAILDIR

/* nologdirr */
#undef NOLOGDIR

#undef EXEEXT
/* Win32 substitutions */
#undef WIN32

#undef WINLIBS

#undef UTILPROGS

/* Set if you have POSIX pthreads */
#undef HAVE_PTHREADS

/* Set if you want Smart UPS support */
#undef HAVE_APCSMART_DRIVER

/* Set if you want dumb support */
#undef HAVE_DUMB_DRIVER

#undef HAVE_NANOSLEEP

/* Set if you want NIS server support */
#undef HAVE_NISSERVER

/* Set if you want NIS client support */
#undef HAVE_NET_DRIVER

/* Set by configure if one of the two above are set */
#undef HAVE_NISLIB

/* Set if you want USB support */
#undef HAVE_USB_DRIVER

/* Set if you want SNMP support */
#undef HAVE_SNMP_DRIVER

#undef HAVE_UCD_SNMP
#undef HAVE_NET_SNMP

/* Set if you want TEST driver */
#undef HAVE_TEST_DRIVER

/* Set if you want the old master/slave network code */
#undef HAVE_OLDNET

/* Set if you want to compile powerflute */
#undef HAVE_POWERFLUTE

/* Definitions for GD graphic library */
#undef SYS_IMGFMT_PNG
#undef SYS_IMGFMT_GIF
#undef IMGFMT_GIF

/* Set if have arps/nameser.h */
#undef HAVE_NAMESER_H

/* Set if socklen_t defined */
#undef HAVE_SOCKLEN_T
#undef SOCKLEN_T


#undef HAVE_USLEEP




/* Leave that blank line there!!  Autoheader needs it.
   If you're adding to this file, keep in mind:
   The entries are in sort -df order: alphabetical, case insensitive,
   ignoring punctuation (such as underscores).  */
