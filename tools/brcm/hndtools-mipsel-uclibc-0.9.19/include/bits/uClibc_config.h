/*
 * Automatically generated C config: don't edit
 */
#if !defined __FEATURES_H && !defined __need_uClibc_config_h
#error Never include <bits/uClibc_config.h> directly; use <features.h> instead.
#endif
#define AUTOCONF_INCLUDED

/*
 * Version Number
 */
#define __UCLIBC_MAJOR__ 0
#define __UCLIBC_MINOR__ 9
#define __UCLIBC_SUBLEVEL__ 19

/*
 * Target Architecture Features and Options
 */
#define __HAVE_ELF__ 1
#define __UCLIBC_HAS_MMU__ 1
#define __UCLIBC_HAS_FLOATS__ 1
#define __HAS_FPU__ 1
#define __DO_C99_MATH__ 1
#define __WARNINGS__ "-Wall"
#define __KERNEL_SOURCE__ "/opt/brcm/hndtools-mipsel-linux/mipsel-linux"
#define __C_SYMBOL_PREFIX__ ""
#define __HAVE_DOT_CONFIG__ 1

/*
 * General Library Settings
 */
#define __DOPIC__ 1
#define __HAVE_SHARED__ 1
#undef __ADD_LIBGCC_FUNCTIONS__
#define __BUILD_UCLIBC_LDSO__ 1
#define __LDSO_LDD_SUPPORT__ 1
#define __UCLIBC_CTOR_DTOR__ 1
#define __UCLIBC_HAS_THREADS__ 1
#define __UCLIBC_HAS_LFS__ 1
#undef __MALLOC__
#define __MALLOC_930716__ 1
#define __UCLIBC_DYNAMIC_ATEXIT__ 1
#define __HAS_SHADOW__ 1
#define __UCLIBC_HAS_REGEX__ 1
#define __UNIX98PTY_ONLY__ 1
#define __ASSUME_DEVPTS__ 1

/*
 * Networking Support
 */
#undef __UCLIBC_HAS_IPV6__
#undef __UCLIBC_HAS_RPC__

/*
 * String and Stdio Support
 */
#undef __UCLIBC_HAS_WCHAR__
#undef __USE_OLD_VFPRINTF__

/*
 * Library Installation Options
 */
#define __SHARED_LIB_LOADER_PATH__ "/lib"
#define __DEVEL_PREFIX__ "/opt/brcm/hndtools-mipsel-uclibc-0.9.19"
#define __SYSTEM_DEVEL_PREFIX__ "$(DEVEL_PREFIX)"
#define __DEVEL_TOOL_PREFIX__ "$(DEVEL_PREFIX)/usr"

/*
 * uClibc hacking options
 */
#undef __DODEBUG__
#undef __DOASSERTS__
#undef __SUPPORT_LD_DEBUG__
#undef __SUPPORT_LD_DEBUG_EARLY__
