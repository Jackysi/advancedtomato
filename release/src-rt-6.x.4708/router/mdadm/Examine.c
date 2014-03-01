/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2001-2006 Neil Brown <neilb@suse.de>
 *
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    Author: Neil Brown
 *    Email: <neilb@cse.unsw.edu.au>
 *    Paper: Neil Brown
 *           School of Computer Science and Engineering
 *           The University of New South Wales
 *           Sydney, 2052
 *           Australia
 */

#include	"mdadm.h"
#include	"dlink.h"

#if ! defined(__BIG_ENDIAN) && ! defined(__LITTLE_ENDIAN)
#error no endian defined
#endif
#include	"md_u.h"
#include	"md_p.h"
int Examine(mddev_dev_t devlist, int brief, int scan,
	    int SparcAdjust, struct supertype *forcest,
	    char *homehost)
{

	/* Read the raid superblock from a device and
	 * display important content.
	 *
	 * If cannot be found, print reason: too small, bad magic
	 *
	 * Print:
	 *   version, ctime, level, size, raid+spare+
	 *   prefered minor
	 *   uuid
	 *
	 *   utime, state etc
	 *
	 * If (brief) gather devices for same array and just print a mdadm.conf line including devices=
	 * if devlist==NULL, use conf_get_devs()
	 */
	int fd; 
	void *super = NULL;
	int rv = 0;
	int err = 0;

	struct array {
		void *super;
		struct supertype *st;
		struct mdinfo info;
		void *devs;
		struct array *next;
		int spares;
	} *arrays = NULL;

	for (; devlist ; devlist=devlist->next) {
		struct supertype *st = forcest;

		fd = dev_open(devlist->devname, O_RDONLY);
		if (fd < 0) {
			if (!scan) {
				fprintf(stderr,Name ": cannot open %s: %s\n",
					devlist->devname, strerror(errno));
				rv = 1;
			}
			err = 1;
		}
		else {
			if (!st)
				st = guess_super(fd);
			if (st)
				err = st->ss->load_super(st, fd, &super, (brief||scan)?NULL:devlist->devname);
			else {
				if (!brief) {
					fprintf(stderr, Name ": No md superblock detected on %s.\n", devlist->devname);
					rv = 1;
				}
				err = 1;
			}
			close(fd);
		}
		if (err)
			continue;

		if (SparcAdjust)
			st->ss->update_super(NULL, super, "sparc2.2", devlist->devname, 0, 0, NULL);
		/* Ok, its good enough to try, though the checksum could be wrong */
		if (brief) {
			struct array *ap;
			char *d;
			for (ap=arrays; ap; ap=ap->next) {
				if (st->ss == ap->st->ss && st->ss->compare_super(&ap->super, super)==0)
					break;
			}
			if (!ap) {
				ap = malloc(sizeof(*ap));
				ap->super = super;
				ap->devs = dl_head();
				ap->next = arrays;
				ap->spares = 0;
				ap->st = st;
				arrays = ap;
				st->ss->getinfo_super(&ap->info, super);
			} else {
				st->ss->getinfo_super(&ap->info, super);
				free(super);
			}
			if (!(ap->info.disk.state & MD_DISK_SYNC))
				ap->spares++;
			d = dl_strdup(devlist->devname);
			dl_add(ap->devs, d);
		} else {
			printf("%s:\n",devlist->devname);
			st->ss->examine_super(super, homehost);
			free(super);
		}
	}
	if (brief) {
		struct array *ap;
		for (ap=arrays; ap; ap=ap->next) {
			char sep='=';
			char *d;
			ap->st->ss->brief_examine_super(ap->super);
			if (ap->spares) printf("   spares=%d", ap->spares);
			if (brief > 1) {
				printf("   devices");
				for (d=dl_next(ap->devs); d!= ap->devs; d=dl_next(d)) {
					printf("%c%s", sep, d);
					sep=',';
				}
			}
			free(ap->super);
			/* FIXME free ap */
			if (ap->spares || brief > 1)
				printf("\n");
		}
	}
	return rv;
}
