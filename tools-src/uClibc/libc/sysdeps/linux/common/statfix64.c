/* vi: set sw=4 ts=4: */
/* Convert from the kernel's version of struct stat to libc's version
 *
 * Copyright (C) 2000 by Lineo, inc. and Erik Andersen
 * Copyright (C) 2000,2001 by Erik Andersen <andersen@uclibc.org>
 * Written by Erik Andersen <andersen@uclibc.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


/* Pull in whatever this particular arch's kernel thinks the kernel version of
 * struct stat should look like.  It turns out that each arch has a different
 * opinion on the subject.  Then pull in libc's version of struct stat... */
#include "statfix64.h"

#ifdef __UCLIBC_HAVE_LFS__

/* Convert from the kernel's version of struct stat to libc's version  */
void statfix64(struct libc_stat64 *libcstat, struct kernel_stat64 *kstat)
{
	libcstat->st_dev = kstat->st_dev;
	libcstat->st_ino = kstat->st_ino;
	libcstat->st_mode = kstat->st_mode;
	libcstat->st_nlink = kstat->st_nlink;
	libcstat->st_uid = kstat->st_uid;
	libcstat->st_gid = kstat->st_gid;
	libcstat->st_rdev = kstat->st_rdev;
	libcstat->st_size = kstat->st_size;
	libcstat->st_blksize = kstat->st_blksize;
	libcstat->st_blocks = kstat->st_blocks;
	libcstat->st_atime = kstat->st_atime;
	libcstat->st_mtime = kstat->st_mtime;
	libcstat->st_ctime = kstat->st_ctime;
}

#endif /* __UCLIBC_HAVE_LFS__ */

