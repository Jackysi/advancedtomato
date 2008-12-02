/*
 * Copyright (C) 2000 Manuel Novoa III
 *
 * Notes:
 *
 * The primary objective of this implementation was minimal size while
 * providing robustness and resonable accuracy.
 *
 * This implementation depends on IEEE floating point behavior and expects
 * to be able to generate +/- infinity as a result.
 *
 * There are a number of compile-time options below.
 *
 */

/*****************************************************************************/
/*                            OPTIONS                                        */
/*****************************************************************************/

/* Set if we want to scale with a O(log2(exp)) multiplications. */
#define _STRTOD_LOG_SCALING      1

/* Set if we want strtod to set errno appropriately. */
/* NOTE: Implies all options below and pulls in _zero_or_inf_check. */
#define _STRTOD_ERRNO            0

/* Set if we want support for the endptr arg. */
/* Implied by _STRTOD_ERRNO. */
#define _STRTOD_ENDPTR           1

/* Set if we want to prevent overflow in accumulating the exponent. */
#define _STRTOD_RESTRICT_EXP     1

/* Set if we want to process mantissa digits more intelligently. */
/* Implied by _STRTOD_ERRNO. */
#define _STRTOD_RESTRICT_DIGITS  1

/* Set if we want to skip scaling 0 for the exponent. */
/* Implied by _STRTOD_ERRNO. */
#define _STRTOD_ZERO_CHECK       0

/*****************************************************************************/
/* Don't change anything that follows.                                       */
/*****************************************************************************/

#if _STRTOD_ERRNO
#undef _STRTOD_ENDPTR
#undef _STRTOD_RESTRICT_EXP
#undef _STRTOD_RESTRICT_DIGITS
#undef _STRTOD_ZERO_CHECK
#define _STRTOD_ENDPTR           1
#define _STRTOD_RESTRICT_EXP     1
#define _STRTOD_RESTRICT_DIGITS  1
#define _STRTOD_ZERO_CHECK       1
#endif

/*****************************************************************************/

#include <stdlib.h>

#include <float.h>

#if _STRTOD_RESTRICT_DIGITS
#define MAX_SIG_DIGITS 20
#define EXP_DENORM_ADJUST MAX_SIG_DIGITS
#define MAX_ALLOWED_EXP (MAX_SIG_DIGITS  + EXP_DENORM_ADJUST - DBL_MIN_10_EXP)

#if DBL_DIG > MAX_SIG_DIGITS
#error need to adjust MAX_SIG_DIGITS
#endif

#include <limits.h>
#if MAX_ALLOWED_EXP > INT_MAX
#error size assumption violated for MAX_ALLOWED_EXP
#endif
#else
/* We want some excess if we're not restricting mantissa digits. */
#define MAX_ALLOWED_EXP ((20 - DBL_MIN_10_EXP) * 2)
#endif

#include <ctype.h>
/* Note: For i386 the macro resulted in smaller code than the function call. */
#if 1
#undef isdigit
#define isdigit(x) ( (x >= '0') && (x <= '9') )
#endif

#if _STRTOD_ERRNO
#include <errno.h>
extern int _zero_or_inf_check(double x);
#endif

double strtod(const char *str, char **endptr)
{
    double number;
#if _STRTOD_LOG_SCALING
    double p10;
#endif
    char *pos0;
#if _STRTOD_ENDPTR
    char *pos1;
#endif
    char *pos = (char *) str;
    int exponent_power;
    int exponent_temp;
    int negative;
#if _STRTOD_RESTRICT_DIGITS || _STRTOD_ENDPTR
    int num_digits;
#endif

    while (isspace(*pos)) {	/* skip leading whitespace */
	++pos;
    }

    negative = 0;
    switch(*pos) {		/* handle optional sign */
    case '-': negative = 1;	/* fall through to increment position */
    case '+': ++pos;
    }

    number = 0.;
#if _STRTOD_RESTRICT_DIGITS || _STRTOD_ENDPTR
    num_digits = -1;
#endif
    exponent_power = 0;
    pos0 = NULL;

 LOOP:
    while (isdigit(*pos)) {	/* process string of digits */
#if _STRTOD_RESTRICT_DIGITS
	if (num_digits < 0) {	/* first time through? */
	    ++num_digits;	/* we've now seen a digit */
	}
	if (num_digits || (*pos != '0')) { /* had/have nonzero */
	    ++num_digits;
	    if (num_digits <= MAX_SIG_DIGITS) { /* is digit significant */
		number = number * 10. + (*pos - '0');
	    }
	}
#else
#if _STRTOD_ENDPTR
	++num_digits;
#endif
	number = number * 10. + (*pos - '0');
#endif
	++pos;
    }

    if ((*pos == '.') && !pos0) { /* is this the first decimal point? */
	pos0 = ++pos;		/* save position of decimal point */
	goto LOOP;		/* and process rest of digits */
    }

#if _STRTOD_ENDPTR
    if (num_digits<0) {		/* must have at least one digit */
	pos = (char *) str;
	goto DONE;
    }
#endif

#if _STRTOD_RESTRICT_DIGITS
    if (num_digits > MAX_SIG_DIGITS) { /* adjust exponent for skipped digits */
	exponent_power += num_digits - MAX_SIG_DIGITS;
    }
#endif

    if (pos0) {
	exponent_power += pos0 - pos; /* adjust exponent for decimal point */
    }

    if (negative) {		/* correct for sign */
	number = -number;
	negative = 0;		/* reset for exponent processing below */
    }

    /* process an exponent string */
    if (*pos == 'e' || *pos == 'E') {
#if _STRTOD_ENDPTR
	pos1 = pos;
#endif
	switch(*++pos) {	/* handle optional sign */
	case '-': negative = 1;	/* fall through to increment pos */
	case '+': ++pos;
	}

	pos0 = pos;
	exponent_temp = 0;
	while (isdigit(*pos)) {	/* process string of digits */
#if _STRTOD_RESTRICT_EXP
	    if (exponent_temp < MAX_ALLOWED_EXP) { /* overflow check */
		exponent_temp = exponent_temp * 10 + (*pos - '0');
	    }
#else
	    exponent_temp = exponent_temp * 10 + (*pos - '0');
#endif
	    ++pos;
	}

#if _STRTOD_ENDPTR
	if (pos == pos0) {	/* were there no digits? */
	    pos = pos1;		/* back up to e|E */
	} /* else */
#endif
	if (negative) {
	    exponent_power -= exponent_temp;
	} else {
	    exponent_power += exponent_temp;
	}
    }

#if _STRTOD_ZERO_CHECK
    if (number == 0.) {
	goto DONE;
    }
#endif

    /* scale the result */
#if _STRTOD_LOG_SCALING
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
#else
    while (exponent_power) {
	if (exponent_power < 0) {
	    number /= 10.;
	    exponent_power++;
	} else {
	    number *= 10.;
	    exponent_power--;
	}
    }
#endif

#if _STRTOD_ERRNO
    if (_zero_or_inf_check(number)) {
	__set_errno(ERANGE);
    }
#endif

 DONE:
#if _STRTOD_ENDPTR
    if (endptr) {
	*endptr = pos;
    }
#endif

    return number;
}
