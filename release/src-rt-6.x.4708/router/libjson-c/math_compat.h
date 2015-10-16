#ifndef __math_compat_h
#define __math_compat_h

#undef isnan
#define isnan(x) __builtin_isnan(x)
#undef isinf
#define isinf(x) __builtin_isinf(x)

#endif
