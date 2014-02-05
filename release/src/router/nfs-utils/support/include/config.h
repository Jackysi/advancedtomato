/* support/include/config.h.  Generated from config.h.in by configure.  */
/* support/include/config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if the `closedir' function returns void instead of `int'. */
#define CLOSEDIR_VOID 1

/* Define to one of `_getb67', `GETB67', `getb67' for Cray-2 and Cray-YMP
   systems. This function is required for `alloca.c' support on those systems.
   */
/* #undef CRAY_STACKSEG_END */

/* Define to 1 if using `alloca.c'. */
/* #undef C_ALLOCA */

/* Define to the type of elements in the array set by `getgroups'. Usually
   this is either `int' or `gid_t'. */
#define GETGROUPS_T gid_t

/* Define this if you want rpcsec_gss support compiled in */
/* #undef GSS_SUPPORTED */

/* Define to 1 if you have the `alarm' function. */
#define HAVE_ALARM 1

/* Define to 1 if you have `alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if you have <alloca.h> and it should be used (not on Ultrix).
   */
#define HAVE_ALLOCA_H 1

/* Define to 1 if you have the <arpa/inet.h> header file. */
#define HAVE_ARPA_INET_H 1

/* Define to 1 if you have the `atexit' function. */
#define HAVE_ATEXIT 1

/* Define this if the rpcsec_gss library has the function
   authgss_set_debug_level */
/* #undef HAVE_AUTHGSS_SET_DEBUG_LEVEL */

/* Define to 1 if you have the `bindresvport_sa' function. */
/* #undef HAVE_BINDRESVPORT_SA */

/* Define this if you want to use BSD signal semantics */
/* #undef HAVE_BSD_SIGNALS */

/* Define to 1 if you have the `clnt_create' function. */
/* #undef HAVE_CLNT_CREATE */

/* Define to 1 if you have the `clnt_create_timed' function. */
/* #undef HAVE_CLNT_CREATE_TIMED */

/* Define to 1 if you have the `clnt_dg_create' function. */
/* #undef HAVE_CLNT_DG_CREATE */

/* Define to 1 if you have the `clnt_vc_create' function. */
/* #undef HAVE_CLNT_VC_CREATE */

/* Define to 1 if you have the <com_err.h> header file. */
/* #undef HAVE_COM_ERR_H */

/* Define this to 1 if AI_ADDRCONFIG macro is defined */
#define HAVE_DECL_AI_ADDRCONFIG 1

/* Define to 1 if you have the <dirent.h> header file, and it defines `DIR'.
   */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define to 1 if you don't have `vprintf' but do have `_doprnt.' */
/* #undef HAVE_DOPRNT */

/* Define to 1 if you have the `dup2' function. */
#define HAVE_DUP2 1

/* Define to 1 if you have the <et/com_err.h> header file. */
#define HAVE_ET_COM_ERR_H 1

/* Define to 1 if you have the <event.h> header file. */
#define HAVE_EVENT_H 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the `fdatasync' function. */
#define HAVE_FDATASYNC 1

/* Define to 1 if you have the `fork' function. */
#define HAVE_FORK 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define to 1 if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define to 1 if your system has a working `getgroups' function. */
/* #undef HAVE_GETGROUPS */

/* Define to 1 if you have the `gethostbyaddr' function. */
#define HAVE_GETHOSTBYADDR 1

/* Define to 1 if you have the `gethostbyname' function. */
#define HAVE_GETHOSTBYNAME 1

/* Define to 1 if you have the `gethostname' function. */
#define HAVE_GETHOSTNAME 1

/* Define to 1 if you have the `getmntent' function. */
#define HAVE_GETMNTENT 1

/* Define to 1 if you have the `getnameinfo' function. */
#define HAVE_GETNAMEINFO 1

/* Define to 1 if you have the `getnetconfig' function. */
/* #undef HAVE_GETNETCONFIG */

/* Define to 1 if you have the `getrpcbyname' function. */
#define HAVE_GETRPCBYNAME 1

/* Define to 1 if you have the `gettimeofday' function. */
#define HAVE_GETTIMEOFDAY 1

/* Define this if the Kerberos GSS library supports gss_krb5_ccache_name */
/* #undef HAVE_GSS_KRB5_CCACHE_NAME */

/* Define to 1 if you have the `hasmntopt' function. */
#define HAVE_HASMNTOPT 1

/* Define this if you have Heimdal Kerberos libraries */
/* #undef HAVE_HEIMDAL */

/* Define to 1 if you have the <ifaddrs.h> header file. */
#define HAVE_IFADDRS_H 1

/* Define to 1 if you have the `inet_ntoa' function. */
#define HAVE_INET_NTOA 1

/* Define to 1 if you have the `innetgr' function. */
/* #undef HAVE_INNETGR */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define this if you have MIT Kerberos libraries */
/* #undef HAVE_KRB5 */

/* Define this if the function krb5_get_error_message is available */
/* #undef HAVE_KRB5_GET_ERROR_MESSAGE */

/* Define this if the function krb5_get_init_creds_opt_set_addressless is
   available */
/* #undef HAVE_KRB5_GET_INIT_CREDS_OPT_SET_ADDRESSLESS */

/* Define to 1 if you have the <libintl.h> header file. */
/* #undef HAVE_LIBINTL_H */

/* Define to 1 if you have the `tirpc' library (-ltirpc). */
/* #undef HAVE_LIBTIRPC */

/* tcp-wrapper */
/* #undef HAVE_LIBWRAP */

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if `lstat' has the bug that it succeeds when given the
   zero-length file name argument. */
#define HAVE_LSTAT_EMPTY_STRING_BUG 1

/* Define this if the Kerberos GSS library supports
   gss_krb5_export_lucid_sec_context */
/* #undef HAVE_LUCID_CONTEXT_SUPPORT */

/* Define to 1 if you have the <malloc.h> header file. */
#define HAVE_MALLOC_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `memset' function. */
#define HAVE_MEMSET 1

/* Define to 1 if you have the `mkdir' function. */
#define HAVE_MKDIR 1

/* Define to 1 if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define to 1 if you have the <netdb.h> header file. */
#define HAVE_NETDB_H 1

/* Define to 1 if you have the <netinet/in.h> header file. */
#define HAVE_NETINET_IN_H 1

/* Whether nfs4_set_debug() is present in libnfsidmap */
/* #undef HAVE_NFS4_SET_DEBUG */

/* Define to 1 if you have the <nfsidmap.h> header file. */
#define HAVE_NFSIDMAP_H 1

/* Define to 1 if you have the `pathconf' function. */
#define HAVE_PATHCONF 1

/* Define to 1 if you have the <paths.h> header file. */
#define HAVE_PATHS_H 1

/* Define to 1 if you have the `realpath' function. */
#define HAVE_REALPATH 1

/* Define to 1 if you have the `rmdir' function. */
#define HAVE_RMDIR 1

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define this if the Kerberos GSS library supports
   gss_krb5_set_allowable_enctypes */
/* #undef HAVE_SET_ALLOWABLE_ENCTYPES */

/* Define to 1 if you have the `sigprocmask' function. */
#define HAVE_SIGPROCMASK 1

/* Define to 1 if you have the `socket' function. */
#define HAVE_SOCKET 1

/* Define to 1 if you have the <spkm3.h> header file. */
/* #undef HAVE_SPKM3_H */

/* Define to 1 if `stat' has the bug that it succeeds when given the
   zero-length file name argument. */
#define HAVE_STAT_EMPTY_STRING_BUG 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#define HAVE_STRCASECMP 1

/* Define to 1 if you have the `strchr' function. */
#define HAVE_STRCHR 1

/* Define to 1 if you have the `strdup' function. */
#define HAVE_STRDUP 1

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strrchr' function. */
#define HAVE_STRRCHR 1

/* Define to 1 if you have the `strtol' function. */
#define HAVE_STRTOL 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the <syslog.h> header file. */
#define HAVE_SYSLOG_H 1

/* Define to 1 if you have the <sys/dir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_DIR_H */

/* Define to 1 if you have the <sys/file.h> header file. */
#define HAVE_SYS_FILE_H 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/mount.h> header file. */
#define HAVE_SYS_MOUNT_H 1

/* Define to 1 if you have the <sys/ndir.h> header file, and it defines `DIR'.
   */
/* #undef HAVE_SYS_NDIR_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/select.h> header file. */
#define HAVE_SYS_SELECT_H 1

/* Define to 1 if you have the <sys/socket.h> header file. */
#define HAVE_SYS_SOCKET_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/vfs.h> header file. */
#define HAVE_SYS_VFS_H 1

/* tcp-wrapper */
/* #undef HAVE_TCP_WRAPPER */

/* Define to 1 if you have the <tirpc/netconfig.h> header file. */
/* #undef HAVE_TIRPC_NETCONFIG_H */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `vfork' function. */
#define HAVE_VFORK 1

/* Define to 1 if you have the <vfork.h> header file. */
/* #undef HAVE_VFORK_H */

/* Define to 1 if you have the `vprintf' function. */
#define HAVE_VPRINTF 1

/* Define to 1 if `fork' works. */
#define HAVE_WORKING_FORK 1

/* Define to 1 if `vfork' works. */
#define HAVE_WORKING_VFORK 1

/* Define to 1 if you have the `xdr_rpcb' function. */
/* #undef HAVE_XDR_RPCB */

/* Define this if you want IPv6 support compiled in */
/* #undef IPV6_SUPPORTED */

/* Define this as the Kerberos version number */
/* #undef KRB5_VERSION */

/* tcp-wrapper */
/* #undef LIBWRAP */

/* Define to 1 if `lstat' dereferences a symlink specified with a trailing
   slash. */
/* #undef LSTAT_FOLLOWS_SLASHED_SYMLINK */

/* Define to 1 if `major', `minor', and `makedev' are declared in <mkdev.h>.
   */
/* #undef MAJOR_IN_MKDEV */

/* Define to 1 if `major', `minor', and `makedev' are declared in
   <sysmacros.h>. */
/* #undef MAJOR_IN_SYSMACROS */

/* Define this if you want NFSv3 support compiled in */
#define NFS3_SUPPORTED 1

/* Define this if you want NFSv4 support compiled in */
#define NFS4_SUPPORTED 1

/* This defines the location of the NFS state files. Warning: this must match
   definitions in config.mk! */
#define NFS_STATEDIR "/var/lib/nfs"

/* Define to 1 if your C compiler doesn't accept -c and -o together. */
/* #undef NO_MINUS_C_MINUS_O */

/* Name of package */
#define PACKAGE "nfs-utils"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "linux-nfs@vger.kernel.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "linux nfs-utils"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "linux nfs-utils 1.1.5"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "nfs-utils"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.1.5"

/* Define as the return type of signal handlers (`int' or `void'). */
#define RETSIGTYPE void

/* Define to the type of arg 1 for `select'. */
#define SELECT_TYPE_ARG1 int

/* Define to the type of args 2, 3 and 4 for `select'. */
#define SELECT_TYPE_ARG234 (fd_set *)

/* Define to the type of arg 5 for `select'. */
#define SELECT_TYPE_ARG5 (struct timeval *)

/* The size of `int', as computed by sizeof. */
#define SIZEOF_INT 4

/* The size of `long', as computed by sizeof. */
#define SIZEOF_LONG 4

/* The size of `short', as computed by sizeof. */
#define SIZEOF_SHORT 2

/* The size of `size_t', as computed by sizeof. */
#define SIZEOF_SIZE_T 4

/* The size of `socklen_t', as computed by sizeof. */
#define SIZEOF_SOCKLEN_T 4

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define this to a script which can start statd on mount */
#define START_STATD "/usr/sbin/start-statd"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if you can safely include both <sys/time.h> and <time.h>. */
#define TIME_WITH_SYS_TIME 1

/* Define to 1 if your <sys/time.h> declares `struct tm'. */
/* #undef TM_IN_SYS_TIME */

/* Define if you want to use blkid to find uuid of filesystems */
#define USE_BLKID 0

/* Define this if the private function, gss_krb5_cache_name, must be used to
   tell the Kerberos library which credentials cache to use. Otherwise, this
   is done by setting the KRB5CCNAME environment variable */
/* #undef USE_GSS_KRB5_CCACHE_NAME */

/* Version number of package */
#define VERSION "1.1.5"

/* Number of bits in a file offset, on hosts where this is settable. */
#define _FILE_OFFSET_BITS 64

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef gid_t */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to `long int' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned int' if <sys/types.h> does not define. */
/* #undef size_t */

/* Define to `int' if <sys/types.h> doesn't define. */
/* #undef uid_t */

/* Define as `fork' if `vfork' does not work. */
/* #undef vfork */
