AC_PREREQ(2.61)
AC_INIT(dnscrypt-proxy, 0.9.3, bugs at opendns dot com)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/dnscrypt-proxy/app.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE([1.9 dist-bzip2])
AM_MAINTAINER_MODE
AM_DEP_TRACK

AC_SUBST(VERSION)
ISODATE=`date +%Y-%m-%d`
AC_SUBST(ISODATE)

# Checks for programs.
AX_CHECK_GNU_MAKE
AS_IF([test -n "${ifGNUmake}"],[
  AC_MSG_ERROR([
Found: ${MAKE}
GNU make is required and was not found. If GNU make is not installed,
please install it. www.gnu.org. If GNU make is installed, please adjust
your PATH to make sure that GNU make is found before any other version
of make that is installed on your system. rm config.cache and then
re-run configure again.])
],[
  AC_SUBST(MAKE, $_cv_gnu_make_command)
])

LX_CFLAGS=${CFLAGS-NONE}
AC_PROG_CC_C99
AC_USE_SYSTEM_EXTENSIONS
CPPFLAGS="$CPPFLAGS -D_XPG4_2=1 -D_GNU_SOURCE=1"

AX_PTHREAD

AS_IF([test "$cross_compiling" != no],
  AC_DEFINE(CROSS_COMPILING,,[define if you are cross-compiling])
)

AX_CHECK_COMPILE_FLAG([-fPIE], [
  AX_CHECK_LINK_FLAG([-fPIE],
    [CFLAGS="$CFLAGS -fPIE"]
  )
])

AX_CHECK_COMPILE_FLAG([-fcatch-undefined-c99-behavior], [CFLAGS="$CFLAGS -fcatch-undefined-c99-behavior"])
AX_CHECK_COMPILE_FLAG([-fno-strict-aliasing], [CFLAGS="$CFLAGS -fno-strict-aliasing"])
AX_CHECK_COMPILE_FLAG([-fno-strict-overflow], [CFLAGS="$CFLAGS -fno-strict-overflow"])

AS_IF([echo `(uname -s) 2>/dev/null` | $GREP "CYGWIN" > /dev/null], [
  AX_CHECK_LINK_FLAG([-Wl,--dynamicbase], [LDFLAGS="$LDFLAGS -Wl,--dynamicbase"])
  AX_CHECK_LINK_FLAG([-Wl,--nxcompat], [LDFLAGS="$LDFLAGS -Wl,--nxcompat"])
], [
  AS_IF([test `(uname -s) 2>/dev/null` = "DragonFly"],
    [
      AX_CHECK_COMPILE_FLAG([-fstack-protector], [
        AX_CHECK_LINK_FLAG([-fstack-protector],
          [CFLAGS="$CFLAGS -fstack-protector"]
        )
      ])
    ],
    [
      AX_CHECK_COMPILE_FLAG([-fstack-protector-all], [
        AX_CHECK_LINK_FLAG([-fstack-protector-all],
          [CFLAGS="$CFLAGS -fstack-protector-all"]
        )
      ])
    ]
  )
  AS_IF([test "x$ac_compiler_gnu" = "xyes"],[
    AS_IF([$CC --version 2>/dev/null | $GREP "Free Software Foundation" > /dev/null 2>&1],[
      CPPFLAGS="$CPPFLAGS -D_FORTIFY_SOURCE=2"
      AX_CHECK_COMPILE_FLAG([--param ssp-buffer-size=1], [CFLAGS="$CFLAGS --param ssp-buffer-size=1"])
    ])
  ])
])

AX_CHECK_COMPILE_FLAG([-Wbounded], [CFLAGS="$CFLAGS -Wbounded"])
AX_CHECK_COMPILE_FLAG([-Winit-self], [CFLAGS="$CFLAGS -Winit-self"])
AX_CHECK_COMPILE_FLAG([-Wwrite-strings], [CFLAGS="$CFLAGS -Wwrite-strings"])
AX_CHECK_COMPILE_FLAG([-Wdiv-by-zero], [CFLAGS="$CFLAGS -Wdiv-by-zero"])

AC_ARG_VAR([CWFLAGS], [define to compilation flags for generating extra warnings])
AX_CHECK_COMPILE_FLAG([-Wall], [CWFLAGS="$CWFLAGS -Wall"])
AX_CHECK_COMPILE_FLAG([-Wbad-function-cast], [CWFLAGS="$CWFLAGS -Wbad-function-cast"])
AX_CHECK_COMPILE_FLAG([-Wcast-align], [CWFLAGS="$CWFLAGS -Wcast-align"])
AX_CHECK_COMPILE_FLAG([-Wcast-qual], [CWFLAGS="$CWFLAGS -Wcast-qual"])
AX_CHECK_COMPILE_FLAG([-Wchar-subscripts], [CWFLAGS="$CWFLAGS -Wchar-subscripts"])
AX_CHECK_COMPILE_FLAG([-Wcomment], [CWFLAGS="$CWFLAGS -Wcomment"])
AX_CHECK_COMPILE_FLAG([-Wextra], [CWFLAGS="$CWFLAGS -Wextra"])
AX_CHECK_COMPILE_FLAG([-Wfloat-equal], [CWFLAGS="$CWFLAGS -Wfloat-equal"])
AX_CHECK_COMPILE_FLAG([-Wformat=2], [CWFLAGS="$CWFLAGS -Wformat=2"])
AX_CHECK_COMPILE_FLAG([-Wimplicit], [CWFLAGS="$CWFLAGS -Wimplicit"])
AX_CHECK_COMPILE_FLAG([-Wmissing-declarations], [CWFLAGS="$CWFLAGS -Wmissing-declarations"])
AX_CHECK_COMPILE_FLAG([-Wmissing-noreturn], [CWFLAGS="$CWFLAGS -Wmissing-noreturn"])
AX_CHECK_COMPILE_FLAG([-Wmissing-prototypes], [CWFLAGS="$CWFLAGS -Wmissing-prototypes"])
AX_CHECK_COMPILE_FLAG([-Wnormalized=id], [CWFLAGS="$CWFLAGS -Wnormalized=id"])
AX_CHECK_COMPILE_FLAG([-Woverride-init], [CWFLAGS="$CWFLAGS -Woverride-init"])
AX_CHECK_COMPILE_FLAG([-Wparentheses], [CWFLAGS="$CWFLAGS -Wparentheses"])
AX_CHECK_COMPILE_FLAG([-Wpointer-arith], [CWFLAGS="$CWFLAGS -Wpointer-arith"])
AX_CHECK_COMPILE_FLAG([-Wredundant-decls], [CWFLAGS="$CWFLAGS -Wredundant-decls"])
AX_CHECK_COMPILE_FLAG([-Wstrict-prototypes], [CWFLAGS="$CWFLAGS -Wstrict-prototypes"])
AX_CHECK_COMPILE_FLAG([-Wswitch-enum], [CWFLAGS="$CWFLAGS -Wswitch-enum"])
AX_CHECK_COMPILE_FLAG([-Wvariable-decl], [CWFLAGS="$CWFLAGS -Wvariable-decl"])

AX_CHECK_LINK_FLAG([-pie], [LDFLAGS="$LDFLAGS -pie"])
AX_CHECK_LINK_FLAG([-z relro], [LDFLAGS="$LDFLAGS -z relro"])
AX_CHECK_LINK_FLAG([-z now], [LDFLAGS="$LDFLAGS -z now"])

AC_ARG_VAR([RONN], [path to the ronn text processor])
AC_PATH_PROG([RONN], [ronn], [#])

AC_ARG_VAR([SIXJACK], [path to 6jack])
AC_PATH_PROG([SIXJACK], [sixjack], [#])

AC_ARG_VAR([AR], [path to the ar utility])
AC_CHECK_TOOL([AR], [ar], [ar])

AS_IF([test `(uname -s) 2>/dev/null` = "Darwin"], [
  AC_ARG_VAR([DTRACE], [path to dtrace if you have it])
  AC_PATH_PROG([DTRACE], [dtrace])
])
AS_IF([test -n "$DTRACE"],[
  PROBES_SOURCE="probes_dnscrypt_proxy.h"
],[
  DTRACE="#"
  PROBES_SOURCE="probes_no_dtrace.h"
])

AS_IF([test -d /usr/local/include], [
  CPPFLAGS="$CPPFLAGS -I/usr/local/include"
])

AS_IF([test -d /usr/local/lib], [
  LDFLAGS="$LDFLAGS -L/usr/local/lib"
])

LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# Checks for header files.
AC_SYS_LARGEFILE
AC_CHECK_HEADERS([execinfo.h paths.h sys/cdefs.h sys/feature_tests.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_RESTRICT

AC_MSG_CHECKING([whether sin_len is defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
]], [[
do {
 struct sockaddr_in a;
 (void) a.sin_len;
} while(0)
]])],[
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_SIN_LEN,,[define if you have sin_len])
],[
  AC_MSG_RESULT(no)
])

AC_MSG_CHECKING([whether __ss_family is defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
]], [[
do {
 struct sockaddr_storage a;
 (void) a.__ss_family;
} while(0)
]])],[
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE___SS_FAMILY,,[define if you have __ss_family])
],[
  AC_MSG_RESULT(no)
])

AC_MSG_CHECKING([whether ss_len is defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
]], [[
do {
 struct sockaddr_storage a;
 (void) a.ss_len;
} while(0)
]])],[
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_SS_LEN,,[define if you have ss_len])
],[
  AC_MSG_RESULT(no)
])

AC_MSG_CHECKING([whether __ss_len is defined])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/socket.h>
]], [[
do {
 struct sockaddr_storage a;
 (void) a.__ss_len;
} while(0)
]])],[
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE___SS_LEN,,[define if you have __ss_len])
],[
  AC_MSG_RESULT(no)
])

# Checks for library functions.
AC_SEARCH_LIBS(pow, [m])
AC_SEARCH_LIBS(dlopen, [dl])
AC_SEARCH_LIBS(clock_gettime, [rt],
  [AC_DEFINE(HAVE_CLOCK_GETTIME,,[define if you have clock_gettime()])])

AC_SEARCH_LIBS(backtrace, [execinfo],
  [AC_DEFINE(HAVE_BACKTRACE,,[define if you have backtrace()])])

AC_SEARCH_LIBS(gethostbyname, [resolv nsl])
AC_SEARCH_LIBS(recvfrom, [socket])
AC_SEARCH_LIBS(kvm_open, [kvm])

AS_IF([echo `(uname -s) 2>/dev/null` | $GREP "CYGWIN" > /dev/null], [
  AC_CHECK_LIB([ws2_32], [_head_libws2_32_a])
  AC_CHECK_LIB([psapi], [_head_libpsapi_a])
  AC_CHECK_LIB([iphlpapi], [_head_libiphlpapi_a])
], [ ])

AC_CHECK_HEADER([CoreServices/CoreServices.h],
  [LIBS="$LIBS -framework CoreFoundation -framework CoreServices"])

# Switches
AC_HEADER_ASSERT

AC_ARG_WITH(debug,
[AS_HELP_STRING(--with-debug,For maintainers only - please do not use)],
[ AS_IF([test "x$withval" = "xyes"], [
    AS_IF([test "x$LX_CFLAGS" = "xNONE"], [
      nxflags=""
      for flag in `echo $CFLAGS`; do
        case "$flag" in
          -O*) ;;
          -g*) ;;
          *) nxflags="$nxflags $flag"
        esac
      done
      CFLAGS="$nxflags -O0 -g3"
    ])
    AM_CFLAGS="$AM_CFLAGS -DDEBUG=1"
  ])
])

AC_ARG_WITH(safecode,
[AS_HELP_STRING(--with-safecode,For maintainers only - please do not use)],
[ AS_IF([test "x$withval" = "xyes"], [
    AC_ARG_VAR([SAFECODE_HOME], [set to the safecode base directory])
    : ${SAFECODE_HOME:=/opt/safecode}
    LDFLAGS="$LDFLAGS -L${SAFECODE_HOME}/lib"
    LIBS="$LIBS -lsc_dbg_rt -lpoolalloc_bitmap -lstdc++"
    AM_CFLAGS="$AM_CFLAGS -fmemsafety"
  ])
])

NACL_HOSTNAME="localhost"
NACL_PATH=libnacl/build/${NACL_HOSTNAME}

AC_SUBST([MAINT])
AC_SUBST([NACL_PATH])
AC_SUBST([PROBES_SOURCE])

AC_CONFIG_FILES([Makefile
                 man/Makefile
                 src/Makefile
                 src/dnscrypt-proxy/Makefile
                 src/libnacl/Makefile
                 src/libnacl/tests/Makefile
                 src/libnacl/okcompilers/c
                 src/libnacl/okcompilers/do
                 src/libuv/Makefile
                 test/Makefile])

AC_OUTPUT

chmod +x src/libnacl/okcompilers/do

