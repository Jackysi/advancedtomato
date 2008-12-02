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

/* Nov. 1, 2002
 *
 * Reworked setlocale() return values and locale arg processing to
 *   be more like glibc.  Applications expecting to be able to
 *   query locale settings should now work... at the cost of almost
 *   doubling the size of the setlocale object code.
 * Fixed a bug in the internal fixed-size-string locale specifier code.
 *
 * Dec 20, 2002
 *
 * Added in collation support and updated stub nl_langinfo.
 */


/*  TODO:
 *  Implement the shared mmap code so non-mmu platforms can use this.
 *  Add some basic collate functionality similar to what the previous
 *    locale support had (8-bit codesets only).
 */

#define _GNU_SOURCE
#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <limits.h>
#include <stdint.h>
#include <assert.h>

#ifndef __LOCALE_C_ONLY

#define CUR_LOCALE_SPEC			(__global_locale.cur_locale)
#undef CODESET_LIST
#define CODESET_LIST			(__locale_mmap->codeset_list)

#endif /* __LOCALE_C_ONLY */

/**********************************************************************/
#ifdef L_setlocale

#ifdef __LOCALE_C_ONLY

link_warning(setlocale,"the 'setlocale' function supports only C|POSIX locales")

static const char C_string[] = "C";

char *setlocale(int category, register const char *locale)
{
	return ( (((unsigned int)(category)) <= LC_ALL)
			 && ( (!locale)		/* Request for locale category string. */
				  || (!*locale)	/* Implementation-defined default is C. */
				  || ((*locale == 'C') && !locale[1])
				  || (!strcmp(locale, "POSIX"))) )
		? (char *) C_string		/* Always in C/POSIX locale. */
		: NULL;
}

#else  /* ---------------------------------------------- __LOCALE_C_ONLY */

#if !defined(NUM_LOCALES) || (NUM_LOCALES <= 1)
#error locales enabled, but not data other than for C locale!
#endif

#define LOCALE_NAMES			(__locale_mmap->locale_names5)
#define LOCALES					(__locale_mmap->locales)
#define LOCALE_AT_MODIFIERS 	(__locale_mmap->locale_at_modifiers)
#define CATEGORY_NAMES			(__locale_mmap->lc_names)

static const char posix[] = "POSIX";
static const char utf8[] = "UTF-8";

#ifdef __UCLIBC_MJN3_ONLY__
#warning REMINDER: redo the MAX_LOCALE_STR stuff...
#endif
#define MAX_LOCALE_STR    256 /* TODO: Only sufficient for current case. */

static char hr_locale[MAX_LOCALE_STR];

static __inline char *human_readable_locale(int category, const unsigned char *s)
{
	const unsigned char *loc;
	char *n;
	int i;

	++s;

	if (category == LC_ALL) {
		for (i = 0 ; i < LC_ALL-1 ; i += 2) {
			if ((s[i] != s[i+2]) || (s[i+1] != s[i+3])) {
				goto SKIP;
			}
		}
		/* All categories the same, so simplify string by using a single
		 * category. */
		category = LC_CTYPE;
	}

 SKIP:
	i = (category == LC_ALL) ? 0 : category;
	s += 2*i;
	n = hr_locale;

	do {
		if ((*s != 0xff) || (s[1] != 0xff)) {
			loc = LOCALES + WIDTH_LOCALES * ((((int)(*s & 0x7f)) << 7) + (s[1] & 0x7f));
			if (category == LC_ALL) {
				n = stpcpy(n, CATEGORY_NAMES + (int) CATEGORY_NAMES[i]);
				*n++ = '=';
			}
			if (*loc == 0) {
				*n++ = 'C';
				*n = 0;
			} else {
				char at = 0;
				memcpy(n, LOCALE_NAMES + 5*((*loc)-1), 5);
				if (n[2] != '_') {
					at = n[2];
					n[2] = '_';
				}
				n += 5;
				*n++ = '.';
				if (loc[2] == 2) {
					n = stpcpy(n, utf8);
				} else if (loc[2] >= 3) {
					n = stpcpy(n, CODESET_LIST + (int)(CODESET_LIST[loc[2] - 3]));
				}
				if (at) {
					const char *q;
					*n++ = '@';
					q = LOCALE_AT_MODIFIERS;
					do {
						if (q[1] == at) {
							n = stpcpy(n, q+2);
							break;
						}
						q += 2 + *q;
					} while (*q);
				}
			}
			*n++ = ';';
		}
		s += 2;
	} while (++i < category);

	*--n = 0;					/* Remove trailing ';' and nul-terminate. */
	assert(n-hr_locale < MAX_LOCALE_STR);
	return hr_locale;
}

static int find_locale(int category, const char *p, unsigned char *new_locale)
{
	int i;
	const unsigned char *s;
	uint16_t n;
	unsigned char lang_cult, codeset;

#if defined(LOCALE_AT_MODIFIERS_LENGTH) && 1
	/* Support standard locale handling for @-modifiers. */

#ifdef __UCLIBC_MJN3_ONLY__
#warning REMINDER: fix buf size in find_locale
#endif
	char buf[18];	/* TODO: 7+{max codeset name length} */
	const char *q;

	if ((q = strchr(p,'@')) != NULL) {
		if ((((size_t)((q-p)-5)) > (sizeof(buf) - 5)) || (p[2] != '_')) {
			return 0;
		}
		/* locale name at least 5 chars long and 3rd char is '_' */
		s = LOCALE_AT_MODIFIERS;
		do {
			if (!strcmp(s+2, q+1)) {
				break;
			}
			s += 2 + *s;		/* TODO - fix this throughout */
		} while (*s);
		if (!*s) {
			return 0;
		}
		assert(q - p < sizeof(buf));
		memcpy(buf, p, q-p);
		buf[q-p] = 0;
		buf[2] = s[1];
		p = buf;
	}
#endif

	lang_cult = codeset = 0;	/* Assume C and default codeset.  */
	if (((*p == 'C') && !p[1]) || !strcmp(p, posix)) {
		goto FIND_LOCALE;
	}

	if ((strlen(p) > 5) && (p[5] == '.')) {	/* Codeset in locale name? */
		/* TODO: maybe CODESET_LIST + *s ??? */
		/* 7bit is 1, UTF-8 is 2, 8-bit is >= 3 */
		codeset = 2;
		if (strcmp(utf8,p+6) != 0) {/* TODO - fix! */
			s = CODESET_LIST;
			do {
				++codeset;		/* Increment codeset first. */
				if (!strcmp(CODESET_LIST+*s, p+6)) {
					goto FIND_LANG_CULT;
				}
			} while (*++s);
			return 0;			/* No matching codeset! */
		}
	}

 FIND_LANG_CULT:				/* Find language_culture number. */
	s = LOCALE_NAMES;
	do {						/* TODO -- do a binary search? */
		/* TODO -- fix gen_mmap!*/
		++lang_cult;			/* Increment first since C/POSIX is 0. */
		if (!strncmp(s,p,5)) { /* Found a matching locale name; */
			goto FIND_LOCALE;
		}
		s += 5;
	} while (lang_cult < NUM_LOCALE_NAMES);
	return 0;					/* No matching language_culture! */

 FIND_LOCALE:					/* Find locale row matching name and codeset */
	s = LOCALES;
	n = 0;
	do {						/* TODO -- do a binary search? */
		if ((lang_cult == *s) && ((codeset == s[1]) || (codeset == s[2]))) {
			i = ((category == LC_ALL) ? 0 : category);
			s = new_locale + 2*i;
			do {
				/* Encode current locale row number. */
				*((unsigned char *) ++s) = (n >> 7) | 0x80;
				*((unsigned char *) ++s) = (n & 0x7f) | 0x80;
			} while (++i < category);

			return i;			/* Return non-zero */
		}
		s += WIDTH_LOCALES;
		++n;
	} while (n <= NUM_LOCALES);	/* We started at 1!!! */

	return 0;					/* Unsupported locale. */
}

static unsigned char *composite_locale(int category, const char *locale, unsigned char *new_locale)
{
	char buf[MAX_LOCALE_STR];
	char *t;
	char *e;
	int c;

	if (!strchr(locale,'=')) {
		if (!find_locale(category, locale, new_locale)) {
			return NULL;
		}
		return new_locale;
	}

	if (strlen(locale) >= sizeof(buf)) {
		return NULL;
	}
	stpcpy(buf, locale);

	t = strtok_r(buf, "=", &e);	/* This can't fail because of strchr test above. */
	do {
		for (c = 0 ; c < LC_ALL ; c++) { /* Find the category... */
			if (!strcmp(CATEGORY_NAMES + (int) CATEGORY_NAMES[c], t)) {
				break;
			}
		}
		t = strtok_r(NULL, ";", &e);
		if ((category == LC_ALL) || (c == category)) {
			if (!t || !find_locale(c, t, new_locale)) {
				return NULL;
			}
		}
	} while ((t = strtok_r(NULL, "=", &e)) != NULL);

	return new_locale;
}

char *setlocale(int category, const char *locale)
{
	const unsigned char *p;
	int i;
	unsigned char new_locale[LOCALE_STRING_SIZE];

	if (((unsigned int)(category)) > LC_ALL) {
		/* TODO - set errno?  SUSv3 doesn't say too. */
		return NULL;			/* Illegal/unsupported category. */
	}

	if (locale != NULL) {  /* Not just a query... */
		stpcpy(new_locale, CUR_LOCALE_SPEC); /* Start with current. */

		if (!*locale) {				/* locale == "", so check environment. */
			i = ((category == LC_ALL) ? 0 : category);
			do {
				/* Note: SUSv3 doesn't define a fallback mechanism here.  So,
				 * if LC_ALL is invalid, we do _not_ continue trying the other
				 * environment vars. */
				if (!(p = getenv("LC_ALL"))) {
					if (!(p = getenv(CATEGORY_NAMES + CATEGORY_NAMES[i]))) {
						if (!(p = getenv("LANG"))) {
							p = posix;
						}
					}
				}

				/* The user set something... is it valid? */
				/* Note: Since we don't support user-supplied locales and
				 * alternate paths, we don't need to worry about special
				 * handling for suid/sgid apps. */
				if (!find_locale(i, p, new_locale)) {
					return NULL;
				}
			} while (++i < category);
		} else if (!composite_locale(category, locale, new_locale)) {
			return NULL;
		}

		/* TODO: Ok, everything checks out, so install the new locale. */
		_locale_set(new_locale);
	}

	/* Either a query or a successful set, so return current locale string. */
	return human_readable_locale(category, CUR_LOCALE_SPEC);
}

#endif /* __LOCALE_C_ONLY */

#endif
/**********************************************************************/
#ifdef L_localeconv

/* Note: We assume here that the compiler does the sane thing regarding
 * placement of the fields in the struct.  If necessary, we could ensure
 * this usings an array of offsets but at some size cost. */

#ifdef __LOCALE_C_ONLY

link_warning(localeconv,"the 'localeconv' function is hardwired for C/POSIX locale only")

static struct lconv the_lconv;

static const char decpt[] = ".";

struct lconv *localeconv(void)
{
	register char *p = (char *)(&the_lconv);

	*((char **)p) = (char *) decpt;
	do {
		p += sizeof(char **);
		*((char **)p) = (char *) (decpt+1);
	} while (p < (char *) &the_lconv.negative_sign);

	p = (&the_lconv.int_frac_digits);
	do {
		*p = CHAR_MAX;
		++p;
	} while (p <= &the_lconv.int_n_sign_posn);

	return &the_lconv;
}

#else  /* __LOCALE_C_ONLY */

static struct lconv the_lconv;

struct lconv *localeconv(void)
{
	register char *p = (char *) &the_lconv;
	register char **q = (char **) &__global_locale.decimal_point;

	do {
		*((char **)p) = *q;
		p += sizeof(char **);
		++q;
	} while (p < &the_lconv.int_frac_digits);

	do {
		*p = **q;
		++p;
		++q;
	} while (p <= &the_lconv.int_n_sign_posn);

	return &the_lconv;
}

#endif /* __LOCALE_C_ONLY */

#endif
/**********************************************************************/
#ifdef L__locale_init

#ifndef __LOCALE_C_ONLY

#define C_LOCALE_SELECTOR "\x23\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80"
#define LOCALE_INIT_FAILED "locale init failed!\n"

#define CUR_LOCALE_SPEC			(__global_locale.cur_locale)

__locale_t __global_locale;

typedef struct {
	uint16_t num_base;
	uint16_t num_der;
	uint16_t MAX_WEIGHTS;
	uint16_t num_index2weight;
#define num_index2ruleidx num_index2weight
	uint16_t num_weightstr;
	uint16_t num_multistart;
	uint16_t num_override;
	uint16_t num_ruletable;
} coldata_header_t;

typedef struct {
	uint16_t num_weights;
	uint16_t num_starters;
	uint16_t ii_shift;
	uint16_t ti_shift;
	uint16_t ii_len;
	uint16_t ti_len;
	uint16_t max_weight;
	uint16_t num_col_base;
	uint16_t max_col_index;
	uint16_t undefined_idx;
	uint16_t range_low;
	uint16_t range_count;
	uint16_t range_base_weight;
	uint16_t range_rule_offset;

	uint16_t index2weight_offset;
	uint16_t index2ruleidx_offset;
	uint16_t multistart_offset;
	uint16_t wcs2colidt_offset_low;
	uint16_t wcs2colidt_offset_hi;
} coldata_base_t;

typedef struct {
	uint16_t base_idx;
	uint16_t undefined_idx;
	uint16_t overrides_offset;
	uint16_t multistart_offset;
} coldata_der_t;

static int init_cur_collate(int der_num)
{
	__collate_t *cur_collate = &__global_locale.collate;
	const uint16_t *__locale_collate_tbl = __locale_mmap->collate_data;
	coldata_header_t *cdh;
	coldata_base_t *cdb;
	coldata_der_t *cdd;
	const uint16_t *p;
	size_t n;
	uint16_t i, w;

	assert(sizeof(coldata_base_t) == 19*2);
	assert(sizeof(coldata_der_t) == 4*2);
	assert(sizeof(coldata_header_t) == 8*2);

	if (!der_num) { 			/* C locale... special */
		cur_collate->num_weights = 0;
		return 1;
	}

	--der_num;

	cdh = (coldata_header_t *) __locale_collate_tbl;

	if (der_num >= cdh->num_der) {
		return 0;
	}

	cdd = (coldata_der_t *)(__locale_collate_tbl
							+ (sizeof(coldata_header_t)
							   + cdh->num_base * sizeof(coldata_base_t)
							   + der_num * sizeof(coldata_der_t)
							   )/2 );

	cdb = (coldata_base_t *)(__locale_collate_tbl
							 + (sizeof(coldata_header_t)
								+ cdd->base_idx * sizeof(coldata_base_t)
								)/2 );

	memcpy(cur_collate, cdb, offsetof(coldata_base_t,index2weight_offset));
	cur_collate->undefined_idx = cdd->undefined_idx;

	cur_collate->ti_mask = (1 << cur_collate->ti_shift)-1;
	cur_collate->ii_mask = (1 << cur_collate->ii_shift)-1;

/*	 printf("base=%d  num_col_base: %d  %d\n", cdd->base_idx ,cur_collate->num_col_base, cdb->num_col_base); */

	n = (sizeof(coldata_header_t) + cdh->num_base * sizeof(coldata_base_t)
		 + cdh->num_der * sizeof(coldata_der_t))/2;

/*	 printf("n   = %d\n", n); */
	cur_collate->index2weight_tbl = __locale_collate_tbl + n + cdb->index2weight_offset;
/*	 printf("i2w = %d\n", n + cdb->index2weight_offset); */
	n += cdh->num_index2weight;
	cur_collate->index2ruleidx_tbl = __locale_collate_tbl + n + cdb->index2ruleidx_offset;
/*	 printf("i2r = %d\n", n + cdb->index2ruleidx_offset); */
	n += cdh->num_index2ruleidx;
	cur_collate->multistart_tbl = __locale_collate_tbl + n + cdd->multistart_offset;
/*	 printf("mts = %d\n", n + cdb->multistart_offset); */
	n += cdh->num_multistart;
	cur_collate->overrides_tbl = __locale_collate_tbl + n + cdd->overrides_offset;
/*	 printf("ovr = %d\n", n + cdd->overrides_offset); */
	n += cdh->num_override;
	cur_collate->ruletable = __locale_collate_tbl + n;
/*	 printf("rtb = %d\n", n); */
	n += cdh->num_ruletable;
	cur_collate->weightstr = __locale_collate_tbl + n;
/*	 printf("wts = %d\n", n); */
	n += cdh->num_weightstr;
	cur_collate->wcs2colidt_tbl = __locale_collate_tbl + n
		+ (((unsigned long)(cdb->wcs2colidt_offset_hi)) << 16)
		+ cdb->wcs2colidt_offset_low;
/*	 printf("wcs = %lu\n", n	+ (((unsigned long)(cdb->wcs2colidt_offset_hi)) << 16) */
/* 		   + cdb->wcs2colidt_offset_low); */

	cur_collate->MAX_WEIGHTS = cdh->MAX_WEIGHTS;

#ifdef __UCLIBC_MJN3_ONLY__
#warning if calloc fails, this is WRONG.  there is also a memory leak here at the moment
#warning fix the +1 by increasing max_col_index?
#endif
	cur_collate->index2weight = calloc(2*cur_collate->max_col_index+2, sizeof(uint16_t));
	if (!cur_collate->index2weight) {
		return 0;
	}
	cur_collate->index2ruleidx = cur_collate->index2weight + cur_collate->max_col_index + 1;

	memcpy(cur_collate->index2weight, cur_collate->index2weight_tbl,
		   cur_collate->num_col_base * sizeof(uint16_t));
	memcpy(cur_collate->index2ruleidx, cur_collate->index2ruleidx_tbl,
		   cur_collate->num_col_base * sizeof(uint16_t));

	/* now do the overrides */
	p = cur_collate->overrides_tbl;
	while (*p > 1) {
/* 		fprintf(stderr, "processing override -- count = %d\n", *p); */
		n = *p++;
		w = *p++;
		do {
			i = *p++;
/* 			fprintf(stderr, "	i=%d w=%d *p=%d\n", i, w, *p); */
			cur_collate->index2weight[i-1] = w++;
			cur_collate->index2ruleidx[i-1] = *p++;
		} while (--n);
	}
	while (*++p) {
		i = *p;
		cur_collate->index2weight[i-1] = *++p;
		cur_collate->index2ruleidx[i-1] = *++p;
	}


	for (i=0 ; i < cur_collate->multistart_tbl[0] ; i++) {
		p = cur_collate->multistart_tbl;
/* 		fprintf(stderr, "%2d of %2d: %d ", i,  cur_collate->multistart_tbl[0], p[i]); */
		p += p[i];

		do {
			n = *p++;
			do {
				if (!*p) {		/* found it */
/* 					fprintf(stderr, "found: n=%d (%#lx) |%.*ls|\n", n, (int) *cs->s, n, cs->s); */
/* 					fprintf(stderr, ": %d - single\n", n); */
					goto FOUND;
 				}
				/* the lookup check here is safe since we're assured that *p is a valid colidex */
/* 				fprintf(stderr, "lookup(%lc)==%d  *p==%d\n", cs->s[n], lookup(cs->s[n]), (int) *p); */
/* 				fprintf(stderr, ": %d - ", n); */
				do {
/* 					fprintf(stderr, "%d|",  *p); */
				} while (*p++);
				break;
			} while (1);
		} while (1);
	FOUND:
		continue;
	}

	return 1;
}

void _locale_init(void)
{
	/* TODO: mmap the locale file  */

	/* TODO - ??? */
	memset(CUR_LOCALE_SPEC, 0, LOCALE_STRING_SIZE);
	CUR_LOCALE_SPEC[0] = '#';

	memcpy(__global_locale.category_item_count,
		   __locale_mmap->lc_common_item_offsets_LEN,
		   LC_ALL);

	++__global_locale.category_item_count[0]; /* Increment for codeset entry. */
	__global_locale.category_offsets[0] = offsetof(__locale_t, outdigit0_mb);
	__global_locale.category_offsets[1] = offsetof(__locale_t, decimal_point);
	__global_locale.category_offsets[2] = offsetof(__locale_t, int_curr_symbol);
	__global_locale.category_offsets[3] = offsetof(__locale_t, abday_1);
/*  	__global_locale.category_offsets[4] = offsetof(__locale_t, collate???); */
	__global_locale.category_offsets[5] = offsetof(__locale_t, yesexpr);

#ifdef __CTYPE_HAS_8_BIT_LOCALES
	__global_locale.tbl8ctype
		= (const unsigned char *) &__locale_mmap->tbl8ctype;
    __global_locale.tbl8uplow
		= (const unsigned char *) &__locale_mmap->tbl8uplow;
#ifdef __WCHAR_ENABLED
	__global_locale.tbl8c2wc
		= (const uint16_t *) &__locale_mmap->tbl8c2wc;
	__global_locale.tbl8wc2c
		= (const unsigned char *) &__locale_mmap->tbl8wc2c;
	/* translit  */
#endif /* __WCHAR_ENABLED */
#endif /* __CTYPE_HAS_8_BIT_LOCALES */
#ifdef __WCHAR_ENABLED
	__global_locale.tblwctype
		= (const unsigned char *) &__locale_mmap->tblwctype;
	__global_locale.tblwuplow
		= (const unsigned char *) &__locale_mmap->tblwuplow;
	__global_locale.tblwuplow_diff
		= (const uint16_t *) &__locale_mmap->tblwuplow_diff;
/* 	__global_locale.tblwcomb */
/* 		= (const unsigned char *) &__locale_mmap->tblwcomb; */
	/* width?? */
#endif /* __WCHAR_ENABLED */

	_locale_set(C_LOCALE_SELECTOR);
}

static const char ascii[] = "ASCII";
static const char utf8[] = "UTF-8";

void _locale_set(const unsigned char *p)
{
	const char **x;
	unsigned char *s = CUR_LOCALE_SPEC + 1;
	const size_t *stp;
	const unsigned char *r;
	const uint16_t *io;
	const uint16_t *ii;
	const unsigned char *d;
	int row;					/* locale row */
	int crow;					/* category row */
	int len;
	int c;
	int i = 0;

	++p;
	do {
		if ((*p != *s) || (p[1] != s[1])) {
			row  = (((int)(*p & 0x7f)) << 7) + (p[1] & 0x7f);
			assert(row < NUM_LOCALES);

			*s = *p;
			s[1] = p[1];

			if ((i != LC_COLLATE)
				&& ((len = __locale_mmap->lc_common_item_offsets_LEN[i]) != 0)
				) {
				crow = __locale_mmap->locales[ WIDTH_LOCALES * row + 3 + i ]
					* len;
				x = (const char **)(((char *) &__global_locale)
									+ __global_locale.category_offsets[i]);
				stp = __locale_mmap->lc_common_tbl_offsets + 4*i;
				r = (const unsigned char *)( ((char *)__locale_mmap) + *stp );
				io = (const uint16_t *)( ((char *)__locale_mmap) + *++stp );
				ii = (const uint16_t *)( ((char *)__locale_mmap) + *++stp );
				d = (const unsigned char *)( ((char *)__locale_mmap) + *++stp );
				for (c=0 ; c < len ; c++) {
					*(x + c) = d + ii[ r[crow + c] + io[c] ];
				}
			}
			if (i == LC_CTYPE) {
				c = __locale_mmap->locales[ WIDTH_LOCALES * row + 2 ]; /* codeset */
				if (c <= 2) {
					if (c == 2) {
						__global_locale.codeset = utf8;
						__global_locale.encoding = __ctype_encoding_utf8;
						/* TODO - fix for bcc */
						__global_locale.mb_cur_max = 6;
					} else {
						assert(c==1);
						__global_locale.codeset = ascii;
						__global_locale.encoding = __ctype_encoding_7_bit;
						__global_locale.mb_cur_max = 1;
					}
				} else {
					const codeset_8_bit_t *c8b;
					r = CODESET_LIST;
					__global_locale.codeset = r + r[c -= 3];
					__global_locale.encoding = __ctype_encoding_8_bit;
#ifdef __UCLIBC_MJN3_ONLY__
#warning REMINDER: update 8 bit mb_cur_max when trasnlit implemented!
#endif
					/* TODO - update when translit implemented! */
					__global_locale.mb_cur_max = 1;
					c8b = __locale_mmap->codeset_8_bit + c;
#ifdef __CTYPE_HAS_8_BIT_LOCALES
					__global_locale.idx8ctype = c8b->idx8ctype;
					__global_locale.idx8uplow = c8b->idx8uplow;
#ifdef __WCHAR_ENABLED
					__global_locale.idx8c2wc = c8b->idx8c2wc;
					__global_locale.idx8wc2c = c8b->idx8wc2c;
					/* translit  */
#endif /* __WCHAR_ENABLED */
#endif /* __CTYPE_HAS_8_BIT_LOCALES */
				}
#ifdef __UCLIBC_MJN3_ONLY__
#warning might want to just put this in the locale_mmap object
#endif
				d = __global_locale.outdigit_length;
				x = &__global_locale.outdigit0_mb;
				for (c = 0 ; c < 10 ; c++) {
					((unsigned char *)d)[c] = strlen(x[c]);
					assert(d[c] > 0);
				}
			} else if (i == LC_COLLATE) {
				init_cur_collate(__locale_mmap->locales[ WIDTH_LOCALES * row + 3 + i ]);
			}
		}
		++i;
		p += 2;
		s += 2;
	} while (i < LC_ALL);
}

#endif /* __LOCALE_C_ONLY */

#endif
/**********************************************************************/
#ifdef L_nl_langinfo

#include <langinfo.h>
#include <nl_types.h>

#ifdef __LOCALE_C_ONLY

/* We need to index 320 bytes of data, so you might initially think we
 * need to store the offsets in shorts.  But since the offset of the
 * 64th item is 182, we'll store "offset - 2*64" for all items >= 64
 * and always calculate the data offset as "offset[i] + 2*(i & 64)".
 * This allows us to pack the data offsets in an unsigned char while
 * also avoiding an "if".
 *
 * Note: Category order is assumed to be:
 *   ctype, numeric, monetary, time, collate, messages, all
 */

#define C_LC_ALL 6

/* Combine the data to avoid size penalty for seperate char arrays when
 * compiler aligns objects.  The original code is left in as documentation. */
#define cat_start nl_data
#define C_locale_data (nl_data + C_LC_ALL + 1 + 90)

static const unsigned char nl_data[C_LC_ALL + 1 + 90 + 320] = {
/* static const char cat_start[LC_ALL + 1] = { */
        '\x00', '\x0b', '\x0e', '\x24', '\x56', '\x56', '\x5a', 
/* }; */
/* static const char item_offset[90] = { */
	'\x00', '\x02', '\x04', '\x06', '\x08', '\x0a', '\x0c', '\x0e', 
	'\x10', '\x12', '\x14', '\x1a', '\x1b', '\x1b', '\x1b', '\x1b', 
	'\x1b', '\x1b', '\x1b', '\x1b', '\x1b', '\x1c', '\x1c', '\x1c', 
	'\x1c', '\x1c', '\x1c', '\x1c', '\x1c', '\x1c', '\x1c', '\x1c', 
	'\x1c', '\x1c', '\x1c', '\x1e', '\x20', '\x24', '\x28', '\x2c', 
	'\x30', '\x34', '\x38', '\x3c', '\x43', '\x4a', '\x52', '\x5c', 
	'\x65', '\x6c', '\x75', '\x79', '\x7d', '\x81', '\x85', '\x89', 
	'\x8d', '\x91', '\x95', '\x99', '\x9d', '\xa1', '\xa5', '\xad', 
	'\x36', '\x3c', '\x42', '\x46', '\x4b', '\x50', '\x57', '\x61', 
	'\x69', '\x72', '\x7b', '\x7e', '\x81', '\x96', '\x9f', '\xa8', 
	'\xb3', '\xb3', '\xb3', '\xb3', '\xb3', '\xb3', '\xb4', '\xba', 
	'\xbf', '\xbf', 
/* }; */
/* static const char C_locale_data[320] = { */
	   '0', '\x00',    '1', '\x00',    '2', '\x00',    '3', '\x00', 
	   '4', '\x00',    '5', '\x00',    '6', '\x00',    '7', '\x00', 
	   '8', '\x00',    '9', '\x00',    'A',    'S',    'C',    'I', 
	   'I', '\x00',    '.', '\x00', '\x7f', '\x00',    '-', '\x00', 
	   'S',    'u',    'n', '\x00',    'M',    'o',    'n', '\x00', 
	   'T',    'u',    'e', '\x00',    'W',    'e',    'd', '\x00', 
	   'T',    'h',    'u', '\x00',    'F',    'r',    'i', '\x00', 
	   'S',    'a',    't', '\x00',    'S',    'u',    'n',    'd', 
	   'a',    'y', '\x00',    'M',    'o',    'n',    'd',    'a', 
	   'y', '\x00',    'T',    'u',    'e',    's',    'd',    'a', 
	   'y', '\x00',    'W',    'e',    'd',    'n',    'e',    's', 
	   'd',    'a',    'y', '\x00',    'T',    'h',    'u',    'r', 
	   's',    'd',    'a',    'y', '\x00',    'F',    'r',    'i', 
	   'd',    'a',    'y', '\x00',    'S',    'a',    't',    'u', 
	   'r',    'd',    'a',    'y', '\x00',    'J',    'a',    'n', 
	'\x00',    'F',    'e',    'b', '\x00',    'M',    'a',    'r', 
	'\x00',    'A',    'p',    'r', '\x00',    'M',    'a',    'y', 
	'\x00',    'J',    'u',    'n', '\x00',    'J',    'u',    'l', 
	'\x00',    'A',    'u',    'g', '\x00',    'S',    'e',    'p', 
	'\x00',    'O',    'c',    't', '\x00',    'N',    'o',    'v', 
	'\x00',    'D',    'e',    'c', '\x00',    'J',    'a',    'n', 
	   'u',    'a',    'r',    'y', '\x00',    'F',    'e',    'b', 
	   'r',    'u',    'a',    'r',    'y', '\x00',    'M',    'a', 
	   'r',    'c',    'h', '\x00',    'A',    'p',    'r',    'i', 
	   'l', '\x00',    'M',    'a',    'y', '\x00',    'J',    'u', 
	   'n',    'e', '\x00',    'J',    'u',    'l',    'y', '\x00', 
	   'A',    'u',    'g',    'u',    's',    't', '\x00',    'S', 
	   'e',    'p',    't',    'e',    'm',    'b',    'e',    'r', 
	'\x00',    'O',    'c',    't',    'o',    'b',    'e',    'r', 
	'\x00',    'N',    'o',    'v',    'e',    'm',    'b',    'e', 
	   'r', '\x00',    'D',    'e',    'c',    'e',    'm',    'b', 
	   'e',    'r', '\x00',    'A',    'M', '\x00',    'P',    'M', 
	'\x00',    '%',    'a',    ' ',    '%',    'b',    ' ',    '%', 
	   'e',    ' ',    '%',    'H',    ':',    '%',    'M',    ':', 
	   '%',    'S',    ' ',    '%',    'Y', '\x00',    '%',    'm', 
	   '/',    '%',    'd',    '/',    '%',    'y', '\x00',    '%', 
	   'H',    ':',    '%',    'M',    ':',    '%',    'S', '\x00', 
	   '%',    'I',    ':',    '%',    'M',    ':',    '%',    'S', 
	   ' ',    '%',    'p', '\x00',    '^',    '[',    'y',    'Y', 
	   ']', '\x00',    '^',    '[',    'n',    'N',    ']', '\x00', 
};

char *nl_langinfo(nl_item item)
{
	unsigned int c;
	unsigned int i;

	if ((c = _NL_ITEM_CATEGORY(item)) < C_LC_ALL) {
		if ((i = cat_start[c] + _NL_ITEM_INDEX(item)) < cat_start[c+1]) {
/*  			return (char *) C_locale_data + item_offset[i] + (i & 64); */
			return (char *) C_locale_data + nl_data[C_LC_ALL+1+i] + 2*(i & 64);
		}
	}
	return (char *) cat_start;	/* Conveniently, this is the empty string. */
}

#else  /* __LOCALE_C_ONLY */

static const char empty[] = "";

char *nl_langinfo(nl_item item)
{
	unsigned int c = _NL_ITEM_CATEGORY(item);
	unsigned int i = _NL_ITEM_INDEX(item);

	if ((c < LC_ALL) && (i < __global_locale.category_item_count[c])) {
		return ((char **)(((char *) &__global_locale)
						  + __global_locale.category_offsets[c]))[i];

	}
	return (char *) empty;
}

#endif /* __LOCALE_C_ONLY */

#endif
/**********************************************************************/
