/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2002-2006 Neil Brown <neilb@suse.de>
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
#include	"md_p.h"
#include	"md_u.h"

int Query(char *dev)
{
	/* Give a brief description of the device,
	 * whether it is an md device and whether it has 
	 * a superblock
	 */
	int fd = open(dev, O_RDONLY, 0);
	int vers;
	int ioctlerr;
	int superror, superrno;
	struct mdinfo info;
	mdu_array_info_t array;
	void *super;
	struct supertype *st = NULL;

	unsigned long long larray_size;
	struct stat stb;
	char *mddev;
	mdu_disk_info_t disc;
	char *activity;

	if (fd < 0){
		fprintf(stderr, Name ": cannot open %s: %s\n",
			dev, strerror(errno));
		return 1;
	}

	vers = md_get_version(fd);
	if (ioctl(fd, GET_ARRAY_INFO, &array)<0)
		ioctlerr = errno;
	else ioctlerr = 0;
 
	fstat(fd, &stb);

	if (vers>=9000 && !ioctlerr) {
		if (!get_dev_size(fd, NULL, &larray_size))
			larray_size = 0;
	}

	if (vers < 0) 
		printf("%s: is not an md array\n", dev);
	else if (vers < 9000)
		printf("%s: is an md device, but kernel cannot provide details\n", dev);
	else if (ioctlerr == ENODEV)
		printf("%s: is an md device which is not active\n", dev);
	else if (ioctlerr)
		printf("%s: is an md device, but gives \"%s\" when queried\n",
		       dev, strerror(ioctlerr));
	else {
		printf("%s: %s %s %d devices, %d spare%s. Use mdadm --detail for more detail.\n",
		       dev,
		       human_size_brief(larray_size),
		       map_num(pers, array.level),
		       array.raid_disks,
		       array.spare_disks, array.spare_disks==1?"":"s");
	}
	st = guess_super(fd);
	if (st) {
		superror = st->ss->load_super(st, fd, &super, dev);
		superrno = errno;
	} else 
		superror = -1;
	close(fd);
	if (superror == 0) {
		/* array might be active... */
		st->ss->getinfo_super(&info, super);
		if (st->ss->major == 0) {
			mddev = get_md_name(info.array.md_minor);
			disc.number = info.disk.number;
			activity = "undetected";
			if (mddev && (fd = open(mddev, O_RDONLY))>=0) {
				if (md_get_version(fd) >= 9000 &&	
				    ioctl(fd, GET_ARRAY_INFO, &array)>= 0) {
					if (ioctl(fd, GET_DISK_INFO, &disc) >= 0 &&
					    makedev((unsigned)disc.major,(unsigned)disc.minor) == stb.st_rdev)
						activity = "active";
					else
						activity = "mismatch";
				}
				close(fd);
			}
		} else {
			activity = "unknown";
			mddev = "array";
		}
		printf("%s: device %d in %d device %s %s %s.  Use mdadm --examine for more detail.\n",
		       dev, 
		       info.disk.number, info.array.raid_disks,
		       activity,
		       map_num(pers, info.array.level),
		       mddev);
		if (st->ss->major == 0)
			put_md_name(mddev);
	}
	return 0;
}

	
