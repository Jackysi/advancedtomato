/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> // !!TB
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <dirent.h> //!!TB
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <sys/types.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"

#if 0
#define _dprintf	cprintf
#else
#define _dprintf(args...)	do { } while(0)
#endif


int get_wan_proto(void)
{
	const char *names[] = {	// order must be synced with def at shared.h
		"static",
		"dhcp",
		"l2tp",
		"pppoe",
		"pptp",
		NULL
	};
	int i;
	const char *p;

	p = nvram_safe_get("wan_proto");
	for (i = 0; names[i] != NULL; ++i) {
		if (strcmp(p, names[i]) == 0) return i + 1;
	}
	return WP_DISABLED;
}

int using_dhcpc(void)
{
	switch (get_wan_proto()) {
	case WP_DHCP:
	case WP_L2TP:
		return 1;
	}
	return 0;
}

int wl_client(void)
{
	return ((nvram_match("wl_mode", "sta")) || (nvram_match("wl_mode", "wet")));
}

void notice_set(const char *path, const char *format, ...)
{
	char p[256];
	char buf[2048];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	mkdir("/var/notice", 0755);
	snprintf(p, sizeof(p), "/var/notice/%s", path);
	f_write_string(p, buf, 0, 0);
	if (buf[0]) syslog(LOG_INFO, "notice[%s]: %s", path, buf);
}


//	#define _x_dprintf(args...)	syslog(LOG_DEBUG, args);
#define _x_dprintf(args...)	do { } while (0);

int check_wanup(void)
{
	int up = 0;
	int proto;
	char buf1[64];
	char buf2[64];
	const char *name;
    int f;
    struct ifreq ifr;

	proto = get_wan_proto();
	if (proto == WP_DISABLED) return 0;

	if ((proto == WP_PPTP) || (proto == WP_L2TP) || (proto == WP_PPPOE)) {
		if (f_read_string("/tmp/ppp/link", buf1, sizeof(buf1)) > 0) {
				// contains the base name of a file in /var/run/ containing pid of a daemon
				snprintf(buf2, sizeof(buf2), "/var/run/%s.pid", buf1);
				if (f_read_string(buf2, buf1, sizeof(buf1)) > 0) {
					name = psname(atoi(buf1), buf2, sizeof(buf2));
					if (proto == WP_PPPOE) {
						if (strcmp(name, "pppoecd") == 0) up = 1;
					}
					else {
						if (strcmp(name, "pppd") == 0) up = 1;
					}
				}
				else {
					_dprintf("%s: error reading %s\n", __FUNCTION__, buf2);
				}
			if (!up) {
				unlink("/tmp/ppp/link");
				_x_dprintf("required daemon not found, assuming link is dead\n");
			}
		}
		else {
			_x_dprintf("%s: error reading %s\n", __FUNCTION__, "/tmp/ppp/link");
		}
	}
	else if (!nvram_match("wan_ipaddr", "0.0.0.0")) {
		up = 1;
	}
	else {
		_x_dprintf("%s: default !up\n", __FUNCTION__);
	}

	if ((up) && ((f = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)) {
		strlcpy(ifr.ifr_name, nvram_safe_get("wan_iface"), sizeof(ifr.ifr_name));
		if (ioctl(f, SIOCGIFFLAGS, &ifr) < 0) {
			up = 0;
			_x_dprintf("%s: SIOCGIFFLAGS\n", __FUNCTION__);
		}
		close(f);
		if ((ifr.ifr_flags & IFF_UP) == 0) {
			up = 0;
			_x_dprintf("%s: !IFF_UP\n", __FUNCTION__);
		}
	}

	return up;
}


const dns_list_t *get_dns(void)
{
	static dns_list_t dns;
	char s[512];
	int n;
	int i, j;
	struct in_addr ia;
	char d[4][16];

	dns.count = 0;

	strlcpy(s, nvram_safe_get("wan_dns"), sizeof(s));
	if ((nvram_match("dns_addget", "1")) || (s[0] == 0)) {
		n = strlen(s);
		snprintf(s + n, sizeof(s) - n, " %s", nvram_safe_get("wan_get_dns"));
	}

	n = sscanf(s, "%15s %15s %15s %15s", d[0], d[1], d[2], d[3]);
	for (i = 0; i < n; ++i) {
		if (inet_pton(AF_INET, d[i], &ia) > 0) {
			for (j = dns.count - 1; j >= 0; --j) {
				if (dns.dns[j].s_addr == ia.s_addr) break;
			}
			if (j < 0) {
				dns.dns[dns.count++].s_addr = ia.s_addr;
				if (dns.count == 3) break;
			}
		}
	}

	return &dns;
}

// -----------------------------------------------------------------------------

void set_action(int a)
{
	int r = 3;
	while (f_write("/var/lock/action", &a, sizeof(a), 0, 0) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return;
	}
	if (a != ACT_IDLE) sleep(2);
}

int check_action(void)
{
	int a;
	int r = 3;

	while (f_read("/var/lock/action", &a, sizeof(a)) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return ACT_UNKNOWN;
	}
	return a;
}

int wait_action_idle(int n)
{
	while (n-- > 0) {
		if (check_action() == ACT_IDLE) return 1;
		sleep(1);
	}
	return 0;
}

// -----------------------------------------------------------------------------

const char *get_wanip(void)
{
	const char *p;

	if (!check_wanup()) return "0.0.0.0";
	switch (get_wan_proto()) {
	case WP_DISABLED:
		return "0.0.0.0";
	case WP_PPTP:
		p = "pptp_get_ip";
		break;
	case WP_L2TP:
		p = "l2tp_get_ip";
		break;
	default:
		p = "wan_ipaddr";
		break;
	}
	return nvram_safe_get(p);
}

long get_uptime(void)
{
	struct sysinfo si;
	sysinfo(&si);
	return si.uptime;
}

int get_radio(void)
{
	uint32 n;

	return (wl_ioctl(nvram_safe_get("wl_ifname"), WLC_GET_RADIO, &n, sizeof(n)) == 0) &&
		((n & WL_RADIO_SW_DISABLE)  == 0);
}

void set_radio(int on)
{
	uint32 n;

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
	n = on ? (WL_RADIO_SW_DISABLE << 16) : ((WL_RADIO_SW_DISABLE << 16) | 1);
	wl_ioctl(nvram_safe_get("wl_ifname"), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_WLAN, 0);
		led(LED_DIAG, 0);
	}
#else
	n = on ? 0 : WL_RADIO_SW_DISABLE;
	wl_ioctl(nvram_safe_get("wl_ifname"), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_DIAG, 0);
	}
#endif
}

int nvram_get_int(const char *key)
{
	return atoi(nvram_safe_get(key));
}

/*
long nvram_xget_long(const char *name, long min, long max, long def)
{
	const char *p;
	char *e;
	long n;

	p = nvram_get(name);
	if ((p != NULL) && (*p != 0)) {
		n = strtol(p, &e, 0);
		if ((e != p) && ((*e == 0) || (*e == ' ')) && (n > min) && (n < max)) {
			return n;
		}
	}
	return def;
}
*/

int nvram_get_file(const char *key, const char *fname, int max)
{
	int n;
	char *p;
	char *b;
	int r;

	r = 0;
	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		if ((b = malloc(base64_decoded_len(n) + 128)) != NULL) {
			n = base64_decode(p, b, n);
			if (n > 0) r = (f_write(fname, b, n, 0, 0644) == n);
			free(b);
		}
	}
	return r;
/*
	char b[2048];
	int n;
	char *p;

	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		n = base64_decode(p, b, n);
		if (n > 0) return (f_write(fname, b, n, 0, 0700) == n);
	}
	return 0;
*/
}

int nvram_set_file(const char *key, const char *fname, int max)
{
	char *in;
	char *out;
	long len;
	int n;
	int r;

	if ((len = f_size(fname)) > max) return 0;
	max = (int)len;
	r = 0;
	if (f_read_alloc(fname, &in, max) == max) {
		if ((out = malloc(base64_encoded_len(max) + 128)) != NULL) {
			n = base64_encode(in, out, max);
			out[n] = 0;
			nvram_set(key, out);
			free(out);
			r = 1;
		}
		free(in);
	}
	return r;
/*
	char a[2048];
	char b[4096];
	int n;

	if (((n = f_read(fname, &a, sizeof(a))) > 0) && (n <= max)) {
		n = base64_encode(a, b, n);
		b[n] = 0;
		nvram_set(key, b);
		return 1;
	}
	return 0;
*/
}

int nvram_contains_word(const char *key, const char *word)
{
	return (find_word(nvram_safe_get(key), word) != NULL);
}

int nvram_is_empty(const char *key)
{
	char *p;
	return (((p = nvram_get(key)) == NULL) || (*p == 0));
}

void nvram_commit_x(void)
{
	if (!nvram_get_int("debug_nocommit")) nvram_commit();
}

int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, int timeout)
{
	fd_set fds;
	struct timeval tv;
	int flags;
	int n;
	int r;

	if (((flags = fcntl(fd, F_GETFL, 0)) < 0) ||
		(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)) {
		_dprintf("%s: error in F_*ETFL %d\n", __FUNCTION__, fd);
		return -1;
	}

	if (connect(fd, addr, len) < 0) {
//		_dprintf("%s: connect %d = <0\n", __FUNCTION__, fd);

		if (errno != EINPROGRESS) {
			_dprintf("%s: error in connect %d errno=%d\n", __FUNCTION__, fd, errno);
			return -1;
		}

		while (1) {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			r = select(fd + 1, NULL, &fds, NULL, &tv);
			if (r == 0) {
				_dprintf("%s: timeout in select %d\n", __FUNCTION__, fd);
				return -1;
			}
			else if (r < 0) {
				if (errno != EINTR) {
					_dprintf("%s: error in select %d\n", __FUNCTION__, fd);
					return -1;
				}
				// loop
			}
			else {
				r = 0;
				n = sizeof(r);
				if ((getsockopt(fd, SOL_SOCKET, SO_ERROR, &r, &n) < 0) || (r != 0)) {
					_dprintf("%s: error in SO_ERROR %d\n", __FUNCTION__, fd);
					return -1;
				}
				break;
			}
		}
	}

	if (fcntl(fd, F_SETFL, flags) < 0) {
		_dprintf("%s: error in F_*ETFL %d\n", __FUNCTION__, fd);
		return -1;
	}

//	_dprintf("%s: OK %d\n", __FUNCTION__, fd);
	return 0;
}

/*
int time_ok(void)
{
	return time(0) > Y2K;
}
*/

// -----------------------------------------------------------------------------
//!!TB - USB Support

/* Serialize using fcntl() calls 
 */

int file_lock(char *tag)
{
	char fn[64];
	struct flock lock;
	int lockfd = -1;

	sprintf(fn, "/var/lock/%s.lock", tag);
	if ((lockfd = open(fn, O_CREAT | O_RDWR, 0666)) < 0)
		goto lock_error;

	memset(&lock, 0, sizeof(lock));
	lock.l_type = F_WRLCK;
	lock.l_pid = getpid();

	if (fcntl(lockfd, F_SETLKW, &lock) < 0) {
		close(lockfd);
		goto lock_error;
	}

	return lockfd;
lock_error:
	// No proper error processing
	syslog(LOG_DEBUG, "Error %d locking %s, proceeding anyway", errno, fn);
	return -1;
}

void file_unlock(int lockfd)
{
	if (lockfd >= 0) {
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
