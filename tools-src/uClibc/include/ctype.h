/*  Copyright (C) 2002     Manuel Novoa III
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* NOTE: It is assumed here and throughout the library that the underlying
 * char encoding for the portable C character set is ASCII (host & target). */

#ifndef _CTYPE_H
#define _CTYPE_H

#include <features.h>
#include <bits/uClibc_ctype.h>

__BEGIN_DECLS

extern int isalnum(int c) __THROW;
extern int isalpha(int c) __THROW;
#ifdef __USE_ISOC99
extern int isblank(int c) __THROW;
#endif
extern int iscntrl(int c) __THROW;
extern int isdigit(int c) __THROW;
extern int isgraph(int c) __THROW;
extern int islower(int c) __THROW;
extern int isprint(int c) __THROW;
extern int ispunct(int c) __THROW;
extern int isspace(int c) __THROW;
extern int isupper(int c) __THROW;
extern int isxdigit(int c) __THROW;

extern int tolower(int c) __THROW;
extern int toupper(int c) __THROW;

#if defined __USE_SVID || defined __USE_MISC || defined __USE_XOPEN
extern int isascii(int c) __THROW;
extern int toascii(int c) __THROW;
#endif

/* The following are included for compatibility with older versions of
 * uClibc; but now they're only visible if MISC funcctionality is requested.
 * However, as they are locale-independent, the hidden macro versions are
 * always present. */
#ifdef __USE_MISC
extern int isxlower(int c) __THROW;	/* uClibc-specific. */
extern int isxupper(int c) __THROW;	/* uClibc-specific. */
#endif

/* Next, some ctype macros which are valid for all supported locales. */
/* WARNING: isspace and isblank need to be reverified if more 8-bit codesets
 * are added!!!  But isdigit and isxdigit are always valid. */

#define __isspace(c)	__C_isspace(c)
#define __isblank(c)	__C_isblank(c)

#define __isdigit(c)	__C_isdigit(c)
#define __isxdigit(c)	__C_isxdigit(c)

/* Now some non-ansi/iso c99 macros. */

#define __isascii(c) (((c) & ~0x7f) == 0)
#define __toascii(c) ((c) & 0x7f)
#define _toupper(c) ((c) ^ 0x20)
#define _tolower(c) ((c) | 0x20)


/* For compatibility with older versions of uClibc.  Are these ever used? */
#define __isxlower(c)	__C_isxlower(c)	/* uClibc-specific. */
#define __isxupper(c)	__C_isxupper(c)	/* uClibc-specific. */

/* Apparently, glibc implements things as macros if __NO_CTYPE isn't defined.
 * If we don't have locale support, we'll do the same.  Otherwise, we'll
 * only use macros for the supported-locale-invariant cases. */
#if 0
/* Currently broken, since masking macros, other than getc and putc, must
 * evaluate their args exactly once.  Will be fixed by the next release.  mjn3 */
/* #ifndef __NO_CTYPE */

#define isdigit(c)	__isdigit(c)
#define isxdigit(c)	__isxdigit(c)
#define isspace(c)	__isspace(c)
#ifdef __USE_ISOC99
#define isblank(c)	__isblank(c)
#endif

#if defined __USE_SVID || defined __USE_MISC || defined __USE_XOPEN
#define isascii(c)	__isascii(c)
#define toascii(c)	__toascii(c)
#endif

#ifdef __USE_MISC
#define isxlower(c)	__C_isxlower(c)	/* uClibc-specific. */
#define isxupper(c)	__C_isxupper(c)	/* uClibc-specific. */
#endif

/* TODO - Should test for 8-bit codesets instead, but currently impossible. */
#ifndef __UCLIBC_HAS_LOCALE__

#define isalnum(c)	__C_isalnum(c)
#define isalpha(c)	__C_isalpha(c)
#define iscntrl(c)	__C_iscntrl(c)
#define isgraph(c)	__C_isgraph(c)
#define islower(c)	__C_islower(c)
#define isprint(c)	__C_isprint(c)
#define ispunct(c)	__C_ispunct(c)
#define isupper(c)	__C_isupper(c)

#define tolower(c)	__C_tolower(c)
#define toupper(c)	__C_toupper(c)

#endif /*  __UCLIBC_HAS_LOCALE__ */

#endif /* __NO_CTYPE */

__END_DECLS

#endif /* _CTYPE_H */
