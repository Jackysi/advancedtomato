#ifndef STATFIX_H
#define STATFIX_H

#include <features.h>
#undef __OPTIMIZE__
#include <sys/types.h>
#include <bits/wordsize.h>

#if defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS != 64 
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS   64
#endif
#ifndef __USE_LARGEFILE64
# define __USE_LARGEFILE64      1
#endif
/* We absolutely do _NOT_ want interfaces silently
 *  *  * renamed under us or very bad things will happen... */
#ifdef __USE_FILE_OFFSET64
# undef __USE_FILE_OFFSET64
#endif

#if defined __UCLIBC_HAVE_LFS__ 
#if defined __WORDSIZE && (__WORDSIZE >= 64) 

/* 64 bit arch stuff... */

/* Pull in whatever this particular arch's kernel thinks the kernel version of
 *  * struct stat should look like.  It turns out that each arch has a different
 *   * opinion on the subject, and different kernel revs use different names... */
#define stat kernel_stat64
#define new_stat kernel_stat64
#define stat64 kernel_stat64
#define kernel_stat kernel_stat64
#include <asm/stat.h> 
#undef stat64
#undef new_stat
#undef stat


/* Now pull in libc's version of stat */
#define stat libc_stat
#define stat64 libc_stat64
#define _SYS_STAT_H
#include <bits/stat.h>
#undef stat64
#undef stat

extern void statfix64(struct libc_stat64 *libcstat, struct kernel_stat64 *kstat);
extern int __fxstat64(int version, int fd, struct libc_stat64 * statbuf);

#else   

/* 32 bit arch stuff */


/* Pull in whatever this particular arch's kernel thinks the kernel version of
 *  * struct stat should look like.  It turns out that each arch has a different
 *   * opinion on the subject, and different kernel revs use different names... */
#define stat kernel_stat
#define new_stat kernel_stat
#define stat64 kernel_stat64
#include <asm/stat.h>
#undef stat64
#undef new_stat
#undef stat

/* Now pull in libc's version of stat */
#define stat libc_stat
#define stat64 libc_stat64
#define _SYS_STAT_H
#include <bits/stat.h>
#undef stat64
#undef stat

extern void statfix64(struct libc_stat64 *libcstat, struct kernel_stat64 *kstat);
extern int __fxstat64(int version, int fd, struct libc_stat64 * statbuf);


#endif /* __WORDSIZE */
#endif /* __UCLIBC_HAVE_LFS__ */

#endif
