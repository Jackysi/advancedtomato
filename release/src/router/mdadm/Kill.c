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
 *
 *    Added by Dale Stephenson
 *    steph@snapserver.com
 */

#include	"mdadm.h"
#include	"md_u.h"
#include	"md_p.h"

int Kill(char *dev, int force, int quiet)
{
	/*
	 * Nothing fancy about Kill.  It just zeroes out a superblock
	 * Definitely not safe.
	 */

	void *super;
	int fd, rv = 0;
	struct supertype *st;
		
	fd = open(dev, O_RDWR|O_EXCL);
	if (fd < 0) {
		if (!quiet)
			fprintf(stderr, Name ": Couldn't open %s for write - not zeroing\n",
				dev);
		close(fd);
		return 1;
	}
	st = guess_super(fd);
	if (st == NULL) {
		if (!quiet)
			fprintf(stderr, Name ": Unrecognised md component device - %s\n", dev);
		close(fd);
		return 1;
	}
	rv = st->ss->load_super(st, fd, &super, dev);
	if (force && rv >= 2)
		rv = 0; /* ignore bad data in superblock */
	if (rv== 0 || (force && rv >= 2)) {
		mdu_array_info_t info;
		info.major_version = -1; /* zero superblock */
		free(super);
		st->ss->init_super(st, &super, &info, 0, "", NULL, NULL);
		if (st->ss->store_super(st, fd, super)) {
			if (!quiet)
				fprintf(stderr, Name ": Could not zero superblock on %s\n",
					dev);
			rv = 1;
		} else if (rv) {
			if (!quiet)
				fprintf(stderr, Name ": superblock zeroed anyway\n");
			rv = 0;
		}
	}
	close(fd);
	return rv;
}
