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

#include "mdadm.h"
#include	"md_u.h"
#include	"md_p.h"
#include	<ctype.h>

int Create(struct supertype *st, char *mddev, int mdfd,
	   int chunk, int level, int layout, unsigned long long size, int raiddisks, int sparedisks,
	   char *name, char *homehost, int *uuid,
	   int subdevs, mddev_dev_t devlist,
	   int runstop, int verbose, int force, int assume_clean,
	   char *bitmap_file, int bitmap_chunk, int write_behind, int delay)
{
	/*
	 * Create a new raid array.
	 *
	 * First check that necessary details are available
	 * (i.e. level, raid-disks)
	 *
	 * Then check each disk to see what might be on it
	 * and report anything interesting.
	 *
	 * If anything looks odd, and runstop not set,
	 * abort.
	 *
	 * SET_ARRAY_INFO and ADD_NEW_DISK, and
	 * if runstop==run, or raiddisks disks were used,
	 * RUN_ARRAY
	 */
	unsigned long long minsize=0, maxsize=0;
	char *mindisc = NULL;
	char *maxdisc = NULL;
	int dnum;
	mddev_dev_t dv;
	int fail=0, warn=0;
	struct stat stb;
	int first_missing = subdevs * 2;
	int missing_disks = 0;
	int insert_point = subdevs * 2; /* where to insert a missing drive */
	void *super;
	int pass;
	int vers;
	int rv;
	int bitmap_fd;
	unsigned long long bitmapsize;

	mdu_array_info_t array;
	int major = BITMAP_MAJOR_HI;

	memset(&array, 0, sizeof(array));

	vers = md_get_version(mdfd);
	if (vers < 9000) {
		fprintf(stderr, Name ": Create requires md driver version 0.90.0 or later\n");
		return 1;
	}
	if (level == UnSet) {
		fprintf(stderr,
			Name ": a RAID level is needed to create an array.\n");
		return 1;
	}
	if (raiddisks < 1) {
		fprintf(stderr,
			Name ": a number of --raid-devices must be given to create an array\n");
		return 1;
	}
	if (raiddisks < 4 && level == 6) {
		fprintf(stderr,
			Name ": at least 4 raid-devices needed for level 6\n");
		return 1;
	}
	if (raiddisks > 256 && level == 6) {
		fprintf(stderr,
			Name ": no more than 256 raid-devices supported for level 6\n");
		return 1;
	}
	if (raiddisks < 2 && level >= 4) {
		fprintf(stderr,
			Name ": at least 2 raid-devices needed for level 4 or 5\n");
		return 1;
	}
	if (subdevs > raiddisks+sparedisks) {
		fprintf(stderr, Name ": You have listed more devices (%d) than are in the array(%d)!\n", subdevs, raiddisks+sparedisks);
		return 1;
	}
	if (subdevs < raiddisks+sparedisks) {
		fprintf(stderr, Name ": You haven't given enough devices (real or missing) to create this array\n");
		return 1;
	}

	/* now set some defaults */
	if (layout == UnSet)
		switch(level) {
		default: /* no layout */
			layout = 0;
			break;
		case 10:
			layout = 0x102; /* near=2, far=1 */
			if (verbose > 0)
				fprintf(stderr,
					Name ": layout defaults to n1\n");
			break;
		case 5:
		case 6:
			layout = map_name(r5layout, "default");
			if (verbose > 0)
				fprintf(stderr,
					Name ": layout defaults to %s\n", map_num(r5layout, layout));
			break;
		case LEVEL_FAULTY:
			layout = map_name(faultylayout, "default");

			if (verbose > 0)
				fprintf(stderr,
					Name ": layout defaults to %s\n", map_num(faultylayout, layout));
			break;
		}

	if (level == 10)
		/* check layout fits in array*/
		if ((layout&255) * ((layout>>8)&255) > raiddisks) {
			fprintf(stderr, Name ": that layout requires at least %d devices\n",
				(layout&255) * ((layout>>8)&255));
			return 1;
		}

	switch(level) {
	case 4:
	case 5:
	case 10:
	case 6:
	case 0:
	case -1: /* linear */
		if (chunk == 0) {
			chunk = 64;
			if (verbose > 0)
				fprintf(stderr, Name ": chunk size defaults to 64K\n");
		}
		break;
	default: /* raid1, multipath */
		if (chunk) {
			chunk = 0;
			if (verbose > 0)
				fprintf(stderr, Name ": chunk size ignored for this level\n");
		}
		break;
	}

	/* now look at the subdevs */
	array.active_disks = 0;
	array.working_disks = 0;
	dnum = 0;
	for (dv=devlist; dv; dv=dv->next, dnum++) {
		char *dname = dv->devname;
		unsigned long long ldsize, freesize;
		int fd;
		if (strcasecmp(dname, "missing")==0) {
			if (first_missing > dnum)
				first_missing = dnum;
			missing_disks ++;
			continue;
		}
		array.working_disks++;
		if (dnum < raiddisks)
			array.active_disks++;
		fd = open(dname, O_RDONLY|O_EXCL, 0);
		if (fd <0 ) {
			fprintf(stderr, Name ": Cannot open %s: %s\n",
				dname, strerror(errno));
			fail=1;
			continue;
		}
		if (!get_dev_size(fd, dname, &ldsize)) {
			fail = 1;
			close(fd);
			continue;
		}
		if (st == NULL) {
			struct createinfo *ci = conf_get_create_info();
			if (ci)
				st = ci->supertype;
		}
		if (st == NULL) {
			/* Need to choose a default metadata, which is different
			 * depending on the sizes of devices
			 */
			int i;
			char *name = "default";
			if (level >= 1 && ldsize > (0x7fffffffULL<<10))
				name = "default/large";
			for(i=0; !st && superlist[i]; i++)
				st = superlist[i]->match_metadata_desc(name);

			if (!st) {
				fprintf(stderr, Name ": internal error - no default metadata style\n");
				exit(2);
			}
			if (st->ss->major != 0 ||
			    st->minor_version != 90)
				fprintf(stderr, Name ": Defaulting to verion %d.%d metadata\n",
					st->ss->major,
					st->minor_version);
		}
		freesize = st->ss->avail_size(st, ldsize >> 9);
		if (freesize == 0) {
			fprintf(stderr, Name ": %s is too small: %luK\n",
				dname, (unsigned long)(ldsize>>10));
			fail = 1;
			close(fd);
			continue;
		}

		freesize /= 2; /* convert to K */
		if (chunk) {
			/* round to chunk size */
			freesize = freesize & ~(chunk-1);
		}

		if (size && freesize < size) {
			fprintf(stderr, Name ": %s is smaller that given size."
				" %lluK < %lluK + superblock\n", dname, freesize, size);
			fail = 1;
			close(fd);
			continue;
		}
		if (maxdisc == NULL || (maxdisc && freesize > maxsize)) {
			maxdisc = dname;
			maxsize = freesize;
		}
		if (mindisc ==NULL || (mindisc && freesize < minsize)) {
			mindisc = dname;
			minsize = freesize;
		}
		if (runstop != 1 || verbose >= 0) {
			warn |= check_ext2(fd, dname);
			warn |= check_reiser(fd, dname);
			warn |= check_raid(fd, dname);
		}
		close(fd);
	}
	if (fail) {
		fprintf(stderr, Name ": create aborted\n");
		return 1;
	}
	if (size == 0) {
		if (mindisc == NULL) {
			fprintf(stderr, Name ": no size and no drives given - aborting create.\n");
			return 1;
		}
		if (level > 0 || level == LEVEL_MULTIPATH || level == LEVEL_FAULTY) {
			/* size is meaningful */
			if (minsize > 0x100000000ULL && st->ss->major == 0) {
				fprintf(stderr, Name ": devices too large for RAID level %d\n", level);	
				return 1;
			}
			size = minsize;
			if (verbose > 0)
				fprintf(stderr, Name ": size set to %lluK\n", size);
		}
	}
	if (level > 0 && ((maxsize-size)*100 > maxsize)) {
		if (runstop != 1 || verbose >= 0)
			fprintf(stderr, Name ": largest drive (%s) exceed size (%lluK) by more than 1%%\n",
				maxdisc, size);
		warn = 1;
	}

	if (warn) {
		if (runstop!= 1) {
			if (!ask("Continue creating array? ")) {
				fprintf(stderr, Name ": create aborted.\n");
				return 1;
			}
		} else {
			if (verbose > 0)
				fprintf(stderr, Name ": creation continuing despite oddities due to --run\n");
		}
	}

	/* If this is  raid5, we want to configure the last active slot
	 * as missing, so that a reconstruct happens (faster than re-parity)
	 * FIX: Can we do this for raid6 as well?
	 */
	if (assume_clean==0 && force == 0 && first_missing >= raiddisks) {
		switch ( level ) {
		case 5:
			insert_point = raiddisks-1;
			sparedisks++;
			array.active_disks--;
			missing_disks++;
			break;
		default:
			break;
		}
	}
	
	/* Ok, lets try some ioctls */

	array.level = level;
	array.size = size;
	array.raid_disks = raiddisks;
	/* The kernel should *know* what md_minor we are dealing
	 * with, but it chooses to trust me instead. Sigh
	 */
	array.md_minor = 0;
	if (fstat(mdfd, &stb)==0)
		array.md_minor = minor(stb.st_rdev);
	array.not_persistent = 0;
	/*** FIX: Need to do something about RAID-6 here ***/
	if ( ( (level == 5) &&
	       (insert_point < raiddisks || first_missing < raiddisks) )
	     ||
	     ( level == 6 && missing_disks == 2)
	     ||
	     assume_clean
		)
		array.state = 1; /* clean, but one+ drive will be missing */
	else
		array.state = 0; /* not clean, but no errors */

	if (level == 10) {
		/* for raid10, the bitmap size is the capacity of the array,
		 * which is array.size * raid_disks / ncopies;
		 * .. but convert to sectors.
		 */
		int ncopies = ((layout>>8) & 255) * (layout & 255);
		bitmapsize = (unsigned long long)size * raiddisks / ncopies * 2;
/*		printf("bms=%llu as=%d rd=%d nc=%d\n", bitmapsize, size, raiddisks, ncopies);*/
	} else
		bitmapsize = (unsigned long long)size * 2;

	/* There is lots of redundancy in these disk counts,
	 * raid_disks is the most meaningful value
	 *          it describes the geometry of the array
	 *          it is constant
	 * nr_disks is total number of used slots.
	 *          it should be raid_disks+spare_disks
	 * spare_disks is the number of extra disks present
	 *          see above
	 * active_disks is the number of working disks in
	 *          active slots. (With raid_disks)
	 * working_disks is the total number of working disks,
	 *          including spares
	 * failed_disks is the number of disks marked failed
	 *
         * Ideally, the kernel would keep these (except raid_disks)
	 * up-to-date as we ADD_NEW_DISK, but it doesn't (yet).
	 * So for now, we assume that all raid and spare
	 * devices will be given.
	 */
	array.spare_disks=sparedisks;
	array.failed_disks=missing_disks;
	array.nr_disks = array.working_disks + array.failed_disks;
	array.layout = layout;
	array.chunk_size = chunk*1024;
	array.major_version = st->ss->major;

	if (name == NULL || *name == 0) {
		/* base name on mddev */
		/*  /dev/md0 -> 0
		 *  /dev/md_d0 -> d0
		 *  /dev/md/1 -> 1
		 *  /dev/md/d1 -> d1
		 *  /dev/md/home -> home
		 *  /dev/mdhome -> home
		 */
		name = strrchr(mddev, '/');
		if (name) {
			name++;
			if (strncmp(name, "md_d", 4)==0 &&
			    strlen(name) > 4 &&
			    isdigit(name[4]) &&
			    (name-mddev) == 5 /* /dev/ */)
				name += 3;
			else if (strncmp(name, "md", 2)==0 &&
				 strlen(name) > 2 &&
				 isdigit(name[2]) &&
				 (name-mddev) == 5 /* /dev/ */)
				name += 2;
		}
	}
	if (!st->ss->init_super(st, &super, &array, size, name, homehost, uuid))
		return 1;

	if (bitmap_file && vers < 9003) {
		major = BITMAP_MAJOR_HOSTENDIAN;
#ifdef __BIG_ENDIAN
		fprintf(stderr, Name ": Warning - bitmaps created on this kernel are not portable\n"
			"  between different architectured.  Consider upgrading the Linux kernel.\n");
#endif
	}

	if (bitmap_file && strcmp(bitmap_file, "internal")==0) {
		if ((vers%100) < 2) {
			fprintf(stderr, Name ": internal bitmaps not supported by this kernel.\n");
			return 1;
		}
		if (!st->ss->add_internal_bitmap(st, super, &bitmap_chunk,
						 delay, write_behind,
						 bitmapsize, 1, major)) {
			fprintf(stderr, Name ": Given bitmap chunk size not supported.\n");
			return 1;
		}
		bitmap_file = NULL;
	}



	if ((vers % 100) >= 1) { /* can use different versions */
		mdu_array_info_t inf;
		memset(&inf, 0, sizeof(inf));
		inf.major_version = st->ss->major;
		inf.minor_version = st->minor_version;
		rv = ioctl(mdfd, SET_ARRAY_INFO, &inf);
	} else 
		rv = ioctl(mdfd, SET_ARRAY_INFO, NULL);
	if (rv) {
		fprintf(stderr, Name ": SET_ARRAY_INFO failed for %s: %s\n",
			mddev, strerror(errno));
		return 1;
	}

	if (bitmap_file) {
		int uuid[4];

		st->ss->uuid_from_super(uuid, super);
		if (CreateBitmap(bitmap_file, force, (char*)uuid, bitmap_chunk,
				 delay, write_behind,
				 bitmapsize,
				 major)) {
			return 1;
		}
		bitmap_fd = open(bitmap_file, O_RDWR);
		if (bitmap_fd < 0) {
			fprintf(stderr, Name ": weird: %s cannot be openned\n",
				bitmap_file);
			return 1;
		}
		if (ioctl(mdfd, SET_BITMAP_FILE, bitmap_fd) < 0) {
			fprintf(stderr, Name ": Cannot set bitmap file for %s: %s\n",
				mddev, strerror(errno));
			return 1;
		}
	}



	for (pass=1; pass <=2 ; pass++) {
		mddev_dev_t moved_disk = NULL; /* the disk that was moved out of the insert point */

		for (dnum=0, dv = devlist ; dv ;
		     dv=(dv->next)?(dv->next):moved_disk, dnum++) {
			int fd;
			struct stat stb;
			mdu_disk_info_t disk;

			disk.number = dnum;
			if (dnum == insert_point) {
				moved_disk = dv;
			}
			disk.raid_disk = disk.number;
			if (disk.raid_disk < raiddisks)
				disk.state = (1<<MD_DISK_ACTIVE) |
						(1<<MD_DISK_SYNC);
			else
				disk.state = 0;
			if (dv->writemostly)
				disk.state |= (1<<MD_DISK_WRITEMOSTLY);

			if (dnum == insert_point ||
			    strcasecmp(dv->devname, "missing")==0) {
				disk.major = 0;
				disk.minor = 0;
				disk.state = (1<<MD_DISK_FAULTY);
			} else {
				fd = open(dv->devname, O_RDONLY|O_EXCL, 0);
				if (fd < 0) {
					fprintf(stderr, Name ": failed to open %s after earlier success - aborting\n",
						dv->devname);
					return 1;
				}
				fstat(fd, &stb);
				disk.major = major(stb.st_rdev);
				disk.minor = minor(stb.st_rdev);
				remove_partitions(fd);
				close(fd);
			}
			switch(pass){
			case 1:
				st->ss->add_to_super(super, &disk);
				break;
			case 2:
				if (disk.state == 1) break;
				Kill(dv->devname, 0, 1); /* Just be sure it is clean */
				Kill(dv->devname, 0, 1); /* and again, there could be two superblocks */
				st->ss->write_init_super(st, super, &disk, dv->devname);

				if (ioctl(mdfd, ADD_NEW_DISK, &disk)) {
					fprintf(stderr, Name ": ADD_NEW_DISK for %s failed: %s\n",
						dv->devname, strerror(errno));
					free(super);
					return 1;
				}

				break;
			}
			if (dv == moved_disk && dnum != insert_point) break;
		}
	}
	free(super);

	/* param is not actually used */
	if (runstop == 1 || subdevs >= raiddisks) {
		mdu_param_t param;
		if (ioctl(mdfd, RUN_ARRAY, &param)) {
			fprintf(stderr, Name ": RUN_ARRAY failed: %s\n",
				strerror(errno));
			Manage_runstop(mddev, mdfd, -1, 0);
			return 1;
		}
		if (verbose >= 0)
			fprintf(stderr, Name ": array %s started.\n", mddev);
	} else {
		fprintf(stderr, Name ": not starting array - not enough devices.\n");
	}
	return 0;
}
