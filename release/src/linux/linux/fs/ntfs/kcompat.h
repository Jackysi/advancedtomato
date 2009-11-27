/*
 * kcompat.h - Various defines needed to easier sync with the 2.5.x version.
 *	       Part of the Linux-NTFS project. Ported from the misc part of
 *	       the 2.5.x kernel.
 *
 * Copyright (c) 2002 Pawel Kot.
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS 
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LINUX_NTFS_KCOMPAT_H
#define _LINUX_NTFS_KCOMPAT_H

#include <linux/version.h>
#include <linux/compiler.h>

#ifndef MAX_BUF_PER_PAGE
#  define MAX_BUF_PER_PAGE (PAGE_CACHE_SIZE / 512)
#endif

#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)

/* Page cache limit. The filesystems should put that into their s_maxbytes
   limits, otherwise bad things can happen in VM. */
#if BITS_PER_LONG==32
#  define MAX_LFS_FILESIZE        (((u64)PAGE_CACHE_SIZE << (BITS_PER_LONG-1))-1)
#elif BITS_PER_LONG==64
#  define MAX_LFS_FILESIZE        0x7fffffffffffffff
#endif

#define PageUptodate(page)	Page_Uptodate(page)
#define page_buffers(page)	(page)->buffers
#define page_has_buffers(page)	((page)->buffers != NULL)

#include <linux/fs.h>

/*
 * macro tricks to expand the set_buffer_foo(), clear_buffer_foo()
 * functions.
 */
#define BUFFER_FNS(bit, name)						\
static inline void set_buffer_##name(struct buffer_head *bh)		\
{									\
	set_bit(BH_##bit, &(bh)->b_state);				\
}									\
static inline void clear_buffer_##name(struct buffer_head *bh)		\
{									\
	clear_bit(BH_##bit, &(bh)->b_state);				\
}

/*
 * test_set_buffer_foo(), clear_set_buffer_foo()
 */
#define TAS_BUFFER_FNS(bit, name)					\
static inline int test_set_buffer_##name(struct buffer_head *bh)	\
{									\
	return test_and_set_bit(BH_##bit, &(bh)->b_state);		\
}									\
static inline int test_clear_buffer_##name(struct buffer_head *bh)	\
{									\
	return test_and_clear_bit(BH_##bit, &(bh)->b_state);		\
}

BUFFER_FNS(Uptodate, uptodate)
BUFFER_FNS(Dirty, dirty)
TAS_BUFFER_FNS(Dirty, dirty)
BUFFER_FNS(Lock, locked)
TAS_BUFFER_FNS(Lock, locked)
BUFFER_FNS(Mapped, mapped)
BUFFER_FNS(New, new)
BUFFER_FNS(Async, async)

#endif /* _LINUX_NTFS_KCOMPAT_H */
