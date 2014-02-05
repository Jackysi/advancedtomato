/*
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
 */

#include "mdadm.h"

/* To restripe, we read from old geometry to a buffer, and
 * read from buffer to new geometry.
 * When reading we don't worry about parity. When writing we do.
 *
 */

static int geo_map(int block, unsigned long long stripe, int raid_disks, int level, int layout)
{
	/* On the given stripe, find which disk in the array with have
	 * block numbered 'block'.
	 */
	int pd;

	switch(level*100 + layout) {
	case 000:
	case 400:
		/* raid 4 isn't messed around by parity blocks */
		if (block == -1)
			return raid_disks-1; /* parity block */
		return block;
	case 500 + ALGORITHM_LEFT_ASYMMETRIC:
		pd = (raid_disks-1) - stripe % raid_disks;
		if (block == -1) return pd;
		if (block >= pd)
			block++;
		return block;

	case 500 + ALGORITHM_RIGHT_ASYMMETRIC:
		pd = stripe % raid_disks;
		if (block == -1) return pd;
		if (block >= pd)
			block++;
		return block;

	case 500 + ALGORITHM_LEFT_SYMMETRIC:
		pd = (raid_disks - 1) - stripe % raid_disks;
		if (block == -1) return pd;
		return (pd + 1 + block) % raid_disks;

	case 500 + ALGORITHM_RIGHT_SYMMETRIC:
		pd = stripe % raid_disks;
		if (block == -1) return pd;
		return (pd + 1 + block) % raid_disks;

	case 600 + ALGORITHM_LEFT_ASYMMETRIC:
		pd = raid_disks - 1 - (stripe % raid_disks);
		if (block == -1) return pd;
		if (pd == raid_disks - 1)
			return block+1;
		if (block >= pd)
			return block+2;
		return block;

	case 600 + ALGORITHM_RIGHT_ASYMMETRIC:
		pd = stripe % raid_disks;
		if (block == -1) return pd;
		if (pd == raid_disks - 1)
			return block+1;
		if (block >= pd)
			return block+2;
		return block;

	case 600 + ALGORITHM_LEFT_SYMMETRIC:
		pd = raid_disks - 1 - (stripe % raid_disks);
		if (block == -1) return pd;
		return (pd + 2 + block) % raid_disks;

	case 600 + ALGORITHM_RIGHT_SYMMETRIC:
		pd = stripe % raid_disks;
		if (block == -1) return pd;
		return (pd + 2 + block) % raid_disks;
	}
	return -1;
}


static void xor_blocks(char *target, char **sources, int disks, int size)
{
	int i, j;
	/* Amazingly inefficient... */
	for (i=0; i<size; i++) {
		char c = 0;
		for (j=0 ; j<disks; j++)
			c ^= sources[j][i];
		target[i] = c;
	}
}

/* Save data:
 * We are given:
 *  A list of 'fds' of the active disks.  For now we require all to be present.
 *  A geomtry: raid_disks, chunk_size, level, layout
 *  A list of 'fds' for mirrored targets.  They are already seeked to
 *    right (Write) location
 *  A start and length
 */

int save_stripes(int *source, unsigned long long *offsets,
		 int raid_disks, int chunk_size, int level, int layout,
		 int nwrites, int *dest,
		 unsigned long long start, unsigned long long length)
{
	char buf[8192];
	int cpos = start % chunk_size; /* where in chunk we are up to */
	int len;
	int data_disks = raid_disks - (level == 0 ? 0 : level <=5 ? 1 : 2);
	int disk;

	while (length > 0) {
		unsigned long long offset;
		int i;
		len = chunk_size - cpos;
		if (len > sizeof(buf)) len = sizeof(buf);
		if (len > length) len = length;
		/* len bytes to be moved from one device */

		offset = (start/chunk_size/data_disks)*chunk_size + cpos;
		disk = start/chunk_size % data_disks;
		disk = geo_map(disk, start/chunk_size/data_disks,
			       raid_disks, level, layout);
		if (lseek64(source[disk], offsets[disk]+offset, 0) < 0)
			return -1;
		if (read(source[disk], buf, len) != len)
			return -1;
		for (i=0; i<nwrites; i++)
			if (write(dest[i], buf, len) != len)
				return -1;
		length -= len;
		start += len;
		cpos += len;
		while (cpos >= chunk_size) cpos -= chunk_size;
	}
	return 0;
}

/* Restore data:
 * We are given:
 *  A list of 'fds' of the active disks. Some may be '-1' for not-available.
 *  A geometry: raid_disks, chunk_size, level, layout
 *  An 'fd' to read from.  It is already seeked to the right (Read) location.
 *  A start and length.
 * The length must be a multiple of the stripe size.
 *
 * We build a full stripe in memory and then write it out.
 * We assume that there are enough working devices.
 */
int restore_stripes(int *dest, unsigned long long *offsets,
		    int raid_disks, int chunk_size, int level, int layout,
		    int source, unsigned long long read_offset,
		    unsigned long long start, unsigned long long length)
{
	char *stripe_buf = malloc(raid_disks * chunk_size);
	char **stripes = malloc(raid_disks * sizeof(char*));
	char **blocks = malloc(raid_disks * sizeof(char*));
	int i;

	int data_disks = raid_disks - (level == 0 ? 0 : level <=5 ? 1 : 2);

	if (stripe_buf == NULL || stripes == NULL || blocks == NULL) {
		free(stripe_buf);
		free(stripes);
		free(blocks);
		return -2;
	}
	for (i=0; i<raid_disks; i++)
		stripes[i] = stripe_buf + i * chunk_size;
	while (length > 0) {
		int len = data_disks * chunk_size;
		unsigned long long offset;
		if (length < len)
			return -3;
		for (i=0; i < data_disks; i++) {
			int disk = geo_map(i, start/chunk_size/data_disks,
					   raid_disks, level, layout);
			blocks[i] = stripes[disk];
			if (lseek64(source, read_offset, 0) != read_offset)
				return -1;
			if (read(source, stripes[disk], chunk_size) != chunk_size)
				return -1;
			read_offset += chunk_size;
		}
		/* We have the data, now do the parity */
		offset = (start/chunk_size/data_disks) * chunk_size;
		if (level >= 4) {
			int disk = geo_map(-1, start/chunk_size/data_disks,
					   raid_disks, level, layout);
			xor_blocks(stripes[disk], blocks, data_disks, chunk_size);
			/* FIXME need to do raid6 Q as well */
		}
		for (i=0; i < raid_disks ; i++)
			if (dest[i] >= 0) {
				if (lseek64(dest[i], offsets[i]+offset, 0) < 0)
					return -1;
				if (write(dest[i], stripes[i], chunk_size) != chunk_size)
					return -1;
			}
		length -= len;
		start += len;
	}
	return 0;
}

#ifdef MAIN

unsigned long long getnum(char *str, char **err)
{
	char *e;
	unsigned long long rv = strtoull(str, &e, 10);
	if (e==str || *e) {
		*err = str;
		return 0;
	}
	return rv;
}

main(int argc, char *argv[])
{
	/* save/restore file raid_disks chunk_size level layout start length devices...
	 */
	int save;
	int *fds;
	char *file;
	int storefd;
	unsigned long long *offsets;
	int raid_disks, chunk_size, level, layout;
	unsigned long long start, length;
	int i;

	char *err = NULL;
	if (argc < 10) {
		fprintf(stderr, "Usage: test_stripe save/restore file raid_disks"
			" chunk_size level layout start length devices...\n");
		exit(1);
	}
	if (strcmp(argv[1], "save")==0)
		save = 1;
	else if (strcmp(argv[1], "restore") == 0)
		save = 0;
	else {
		fprintf(stderr, "test_stripe: must give 'save' or 'restore'.\n");
		exit(2);
	}

	file = argv[2];
	raid_disks = getnum(argv[3], &err);
	chunk_size = getnum(argv[4], &err);
	level = getnum(argv[5], &err);
	layout = getnum(argv[6], &err);
	start = getnum(argv[7], &err);
	length = getnum(argv[8], &err);
	if (err) {
		fprintf(stderr, "test_stripe: Bad number: %s\n", err);
		exit(2);
	}
	if (argc != raid_disks + 9) {
		fprintf(stderr, "test_stripe: wrong number of devices: want %d found %d\n",
			raid_disks, argc-9);
		exit(2);
	}
	fds = malloc(raid_disks * sizeof(*fds));
	offsets = malloc(raid_disks * sizeof(*offsets));
	memset(offsets, 0, raid_disks * sizeof(*offsets));

	storefd = open(file, O_RDWR);
	if (storefd < 0) {
		perror(file);
		fprintf(stderr, "test_stripe: could not open %s.\n", file);
		exit(3);
	}
	for (i=0; i<raid_disks; i++) {
		fds[i] = open(argv[9+i], O_RDWR);
		if (fds[i] < 0) {
			perror(argv[9+i]);
			fprintf(stderr,"test_stripe: cannot open %s.\n", argv[9+i]);
			exit(3);
		}
	}

	if (save) {
		int rv = save_stripes(fds, offsets,
				      raid_disks, chunk_size, level, layout,
				      1, &storefd,
				      start, length);
		if (rv != 0) {
			fprintf(stderr, "test_stripe: save_stripes returned %d\n", rv);
			exit(1);
		}
	} else {
		int rv = restore_stripes(fds, offsets,
					 raid_disks, chunk_size, level, layout,
					 storefd, 0ULL,
					 start, length);
		if (rv != 0) {
			fprintf(stderr, "test_stripe: restore_stripes returned %d\n", rv);
			exit(1);
		}
	}
	exit(0);
}

#endif /* MAIN */
