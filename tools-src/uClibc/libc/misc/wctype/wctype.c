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

#include <wctype.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <locale.h>
#include <ctype.h>

/* We know wide char support is enabled.  We wouldn't be here otherwise. */

/* Define this if you want to unify the towupper and towlower code in the
 * towctrans function. */
/*  #define SMALL_UPLOW */

#ifndef __LOCALE_C_ONLY
#define __WCTYPE_WITH_LOCALE
#endif

/**********************************************************************/

#ifndef __PASTE
#define __PASTE(X,Y)		X ## Y
#endif

#define C_MACRO(X)		__PASTE(__C_,X)(wc)

#define CT_MACRO(X)		__PASTE(__ctype_,X)(wc)

/**********************************************************************/

/* TODO: fix this! */
#ifdef __WCTYPE_WITH_LOCALE

#define WCctype			(__global_locale.tblwctype)
#define WCuplow			(__global_locale.tblwuplow)
#define WCcmob			(__global_locale.tblwcomb)
#define WCuplow_diff	(__global_locale.tblwuplow_diff)

#define ENCODING		(__global_locale.encoding)

#define ISW_FUNC_BODY(NAME) \
int NAME (wint_t wc) \
{ \
	return iswctype(wc, __PASTE(_CTYPE_,NAME)); \
}

#else  /* __WCTYPE_WITH_LOCALE */

#define ISW_FUNC_BODY(NAME) \
int NAME (wint_t wc) \
{ \
	return C_MACRO(NAME); \
}

#endif /* __WCTYPE_WITH_LOCALE */

/**********************************************************************/
#ifdef L_iswalnum

ISW_FUNC_BODY(iswalnum);

#endif
/**********************************************************************/
#ifdef L_iswalpha

ISW_FUNC_BODY(iswalpha);

#endif
/**********************************************************************/
#ifdef L_iswblank

ISW_FUNC_BODY(iswblank);

#endif
/**********************************************************************/
#ifdef L_iswcntrl

ISW_FUNC_BODY(iswcntrl);

#endif
/**********************************************************************/
#ifdef L_iswdigit

int iswdigit(wint_t wc)
{
	return __C_iswdigit(wc);
}

#endif
/**********************************************************************/
#ifdef L_iswgraph

ISW_FUNC_BODY(iswgraph);

#endif
/**********************************************************************/
#ifdef L_iswlower

ISW_FUNC_BODY(iswlower);

#endif
/**********************************************************************/
#ifdef L_iswprint

ISW_FUNC_BODY(iswprint);

#endif
/**********************************************************************/
#ifdef L_iswpunct

ISW_FUNC_BODY(iswpunct);

#endif
/**********************************************************************/
#ifdef L_iswspace

ISW_FUNC_BODY(iswspace);

#endif
/**********************************************************************/
#ifdef L_iswupper

ISW_FUNC_BODY(iswupper);

#endif
/**********************************************************************/
#ifdef L_iswxdigit

int iswxdigit(wint_t wc)
{
	return __C_iswxdigit(wc);
}

#endif
/**********************************************************************/
#ifdef L_towlower

#ifdef __WCTYPE_WITH_LOCALE

#ifdef SMALL_UPLOW

wint_t towlower(wint_t wc)
{
	return towctrans(wc, _CTYPE_tolower);
}

#else

wint_t towlower(wint_t wc)
{
	unsigned int sc, n, i;
	__uwchar_t u = wc;

	if (ENCODING == __ctype_encoding_7_bit) {
		/* We're in the C/POSIX locale, so ignore the tables. */
		return __C_towlower(wc);
	}

	if (u <= WC_TABLE_DOMAIN_MAX) {
		sc = u & ((1 << WCuplow_TI_SHIFT) - 1);
		u >>= WCuplow_TI_SHIFT;
		n = u & ((1 << WCuplow_II_SHIFT) - 1);
		u >>= WCuplow_II_SHIFT;

		i = ((unsigned int) WCuplow[u]) << WCuplow_II_SHIFT;
		i = ((unsigned int) WCuplow[WCuplow_II_LEN + i + n])
			<< WCuplow_TI_SHIFT;
		i = ((unsigned int) WCuplow[WCuplow_II_LEN + WCuplow_TI_LEN
										+ i + sc]) << 1;
		wc += WCuplow_diff[i + 1];
	}
	return wc;
}

#endif

#else  /* __WCTYPE_WITH_LOCALE */

wint_t towlower(wint_t wc)
{
	return __C_towlower(wc);
}

#endif /* __WCTYPE_WITH_LOCALE */

#endif
/**********************************************************************/
#ifdef L_towupper

#ifdef __WCTYPE_WITH_LOCALE

#ifdef SMALL_UPLOW

wint_t towupper(wint_t wc)
{
	return towctrans(wc, _CTYPE_toupper);
}

#else

wint_t towupper(wint_t wc)
{
	unsigned int sc, n, i;
	__uwchar_t u = wc;

	if (ENCODING == __ctype_encoding_7_bit) {
		/* We're in the C/POSIX locale, so ignore the tables. */
		return __C_towupper(wc);
	}

	if (u <= WC_TABLE_DOMAIN_MAX) {
		sc = u & ((1 << WCuplow_TI_SHIFT) - 1);
		u >>= WCuplow_TI_SHIFT;
		n = u & ((1 << WCuplow_II_SHIFT) - 1);
		u >>= WCuplow_II_SHIFT;

		i = ((unsigned int) WCuplow[u]) << WCuplow_II_SHIFT;
		i = ((unsigned int) WCuplow[WCuplow_II_LEN + i + n])
			<< WCuplow_TI_SHIFT;
		i = ((unsigned int) WCuplow[WCuplow_II_LEN + WCuplow_TI_LEN
										+ i + sc]) << 1;
		wc += WCuplow_diff[i];
	}
	return wc;
}

#endif

#else  /* __WCTYPE_WITH_LOCALE */

wint_t towupper(wint_t wc)
{
	return __C_towupper(wc);
}

#endif /* __WCTYPE_WITH_LOCALE */

#endif
/**********************************************************************/
#ifdef L_wctype

static const unsigned char typestring[] = __CTYPE_TYPESTRING;
/*  extern const unsigned char typestring[]; */

wctype_t wctype(const char *property)
{
	const unsigned char *p;
	int i;

	p = typestring;
	i = 1;
	do {
		if (!strcmp(property, ++p)) {
			return i;
		}
		++i;
		p += p[-1];
	} while (*p);

	/* TODO - Add locale-specific classifications. */
	return 0;
}

#endif
/**********************************************************************/
#ifdef L_iswctype

#ifdef __UCLIBC_MJN3_ONLY__
#warning duh... replace the range-based classification with table lookup!
#endif

#ifdef __WCTYPE_WITH_LOCALE

#ifdef __UCLIBC_MJN3_ONLY__
#warning TODO: need to fix locale ctype table lookup stuff
#endif
#if 0
extern const char ctype_range[];
#else
static const char ctype_range[] = {
	__CTYPE_RANGES
};
#endif

#ifdef __UCLIBC_MJN3_ONLY__
#warning TODO: need to handle combining class!
#endif

#define WCctype_TI_MASK		((1 << WCctype_TI_SHIFT) - 1)
#define WCctype_II_MASK		((1 << WCctype_II_SHIFT) - 1)

int iswctype(wint_t wc, wctype_t desc)
{
	unsigned int sc, n, i0, i1;
	unsigned char d = __CTYPE_unclassified;

	if ((ENCODING != __ctype_encoding_7_bit) || (((__uwchar_t) wc) <= 0x7f)){
		if (desc < _CTYPE_iswxdigit) {
			if (((__uwchar_t) wc) <= WC_TABLE_DOMAIN_MAX) {
				/* From here on, we know wc > 0. */
				sc = wc & WCctype_TI_MASK;
				wc >>= WCctype_TI_SHIFT;
				n = wc & WCctype_II_MASK;
				wc >>= WCctype_II_SHIFT;

				i0 = WCctype[wc];
				i0 <<= WCctype_II_SHIFT;
				i1 = WCctype[WCctype_II_LEN + i0 + n];
				i1 <<= (WCctype_TI_SHIFT-1);
				d = WCctype[WCctype_II_LEN + WCctype_TI_LEN + i1 + (sc >> 1)];

				d = (sc & 1) ? (d >> 4) : (d & 0xf);
			} else if ( ((((__uwchar_t)(wc - 0xe0020UL)) <= 0x5f)
						 || (wc == 0xe0001UL))
						|| ( (((__uwchar_t)(wc - 0xf0000UL)) < 0x20000UL)
							 && ((wc & 0xffffU) <= 0xfffdU))
						) {
				d = __CTYPE_punct;
			}

			return ( ((unsigned char)(d - ctype_range[2*desc]))
					 <= ctype_range[2*desc + 1] )
				&& ((desc != _CTYPE_iswblank) || (d & 1));
		}

		/* TODO - Add locale-specific classifications. */
		return (desc == _CTYPE_iswxdigit) ? __C_iswxdigit(wc) : 0;
	}
	return 0;
}

#else

static const unsigned char WCctype[] = {
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_space_blank     << 4),
	__CTYPE_cntrl_space_nonblank | (__CTYPE_cntrl_space_nonblank  << 4),
	__CTYPE_cntrl_space_nonblank | (__CTYPE_cntrl_space_nonblank  << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_cntrl_nonspace       | (__CTYPE_cntrl_nonspace        << 4),
	__CTYPE_print_space_blank    | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_digit                | (__CTYPE_digit                 << 4),
	__CTYPE_digit                | (__CTYPE_digit                 << 4),
	__CTYPE_digit                | (__CTYPE_digit                 << 4),
	__CTYPE_digit                | (__CTYPE_digit                 << 4),
	__CTYPE_digit                | (__CTYPE_digit                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_alpha_upper           << 4),
	__CTYPE_alpha_upper          | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_alpha_lower           << 4),
	__CTYPE_alpha_lower          | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_punct                 << 4),
	__CTYPE_punct                | (__CTYPE_cntrl_nonspace        << 4),
};

static const char ctype_range[] = {
	__CTYPE_RANGES
};

int iswctype(wint_t wc, wctype_t desc)
{
	unsigned char d = __CTYPE_unclassified;

	if (((__uwchar_t) wc) <= 0x7f) {
		if (desc < _CTYPE_iswxdigit) {
			d = WCctype[wc >> 1];
			d = (wc & 1) ? (d >> 4) : (d & 0xf);

			return ( ((unsigned char)(d - ctype_range[2*desc]))
					 <= ctype_range[2*desc + 1] )
				&& ((desc != _CTYPE_iswblank) || (d & 1));
		}

		if (desc == _CTYPE_iswxdigit) {
			return __C_isxdigit(((char) wc));
		}
	}
	return 0;
}

#endif

#endif
/**********************************************************************/
#ifdef L_towctrans

#ifdef __WCTYPE_WITH_LOCALE

#ifdef SMALL_UPLOW

wint_t towctrans(wint_t wc, wctrans_t desc)
{
	unsigned int sc, n, i;
	__uwchar_t u = wc;

	/* TODO - clean up */
	if (ENCODING == __ctype_encoding_7_bit) {
		if ((((__uwchar_t) wc) > 0x7f)
			|| (((unsigned int)(desc - _CTYPE_tolower))
				> (_CTYPE_toupper - _CTYPE_tolower))
			){
			/* We're in the C/POSIX locale, so ignore non-ASCII values
			 * as well an any mappings other than toupper or tolower. */
			return wc;
		}
	}

	if (((unsigned int)(desc - _CTYPE_tolower))
		<= (_CTYPE_totitle - _CTYPE_tolower)
		) {
		if (u <= WC_TABLE_DOMAIN_MAX) {
			sc = u & ((1 << WCuplow_TI_SHIFT) - 1);
			u >>= WCuplow_TI_SHIFT;
			n = u & ((1 << WCuplow_II_SHIFT) - 1);
			u >>= WCuplow_II_SHIFT;

			i = ((unsigned int) WCuplow[u]) << WCuplow_II_SHIFT;
			i = ((unsigned int) WCuplow[WCuplow_II_LEN + i + n])
				<< WCuplow_TI_SHIFT;
			i = ((unsigned int) WCuplow[WCuplow_II_LEN + WCuplow_TI_LEN
											+ i + sc]) << 1;
			if (desc == _CTYPE_tolower) {
				++i;
			}
			wc += WCuplow_diff[i];
			if (desc == _CTYPE_totitle) {
				/* WARNING! These special cases work for glibc 2.2.4.  Changes
				 * may be needed if the glibc locale tables are updated. */
				if ( (((__uwchar_t)(wc - 0x1c4)) <= (0x1cc - 0x1c4))
					 || (wc == 0x1f1)
					 ) {
					++wc;
				}
			}
		}
	} else {
		/* TODO - Deal with other transliterations. */
		__set_errno(EINVAL);
	}

	return wc;
}

#else

wint_t towctrans(wint_t wc, wctrans_t desc)
{
	if (ENCODING == __ctype_encoding_7_bit) {
		if ((((__uwchar_t) wc) > 0x7f)
			|| (((unsigned int)(desc - _CTYPE_tolower))
				> (_CTYPE_toupper - _CTYPE_tolower))
			){
			/* We're in the C/POSIX locale, so ignore non-ASCII values
			 * as well an any mappings other than toupper or tolower. */
			return wc;
		}
	}

	if (desc == _CTYPE_tolower) {
		return towlower(wc);
	} else if (((unsigned int)(desc - _CTYPE_toupper))
		<= (_CTYPE_totitle - _CTYPE_toupper)
		) {
		wc = towupper(wc);
		if (desc == _CTYPE_totitle) {
			/* WARNING! These special cases work for glibc 2.2.4.  Changes
			 * may be needed if the glibc locale tables are updated. */
			if ( (((__uwchar_t)(wc - 0x1c4)) <= (0x1cc - 0x1c4))
				 || (wc == 0x1f1)
				 ) {
				++wc;
			}
		}
	} else {
		/* TODO - Deal with other transliterations. */
		__set_errno(EINVAL);
	}
	return wc;
}

#endif

#else  /* __WCTYPE_WITH_LOCALE */

/* Minimal support for C/POSIX locale. */

wint_t towctrans(wint_t wc, wctrans_t desc)
{
	if (((unsigned int)(desc - _CTYPE_tolower))
		<= (_CTYPE_toupper - _CTYPE_tolower)
		) {
		/* Transliteration is either tolower or toupper. */
		if (((__uwchar_t) wc) <= 0x7f) {
			return (desc == _CTYPE_tolower) ? _tolower(wc) : _toupper(wc);
		}
	} else {
		__set_errno(EINVAL);	/* Invalid transliteration. */
	}
	return wc;
}

#endif /* __WCTYPE_WITH_LOCALE */

#endif
/**********************************************************************/
#ifdef L_wctrans

static const char transstring[] = __CTYPE_TRANSTRING;

wctrans_t wctrans(const char *property)
{
	const unsigned char *p;
	int i;

	p = transstring;
	i = 1;
	do {
		if (!strcmp(property, ++p)) {
			return i;
		}
		++i;
		p += p[-1];
	} while (*p);

	/* TODO - Add locale-specific translations. */
	return 0;
}

#endif
/**********************************************************************/
