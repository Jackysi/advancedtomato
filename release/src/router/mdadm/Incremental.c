/*
 * Incremental.c - support --incremental.  Part of:
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2006 Neil Brown <neilb@suse.de>
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
 *    Email: <neilb@suse.de>
 *    Paper: Neil Brown
 *           Novell Inc
 *           GPO Box Q1283
 *           QVB Post Office, NSW 1230
 *           Australia
 */

#include	"mdadm.h"

static int count_active(struct supertype *st, int mdfd, char **availp,
			struct mdinfo *info);
static void find_reject(int mdfd, struct supertype *st, struct sysarray *sra,
			int number, __u64 events, int verbose,
			char *array_name);

int Incremental(char *devname, int verbose, int runstop,
		struct supertype *st, char *homehost, int autof)
{
	/* Add this device to an array, creating the array if necessary
	 * and starting the array if sensibe or - if runstop>0 - if possible.
	 *
	 * This has several steps:
	 *
	 * 1/ Check if device is permitted by mdadm.conf, reject if not.
	 * 2/ Find metadata, reject if none appropriate (check
	 *       version/name from args)
	 * 3/ Check if there is a match in mdadm.conf
	 * 3a/ if not, check for homehost match.  If no match, reject.
	 * 4/ Determine device number.
	 * - If in mdadm.conf with std name, use that
	 * - UUID in /var/run/mdadm.map  use that
	 * - If name is suggestive, use that. unless in use with different uuid.
	 * - Choose a free, high number.
	 * - Use a partitioned device unless strong suggestion not to.
	 *         e.g. auto=md
	 * 5/ Find out if array already exists
	 * 5a/ if it does not
	 * - choose a name, from mdadm.conf or 'name' field in array.
	 * - create the array
	 * - add the device
	 * 5b/ if it does
	 * - check one drive in array to make sure metadata is a reasonably
	 *       close match.  Reject if not (e.g. different type)
	 * - add the device
	 * 6/ Make sure /var/run/mdadm.map contains this array.
	 * 7/ Is there enough devices to possibly start the array?
	 * 7a/ if not, finish with success.
	 * 7b/ if yes,
	 * - read all metadata and arrange devices like -A does
	 * - if number of OK devices match expected, or -R and there are enough,
	 *   start the array (auto-readonly).
	 */
	struct stat stb;
	void *super, *super2;
	struct mdinfo info, info2;
	struct mddev_ident_s *array_list, *match;
	char chosen_name[1024];
	int rv;
	int devnum;
	struct map_ent *mp, *map = NULL;
	int dfd, mdfd;
	char *avail;
	int active_disks;


	struct createinfo *ci = conf_get_create_info();

	if (autof == 0)
		autof = ci->autof;

	/* 1/ Check if devices is permitted by mdadm.conf */

	if (!conf_test_dev(devname)) {
		if (verbose >= 0)
			fprintf(stderr, Name
				": %s not permitted by mdadm.conf.\n",
				devname);
		return 1;
	}

	/* 2/ Find metadata, reject if none appropriate (check
	 *            version/name from args) */

	dfd = dev_open(devname, O_RDONLY|O_EXCL);
	if (dfd < 0) {
		if (verbose >= 0)
			fprintf(stderr, Name ": cannot open %s: %s.\n",
				devname, strerror(errno));
		return 1;
	}
	if (fstat(dfd, &stb) < 0) {
		if (verbose >= 0)
			fprintf(stderr, Name ": fstat failed for %s: %s.\n",
				devname, strerror(errno));
		close(dfd);
		return 1;
	}
	if ((stb.st_mode & S_IFMT) != S_IFBLK) {
		if (verbose >= 0)
			fprintf(stderr, Name ": %s is not a block device.\n",
				devname);
		close(dfd);
		return 1;
	}

	if (st == NULL && (st = guess_super(dfd)) == NULL) {
		if (verbose >= 0)
			fprintf(stderr, Name
				": no recognisable superblock on %s.\n",
				devname);
		close(dfd);
		return 1;
	}
	if (st->ss->load_super(st, dfd, &super, NULL)) {
		if (verbose >= 0)
			fprintf(stderr, Name ": no RAID superblock on %s.\n",
				devname);
		close(dfd);
		return 1;
	}
	st->ss->getinfo_super(&info, super);
	close (dfd);

	/* 3/ Check if there is a match in mdadm.conf */

	array_list = conf_get_ident(NULL);
	match = NULL;
	for (; array_list; array_list = array_list->next) {
		if (array_list->uuid_set &&
		    same_uuid(array_list->uuid, info.uuid, st->ss->swapuuid)
		    == 0) {
			if (verbose >= 2)
				fprintf(stderr, Name
					": UUID differs from %s.\n",
					array_list->devname);
			continue;
		}
		if (array_list->name[0] &&
		    strcasecmp(array_list->name, info.name) != 0) {
			if (verbose >= 2)
				fprintf(stderr, Name
					": Name differs from %s.\n",
					array_list->devname);
			continue;
		}
		if (array_list->devices &&
		    !match_oneof(array_list->devices, devname)) {
			if (verbose >= 2)
				fprintf(stderr, Name
					": Not a listed device for %s.\n",
					array_list->devname);
			continue;
		}
		if (array_list->super_minor != UnSet &&
		    array_list->super_minor != info.array.md_minor) {
			if (verbose >= 2)
				fprintf(stderr, Name
					": Different super-minor to %s.\n",
					array_list->devname);
			continue;
		}
		if (!array_list->uuid_set &&
		    !array_list->name[0] &&
		    !array_list->devices &&
		    array_list->super_minor == UnSet) {
			if (verbose  >= 2)
				fprintf(stderr, Name
			     ": %s doesn't have any identifying information.\n",
					array_list->devname);
			continue;
		}
		/* FIXME, should I check raid_disks and level too?? */

		if (match) {
			if (verbose >= 0)
				fprintf(stderr, Name
		   ": we match both %s and %s - cannot decide which to use.\n",
					match->devname, array_list->devname);
			return 2;
		}
		match = array_list;
	}

	/* 3a/ if not, check for homehost match.  If no match, reject. */
	if (!match) {
		if (homehost == NULL ||
		    st->ss->match_home(super, homehost) == 0) {
			if (verbose >= 0)
				fprintf(stderr, Name
	      ": not found in mdadm.conf and not identified by homehost.\n");
			return 2;
		}
	}
	/* 4/ Determine device number. */
	/* - If in mdadm.conf with std name, use that */
	/* - UUID in /var/run/mdadm.map  use that */
	/* - If name is suggestive, use that. unless in use with */
	/*           different uuid. */
	/* - Choose a free, high number. */
	/* - Use a partitioned device unless strong suggestion not to. */
	/*         e.g. auto=md */
	if (match && is_standard(match->devname, &devnum))
		/* We have devnum now */;
	else if ((mp = map_by_uuid(&map, info.uuid)) != NULL)
		devnum = mp->devnum;
	else {
		/* Have to guess a bit. */
		int use_partitions = 1;
		char *np, *ep;
		if ((autof&7) == 3 || (autof&7) == 5)
			use_partitions = 0;
		np = strchr(info.name, ':');
		if (np)
			np++;
		else
			np = info.name;
		devnum = strtoul(np, &ep, 10);
		if (ep > np && *ep == 0) {
			/* This is a number.  Let check that it is unused. */
			if (mddev_busy(use_partitions ? (-1-devnum) : devnum))
				devnum = -1;
		} else
			devnum = -1;

		if (devnum < 0) {
			/* Haven't found anything yet, choose something free */
			/* There is similar code in mdopen.c - should unify */
			for (devnum = 127 ; devnum != 128 ;
			     devnum = devnum ? devnum-1 : (1<<22)-1) {
				if (mddev_busy(use_partitions ?
					       (-1-devnum) : devnum))
					break;
			}
			if (devnum == 128) {
				fprintf(stderr, Name
					": No spare md devices!!\n");
				return 2;
			}
		}
		devnum = use_partitions ? (-1-devnum) : devnum;
	}
	mdfd = open_mddev_devnum(match ? match->devname : NULL,
				 devnum,
				 info.name,
				 chosen_name);
	if (mdfd < 0) {
		fprintf(stderr, Name ": failed to open %s: %s.\n",
			chosen_name, strerror(errno));
		return 2;
	}
	/* 5/ Find out if array already exists */
	if (! mddev_busy(devnum)) {
	/* 5a/ if it does not */
	/* - choose a name, from mdadm.conf or 'name' field in array. */
	/* - create the array */
	/* - add the device */
		mdu_array_info_t ainf;
		mdu_disk_info_t disk;
		char md[20];
		struct sysarray *sra;

		memset(&ainf, 0, sizeof(ainf));
		ainf.major_version = st->ss->major;
		ainf.minor_version = st->minor_version;
		if (ioctl(mdfd, SET_ARRAY_INFO, &ainf) != 0) {
			fprintf(stderr, Name
				": SET_ARRAY_INFO failed for %s: %s\b",
				chosen_name, strerror(errno));
			close(mdfd);
			return 2;
		}
		sprintf(md, "%d.%d\n", st->ss->major, st->minor_version);
		sra = sysfs_read(mdfd, devnum, GET_VERSION);
		sysfs_set_str(sra, NULL, "metadata_version", md);
		memset(&disk, 0, sizeof(disk));
		disk.major = major(stb.st_rdev);
		disk.minor = minor(stb.st_rdev);
		sysfs_free(sra);
		if (ioctl(mdfd, ADD_NEW_DISK, &disk) != 0) {
			fprintf(stderr, Name ": failed to add %s to %s: %s.\n",
				devname, chosen_name, strerror(errno));
			ioctl(mdfd, STOP_ARRAY, 0);
			close(mdfd);
			return 2;
		}
		sra = sysfs_read(mdfd, devnum, GET_DEVS);
		if (!sra || !sra->devs || sra->devs->role >= 0) {
			/* It really should be 'none' - must be old buggy
			 * kernel, and mdadm -I may not be able to complete.
			 * So reject it.
			 */
			ioctl(mdfd, STOP_ARRAY, NULL);
			fprintf(stderr, Name
		      ": You have an old buggy kernel which cannot support\n"
				"      --incremental reliably.  Aborting.\n");
			close(mdfd);
			sysfs_free(sra);
			return 2;
		}
	} else {
	/* 5b/ if it does */
	/* - check one drive in array to make sure metadata is a reasonably */
	/*        close match.  Reject if not (e.g. different type) */
	/* - add the device */
		char dn[20];
		int dfd2;
		mdu_disk_info_t disk;
		int err;
		struct sysarray *sra;
		sra = sysfs_read(mdfd, devnum, (GET_VERSION | GET_DEVS |
						GET_STATE));
		if (sra->major_version != st->ss->major ||
		    sra->minor_version != st->minor_version) {
			if (verbose >= 0)
				fprintf(stderr, Name
	      ": %s has different metadata to chosen array %s %d.%d %d.%d.\n",
					devname, chosen_name,
					sra->major_version, sra->minor_version,
					st->ss->major, st->minor_version);
			close(mdfd);
			return 1;
		}
		sprintf(dn, "%d:%d", sra->devs->major, sra->devs->minor);
		dfd2 = dev_open(dn, O_RDONLY);
		if (st->ss->load_super(st, dfd2,&super2, NULL)) {
			fprintf(stderr, Name
				": Strange error loading metadata for %s.\n",
				chosen_name);
			close(mdfd);
			close(dfd2);
			return 2;
		}
		close(dfd2);
		st->ss->getinfo_super(&info2, super2);
		if (info.array.level != info2.array.level ||
		    memcmp(info.uuid, info2.uuid, 16) != 0 ||
		    info.array.raid_disks != info2.array.raid_disks) {
			fprintf(stderr, Name
				": unexpected difference between %s and %s.\n",
				chosen_name, devname);
			close(mdfd);
			return 2;
		}
		memset(&disk, 0, sizeof(disk));
		disk.major = major(stb.st_rdev);
		disk.minor = minor(stb.st_rdev);
		err = ioctl(mdfd, ADD_NEW_DISK, &disk);
		if (err < 0 && errno == EBUSY) {
			/* could be another device present with the same
			 * disk.number. Find and reject any such
			 */
			find_reject(mdfd, st, sra, info.disk.number,
				    info.events, verbose, chosen_name);
			err = ioctl(mdfd, ADD_NEW_DISK, &disk);
		}
		if (err < 0) {
			fprintf(stderr, Name ": failed to add %s to %s: %s.\n",
				devname, chosen_name, strerror(errno));
			close(mdfd);
			return 2;
		}
	}
	/* 6/ Make sure /var/run/mdadm.map contains this array. */
	map_update(&map, devnum,
		   info.array.major_version,
		   info.array.minor_version,
		   info.uuid, chosen_name);

	/* 7/ Is there enough devices to possibly start the array? */
	/* 7a/ if not, finish with success. */
	avail = NULL;
	active_disks = count_active(st, mdfd, &avail, &info);
	if (enough(info.array.level, info.array.raid_disks,
		   info.array.layout, info.array.state & 1,
		   avail, active_disks) == 0) {
		free(avail);
		if (verbose >= 0)
			fprintf(stderr, Name
			     ": %s attached to %s, not enough to start (%d).\n",
				devname, chosen_name, active_disks);
		close(mdfd);
		return 0;
	}
	free(avail);

	/* 7b/ if yes, */
	/* - if number of OK devices match expected, or -R and there */
	/*             are enough, */
	/*   + add any bitmap file  */
	/*   + start the array (auto-readonly). */
{
	mdu_array_info_t ainf;

	if (ioctl(mdfd, GET_ARRAY_INFO, &ainf) == 0) {
		if (verbose >= 0)
			fprintf(stderr, Name
			   ": %s attached to %s which is already active.\n",
				devname, chosen_name);
		close (mdfd);
		return 0;
	}
}
	if (runstop > 0 || active_disks >= info.array.working_disks) {
		struct sysarray *sra;
		/* Let's try to start it */
		if (match && match->bitmap_file) {
			int bmfd = open(match->bitmap_file, O_RDWR);
			if (bmfd < 0) {
				fprintf(stderr, Name
					": Could not open bitmap file %s.\n",
					match->bitmap_file);
				close(mdfd);
				return 1;
			}
			if (ioctl(mdfd, SET_BITMAP_FILE, bmfd) != 0) {
				close(bmfd);
				fprintf(stderr, Name
					": Failed to set bitmapfile for %s.\n",
					chosen_name);
				close(mdfd);
				return 1;
			}
			close(bmfd);
		}
		sra = sysfs_read(mdfd, devnum, 0);
		if (sra == NULL || active_disks >= info.array.working_disks)
			rv = ioctl(mdfd, RUN_ARRAY, NULL);
		else
			rv = sysfs_set_str(sra, NULL,
					   "array_state", "read-auto");
		if (rv == 0) {
			if (verbose >= 0)
				fprintf(stderr, Name
			   ": %s attached to %s, which has been started.\n",
					devname, chosen_name);
			rv = 0;
		} else {
			fprintf(stderr, Name
                             ": %s attached to %s, but failed to start: %s.\n",
				devname, chosen_name, strerror(errno));
			rv = 1;
		}
	} else {
		if (verbose >= 0)
			fprintf(stderr, Name
                          ": %s attached to %s, not enough to start safely.\n",
				devname, chosen_name);
		rv = 0;
	}
	close(mdfd);
	return rv;
}

static void find_reject(int mdfd, struct supertype *st, struct sysarray *sra,
			int number, __u64 events, int verbose,
			char *array_name)
{
	/* Find an device attached to this array with a disk.number of number
	 * and events less than the passed events, and remove the device.
	 */
	struct sysdev *d;
	mdu_array_info_t ra;

	if (ioctl(mdfd, GET_ARRAY_INFO, &ra) == 0)
		return; /* not safe to remove from active arrays
			 * without thinking more */

	for (d = sra->devs; d ; d = d->next) {
		char dn[10];
		int dfd;
		void *super;
		struct mdinfo info;
		sprintf(dn, "%d:%d", d->major, d->minor);
		dfd = dev_open(dn, O_RDONLY);
		if (dfd < 0)
			continue;
		if (st->ss->load_super(st, dfd, &super, NULL)) {
			close(dfd);
			continue;
		}
		st->ss->getinfo_super(&info, super);
		free(super);
		close(dfd);

		if (info.disk.number != number ||
		    info.events >= events)
			continue;

		if (d->role > -1)
			sysfs_set_str(sra, d, "slot", "none");
		if (sysfs_set_str(sra, d, "state", "remove") == 0)
			if (verbose >= 0)
				fprintf(stderr, Name
					": removing old device %s from %s\n",
					d->name+4, array_name);
	}
}

static int count_active(struct supertype *st, int mdfd, char **availp,
			struct mdinfo *bestinfo)
{
	/* count how many devices in sra think they are active */
	struct sysdev *d;
	int cnt = 0, cnt1 = 0;
	__u64 max_events = 0;
	void *best_super = NULL;
	struct sysarray *sra = sysfs_read(mdfd, -1, GET_DEVS | GET_STATE);
	char *avail = NULL;

	for (d = sra->devs ; d ; d = d->next) {
		char dn[30];
		int dfd;
		void *super;
		int ok;
		struct mdinfo info;

		sprintf(dn, "%d:%d", d->major, d->minor);
		dfd = dev_open(dn, O_RDONLY);
		if (dfd < 0)
			continue;
		ok =  st->ss->load_super(st, dfd, &super, NULL);
		close(dfd);
		if (ok != 0)
			continue;
		st->ss->getinfo_super(&info, super);
		if (info.disk.state & (1<<MD_DISK_SYNC))
		{
			if (avail == NULL) {
				avail = malloc(info.array.raid_disks);
				memset(avail, 0, info.array.raid_disks);
			}
			if (cnt == 0) {
				cnt++;
				max_events = info.events;
				avail[info.disk.raid_disk] = 2;
				best_super = super; super = NULL;
			} else if (info.events == max_events) {
				cnt++;
				avail[info.disk.raid_disk] = 2;
			} else if (info.events == max_events-1) {
				cnt1++;
				avail[info.disk.raid_disk] = 1;
			} else if (info.events < max_events - 1)
				;
			else if (info.events == max_events+1) {
				int i;
				cnt1 = cnt;
				cnt = 1;
				max_events = info.events;
				for (i=0; i<info.array.raid_disks; i++)
					if (avail[i])
						avail[i]--;
				avail[info.disk.raid_disk] = 2;
				free(best_super);
				best_super = super;
				super = NULL;
			} else { /* info.events much bigger */
				cnt = 1; cnt1 = 0;
				memset(avail, 0, info.disk.raid_disk);
				max_events = info.events;
				free(best_super);
				best_super = super;
				super = NULL;
			}
		}
		if (super)
			free(super);
	}
	if (best_super) {
		st->ss->getinfo_super(bestinfo,best_super);
		free(best_super);
	}
	return cnt + cnt1;
}

void RebuildMap(void)
{
	struct mdstat_ent *mdstat = mdstat_read(0, 0);
	struct mdstat_ent *md;
	struct map_ent *map = NULL;
	int mdp = get_mdp_major();

	for (md = mdstat ; md ; md = md->next) {
		struct sysarray *sra = sysfs_read(-1, md->devnum, GET_DEVS);
		struct sysdev *sd;

		for (sd = sra->devs ; sd ; sd = sd->next) {
			char dn[30];
			int dfd;
			int ok;
			struct supertype *st;
			char *path;
			void *super;
			struct mdinfo info;

			sprintf(dn, "%d:%d", sd->major, sd->minor);
			dfd = dev_open(dn, O_RDONLY);
			if (dfd < 0)
				continue;
			st = guess_super(dfd);
			if ( st == NULL)
				ok = -1;
			else
				ok = st->ss->load_super(st, dfd, &super, NULL);
			close(dfd);
			if (ok != 0)
				continue;
			st->ss->getinfo_super(&info, super);
			if (md->devnum > 0)
				path = map_dev(MD_MAJOR, md->devnum, 0);
			else
				path = map_dev(mdp, (-1-md->devnum)<< 6, 0);
			map_add(&map, md->devnum, st->ss->major,
				st->minor_version,
				info.uuid, path ? : "/unknown");
			free(super);
			break;
		}
	}
	map_write(map);
	map_free(map);
}

int IncrementalScan(int verbose)
{
	/* look at every device listed in the 'map' file.
	 * If one is found that is not running then:
	 *  look in mdadm.conf for bitmap file.
	 *   if one exists, but array has none, add it.
	 *  try to start array in auto-readonly mode
	 */
	struct map_ent *mapl = NULL;
	struct map_ent *me;
	mddev_ident_t devs, mddev;
	int rv = 0;

	map_read(&mapl);
	devs = conf_get_ident(NULL);

	for (me = mapl ; me ; me = me->next) {
		char path[1024];
		mdu_array_info_t array;
		mdu_bitmap_file_t bmf;
		struct sysarray *sra;
		int mdfd = open_mddev_devnum(me->path, me->devnum, NULL, path);
		if (mdfd < 0)
			continue;
		if (ioctl(mdfd, GET_ARRAY_INFO, &array) == 0 ||
		    errno != ENODEV) {
			close(mdfd);
			continue;
		}
		/* Ok, we can try this one.   Maybe it needs a bitmap */
		for (mddev = devs ; mddev ; mddev = mddev->next)
			if (strcmp(mddev->devname, me->path) == 0)
				break;
		if (mddev && mddev->bitmap_file) {
			/*
			 * Note: early kernels will wrongly fail this, so it
			 * is a hint only
			 */
			int added = -1;
			if (ioctl(mdfd, GET_ARRAY_INFO, &bmf) < 0) {
				int bmfd = open(mddev->bitmap_file, O_RDWR);
				if (bmfd >= 0) {
					added = ioctl(mdfd, SET_BITMAP_FILE,
						      bmfd);
					close(bmfd);
				}
			}
			if (verbose >= 0) {
				if (added == 0)
					fprintf(stderr, Name
						": Added bitmap %s to %s\n",
						mddev->bitmap_file, me->path);
				else if (errno != EEXIST)
					fprintf(stderr, Name
					   ": Failed to add bitmap to %s: %s\n",
						me->path, strerror(errno));
			}
		}
		sra = sysfs_read(mdfd, 0, 0);
		if (sra) {
			if (sysfs_set_str(sra, NULL,
					  "array_state", "read-auto") == 0) {
				if (verbose >= 0)
					fprintf(stderr, Name
						": started array %s\n",
						me->path);
			} else {
				fprintf(stderr, Name
					": failed to start array %s: %s\n",
					me->path, strerror(errno));
				rv = 1;
			}
		}
	}
	return rv;
}
