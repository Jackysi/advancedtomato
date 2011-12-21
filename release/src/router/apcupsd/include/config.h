/* include/config.h.  Generated from config.h.in by configure.  */
/* autoconf/config.h.in.  Generated automatically from autoconf/configure.in by autoheader.  */

/* Define if using alloca.c.  */
/* #undef C_ALLOCA */

/* Define to empty if the keyword does not work.  */
/* #undef const */

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
/* #undef CRAY_STACKSEG_END */

/* Define to the type of elements in the array set by `getgroups'.
   Usually this is either `int' or `gid_t'.  */
#define GETGROUPS_T gid_t

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef gid_t */

/* Define if you have alloca, as a function or macro.  */
/* #undef HAVE_ALLOCA */

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
/* #undef HAVE_ALLOCA_H */

/* Define if you have a working `mmap' system call.  */
/* #undef HAVE_MMAP */

/* Define if you have the strftime function.  */
#define HAVE_STRFTIME 1

/* Define if you have the ANSI # stringizing operator in cpp. */
/* #undef HAVE_STRINGIZE */

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#define HAVE_SYS_WAIT_H 1

/* Define if your struct tm has tm_zone.  */
#define HAVE_TM_ZONE 1

/* Define if you don't have tm_zone but do have the external array
   tzname.  */
/* #undef HAVE_TZNAME */

/* Define as __inline if that's what the C compiler calls it.  */
/* #undef inline */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef mode_t */

/* Define if your C compiler doesn't accept -c and -o together.  */
/* #undef NO_MINUS_C_MINUS_O */

/* Define to `long' if <sys/types.h> doesn't define.  */
/* #undef off_t */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef pid_t */

/* Define if you need to in order for stat and other things to work.  */
/* #undef _POSIX_SOURCE */

/* Define as the return type of signal handlers (int or void).  */
#define RETSIGTYPE void

/* Define if the `setpgrp' function takes no argument.  */
/* #undef SETPGRP_VOID */

/* Define to `unsigned' if <sys/types.h> doesn't define.  */
/* #undef size_t */

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
/* #undef STACK_DIRECTION */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly.  */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#define TIME_WITH_SYS_TIME 1

/* Define if your <sys/time.h> declares struct tm.  */
/* #undef TM_IN_SYS_TIME */

/* Define to `int' if <sys/types.h> doesn't define.  */
/* #undef uid_t */

/* Define to 1 if NLS is requested.  */
/* #undef ENABLE_NLS */

/* Define as 1 if you have catgets and don't want to use GNU gettext.  */
/* #undef HAVE_CATGETS */

/* Define as 1 if you have gettext and don't want to use GNU gettext.  */
/* #undef HAVE_GETTEXT */

/* Define if your locale.h file contains LC_MESSAGES.  */
/* #undef HAVE_LC_MESSAGES */

/* Define if you have strftime() */
#define HAVE_STRFTIME 1

/* Define as 1 if you have the stpcpy function.  */
/* #undef HAVE_STPCPY */

/* Define if `union wait' is the type of the first arg to wait functions.  */
#define HAVE_UNION_WAIT 1

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE 1

/* Define if you have GNU's getopt family of functions.  */
#define HAVE_GETOPTLONG 1

/* Define if you have strstr */
#define HAVE_STRSTR 1

/* Define if you have localtime_r */
#define HAVE_LOCALTIME_R 1

/* Define if you have inet_pton */
#define HAVE_INETPTON 1

/* Define if you have setproctitle */
/* #undef HAVE_SETPROCTITLE */

/* Define to the name of the distribution.  */
/* #undef PACKAGE */

/* Define to the host we are compiling for.  */
#define HOST "debian"

/* Define the default pid files directory. */
#define PIDDIR "/var/run"

/* Define the default "log" files directory. */
#define LOGDIR "/var/log"

/* Network Information port */
#define NISPORT 3551

/* tcp wrappers */
/* #undef HAVE_LIBWRAP */

/* OSes */
#define HAVE_LINUX_OS 1
/* #undef HAVE_FREEBSD_OS */
/* #undef HAVE_NETBSD_OS */
/* #undef HAVE_OPENBSD_OS */
/* #undef HAVE_BSDI_OS */
/* #undef HAVE_HPUX_OS */
/* #undef HAVE_SUN_OS */
/* #undef HAVE_AIX_OS */
/* #undef HAVE_SGI_OS */
/* #undef HAVE_OSF1_OS */
/* #undef HAVE_OPENSERVER_OS */
/* #undef HAVE_DARWIN_OS */
/* #undef HAVE_QNX_OS */
/* #undef HAVE_MINGW */

/* Compiler */
#define HAVE_GCC 1

/* sysconfdir */
#define SYSCONFDIR "/usr/local/apcupsd"

/* Power fail dir */
#define PWRFAILDIR "/usr/local/apcupsd"

/* nologdirr */
#define NOLOGDIR "/etc"

#define EXEEXT ""
/* Win32 substitutions */
/* #undef WIN32 */

/* Set if you have POSIX pthreads */
#define HAVE_PTHREADS 1

/* Set if you want Smart UPS support */
#define HAVE_APCSMART_DRIVER 1

/* Set if you want dumb support */
/* #undef HAVE_DUMB_DRIVER */

/* Set if you want NIS server support */
#define HAVE_NISSERVER 1

/* Set if you want NIS client support */
#define HAVE_NET_DRIVER 1

/* Set by configure if one of the two above are set */
#define HAVE_NISLIB 1

/* Set if you want USB support */
#define HAVE_USB_DRIVER 1

/* Set if you want SNMP support */
/* #undef HAVE_SNMP_DRIVER */

/* Set if you want SNMP Lite support */
/* #undef HAVE_SNMPLITE_DRIVER */

/* #undef HAVE_UCD_SNMP */
/* #undef HAVE_NET_SNMP */

/* Set if you want TEST driver */
/* #undef HAVE_TEST_DRIVER */

/* Set if you want PCNET driver */
/* #undef HAVE_PCNET_DRIVER */

/* Set if you want the old master/slave network code */
/* #undef HAVE_OLDNET */

/* Set if you want to compile powerflute */
/* #undef HAVE_POWERFLUTE */

/* Definitions for GD graphic library */
/* #undef SYS_IMGFMT_PNG */
/* #undef SYS_IMGFMT_GIF */
/* #undef IMGFMT_GIF */

/* Set if have arps/nameser.h */
#define HAVE_NAMESER_H 1

/* Set if socklen_t defined */
#define HAVE_SOCKLEN_T 1

/* Define if you have the __argz_count function.  */
/* #undef HAVE___ARGZ_COUNT */

/* Define if you have the __argz_next function.  */
/* #undef HAVE___ARGZ_NEXT */

/* Define if you have the __argz_stringify function.  */
/* #undef HAVE___ARGZ_STRINGIFY */

/* Define if you have the abort function.  */
#define HAVE_ABORT 1

/* Define if you have the calloc function.  */
#define HAVE_CALLOC 1

/* Define if you have the dcgettext function.  */
/* #undef HAVE_DCGETTEXT */

/* Define if you have the fork function.  */
#define HAVE_FORK 1

/* Define if you have the getcwd function.  */
/* #undef HAVE_GETCWD */

/* Define if you have the getpagesize function.  */
/* #undef HAVE_GETPAGESIZE */

/* Define if you have the getpid function.  */
#define HAVE_GETPID 1

/* Define if you have the ioctl function.  */
#define HAVE_IOCTL 1

/* Define if you have the kill function.  */
#define HAVE_KILL 1

/* Define if you have the munmap function.  */
/* #undef HAVE_MUNMAP */

/* Define if you have the putenv function.  */
/* #undef HAVE_PUTENV */

/* Define if you have the rewind function.  */
#define HAVE_REWIND 1

/* Define if you have the select function.  */
#define HAVE_SELECT 1

/* Define if you have the setenv function.  */
/* #undef HAVE_SETENV */

/* Define if you have the setlocale function.  */
/* #undef HAVE_SETLOCALE */

/* Define if you have the setpgrp function.  */
#define HAVE_SETPGRP 1

/* Define if you have the setsid function.  */
#define HAVE_SETSID 1

/* Define if you have the shmctl function.  */
/* #undef HAVE_SHMCTL */

/* Define if you have the signal function.  */
#define HAVE_SIGNAL 1

/* Define if you have the snprintf function.  */
#define HAVE_SNPRINTF 1

/* Define if you have the stpcpy function.  */
/* #undef HAVE_STPCPY */

/* Define if you have the strcasecmp function.  */
#define HAVE_STRCASECMP 1

/* Define if you have the strchr function.  */
/* #undef HAVE_STRCHR */

/* Define if you have the strdup function.  */
/* #undef HAVE_STRDUP */

/* Define if you have the strerror function.  */
#define HAVE_STRERROR 1

/* Define if you have the strncmp function.  */
#define HAVE_STRNCMP 1

/* Define if you have the strncpy function.  */
#define HAVE_STRNCPY 1

/* Define if you have the tcgetattr function.  */
#define HAVE_TCGETATTR 1

/* Define if you have the vfprintf function.  */
#define HAVE_VFPRINTF 1

/* Define if you have the vsnprintf function.  */
#define HAVE_VSNPRINTF 1

/* Define if you have the wait function.  */
#define HAVE_WAIT 1

/* Define if you have the wait3 function.  */
#define HAVE_WAIT3 1

/* Define if you have the waitpid function.  */
#define HAVE_WAITPID 1

/* Define if you have the <argz.h> header file.  */
/* #undef HAVE_ARGZ_H */

/* Define if you have the <arpa/inet.h> header file.  */
#define HAVE_ARPA_INET_H 1

/* Define if you have the <arpa/nameser.h> header file.  */
#define HAVE_ARPA_NAMESER_H 1

/* Define if you have the <ctype.h> header file.  */
#define HAVE_CTYPE_H 1

/* Define if you have the <curses.h> header file.  */
/* #undef HAVE_CURSES_H */

/* Define if you have the <errno.h> header file.  */
#define HAVE_ERRNO_H 1

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H 1

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H 1

/* Define if you have the <locale.h> header file.  */
/* #undef HAVE_LOCALE_H */

/* Define if you have the <malloc.h> header file.  */
/* #undef HAVE_MALLOC_H */

/* Define if you have the <menu.h> header file.  */
/* #undef HAVE_MENU_H */

/* Define if you have the <ncurses/curses.h> header file.  */
/* #undef HAVE_NCURSES_CURSES_H */

/* Define if you have the <ncurses/menu.h> header file.  */
/* #undef HAVE_NCURSES_MENU_H */

/* Define if you have the <ncurses/panel.h> header file.  */
/* #undef HAVE_NCURSES_PANEL_H */

/* Define if you have the <netdb.h> header file.  */
#define HAVE_NETDB_H 1

/* Define if you have the <netinet/in.h> header file.  */
#define HAVE_NETINET_IN_H 1

/* Define if you have the <nl_types.h> header file.  */
/* #undef HAVE_NL_TYPES_H */

/* Define if you have the <panel.h> header file.  */
/* #undef HAVE_PANEL_H */

/* Define if you have the <pwd.h> header file.  */
#define HAVE_PWD_H 1

/* Define if you have the <signal.h> header file.  */
#define HAVE_SIGNAL_H 1

/* Define if you have the <stdarg.h> header file.  */
#define HAVE_STDARG_H 1

/* Define if you have the <stdio.h> header file.  */
#define HAVE_STDIO_H 1

/* Define if you have the <stdlib.h> header file.  */
#define HAVE_STDLIB_H 1

/* Define if you have the <string.h> header file.  */
#define HAVE_STRING_H 1

/* Define if you have the <strings.h> header file.  */
#define HAVE_STRINGS_H 1

/* Define if you have the <sys/ioctl.h> header file.  */
#define HAVE_SYS_IOCTL_H 1

/* Define if you have the <sys/ipc.h> header file.  */
/* #undef HAVE_SYS_IPC_H */

/* Define if you have the <sys/param.h> header file.  */
/* #undef HAVE_SYS_PARAM_H */

/* Define if you have the <sys/sem.h> header file.  */
/* #undef HAVE_SYS_SEM_H */

/* Define if you have the <sys/shm.h> header file.  */
/* #undef HAVE_SYS_SHM_H */

/* Define if you have the <sys/socket.h> header file.  */
#define HAVE_SYS_SOCKET_H 1

/* Define if you have the <sys/stat.h> header file.  */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/types.h> header file.  */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <syslog.h> header file.  */
#define HAVE_SYSLOG_H 1

/* Define if you have the <termios.h> header file.  */
#define HAVE_TERMIOS_H 1

/* Define if you have the <unistd.h> header file.  */
#define HAVE_UNISTD_H 1

/* Define if you have the i library (-li).  */
/* #undef HAVE_LIBI */

/* Define if you have the inet library (-linet).  */
/* #undef HAVE_LIBINET */

/* Define if you have the intl library (-lintl).  */
/* #undef HAVE_LIBINTL */

/* Define if you have the nsl library (-lnsl).  */
/* #undef HAVE_LIBNSL */

/* Define if you have the resolv library (-lresolv).  */
/* #undef HAVE_LIBRESOLV */

/* Define if you have the socket library (-lsocket).  */
/* #undef HAVE_LIBSOCKET */

/* Define if you have the xnet library (-lxnet).  */
/* #undef HAVE_LIBXNET */

#define HAVE_NANOSLEEP 1
/* #undef int32_t */

/* Which variant of gethostbyname_r() do we have */
/* #undef HAVE_FUNC_GETHOSTBYNAME_R_0 */
/* #undef HAVE_FUNC_GETHOSTBYNAME_R_3 */
/* #undef HAVE_FUNC_GETHOSTBYNAME_R_5 */
#define HAVE_FUNC_GETHOSTBYNAME_R_6 1

