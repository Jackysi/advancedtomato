/*
 * mdadm - manage Linux "md" devices aka RAID arrays.
 *
 * Copyright (C) 2004 Paul Clements, SteelEye Technology, Inc.
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
 */

#include "mdadm.h"

#define min(a,b) (((a) < (b)) ? (a) : (b))

inline void sb_le_to_cpu(bitmap_super_t *sb)
{
	sb->magic = __le32_to_cpu(sb->magic);
	sb->version = __le32_to_cpu(sb->version);
	/* uuid gets no translation */
	sb->events = __le64_to_cpu(sb->events);
	sb->events_cleared = __le64_to_cpu(sb->events_cleared);
	sb->state = __le32_to_cpu(sb->state);
	sb->chunksize = __le32_to_cpu(sb->chunksize);
	sb->daemon_sleep = __le32_to_cpu(sb->daemon_sleep);
	sb->sync_size = __le64_to_cpu(sb->sync_size);
	sb->write_behind = __le32_to_cpu(sb->write_behind);
}

inline void sb_cpu_to_le(bitmap_super_t *sb)
{
	sb_le_to_cpu(sb); /* these are really the same thing */
}

mapping_t bitmap_states[] = {
	{ "OK", 0 },
	{ "Out of date", 2 },
	{ NULL, -1 }
};

const char *bitmap_state(int state_num)
{
	char *state = map_num(bitmap_states, state_num);
	return state ? state : "Unknown";
}

const char *human_chunksize(unsigned long bytes)
{
	static char buf[16];
	char *suffixes[] = { "B", "KB", "MB", "GB", "TB", NULL };
	int i = 0;

	while (bytes >> 10) {
		bytes >>= 10;
		i++;
	}

	snprintf(buf, sizeof(buf), "%lu %s", bytes, suffixes[i]);

	return buf;
}

typedef struct bitmap_info_s {
	bitmap_super_t sb;
	unsigned long long total_bits;
	unsigned long long dirty_bits;
} bitmap_info_t;

/* count the dirty bits in the first num_bits of byte */
inline int count_dirty_bits_byte(char byte, int num_bits)
{
	int num = 0;

	switch (num_bits) { /* fall through... */
		case 8:	if (byte & 128) num++;
		case 7:	if (byte &  64) num++;
		case 6:	if (byte &  32) num++;
		case 5:	if (byte &  16) num++;
		case 4:	if (byte &   8) num++;
		case 3: if (byte &   4) num++;
		case 2:	if (byte &   2) num++;
		case 1:	if (byte &   1) num++;
		default: break;
	}

	return num;
}

int count_dirty_bits(char *buf, int num_bits)
{
	int i, num = 0;

	for (i=0; i < num_bits / 8; i++)
		num += count_dirty_bits_byte(buf[i], 8);

	if (num_bits % 8) /* not an even byte boundary */
		num += count_dirty_bits_byte(buf[i], num_bits % 8);

	return num;
}

/* calculate the size of the bitmap given the array size and bitmap chunksize */
unsigned long long bitmap_bits(unsigned long long array_size,
				unsigned long chunksize)
{
	return (array_size * 512 + chunksize - 1) / chunksize;
}

bitmap_info_t *bitmap_fd_read(int fd, int brief)
{
	/* Note: fd might be open O_DIRECT, so we must be
	 * careful to align reads properly
	 */
	unsigned long long total_bits = 0, read_bits = 0, dirty_bits = 0;
	bitmap_info_t *info;
	char *buf, *unaligned;
	int n, skip;

	unaligned = malloc(8192*2);
	buf = (char*) ((unsigned long)unaligned | 8191)+1;
	n = read(fd, buf, 8192);

	info = malloc(sizeof(*info));
	if (info == NULL) {
#if __GNUC__ < 3
		fprintf(stderr, Name ": failed to allocate %d bytes\n",
				(int)sizeof(*info));
#else
		fprintf(stderr, Name ": failed to allocate %zd bytes\n",
				sizeof(*info));
#endif
		return NULL;
	}

	if (n < sizeof(info->sb)) {
		fprintf(stderr, Name ": failed to read superblock of bitmap "
			"file: %s\n", strerror(errno));
		free(info);
		free(unaligned);
		return NULL;
	}
	memcpy(&info->sb, buf, sizeof(info->sb));
	skip = sizeof(info->sb);

	sb_le_to_cpu(&info->sb); /* convert superblock to CPU byte ordering */
	
	if (brief || info->sb.sync_size == 0)
		goto out;

	/* read the rest of the file counting total bits and dirty bits --
	 * we stop when either:
	 * 1) we hit EOF, in which case we assume the rest of the bits (if any)
	 *    are dirty
	 * 2) we've read the full bitmap, in which case we ignore any trailing
	 *    data in the file
	 */
	total_bits = bitmap_bits(info->sb.sync_size, info->sb.chunksize);

	while(read_bits < total_bits) {
		unsigned long long remaining = total_bits - read_bits;

		if (n == 0) {
			n = read(fd, buf, 8192);
			skip = 0;
			if (n <= 0)
				break;
		}
		if (remaining > (n-skip) * 8) /* we want the full buffer */
			remaining = (n-skip) * 8;

		dirty_bits += count_dirty_bits(buf+skip, remaining);

		read_bits += remaining;
		n = 0;
	}

	if (read_bits < total_bits) { /* file truncated... */
		fprintf(stderr, Name ": WARNING: bitmap file is not large "
			"enough for array size %llu!\n\n",
			(unsigned long long)info->sb.sync_size);
		total_bits = read_bits;
	}
out:
	info->total_bits = total_bits;
	info->dirty_bits = dirty_bits;
	return info;
}

bitmap_info_t *bitmap_file_read(char *filename, int brief, struct supertype **stp)
{
	int fd;
	bitmap_info_t *info;
	struct stat stb;
	struct supertype *st = *stp;

	if (stat(filename, &stb) < 0) {
		fprintf(stderr, Name ": failed to find file %s: %s\n",
			filename, strerror(errno));
		return NULL;
	}
	if ((S_IFMT & stb.st_mode) == S_IFBLK) {
		fd = open(filename, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, Name ": failed to open bitmap file %s: %s\n",
				filename, strerror(errno));
			return NULL;
		}
		/* block device, so we are probably after an internal bitmap */
		if (!st) st = guess_super(fd);
		if (!st) {
			/* just look at device... */
			lseek(fd, 0, 0);
		} else {	
			st->ss->locate_bitmap(st, fd, NULL);
		}
		ioctl(fd, BLKFLSBUF, 0); /* make sure we read current data */
		*stp = st;
	} else {
		fd = open(filename, O_RDONLY|O_DIRECT);
		if (fd < 0) {
			fprintf(stderr, Name ": failed to open bitmap file %s: %s\n",
				filename, strerror(errno));
			return NULL;
		}
	}

	info = bitmap_fd_read(fd, brief);
	close(fd);
	return info;
}

__u32 swapl(__u32 l)
{
	char *c = (char*)&l;
	char t= c[0];
	c[0] = c[3];
	c[3] = t;

	t = c[1];
	c[1] = c[2];
	c[2] = t;
	return l;
}
int ExamineBitmap(char *filename, int brief, struct supertype *st)
{
	/*
	 * Read the bitmap file and display its contents
	 */

	bitmap_super_t *sb;
	bitmap_info_t *info;
	int rv = 1;
	char buf[64];
	int swap;

	info = bitmap_file_read(filename, brief, &st);
	if (!info)
		return rv;

	sb = &info->sb;
	printf("        Filename : %s\n", filename);
	printf("           Magic : %08x\n", sb->magic);
	if (sb->magic != BITMAP_MAGIC) {
		fprintf(stderr, Name ": invalid bitmap magic 0x%x, the bitmap file appears to be corrupted\n", sb->magic);
	}
	printf("         Version : %d\n", sb->version);
	if (sb->version < BITMAP_MAJOR_LO ||
	    sb->version > BITMAP_MAJOR_HI) {
		fprintf(stderr, Name ": unknown bitmap version %d, either the bitmap file is corrupted or you need to upgrade your tools\n", sb->version);
		goto free_info;
	}

	rv = 0;
	if (st)
		swap = st->ss->swapuuid;
	else
#if __BYTE_ORDER == BIG_ENDIAN
		swap = 0;
#else
		swap = 1;
#endif
	if (swap) {
	printf("            UUID : %08x:%08x:%08x:%08x\n",
					swapl(*(__u32 *)(sb->uuid+0)),
					swapl(*(__u32 *)(sb->uuid+4)),
					swapl(*(__u32 *)(sb->uuid+8)),
					swapl(*(__u32 *)(sb->uuid+12)));
	} else {
	printf("            UUID : %08x:%08x:%08x:%08x\n",
					*(__u32 *)(sb->uuid+0),
					*(__u32 *)(sb->uuid+4),
					*(__u32 *)(sb->uuid+8),
					*(__u32 *)(sb->uuid+12));
	}
	printf("          Events : %llu\n", (unsigned long long)sb->events);
	printf("  Events Cleared : %llu\n", (unsigned long long)sb->events_cleared);
	printf("           State : %s\n", bitmap_state(sb->state));
	printf("       Chunksize : %s\n", human_chunksize(sb->chunksize));
	printf("          Daemon : %ds flush period\n", sb->daemon_sleep);
	if (sb->write_behind)
		sprintf(buf, "Allow write behind, max %d", sb->write_behind);
	else
		sprintf(buf, "Normal");
	printf("      Write Mode : %s\n", buf);
	printf("       Sync Size : %llu%s\n", (unsigned long long)sb->sync_size/2,
					human_size(sb->sync_size * 512));
	if (brief)
		goto free_info;
	printf("          Bitmap : %llu bits (chunks), %llu dirty (%2.1f%%)\n",
			info->total_bits, info->dirty_bits,
			100.0 * info->dirty_bits / (info->total_bits + 1));
free_info:
	free(info);
	return rv;
}

int CreateBitmap(char *filename, int force, char uuid[16],
		 unsigned long chunksize, unsigned long daemon_sleep,
		 unsigned long write_behind,
		 unsigned long long array_size /* sectors */,
		 int major)
{
	/*
	 * Create a bitmap file with a superblock and (optionally) a full bitmap
	 */

	FILE *fp;
	int rv = 1;
	char block[512];
	bitmap_super_t sb;
	long long bytes, filesize;

	if (!force && access(filename, F_OK) == 0) {
		fprintf(stderr, Name ": bitmap file %s already exists, use --force to overwrite\n", filename);
		return rv;
	}

	fp = fopen(filename, "w");
	if (fp == NULL) {
		fprintf(stderr, Name ": failed to open bitmap file %s: %s\n",
			filename, strerror(errno));
		return rv;
	}

	if (chunksize == UnSet) {
		/* We don't want more than 2^21 chunks, as 2^11 fill up one
		 * 4K page (2 bytes per chunk), and 2^10 address of those
		 * fill up a 4K indexing page.  2^20 might be safer, especially
		 * on 64bit hosts, so use that.
		 */
		chunksize = DEFAULT_BITMAP_CHUNK;
		/* <<20 for 2^20 chunks, >>9 to convert bytes to sectors */
		while (array_size > (chunksize << (20-9)))
			chunksize <<= 1;
	}

	memset(&sb, 0, sizeof(sb));
	sb.magic = BITMAP_MAGIC;
	sb.version = major;
	if (uuid != NULL)
		memcpy(sb.uuid, uuid, 16);
	sb.chunksize = chunksize;
	sb.daemon_sleep = daemon_sleep;
	sb.write_behind = write_behind;
	sb.sync_size = array_size;

	sb_cpu_to_le(&sb); /* convert to on-disk byte ordering */

	if (fwrite(&sb, sizeof(sb), 1, fp) != 1) {
		fprintf(stderr, Name ": failed to write superblock to bitmap file %s: %s\n", filename, strerror(errno));
		goto out;
	}

	/* calculate the size of the bitmap and write it to disk */
	bytes = (bitmap_bits(array_size, chunksize) + 7) / 8;
	if (!bytes) {
		rv = 0;
		goto out;
	}

	filesize = bytes + sizeof(sb);

	memset(block, 0xff, sizeof(block));

	while (bytes > 0) {
		if (fwrite(block, sizeof(block), 1, fp) != 1) {
			fprintf(stderr, Name ": failed to write bitmap file %s: %s\n", filename, strerror(errno));
			goto out;
		}
		bytes -= sizeof(block);
	}
	
	rv = 0;
	fflush(fp);
	/* make the file be the right size (well, to the nearest byte) */
	if (ftruncate(fileno(fp), filesize))
		perror("ftrunace");
out:
	fclose(fp);
	if (rv)
		unlink(filename); /* possibly corrupted, better get rid of it */
	return rv;
}

int bitmap_update_uuid(int fd, int *uuid, int swap)
{
	struct bitmap_super_s bm;
	if (lseek(fd, 0, 0) != 0)
		return 1;
	if (read(fd, &bm, sizeof(bm)) != sizeof(bm))
		return 1;
	if (bm.magic != __cpu_to_le32(BITMAP_MAGIC))
		return 1;
	copy_uuid(bm.uuid, uuid, swap);
	if (lseek(fd, 0, 0) != 0)
		return 2;
	if (write(fd, &bm, sizeof(bm)) != sizeof(bm)) {
		lseek(fd, 0, 0);
		return 2;
	}
	lseek(fd, 0, 0);
	return 0;
}
