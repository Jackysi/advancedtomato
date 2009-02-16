/*
 * JFFS -- Journaling Flash File System, Linux implementation.
 *
 * Copyright (C) 2000  Axis Communications AB.
 *
 * Created by Simon Kagstrom <simonk@axis.com>.
 *
 * $Id: jffs_proc.c,v 1.1.1.4 2003/10/14 08:09:00 sparq Exp $
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *  Overview:
 *   This file defines JFFS partition entries in the proc file system.
 *
 *  TODO:
 *   Create some more proc files for different kinds of info, i.e. statistics
 *   about written and read bytes, number of calls to different routines,
 *   reports about failures.
 */

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/jffs.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/types.h>
#include "jffs_fm.h"
#include "jffs_proc.h"

/*
 * Structure for a JFFS partition in the system
 */
struct jffs_partition_dir {
	struct jffs_control *c;
	struct proc_dir_entry *part_root;
	struct proc_dir_entry *part_info;
	struct proc_dir_entry *part_layout;
	struct jffs_partition_dir *next;
};

/*
 * Structure for top-level entry in '/proc/fs' directory
 */
struct proc_dir_entry *jffs_proc_root;

/*
 * Linked list of 'jffs_partition_dirs' to help us track
 * the mounted JFFS partitions in the system
 */
static struct jffs_partition_dir *jffs_part_dirs = 0;

/*
 * Read functions for entries
 */
static int jffs_proc_info_read(char *page, char **start, off_t off,
		int count, int *eof, void *data);
static int jffs_proc_layout_read (char *page, char **start, off_t off,
		int count, int *eof, void *data);


/*
 * Register a JFFS partition directory (called upon mount)
 */
int jffs_register_jffs_proc_dir(kdev_t dev, struct jffs_control *c)
{
	struct jffs_partition_dir *part_dir;
	struct proc_dir_entry *part_info = 0;
	struct proc_dir_entry *part_layout = 0;
	struct proc_dir_entry *part_root = 0;

	/* Allocate structure for local JFFS partition table */
	if (!(part_dir = (struct jffs_partition_dir *)
		kmalloc (sizeof (struct jffs_partition_dir), GFP_KERNEL))) {
		return -ENOMEM;
	}

	/* Create entry for this partition */
	if ((part_root = create_proc_entry (kdevname(dev),
		S_IFDIR | S_IRUGO | S_IXUGO, jffs_proc_root))) {
		part_root->read_proc = jffs_proc_info_read;
		part_root->data = (void *) c;
	}
	else {
		kfree (part_dir);
		return -ENOMEM;
	}

	/* Create entry for 'info' file */
	if ((part_info = create_proc_entry ("info", 0, part_root))) {
		part_info->read_proc = jffs_proc_info_read;
		part_info->data = (void *) c;
	}
	else {
		remove_proc_entry (part_root->name, jffs_proc_root);
		kfree (part_dir);
		return -ENOMEM;
	}

	/* Create entry for 'layout' file */
	if ((part_layout = create_proc_entry ("layout", 0, part_root))) {
		part_layout->read_proc = jffs_proc_layout_read;
		part_layout->data = (void *) c;
	}
	else {
		remove_proc_entry (part_info->name, part_root);
		remove_proc_entry (part_root->name, jffs_proc_root);
		kfree (part_dir);
		return -ENOMEM;
	}

	/* Fill in structure for table and insert in the list */
	part_dir->c = c;
	part_dir->part_root = part_root;
	part_dir->part_info = part_info;
	part_dir->part_layout = part_layout;
	part_dir->next = jffs_part_dirs;
	jffs_part_dirs = part_dir;

	/* Return happy */
	return 0;
}


/*
 * Unregister a JFFS partition directory (called at umount)
 */
int jffs_unregister_jffs_proc_dir(struct jffs_control *c)
{
	struct jffs_partition_dir *part_dir = jffs_part_dirs;
	struct jffs_partition_dir *prev_part_dir = 0;

	while (part_dir) {
		if (part_dir->c == c) {
			/* Remove entries for partition */
			remove_proc_entry (part_dir->part_info->name,
				part_dir->part_root);
			remove_proc_entry (part_dir->part_layout->name,
				part_dir->part_root);
			remove_proc_entry (part_dir->part_root->name,
				jffs_proc_root);

			/* Remove entry from list */
			if (prev_part_dir)
				prev_part_dir->next = part_dir->next;
			else
				jffs_part_dirs = part_dir->next;

			/*
			 * Check to see if this is the last one
			 * and remove the entry from '/proc/fs'
			 * if it is.
			 */
			if (jffs_part_dirs == part_dir->next)
#if LINUX_VERSION_CODE < 0x020300
				remove_proc_entry ("jffs", &proc_root_fs);
#else
				remove_proc_entry ("jffs", proc_root_fs);
#endif

			/* Free memory for entry */
			kfree(part_dir);

			/* Return happy */
			return 0;
		}

		/* Move to next entry */
		prev_part_dir = part_dir;
		part_dir = part_dir->next;
	}

	/* Return unhappy */
	return -1;
}


/*
 * Read a JFFS partition's `info' file
 */
static int jffs_proc_info_read (char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct jffs_control *c = (struct jffs_control *) data;
	int len = 0;

	/* Get information on the parition */
	len += sprintf (page,
		"partition size:     %08lX (%lu)\n"
		"sector size:        %08lX (%lu)\n"
		"used size:          %08lX (%lu)\n"
		"dirty size:         %08lX (%lu)\n"
		"free size:          %08lX (%lu)\n"
		"head ofs:           %08lx\n"
		"tail ofs:           %08lx\n"
		"free_size 1:        %08lx (%lu)\n"
		"free_size 2:        %08lx (%lu)\n",
		(unsigned long) c->fmc->flash_size, (unsigned long)c->fmc->flash_size,
		(unsigned long) c->fmc->sector_size,(unsigned long) c->fmc->sector_size,
		(unsigned long) c->fmc->used_size, (unsigned long) c->fmc->used_size,
		(unsigned long) c->fmc->dirty_size, (unsigned long)c->fmc->dirty_size,
		(unsigned long) (c->fmc->flash_size - (c->fmc->used_size + c->fmc->dirty_size)),
		      (unsigned long) (c->fmc->flash_size - (c->fmc->used_size + c->fmc->dirty_size)),

			c->fmc->head? c->fmc->head->offset : 0xdeaddead,
			c->fmc->tail? c->fmc->tail->offset : 0xdeaddead,
			jffs_free_size1(c->fmc), jffs_free_size1(c->fmc),
			jffs_free_size2(c->fmc), jffs_free_size2(c->fmc));

	/* We're done */
	*eof = 1;

	/* Return length */
	return len;
}


/*
 * Read a JFFS partition's `layout' file
 */
static int jffs_proc_layout_read (char *page, char **start, off_t off,
		int count, int *eof, void *data)
{
	struct jffs_control *c = (struct jffs_control *) data;
	struct jffs_fm *fm = 0;
	struct jffs_fm *last_fm = 0;
	int len = 0;

	/* Get the first item in the list */
 	fm = c->fmc->head;

#if 0
	/* Print free space */		/* Yeah, but this isn't right!! */
	if (fm && fm->offset) {
		len += sprintf (page, "00000000 %08lX free\n",
			(unsigned long) fm->offset);
	}
#endif
	/* Loop through all of the flash control structures */
	while (fm && (len < (off + count))) {
		if (fm->nodes) {
			len += sprintf (page + len,
				"%8lX %8lX ino=%8lX, ver=%8lX"
				" [ofs: %8lx  siz: %8lx  repl: %8lx]\n",
			(unsigned long) fm->offset,
			(unsigned long) fm->size,
			(unsigned long) fm->nodes->node->ino,
			(unsigned long) fm->nodes->node->version,
			(unsigned long) fm->nodes->node->data_offset,
			(unsigned long) fm->nodes->node->data_size,
			(unsigned long) fm->nodes->node->removed_size);
		}
		else {
			len += sprintf (page + len,
				"%08lX %08lX dirty\n",
				(unsigned long) fm->offset,
				(unsigned long) fm->size);
		}
		last_fm = fm;
		fm = fm->next;
	}

#if 0
	/* Print free space */		/* Yeah, but this isn't right!! */
	if ((len < (off + count)) && last_fm
	    && (last_fm->offset < c->fmc->flash_size)) {
		len += sprintf (page + len,
			       "%08lX %08lX free\n",
			       (unsigned long) last_fm->offset + 
				last_fm->size,
			       (unsigned long) (c->fmc->flash_size -
						    (last_fm->offset + last_fm->size)));
	}
#endif
	/* We're done */
	*eof = 1;

	/* Return length */
	return len;
}
