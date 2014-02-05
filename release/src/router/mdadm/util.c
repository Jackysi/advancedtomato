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
#include	<sys/utsname.h>
#include	<ctype.h>

/*
 * following taken from linux/blkpg.h because they aren't
 * anywhere else and it isn't safe to #include linux/ * stuff.
 */

#define BLKPG      _IO(0x12,105)

/* The argument structure */
struct blkpg_ioctl_arg {
        int op;
        int flags;
        int datalen;
        void *data;
};

/* The subfunctions (for the op field) */
#define BLKPG_ADD_PARTITION	1
#define BLKPG_DEL_PARTITION	2

/* Sizes of name fields. Unused at present. */
#define BLKPG_DEVNAMELTH	64
#define BLKPG_VOLNAMELTH	64

/* The data structure for ADD_PARTITION and DEL_PARTITION */
struct blkpg_partition {
	long long start;		/* starting offset in bytes */
	long long length;		/* length in bytes */
	int pno;			/* partition number */
	char devname[BLKPG_DEVNAMELTH];	/* partition name, like sda5 or c0d1p2,
					   to be used in kernel messages */
	char volname[BLKPG_VOLNAMELTH];	/* volume label */
};

/*
 * Parse a 128 bit uuid in 4 integers
 * format is 32 hexx nibbles with options :.<space> separator
 * If not exactly 32 hex digits are found, return 0
 * else return 1
 */
int parse_uuid(char *str, int uuid[4])
{
    int hit = 0; /* number of Hex digIT */
    int i;
    char c;
    for (i=0; i<4; i++) uuid[i]=0;

    while ((c= *str++)) {
	int n;
	if (c>='0' && c<='9')
	    n = c-'0';
	else if (c>='a' && c <= 'f')
	    n = 10 + c - 'a';
	else if (c>='A' && c <= 'F')
	    n = 10 + c - 'A';
	else if (strchr(":. -", c))
	    continue;
	else return 0;

	if (hit<32) {
	    uuid[hit/8] <<= 4;
	    uuid[hit/8] += n;
	}
	hit++;
    }
    if (hit == 32)
	return 1;
    return 0;
    
}


/*
 * Get the md version number.
 * We use the RAID_VERSION ioctl if it is supported
 * If not, but we have a block device with major '9', we assume
 * 0.36.0
 *
 * Return version number as 24 but number - assume version parts
 * always < 255
 */

int md_get_version(int fd)
{
    struct stat stb;
    mdu_version_t vers;

    if (fstat(fd, &stb)<0)
	return -1;
    if ((S_IFMT&stb.st_mode) != S_IFBLK)
	return -1;

    if (ioctl(fd, RAID_VERSION, &vers) == 0)
	return  (vers.major*10000) + (vers.minor*100) + vers.patchlevel;
    if (errno == EACCES)
	    return -1;
    if (major(stb.st_rdev) == MD_MAJOR)
	return (3600);
    return -1;
}

    
int get_linux_version()
{
	struct utsname name;
	char *cp;
	int a,b,c;
	if (uname(&name) <0)
		return -1;

	cp = name.release;
	a = strtoul(cp, &cp, 10);
	if (*cp != '.') return -1;
	b = strtoul(cp+1, &cp, 10);
	if (*cp != '.') return -1;
	c = strtoul(cp+1, NULL, 10);

	return (a*1000000)+(b*1000)+c;
}

void remove_partitions(int fd)
{
	/* remove partitions from this block devices.
	 * This is used for components added to an array
	 */
#ifdef BLKPG_DEL_PARTITION
	struct blkpg_ioctl_arg a;
	struct blkpg_partition p;

	a.op = BLKPG_DEL_PARTITION;
	a.data = (void*)&p;
	a.datalen = sizeof(p);
	a.flags = 0;
	memset(a.data, 0, a.datalen);
	for (p.pno=0; p.pno < 16; p.pno++)
		ioctl(fd, BLKPG, &a);
#endif
}

int enough(int level, int raid_disks, int layout, int clean,
	   char *avail, int avail_disks)
{
	int copies, first;
	switch (level) {
	case 10:
		/* This is the tricky one - we need to check
		 * which actual disks are present.
		 */
		copies = (layout&255)* ((layout>>8) & 255);
		first=0;
		do {
			/* there must be one of the 'copies' form 'first' */
			int n = copies;
			int cnt=0;
			while (n--) {
				if (avail[first])
					cnt++;
				first = (first+1) % raid_disks;
			}
			if (cnt == 0)
				return 0;

		} while (first != 0);
		return 1;

	case -4:
		return avail_disks>= 1;
	case -1:
	case 0:
		return avail_disks == raid_disks;
	case 1:
		return avail_disks >= 1;
	case 4:
	case 5:
		if (clean)
			return avail_disks >= raid_disks-1;
		else
			return avail_disks >= raid_disks;
	case 6:
		if (clean)
			return avail_disks >= raid_disks-2;
		else
			return avail_disks >= raid_disks;
	default:
		return 0;
	}
}

int same_uuid(int a[4], int b[4], int swapuuid)
{
	if (swapuuid) {
		/* parse uuids are hostendian.
		 * uuid's from some superblocks are big-ending
		 * if there is a difference, we need to swap.. 
		 */
		unsigned char *ac = (unsigned char *)a;
		unsigned char *bc = (unsigned char *)b;
		int i;
		for (i=0; i<16; i+= 4) {
			if (ac[i+0] != bc[i+3] ||
			    ac[i+1] != bc[i+2] ||
			    ac[i+2] != bc[i+1] ||
			    ac[i+3] != bc[i+0])
				return 0;
		}
		return 1;
	} else {
		if (a[0]==b[0] &&
		    a[1]==b[1] &&
		    a[2]==b[2] &&
		    a[3]==b[3])
			return 1;
		return 0;
	}
}
void copy_uuid(void *a, int b[4], int swapuuid)
{
	if (swapuuid) {
		/* parse uuids are hostendian.
		 * uuid's from some superblocks are big-ending
		 * if there is a difference, we need to swap..
		 */
		unsigned char *ac = (unsigned char *)a;
		unsigned char *bc = (unsigned char *)b;
		int i;
		for (i=0; i<16; i+= 4) {
			ac[i+0] = bc[i+3];
			ac[i+1] = bc[i+2];
			ac[i+2] = bc[i+1];
			ac[i+3] = bc[i+0];
		}
	} else
		memcpy(a, b, 16);
}

#ifndef MDASSEMBLE
int check_ext2(int fd, char *name)
{
	/*
	 * Check for an ext2fs file system.
	 * Superblock is always 1K at 1K offset
	 *
	 * s_magic is le16 at 56 == 0xEF53
	 * report mtime - le32 at 44
	 * blocks - le32 at 4
	 * logblksize - le32 at 24
	 */
	unsigned char sb[1024];
	time_t mtime;
	int size, bsize;
	if (lseek(fd, 1024,0)!= 1024)
		return 0;
	if (read(fd, sb, 1024)!= 1024)
		return 0;
	if (sb[56] != 0x53 || sb[57] != 0xef)
		return 0;

	mtime = sb[44]|(sb[45]|(sb[46]|sb[47]<<8)<<8)<<8;
	bsize = sb[24]|(sb[25]|(sb[26]|sb[27]<<8)<<8)<<8;
	size = sb[4]|(sb[5]|(sb[6]|sb[7]<<8)<<8)<<8;
	fprintf(stderr, Name ": %s appears to contain an ext2fs file system\n",
		name);
	fprintf(stderr,"    size=%dK  mtime=%s",
		size*(1<<bsize), ctime(&mtime));
	return 1;
}

int check_reiser(int fd, char *name)
{
	/*
	 * superblock is at 64K
	 * size is 1024;
	 * Magic string "ReIsErFs" or "ReIsEr2Fs" at 52
	 *
	 */
	unsigned char sb[1024];
	unsigned long size;
	if (lseek(fd, 64*1024, 0) != 64*1024)
		return 0;
	if (read(fd, sb, 1024) != 1024)
		return 0;
	if (strncmp((char*)sb+52, "ReIsErFs",8)!=0 &&
	    strncmp((char*)sb+52, "ReIsEr2Fs",9)!=0)
		return 0;
	fprintf(stderr, Name ": %s appears to contain a reiserfs file system\n",name);
	size = sb[0]|(sb[1]|(sb[2]|sb[3]<<8)<<8)<<8;
	fprintf(stderr, "    size = %luK\n", size*4);
		
	return 1;
}

int check_raid(int fd, char *name)
{
	void *super;
	struct mdinfo info;
	time_t crtime;
	char *level;
	struct supertype *st = guess_super(fd);

	if (!st) return 0;
	st->ss->load_super(st, fd, &super, name);
	/* Looks like a raid array .. */
	fprintf(stderr, Name ": %s appears to be part of a raid array:\n",
		name);
	st->ss->getinfo_super(&info, super);
	free(super);
	crtime = info.array.ctime;
	level = map_num(pers, info.array.level);
	if (!level) level = "-unknown-";
	fprintf(stderr, "    level=%s devices=%d ctime=%s",
		level, info.array.raid_disks, ctime(&crtime));
	return 1;
}

int ask(char *mesg)
{
	char *add = "";
	int i;
	for (i=0; i<5; i++) {
		char buf[100];
		fprintf(stderr, "%s%s", mesg, add);
		fflush(stderr);
		if (fgets(buf, 100, stdin)==NULL)
			return 0;
		if (buf[0]=='y' || buf[0]=='Y')
			return 1;
		if (buf[0]=='n' || buf[0]=='N')
			return 0;
		add = "(y/n) ";
	}
	fprintf(stderr, Name ": assuming 'no'\n");
	return 0;
}
#endif /* MDASSEMBLE */

char *map_num(mapping_t *map, int num)
{
	while (map->name) {
		if (map->num == num)
			return map->name;
		map++;
	}
	return NULL;
}

int map_name(mapping_t *map, char *name)
{
	while (map->name) {
		if (strcmp(map->name, name)==0)
			return map->num;
		map++;
	}
	return UnSet;
}


int is_standard(char *dev, int *nump)
{
	/* tests if dev is a "standard" md dev name.
	 * i.e if the last component is "/dNN" or "/mdNN",
	 * where NN is a string of digits 
	 */
	char *d = strrchr(dev, '/');
	int type=0;
	int num;
	if (!d)
		return 0;
	if (strncmp(d, "/d",2)==0)
		d += 2, type=1; /* /dev/md/dN{pM} */
	else if (strncmp(d, "/md_d", 5)==0)
		d += 5, type=1; /* /dev/md_dNpM */
	else if (strncmp(d, "/md", 3)==0)
		d += 3, type=-1; /* /dev/mdN */
	else if (d-dev > 3 && strncmp(d-2, "md/", 3)==0)
		d += 1, type=-1; /* /dev/md/N */
	else
		return 0;
	if (!*d)
		return 0;
	num = atoi(d);
	while (isdigit(*d))
		d++;
	if (*d)
		return 0;
	if (nump) *nump = num;

	return type;
}


/*
 * convert a major/minor pair for a block device into a name in /dev, if possible.
 * On the first call, walk /dev collecting name.
 * Put them in a simple linked listfor now.
 */
struct devmap {
    int major, minor;
    char *name;
    struct devmap *next;
} *devlist = NULL;
int devlist_ready = 0;

int add_dev(const char *name, const struct stat *stb, int flag, struct FTW *s)
{
	struct stat st;
	if (S_ISLNK(stb->st_mode)) {
		stat(name, &st);
		stb = &st;
	}

	if ((stb->st_mode&S_IFMT)== S_IFBLK) {
		char *n = strdup(name);
		struct devmap *dm = malloc(sizeof(*dm));
		if (strncmp(n, "/dev/./", 7)==0)
			strcpy(n+4, name+6);
		if (dm) {
			dm->major = major(stb->st_rdev);
			dm->minor = minor(stb->st_rdev);
			dm->name = n;
			dm->next = devlist;
			devlist = dm;
		}
	}
	return 0;
}

#ifndef HAVE_NFTW
#ifdef HAVE_FTW
int add_dev_1(const char *name, const struct stat *stb, int flag)
{
	return add_dev(name, stb, flag, NULL);
}
int nftw(const char *path, int (*han)(const char *name, const struct stat *stb, int flag, struct FTW *s), int nopenfd, int flags)
{
	return ftw(path, add_dev_1, nopenfd);
}
#else
int add_dev(const char *name, const struct stat *stb, int flag, struct FTW *s)
{
	return 0;
}
int nftw(const char *path, int (*han)(const char *name, const struct stat *stb, int flag, struct FTW *s), int nopenfd, int flags)
{
	return 0;
}
#endif /* HAVE_FTW */
#endif /* HAVE_NFTW */

/*
 * Find a block device with the right major/minor number.
 * If we find multiple names, choose the shortest.
 * If we find a non-standard name, it is probably there
 * deliberately so prefer it over a standard name.
 * This applies only to names for MD devices.
 */
char *map_dev(int major, int minor, int create)
{
	struct devmap *p;
	char *std = NULL, *nonstd=NULL;
	int did_check = 0;

	if (major == 0 && minor == 0)
			return NULL;

 retry:
	if (!devlist_ready) {
		char *dev = "/dev";
		struct stat stb;
		while(devlist) {
			struct devmap *d = devlist;
			devlist = d->next;
			free(d->name);
			free(d);
		}
		if (lstat(dev, &stb)==0 &&
		    S_ISLNK(stb.st_mode))
			dev = "/dev/.";
		nftw(dev, add_dev, 10, FTW_PHYS);
		devlist_ready=1;
		did_check = 1;
	}

	for (p=devlist; p; p=p->next)
		if (p->major == major &&
		    p->minor == minor) {
			if (is_standard(p->name, NULL)) {
				if (std == NULL ||
				    strlen(p->name) < strlen(std))
					std = p->name;
			} else {
				if (nonstd == NULL ||
				    strlen(p->name) < strlen(nonstd))
					nonstd = p->name;
			}
		}
	if (!std && !nonstd && !did_check) {
		devlist_ready = 0;
		goto retry;
	}
	if (create && !std && !nonstd) {
		static char buf[30];
		snprintf(buf, sizeof(buf), "%d:%d", major, minor);
		nonstd = buf;
	}

	return nonstd ? nonstd : std;
}

unsigned long calc_csum(void *super, int bytes)
{
	unsigned long long newcsum = 0;
	int i;
	unsigned int csum;
	unsigned int *superc = (unsigned int*) super;

	for(i=0; i<bytes/4; i++)
		newcsum+= superc[i];
	csum = (newcsum& 0xffffffff) + (newcsum>>32);
#ifdef __alpha__
/* The in-kernel checksum calculation is always 16bit on 
 * the alpha, though it is 32 bit on i386...
 * I wonder what it is elsewhere... (it uses and API in
 * a way that it shouldn't).
 */
	csum = (csum & 0xffff) + (csum >> 16);
	csum = (csum & 0xffff) + (csum >> 16);
#endif
	return csum;
}

#ifndef MDASSEMBLE
char *human_size(long long bytes)
{
	static char buf[30];

	/* We convert bytes to either centi-M{ega,ibi}bytes or
	 * centi-G{igi,ibi}bytes, with appropriate rounding,
	 * and then print 1/100th of those as a decimal.
	 * We allow upto 2048Megabytes before converting to
	 * gigabytes, as that shows more precision and isn't
	 * too large a number.
	 * Terrabytes are not yet handled.
	 */

	if (bytes < 5000*1024)
		buf[0]=0;
	else if (bytes < 2*1024LL*1024LL*1024LL) {
		long cMiB = (bytes / ( (1LL<<20) / 200LL ) +1) /2;
		long cMB  = (bytes / ( 1000000LL / 200LL ) +1) /2;
		snprintf(buf, sizeof(buf), " (%ld.%02ld MiB %ld.%02ld MB)",
			cMiB/100 , cMiB % 100,
			cMB/100, cMB % 100);
	} else {
		long cGiB = (bytes / ( (1LL<<30) / 200LL ) +1) /2;
		long cGB  = (bytes / (1000000000LL/200LL ) +1) /2;
		snprintf(buf, sizeof(buf), " (%ld.%02ld GiB %ld.%02ld GB)",
			cGiB/100 , cGiB % 100,
			cGB/100, cGB % 100);
	}
	return buf;
}

char *human_size_brief(long long bytes)
{
	static char buf[30];
	

	if (bytes < 5000*1024)
		snprintf(buf, sizeof(buf), "%ld.%02ldKiB",
			(long)(bytes>>10), (long)(((bytes&1023)*100+512)/1024)
			);
	else if (bytes < 2*1024LL*1024LL*1024LL)
		snprintf(buf, sizeof(buf), "%ld.%02ldMiB",
			(long)(bytes>>20),
			(long)((bytes&0xfffff)+0x100000/200)/(0x100000/100)
			);
	else
		snprintf(buf, sizeof(buf), "%ld.%02ldGiB",
			(long)(bytes>>30),
			(long)(((bytes>>10)&0xfffff)+0x100000/200)/(0x100000/100)
			);
	return buf;
}
#endif

#if !defined(MDASSEMBLE) || defined(MDASSEMBLE) && defined(MDASSEMBLE_AUTO)
int get_mdp_major(void)
{
static int mdp_major = -1;
	FILE *fl;
	char *w;
	int have_block = 0;
	int have_devices = 0;
	int last_num = -1;

	if (mdp_major != -1)
		return mdp_major;
	fl = fopen("/proc/devices", "r");
	if (!fl)
		return -1;
	while ((w = conf_word(fl, 1))) {
		if (have_block && strcmp(w, "devices:")==0)
			have_devices = 1;
		have_block =  (strcmp(w, "Block")==0);
		if (isdigit(w[0]))
			last_num = atoi(w);
		if (have_devices && strcmp(w, "mdp")==0)
			mdp_major = last_num;
		free(w);
	}
	fclose(fl);
	return mdp_major;
}



char *get_md_name(int dev)
{
	/* find /dev/md%d or /dev/md/%d or make a device /dev/.tmp.md%d */
	/* if dev < 0, want /dev/md/d%d or find mdp in /proc/devices ... */
	static char devname[50];
	struct stat stb;
	dev_t rdev;
	char *dn;

	if (dev < 0) {
		int mdp =  get_mdp_major();
		if (mdp < 0) return NULL;
		rdev = makedev(mdp, (-1-dev)<<6);
		snprintf(devname, sizeof(devname), "/dev/md/d%d", -1-dev);
		if (stat(devname, &stb) == 0
		    && (S_IFMT&stb.st_mode) == S_IFBLK
		    && (stb.st_rdev == rdev))
			return devname;
	} else {
		rdev = makedev(MD_MAJOR, dev);
		snprintf(devname, sizeof(devname), "/dev/md%d", dev);
		if (stat(devname, &stb) == 0
		    && (S_IFMT&stb.st_mode) == S_IFBLK
		    && (stb.st_rdev == rdev))
			return devname;

		snprintf(devname, sizeof(devname), "/dev/md/%d", dev);
		if (stat(devname, &stb) == 0
		    && (S_IFMT&stb.st_mode) == S_IFBLK
		    && (stb.st_rdev == rdev))
			return devname;
	}
	dn = map_dev(major(rdev), minor(rdev), 0);
	if (dn)
		return dn;
	snprintf(devname, sizeof(devname), "/dev/.tmp.md%d", dev);
	if (mknod(devname, S_IFBLK | 0600, rdev) == -1)
		if (errno != EEXIST)
			return NULL;

	if (stat(devname, &stb) == 0
	    && (S_IFMT&stb.st_mode) == S_IFBLK
	    && (stb.st_rdev == rdev))
		return devname;
	unlink(devname);
	return NULL;
}

void put_md_name(char *name)
{
	if (strncmp(name, "/dev/.tmp.md", 12)==0)
		unlink(name);
}
#endif /* !defined(MDASSEMBLE) || defined(MDASSEMBLE) && defined(MDASSEMBLE_AUTO) */

int dev_open(char *dev, int flags)
{
	/* like 'open', but if 'dev' matches %d:%d, create a temp
	 * block device and open that
	 */
	char *e;
	int fd = -1;
	char devname[32];
	int major;
	int minor;

	if (!dev) return -1;

	major = strtoul(dev, &e, 0);
	if (e > dev && *e == ':' && e[1] &&
	    (minor = strtoul(e+1, &e, 0)) >= 0 &&
	    *e == 0) {
		snprintf(devname, sizeof(devname), "/dev/.tmp.md.%d:%d", major, minor);
		if (mknod(devname, S_IFBLK|0600, makedev(major, minor))==0) {
			fd = open(devname, flags);
			unlink(devname);
		}
	} else
		fd = open(dev, flags);
	return fd;
}

struct superswitch *superlist[] = { &super0, &super1, NULL };

struct supertype *super_by_version(int vers, int minor)
{
	struct supertype *st = malloc(sizeof(*st));
	if (!st) return st;
	if (vers == 0) {
		st->ss = &super0;
		st->max_devs = MD_SB_DISKS;
	}

	if (vers == 1) {
		st->ss = &super1;
		st->max_devs = 384;
	}
	st->minor_version = minor;
	return st;
}

struct supertype *guess_super(int fd)
{
	/* try each load_super to find the best match,
	 * and return the best superswitch
	 */
	struct superswitch  *ss;
	struct supertype *st;
	unsigned long besttime = 0;
	int bestsuper = -1;
	
	void *sbp = NULL;
	int i;

	st = malloc(sizeof(*st));
	memset(st, 0, sizeof(*st));
	for (i=0 ; superlist[i]; i++) {
		int rv;
		ss = superlist[i];
		st->ss = NULL;
		rv = ss->load_super(st, fd, &sbp, NULL);
		if (rv == 0) {
			struct mdinfo info;
			ss->getinfo_super(&info, sbp);
			if (bestsuper == -1 ||
			    besttime < info.array.ctime) {
				bestsuper = i;
				besttime = info.array.ctime;
			}
			free(sbp);
		}
	}
	if (bestsuper != -1) {
		int rv;
		st->ss = NULL;
		rv = superlist[bestsuper]->load_super(st, fd, &sbp, NULL);
		if (rv == 0) {
			free(sbp);
			return st;
		}
	}
	free(st);
	return NULL;
}

/* Return size of device in bytes */
int get_dev_size(int fd, char *dname, unsigned long long *sizep)
{
	unsigned long long ldsize;
#ifdef BLKGETSIZE64
	if (ioctl(fd, BLKGETSIZE64, &ldsize) != 0)
#endif
	{
		unsigned long dsize;
		if (ioctl(fd, BLKGETSIZE, &dsize) == 0) {
			ldsize = dsize;
			ldsize <<= 9;
		} else {
			if (dname)
				fprintf(stderr, Name ": Cannot get size of %s: %s\b",
					dname, strerror(errno));
			return 0;
		}
	}
	*sizep = ldsize;
	return 1;
}

void get_one_disk(int mdfd, mdu_array_info_t *ainf, mdu_disk_info_t *disk)
{
	int d;
	ioctl(mdfd, GET_ARRAY_INFO, ainf);
	for (d = 0 ; d < ainf->raid_disks + ainf->nr_disks ; d++)
		if (ioctl(mdfd, GET_DISK_INFO, disk) == 0)
			return;
}
#ifdef __TINYC__
/* tinyc doesn't optimize this check in ioctl.h out ... */
unsigned int __invalid_size_argument_for_IOC = 0;
#endif

