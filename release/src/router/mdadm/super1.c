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
/*
 * The version-1 superblock :
 * All numeric fields are little-endian.
 *
 * total size: 256 bytes plus 2 per device.
 *  1K allows 384 devices.
 */
struct mdp_superblock_1 {
	/* constant array information - 128 bytes */
	__u32	magic;		/* MD_SB_MAGIC: 0xa92b4efc - little endian */
	__u32	major_version;	/* 1 */
	__u32	feature_map;	/* 0 for now */
	__u32	pad0;		/* always set to 0 when writing */

	__u8	set_uuid[16];	/* user-space generated. */
	char	set_name[32];	/* set and interpreted by user-space */

	__u64	ctime;		/* lo 40 bits are seconds, top 24 are microseconds or 0*/
	__u32	level;		/* -4 (multipath), -1 (linear), 0,1,4,5 */
	__u32	layout;		/* only for raid5 currently */
	__u64	size;		/* used size of component devices, in 512byte sectors */

	__u32	chunksize;	/* in 512byte sectors */
	__u32	raid_disks;
	__u32	bitmap_offset;	/* sectors after start of superblock that bitmap starts
				 * NOTE: signed, so bitmap can be before superblock
				 * only meaningful of feature_map[0] is set.
				 */

	/* These are only valid with feature bit '4' */
	__u32	new_level;	/* new level we are reshaping to		*/
	__u64	reshape_position;	/* next address in array-space for reshape */
	__u32	delta_disks;	/* change in number of raid_disks		*/
	__u32	new_layout;	/* new layout					*/
	__u32	new_chunk;	/* new chunk size (bytes)			*/
	__u8	pad1[128-124];	/* set to 0 when written */

	/* constant this-device information - 64 bytes */
	__u64	data_offset;	/* sector start of data, often 0 */
	__u64	data_size;	/* sectors in this device that can be used for data */
	__u64	super_offset;	/* sector start of this superblock */
	__u64	recovery_offset;/* sectors before this offset (from data_offset) have been recovered */
	__u32	dev_number;	/* permanent identifier of this  device - not role in raid */
	__u32	cnt_corrected_read; /* number of read errors that were corrected by re-writing */
	__u8	device_uuid[16]; /* user-space setable, ignored by kernel */
        __u8    devflags;        /* per-device flags.  Only one defined...*/
#define WriteMostly1    1        /* mask for writemostly flag in above */
	__u8	pad2[64-57];	/* set to 0 when writing */

	/* array state information - 64 bytes */
	__u64	utime;		/* 40 bits second, 24 btes microseconds */
	__u64	events;		/* incremented when superblock updated */
	__u64	resync_offset;	/* data before this offset (from data_offset) known to be in sync */
	__u32	sb_csum;	/* checksum upto devs[max_dev] */
	__u32	max_dev;	/* size of devs[] array to consider */
	__u8	pad3[64-32];	/* set to 0 when writing */

	/* device state information. Indexed by dev_number.
	 * 2 bytes per device
	 * Note there are no per-device state flags. State information is rolled
	 * into the 'roles' value.  If a device is spare or faulty, then it doesn't
	 * have a meaningful role.
	 */
	__u16	dev_roles[0];	/* role in array, or 0xffff for a spare, or 0xfffe for faulty */
};

struct misc_dev_info {
	__u64 device_size;
};

/* feature_map bits */
#define MD_FEATURE_BITMAP_OFFSET	1
#define	MD_FEATURE_RECOVERY_OFFSET	2 /* recovery_offset is present and
					   * must be honoured
					   */
#define	MD_FEATURE_RESHAPE_ACTIVE	4

#define	MD_FEATURE_ALL			(1|2|4)

#ifndef offsetof
#define offsetof(t,f) ((size_t)&(((t*)0)->f))
#endif
static unsigned int calc_sb_1_csum(struct mdp_superblock_1 * sb)
{
	unsigned int disk_csum, csum;
	unsigned long long newcsum;
	int size = sizeof(*sb) + __le32_to_cpu(sb->max_dev)*2;
	unsigned int *isuper = (unsigned int*)sb;
	int i;

/* make sure I can count... */
	if (offsetof(struct mdp_superblock_1,data_offset) != 128 ||
	    offsetof(struct mdp_superblock_1, utime) != 192 ||
	    sizeof(struct mdp_superblock_1) != 256) {
		fprintf(stderr, "WARNING - superblock isn't sized correctly\n");
	}

	disk_csum = sb->sb_csum;
	sb->sb_csum = 0;
	newcsum = 0;
	for (i=0; size>=4; size -= 4 ) {
		newcsum += __le32_to_cpu(*isuper);
		isuper++;
	}

	if (size == 2)
		newcsum += __le16_to_cpu(*(unsigned short*) isuper);

	csum = (newcsum & 0xffffffff) + (newcsum >> 32);
	sb->sb_csum = disk_csum;
	return __cpu_to_le32(csum);
}

#ifndef MDASSEMBLE
static void examine_super1(void *sbv, char *homehost)
{
	struct mdp_superblock_1 *sb = sbv;
	time_t atime;
	int d;
	int faulty;
	int i;
	char *c;
	int l = homehost ? strlen(homehost) : 0;
	int layout;

	printf("          Magic : %08x\n", __le32_to_cpu(sb->magic));
	printf("        Version : %02d\n", 1);
	printf("    Feature Map : 0x%x\n", __le32_to_cpu(sb->feature_map));
	printf("     Array UUID : ");
	for (i=0; i<16; i++) {
		if ((i&3)==0 && i != 0) printf(":");
		printf("%02x", sb->set_uuid[i]);
	}
	printf("\n");
	printf("           Name : %.32s", sb->set_name);
	if (l > 0 && l < 32 &&
	    sb->set_name[l] == ':' &&
	    strncmp(sb->set_name, homehost, l) == 0)
		printf("  (local to host %s)", homehost);
	printf("\n");
	atime = __le64_to_cpu(sb->ctime) & 0xFFFFFFFFFFULL;
	printf("  Creation Time : %.24s\n", ctime(&atime));
	c=map_num(pers, __le32_to_cpu(sb->level));
	printf("     Raid Level : %s\n", c?c:"-unknown-");
	printf("   Raid Devices : %d\n", __le32_to_cpu(sb->raid_disks));
	printf("\n");
	printf("  Used Dev Size : %llu%s\n",
	       (unsigned long long)sb->data_size,
	       human_size(sb->data_size<<9));
	if (__le32_to_cpu(sb->level) >= 0) {
		int ddsks=0;
		switch(__le32_to_cpu(sb->level)) {
		case 1: ddsks=1;break;
		case 4:
		case 5: ddsks = __le32_to_cpu(sb->raid_disks)-1; break;
		case 6: ddsks = __le32_to_cpu(sb->raid_disks)-2; break;
		case 10:
			layout = __le32_to_cpu(sb->layout);
			ddsks = __le32_to_cpu(sb->raid_disks)
				 / (layout&255) / ((layout>>8)&255);
		}
		if (ddsks)
			printf("     Array Size : %llu%s\n",
			       ddsks*(unsigned long long)__le64_to_cpu(sb->size),
			       human_size(ddsks*__le64_to_cpu(sb->size)<<9));
		if (sb->size != sb->data_size)
			printf("      Used Size : %llu%s\n",
			       (unsigned long long)__le64_to_cpu(sb->size),
			       human_size(__le64_to_cpu(sb->size)<<9));
	}
	if (sb->data_offset)
		printf("    Data Offset : %llu sectors\n",
		       (unsigned long long)__le64_to_cpu(sb->data_offset));
	printf("   Super Offset : %llu sectors\n",
	       (unsigned long long)__le64_to_cpu(sb->super_offset));
	if (__le32_to_cpu(sb->feature_map) & MD_FEATURE_RECOVERY_OFFSET)
		printf("Recovery Offset : %llu sectors\n", (unsigned long long)__le64_to_cpu(sb->recovery_offset));
	printf("          State : %s\n", (__le64_to_cpu(sb->resync_offset)+1)? "active":"clean");
	printf("    Device UUID : ");
	for (i=0; i<16; i++) {
		if ((i&3)==0 && i != 0) printf(":");
		printf("%02x", sb->device_uuid[i]);
	}
	printf("\n");
	printf("\n");
	if (sb->feature_map & __cpu_to_le32(MD_FEATURE_BITMAP_OFFSET)) {
		printf("Internal Bitmap : %ld sectors from superblock\n",
		       (long)__le32_to_cpu(sb->bitmap_offset));
	}
	if (sb->feature_map & __le32_to_cpu(MD_FEATURE_RESHAPE_ACTIVE)) {
		printf("  Reshape pos'n : %llu%s\n", (unsigned long long)__le64_to_cpu(sb->reshape_position)/2,
		       human_size(__le64_to_cpu(sb->reshape_position)<<9));
		if (__le32_to_cpu(sb->delta_disks)) {
			printf("  Delta Devices : %d", __le32_to_cpu(sb->delta_disks));
			if (__le32_to_cpu(sb->delta_disks))
				printf(" (%d->%d)\n",
				       __le32_to_cpu(sb->raid_disks)-__le32_to_cpu(sb->delta_disks),
				       __le32_to_cpu(sb->raid_disks));
			else
				printf(" (%d->%d)\n", __le32_to_cpu(sb->raid_disks),
				       __le32_to_cpu(sb->raid_disks)+__le32_to_cpu(sb->delta_disks));
		}
		if (__le32_to_cpu(sb->new_level) != __le32_to_cpu(sb->level)) {
			c = map_num(pers, __le32_to_cpu(sb->new_level));
			printf("      New Level : %s\n", c?c:"-unknown-");
		}
		if (__le32_to_cpu(sb->new_layout) != __le32_to_cpu(sb->layout)) {
			if (__le32_to_cpu(sb->level) == 5) {
				c = map_num(r5layout, __le32_to_cpu(sb->new_layout));
				printf("     New Layout : %s\n", c?c:"-unknown-");
			}
			if (__le32_to_cpu(sb->level) == 10) {
				printf("     New Layout : near=%d, %s=%d\n",
				       __le32_to_cpu(sb->new_layout)&255,
				       (__le32_to_cpu(sb->new_layout)&0x10000)?"offset":"far",
				       (__le32_to_cpu(sb->new_layout)>>8)&255);
			}
		}
		if (__le32_to_cpu(sb->new_chunk) != __le32_to_cpu(sb->chunksize))
			printf("  New Chunksize : %dK\n", __le32_to_cpu(sb->new_chunk)/2);
		printf("\n");
	}
	if (sb->devflags) {
		printf("      Flags :");
		if (sb->devflags & WriteMostly1)
			printf(" write-mostly");
		printf("\n");
	}

	atime = __le64_to_cpu(sb->utime) & 0xFFFFFFFFFFULL;
	printf("    Update Time : %.24s\n", ctime(&atime));

	if (calc_sb_1_csum(sb) == sb->sb_csum)
		printf("       Checksum : %x - correct\n", __le32_to_cpu(sb->sb_csum));
	else
		printf("       Checksum : %x - expected %x\n", __le32_to_cpu(sb->sb_csum),
		       __le32_to_cpu(calc_sb_1_csum(sb)));
	printf("         Events : %llu\n", (unsigned long long)__le64_to_cpu(sb->events));
	printf("\n");
	if (__le32_to_cpu(sb->level) == 5) {
		c = map_num(r5layout, __le32_to_cpu(sb->layout));
		printf("         Layout : %s\n", c?c:"-unknown-");
	}
	if (__le32_to_cpu(sb->level) == 10) {
		int lo = __le32_to_cpu(sb->layout);
		printf("         Layout : near=%d, %s=%d\n",
		       lo&255,
		       (lo&0x10000)?"offset":"far",
		       (lo>>8)&255);
	}
	switch(__le32_to_cpu(sb->level)) {
	case 0:
	case 4:
	case 5:
	case 6:
	case 10:
		printf("     Chunk Size : %dK\n", __le32_to_cpu(sb->chunksize)/2);
		break;
	case -1:
		printf("       Rounding : %dK\n", __le32_to_cpu(sb->chunksize)/2);
		break;
	default: break;
	}
	printf("\n");
	printf("    Array Slot : %d (", __le32_to_cpu(sb->dev_number));
	for (i= __le32_to_cpu(sb->max_dev); i> 0 ; i--)
		if (__le16_to_cpu(sb->dev_roles[i-1]) != 0xffff)
			break;
	for (d=0; d < i; d++) {
		int role = __le16_to_cpu(sb->dev_roles[d]);
		if (d) printf(", ");
		if (role == 0xffff) printf("empty");
		else if(role == 0xfffe) printf("failed");
		else printf("%d", role);
	}
	printf(")\n");
	printf("   Array State : ");
	for (d=0; d<__le32_to_cpu(sb->raid_disks); d++) {
		int cnt = 0;
		int me = 0;
		int i;
		for (i=0; i< __le32_to_cpu(sb->max_dev); i++) {
			int role = __le16_to_cpu(sb->dev_roles[i]);
			if (role == d) {
				if (i == __le32_to_cpu(sb->dev_number))
					me = 1;
				cnt++;
			}
		}
		if (cnt > 1) printf("?");
		else if (cnt == 1 && me) printf("U");
		else if (cnt == 1) printf("u");
		else printf ("_");
	}
	faulty = 0;
	for (i=0; i< __le32_to_cpu(sb->max_dev); i++) {
		int role = __le16_to_cpu(sb->dev_roles[i]);
		if (role == 0xFFFE)
			faulty++;
	}
	if (faulty) printf(" %d failed", faulty);
	printf("\n");
}


static void brief_examine_super1(void *sbv)
{
	struct mdp_superblock_1 *sb = sbv;
	int i;
	char *nm;
	char *c=map_num(pers, __le32_to_cpu(sb->level));

	nm = strchr(sb->set_name, ':');
	if (nm)
		nm++;
	else if (sb->set_name[0])
		nm = sb->set_name;
	else
		nm = "??";

	printf("ARRAY /dev/md/%s level=%s metadata=1 num-devices=%d UUID=",
	       nm,
	       c?c:"-unknown-", __le32_to_cpu(sb->raid_disks));
	for (i=0; i<16; i++) {
		if ((i&3)==0 && i != 0) printf(":");
		printf("%02x", sb->set_uuid[i]);
	}
	if (sb->set_name[0])
		printf(" name=%.32s", sb->set_name);
	printf("\n");
}

static void detail_super1(void *sbv, char *homehost)
{
	struct mdp_superblock_1 *sb = sbv;
	int i;
	int l = homehost ? strlen(homehost) : 0;

	printf("           Name : %.32s", sb->set_name);
	if (l > 0 && l < 32 &&
	    sb->set_name[l] == ':' &&
	    strncmp(sb->set_name, homehost, l) == 0)
		printf("  (local to host %s)", homehost);
	printf("\n           UUID : ");
	for (i=0; i<16; i++) {
		if ((i&3)==0 && i != 0) printf(":");
		printf("%02x", sb->set_uuid[i]);
	}
	printf("\n         Events : %llu\n\n", (unsigned long long)__le64_to_cpu(sb->events));
}

static void brief_detail_super1(void *sbv)
{
	struct mdp_superblock_1 *sb = sbv;
	int i;

	if (sb->set_name[0])
		printf(" name=%.32s", sb->set_name);
	printf(" UUID=");
	for (i=0; i<16; i++) {
		if ((i&3)==0 && i != 0) printf(":");
		printf("%02x", sb->set_uuid[i]);
	}
}

#endif

static int match_home1(void *sbv, char *homehost)
{
	struct mdp_superblock_1 *sb = sbv;
	int l = homehost ? strlen(homehost) : 0;

	return (l > 0 && l < 32 &&
		sb->set_name[l] == ':' &&
		strncmp(sb->set_name, homehost, l) == 0);
}

static void uuid_from_super1(int uuid[4], void * sbv)
{
	struct mdp_superblock_1 *super = sbv;
	char *cuuid = (char*)uuid;
	int i;
	for (i=0; i<16; i++)
		cuuid[i] = super->set_uuid[i];
}

static void getinfo_super1(struct mdinfo *info, void *sbv)
{
	struct mdp_superblock_1 *sb = sbv;
	int working = 0;
	int i;
	int role;

	info->array.major_version = 1;
	info->array.minor_version = __le32_to_cpu(sb->feature_map);
	info->array.patch_version = 0;
	info->array.raid_disks = __le32_to_cpu(sb->raid_disks);
	info->array.level = __le32_to_cpu(sb->level);
	info->array.layout = __le32_to_cpu(sb->layout);
	info->array.md_minor = -1;
	info->array.ctime = __le64_to_cpu(sb->ctime);
	info->array.utime = __le64_to_cpu(sb->utime);
	info->array.chunk_size = __le32_to_cpu(sb->chunksize)*512;
	info->array.state =
		(__le64_to_cpu(sb->resync_offset) >= __le64_to_cpu(sb->size))
		? 1 : 0;

	info->data_offset = __le64_to_cpu(sb->data_offset);
	info->component_size = __le64_to_cpu(sb->size);

	info->disk.major = 0;
	info->disk.minor = 0;
	info->disk.number = __le32_to_cpu(sb->dev_number);
	if (__le32_to_cpu(sb->dev_number) >= __le32_to_cpu(sb->max_dev) ||
	    __le32_to_cpu(sb->max_dev) > 512)
		role = 0xfffe;
	else
		role = __le16_to_cpu(sb->dev_roles[__le32_to_cpu(sb->dev_number)]);

	info->disk.raid_disk = -1;
	switch(role) {
	case 0xFFFF:
		info->disk.state = 2; /* spare: ACTIVE, not sync, not faulty */
		break;
	case 0xFFFE:
		info->disk.state = 1; /* faulty */
		break;
	default:
		info->disk.state = 6; /* active and in sync */
		info->disk.raid_disk = role;
	}
	info->events = __le64_to_cpu(sb->events);

	memcpy(info->uuid, sb->set_uuid, 16);

	strncpy(info->name, sb->set_name, 32);
	info->name[32] = 0;

	if (sb->feature_map & __le32_to_cpu(MD_FEATURE_RESHAPE_ACTIVE)) {
		info->reshape_active = 1;
		info->reshape_progress = __le64_to_cpu(sb->reshape_position);
		info->new_level = __le32_to_cpu(sb->new_level);
		info->delta_disks = __le32_to_cpu(sb->delta_disks);
		info->new_layout = __le32_to_cpu(sb->new_layout);
		info->new_chunk = __le32_to_cpu(sb->new_chunk)<<9;
	} else
		info->reshape_active = 0;

	for (i=0; i< __le32_to_cpu(sb->max_dev); i++) {
		role = __le16_to_cpu(sb->dev_roles[i]);
		if (/*role == 0xFFFF || */role < info->array.raid_disks)
			working++;
	}

	info->array.working_disks = working;
}

static int update_super1(struct mdinfo *info, void *sbv, char *update,
			 char *devname, int verbose,
			 int uuid_set, char *homehost)
{
	/* NOTE: for 'assemble' and 'force' we need to return non-zero if any change was made.
	 * For others, the return value is ignored.
	 */
	int rv = 0;
	struct mdp_superblock_1 *sb = sbv;

	if (strcmp(update, "force-one")==0) {
		/* Not enough devices for a working array,
		 * so bring this one up-to-date
		 */
		if (sb->events != __cpu_to_le64(info->events))
			rv = 1;
		sb->events = __cpu_to_le64(info->events);
	}
	if (strcmp(update, "force-array")==0) {
		/* Degraded array and 'force' requests to
		 * maybe need to mark it 'clean'.
		 */
		switch(__le32_to_cpu(sb->level)) {
		case 5: case 4: case 6:
			/* need to force clean */
			if (sb->resync_offset != ~0ULL)
				rv = 1;
			sb->resync_offset = ~0ULL;
		}
	}
	if (strcmp(update, "assemble")==0) {
		int d = info->disk.number;
		int want;
		if (info->disk.state == 6)
			want = __cpu_to_le32(info->disk.raid_disk);
		else
			want = 0xFFFF;
		if (sb->dev_roles[d] != want) {
			sb->dev_roles[d] = want;
			rv = 1;
		}
	}
	if (strcmp(update, "grow") == 0) {
		sb->raid_disks = __cpu_to_le32(info->array.raid_disks);
		/* As we are just adding a spare, there is no need to
		 * make any change to the dev_roles array
		 */
	}
	if (strcmp(update, "resync") == 0) {
		/* make sure resync happens */
		sb->resync_offset = 0ULL;
	}
	if (strcmp(update, "uuid") == 0) {
		copy_uuid(sb->set_uuid, info->uuid, super1.swapuuid);

		if (__le32_to_cpu(sb->feature_map)&MD_FEATURE_BITMAP_OFFSET) {
			struct bitmap_super_s *bm;
			bm = (struct bitmap_super_s*)(sbv+1024);
			memcpy(bm->uuid, sb->set_uuid, 16);
		}
	}
	if (strcmp(update, "homehost") == 0 &&
	    homehost) {
		char *c;
		update = "name";
		c = strchr(sb->set_name, ':');
		if (c)
			strncpy(info->name, c+1, 31 - (c-sb->set_name));
		else
			strncpy(info->name, sb->set_name, 32);
		info->name[32] = 0;
	}
	if (strcmp(update, "name") == 0) {
		if (info->name[0] == 0)
			sprintf(info->name, "%d", info->array.md_minor);
		memset(sb->set_name, 0, sizeof(sb->set_name));
		if (homehost &&
		    strchr(info->name, ':') == NULL &&
		    strlen(homehost)+1+strlen(info->name) < 32) {
			strcpy(sb->set_name, homehost);
			strcat(sb->set_name, ":");
			strcat(sb->set_name, info->name);
		} else
			strcpy(sb->set_name, info->name);
	}
	if (strcmp(update, "devicesize") == 0 &&
	    __le64_to_cpu(sb->super_offset) <
	    __le64_to_cpu(sb->data_offset)) {
		/* set data_size to device size less data_offset */
		struct misc_dev_info *misc = (struct misc_dev_info*)
			(sbv + 1024 + sizeof(struct bitmap_super_s));
		printf("Size was %llu\n", __le64_to_cpu(sb->data_size));
		sb->data_size = __cpu_to_le64(
			misc->device_size - __le64_to_cpu(sb->data_offset));
		printf("Size is %llu\n", __le64_to_cpu(sb->data_size));
	}
	if (strcmp(update, "_reshape_progress")==0)
		sb->reshape_position = __cpu_to_le64(info->reshape_progress);

	sb->sb_csum = calc_sb_1_csum(sb);
	return rv;
}

static int init_super1(struct supertype *st, void **sbp, mdu_array_info_t *info,
		       unsigned long long size, char *name, char *homehost, int *uuid)
{
	struct mdp_superblock_1 *sb = malloc(1024 + sizeof(bitmap_super_t) +
					     sizeof(struct misc_dev_info));
	int spares;
	int rfd;
	char defname[10];
	memset(sb, 0, 1024);

	if (info->major_version == -1) {
		/* zeroing superblock */
		*sbp = sb;
		return 0;
	}

	spares = info->working_disks - info->active_disks;
	if (info->raid_disks + spares  > 384) {
		fprintf(stderr, Name ": too many devices requested: %d+%d > %d\n",
			info->raid_disks , spares, 384);
		return 0;
	}

	sb->magic = __cpu_to_le32(MD_SB_MAGIC);
	sb->major_version = __cpu_to_le32(1);
	sb->feature_map = 0;
	sb->pad0 = 0;

	if (uuid)
		copy_uuid(sb->set_uuid, uuid, super1.swapuuid);
	else {
		if ((rfd = open("/dev/urandom", O_RDONLY)) < 0 ||
		    read(rfd, sb->set_uuid, 16) != 16) {
			*(__u32*)(sb->set_uuid) = random();
			*(__u32*)(sb->set_uuid+4) = random();
			*(__u32*)(sb->set_uuid+8) = random();
			*(__u32*)(sb->set_uuid+12) = random();
		}
		if (rfd >= 0) close(rfd);
	}

	if (name == NULL || *name == 0) {
		sprintf(defname, "%d", info->md_minor);
		name = defname;
	}
	memset(sb->set_name, 0, 32);
	if (homehost &&
	    strchr(name, ':')== NULL &&
	    strlen(homehost)+1+strlen(name) < 32) {
		strcpy(sb->set_name, homehost);
		strcat(sb->set_name, ":");
		strcat(sb->set_name, name);
	} else
		strcpy(sb->set_name, name);

	sb->ctime = __cpu_to_le64((unsigned long long)time(0));
	sb->level = __cpu_to_le32(info->level);
	sb->layout = __cpu_to_le32(info->layout);
	sb->size = __cpu_to_le64(size*2ULL);
	sb->chunksize = __cpu_to_le32(info->chunk_size>>9);
	sb->raid_disks = __cpu_to_le32(info->raid_disks);

	sb->data_offset = __cpu_to_le64(0);
	sb->data_size = __cpu_to_le64(0);
	sb->super_offset = __cpu_to_le64(0);
	sb->recovery_offset = __cpu_to_le64(0);

	sb->utime = sb->ctime;
	sb->events = __cpu_to_le64(1);
	if (info->state & (1<<MD_SB_CLEAN))
		sb->resync_offset = ~0ULL;
	else
		sb->resync_offset = 0;
	sb->max_dev = __cpu_to_le32((1024- sizeof(struct mdp_superblock_1))/
				    sizeof(sb->dev_roles[0]));
	memset(sb->pad3, 0, sizeof(sb->pad3));

	memset(sb->dev_roles, 0xff, 1024 - sizeof(struct mdp_superblock_1));

	*sbp = sb;
	return 1;
}

/* Add a device to the superblock being created */
static void add_to_super1(void *sbv, mdu_disk_info_t *dk)
{
	struct mdp_superblock_1 *sb = sbv;
	__u16 *rp = sb->dev_roles + dk->number;
	if ((dk->state & 6) == 6) /* active, sync */
		*rp = __cpu_to_le16(dk->raid_disk);
	else if ((dk->state & ~2) == 0) /* active or idle -> spare */
		*rp = 0xffff;
	else
		*rp = 0xfffe;
}

static void locate_bitmap1(struct supertype *st, int fd, void *sbv);

static int store_super1(struct supertype *st, int fd, void *sbv)
{
	struct mdp_superblock_1 *sb = sbv;
	unsigned long long sb_offset;
	int sbsize;
	unsigned long long dsize;

	if (!get_dev_size(fd, NULL, &dsize))
		return 1;

	dsize >>= 9;

	if (dsize < 24)
		return 2;

	/*
	 * Calculate the position of the superblock.
	 * It is always aligned to a 4K boundary and
	 * depending on minor_version, it can be:
	 * 0: At least 8K, but less than 12K, from end of device
	 * 1: At start of device
	 * 2: 4K from start of device.
	 */
	switch(st->minor_version) {
	case 0:
		sb_offset = dsize;
		sb_offset -= 8*2;
		sb_offset &= ~(4*2-1);
		break;
	case 1:
		sb_offset = 0;
		break;
	case 2:
		sb_offset = 4*2;
		break;
	default:
		return -EINVAL;
	}



	if (sb_offset != __le64_to_cpu(sb->super_offset) &&
	    0 != __le64_to_cpu(sb->super_offset)
		) {
		fprintf(stderr, Name ": internal error - sb_offset is wrong\n");
		abort();
	}

	if (lseek64(fd, sb_offset << 9, 0)< 0LL)
		return 3;

	sbsize = sizeof(*sb) + 2 * __le32_to_cpu(sb->max_dev);

	if (write(fd, sb, sbsize) != sbsize)
		return 4;

	if (sb->feature_map & __cpu_to_le32(MD_FEATURE_BITMAP_OFFSET)) {
		struct bitmap_super_s *bm = (struct bitmap_super_s*)
			(((char*)sb)+1024);
		if (__le32_to_cpu(bm->magic) == BITMAP_MAGIC) {
			locate_bitmap1(st, fd, sbv);
			if (write(fd, bm, sizeof(*bm)) != sizeof(*bm))
			    return 5;
		}
	}
	fsync(fd);
	return 0;
}

static int load_super1(struct supertype *st, int fd, void **sbp, char *devname);

static int write_init_super1(struct supertype *st, void *sbv,
			     mdu_disk_info_t *dinfo, char *devname)
{
	struct mdp_superblock_1 *sb = sbv;
	void *refsbv = NULL;
	int fd = open(devname, O_RDWR | O_EXCL);
	int rfd;
	int rv;
	int bm_space;

	unsigned long space;
	unsigned long long dsize, array_size;
	long long sb_offset;


	if (fd < 0) {
		fprintf(stderr, Name ": Failed to open %s to write superblock\n",
			devname);
		return -1;
	}

	sb->dev_number = __cpu_to_le32(dinfo->number);
	if (dinfo->state & (1<<MD_DISK_WRITEMOSTLY))
		sb->devflags |= __cpu_to_le32(WriteMostly1);

	if ((rfd = open("/dev/urandom", O_RDONLY)) < 0 ||
	    read(rfd, sb->device_uuid, 16) != 16) {
		*(__u32*)(sb->device_uuid) = random();
		*(__u32*)(sb->device_uuid+4) = random();
		*(__u32*)(sb->device_uuid+8) = random();
		*(__u32*)(sb->device_uuid+12) = random();
	}
	if (rfd >= 0) close(rfd);
	sb->events = 0;

	if (load_super1(st, fd, &refsbv, NULL)==0) {
		struct mdp_superblock_1 *refsb = refsbv;

		memcpy(sb->device_uuid, refsb->device_uuid, 16);
		if (memcmp(sb->set_uuid, refsb->set_uuid, 16)==0) {
			/* same array, so preserve events and dev_number */
			sb->events = refsb->events;
			/* bugs in 2.6.17 and earlier mean the dev_number
			 * chosen in Manage must be preserved
			 */
			if (get_linux_version() >= 2006018)
				sb->dev_number = refsb->dev_number;
		}
		free(refsb);
	}

	if (!get_dev_size(fd, NULL, &dsize))
		return 1;
	dsize >>= 9;

	if (dsize < 24) {
		close(fd);
		return 2;
	}


	/*
	 * Calculate the position of the superblock.
	 * It is always aligned to a 4K boundary and
	 * depending on minor_version, it can be:
	 * 0: At least 8K, but less than 12K, from end of device
	 * 1: At start of device
	 * 2: 4K from start of device.
	 * Depending on the array size, we might leave extra space
	 * for a bitmap.
	 */
	array_size = __le64_to_cpu(sb->size);
	/* work out how much space we left of a bitmap */
	if (array_size >= 200*1024*1024*2)
		bm_space = 128*2;
	else if (array_size > 8*1024*1024*2)
		bm_space = 64*2;
	else
		bm_space = 0;

	switch(st->minor_version) {
	case 0:
		sb_offset = dsize;
		sb_offset -= 8*2;
		sb_offset &= ~(4*2-1);
		sb->super_offset = __cpu_to_le64(sb_offset);
		sb->data_offset = __cpu_to_le64(0);
		sb->data_size = __cpu_to_le64(sb_offset - bm_space);
		break;
	case 1:
		sb->super_offset = __cpu_to_le64(0);
		sb->data_offset = __cpu_to_le64(bm_space + 4*2);
		sb->data_size = __cpu_to_le64(dsize - bm_space - 4*2);
		break;
	case 2:
		sb_offset = 4*2;
		if (dsize - 4*2 - 64*2 >= array_size && array_size > 8*1024*1024*2)
			space = 64*2;
		else
			space = 4*2;
		sb->super_offset = __cpu_to_le64(4*2);
		sb->data_offset = __cpu_to_le64(4*2 + 4*2 + bm_space);
		sb->data_size = __cpu_to_le64(dsize - 4*2 - 4*2 - bm_space );
		break;
	default:
		return -EINVAL;
	}


	sb->sb_csum = calc_sb_1_csum(sb);
	rv = store_super1(st, fd, sb);
	if (rv)
		fprintf(stderr, Name ": failed to write superblock to %s\n", devname);

	if (rv == 0 && (__le32_to_cpu(sb->feature_map) & 1))
		rv = st->ss->write_bitmap(st, fd, sbv);
	close(fd);
	return rv;
}

static int compare_super1(void **firstp, void *secondv)
{
	/*
	 * return:
	 *  0 same, or first was empty, and second was copied
	 *  1 second had wrong number
	 *  2 wrong uuid
	 *  3 wrong other info
	 */
	struct mdp_superblock_1 *first = *firstp;
	struct mdp_superblock_1 *second = secondv;

	if (second->magic != __cpu_to_le32(MD_SB_MAGIC))
		return 1;
	if (second->major_version != __cpu_to_le32(1))
		return 1;

	if (!first) {
		first = malloc(1024+sizeof(bitmap_super_t) +
			       sizeof(struct misc_dev_info));
		memcpy(first, second, 1024+sizeof(bitmap_super_t) +
		       sizeof(struct misc_dev_info));
		*firstp = first;
		return 0;
	}
	if (memcmp(first->set_uuid, second->set_uuid, 16)!= 0)
		return 2;

	if (first->ctime      != second->ctime     ||
	    first->level      != second->level     ||
	    first->layout     != second->layout    ||
	    first->size       != second->size      ||
	    first->chunksize  != second->chunksize ||
	    first->raid_disks != second->raid_disks)
		return 3;
	return 0;
}

static int load_super1(struct supertype *st, int fd, void **sbp, char *devname)
{
	unsigned long long dsize;
	unsigned long long sb_offset;
	struct mdp_superblock_1 *super;
	int uuid[4];
	struct bitmap_super_s *bsb;
	struct misc_dev_info *misc;


	if (st->ss == NULL) {
		int bestvers = -1;
		__u64 bestctime = 0;
		/* guess... choose latest ctime */
		st->ss = &super1;
		for (st->minor_version = 0; st->minor_version <= 2 ; st->minor_version++) {
			switch(load_super1(st, fd, sbp, devname)) {
			case 0: super = *sbp;
				if (bestvers == -1 ||
				    bestctime < __le64_to_cpu(super->ctime)) {
					bestvers = st->minor_version;
					bestctime = __le64_to_cpu(super->ctime);
				}
				free(super);
				*sbp = NULL;
				break;
			case 1: st->ss = NULL; return 1; /*bad device */
			case 2: break; /* bad, try next */
			}
		}
		if (bestvers != -1) {
			int rv;
			st->minor_version = bestvers;
			st->ss = &super1;
			st->max_devs = 384;
			rv = load_super1(st, fd, sbp, devname);
			if (rv) st->ss = NULL;
			return rv;
		}
		st->ss = NULL;
		return 2;
	}
	if (!get_dev_size(fd, devname, &dsize))
		return 1;
	dsize >>= 9;

	if (dsize < 24) {
		if (devname)
			fprintf(stderr, Name ": %s is too small for md: size is %llu sectors.\n",
				devname, dsize);
		return 1;
	}

	/*
	 * Calculate the position of the superblock.
	 * It is always aligned to a 4K boundary and
	 * depending on minor_version, it can be:
	 * 0: At least 8K, but less than 12K, from end of device
	 * 1: At start of device
	 * 2: 4K from start of device.
	 */
	switch(st->minor_version) {
	case 0:
		sb_offset = dsize;
		sb_offset -= 8*2;
		sb_offset &= ~(4*2-1);
		break;
	case 1:
		sb_offset = 0;
		break;
	case 2:
		sb_offset = 4*2;
		break;
	default:
		return -EINVAL;
	}

	ioctl(fd, BLKFLSBUF, 0); /* make sure we read current data */


	if (lseek64(fd, sb_offset << 9, 0)< 0LL) {
		if (devname)
			fprintf(stderr, Name ": Cannot seek to superblock on %s: %s\n",
				devname, strerror(errno));
		return 1;
	}

	super = malloc(1024 + sizeof(bitmap_super_t) +
		       sizeof(struct misc_dev_info));

	if (read(fd, super, 1024) != 1024) {
		if (devname)
			fprintf(stderr, Name ": Cannot read superblock on %s\n",
				devname);
		free(super);
		return 1;
	}

	if (__le32_to_cpu(super->magic) != MD_SB_MAGIC) {
		if (devname)
			fprintf(stderr, Name ": No super block found on %s (Expected magic %08x, got %08x)\n",
				devname, MD_SB_MAGIC, __le32_to_cpu(super->magic));
		free(super);
		return 2;
	}

	if (__le32_to_cpu(super->major_version) != 1) {
		if (devname)
			fprintf(stderr, Name ": Cannot interpret superblock on %s - version is %d\n",
				devname, __le32_to_cpu(super->major_version));
		free(super);
		return 2;
	}
	if (__le64_to_cpu(super->super_offset) != sb_offset) {
		if (devname)
			fprintf(stderr, Name ": No superblock found on %s (super_offset is wrong)\n",
				devname);
		free(super);
		return 2;
	}
	*sbp = super;

	bsb = (struct bitmap_super_s *)(((char*)super)+1024);

	misc = (struct misc_dev_info*) (bsb+1);
	misc->device_size = dsize;

	/* Now check on the bitmap superblock */
	if ((__le32_to_cpu(super->feature_map)&MD_FEATURE_BITMAP_OFFSET) == 0)
		return 0;
	/* Read the bitmap superblock and make sure it looks
	 * valid.  If it doesn't clear the bit.  An --assemble --force
	 * should get that written out.
	 */
	locate_bitmap1(st, fd, super);
	if (read(fd, ((char*)super)+1024, sizeof(struct bitmap_super_s))
	    != sizeof(struct bitmap_super_s))
		goto no_bitmap;

	uuid_from_super1(uuid, super);
	if (__le32_to_cpu(bsb->magic) != BITMAP_MAGIC ||
	    memcmp(bsb->uuid, uuid, 16) != 0)
		goto no_bitmap;
	return 0;

 no_bitmap:
	super->feature_map = __cpu_to_le32(__le32_to_cpu(super->feature_map) & ~1);
	return 0;
}


static struct supertype *match_metadata_desc1(char *arg)
{
	struct supertype *st = malloc(sizeof(*st));
	if (!st) return st;

	st->ss = &super1;
	st->max_devs = 384;
	if (strcmp(arg, "1") == 0 ||
	    strcmp(arg, "1.0") == 0 ||
	    strcmp(arg, "default/large") == 0) {
		st->minor_version = 0;
		return st;
	}
	if (strcmp(arg, "1.1") == 0) {
		st->minor_version = 1;
		return st;
	}
	if (strcmp(arg, "1.2") == 0) {
		st->minor_version = 2;
		return st;
	}

	free(st);
	return NULL;
}

/* find available size on device with this devsize, using
 * superblock type st, and reserving 'reserve' sectors for
 * a possible bitmap
 */
static __u64 avail_size1(struct supertype *st, __u64 devsize)
{
	if (devsize < 24)
		return 0;

	/* if the device is bigger than 8Gig, save 64k for bitmap usage,
	 * if bigger than 200Gig, save 128k
	 */
	if (devsize-64*2 >= 200*1024*1024*2)
		devsize -= 128*2;
	else if (devsize >= 8*1024*1024*2)
		devsize -= 64*2;

	switch(st->minor_version) {
	case 0:
		/* at end */
		return ((devsize - 8*2 ) & ~(4*2-1));
	case 1:
		/* at start, 4K for superblock and possible bitmap */
		return devsize - 4*2;
	case 2:
		/* 4k from start, 4K for superblock and possible bitmap */
		return devsize - (4+4)*2;
	}
	return 0;
}

static int
add_internal_bitmap1(struct supertype *st, void *sbv,
		     int *chunkp, int delay, int write_behind,
		     unsigned long long size,
		     int may_change, int major)
{
	/*
	 * If not may_change, then this is a 'Grow', and the bitmap
	 * must fit after the superblock.
	 * If may_change, then this is create, and we can put the bitmap
	 * before the superblock if we like, or may move the start.
	 * If !may_change, the bitmap MUST live at offset of 1K, until
	 * we get a sysfs interface.
	 *
	 * size is in sectors,  chunk is in bytes !!!
	 */

	unsigned long long bits;
	unsigned long long max_bits;
	unsigned long long min_chunk;
	long offset;
	int chunk = *chunkp;
	int room = 0;
	struct mdp_superblock_1 *sb = sbv;
	bitmap_super_t *bms = (bitmap_super_t*)(((char*)sb) + 1024);

	switch(st->minor_version) {
	case 0:
		/* either 3K after the superblock, or some amount of space
		 * before.
		 */
		if (may_change) {
			/* We are creating array, so we *know* how much room has
			 * been left.
			 */
			offset = 0;
			if (__le64_to_cpu(sb->size) >= 200*1024*1024*2)
				room = 128*2;
			else if (__le64_to_cpu(sb->size) > 8*1024*1024*2)
				room = 64*2;
			else {
				room = 3*2;
				offset = 2;
			}
		} else {
			room = __le64_to_cpu(sb->super_offset)
				- __le64_to_cpu(sb->data_offset)
				- __le64_to_cpu(sb->data_size);
			/* remove '1 ||' when we can set offset via sysfs */
			if (1 || (room < 3*2 &&
				  __le32_to_cpu(sb->max_dev) <= 384)) {
				room = 3*2;
				offset = 1*2;
			} else {
				offset = 0; /* means movable offset */
			}
		}
		break;
	case 1:
	case 2: /* between superblock and data */
		if (may_change) {
			offset = 4*2;
			if (__le64_to_cpu(sb->size) >= 200*1024*1024*2)
				room = 128*2;
			else if (__le64_to_cpu(sb->size) > 8*1024*1024*2)
				room = 64*2;
			else
				room = 3*2;
		} else {
			room = __le64_to_cpu(sb->data_offset)
				- __le64_to_cpu(sb->super_offset);
			if (1 || __le32_to_cpu(sb->max_dev) <= 384) {
				room -= 2;
				offset = 2;
			} else {
				room -= 4*2;
				offset = 4*2;
			}
		}
		break;
	}

	if (chunk == UnSet && room > 128*2)
		/* Limit to 128K of bitmap when chunk size not requested */
		room = 128*2;

	max_bits = (room * 512 - sizeof(bitmap_super_t)) * 8;

	min_chunk = 4096; /* sub-page chunks don't work yet.. */
	bits = (size*512)/min_chunk +1;
	while (bits > max_bits) {
		min_chunk *= 2;
		bits = (bits+1)/2;
	}
	if (chunk == UnSet)
		chunk = min_chunk;
	else if (chunk < min_chunk)
		return 0; /* chunk size too small */
	if (chunk == 0) /* rounding problem */
		return 0;

	if (offset == 0) {
		bits = (size*512) / chunk + 1;
		room = ((bits+7)/8 + sizeof(bitmap_super_t) +511)/512;
		offset = -room;
	}

	sb->bitmap_offset = __cpu_to_le32(offset);

	sb->feature_map = __cpu_to_le32(__le32_to_cpu(sb->feature_map) | 1);
	memset(bms, 0, sizeof(*bms));
	bms->magic = __cpu_to_le32(BITMAP_MAGIC);
	bms->version = __cpu_to_le32(major);
	uuid_from_super1((int*)bms->uuid, sb);
	bms->chunksize = __cpu_to_le32(chunk);
	bms->daemon_sleep = __cpu_to_le32(delay);
	bms->sync_size = __cpu_to_le64(size);
	bms->write_behind = __cpu_to_le32(write_behind);

	*chunkp = chunk;
	return 1;
}


static void locate_bitmap1(struct supertype *st, int fd, void *sbv)
{
	unsigned long long offset;
	struct mdp_superblock_1 *sb;
	int mustfree = 0;

	if (!sbv) {
		if (st->ss->load_super(st, fd, &sbv, NULL))
			return; /* no error I hope... */
		mustfree = 1;
	}
	sb = sbv;

	offset = __le64_to_cpu(sb->super_offset);
	offset += (long) __le32_to_cpu(sb->bitmap_offset);
	if (mustfree)
		free(sb);
	lseek64(fd, offset<<9, 0);
}

static int write_bitmap1(struct supertype *st, int fd, void *sbv)
{
	struct mdp_superblock_1 *sb = sbv;
	bitmap_super_t *bms = (bitmap_super_t*)(((char*)sb)+1024);
	int rv = 0;

	int towrite, n;
	char buf[4096];

	locate_bitmap1(st, fd, sbv);

	if (write(fd, ((char*)sb)+1024, sizeof(bitmap_super_t)) !=
	    sizeof(bitmap_super_t))
		return -2;
	towrite = __le64_to_cpu(bms->sync_size) / (__le32_to_cpu(bms->chunksize)>>9);
	towrite = (towrite+7) >> 3; /* bits to bytes */
	memset(buf, 0xff, sizeof(buf));
	while (towrite > 0) {
		n = towrite;
		if (n > sizeof(buf))
			n = sizeof(buf);
		n = write(fd, buf, n);
		if (n > 0)
			towrite -= n;
		else
			break;
	}
	fsync(fd);
	if (towrite)
		rv = -2;

	return rv;
}

struct superswitch super1 = {
#ifndef MDASSEMBLE
	.examine_super = examine_super1,
	.brief_examine_super = brief_examine_super1,
	.detail_super = detail_super1,
	.brief_detail_super = brief_detail_super1,
#endif
	.match_home = match_home1,
	.uuid_from_super = uuid_from_super1,
	.getinfo_super = getinfo_super1,
	.update_super = update_super1,
	.init_super = init_super1,
	.add_to_super = add_to_super1,
	.store_super = store_super1,
	.write_init_super = write_init_super1,
	.compare_super = compare_super1,
	.load_super = load_super1,
	.match_metadata_desc = match_metadata_desc1,
	.avail_size = avail_size1,
	.add_internal_bitmap = add_internal_bitmap1,
	.locate_bitmap = locate_bitmap1,
	.write_bitmap = write_bitmap1,
	.major = 1,
#if __BYTE_ORDER == BIG_ENDIAN
	.swapuuid = 0,
#else
	.swapuuid = 1,
#endif
};
