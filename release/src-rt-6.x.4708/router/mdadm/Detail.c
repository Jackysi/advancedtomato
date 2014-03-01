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
#include	"md_p.h"
#include	"md_u.h"

int Detail(char *dev, int brief, int test, char *homehost)
{
	/*
	 * Print out details for an md array by using
	 * GET_ARRAY_INFO and GET_DISK_INFO ioctl calls
	 */

	int fd = open(dev, O_RDONLY, 0);
	int vers;
	mdu_array_info_t array;
	mdu_disk_info_t *disks;
	int next;
	int d;
	time_t atime;
	char *c;
	char *devices = NULL;
	int spares = 0;
	struct stat stb;
	int is_26 = get_linux_version() >= 2006000;
	int is_rebuilding = 0;
	int failed = 0;
	struct supertype *st = NULL;
	int max_disks = MD_SB_DISKS; /* just a default */
	struct mdinfo info;

	void *super = NULL;
	int rv = test ? 4 : 1;

	if (fd < 0) {
		fprintf(stderr, Name ": cannot open %s: %s\n",
			dev, strerror(errno));
		return rv;
	}
	vers = md_get_version(fd);
	if (vers < 0) {
		fprintf(stderr, Name ": %s does not appear to be an md device\n",
			dev);
		close(fd);
		return rv;
	}
	if (vers < 9000) {
		fprintf(stderr, Name ": cannot get detail for md device %s: driver version too old.\n",
			dev);
		close(fd);
		return rv;
	}
	if (ioctl(fd, GET_ARRAY_INFO, &array)<0) {
		if (errno == ENODEV)
			fprintf(stderr, Name ": md device %s does not appear to be active.\n",
				dev);
		else
			fprintf(stderr, Name ": cannot get array detail for %s: %s\n",
				dev, strerror(errno));
		close(fd);
		return rv;
	}
	st = super_by_version(array.major_version, array.minor_version);

	if (fstat(fd, &stb) != 0 && !S_ISBLK(stb.st_mode))
		stb.st_rdev = 0;
	rv = 0;

	if (st) max_disks = st->max_devs;

	/* try to load a superblock */
	for (d= 0; d<max_disks; d++) {
		mdu_disk_info_t disk;
		char *dv;
		disk.number = d;
		if (ioctl(fd, GET_DISK_INFO, &disk) < 0)
			continue;
		if (d >= array.raid_disks &&
		    disk.major == 0 &&
		    disk.minor == 0)
			continue;
		if ((dv=map_dev(disk.major, disk.minor, 1))) {
			if (!super && (disk.state & (1<<MD_DISK_ACTIVE))) {
				/* try to read the superblock from this device
				 * to get more info
				 */
				int fd2 = dev_open(dv, O_RDONLY);
				if (fd2 >=0 && st &&
				    st->ss->load_super(st, fd2, &super, NULL) == 0) {
					st->ss->getinfo_super(&info, super);
					if (info.array.ctime != array.ctime ||
					    info.array.level != array.level) {
						free(super);
						super = NULL;
					}
				}
				if (fd2 >= 0) close(fd2);
			}
		}
	}

	/* Ok, we have some info to print... */
	c = map_num(pers, array.level);
	if (brief) 
		printf("ARRAY %s level=%s num-devices=%d", dev, c?c:"-unknown-",array.raid_disks );
	else {
		mdu_bitmap_file_t bmf;
		unsigned long long larray_size;
		struct mdstat_ent *ms = mdstat_read(0, 0);
		struct mdstat_ent *e;
		int devnum = array.md_minor;
		if (major(stb.st_rdev) != MD_MAJOR)
			devnum = -1 - devnum;

		for (e=ms; e; e=e->next)
			if (e->devnum == devnum)
				break;
		if (!get_dev_size(fd, NULL, &larray_size))
			larray_size = 0;

		printf("%s:\n", dev);
		printf("        Version : %02d.%02d.%02d\n",
		       array.major_version, array.minor_version, array.patch_version);
		atime = array.ctime;
		printf("  Creation Time : %.24s\n", ctime(&atime));
		printf("     Raid Level : %s\n", c?c:"-unknown-");
		if (larray_size)
			printf("     Array Size : %llu%s\n", (larray_size>>10), human_size(larray_size));
		if (array.level >= 1) {
			if (array.major_version != 0 &&
			    (larray_size >= 0xFFFFFFFFULL|| array.size == 0)) {
				unsigned long long dsize = get_component_size(fd);
				if (dsize > 0)
					printf("  Used Dev Size : %llu%s\n",
					       dsize,
					 human_size((long long)array.size<<10));
				else
					printf("  Used Dev Size : unknown\n");
			} else
				printf("  Used Dev Size : %d%s\n", array.size,
				       human_size((long long)array.size<<10));
		}
		printf("   Raid Devices : %d\n", array.raid_disks);
		printf("  Total Devices : %d\n", array.nr_disks);
		printf("Preferred Minor : %d\n", array.md_minor);
		printf("    Persistence : Superblock is %spersistent\n",
		       array.not_persistent?"not ":"");
		printf("\n");
		/* Only try GET_BITMAP_FILE for 0.90.01 and later */
		if (vers >= 9001 &&
		    ioctl(fd, GET_BITMAP_FILE, &bmf) == 0 &&
		    bmf.pathname[0]) {
			printf("  Intent Bitmap : %s\n", bmf.pathname);
			printf("\n");
		} else if (array.state & (1<<MD_SB_BITMAP_PRESENT))
			printf("  Intent Bitmap : Internal\n\n");
		atime = array.utime;
		printf("    Update Time : %.24s\n", ctime(&atime));
		printf("          State : %s%s%s%s\n",
		       (array.state&(1<<MD_SB_CLEAN))?"clean":"active",
		       array.active_disks < array.raid_disks? ", degraded":"",
		       (!e || e->percent < 0) ? "" :
		        (e->resync) ? ", resyncing": ", recovering",
		       larray_size ? "": ", Not Started");
		printf(" Active Devices : %d\n", array.active_disks);
		printf("Working Devices : %d\n", array.working_disks);
		printf(" Failed Devices : %d\n", array.failed_disks);
		printf("  Spare Devices : %d\n", array.spare_disks);
		printf("\n");
		if (array.level == 5) {
			c = map_num(r5layout, array.layout);
			printf("         Layout : %s\n", c?c:"-unknown-");
		}
		if (array.level == 10) {
			printf("         Layout : near=%d, %s=%d\n",
			       array.layout&255, (array.layout&0x10000)?"offset":"far",
			       (array.layout>>8)&255);
		}
		switch (array.level) {
		case 0:
		case 4:
		case 5:
		case 10:
		case 6:
			printf("     Chunk Size : %dK\n\n", array.chunk_size/1024);
			break;
		case -1:
			printf("       Rounding : %dK\n\n", array.chunk_size/1024);
			break;
		default: break;
		}
	
		if (e && e->percent >= 0) {
			printf(" Re%s Status : %d%% complete\n",
			       (super && info.reshape_active)? "shape":"build",
			       e->percent);
			is_rebuilding = 1;
		}
		free_mdstat(ms);

		if (super && info.reshape_active) {
#if 0
This is pretty boring
			printf("  Reshape pos'n : %llu%s\n", (unsigned long long) info.reshape_progress<<9,
			       human_size(info.reshape_progress<<9));
#endif
			if (info.delta_disks > 0)
				printf("  Delta Devices : %d, (%d->%d)\n",
				       info.delta_disks, array.raid_disks - info.delta_disks, array.raid_disks);
			if (info.delta_disks < 0)
				printf("  Delta Devices : %d, (%d->%d)\n",
				       info.delta_disks, array.raid_disks, array.raid_disks + info.delta_disks);
			if (info.new_level != array.level) {
				char *c = map_num(pers, info.new_level);
				printf("      New Level : %s\n", c?c:"-unknown-");
			}
			if (info.new_level != array.level ||
			    info.new_layout != array.layout) {
				if (info.new_level == 5) {
					char *c = map_num(r5layout, info.new_layout);
					printf("     New Layout : %s\n",
					       c?c:"-unknown-");
				}
				if (info.new_level == 10) {
					printf("     New Layout : near=%d, %s=%d\n",
					       info.new_layout&255,
					       (info.new_layout&0x10000)?"offset":"far",
					       (info.new_layout>>8)&255);
				}
			}
			if (info.new_chunk != array.chunk_size)
				printf("  New Chunksize : %dK\n", info.new_chunk/1024);
			printf("\n");
		} else if (e && e->percent >= 0)
			printf("\n");
		if (super && st)
			st->ss->detail_super(super, homehost);

		printf("    Number   Major   Minor   RaidDevice State\n");
	}
	disks = malloc(max_disks * sizeof(mdu_disk_info_t));
	for (d=0; d<max_disks; d++) {
		disks[d].state = (1<<MD_DISK_REMOVED);
		disks[d].major = disks[d].minor = 0;
		disks[d].number = disks[d].raid_disk = d;
	}

	next = array.raid_disks;
	for (d=0; d < max_disks; d++) {
		mdu_disk_info_t disk;
		disk.number = d;
		if (ioctl(fd, GET_DISK_INFO, &disk) < 0) {
			if (d < array.raid_disks)
				fprintf(stderr, Name ": cannot get device detail for device %d: %s\n",
					d, strerror(errno));
			continue;
		}
		if (disk.major == 0 && disk.minor == 0)
			continue;
		if (disk.raid_disk >= 0 && disk.raid_disk < array.raid_disks) 
			disks[disk.raid_disk] = disk;
		else if (next < max_disks)
			disks[next++] = disk;
	}

	for (d= 0; d < max_disks; d++) {
		char *dv;
		mdu_disk_info_t disk = disks[d];

		if (d >= array.raid_disks &&
		    disk.major == 0 &&
		    disk.minor == 0)
			continue;
		if (!brief) {
			if (d == array.raid_disks) printf("\n");
			if (disk.raid_disk < 0)
				printf("   %5d   %5d    %5d        -     ", 
				       disk.number, disk.major, disk.minor);
			else
				printf("   %5d   %5d    %5d    %5d     ", 
				       disk.number, disk.major, disk.minor, disk.raid_disk);
			if (disk.state & (1<<MD_DISK_FAULTY)) { 
				printf(" faulty"); 
				if (disk.raid_disk < array.raid_disks &&
				    disk.raid_disk >= 0)
					failed++;
			}
			if (disk.state & (1<<MD_DISK_ACTIVE)) printf(" active");
			if (disk.state & (1<<MD_DISK_SYNC)) printf(" sync");
			if (disk.state & (1<<MD_DISK_REMOVED)) printf(" removed");
			if (disk.state & (1<<MD_DISK_WRITEMOSTLY)) printf(" writemostly");
			if ((disk.state &
			     ((1<<MD_DISK_ACTIVE)|(1<<MD_DISK_SYNC)|(1<<MD_DISK_REMOVED)))
			    == 0) {
				printf(" spare");
				if (is_26) {
					if (disk.raid_disk < array.raid_disks && disk.raid_disk >= 0)
						printf(" rebuilding");
				} else if (is_rebuilding && failed) {
					/* Taking a bit of a risk here, we remove the
					 * device from the array, and then put it back.
					 * If this fails, we are rebuilding
					 */
					int err = ioctl(fd, HOT_REMOVE_DISK, makedev(disk.major, disk.minor));
					if (err == 0) ioctl(fd, HOT_ADD_DISK, makedev(disk.major, disk.minor));
					if (err && errno ==  EBUSY)
						printf(" rebuilding");
				}
			}
		}
		if (disk.state == 0) spares++;
		if (test && d < array.raid_disks && disk.state & (1<<MD_DISK_FAULTY)) {
			if ((rv & 1) && (array.level ==4 || array.level == 5))
				rv |= 2;
			rv |= 1;
		}
		if ((dv=map_dev(disk.major, disk.minor, 0))) {
			if (brief) {
				if (devices) {
					devices = realloc(devices,
							  strlen(devices)+1+strlen(dv)+1);
					strcat(strcat(devices,","),dv);
				} else
					devices = strdup(dv);
			} else
				printf("   %s", dv);
		}
		if (!brief) printf("\n");
	}
	if (spares && brief) printf(" spares=%d", spares);
	if (super && brief && st)
		st->ss->brief_detail_super(super);

	if (brief > 1 && devices) printf("\n   devices=%s", devices);
	if (brief) printf("\n");
	if (test && (rv&2)) rv &= ~1;
	close(fd);
	return rv;
}
