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

int Grow_Add_device(char *devname, int fd, char *newdev)
{
	/* Add a device to an active array.
	 * Currently, just extend a linear array.
	 * This requires writing a new superblock on the
	 * new device, calling the kernel to add the device,
	 * and if that succeeds, update the superblock on
	 * all other devices.
	 * This means that we need to *find* all other devices.
	 */
	struct mdinfo info;

	void *super = NULL;
	struct stat stb;
	int nfd, fd2;
	int d, nd;
	struct supertype *st = NULL;
	

	if (ioctl(fd, GET_ARRAY_INFO, &info.array) < 0) {
		fprintf(stderr, Name ": cannot get array info for %s\n", devname);
		return 1;
	}

	st = super_by_version(info.array.major_version, info.array.minor_version);
	if (!st) {
		fprintf(stderr, Name ": cannot handle arrays with superblock version %d\n", info.array.major_version);
		return 1;
	}

	if (info.array.level != -1) {
		fprintf(stderr, Name ": can only add devices to linear arrays\n");
		return 1;
	}

	nfd = open(newdev, O_RDWR|O_EXCL);
	if (nfd < 0) {
		fprintf(stderr, Name ": cannot open %s\n", newdev);
		return 1;
	}
	fstat(nfd, &stb);
	if ((stb.st_mode & S_IFMT) != S_IFBLK) {
		fprintf(stderr, Name ": %s is not a block device!\n", newdev);
		close(nfd);
		return 1;
	}
	/* now check out all the devices and make sure we can read the superblock */
	for (d=0 ; d < info.array.raid_disks ; d++) {
		mdu_disk_info_t disk;
		char *dv;

		disk.number = d;
		if (ioctl(fd, GET_DISK_INFO, &disk) < 0) {
			fprintf(stderr, Name ": cannot get device detail for device %d\n",
				d);
			return 1;
		}
		dv = map_dev(disk.major, disk.minor, 1);
		if (!dv) {
			fprintf(stderr, Name ": cannot find device file for device %d\n",
				d);
			return 1;
		}
		fd2 = dev_open(dv, O_RDWR);
		if (!fd2) {
			fprintf(stderr, Name ": cannot open device file %s\n", dv);
			return 1;
		}
		if (super) free(super);
		super= NULL;
		if (st->ss->load_super(st, fd2, &super, NULL)) {
			fprintf(stderr, Name ": cannot find super block on %s\n", dv);
			close(fd2);
			return 1;
		}
		close(fd2);
	}
	/* Ok, looks good. Lets update the superblock and write it out to
	 * newdev.
	 */
	
	info.disk.number = d;
	info.disk.major = major(stb.st_rdev);
	info.disk.minor = minor(stb.st_rdev);
	info.disk.raid_disk = d;
	info.disk.state = (1 << MD_DISK_SYNC) | (1 << MD_DISK_ACTIVE);
	st->ss->update_super(&info, super, "grow", newdev, 0, 0, NULL);

	if (st->ss->store_super(st, nfd, super)) {
		fprintf(stderr, Name ": Cannot store new superblock on %s\n", newdev);
		close(nfd);
		return 1;
	}
	close(nfd);

	if (ioctl(fd, ADD_NEW_DISK, &info.disk) != 0) {
		fprintf(stderr, Name ": Cannot add new disk to this array\n");
		return 1;
	}
	/* Well, that seems to have worked.
	 * Now go through and update all superblocks
	 */

	if (ioctl(fd, GET_ARRAY_INFO, &info.array) < 0) {
		fprintf(stderr, Name ": cannot get array info for %s\n", devname);
		return 1;
	}

	nd = d;
	for (d=0 ; d < info.array.raid_disks ; d++) {
		mdu_disk_info_t disk;
		char *dv;

		disk.number = d;
		if (ioctl(fd, GET_DISK_INFO, &disk) < 0) {
			fprintf(stderr, Name ": cannot get device detail for device %d\n",
				d);
			return 1;
		}
		dv = map_dev(disk.major, disk.minor, 1);
		if (!dv) {
			fprintf(stderr, Name ": cannot find device file for device %d\n",
				d);
			return 1;
		}
		fd2 = dev_open(dv, O_RDWR);
		if (fd2 < 0) {
			fprintf(stderr, Name ": cannot open device file %s\n", dv);
			return 1;
		}
		if (st->ss->load_super(st, fd2, &super, NULL)) {
			fprintf(stderr, Name ": cannot find super block on %s\n", dv);
			close(fd);
			return 1;
		}
		info.array.raid_disks = nd+1;
		info.array.nr_disks = nd+1;
		info.array.active_disks = nd+1;
		info.array.working_disks = nd+1;
		info.disk.number = nd;
		info.disk.major = major(stb.st_rdev);
		info.disk.minor = minor(stb.st_rdev);
		info.disk.raid_disk = nd;
		info.disk.state = (1 << MD_DISK_SYNC) | (1 << MD_DISK_ACTIVE);
		st->ss->update_super(&info, super, "grow", dv, 0, 0, NULL);
		
		if (st->ss->store_super(st, fd2, super)) {
			fprintf(stderr, Name ": Cannot store new superblock on %s\n", dv);
			close(fd2);
			return 1;
		}
		close(fd2);
	}

	return 0;
}

int Grow_addbitmap(char *devname, int fd, char *file, int chunk, int delay, int write_behind, int force)
{
	/*
	 * First check that array doesn't have a bitmap
	 * Then create the bitmap
	 * Then add it
	 *
	 * For internal bitmaps, we need to check the version,
	 * find all the active devices, and write the bitmap block
	 * to all devices
	 */
	mdu_bitmap_file_t bmf;
	mdu_array_info_t array;
	struct supertype *st;
	int major = BITMAP_MAJOR_HI;
	int vers = md_get_version(fd);
	unsigned long long bitmapsize, array_size;

	if (vers < 9003) {
		major = BITMAP_MAJOR_HOSTENDIAN;
#ifdef __BIG_ENDIAN
		fprintf(stderr, Name ": Warning - bitmaps created on this kernel are not portable\n"
			"  between different architectured.  Consider upgrading the Linux kernel.\n");
#endif
	}

	if (ioctl(fd, GET_BITMAP_FILE, &bmf) != 0) {
		if (errno == ENOMEM)
			fprintf(stderr, Name ": Memory allocation failure.\n");
		else
			fprintf(stderr, Name ": bitmaps not supported by this kernel.\n");
		return 1;
	}
	if (bmf.pathname[0]) {
		if (strcmp(file,"none")==0) {
			if (ioctl(fd, SET_BITMAP_FILE, -1)!= 0) {
				fprintf(stderr, Name ": failed to remove bitmap %s\n",
					bmf.pathname);
				return 1;
			}
			return 0;
		}
		fprintf(stderr, Name ": %s already has a bitmap (%s)\n",
			devname, bmf.pathname);
		return 1;
	}
	if (ioctl(fd, GET_ARRAY_INFO, &array) != 0) {
		fprintf(stderr, Name ": cannot get array status for %s\n", devname);
		return 1;
	}
	if (array.state & (1<<MD_SB_BITMAP_PRESENT)) {
		if (strcmp(file, "none")==0) {
			array.state &= ~(1<<MD_SB_BITMAP_PRESENT);
			if (ioctl(fd, SET_ARRAY_INFO, &array)!= 0) {
				fprintf(stderr, Name ": failed to remove internal bitmap.\n");
				return 1;
			}
			return 0;
		}
		fprintf(stderr, Name ": Internal bitmap already present on %s\n",
			devname);
		return 1;
	}
	bitmapsize = array.size;
	bitmapsize <<= 1;
	if (get_dev_size(fd, NULL, &array_size) &&
	    array_size > (0x7fffffffULL<<9)) {
		/* Array is big enough that we cannot trust array.size
		 * try other approaches
		 */
		bitmapsize = get_component_size(fd);
	}
	if (bitmapsize == 0) {
		fprintf(stderr, Name ": Cannot reliably determine size of array to create bitmap - sorry.\n");
		return 1;
	}

	if (array.level == 10) {
		int ncopies = (array.layout&255)*((array.layout>>8)&255);
		bitmapsize = bitmapsize * array.raid_disks / ncopies;
	}

	st = super_by_version(array.major_version, array.minor_version);
	if (!st) {
		fprintf(stderr, Name ": Cannot understand version %d.%d\n",
			array.major_version, array.minor_version);
		return 1;
	}
	if (strcmp(file, "none") == 0) {
		fprintf(stderr, Name ": no bitmap found on %s\n", devname);
		return 1;
	} else if (strcmp(file, "internal") == 0) {
		int d;
		for (d=0; d< st->max_devs; d++) {
			mdu_disk_info_t disk;
			char *dv;
			disk.number = d;
			if (ioctl(fd, GET_DISK_INFO, &disk) < 0)
				continue;
			if (disk.major == 0 &&
			    disk.minor == 0)
				continue;
			if ((disk.state & (1<<MD_DISK_SYNC))==0)
				continue;
			dv = map_dev(disk.major, disk.minor, 1);
			if (dv) {
				void *super;
				int fd2 = dev_open(dv, O_RDWR);
				if (fd2 < 0)
					continue;
				if (st->ss->load_super(st, fd2, &super, NULL)==0) {
					if (st->ss->add_internal_bitmap(
						    st, super,
						    &chunk, delay, write_behind,
						    bitmapsize, 0, major)
						)
						st->ss->write_bitmap(st, fd2, super);
					else {
						fprintf(stderr, Name ": failed to create internal bitmap - chunksize problem.\n");
						close(fd2);
						return 1;
					}
				}
				close(fd2);
			}
		}
		array.state |= (1<<MD_SB_BITMAP_PRESENT);
		if (ioctl(fd, SET_ARRAY_INFO, &array)!= 0) {
			fprintf(stderr, Name ": failed to set internal bitmap.\n");
			return 1;
		}
	} else {
		int uuid[4];
		int bitmap_fd;
		int d;
		int max_devs = st->max_devs;
		void *super = NULL;

		/* try to load a superblock */
		for (d=0; d<max_devs; d++) {
			mdu_disk_info_t disk;
			char *dv;
			int fd2;
			disk.number = d;
			if (ioctl(fd, GET_DISK_INFO, &disk) < 0)
				continue;
			if ((disk.major==0 && disk.minor==0) ||
			    (disk.state & (1<<MD_DISK_REMOVED)))
				continue;
			dv = map_dev(disk.major, disk.minor, 1);
			if (!dv) continue;
			fd2 = dev_open(dv, O_RDONLY);
			if (fd2 >= 0 &&
			    st->ss->load_super(st, fd2, &super, NULL) == 0) {
				close(fd2);
				st->ss->uuid_from_super(uuid, super);
				break;
			}
			close(fd2);
		}
		if (d == max_devs) {
			fprintf(stderr, Name ": cannot find UUID for array!\n");
			return 1;
		}
		if (CreateBitmap(file, force, (char*)uuid, chunk,
				 delay, write_behind, bitmapsize, major)) {
			return 1;
		}
		bitmap_fd = open(file, O_RDWR);
		if (bitmap_fd < 0) {
			fprintf(stderr, Name ": weird: %s cannot be opened\n",
				file);
			return 1;
		}
		if (ioctl(fd, SET_BITMAP_FILE, bitmap_fd) < 0) {
			fprintf(stderr, Name ": Cannot set bitmap file for %s: %s\n",
				devname, strerror(errno));
			return 1;
		}
	}

	return 0;
}


/*
 * When reshaping an array we might need to backup some data.
 * This is written to all spares with a 'super_block' describing it.
 * The superblock goes 1K form the end of the used space on the
 * device.
 * It if written after the backup is complete.
 * It has the following structure.
 */

struct mdp_backup_super {
	char	magic[16];  /* md_backup_data-1 */
	__u8	set_uuid[16];
	__u64	mtime;
	/* start/sizes in 512byte sectors */
	__u64	devstart;
	__u64	arraystart;
	__u64	length;
	__u32	sb_csum;	/* csum of preceeding bytes. */
};

int bsb_csum(char *buf, int len)
{
	int i;
	int csum = 0;
	for (i=0; i<len; i++)
		csum = (csum<<3) + buf[0];
	return __cpu_to_le32(csum);
}

int Grow_reshape(char *devname, int fd, int quiet, char *backup_file,
		 long long size,
		 int level, int layout, int chunksize, int raid_disks)
{
	/* Make some changes in the shape of an array.
	 * The kernel must support the change.
	 * Different reshapes have subtly different meaning for different
	 * levels, so we need to check the current state of the array
	 * and go from there.
	 */
	struct mdu_array_info_s array;
	char *c;

	struct mdp_backup_super bsb;
	struct supertype *st;

	int nlevel, olevel;
	int nchunk, ochunk;
	int nlayout, olayout;
	int ndisks, odisks;
	int ndata, odata;
	unsigned long long nstripe, ostripe, last_block;
	int *fdlist;
	unsigned long long *offsets;
	int d, i, spares;
	int nrdisks;
	int err;
	void *super = NULL;

	struct sysarray *sra;
	struct sysdev *sd;

	if (ioctl(fd, GET_ARRAY_INFO, &array) < 0) {
		fprintf(stderr, Name ": %s is not an active md array - aborting\n",
			devname);
		return 1;
	}
	c = map_num(pers, array.level);
	if (c == NULL) c = "-unknown-";
	switch(array.level) {
	default: /* raid0, linear, multipath cannot be reconfigured */
		fprintf(stderr, Name ": %s array %s cannot be reshaped.\n",
			c, devname);
		return 1;

	case LEVEL_FAULTY: /* only 'layout' change is permitted */

		if (size >= 0) {
			fprintf(stderr, Name ": %s: Cannot change size of a 'faulty' array\n",
				devname);
			return 1;
		}
		if (level != UnSet && level != LEVEL_FAULTY) {
			fprintf(stderr, Name ": %s: Cannot change RAID level of a 'faulty' array\n",
				devname);
			return 1;
		}
		if (chunksize  || raid_disks) {
			fprintf(stderr, Name ": %s: Cannot change chunksize or disks of a 'faulty' array\n",
				devname);
			return 1;
		}
		if (layout == UnSet)
			return 0; /* nothing to do.... */

		array.layout = layout;
		if (ioctl(fd, SET_ARRAY_INFO, &array) != 0) {
			fprintf(stderr, Name ": Cannot set layout for %s: %s\n",
				devname, strerror(errno));
			return 1;
		}
		if (!quiet)
			printf("layout for %s set to %d\n", devname, array.layout);
		return 0;

	case 1: /* raid_disks and size can each be changed.  They are independant */

		if (level != UnSet && level != 1) {
			fprintf(stderr, Name ": %s: Cannot change RAID level of a RAID1 array.\n",
				devname);
			return 1;
		}
		if (chunksize || layout != UnSet) {
			fprintf(stderr, Name ": %s: Cannot change chunk size of layout for a RAID1 array.\n",
				devname);
			return 1;
		}

		/* Each can trigger a resync/recovery which will block the
		 * other from happening.  Later we could block
		 * resync for the duration via 'sync_action'...
		 */
		if (raid_disks > 0) {
			array.raid_disks = raid_disks;
			if (ioctl(fd, SET_ARRAY_INFO, &array) != 0) {
				fprintf(stderr, Name ": Cannot set raid-devices for %s: %s\n",
					devname, strerror(errno));
				return 1;
			}
		}
		if (size >= 0) {
			array.size = size;
			if (ioctl(fd, SET_ARRAY_INFO, &array) != 0) {
				fprintf(stderr, Name ": Cannot set device size for %s: %s\n",
					devname, strerror(errno));
				return 1;
			}
		}
		return 0;

	case 4:
	case 5:
	case 6:
		st = super_by_version(array.major_version,
				      array.minor_version);
		/* size can be changed independently.
		 * layout/chunksize/raid_disks/level can be changed
		 * though the kernel may not support it all.
		 * If 'suspend_lo' is not present in devfs, then
		 * these cannot be changed.
		 */
		if (size >= 0) {
			/* Cannot change other details as well.. */
			if (layout != UnSet ||
			    chunksize != 0 ||
			    raid_disks != 0 ||
			    level != UnSet) {
				fprintf(stderr, Name ": %s: Cannot change shape as well as size of a %s array.\n",
					devname, c);
				return 1;
			}
			array.size = size;
			if (ioctl(fd, SET_ARRAY_INFO, &array) != 0) {
				fprintf(stderr, Name ": Cannot set device size/shape for %s: %s\n",
					devname, strerror(errno));
				return 1;
			}
			return 0;
		}
		/* Ok, just change the shape. This can be awkward.
		 *  There are three possibilities.
		 * 1/ The array will shrink.  We don't support this
		 *    possibility.  Maybe one day...
		 * 2/ The array will not change size.  This is easy enough
		 *    to do, but not reliably.  If the process is aborted
		 *    the array *will* be corrupted.  So maybe we can allow
		 *    this but only if the user is really certain.  e.g.
		 *    --really-risk-everything
		 * 3/ The array will grow. This can be reliably achieved.
		 *    However the kernel's restripe routines will cheerfully
		 *    overwrite some early data before it is safe.  So we
		 *    need to make a backup of the early parts of the array
		 *    and be ready to restore it if rebuild aborts very early.
		 *
		 *    We backup data by writing it to all spares (there must be
		 *    at least 1, so even raid6->raid5 requires a spare to be
		 *    present).
		 *
		 *    So: we enumerate the devices in the array and
		 *    make sure we can open all of them.
		 *    Then we freeze the early part of the array and
		 *    backup to the various spares.
		 *    Then we request changes and start the reshape.
		 *    Monitor progress until it has passed the danger zone.
		 *    and finally invalidate the copied data and unfreeze the
		 *    start of the array.
		 *
		 *    Before we can do this we need to decide:
		 *     - will the array grow?  Just calculate size
		 *     - how much needs to be saved: count stripes.
		 *     - where to save data... good question.
		 *
		 */
		nlevel = olevel = array.level;
		nchunk = ochunk = array.chunk_size;
		nlayout = olayout = array.layout;
		ndisks = odisks = array.raid_disks;

		if (level != UnSet) nlevel = level;
		if (chunksize) nchunk = chunksize;
		if (layout != UnSet) nlayout = layout;
		if (raid_disks) ndisks = raid_disks;

		odata = odisks-1;
		if (olevel == 6) odata--; /* number of data disks */
		ndata = ndisks-1;
		if (nlevel == 6) ndata--;

		if (ndata < odata) {
			fprintf(stderr, Name ": %s: Cannot reduce number of data disks (yet).\n",
				devname);
			return 1;
		}
		if (ndata == odata) {
			fprintf(stderr, Name ": %s: Cannot reshape array without increasing size (yet).\n",
				devname);
			return 1;
		}
		/* Well, it is growing... so how much do we need to backup.
		 * Need to backup a full number of new-stripes, such that the
		 * last one does not over-write any place that it would be read
		 * from
		 */
		nstripe = ostripe = 0;
		while (nstripe >= ostripe) {
			nstripe += nchunk/512;
			last_block = nstripe * ndata;
			ostripe = last_block / odata / (ochunk/512) * (ochunk/512);
		}
		printf("mdadm: Need to backup %lluK of critical section..\n", last_block/2);

		sra = sysfs_read(fd, 0,
				 GET_COMPONENT|GET_DEVS|GET_OFFSET|GET_STATE|
				 GET_CACHE);
		if (!sra) {
			fprintf(stderr, Name ": %s: Cannot get array details from sysfs\n",
				devname);
			return 1;
		}

		if (last_block >= sra->component_size/2) {
			fprintf(stderr, Name ": %s: Something wrong - reshape aborted\n",
				devname);
			return 1;
		}
		if (sra->spares == 0 && backup_file == NULL) {
			fprintf(stderr, Name ": %s: Cannot grow - need a spare or backup-file to backup critical section\n",
				devname);
			return 1;
		}

		nrdisks = array.nr_disks + sra->spares;
		/* Now we need to open all these devices so we can read/write.
		 */
		fdlist = malloc((1+nrdisks) * sizeof(int));
		offsets = malloc((1+nrdisks) * sizeof(offsets[0]));
		if (!fdlist || !offsets) {
			fprintf(stderr, Name ": malloc failed: grow aborted\n");
			return 1;
		}
		for (d=0; d <= nrdisks; d++)
			fdlist[d] = -1;
		d = array.raid_disks;
		for (sd = sra->devs; sd; sd=sd->next) {
			if (sd->state & (1<<MD_DISK_FAULTY))
				continue;
			if (sd->state & (1<<MD_DISK_SYNC)) {
				char *dn = map_dev(sd->major, sd->minor, 1);
				fdlist[sd->role] = dev_open(dn, O_RDONLY);
				offsets[sd->role] = sd->offset;
				if (fdlist[sd->role] < 0) {
					fprintf(stderr, Name ": %s: cannot open component %s\n",
						devname, dn?dn:"-unknown-");
					goto abort;
				}
			} else {
				/* spare */
				char *dn = map_dev(sd->major, sd->minor, 1);
				fdlist[d] = dev_open(dn, O_RDWR);
				offsets[d] = sd->offset;
				if (fdlist[d]<0) {
					fprintf(stderr, Name ": %s: cannot open component %s\n",
						devname, dn?dn:"-unknown");
					goto abort;
				}
				d++;
			}
		}
		for (i=0 ; i<array.raid_disks; i++)
			if (fdlist[i] < 0) {
				fprintf(stderr, Name ": %s: failed to find device %d. Array might be degraded.\n"
					" --grow aborted\n", devname, i);
				goto abort;
			}
		spares = sra->spares;
		if (backup_file) {
			fdlist[d] = open(backup_file, O_RDWR|O_CREAT|O_EXCL, 0600);
			if (fdlist[d] < 0) {
				fprintf(stderr, Name ": %s: cannot create backup file %s: %s\n",
					devname, backup_file, strerror(errno));
				goto abort;
			}
			offsets[d] = 8;
			d++;
			spares++;
		}
		if (fdlist[array.raid_disks] < 0) {
			fprintf(stderr, Name ": %s: failed to find a spare and no backup-file given - --grow aborted\n",
				devname);
			goto abort;
		}

		/* Find a superblock */
		if (st->ss->load_super(st, fdlist[0], &super, NULL)) {
			fprintf(stderr, Name ": %s: Cannot find a superblock\n",
				devname);
			goto abort;
		}


		memcpy(bsb.magic, "md_backup_data-1", 16);
		st->ss->uuid_from_super((int*)&bsb.set_uuid, super);
		bsb.mtime = __cpu_to_le64(time(0));
		bsb.arraystart = 0;
		bsb.length = __cpu_to_le64(last_block);

		/* Decide offset for the backup, llseek the spares, and write
		 * a leading superblock 4K earlier.
		 */
		for (i=array.raid_disks; i<d; i++) {
			char buf[4096];
			if (i==d-1 && backup_file) {
				/* This is the backup file */
				offsets[i] = 8;
			} else
				offsets[i] += sra->component_size - last_block - 8;
			if (lseek64(fdlist[i], (offsets[i]<<9) - 4096, 0)
			    != (offsets[i]<<9) - 4096) {
				fprintf(stderr, Name ": could not seek...\n");
				goto abort;
			}
			memset(buf, 0, sizeof(buf));
			bsb.devstart = __cpu_to_le64(offsets[i]);
			bsb.sb_csum = bsb_csum((char*)&bsb, ((char*)&bsb.sb_csum)-((char*)&bsb));
			memcpy(buf, &bsb, sizeof(bsb));
			if (write(fdlist[i], buf, 4096) != 4096) {
				fprintf(stderr, Name ": could not write leading superblock\n");
				goto abort;
			}
		}
		array.level = nlevel;
		array.raid_disks = ndisks;
		array.chunk_size = nchunk;
		array.layout = nlayout;
		if (ioctl(fd, SET_ARRAY_INFO, &array) != 0) {
			if (errno == ENOSPC) {
				/* stripe cache is not big enough.
				 * It needs to be 4 times chunksize_size,
				 * and we assume pagesize is 4K
				 */
				if (sra->cache_size < 4 * (nchunk/4096)) {
					sysfs_set_num(sra, NULL,
						      "stripe_cache_size",
						      4 * (nchunk/4096) +1);
					if (ioctl(fd, SET_ARRAY_INFO,
						  &array) == 0)
						goto ok;
				}
			}
			fprintf(stderr, Name ": Cannot set device size/shape for %s: %s\n",
				devname, strerror(errno));
			goto abort;
		}
		ok: ;

		/* suspend the relevant region */
		sysfs_set_num(sra, NULL, "suspend_hi", 0); /* just in case */
		if (sysfs_set_num(sra, NULL, "suspend_lo", 0) < 0 ||
		    sysfs_set_num(sra, NULL, "suspend_hi", last_block) < 0) {
			fprintf(stderr, Name ": %s: failed to suspend device.\n",
				devname);
			goto abort_resume;
		}


		err = save_stripes(fdlist, offsets,
				   odisks, ochunk, olevel, olayout,
				   spares, fdlist+odisks,
				   0ULL, last_block*512);

		/* abort if there was an error */
		if (err < 0) {
			fprintf(stderr, Name ": %s: failed to save critical region\n",
				devname);
			goto abort_resume;
		}

		for (i=odisks; i<d ; i++) {
			bsb.devstart = __cpu_to_le64(offsets[i]);
			bsb.sb_csum = bsb_csum((char*)&bsb, ((char*)&bsb.sb_csum)-((char*)&bsb));
			if (lseek64(fdlist[i], (offsets[i]+last_block)<<9, 0) < 0 ||
			    write(fdlist[i], &bsb, sizeof(bsb)) != sizeof(bsb) ||
			    fsync(fdlist[i]) != 0) {
				fprintf(stderr, Name ": %s: fail to save metadata for critical region backups.\n",
					devname);
				goto abort_resume;
			}
		}

		/* start the reshape happening */
		if (sysfs_set_str(sra, NULL, "sync_action", "reshape") < 0) {
			fprintf(stderr, Name ": %s: failed to initiate reshape\n",
				devname);
			goto abort_resume;
		}
		/* wait for reshape to pass the critical region */
		while(1) {
			unsigned long long comp;
			if (sysfs_get_ll(sra, NULL, "sync_completed", &comp)<0) {
				sleep(5);
				break;
			}
			if (comp >= nstripe)
				break;
			sleep(1);
		}
		
		/* invalidate superblocks */
		memset(&bsb, 0, sizeof(bsb));
		for (i=odisks; i<d ; i++) {
			lseek64(fdlist[i], (offsets[i]+last_block)<<9, 0);
			if (write(fdlist[i], &bsb, sizeof(bsb)) < 0) {
				fprintf(stderr, Name ": %s: failed to invalidate metadata for raid disk %d\n",
					devname, i);
			}
		}

		/* unsuspend. */
		sysfs_set_num(sra, NULL, "suspend_lo", last_block);

		for (i=0; i<d; i++)
			if (fdlist[i] >= 0)
				close(fdlist[i]);
		free(fdlist);
		free(offsets);
		if (backup_file)
			unlink(backup_file);

		printf(Name ": ... critical section passed.\n");
		break;
	}
	return 0;


 abort_resume:
	sysfs_set_num(sra, NULL, "suspend_lo", last_block);
 abort:
	for (i=0; i<array.nr_disks; i++)
		if (fdlist[i] >= 0)
			close(fdlist[i]);
	free(fdlist);
	free(offsets);
	if (backup_file)
		unlink(backup_file);
	return 1;

}

/*
 * If any spare contains md_back_data-1 which is recent wrt mtime,
 * write that data into the array and update the super blocks with
 * the new reshape_progress
 */
int Grow_restart(struct supertype *st, struct mdinfo *info, int *fdlist, int cnt, char *backup_file)
{
	int i, j;
	int old_disks;
	int err = 0;
	unsigned long long *offsets;

	if (info->delta_disks < 0)
		return 1; /* cannot handle a shrink */
	if (info->new_level != info->array.level ||
	    info->new_layout != info->array.layout ||
	    info->new_chunk != info->array.chunk_size)
		return 1; /* Can only handle change in disks */

	old_disks = info->array.raid_disks - info->delta_disks;

	for (i=old_disks-(backup_file?1:0); i<cnt; i++) {
		void *super = NULL;
		struct mdinfo dinfo;
		struct mdp_backup_super bsb;
		char buf[4096];
		int fd;

		/* This was a spare and may have some saved data on it.
		 * Load the superblock, find and load the
		 * backup_super_block.
		 * If either fail, go on to next device.
		 * If the backup contains no new info, just return
		 * else restore data and update all superblocks
		 */
		if (i == old_disks-1) {
			fd = open(backup_file, O_RDONLY);
			if (fd<0)
				continue;
		} else {
			fd = fdlist[i];
			if (fd < 0)
				continue;
			if (st->ss->load_super(st, fd, &super, NULL))
				continue;

			st->ss->getinfo_super(&dinfo, super);
			free(super); super = NULL;
			if (lseek64(fd,
				    (dinfo.data_offset + dinfo.component_size - 8) <<9,
				    0) < 0)
				continue; /* Cannot seek */
		}
		if (read(fd, &bsb, sizeof(bsb)) != sizeof(bsb))
			continue; /* Cannot read */
		if (memcmp(bsb.magic, "md_backup_data-1", 16) != 0)
			continue;
		if (bsb.sb_csum != bsb_csum((char*)&bsb, ((char*)&bsb.sb_csum)-((char*)&bsb)))
			continue; /* bad checksum */
		if (memcmp(bsb.set_uuid,info->uuid, 16) != 0)
			continue; /* Wrong uuid */

		if (info->array.utime > __le64_to_cpu(bsb.mtime) + 3600 ||
		    info->array.utime < __le64_to_cpu(bsb.mtime))
			continue; /* time stamp is too bad */

		if (__le64_to_cpu(bsb.arraystart) != 0)
			continue; /* Can only handle backup from start of array */
		if (__le64_to_cpu(bsb.length) <
		    info->reshape_progress)
			continue; /* No new data here */

		if (lseek64(fd, __le64_to_cpu(bsb.devstart)*512, 0)< 0)
			continue; /* Cannot seek */
		/* There should be a duplicate backup superblock 4k before here */
		if (lseek64(fd, -4096, 1) < 0 ||
		    read(fd, buf, 4096) != 4096 ||
		    memcmp(buf, &bsb, sizeof(bsb)) != 0)
			continue; /* Cannot find leading superblock */

		/* Now need the data offsets for all devices. */
		offsets = malloc(sizeof(*offsets)*info->array.raid_disks);
		for(j=0; j<info->array.raid_disks; j++) {
			if (fdlist[j] < 0)
				continue;
			if (st->ss->load_super(st, fdlist[j], &super, NULL))
				/* FIXME should be this be an error */
				continue;
			st->ss->getinfo_super(&dinfo, super);
			free(super); super = NULL;
			offsets[j] = dinfo.data_offset;
		}
		printf(Name ": restoring critical section\n");

		if (restore_stripes(fdlist, offsets,
				    info->array.raid_disks,
				    info->new_chunk,
				    info->new_level,
				    info->new_layout,
				    fd, __le64_to_cpu(bsb.devstart)*512,
				    0, __le64_to_cpu(bsb.length)*512)) {
			/* didn't succeed, so giveup */
			return -1;
		}

		/* Ok, so the data is restored. Let's update those superblocks. */

		for (j=0; j<info->array.raid_disks; j++) {
			if (fdlist[j] < 0) continue;
			if (st->ss->load_super(st, fdlist[j], &super, NULL))
				continue;
			st->ss->getinfo_super(&dinfo, super);
			dinfo.reshape_progress = __le64_to_cpu(bsb.length);
			st->ss->update_super(&dinfo, super, "_reshape_progress",NULL,0, 0, NULL);
			st->ss->store_super(st, fdlist[j], super);
			free(super);
		}

		/* And we are done! */
		return 0;
	}
	return err;
}
