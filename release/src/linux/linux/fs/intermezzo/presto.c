/* -*- mode: c; c-basic-offset: 8; indent-tabs-mode: nil; -*-
 * vim:expandtab:shiftwidth=8:tabstop=8:
 *
 *  Author: Peter J. Braam <braam@clusterfs.com>
 *  Copyright (C) 1998 Stelias Computing Inc
 *  Copyright (C) 1999 Red Hat Inc.
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
 *
 * This file implements basic routines supporting the semantics
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/stat.h>
#include <linux/errno.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/locks.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/smp_lock.h>

#include <linux/intermezzo_fs.h>
#include <linux/intermezzo_psdev.h>

int presto_walk(const char *name, struct nameidata *nd)
{
        int err;
        unsigned int flags = LOOKUP_POSITIVE;

        ENTRY;
        err = 0;
        if (path_init(name, flags, nd)) 
                err = path_walk(name, nd);
        return err;
}


/* find the presto minor device for this inode */
int presto_i2m(struct inode *inode)
{
        struct presto_cache *cache;
        ENTRY;
        cache = presto_get_cache(inode);
        CDEBUG(D_PSDEV, "\n");
        if ( !cache ) {
                CERROR("PRESTO: BAD: cannot find cache for dev %d, ino %ld\n",
                       inode->i_dev, inode->i_ino);
                EXIT;
                return -1;
        }
        EXIT;
        return cache->cache_psdev->uc_minor;
}

inline int presto_f2m(struct presto_file_set *fset)
{
        return fset->fset_cache->cache_psdev->uc_minor;

}

inline int presto_c2m(struct presto_cache *cache)
{
        return cache->cache_psdev->uc_minor;

}

struct presto_file_set *presto_path2fileset(const char *name)
{
        struct nameidata nd;
        struct presto_file_set *fileset;
        int error;
        ENTRY;

        error = presto_walk(name, &nd);
        if (!error) { 
                if (!error) 
                        fileset = presto_fset(nd.dentry); 
                path_release(&nd); 
                EXIT;
        } else 
                fileset = ERR_PTR(error);

        EXIT;
        return fileset;
}

/* check a flag on this dentry or fset root.  Semantics:
   - most flags: test if it is set
   - PRESTO_ATTR, PRESTO_DATA return 1 if PRESTO_FSETINSYNC is set
*/
int presto_chk(struct dentry *dentry, int flag)
{
        int minor;
        struct presto_file_set *fset = presto_fset(dentry);

        ENTRY;
        minor = presto_i2m(dentry->d_inode);
        if ( izo_channels[minor].uc_no_filter ) {
                EXIT;
                return ~0;
        }

        /* if the fileset is in sync DATA and ATTR are OK */
        if ( fset &&
             (flag == PRESTO_ATTR || flag == PRESTO_DATA) &&
             (fset->fset_flags & FSET_INSYNC) ) {
                CDEBUG(D_INODE, "fset in sync (ino %ld)!\n",
                       fset->fset_dentry->d_inode->i_ino);
                EXIT;
                return 1;
        }

        EXIT;
        return (presto_d2d(dentry)->dd_flags & flag);
}

/* set a bit in the dentry flags */
void presto_set(struct dentry *dentry, int flag)
{
        ENTRY;
        if ( dentry->d_inode ) {
                CDEBUG(D_INODE, "SET ino %ld, flag %x\n",
                       dentry->d_inode->i_ino, flag);
        }
        if ( presto_d2d(dentry) == NULL) {
                CERROR("dentry without d_fsdata in presto_set: %p: %*s", dentry,
                                dentry->d_name.len, dentry->d_name.name);
                BUG();
        }
        presto_d2d(dentry)->dd_flags |= flag;
        EXIT;
}

/* given a path: complete the closes on the fset */
int lento_complete_closes(char *path)
{
        struct nameidata nd;
        struct dentry *dentry;
        int error;
        struct presto_file_set *fset;
        ENTRY;

        error = presto_walk(path, &nd);
        if (error) {
                EXIT;
                return error;
        }

        dentry = nd.dentry;

        error = -ENXIO;
        if ( !presto_ispresto(dentry->d_inode) ) {
                EXIT;
                goto out_complete;
        }
        
        fset = presto_fset(dentry);
        error = -EINVAL;
        if ( !fset ) {
                CERROR("No fileset!\n");
                EXIT;
                goto out_complete;
        }
        
        /* transactions and locking are internal to this function */ 
        error = presto_complete_lml(fset);
        
        EXIT;
 out_complete:
        path_release(&nd); 
        return error;
}       


/* given a dentry, operate on the flags in its dentry.  Used by downcalls */
int izo_mark_dentry(struct dentry *dentry, int and_flag, int or_flag, 
                       int *res)
{
        int error = 0;

        if (presto_d2d(dentry) == NULL) {
                CERROR("InterMezzo: no ddata for inode %ld in %s\n",
                       dentry->d_inode->i_ino, __FUNCTION__);
                return -EINVAL;
        }

        CDEBUG(D_INODE, "inode: %ld, and flag %x, or flag %x, dd_flags %x\n",
               dentry->d_inode->i_ino, and_flag, or_flag,
               presto_d2d(dentry)->dd_flags);

        presto_d2d(dentry)->dd_flags &= and_flag;
        presto_d2d(dentry)->dd_flags |= or_flag;
        if (res) 
                *res = presto_d2d(dentry)->dd_flags;

        return error;
}

/* given a path, operate on the flags in its cache.  Used by mark_ioctl */
int izo_mark_cache(struct dentry *dentry, int and_flag, int or_flag, 
                   int *res)
{
        struct presto_cache *cache;

        if (presto_d2d(dentry) == NULL) {
                CERROR("InterMezzo: no ddata for inode %ld in %s\n",
                       dentry->d_inode->i_ino, __FUNCTION__);
                return -EINVAL;
        }

        CDEBUG(D_INODE, "inode: %ld, and flag %x, or flag %x, dd_flags %x\n",
               dentry->d_inode->i_ino, and_flag, or_flag,
               presto_d2d(dentry)->dd_flags);

        cache = presto_get_cache(dentry->d_inode);
        if ( !cache ) {
                CERROR("PRESTO: BAD: cannot find cache in izo_mark_cache\n");
                return -EBADF;
        }

        ((int)cache->cache_flags) &= and_flag;
        ((int)cache->cache_flags) |= or_flag;
        if (res)
                *res = (int)cache->cache_flags;

        return 0;
}

int presto_set_max_kml_size(const char *path, unsigned long max_size)
{
        struct presto_file_set *fset;

        ENTRY;

        fset = presto_path2fileset(path);
        if (IS_ERR(fset)) {
                EXIT;
                return PTR_ERR(fset);
        }

        fset->kml_truncate_size = max_size;
        CDEBUG(D_CACHE, "KML truncate size set to %lu bytes for fset %s.\n",
               max_size, path);

        EXIT;
        return 0;
}

int izo_mark_fset(struct dentry *dentry, int and_flag, int or_flag, 
                  int * res)
{
        struct presto_file_set *fset;
        
        fset = presto_fset(dentry);
        if ( !fset ) {
                CERROR("PRESTO: BAD: cannot find cache in izo_mark_cache\n");
                make_bad_inode(dentry->d_inode);
                return -EBADF;
        }
        ((int)fset->fset_flags) &= and_flag;
        ((int)fset->fset_flags) |= or_flag;
        if (res)
                *res = (int)fset->fset_flags;

        return 0;
}

/* talk to Lento about the permit */
static int presto_permit_upcall(struct dentry *dentry)
{
        int rc;
        char *path, *buffer;
        int pathlen;
        int minor;
        int fsetnamelen;
        struct presto_file_set *fset = NULL;

        ENTRY;

        if ( (minor = presto_i2m(dentry->d_inode)) < 0) {
                EXIT;
                return -EINVAL;
        }

        fset = presto_fset(dentry);
        if (!fset) {
                EXIT;
                return -ENOTCONN;
        }
        
        if ( !presto_lento_up(minor) ) {
                if ( fset->fset_flags & FSET_STEAL_PERMIT ) {
                        EXIT;
                        return 0;
                } else {
                        EXIT;
                        return -ENOTCONN;
                }
        }

        PRESTO_ALLOC(buffer, PAGE_SIZE);
        if ( !buffer ) {
                CERROR("PRESTO: out of memory!\n");
                EXIT;
                return -ENOMEM;
        }
        path = presto_path(dentry, fset->fset_dentry, buffer, PAGE_SIZE);
        pathlen = MYPATHLEN(buffer, path);
        fsetnamelen = strlen(fset->fset_name); 
        rc = izo_upc_permit(minor, dentry, pathlen, path, fset->fset_name);
        PRESTO_FREE(buffer, PAGE_SIZE);
        EXIT;
        return rc;
}

/* get a write permit for the fileset of this inode
 *  - if this returns a negative value there was an error
 *  - if 0 is returned the permit was already in the kernel -- or --
 *    Lento gave us the permit without reintegration
 *  - lento returns the number of records it reintegrated 
 *
 * Note that if this fileset has branches, a permit will -never- to a normal
 * process for writing in the data area (ie, outside of .intermezzo)
 */
int presto_get_permit(struct inode * inode)
{
        struct dentry *de;
        struct presto_file_set *fset;
        int minor = presto_i2m(inode);
        int rc = 0;

        ENTRY;
        if (minor < 0) {
                EXIT;
                return -1;
        }

        if ( ISLENTO(minor) ) {
                EXIT;
                return 0;
        }

        if (list_empty(&inode->i_dentry)) {
                CERROR("No alias for inode %d\n", (int) inode->i_ino);
                EXIT;
                return -EINVAL;
        }

        de = list_entry(inode->i_dentry.next, struct dentry, d_alias);

        if (presto_chk(de, PRESTO_DONT_JOURNAL)) {
                EXIT;
                return 0;
        }

        fset = presto_fset(de);
        if ( !fset ) {
                CERROR("Presto: no fileset in presto_get_permit!\n");
                EXIT;
                return -EINVAL;
        }

        if (fset->fset_flags & FSET_HAS_BRANCHES) {
                EXIT;
                return -EROFS;
        }

        spin_lock(&fset->fset_permit_lock);
        if (fset->fset_flags & FSET_HASPERMIT) {
                fset->fset_permit_count++;
                CDEBUG(D_INODE, "permit count now %d, inode %lx\n", 
                       fset->fset_permit_count, inode->i_ino);
                spin_unlock(&fset->fset_permit_lock);
                EXIT;
                return 0;
        }

        /* Allow reintegration to proceed without locks -SHP */
        fset->fset_permit_upcall_count++;
        if (fset->fset_permit_upcall_count == 1) {
                spin_unlock(&fset->fset_permit_lock);
                rc = presto_permit_upcall(fset->fset_dentry);
                spin_lock(&fset->fset_permit_lock);
                fset->fset_permit_upcall_count--;
                if (rc == 0) {
                        izo_mark_fset(fset->fset_dentry, ~0, FSET_HASPERMIT,
                                      NULL);
                        fset->fset_permit_count++;
                } else if (rc == ENOTCONN) {
                        CERROR("InterMezzo: disconnected operation. stealing permit.\n");
                        izo_mark_fset(fset->fset_dentry, ~0, FSET_HASPERMIT,
                                      NULL);
                        fset->fset_permit_count++;
                        /* set a disconnected flag here to stop upcalls */
                        rc = 0;
                } else {
                        CERROR("InterMezzo: presto_permit_upcall failed: %d\n", rc);
                        rc = -EROFS;
                        /* go to sleep here and try again? */
                }
                wake_up_interruptible(&fset->fset_permit_queue);
        } else {
                /* Someone is already doing an upcall; go to sleep. */
                DECLARE_WAITQUEUE(wait, current);

                spin_unlock(&fset->fset_permit_lock);
                add_wait_queue(&fset->fset_permit_queue, &wait);
                while (1) {
                        set_current_state(TASK_INTERRUPTIBLE);

                        spin_lock(&fset->fset_permit_lock);
                        if (fset->fset_permit_upcall_count == 0)
                                break;
                        spin_unlock(&fset->fset_permit_lock);

                        if (signal_pending(current)) {
                                remove_wait_queue(&fset->fset_permit_queue,
                                                  &wait);
                                return -ERESTARTSYS;
                        }
                        schedule();
                }
                remove_wait_queue(&fset->fset_permit_queue, &wait);
                /* We've been woken up: do we have the permit? */
                if (fset->fset_flags & FSET_HASPERMIT)
                        rc = -EAGAIN;
        }

        CDEBUG(D_INODE, "permit count now %d, ino %ld (likely 1), "
               "rc %d\n", fset->fset_permit_count, inode->i_ino, rc);
        spin_unlock(&fset->fset_permit_lock);
        EXIT;
        return rc;
}

int presto_put_permit(struct inode * inode)
{
        struct dentry *de;
        struct presto_file_set *fset;
        int minor = presto_i2m(inode);

        ENTRY;
        if (minor < 0) {
                EXIT;
                return -1;
        }

        if ( ISLENTO(minor) ) {
                EXIT;
                return 0;
        }

        if (list_empty(&inode->i_dentry)) {
                CERROR("No alias for inode %d\n", (int) inode->i_ino);
                EXIT;
                return -1;
        }

        de = list_entry(inode->i_dentry.next, struct dentry, d_alias);

        fset = presto_fset(de);
        if ( !fset ) {
                CERROR("InterMezzo: no fileset in %s!\n", __FUNCTION__);
                EXIT;
                return -1;
        }

        if (presto_chk(de, PRESTO_DONT_JOURNAL)) {
                EXIT;
                return 0;
        }

        spin_lock(&fset->fset_permit_lock);
        if (fset->fset_flags & FSET_HASPERMIT) {
                if (fset->fset_permit_count > 0)
                        fset->fset_permit_count--;
                else
                        CERROR("Put permit while permit count is 0, "
                               "inode %ld!\n", inode->i_ino); 
        } else {
                fset->fset_permit_count = 0;
                CERROR("InterMezzo: put permit while no permit, inode %ld, "
                       "flags %x!\n", inode->i_ino, fset->fset_flags);
        }

        CDEBUG(D_INODE, "permit count now %d, inode %ld\n",
               fset->fset_permit_count, inode->i_ino);

        if (fset->fset_flags & FSET_PERMIT_WAITING &&
            fset->fset_permit_count == 0) {
                CDEBUG(D_INODE, "permit count now 0, ino %ld, wake sleepers\n",
                       inode->i_ino);
                wake_up_interruptible(&fset->fset_permit_queue);
        }
        spin_unlock(&fset->fset_permit_lock);

        EXIT;
        return 0;
}

void presto_getversion(struct presto_version * presto_version,
                       struct inode * inode)
{
        presto_version->pv_mtime = (__u64)inode->i_mtime;
        presto_version->pv_ctime = (__u64)inode->i_ctime;
        presto_version->pv_size  = (__u64)inode->i_size;
}


int izo_revoke_permit(struct dentry *dentry, __u8 uuid[16])
{
        struct presto_file_set *fset; 
        DECLARE_WAITQUEUE(wait, current);
        int minor, rc;

        ENTRY;

        minor = presto_i2m(dentry->d_inode);
        if (minor < 0) {
                EXIT;
                return -ENODEV;
        }

        fset = presto_fset(dentry);
        if (fset == NULL) {
                EXIT;
                return -ENODEV;
        }

        spin_lock(&fset->fset_permit_lock);
        if (fset->fset_flags & FSET_PERMIT_WAITING) {
                CERROR("InterMezzo: Two processes are waiting on the same permit--this not yet supported!  Aborting this particular permit request...\n");
                EXIT;
                spin_unlock(&fset->fset_permit_lock);
                return -EINVAL;
        }

        if (fset->fset_permit_count == 0)
                goto got_permit;

        /* Something is still using this permit.  Mark that we're waiting for it
         * and go to sleep. */
        rc = izo_mark_fset(dentry, ~0, FSET_PERMIT_WAITING, NULL);
        spin_unlock(&fset->fset_permit_lock);
        if (rc < 0) {
                EXIT;
                return rc;
        }

        add_wait_queue(&fset->fset_permit_queue, &wait);
        while (1) {
                set_current_state(TASK_INTERRUPTIBLE);

                spin_lock(&fset->fset_permit_lock);
                if (fset->fset_permit_count == 0)
                        break;
                spin_unlock(&fset->fset_permit_lock);

                if (signal_pending(current)) {
                        remove_wait_queue(&fset->fset_permit_queue, &wait);
                        EXIT;
                        return -ERESTARTSYS;
                }


                schedule();
        }

        remove_wait_queue(&fset->fset_permit_queue, &wait);
 got_permit:
        /* By this point fset->fset_permit_count is zero and we're holding the
         * lock. */
        CDEBUG(D_CACHE, "InterMezzo: releasing permit inode %ld\n",
               dentry->d_inode->i_ino);

        if (uuid != NULL) {
                rc = izo_upc_revoke_permit(minor, fset->fset_name, uuid);
                if (rc < 0) {
                        spin_unlock(&fset->fset_permit_lock);
                        EXIT;
                        return rc;
                }
        }

        izo_mark_fset(fset->fset_dentry, ~FSET_PERMIT_WAITING, 0, NULL);
        izo_mark_fset(fset->fset_dentry, ~FSET_HASPERMIT, 0, NULL);
        spin_unlock(&fset->fset_permit_lock);
        EXIT;
        return 0;
}

inline int presto_is_read_only(struct presto_file_set * fset)
{
        int minor, mask;
        struct presto_cache *cache = fset->fset_cache;

        minor= cache->cache_psdev->uc_minor;
        mask= (ISLENTO(minor)? FSET_LENTO_RO : FSET_CLIENT_RO);
        if ( fset->fset_flags & mask )
                return 1;
        mask= (ISLENTO(minor)? CACHE_LENTO_RO : CACHE_CLIENT_RO);
        return  ((cache->cache_flags & mask)? 1 : 0);
}
