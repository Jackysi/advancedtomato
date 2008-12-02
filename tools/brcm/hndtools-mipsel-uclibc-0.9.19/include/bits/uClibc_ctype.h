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

#if !defined(_CTYPE_H) && !defined(_WCTYPE_H)
#error Always include <{w}ctype.h> rather than <bits/uClibc_ctype.h>
#endif

#ifndef _BITS_CTYPE_H
#define _BITS_CTYPE_H

/* Taking advantage of the C99 mutual-exclusion guarantees for the various
 * (w)ctype classes, including the descriptions of printing and control
 * (w)chars, we can place each in one of the following mutually-exlusive
 * subsets.  Since there are less than 16, we can store the data for
 * each (w)chars in a nibble. In contrast, glibc uses an unsigned int
 * per (w)char, with one bit flag for each is* type.  While this allows
 * a simple '&' operation to determine the type vs. a range test and a
 * little special handling for the "blank" and "xdigit" types in my
 * approach, it also uses 8 times the space for the tables on the typical
 * 32-bit archs we supported.*/
enum {
	__CTYPE_unclassified = 0,
	__CTYPE_alpha_nonupper_nonlower,
	__CTYPE_alpha_lower,
	__CTYPE_alpha_upper_lower,
	__CTYPE_alpha_upper,
	__CTYPE_digit,
	__CTYPE_punct,
	__CTYPE_graph,
	__CTYPE_print_space_nonblank,
	__CTYPE_print_space_blank,
	__CTYPE_space_nonblank_noncntrl,
	__CTYPE_space_blank_noncntrl,
	__CTYPE_cntrl_space_nonblank,
	__CTYPE_cntrl_space_blank,
	__CTYPE_cntrl_nonspace
};

/* Some macros that test for various (w)ctype classes when passed one of the
 * designator values enumerated above. */
#define __CTYPE_isalnum(D)		((unsigned int)(D-1) <= (__CTYPE_digit-1))
#define __CTYPE_isalpha(D)		((unsigned int)(D-1) <= (__CTYPE_alpha_upper-1))
#define __CTYPE_isblank(D) \
	((((unsigned int)(D - __CTYPE_print_space_nonblank)) <= 5) && (D & 1))
#define __CTYPE_iscntrl(D)		(((unsigned int)(D - __CTYPE_cntrl_space_nonblank)) <= 2)
#define __CTYPE_isdigit(D)		(D == __CTYPE_digit)
#define __CTYPE_isgraph(D)		((unsigned int)(D-1) <= (__CTYPE_graph-1))
#define __CTYPE_islower(D)		(((unsigned int)(D - __CTYPE_alpha_lower)) <= 1)
#define __CTYPE_isprint(D)		((unsigned int)(D-1) <= (__CTYPE_print_space_blank-1))
#define __CTYPE_ispunct(D)		(D == __CTYPE_punct)
#define __CTYPE_isspace(D)		(((unsigned int)(D - __CTYPE_print_space_nonblank)) <= 5)
#define __CTYPE_isupper(D)		(((unsigned int)(D - __CTYPE_alpha_upper_lower)) <= 1)
/*  #define __CTYPE_isxdigit(D) -- isxdigit is untestable this way. 
 *  But that's ok as isxdigit() (and isdigit() too) are locale-invariant. */

/* The values for wctype_t. */
enum {
	_CTYPE_unclassified = 0,
	_CTYPE_isalnum,
	_CTYPE_isalpha,
	_CTYPE_isblank,
	_CTYPE_iscntrl,
	_CTYPE_isdigit,
	_CTYPE_isgraph,
	_CTYPE_islower,
	_CTYPE_isprint,
	_CTYPE_ispunct,
	_CTYPE_isspace,
	_CTYPE_isupper,
	_CTYPE_isxdigit				/* _MUST_ be last of the standard classes! */
};


/* The following is used to implement wctype(), but it is defined
 * here because the ordering must agree with that of the enumeration
 * above (ignoring unclassified). */
#define __CTYPE_TYPESTRING \
	"\6alnum\0\6alpha\0\6blank\0\6cntrl\0\6digit\0\6graph\0\6lower\0" \
	"\6print\0\6punct\0\6space\0\6upper\0\7xdigit\0\0"

/* Used in implementing iswctype(), but defined here as it must agree
 * in ordering with the string above. */
#define __CTYPE_RANGES \
	0, -1,								/* unclassified */ \
	1, __CTYPE_digit - 1,				/* alnum */ \
	1, __CTYPE_alpha_upper - 1,			/* alpha */ \
	__CTYPE_print_space_blank, 5,		/* blank -- also must be odd! */ \
	__CTYPE_cntrl_space_nonblank, 2,	/* cntrl */ \
	__CTYPE_digit, 0,					/* digit */ \
	1, __CTYPE_graph - 1,				/* graph */ \
	__CTYPE_alpha_lower, 1,				/* lower */ \
	1, __CTYPE_print_space_blank - 1,	/* print */ \
	__CTYPE_punct, 0,					/* punct */ \
	__CTYPE_print_space_nonblank, 5, 	/* space */ \
	__CTYPE_alpha_upper_lower, 1,		/* upper */ \
	/* No entry for xdigit as it is handled specially. */

#define _CTYPE_iswalnum		_CTYPE_isalnum
#define _CTYPE_iswalpha		_CTYPE_isalpha
#define _CTYPE_iswblank		_CTYPE_isblank
#define _CTYPE_iswcntrl		_CTYPE_iscntrl
#define _CTYPE_iswdigit		_CTYPE_isdigit
#define _CTYPE_iswgraph		_CTYPE_isgraph
#define _CTYPE_iswlower		_CTYPE_islower
#define _CTYPE_iswprint		_CTYPE_isprint
#define _CTYPE_iswpunct		_CTYPE_ispunct
#define _CTYPE_iswspace		_CTYPE_isspace
#define _CTYPE_iswupper		_CTYPE_isupper
#define _CTYPE_iswxdigit	_CTYPE_isxdigit

/* The following is used to implement wctrans(). */

enum {
	_CTYPE_tolower = 1,
	_CTYPE_toupper,
	_CTYPE_totitle
};

#define __CTYPE_TRANSTRING	"\10tolower\0\10toupper\0\10totitle\0\0"

/* Now define some ctype macros valid for the C/POSIX locale. */

/* ASCII ords of \t, \f, \n, \r, and \v are 9, 12, 10, 13, 11 respectively. */
#define __C_isspace(c) \
	((sizeof(c) == sizeof(char)) \
	 ? ((((c) == ' ') || (((unsigned char)((c) - 9)) <= (13 - 9)))) \
	 : ((((c) == ' ') || (((unsigned int)((c) - 9)) <= (13 - 9)))))
#define __C_isblank(c) (((c) == ' ') || ((c) == '\t'))
#define __C_isdigit(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - '0')) < 10) \
	 : (((unsigned int)((c) - '0')) < 10))
#define __C_isxdigit(c) \
	(__C_isdigit(c) \
	 || ((sizeof(c) == sizeof(char)) \
		 ? (((unsigned char)((((c)) | 0x20) - 'a')) < 6) \
		 : (((unsigned int)((((c)) | 0x20) - 'a')) < 6)))
#define __C_iscntrl(c) \
	((sizeof(c) == sizeof(char)) \
	 ? ((((unsigned char)(c)) < 0x20) || ((c) == 0x7f)) \
	 : ((((unsigned int)(c)) < 0x20) || ((c) == 0x7f)))
#define __C_isalpha(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)(((c) | 0x20) - 'a')) < 26) \
	 : (((unsigned int)(((c) | 0x20) - 'a')) < 26))
#define __C_isalnum(c) (__C_isalpha(c) || __C_isdigit(c))
#define __C_isprint(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - 0x20)) <= (0x7e - 0x20)) \
	 : (((unsigned int)((c) - 0x20)) <= (0x7e - 0x20)))
#define __C_islower(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - 'a')) < 26) \
	 : (((unsigned int)((c) - 'a')) < 26))
#define __C_isupper(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned char)((c) - 'A')) < 26) \
	 : (((unsigned int)((c) - 'A')) < 26))
#define __C_ispunct(c) \
	((!__C_isalnum(c)) \
	 && ((sizeof(c) == sizeof(char)) \
		 ? (((unsigned char)((c) - 0x21)) <= (0x7e - 0x21)) \
		 : (((unsigned int)((c) - 0x21)) <= (0x7e - 0x21))))
#define __C_isgraph(c) \
	((sizeof(c) == sizeof(char)) \
	 ? (((unsigned int)((c) - 0x21)) <= (0x7e - 0x21)) \
	 : (((unsigned int)((c) - 0x21)) <= (0x7e - 0x21)))

#define __C_tolower(c) (__C_isupper(c) ? ((c) | 0x20) : (c))
#define __C_toupper(c) (__C_islower(c) ? ((c) ^ 0x20) : (c))

#define __C_isxlower(c) \
	(__C_isdigit(c) \
	 || ((sizeof(c) == sizeof(char)) \
		 ? (((unsigned char)(((c)) - 'a')) < 6) \
		 : (((unsigned int)(((c)) - 'a')) < 6)))
#define __C_isxupper(c) \
	(__C_isdigit(c) \
	 || ((sizeof(c) == sizeof(char)) \
		 ? (((unsigned char)(((c)) - 'A')) < 6) \
		 : (((unsigned int)(((c)) - 'A')) < 6)))

/* TODO: Replace the above with expressions like the following? */
/*  #define __C_isdigit(c) ((sizeof(c) == sizeof(char)) \ */
/*  						? (((unsigned char)((c) - '0')) < 10) \ */
/*  						: (((unsigned int)((c) - '0')) < 10)) */

/* Similarly, define some wctype macros valid for the C/POSIX locale. */

/* First, we need some way to make sure the arg is in range. */
#define __C_classed(c) \
	((sizeof(c) <= sizeof(int)) || (c == ((unsigned char)c)))

#define __C_iswspace(c)		(__C_classed(c) && __C_isspace(c))
#define __C_iswblank(c)		(__C_classed(c) && __C_isblank(c))
#define __C_iswdigit(c)		(__C_classed(c) && __C_isdigit(c))
#define __C_iswxdigit(c)	(__C_classed(c) && __C_isxdigit(c))
#define __C_iswcntrl(c)		(__C_classed(c) && __C_iscntrl(c))
#define __C_iswalpha(c)		(__C_classed(c) && __C_isalpha(c))
#define __C_iswalnum(c)		(__C_classed(c) && __C_isalnum(c))
#define __C_iswprint(c)		(__C_classed(c) && __C_isprint(c))
#define __C_iswlower(c)		(__C_classed(c) && __C_islower(c))
#define __C_iswupper(c)		(__C_classed(c) && __C_isupper(c))
#define __C_iswpunct(c)		(__C_classed(c) && __C_ispunct(c))
#define __C_iswgraph(c)		(__C_classed(c) && __C_isgraph(c))
#define __C_towlower(c) \
	((__C_classed(c) && __C_isupper(c)) ? ((c) | 0x20) : (c))
#define __C_towupper(c) \
	((__C_classed(c) && __C_islower(c)) ? ((c) ^ 0x20) : (c))

/* Now define some macros to aviod the extra wrapper-function call. */
#define __iswalnum(c)		iswctype(c, _CTYPE_iswalnum)
#define __iswalpha(c)		iswctype(c, _CTYPE_iswalpha)
#define __iswblank(c)		iswctype(c, _CTYPE_iswblank)
#define __iswcntrl(c)		iswctype(c, _CTYPE_iswcntrl)
#define __iswgraph(c)		iswctype(c, _CTYPE_iswgraph)
#define __iswlower(c)		iswctype(c, _CTYPE_iswlower)
#define __iswprint(c)		iswctype(c, _CTYPE_iswprint)
#define __iswpunct(c)		iswctype(c, _CTYPE_iswpunct)
#define __iswspace(c)		iswctype(c, _CTYPE_iswspace)
#define __iswupper(c)		iswctype(c, _CTYPE_iswupper)
#define __iswdigit(c)		__C_iswdigit(c)
#define __iswxdigit(c)		__C_iswxdigit(c)

#endif /* _BITS_CTYPE_H */
