/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *  Copyright (C) 1998 Peter J. Braam <braam@clusterfs.com>
 *  Copyright (C) 2000 Red Hat, Inc.
 *  Copyright (C) 2000 Los Alamos National Laboratory
 *  Copyright (C) 2000 TurboLinux, Inc.
 *  Copyright (C) 2001 Mountain View Data, Inc.
 *
 *   This file is part of InterMezzo, http://www.inter-mezzo.org.
 *
 *   InterMezzo is free software; you can redistribute it and/or
 *   modify it under the terms of version 2 of the GNU General Public
 *   License as published by the Free Software Foundation.
 *
 *   InterMezzo is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with InterMezzo; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/types.h>
#include <linux/param.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/smp_lock.h>
#include <linux/locks.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/string.h>
