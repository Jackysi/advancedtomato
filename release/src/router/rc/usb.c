/*

	USB Support

*/
#include "rc.h"

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include <sys/mount.h>
#include <mntent.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/swap.h>

/* Adjust bdflush parameters.
 * Do this here, because Tomato doesn't have the sysctl command.
 * With these values, a disk block should be written to disk within 2 seconds.
 */
#if 1
#include <sys/kdaemon.h>
#define SET_PARM(n) (n * 2 | 1)

void tune_bdflush(void)
{
	bdflush(SET_PARM(5), 100);
	bdflush(SET_PARM(6), 100);
	bdflush(SET_PARM(8), 0);
}
#else
/* Store values in nvram for customization */
void tune_bdflush(void)
{
	unsigned int v[9];
	const char *p;

	p = nvram_safe_get("usb_bdflush");
	// nvram default: 30 500 0 0 100 100 60 0 0
	if (sscanf(p, "%u%u%u%u%u%u%u%u%u",
		&v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8]) == 9) {	// lightly verify
		f_write_string("/proc/sys/vm/bdflush", p, 0, 0);
	}
}
#endif


void start_usb(void)
{
	_dprintf("%s\n", __FUNCTION__);
	tune_bdflush();

	if (nvram_get_int("usb_enable")) {
//		led(LED_AOSS, LED_ON);
		modprobe("usbcore");

		/* mount usb device filesystem */
        	mount("usbdevfs", "/proc/bus/usb", "usbdevfs", MS_MGC_VAL, NULL);

		if (nvram_get_int("usb_storage")) {
			/* insert scsi and storage modules before usb drivers */
			modprobe("scsi_mod");
			modprobe("sd_mod");
			modprobe("usb-storage");

			if (nvram_get_int("usb_fs_ext3")) {
				modprobe("ext2");
				modprobe("jbd");
				modprobe("ext3");
			}

			if (nvram_get_int("usb_fs_fat")) {
				modprobe("fat");
				modprobe("vfat");
			}
		}

		/* if enabled, force USB2 before USB1.1 */
		if (nvram_get_int("usb_usb2")) {
			modprobe("ehci-hcd");
		}

		if (nvram_get_int("usb_uhci")) {
			modprobe("usb-uhci");
		}

		if (nvram_get_int("usb_ohci")) {
			modprobe("usb-ohci");
		}

		if (nvram_get_int("usb_printer")) {
			modprobe("printer");
			symlink("/dev/usb", "/dev/printers");

			/* start printer server */
			xstart("p910nd",
				nvram_get_int("usb_printer_bidirect") ? "-b" : "", //bidirectional
				"-f", "/dev/usb/lp0", // device
				"0" // listen port
			);
		}
	}
	else {
//		led(LED_AOSS, LED_OFF);
	}
}

void stop_usb(void)
{
	// stop printing service
	int i;
	char s[32];
	char pid[] = "/var/run/p9100d.pid";

	// only find and kill the printer server we started (port 0)
	if (f_read_string(pid, s, sizeof(s)) > 0) {
		if ((i = atoi(s)) > 1) {
			kill(i, SIGTERM);
			sleep(1);
			unlink(pid);
		}
	}
	modprobe_r("printer");

	// only stop storage services if disabled
	if (!nvram_get_int("usb_enable") || !nvram_get_int("usb_storage")) {
		// Unmount all partitions
		remove_storage_main();

		// Stop storage services
		modprobe_r("ext2");
		modprobe_r("ext3");
		modprobe_r("jbd");
		modprobe_r("vfat");
		modprobe_r("fat");
		modprobe_r("fuse");
#ifdef TCONFIG_SAMBASRV
		modprobe_r("nls_cp437");
		modprobe_r("nls_cp850");
		modprobe_r("nls_cp852");
		modprobe_r("nls_cp866");
#endif
		modprobe_r("usb-storage");
		modprobe_r("sd_mod");
		modprobe_r("scsi_mod");
	}

	// only unload core modules if usb is disabled
	if (!nvram_get_int("usb_enable")) {
		umount("/proc/bus/usb"); // unmount usb device filesystem
		modprobe_r("usb-ohci");
		modprobe_r("usb-uhci");
		modprobe_r("ehci-hcd");
		modprobe_r("usbcore");
	}
}


#define MOUNT_VAL_FAIL 	0
#define MOUNT_VAL_RONLY	1
#define MOUNT_VAL_RW 	2
#define MOUNT_VAL_EXIST	3

int mount_r(char *mnt_dev, char *mnt_dir, char *type)
{
	struct mntent *mnt;
	int ret;
	char options[40];
	char flagfn[128];
	int dir_made;

	if ((mnt = findmntents(mnt_dev, 0, 0, 0))) {
		syslog(LOG_INFO, "USB partition at %s already mounted on %s",
			mnt_dev, mnt->mnt_dir);
		return MOUNT_VAL_EXIST;
	}

	options[0] = 0;

	if (type) {
		unsigned long flags = MS_NOATIME | MS_NODEV;

		if (strcmp(type, "swap") == 0 || strcmp(type, "mbr") == 0) {
			/* not a mountable partition */
			flags = 0;
		}
		else if (strcmp(type, "ext2") == 0 || strcmp(type, "ext3") == 0) {
		}
#ifdef TCONFIG_SAMBASRV
		else if (strcmp(type, "vfat") == 0) {
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s", 
					isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "",
						nvram_get("smbd_cset"));
			if (nvram_invmatch("smbd_cpage", "")) {
				char *cp = nvram_get("smbd_cpage");
				sprintf(options + strlen(options), ",codepage=%s" + (options[0] ? 0 : 1), cp);
				sprintf(flagfn, "nls_cp%s", cp);

				cp = nvram_get("smbd_nlsmod");
				if ((cp) && (*cp != 0) && (strcmp(cp, flagfn) != 0))
					modprobe_r(cp);

				modprobe(flagfn);
				nvram_set("smbd_nlsmod", flagfn);
			}
		}
		else if (strncmp(type, "ntfs", 4) == 0) {
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s",
					isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "",
						nvram_get("smbd_cset"));
		}
#endif

		if (flags) {
			if ((dir_made = mkdir_if_none(mnt_dir))) {
				/* Create the flag file for remove the directory on dismount. */
				sprintf(flagfn, "%s/.autocreated-dir", mnt_dir);
				f_write(flagfn, NULL, 0, 0, 0);
			}

			ret = mount(mnt_dev, mnt_dir, type, flags, options[0] ? options : "");

			/* try ntfs-3g in case it's installed */
			if (ret != 0 && strncmp(type, "ntfs", 4) == 0) {
				sprintf(options + strlen(options), ",noatime,nodev" + (options[0] ? 0 : 1));
#ifdef TCONFIG_NTFS
				if (nvram_get_int("usb_fs_ntfs"))
#endif
					ret = eval("ntfs-3g", "-o", options, mnt_dev, mnt_dir);
			}
			if (ret != 0) /* give it another try - guess fs */
				ret = eval("mount", "-o", "noatime,nodev", mnt_dev, mnt_dir);

			if (ret == 0) {
				syslog(LOG_INFO, "USB %s%s fs at %s mounted on %s",
					type, (flags & MS_RDONLY) ? " (ro)" : "", mnt_dev, mnt_dir);
				return (flags & MS_RDONLY) ? MOUNT_VAL_RONLY : MOUNT_VAL_RW;
			}

			if (dir_made) {
				unlink(flagfn);
				rmdir(mnt_dir);
			}
		}
	}
	return MOUNT_VAL_FAIL;
}


/* Check if the UFD is still connected because the links created in /dev/discs
 * are not removed when the UFD is  unplugged.
 * The file to read is: /proc/scsi/usb-storage-#/#, where # is the host no.
 * We are looking for "Attached: Yes".
 */
static int usb_ufd_connected(int host_no)
{
	char proc_file[128], line[256];
	FILE *fp;

	sprintf(proc_file, "%s/%s-%d/%d", PROC_SCSI_ROOT, USB_STORAGE, host_no, host_no);
	fp = fopen(proc_file, "r");

	if (!fp) {
		/* try the way it's implemented in newer kernels: /proc/scsi/usb-storage/[host] */
		sprintf(proc_file, "%s/%s/%d", PROC_SCSI_ROOT, USB_STORAGE, host_no);
		fp = fopen(proc_file, "r");
	}

	if (fp) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (strstr(line, "Attached: Yes")) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
	}

	return 0;
}


#ifndef MNT_DETACH
#define MNT_DETACH	0x00000002      /* from linux/fs.h - just detach from the tree */
#endif
int umount_mountpoint(struct mntent *mnt, uint flags);
int uswap_mountpoint(struct mntent *mnt, uint flags);

/* Unmount this partition from all its mountpoints.  Note that it may
 * actually be mounted several times, either with different names or
 * with "-o bind" flag.
 * If the special flagfile is now revealed, delete it and [attempt to] delete
 * the directory.
 */
int umount_partition(char *dev_name, int host_num, int disc_num, int part_num, uint flags)
{
	sync();	/* This won't matter if the device is unplugged, though. */

	if (flags & EFH_HUNKNOWN) {
		/* EFH_HUNKNOWN flag is passed if the host was unknown.
		 * Only unmount disconnected drives in this case.
		 */
		if (usb_ufd_connected(host_num))
			return(0);
	}

	/* Find all the active swaps that are on this device and stop them. */
	findmntents(dev_name, 1, uswap_mountpoint, flags);

	/* Find all the mountpoints that are for this device and unmount them. */
	findmntents(dev_name, 0, umount_mountpoint, flags);
	return 0;
}

int uswap_mountpoint(struct mntent *mnt, uint flags)
{
	swapoff(mnt->mnt_fsname);
	return 0;
}

int umount_mountpoint(struct mntent *mnt, uint flags)
{
	int ret = 1, count;
	char flagfn[128];

		sprintf(flagfn, "%s/.autocreated-dir", mnt->mnt_dir);
		run_nvscript("script_autostop", mnt->mnt_dir, 5);
		count = 0;
		while ((ret = umount(mnt->mnt_dir)) && (count < 2)) {
			count++;
			sleep(1);
		}

		if (!ret)
			syslog(LOG_INFO, "USB partition unmounted from %s", mnt->mnt_dir);

		if (ret && ((flags & EFH_USER) == 0)) {
			/* Make one more try to do a lazy unmount unless it's an unmount
			 * request from the Web GUI.
			 * MNT_DETACH will expose the underlying mountpoint directory to all
			 * except whatever has cd'ed to the mountpoint (thereby making it busy).
			 * So the unmount can't actually fail. It disappears from the ken of
			 * everyone else immediately, and from the ken of whomever is keeping it
			 * busy until they move away from it. And then it disappears for real.
			 */
			ret = umount2(mnt->mnt_dir, MNT_DETACH);
			syslog(LOG_INFO, "USB partition busy - will unmount ASAP from %s", mnt->mnt_dir);
		}

		if (!ret) {
			if ((unlink(flagfn) == 0)) {
				// Only delete the directory if it was auto-created
				rmdir(mnt->mnt_dir);
			}
		}
	return(!ret);
}


/* Mount this partition on this disc.
 * If the device is already mounted on any mountpoint, don't mount it again.
 * If this is a swap partition, try swapon -a.
 * If this is a regular partition, try mount -a.
 *
 * Before we mount any partitions:
 *	If the type is swap and /etc/fstab exists, do "swapon -a"
 *	If /etc/fstab exists, do "mount -a".
 *  We delay invoking mount because mount will probe all the partitions
 *	to read the labels, and we don't want it to do that early on.
 *  We don't invoke swapon until we actually find a swap partition.
 *
 * If the mount succeeds, execute the *.autorun scripts in the top
 * directory of the newly mounted partition.
 * Returns NZ for success, 0 if we did not mount anything.
 */
int mount_partition(char *dev_name, int host_num, int disc_num, int part_num, uint flags)
{
	char the_label[128], mountpoint[128];
	int ret;
	char *type;
	static char *swp_argv[] = { "swapon", "-a", NULL };
	static char *mnt_argv[] = { "mount", "-a", NULL };
	struct mntent *mnt;

	if ((type = detect_fs_type(dev_name)) == NULL)
		return(0);

	if (f_exists("/etc/fstab")) {
		if (strcmp(type, "swap") == 0) {
			_eval(swp_argv, NULL, 0, NULL);
			return(0);
		}

		if (mount_r(dev_name, NULL, NULL) == MOUNT_VAL_EXIST)
			return(0);

		_eval(mnt_argv, NULL, 0, NULL);
		if ((mnt = findmntents(dev_name, 0, 0, 0))) {
			run_userfile(mnt->mnt_dir, ".autorun", mnt->mnt_dir, 3);
			return(1);
		}
	}

	if (find_label(dev_name, the_label)) {
		sprintf(mountpoint, "%s/%s", MOUNT_ROOT, the_label);
		if ((ret = mount_r(dev_name, mountpoint, type))) {
			if (ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW)
				run_userfile(mountpoint, ".autorun", mountpoint, 3);
			return(ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW);
		}
	}

	/* Can't mount to /mnt/LABEL, so try mounting to /mnt/discDN_PN */
	sprintf(mountpoint, "%s/disc%d_%d", MOUNT_ROOT, disc_num, part_num);
	ret = mount_r(dev_name, mountpoint, type);
	if (ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW)
		run_userfile(mountpoint, ".autorun", mountpoint, 3);
	return(ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW);
}


/* Mount or unmount all partitions on this controller.
 * Parameter: action_add:
 * 0  = unmount
 * >0 = mount only if automount config option is enabled.
 * <0 = mount regardless of config option.
 */
void hotplug_usb_storage_device(int host_no, int action_add, uint flags)
{
	if (!nvram_get_int("usb_enable"))
		return;
	_dprintf("%s: host %d action: %d\n", __FUNCTION__, host_no, action_add);

	if (action_add) {
		if (nvram_get_int("usb_storage") && (nvram_get_int("usb_automount") || action_add < 0)) {
			/* Do not probe the device here. It's either initiated by user,
			 * or hotplug_usb() already did.
			 */
			if (exec_for_host(host_no, 0x00, flags, mount_partition)) {
				restart_nas_services(1); // restart all NAS applications
				run_nvscript("script_usbmount", NULL, 3);
			}
		}
	}
	else {
		if (nvram_get_int("usb_storage") || ((flags & EFH_USER) == 0)) {
			/* When unplugged, unmount the device even if
			 * usb storage is disabled in the GUI.
			 */
			if (flags & EFH_USER) {
				/* Unmount from Web. Run user pre-unmount script if any.
				 */
				run_nvscript("script_usbumount", NULL, 3);
			}
			/* Kill all NAS applications here
			 * so they are not keeping the device busy.
			 */
			restart_nas_services(0);
			exec_for_host(host_no, (flags & EFH_USER) ? 0x00 : 0x02, flags, umount_partition);
			/* Restart NAS applications */
			restart_nas_services(1);
		}
	}
}


/* This gets called at reboot or upgrade.  The system is stopping. */
void remove_storage_main(void)
{
	if (nvram_get_int("usb_enable") && nvram_get_int("usb_storage")) {
		if (nvram_get_int("usb_automount")) {
			// run pre-unmount script if any
			run_nvscript("script_usbumount", NULL, 3);
		}
	}
	restart_nas_services(0);
	/* Unmount all partitions */
	exec_for_host(-1, 0x02, 0, umount_partition);
}


/******* KLUDGE to try to workaround a kernel concurrency bug. *******/
/*
 * There is no reason that we should have to serialize this code, but
 * we have to.  Otherwise we will occasionally lock up when
 * we repeatedly plug and unplug a hub with several USB storage devices
 * connected to it.  This is probably be a bug in the kernel.  Like some
 * critical data structure that isn't protected by a lock.  There's a
 * lot of kernel threads that are doing things to the USB device.
 *
 * Oh well-----serializing this code seems to avoid the problem.
 *
 * Notes on testing the position of the lock.
 * Spawning 5 hotplug threads.  2 for hub, 3 scsi hosts.
 * 6 total partitions on 3 hosts. (1 & 2 & 3)
 * 1st for add, 2nd for remove, loop 20+ times.
 * No /etc/fstab.
 * "fails" = router locks up
 *
 * -- around everything:		works
 * -- around the (*func) call		fails
 * -- around all of exec_for_host	works
 * -- Around the "when to update"	fails
 * -- The when-to-update's & func code	works
 * -- Around the /proc/parts code	fails	
 *
 * It appears to crash when we try to do the mount (?? maybe BLKRRPART) of one
 * disk at the same time as the kernel is recognizing/processing another disk.
 * This can happen on bootup, or when a daisy-chained hub is plugged in.
 * And other times, when mounting lots of disks at once via the admin
 * interface.
 *
 * The problem isn't _this_ drive.  The problem is another drive that the
 * kernel is recognizing/processing (in another thread) _after_ this drive.
 * It we ask the kernel to do something with _this_ drive at the same time it is
 * working on _that_ later drive, then it tends to crash.
 * The work-around is to stall our processing of _this_ drive until after
 * the kernel has finished its work on _that_ drive.  Kinda hard for us to do,
 * since we don't yet know that _that_ drive even exists!
 * I've tried all sorts of things to try to detect/avoid the problem.
 * Thw worst time is when a hub & discs are first detected (bootup or initial
 * plugin.  Because the kernel automatically reads the partition table as the
 * disks are detected.
 */


/* Get the size. It'll change as USB devices are added, detected, & removed. */
unsigned int get_size(int host)
{
	char bfr[1024];
	int fd;
	int i = -1;
	unsigned int total = 0;

	fd = open("/proc/partitions", O_RDONLY);
	if (fd >= 0) {
		while ((i = read(fd, bfr, sizeof(bfr))) > 0) {
			total += i;
		}
		close(fd);
	}
	//syslog(LOG_INFO, "**** Size: %4d  HN: %5i: %d  errno: %d\n", total, host, i, errno);
	return(total);
}


/* Sleep for minimum of X seconds, waiting for the known partition
 * information to stabilize.  Each time we find that the
 * partition setup has changed, extend the time.
 * The part info will change whenever new partitions are discovered
 * or old ones are deleted.
 * This will happen with automatically by the kernel, or
 * explicitly by a user program doing an ioctl BLKRRPART.
 */
void wait_for_stabilize(int tm, int host_num)
{
	int n = get_size(host_num);
	int m;

	while (--tm > 0) {
		sleep(1);
		m = get_size(host_num);
		if (m != n)
			++tm;
		n = m;
	}
}


/* Plugging or removing usb device
 *
 * On an occurrance, multiple hotplug events may be fired off.
 * For example, if a hub is plugged or unplugged, an event
 * will be generated for everything downstream of it, plus one for
 * the hub itself.  These are fired off simultaneously, not serially.
 * This means that many many hotplug processes will be running at
 * the same time.
 *
 * The hotplug event generated by the kernel gives us several pieces
 * of information:
 * PRODUCT is vendorid/productid/rev#.
 * DEVICE is /proc/bus/usb/bus#/dev#
 * ACTION is add or remove
 * SCSI_HOST is the host (controller) number (this relies on the custom kernel patch)
 *
 * Note that when we get a hotplug add event, the USB susbsystem may
 * or may not have yet tried to read the partition table of the
 * device.  For a new controller that has never been seen before,
 * generally yes.  For a re-plug of a controller that has been seen
 * before, generally no.
 *
 * On a remove, the partition info has not yet been expunged.  The
 * partitions show up as /dev/discs/disc#/part#, and /proc/partitions.
 * It appears that doing a "stat" for a non-existant partition will
 * causes the kernel to re-validate the device and update the
 * partition table info.  However, it won't re-validate if the disc is
 * mounted--you'll get a "Device busy for revalidation (usage=%d)" in
 * syslog.
 *
 * The $INTERFACE is "class/subclass/protocol"
 * Some interesting classes:
 *	8 = mass storage
 *	7 = printer
 *	3 = HID.   3/1/2 = mouse.
 *	6 = still image (6/1/1 = Digital camera Camera)
 *	9 = Hub
 *	255 = scanner (255/255/255)
 *
 * Observed:
 *	Hub seems to have no INTERFACE (null), and TYPE of "9/0/0"
 *	Flash disk seems to have INTERFACE of "8/6/80", and TYPE of "0/0/0"
 *
 * When a hub is unplugged, a hotplug event is generated for it and everything
 * downstream from it.  You cannot depend on getting these events in any
 * particular order, since there will be many hotplug programs all fired off
 * at almost the same time.
 * On a remove, don't try to access the downstream devices right away, give the
 * kernel time to finish cleaning up all the data structures, which will be
 * in the process of being torn down.
 *
 * On the initial plugin, the first time the kernel usb-storage subsystem sees
 * the host (identified by GUID), it automatically reads the partition table.
 * On subsequent plugins, it does not.
 *
 * Special values for Web Administration to unmount or remount
 * all partitions of the host:
 *	INTERFACE=TOMATO/...
 *	ACTION=add/remove
 *	SCSI_HOST=<host_no>
 * If host_no is negative, we unmount all partions of *all* hosts.
 */
void hotplug_usb(void)
{
	int add;
	int host = -1;
	char *interface = getenv("INTERFACE");
	char *action = getenv("ACTION");
	char *product = getenv("PRODUCT");
	char *device = getenv("DEVICE");
	char *scsi_host = getenv("SCSI_HOST");

	//_dprintf("USB hotplug INTERFACE=%s ACTION=%s PRODUCT=%s HOST=%s DEVICE=%s\n",
	//	interface, action, product, scsi_host, device);

	if (!nvram_get_int("usb_enable")) return;
	if (!interface || !action || !product)	/* Hubs bail out here. */
		return;

	if (scsi_host)
		host = atoi(scsi_host);

	add = (strcmp(action, "add") == 0);
	if (add && (strncmp(interface, "TOMATO/", 7) != 0)) {
		/* Give the kernel time to settle down. */
		syslog(LOG_DEBUG, "Waiting for device %s [INTERFACE=%s PRODUCT=%s] to settle before scanning",
			device, interface, product);
		wait_for_stabilize(4, host);
	}
	int fd = nvram_get_int("usb_nolock") ? -1 : file_lock("usb");

	if (strncmp(interface, "TOMATO/", 7) == 0) {	/* web admin */
		if (scsi_host == NULL)
			host = atoi(product);	// for backward compatibility
		/* If host is negative, unmount all partitions of *all* hosts.
		 * This feature can be used in custom scripts as following:
		 *
		 * # INTERFACE=TOMATO/1 ACTION=remove PRODUCT=-1 SCSI_HOST=-1 hotplug usb
		 *
		 * PRODUCT is required to pass the env variables verification.
		 */
		/* Unmount or remount all partitions of the host. */
		hotplug_usb_storage_device(host, add ? -1 : 0, EFH_USER);
	}
	else if (strncmp(interface, "8/", 2) == 0) {	/* usb storage */
		if (add)
			exec_for_host(host, 0x01, 0, (host_exec) NULL);	/* so the user's hotplug script mount can work. */
		run_nvscript("script_usbhotplug", NULL, 2);
		hotplug_usb_storage_device(host, add, host < 0 ? EFH_HUNKNOWN : 0);
	}
	else {	/* It's some other type of USB device, not storage. */
		/* Do nothing.  The user's hotplug script must do it all. */
		run_nvscript("script_usbhotplug", NULL, 2);
	}

	file_unlock(fd);
}

