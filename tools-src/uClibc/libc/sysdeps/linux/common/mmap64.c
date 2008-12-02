/* Copyright (C) 1997, 1998, 1999, 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Daniel Jacobowitz <dan@debian.org>, 1999.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <errno.h>
#include <unistd.h>
#include <sysdep.h>
#include <sys/mman.h>

#if defined __UCLIBC_HAS_LFS__

#if defined _FILE_OFFSET_BITS && _FILE_OFFSET_BITS != 64 
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS   64
#endif
#ifndef __USE_LARGEFILE64
# define __USE_LARGEFILE64      1
#endif
/* We absolutely do _NOT_ want interfaces silently
 *  * renamed under us or very bad things will happen... */
#ifdef __USE_FILE_OFFSET64
# undef __USE_FILE_OFFSET64
#endif

#undef	creat
__ptr_t mmap64(__ptr_t addr, size_t len, int prot, int flags, int fd, __off64_t offset)
{
    if (offset != (off_t) offset || (offset + len) != (off_t) (offset + len))
    {
	__set_errno (EINVAL);
	return MAP_FAILED;
    }

    return mmap (addr, len, prot, flags, fd, (off_t) offset);
}
#endif /* __UCLIBC_HAS_LFS__ */

