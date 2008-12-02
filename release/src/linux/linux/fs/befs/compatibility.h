/*
 * linux/fs/befs/compatiblity.h
 *
 * Copyright (C) 2001 Will Dyson <will_dyson@pobox.com>
 *   AKA <will@cs.earlham.edu>
 *
 * This file trys to take care of differences between
 * kernel versions
 */

#include <linux/version.h>

/* New interfaces in 2.4.10 */
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,10)

#define min_t(type,x,y) \
({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })

#define max_t(type,x,y) \
({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })

#define vsnprintf(buf, n, fmt, args) vsprintf(buf, fmt, args)

#define MODULE_LICENSE(x)

#endif				/* LINUX_VERSION_CODE */
