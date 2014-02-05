/*
 * mdassemble - assemble Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2001-2006 Neil Brown <neilb@suse.de>
 * Copyright (C) 2003 Luca Berra <bluca@vodka.it>
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

#include "mdadm.h"
#include "md_p.h"

/* from readme.c */
mapping_t pers[] = {
	{ "linear", -1},
	{ "raid0", 0},
	{ "0", 0},
	{ "stripe", 0},
	{ "raid1", 1},
	{ "1", 1},
	{ "mirror", 1},
	{ "raid4", 4},
	{ "4", 4},
	{ "raid5", 5},
	{ "5", 5},
	{ "multipath", -4},
	{ "mp", -4},
	{ "raid6", 6},
	{ "6", 6},
	{ "raid10", 10},
	{ "10", 10},
	{ NULL, 0}
};

#ifndef MDASSEMBLE_AUTO
/* from mdopen.c */
int open_mddev(char *dev, int autof/*unused */)
{
	int mdfd = open(dev, O_RDWR, 0);
	if (mdfd < 0)
		fprintf(stderr, Name ": error opening %s: %s\n",
			dev, strerror(errno));
	else if (md_get_version(mdfd) <= 0) {
		fprintf(stderr, Name ": %s does not appear to be an md device\n",
			dev);
		close(mdfd);
		mdfd = -1;
	}
	return mdfd;
}
#endif

int rv;
int mdfd = -1;
int runstop = 0;
int readonly = 0;
int verbose = 0;
int force = 0;

int main(int argc, char *argv[]) {
	mddev_ident_t array_list =  conf_get_ident(NULL);
	if (!array_list) {
		fprintf(stderr, Name ": No arrays found in config file\n");
		rv = 1;
	} else
		for (; array_list; array_list = array_list->next) {
			mdu_array_info_t array;
			mdfd = open_mddev(array_list->devname, array_list->autof);
			if (mdfd < 0) {
				rv |= 1;
				continue;
			}
			if (ioctl(mdfd, GET_ARRAY_INFO, &array) < 0) {
				rv |= Assemble(array_list->st, array_list->devname, mdfd,
					   array_list, NULL, NULL,
					   readonly, runstop, NULL, NULL, verbose, force);
			} else {
				rv |= Manage_ro(array_list->devname, mdfd, -1); /* make it readwrite */
			}
			close(mdfd);
		}
	return rv;
}
