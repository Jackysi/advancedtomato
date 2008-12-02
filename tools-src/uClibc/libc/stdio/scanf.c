
/*
 * Modified by Manuel Novoa III       Mar 13, 2001
 *
 * The vfscanf routine was completely rewritten to add features and remove
 * bugs.  The function __strtold, based on my strtod code in stdlib, was
 * added to provide floating point support for the scanf functions.
 *
 * So far they pass the test cases from glibc-2.1.3, except in two instances.
 * In one case, the test appears to be broken.  The other case is something
 * I need to research further.  This version of scanf assumes it can only
 * peek one character ahead.  Apparently, glibc looks further.  The difference
 * can be seen when parsing a floating point value in the character
 * sequence "100ergs".  glibc is able to back up before the 'e' and return
 * a value of 100, whereas this scanf reports a bad match with the stream
 * pointer at 'r'.  A similar situation can also happen when parsing hex
 * values prefixed by 0x or 0X; a failure would occur for "0xg".  In order to
 * fix this, I need to rework the "ungetc" machinery in stdio.c again.
 * I do have one reference though, that seems to imply scanf has a single
 * character of lookahead.
 *
 * May 20, 2001
 *
 * Quote from ANSI/ISO C99 standard:
 *
 *    fscanf pushes back at most one input character onto the input stream.
 *    Therefore, some sequences that are acceptable to strtod, strtol, etc.,
 *    are unacceptable to fscanf.
 *
 * So uClibc's *scanf functions conform to the standard, and glibc's
 * implementation doesn't for the "100ergs" case mentioned above.
 *
 * Sep 6, 2002
 * Patch from Tero_Lyytikäinen <tero@paravant.fi> to fix bug in matchchar case.
 */

#define _ISOC99_SOURCE			/* for LLONG_MAX primarily... */
#define _GNU_SOURCE
#define _STDIO_UTILITY
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#ifdef __STDIO_THREADSAFE
#include <stdio_ext.h>
#include <pthread.h>
#endif /* __STDIO_THREADSAFE */

#ifdef L_scanf
#ifdef __STDC__
int scanf(const char *fmt, ...)
#else
int scanf(fmt, va_alist)
__const char *fmt;
va_dcl
#endif
{
	va_list ptr;
	int rv;

	va_start(ptr, fmt);
	rv = vfscanf(stdin, fmt, ptr);
	va_end(ptr);
	return rv;
}
#endif

#ifdef L_sscanf
#if !defined(__STDIO_BUFFERS) && !defined(__STDIO_GLIBC_CUSTOM_STREAMS)
#warning skipping sscanf since no buffering and no custom streams!
#else

int sscanf(const char *sp, const char *fmt, ...)
{
	va_list ptr;
	int rv;

	va_start(ptr, fmt);
	rv = vsscanf(sp, fmt, ptr);
	va_end(ptr);
	return rv;
}

#endif
#endif

#ifdef L_fscanf
#ifdef __STDC__
int fscanf(FILE * fp, const char *fmt, ...)
#else
int fscanf(fp, fmt, va_alist)
FILE *fp;
__const char *fmt;
va_dcl
#endif
{
	va_list ptr;
	int rv;

	va_start(ptr, fmt);
	rv = vfscanf(fp, fmt, ptr);
	va_end(ptr);
	return rv;
}
#endif

#ifdef L_vscanf
int vscanf(fmt, ap)
__const char *fmt;
va_list ap;
{
	return vfscanf(stdin, fmt, ap);
}
#endif

#ifdef L_vsscanf
#ifdef __STDIO_BUFFERS
int vsscanf(__const char *sp, __const char *fmt, va_list ap)
{
	FILE string[1];

	string->filedes = -2;		/* for debugging */
	string->modeflags = (__FLAG_NARROW|__FLAG_READONLY);
	string->bufstart = string->bufpos = (unsigned char *) ((void *) sp);
	string->bufgetc = (char *) ((unsigned) -1);

#ifdef __STDIO_MBSTATE
	__INIT_MBSTATE(&(string->state));
#endif /* __STDIO_MBSTATE */

#ifdef __STDIO_THREADSAFE
	string->user_locking = 0;
	__stdio_init_mutex(&string->lock);
#endif

	return vfscanf(string, fmt, ap);
}
#else  /* __STDIO_BUFFERS */
#ifdef __STDIO_GLIBC_CUSTOM_STREAMS
int vsscanf(__const char *sp, __const char *fmt, va_list ap)
{
	FILE *f;
	int rv;

	if ((f = fmemopen((char *)sp, strlen(sp), "r")) == NULL) {
		return -1;
	}
	rv = vfscanf(f, fmt, ap);
	fclose(f);

	return rv;
}
#else  /* __STDIO_GLIBC_CUSTOM_STREAMS */
#warning skipping vsscanf since no buffering and no custom streams!
#endif /* __STDIO_GLIBC_CUSTOM_STREAMS */
#endif /* __STDIO_BUFFERS */
#endif

#ifdef L_vfscanf

#include <assert.h>
#include <ctype.h>
#include <limits.h>

static int valid_digit(char c, char base)
{
	if (base == 16) {
		return isxdigit(c);
	} else {
		return (__isdigit(c) && (c < '0' + base));
	}
}

extern unsigned long
_stdlib_strto_l(register const char * __restrict str,
				char ** __restrict endptr, int base, int sflag);
#ifdef LLONG_MAX
extern unsigned long long
_stdlib_strto_ll(register const char * __restrict str,
				 char ** __restrict endptr, int base, int sflag);
#endif

struct scan_cookie {
	FILE *fp;
	int nread;
	int width;
	int width_flag;
	int ungot_char;
	int ungot_flag;
	int app_ungot;
};

static const char qual[] = "hl" /* "jtz" */ "Lq";
/* char = -2, short = -1, int = 0, long = 1, long long = 2 */
static const char qsz[] = { -1, 1,           2, 2 };

#ifdef __UCLIBC_HAS_FLOATS__
static int __strtold(long double *ld, struct scan_cookie *sc);
						   /*01234567890123456 */
static const char spec[]  = "%n[csoupxXidfeEgG";
#else
static const char spec[]  = "%n[csoupxXid";
#endif
/* radix[i] <-> spec[i+5]     o   u   p   x   X  i   d */
static const char radix[] = { 8, 10, 16, 16, 16, 0, 10 };

static void init_scan_cookie(register struct scan_cookie *sc,
							 register FILE *fp)
{
	sc->fp = fp;
	sc->nread = 0;
	sc->width_flag = 0;
	sc->ungot_flag = 0;
	sc->app_ungot = ((fp->modeflags & __MASK_UNGOT) ? fp->ungot[1] : 0);
}

/* TODO -- what about literal '\0' chars in a file??? */

static int scan_getc_nw(register struct scan_cookie *sc)
{
	if (sc->ungot_flag == 0) {
		sc->ungot_char = getc(sc->fp);
	} else {
		sc->ungot_flag = 0;
	}
	if (sc->ungot_char > 0) {
		++sc->nread;
	}
	sc->width_flag = 0;
	return sc->ungot_char;
}

static int scan_getc(register struct scan_cookie *sc)
{
	if (sc->ungot_flag == 0) {
		sc->ungot_char = getc(sc->fp);
	}
	sc->width_flag = 1;
	if (--sc->width < 0) {
		sc->ungot_flag = 1;
		return 0;
	}
	sc->ungot_flag = 0;
	if (sc->ungot_char > 0) {
		++sc->nread;
	}
	return sc->ungot_char;
}

static void scan_ungetc(register struct scan_cookie *sc)
{
	if (sc->ungot_flag != 0) {
		assert(sc->width < 0);
		return;
	}
	if (sc->width_flag) {
		++sc->width;
	}
	sc->ungot_flag = 1;
	if (sc->ungot_char > 0) {	/* not EOF or EOS */
		--sc->nread;
	}
}

static void kill_scan_cookie(register struct scan_cookie *sc)
{
	if (sc->ungot_flag) {
		ungetc(sc->ungot_char,sc->fp);
		/* Deal with distiction between user and scanf ungots. */
		if (sc->nread == 0) {	/* Only one char was read... app ungot? */
			sc->fp->ungot[1] = sc->app_ungot; /* restore ungot state. */
		}
	}
}

int vfscanf(FILE *fp, const char *format, va_list ap)
{
#define STRTO_L_(s,e,b,sf) _stdlib_strto_ll(s,e,b,sf)
#define MAX_DIGITS 64
#define UV_TYPE unsigned long long
#define V_TYPE long long
#ifdef __UCLIBC_HAS_FLOATS__
	long double ld;
#endif
	UV_TYPE uv;
	struct scan_cookie sc;
	register unsigned const char *fmt;
	const char *p;
	register unsigned char *b;
	void *vp;
	int cc, i, cnt;
	signed char lval;
	unsigned char store, usflag, base, invert, r0, r1;
	unsigned char buf[MAX_DIGITS+2];
	unsigned char scanset[UCHAR_MAX + 1];

	__STDIO_THREADLOCK(fp);

	init_scan_cookie(&sc,fp);

	fmt = (unsigned const char *) format;
	cnt = 0;

	while (*fmt) {
		store = 1;
		lval = 0;
		sc.width = INT_MAX;
		if (*fmt == '%') {		/* Conversion specification. */
			++fmt;
			if (*fmt == '*') {	/* Suppress assignment. */
				store = 0;
				++fmt;
			}
			for (i = 0 ; __isdigit(*fmt) ; sc.width = i) {
				i = (i * 10) + (*fmt++ - '0'); /* Get specified width. */
			}
			for (i = 0 ; i < sizeof(qual) ; i++) { /* Optional qualifier. */
				if (qual[i] == *fmt) {
					++fmt;
					lval += qsz[i];
					if ((i < 2) && (qual[i] == *fmt)) {	/* Double h or l. */
						++fmt;
						lval += qsz[i];
					}
					break;
				}
			}
			for (p = spec ; *p ; p++) {	/* Process format specifier. */
				if (*fmt != *p) continue;
				if (p-spec < 1) { /* % - match a '%'*/
					goto matchchar;
				}
				if (p-spec < 2) { /* n - store number of chars read */
					*(va_arg(ap, int *)) = sc.nread;
					scan_getc_nw(&sc);
					goto nextfmt;
				}
				if (p-spec > 3) { /* skip white space if not c or [ */
					do {
						i = scan_getc_nw(&sc);
					} while (__isspace(i));
					scan_ungetc(&sc);
				}
				if (p-spec < 5) { /* [,c,s - string conversions */
					invert = 0;
					if (*p == 'c') {
						invert = 1;
						if (sc.width == INT_MAX) {
							sc.width = 1;
						}
					}
					for (i=0 ; i<= UCHAR_MAX ; i++) {
						scanset[i] = ((*p == 's') ? (__isspace(i) == 0) : 0);
					}
					if (*p == '[') { /* need to build a scanset */
						if (*++fmt == '^') {
							invert = 1;
							++fmt;
						}
						if (*fmt == ']') {
							scanset[(int)']'] = 1;
							++fmt;
						}
						r0 = 0;
						while (*fmt && *fmt !=']') { /* build scanset */
							if ((*fmt == '-') && r0 && (fmt[1] != ']')) {
								/* range */
								++fmt;
								if (*fmt < r0) {
									r1 = r0;
									r0 = *fmt;
								} else {
									r1 = *fmt;
								}
								for (i=r0 ; i<= r1 ; i++) {
									scanset[i] = 1;
								}
								r0 = 0;
							} else {
								r0 = *fmt;
								scanset[r0] = 1;
							}
							++fmt;
						}
						if (!*fmt) { /* format string exhausted! */
							goto done;
						}
					}
					/* ok -- back to common work */
					if (sc.width <= 0) {
						goto done;
					}
					if (store) {
						b = va_arg(ap, unsigned char *);
					} else {
						b = buf;
					}
					cc = scan_getc(&sc);
					if (cc <= 0) {
						scan_ungetc(&sc);
						goto done; /* return EOF if cnt == 0 */
					}
					i = 0;
					while ((cc>0) && (scanset[cc] != invert)) {
						i = 1; /* yes, we stored something */
						*b = cc;
						b += store;
						cc = scan_getc(&sc);
					}
					if (i==0) {
						scan_ungetc(&sc);
						goto done; /* return cnt */
					}
					if (*p != 'c') { /* nul-terminate the stored string */
						*b = 0;
					}
					cnt += store;
					goto nextfmt;
				}
				if (p-spec < 12) { /* o,u,p,x,X,i,d - (un)signed integer */
					if (*p == 'p') {
						/* assume pointer same size as int or long. */
						lval = (sizeof(char *) == sizeof(long));
					}
					usflag = ((p-spec) < 10); /* (1)0 if (un)signed */
					base = radix[(int)(p-spec) - 5];
					b = buf;
					if (sc.width <= 0) {
						goto done;
					}
					cc = scan_getc(&sc);
					if ((cc == '+') || (cc == '-')) { /* Handle leading sign.*/
						*b++ = cc;
						cc = scan_getc(&sc);
					}
					if (cc == '0') { /* Possibly set base and handle prefix. */
						if ((base == 0) || (base == 16)) {
							cc = scan_getc(&sc);
							if ((cc == 'x') || (cc == 'X')) {
								/* We're committed to base 16 now. */
								base = 16;
								cc = scan_getc(&sc);
							} else { /* oops... back up */
								scan_ungetc(&sc);
								cc = '0';
								if (base == 0) {
									base = 8;
								}
							}
						}
					}
					if (base == 0) { /* Default to base 10 */
						base = 10;
					}
					/* At this point, we're ready to start reading digits. */
					if (cc == '0') {
						*b++ = cc; /* Store first leading 0 */
						do {	/*     but ignore others. */
							cc = scan_getc(&sc);
						} while (cc == '0');
					}
					while (valid_digit(cc,base)) { /* Now for nonzero digits.*/
						if (b - buf < MAX_DIGITS) {
							*b++ = cc;
						}
						cc = scan_getc(&sc);
					}
					*b = 0;	/* null-terminate */
					if ((b == buf) || (*--b == '+') || (*b == '-')) {
						scan_ungetc(&sc);
						goto done; /* No digits! */
					}
					if (store) {
						if (*buf == '-') {
							usflag = 0;
						}
						uv = STRTO_L_(buf, NULL, base, 1-usflag);
						vp = va_arg(ap, void *);
						switch (lval) {
							case 2:	/* If no long long, treat as long . */
								*((unsigned long long *)vp) = uv;
								break;
							case 1:
#if ULONG_MAX == UINT_MAX
							case 0:	/* int and long int are the same */
#endif
								if (usflag) {
									if (uv > ULONG_MAX) {
										uv = ULONG_MAX;
									}
								} else if (((V_TYPE)uv) > LONG_MAX) {
									uv = LONG_MAX;
								} else if (((V_TYPE)uv) < LONG_MIN) {
									uv = (UV_TYPE) LONG_MIN;
								}
								*((unsigned long *)vp) = (unsigned long)uv;
								break;
#if ULONG_MAX != UINT_MAX
							case 0:	/* int and long int are different */
								if (usflag) {
									if (uv > UINT_MAX) {
										uv = UINT_MAX;
									}
								} else if (((V_TYPE)uv) > INT_MAX) {
									uv = INT_MAX;
								} else if (((V_TYPE)uv) < INT_MIN) {
									uv = (UV_TYPE) INT_MIN;
								}
								*((unsigned int *)vp) = (unsigned int)uv;
								break;
#endif
							case (signed char)(-1):
								if (usflag) {
									if (uv > USHRT_MAX) {
										uv = USHRT_MAX;
									}
								} else if (((V_TYPE)uv) > SHRT_MAX) {
									uv = SHRT_MAX;
								} else if (((V_TYPE)uv) < SHRT_MIN) {
									uv = (UV_TYPE) SHRT_MIN;
								}
								*((unsigned short *)vp) = (unsigned short)uv;
								break;
							case (signed char)(-2):
								if (usflag) {
									if (uv > UCHAR_MAX) {
										uv = UCHAR_MAX;
									}
								} else if (((V_TYPE)uv) > CHAR_MAX) {
									uv = CHAR_MAX;
								} else if (((V_TYPE)uv) < CHAR_MIN) {
									uv = (UV_TYPE) CHAR_MIN;
								}
								*((unsigned char *)vp) = (unsigned char) uv;
								break;
							default:
								assert(0);
						}
						++cnt;
					}
					goto nextfmt;
				}
#ifdef __UCLIBC_HAS_FLOATS__
				else {			/* floating point */
					if (sc.width <= 0) {
						goto done;
					}
					if (__strtold(&ld, &sc)) { /* Success! */
						if (store) {
							vp = va_arg(ap, void *);
							switch (lval) {
								case 2:
									*((long double *)vp) = ld;
									break;
								case 1:
									*((double *)vp) = (double) ld;
									break;
								case 0:
									*((float *)vp) = (float) ld;
									break;
								default: /* Illegal qualifier! */
									assert(0);
									goto done;
							}
							++cnt;
						}
						goto nextfmt;
					}
				}
#else
				assert(0);
#endif
				goto done;
			}
			/* Unrecognized specifier! */
			goto RETURN_cnt;
		} if (__isspace(*fmt)) {	/* Consume all whitespace. */
			do {
				i = scan_getc_nw(&sc);
			} while (__isspace(i));
		} else {				/* Match the current fmt char. */
		matchchar:
			if (scan_getc_nw(&sc) != *fmt) {
				scan_ungetc(&sc);
				goto done;
			}
			scan_getc_nw(&sc);
		}
	nextfmt:
		scan_ungetc(&sc);
		++fmt;
	}

  done:						/* end of scan */
	kill_scan_cookie(&sc);

	if ((sc.ungot_char <= 0) && (cnt == 0) && (*fmt)) {
		cnt = EOF;
	}

 RETURN_cnt:
	__STDIO_THREADUNLOCK(fp);

	return (cnt);
}

/*****************************************************************************/
#ifdef __UCLIBC_HAS_FLOATS__

#include <float.h>

#define MAX_SIG_DIGITS 20
#define MAX_IGNORED_DIGITS 2000
#define MAX_ALLOWED_EXP (MAX_SIG_DIGITS + MAX_IGNORED_DIGITS + LDBL_MAX_10_EXP)

#if LDBL_DIG > MAX_SIG_DIGITS
#error need to adjust MAX_SIG_DIGITS
#endif

#include <limits.h>
#if MAX_ALLOWED_EXP > INT_MAX
#error size assumption violated for MAX_ALLOWED_EXP
#endif

int __strtold(long double *ld, struct scan_cookie *sc)
{
    long double number;
    long double p10;
    int exponent_power;
    int exponent_temp;
    int negative;
    int num_digits;
    int since_decimal;
	int c;

	c = scan_getc(sc);				/* Decrements width. */

    negative = 0;
    switch(c) {					/* Handle optional sign. */
		case '-': negative = 1;	/* Fall through to get next char. */
		case '+': c = scan_getc(sc);
    }

    number = 0.;
    num_digits = -1;
    exponent_power = 0;
    since_decimal = INT_MIN;

 LOOP:
    while (__isdigit(c)) {		/* Process string of digits. */
		++since_decimal;
		if (num_digits < 0) {	/* First time through? */
			++num_digits;		/* We've now seen a digit. */
		}
		if (num_digits || (c != '0')) { /* had/have nonzero */
			++num_digits;
			if (num_digits <= MAX_SIG_DIGITS) { /* Is digit significant? */
				number = number * 10. + (c - '0');
			}
		}
		c = scan_getc(sc);
    }

    if ((c == '.') && (since_decimal < 0)) { /* If no previous decimal pt, */
		since_decimal = 0;		/* save position of decimal point */
		c = scan_getc(sc);			/* and process rest of digits */
		goto LOOP;
    }

    if (num_digits<0) {			/* Must have at least one digit. */
		goto FAIL;
    }

    if (num_digits > MAX_SIG_DIGITS) { /* Adjust exp for skipped digits. */
		exponent_power += num_digits - MAX_SIG_DIGITS;
    }

    if (since_decimal >= 0) {		/* Adjust exponent for decimal point. */
		exponent_power -= since_decimal;
    }

    if (negative) {				/* Correct for sign. */
		number = -number;
		negative = 0;			/* Reset for exponent processing below. */
    }

    /* Process an exponent string. */
    if (c == 'e' || c == 'E') {
		c = scan_getc(sc);
		switch(c) {				/* Handle optional sign. */
			case '-': negative = 1;	/* Fall through to get next char. */
			case '+': c = scan_getc(sc);
		}

		num_digits = 0;
		exponent_temp = 0;
		while (__isdigit(c)) {	/* Process string of digits. */
			if (exponent_temp < MAX_ALLOWED_EXP) { /* overflow check */
				exponent_temp = exponent_temp * 10 + (c - '0');
			}
			c = scan_getc(sc);
			++num_digits;
		}

		if (num_digits == 0) {	/* Were there no exp digits? */
			goto FAIL;
		} /* else */
		if (negative) {
			exponent_power -= exponent_temp;
		} else {
			exponent_power += exponent_temp;
		}
    }

    if (number != 0.) {
		/* Now scale the result. */
		exponent_temp = exponent_power;
		p10 = 10.;

		if (exponent_temp < 0) {
			exponent_temp = -exponent_temp;
		}

		while (exponent_temp) {
			if (exponent_temp & 1) {
				if (exponent_power < 0) {
					number /= p10;
				} else {
					number *= p10;
				}
			}
			exponent_temp >>= 1;
			p10 *= p10;
		}
	}
	*ld = number;
	return 1;

 FAIL:
	scan_ungetc(sc);
	return 0;
}
#endif /* __UCLIBC_HAS_FLOATS__ */
#endif
