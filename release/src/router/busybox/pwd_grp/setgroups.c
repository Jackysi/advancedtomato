/* vi: set sw=4 ts=4: */
/*
 * Taken from the set of syscalls for uClibc
 *
 * Copyright (C) 1999,2000,2001 by Lineo, inc.  Written by Erik Andersen
 * <andersen@lineo.com>, <andersee@debian.org>
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
 *
 */

#include <errno.h>
#include <unistd.h>
#include <features.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <asm/unistd.h>
#include "grp.h"

//#define __NR_setgroups        81
_syscall2(int, setgroups, size_t, size, const gid_t *, list);

