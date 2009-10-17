/*

	Tomato Firmware
	USB Support Module

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"


/* Serialize using fcntl() calls 
 */

int file_lock(char *tag)
{
	char fn[64];
	struct flock lock;
	int lockfd = -1;
	pid_t lockpid;

	sprintf(fn, "/var/lock/%s.lock", tag);
	if ((lockfd = open(fn, O_CREAT | O_RDWR, 0666)) < 0)
		goto lock_error;

	pid_t pid = getpid();
	if (read(lockfd, &lockpid, sizeof(pid_t))) {
		// check if we already hold a lock
		if (pid == lockpid) {
			// don't close the file here as that will release all locks
			return -1;
		}
	}

	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_pid = pid;

	if (fcntl(lockfd, F_SETLKW, &lock) < 0) {
		close(lockfd);
		goto lock_error;
	}

	lseek(lockfd, 0, SEEK_SET);
	write(lockfd, &pid, sizeof(pid_t));
	return lockfd;
lock_error:
	// No proper error processing
	syslog(LOG_DEBUG, "Error %d locking %s, proceeding anyway", errno, fn);
	return -1;
}

void file_unlock(int lockfd)
{
	if (lockfd >= 0) {
		ftruncate(lockfd, 0);
		close(lockfd);
	}
}

char *detect_fs_type(char *device)
{
	int fd;
	unsigned char buf[4096];

	if ((fd = open(device, O_RDONLY)) < 0)
		return NULL;

	if (read(fd, buf, sizeof(buf)) != sizeof(buf))
	{
		close(fd);
		return NULL;
	}

	close(fd);

	/* first check for mbr */
	if (*device && device[strlen(device) - 1] > '9' &&
		buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
		((buf[0x1be] | buf[0x1ce] | buf[0x1de] | buf[0x1ee]) & 0x7f) == 0) /* boot flags */ 
	{
		return "mbr";
	} 
	/* detect swap */
	else if (memcmp(buf + 4086, "SWAPSPACE2", 10) == 0 ||
		memcmp(buf + 4086, "SWAP-SPACE", 10) == 0)
	{
		return "swap";
	}
	/* detect ext2/3 */
	else if (buf[0x438] == 0x53 && buf[0x439] == 0xEF)
	{
		return ((buf[0x460] & 0x0008 /* JOURNAL_DEV */) != 0 ||
			(buf[0x45c] & 0x0004 /* HAS_JOURNAL */) != 0) ? "ext3" : "ext2";
	}
	/* detect ntfs */
	else if (buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
		memcmp(buf + 3, "NTFS    ", 8) == 0)
	{
		return "ntfs";
	}
	/* detect vfat */
	else if (buf[510] == 0x55 && buf[511] == 0xAA && /* signature */
		buf[11] == 0 && buf[12] >= 1 && buf[12] <= 8 /* sector size 512 - 4096 */ &&
		buf[13] != 0 && (buf[13] & (buf[13] - 1)) == 0) /* sectors per cluster */
	{
		return "vfat";
	}

	return NULL;
}


/* Execute a function for each disc partition on the specified controller.
 *
 * Directory /dev/discs/ looks like this:
 * disc0 -> ../scsi/host0/bus0/target0/lun0/
 * disc1 -> ../scsi/host1/bus0/target0/lun0/
 * disc2 -> ../scsi/host2/bus0/target0/lun0/
 * disc3 -> ../scsi/host2/bus0/target0/lun1/
 *
 * Scsi host 2 supports multiple drives.
 * Scsi host 0 & 1 support one drive.
 *
 * For attached drives, like this.  If not attached, there is no "part#" item.
 * Here, only one drive, with 2 partitions, is plugged in.
 * /dev/discs/disc0/disc
 * /dev/discs/disc0/part1
 * /dev/discs/disc0/part2
 * /dev/discs/disc1/disc
 * /dev/discs/disc2/disc
 *
 * Which is the same as:
 * /dev/scsi/host0/bus0/target0/lun0/disc
 * /dev/scsi/host0/bus0/target0/lun0/part1
 * /dev/scsi/host0/bus0/target0/lun0/part2
 * /dev/scsi/host1/bus0/target0/lun0/disc
 * /dev/scsi/host2/bus0/target0/lun0/disc
 * /dev/scsi/host2/bus0/target0/lun1/disc
 *
 * Implementation notes:
 * Various mucking about with a disc that just got plugged in or unplugged
 * will make the scsi subsystem try a re-validate, and read the partition table of the disc.
 * This will make sure the partitions show up.
 *
 * It appears to try to do the revalidate and re-read & update the partition
 * information when this code does the "readdir of /dev/discs/disc0/?".  If the
 * disc has any mounted partitions the revalidate will be rejected.  So the
 * current partition info will remain.  On an unplug event, when it is doing the
 * readdir's, it will try to do the revalidate as we are doing the readdir's.
 * But luckily they'll be rejected, otherwise the later partitions will disappear as
 * soon as we get the first one.
 * But be very careful!  If something goes not exactly right, the partition entries
 * will disappear before we've had a chance to unmount from them.
 *
 * To avoid this automatic revalidation, we go through /proc/partitions looking for the partitions
 * that /dev/discs point to.  That will avoid the implicit revalidate attempt.
 * Which means that we had better do it ourselves.  An ioctl BLKRRPART does just that.
 * 
 * 
 * If host < 0, do all hosts.   If >= 0, it is the host number to do.
 * When_to_update, flags:
 *	0x01 = before reading partition info
 *	0x02 = after reading partition info
 *
 */

/* So as not to include linux/fs.h, let's explicitly do this here. */
#ifndef BLKRRPART
#define BLKRRPART	_IO(0x12,95)	/* re-read partition table */
#endif

int exec_for_host(int host, int when_to_update, uint flags, host_exec func)
{
	DIR *usb_dev_disc;
	char bfr[128];	/* Will be: /dev/discs/disc#					*/
	char link[256];	/* Will be: ../scsi/host#/bus0/target0/lun#  that bfr links to. */
			/* When calling the func, will be: /dev/discs/disc#/part#	*/
	char bfr2[128];	/* Will be: /dev/discs/disc#/disc     for the BLKRRPART.	*/
	int fd;
	char *cp;
	int len;
	int host_no;	/* SCSI controller/host # */
	int disc_num;	/* Disc # */
	int part_num;	/* Parition # */
	struct dirent *dp;
	FILE *prt_fp;
	char *mp;	/* Ptr to after any leading ../ path */
	int siz;
	char line[256];
	int result = 0;

	flags |= EFH_1ST_HOST;
	if ((usb_dev_disc = opendir(DEV_DISCS_ROOT))) {
		while ((dp = readdir(usb_dev_disc))) {
			sprintf(bfr, "%s/%s", DEV_DISCS_ROOT, dp->d_name);
			if (strncmp(dp->d_name, "disc", 4) != 0)
				continue;

			disc_num = atoi(dp->d_name + 4);
			len = readlink(bfr, link, sizeof(link) - 1);
			if (len < 0)
				continue;

			link[len] = 0;
			cp = strstr(link, "/scsi/host");
			if (!cp)
				continue;

			host_no = atoi(cp + 10);
			if (host >= 0 && host_no != host)
				continue;

			/* We have found a disc that is on this controller.
			 * Loop thru all the partitions on this disc.
			 * The new way, reading thru /proc/partitions.
			 */
			mp = link;
			if ((cp = strstr(link, "../")) != NULL)
				mp = cp + 3;
			siz = strlen(mp);

			if (when_to_update & 0x01) {
				sprintf(bfr2, "%s/disc", bfr);	/* Prepare for BLKRRPART */
				if ((fd = open(bfr2, O_RDONLY | O_NONBLOCK)) >= 0) {
					ioctl(fd, BLKRRPART);
					close(fd);
				}
			}

			flags |= EFH_1ST_DISC;
			if (func && (prt_fp = fopen("/proc/partitions", "r"))) {
				while (fgets(line, sizeof(line) - 2, prt_fp)) {
					if (sscanf(line, " %*s %*s %*s %s", bfr2) == 1) {
						if ((cp = strstr(bfr2, "/part")) && strncmp(bfr2, mp, siz) == 0) {
							part_num = atoi(cp + 5);
							sprintf(line, "%s/part%d", bfr, part_num);
							result = (*func)(line, host_no, disc_num, part_num, flags) || result;
							flags &= ~(EFH_1ST_HOST | EFH_1ST_DISC);
						}
					}
				}
				fclose(prt_fp);
			}

			if (when_to_update & 0x02) {
				sprintf(bfr2, "%s/disc", bfr);	/* Prepare for BLKRRPART */
				if ((fd = open(bfr2, O_RDONLY | O_NONBLOCK)) >= 0) {
					ioctl(fd, BLKRRPART);
					close(fd);
				}
			}
		}
		closedir(usb_dev_disc);
	}
	return result;
}

/* Concept taken from the e2fsprogs/ismounted.c.
 * Find wherever 'file' (actually: device) is mounted.
 * Either the exact same device-name, or another device-name.
 * The latter is detected by comparing the rdev or dev&inode.
 * So aliasing won't fool us---we'll still find if it's mounted.
 * Return its mnt entry.
 * In particular, the caller would look at the mnt->mountpoint.
 *
 * Find the matching devname(s) in mounts or swaps.
 * If func is supplied, call it for each match.  If not, return mnt on the first match.
 */

static inline int is_same_device(char *fsname, dev_t file_rdev, dev_t file_dev, ino_t file_ino)
{
	struct stat st_buf;

	if (stat(fsname, &st_buf) == 0) {
		if (S_ISBLK(st_buf.st_mode)) {
			if (file_rdev && (file_rdev == st_buf.st_rdev))
				return 1;
		}
		else {
			if (file_dev && ((file_dev == st_buf.st_dev) &&
				(file_ino == st_buf.st_ino)))
				return 1;
			/* Check for [swap]file being on the device. */
			if (file_dev == 0 && file_ino == 0 && file_rdev == st_buf.st_dev)
				return 1;
		}
	}
	return 0;
}


struct mntent *findmntents(char *file, int swp, int (*func)(struct mntent *mnt, uint flags), uint flags)
{
	struct mntent 	*mnt;
	struct stat	st_buf;
	dev_t		file_dev=0, file_rdev=0;
	ino_t		file_ino=0;
	FILE 		*f;
	
	if ((f = setmntent(swp? "/proc/swaps": "/proc/mounts", "r")) == NULL)
		return NULL;

	if (stat(file, &st_buf) == 0) {
		if (S_ISBLK(st_buf.st_mode)) {
			file_rdev = st_buf.st_rdev;
		}
		else {
			file_dev = st_buf.st_dev;
			file_ino = st_buf.st_ino;
		}
	}
	while ((mnt = getmntent(f)) != NULL) {
		if (strcmp(file, mnt->mnt_fsname) == 0 ||
			is_same_device(mnt->mnt_fsname, file_rdev , file_dev, file_ino)) {
			if (func == NULL)
				break;
			(*func)(mnt, flags);
		}
	}

	endmntent(f);
	return mnt;
}


//#define SAME_AS_KERNEL
/* Simulate a hotplug event, as if a USB storage device
 * got plugged or unplugged.
 * Either use a hardcoded program name, or the same
 * hotplug program that the kernel uses for a real event.
 */
void add_remove_usbhost(char *host, int add)
{
	setenv("ACTION", add ? "add" : "remove", 1);
	setenv("SCSI_HOST", host, 1);
	setenv("PRODUCT", host, 1);
	setenv("INTERFACE", "TOMATO/0", 1);
#ifdef SAME_AS_KERNEL
	char pgm[256] = "/sbin/hotplug usb";
	int fd = open("/proc/sys/kernel/hotplug", O_RDONLY);
	if (fd) {
		if (read(fd, pgm, sizeof(pgm) - 5) >= 0) {
			if ((p = strchr(pgm, '\n')) != NULL)
				*p = 0;
			strcat(pgm, " usb");
		}
		close(fd);
	}
	system(pgm);
#else
	// don't use value from /proc/sys/kernel/hotplug 
	// since it may be overriden by a user.
	system("/sbin/hotplug usb");
#endif
	unsetenv("INTERFACE");
	unsetenv("PRODUCT");
	unsetenv("SCSI_HOST");
	unsetenv("ACTION");
}


/****************************************************/
/* Use busybox routines to get labels for fat & ext */
/* Probe for label the same way that mount does.    */
/****************************************************/

#define VOLUME_ID_LABEL_SIZE		64
#define VOLUME_ID_UUID_SIZE		36
#define SB_BUFFER_SIZE			0x11000

struct volume_id {
	int		fd;
	int		error;
	size_t		sbbuf_len;
	size_t		seekbuf_len;
	uint8_t		*sbbuf;
	uint8_t		*seekbuf;
	uint64_t	seekbuf_off;
	char		label[VOLUME_ID_LABEL_SIZE+1];
	char		uuid[VOLUME_ID_UUID_SIZE+1];
};

extern void *volume_id_get_buffer(struct volume_id *id, uint64_t off, size_t len);
extern void volume_id_free_buffer(struct volume_id *id);
extern int volume_id_probe_ext(struct volume_id *id);
extern int volume_id_probe_vfat(struct volume_id *id);
extern int volume_id_probe_ntfs(struct volume_id *id);
extern int volume_id_probe_linux_swap(struct volume_id *id);

/* Put the label in *label.
 * Return 0 if no label found, NZ if there is a label.
 */
int find_label(char *dev_name, char *label)
{
	struct volume_id id;

	memset(&id, 0x00, sizeof(id));
	label[0] = '\0';
	if ((id.fd = open(dev_name, O_RDONLY)) < 0)
		return 0;

	if (volume_id_probe_vfat(&id) == 0 || id.error)
		goto ret;

	volume_id_get_buffer(&id, 0, SB_BUFFER_SIZE);

	if (volume_id_probe_ext(&id) == 0 || id.error)
		goto ret;
	if (volume_id_probe_linux_swap(&id) == 0 || id.error)
		goto ret;
	if (volume_id_probe_ntfs(&id) == 0 || id.error)
		goto ret;
ret:
	volume_id_free_buffer(&id);
	if (id.label[0] != '\0')
		strcpy(label, id.label);
	close(id.fd);
	return(label[0] != '\0');
}

void *xmalloc(size_t siz)
{
	return (malloc(siz));
}

void *xrealloc(void *old, size_t size)
{
	return realloc(old, size);
}

void volume_id_set_uuid() {}

ssize_t full_read(int fd, void *buf, size_t len)
{
	return read(fd, buf, len);
}
