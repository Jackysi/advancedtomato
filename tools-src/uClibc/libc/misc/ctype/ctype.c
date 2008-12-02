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

/*  ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION!
 *
 *  Besides uClibc, I'm using this code in my libc for elks, which is
 *  a 16-bit environment with a fairly limited compiler.  It would make
 *  things much easier for me if this file isn't modified unnecessarily.
 *  In particular, please put any new or replacement functions somewhere
 *  else, and modify the makefile to use your version instead.
 *  Thanks.  Manuel
 *
 *  ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION!   ATTENTION! */

#define _GNU_SOURCE
#define __NO_CTYPE

#include <ctype.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>
#include <locale.h>

/**********************************************************************/

extern int __isctype_loc(int c, int ct);

/* Some macros used throughout the file. */
#define U		((unsigned char)c)
/*  #define LCT		(__cur_locale->ctype) */
#define LCT		(&__global_locale)

/**********************************************************************/

#ifndef __PASTE
#define __PASTE(X,Y)		X ## Y
#endif

#define C_MACRO(X)		__PASTE(__C_,X)(c)

#define CT_MACRO(X)		__PASTE(__ctype_,X)(c)

/**********************************************************************/

#ifndef __CTYPE_HAS_8_BIT_LOCALES

#define IS_FUNC_BODY(NAME) \
int NAME (int c) \
{ \
	return C_MACRO(NAME); \
}

#else

/* It may be worth defining __isctype_loc over the whole range of char. */
/*  #define IS_FUNC_BODY(NAME) \ */
/*  int NAME (int c) \ */
/*  { \ */
/*  	return __isctype_loc(c, __PASTE(_CTYPE_,NAME)); \ */
/*  } */

#define IS_FUNC_BODY(NAME) \
int NAME (int c) \
{ \
	if (((unsigned int) c) <= 0x7f) { \
		return C_MACRO(NAME); \
	} \
	return __isctype_loc(c, __PASTE(_CTYPE_,NAME)); \
}

#endif /* __CTYPE_HAS_8_BIT_LOCALES */

/**********************************************************************/
#ifdef L_isalnum

IS_FUNC_BODY(isalnum);

#endif
/**********************************************************************/
#ifdef L_isalpha

IS_FUNC_BODY(isalpha);

#endif
/**********************************************************************/
#ifdef L_isblank

/* Warning!!! This is correct for all the currently supported 8-bit locales.
 * If any are added though, this will need to be verified. */

int isblank(int c)
{
	return __isblank(c);
}

#endif
/**********************************************************************/
#ifdef L_iscntrl

IS_FUNC_BODY(iscntrl);

#endif
/**********************************************************************/
#ifdef L_isdigit

int isdigit(int c)
{
	return __isdigit(c);
}

#endif
/**********************************************************************/
#ifdef L_isgraph

IS_FUNC_BODY(isgraph);

#endif
/**********************************************************************/
#ifdef L_islower

IS_FUNC_BODY(islower);

#endif
/**********************************************************************/
#ifdef L_isprint

IS_FUNC_BODY(isprint);

#endif
/**********************************************************************/
#ifdef L_ispunct

IS_FUNC_BODY(ispunct);

#endif
/**********************************************************************/
#ifdef L_isspace

/* Warning!!! This is correct for all the currently supported 8-bit locales.
 * If any are added though, this will need to be verified. */

int isspace(int c)
{
	return __isspace(c);
}

#endif
/**********************************************************************/
#ifdef L_isupper

IS_FUNC_BODY(isupper);

#endif
/**********************************************************************/
#ifdef L_isxdigit

int isxdigit(int c)
{
	return __isxdigit(c);
}

#endif
/**********************************************************************/
#ifdef L_tolower

#ifdef __CTYPE_HAS_8_BIT_LOCALES

int tolower(int c)
{
	return ((((unsigned int) c) <= 0x7f)
			|| (LCT->encoding != __ctype_encoding_8_bit))
		? __C_tolower(c)
		: ( __isctype_loc(c, _CTYPE_isupper)
			? (unsigned char)
			( U - LCT->tbl8uplow[ ((int)
								   (LCT->idx8uplow[(U & 0x7f)
												  >> Cuplow_IDX_SHIFT])
								   << Cuplow_IDX_SHIFT)
								+ (U & ((1 << Cuplow_IDX_SHIFT) - 1)) ])
			: c );
}

#else  /* __CTYPE_HAS_8_BIT_LOCALES */

int tolower(int c)
{
	return __C_tolower(c);
}

#endif /* __CTYPE_HAS_8_BIT_LOCALES */

#endif
/**********************************************************************/
#ifdef L_toupper

#ifdef __CTYPE_HAS_8_BIT_LOCALES

int toupper(int c)
{
	return ((((unsigned int) c) <= 0x7f)
			|| (LCT->encoding != __ctype_encoding_8_bit))
		? __C_toupper(c)
		: ( __isctype_loc(c, _CTYPE_islower)
			? (unsigned char)
			( U + LCT->tbl8uplow[ ((int)
								   (LCT->idx8uplow[(U & 0x7f)
												  >> Cuplow_IDX_SHIFT])
								   << Cuplow_IDX_SHIFT)
								+ (U & ((1 << Cuplow_IDX_SHIFT) - 1)) ])
			: c );
}

#else  /* __CTYPE_HAS_8_BIT_LOCALES */

int toupper(int c)
{
	return __C_toupper(c);
}

#endif /* __CTYPE_HAS_8_BIT_LOCALES */

#endif
/**********************************************************************/
#ifdef L_isascii

int isascii(int c)
{
	return __isascii(c);
}

#endif
/**********************************************************************/
#ifdef L_toascii

int toascii(int c)
{
	return __toascii(c);
}

#endif
/**********************************************************************/
#ifdef L_isxlower

int isxlower(int c)
{
	return __isxlower(c);
}

#endif
/**********************************************************************/
#ifdef L_isxupper

int isxupper(int c)
{
	return __isxupper(c);
}

#endif
/**********************************************************************/
#ifdef L___isctype_loc
#ifdef __CTYPE_HAS_8_BIT_LOCALES

/* This internal routine is similar to iswctype(), but it doesn't
 * work for any non-standard types, itdoesn't work for "xdigit"s,
 * and it doesn't work for chars between 0 and 0x7f (although that
 * may change). */

static const char ctype_range[] = {
	__CTYPE_RANGES
};

int __isctype_loc(int c, int ct)
{
	unsigned char d;

	assert(((unsigned int)ct) < _CTYPE_isxdigit);
	assert(((unsigned int)c) > 0x7f);

#if (CHAR_MIN == 0)				/* We don't have signed chars... */
	if ((LCT->encoding != __ctype_encoding_8_bit)
		|| (((unsigned int) c) > UCHAR_MAX)
		) {
		return 0;
	}
#else
	/* Allow non-EOF negative char values for glibc compatiblity. */
	if ((LCT->encoding != __ctype_encoding_8_bit) || (c == EOF) 
		|| ( ((unsigned int)(c - CHAR_MIN)) > (UCHAR_MAX - CHAR_MIN))
		) {
		return 0;
	}
#endif

	/* TODO - test assumptions??? 8-bit chars -- or ensure in generator. */

#define Cctype_TBL_MASK		((1 << Cctype_IDX_SHIFT) - 1)
#define Cctype_IDX_OFFSET	(128 >> Cctype_IDX_SHIFT)

	c &= 0x7f;
#ifdef Cctype_PACKED
	d = LCT->tbl8ctype[ ((int)(LCT->idx8ctype[(U >> Cctype_IDX_SHIFT) ])
						 << (Cctype_IDX_SHIFT - 1))
					  + ((U & Cctype_TBL_MASK) >> 1)];
	d = (U & 1) ? (d >> 4) : (d & 0xf);
#else
	d = LCT->tbl8ctype[ ((int)(LCT->idx8ctype[(U >> Cctype_IDX_SHIFT) ])
						 << Cctype_IDX_SHIFT)
					  + (U & Cctype_TBL_MASK) ];
#endif
	return ( ((unsigned char)(d - ctype_range[2*ct])) <= ctype_range[2*ct+1] );
}

#endif /* __CTYPE_HAS_8_BIT_LOCALES */
#endif
/**********************************************************************/
