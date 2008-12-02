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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/locks.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#ifdef CONFIG_OBDFS_FS
#include /usr/src/obd/include/linux/obdfs.h
#endif

#include <linux/intermezzo_fs.h>
#include <linux/intermezzo_psdev.h>

#ifdef CONFIG_OBDFS_FS


static unsigned long presto_obdfs_freespace(struct presto_file_set *fset,
                                         struct super_block *sb)
{
        return 0x0fffff; 
}

/* start the filesystem journal operations */
static void *presto_obdfs_trans_start(struct presto_file_set *fset, 
                                   struct inode *inode, 
                                   int op)
{

        return (void *) 1;
}


void presto_obdfs_trans_commit(struct presto_file_set *fset, void *handle)
{
}

void presto_obdfs_journal_file_data(struct inode *inode)
{
#ifdef EXT3_JOURNAL_DATA_FL
        inode->u.ext3_i.i_flags |= EXT3_JOURNAL_DATA_FL;
#else
#warning You must have a facility to enable journaled writes for recovery!
#endif
}

struct journal_ops presto_obdfs_journal_ops = {
        .tr_avail        = presto_obdfs_freespace,
        .tr_start        =  presto_obdfs_trans_start,
        .tr_commit       = presto_obdfs_trans_commit,
        .tr_journal_data = presto_obdfs_journal_file_data
};

#endif
