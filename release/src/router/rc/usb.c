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
#ifdef LINUX26
void tune_bdflush(void)
{
	f_write_string("/proc/sys/vm/dirty_expire_centisecs", "200", 0, 0);
	f_write_string("/proc/sys/vm/dirty_writeback_centisecs", "200", 0, 0);
}
#else
#include <sys/kdaemon.h>
#define SET_PARM(n) (n * 2 | 1)
void tune_bdflush(void)
{
	bdflush(SET_PARM(5), 100);
	bdflush(SET_PARM(6), 100);
	bdflush(SET_PARM(8), 0);
}
#endif // LINUX26

#define USBCORE_MOD	"usbcore"
#define USB20_MOD	"ehci-hcd"
#define USBSTORAGE_MOD	"usb-storage"
#define SCSI_MOD	"scsi_mod"
#define SD_MOD		"sd_mod"
#ifdef LINUX26
#define USBOHCI_MOD	"ohci-hcd"
#define USBUHCI_MOD	"uhci-hcd"
#define USBPRINTER_MOD	"usblp"
#define SCSI_WAIT_MOD	"scsi_wait_scan"
#define USBFS		"usbfs"
#else
#define USBOHCI_MOD	"usb-ohci"
#define USBUHCI_MOD	"usb-uhci"
#define USBPRINTER_MOD	"printer"
#define USBFS		"usbdevfs"
#endif

static int p9100d_sig(int sig)
{
	const char p910pid[] = "/var/run/p9100d.pid";
	char s[32];
	int pid;

	if (f_read_string(p910pid, s, sizeof(s)) > 0) {
		if ((pid = atoi(s)) > 1) {
			if (kill(pid, sig) == 0) {
				if (sig == SIGTERM) {
					sleep(1);
					unlink(p910pid);
				}
				return 0;
			}
		}
	}
	return -1;
}

void start_usb(void)
{
	char param[32];
	int i;

	if (nvram_match("boardtype", "0x052b")) { // Netgear WNR3500L v2 - initialize USB port
		xstart("gpio", "enable", "20");
	}

	_dprintf("%s\n", __FUNCTION__);
	tune_bdflush();

	if (nvram_get_int("usb_enable")) {
		modprobe(USBCORE_MOD);

		/* mount usb device filesystem */
        	mount(USBFS, "/proc/bus/usb", USBFS, MS_MGC_VAL, NULL);

#ifdef LINUX26
		i = do_led(LED_USB, LED_PROBE);
		if (i != 255) {
			modprobe("ledtrig-usbdev");
			modprobe("leds-usb");
			sprintf(param, "%d", i);
			f_write_string("/proc/leds-usb/gpio_pin", param, 0, 0);
		}
#endif
#ifdef TCONFIG_USBAP
			char instance[20];
			sprintf(instance, "instance_base=1");
			modprobe("wl_high", instance );
#endif

		if (nvram_get_int("usb_storage")) {
			/* insert scsi and storage modules before usb drivers */
			modprobe(SCSI_MOD);
#ifdef LINUX26
			modprobe(SCSI_WAIT_MOD);
#endif
			modprobe(SD_MOD);
			modprobe(USBSTORAGE_MOD);

			if (nvram_get_int("usb_fs_ext3")) {
#ifdef LINUX26
				modprobe("mbcache");	// used by ext2/ext3
#endif
				/* insert ext3 first so that lazy mount tries ext3 before ext2 */
				modprobe("jbd");
				modprobe("ext3");
				modprobe("ext2");
			}

			if (nvram_get_int("usb_fs_fat")) {
				modprobe("fat");
				modprobe("vfat");
			}
#ifdef TCONFIG_HFS
			if (nvram_get_int("usb_fs_hfs")) {
				modprobe("hfs");
				modprobe("hfsplus");
			}
#endif

#if defined(LINUX26) && defined(TCONFIG_MICROSD)
			if (nvram_get_int("usb_mmc") == 1) {
				/* insert SD/MMC modules if present */
				modprobe("mmc_core");
				modprobe("mmc_block");
				modprobe("sdhci");
			}
#endif
		}

		/* if enabled, force USB2 before USB1.1 */
		if (nvram_get_int("usb_usb2") == 1) {
			i = nvram_get_int("usb_irq_thresh");
			if ((i < 0) || (i > 6))
				i = 0;
			sprintf(param, "log2_irq_thresh=%d", i);
			modprobe(USB20_MOD, param);
		}

		if (nvram_get_int("usb_uhci") == 1) {
			modprobe(USBUHCI_MOD);
		}

		if (nvram_get_int("usb_ohci") == 1) {
			modprobe(USBOHCI_MOD);
		}

		if (nvram_get_int("usb_printer")) {
			symlink("/dev/usb", "/dev/printers");
			modprobe(USBPRINTER_MOD);

			/* start printer server only if not already running */
			if (p9100d_sig(0) != 0) {
				eval("p910nd",
				nvram_get_int("usb_printer_bidirect") ? "-b" : "", //bidirectional
				"-f", "/dev/usb/lp0", // device
				"0" // listen port
				);
			}
		}
#ifdef LINUX26
		if (nvram_get_int("idle_enable") == 1) {
			xstart( "sd-idle" );
		}
#endif

#ifdef TCONFIG_UPS
		start_ups();
#endif

#ifdef TCONFIG_USBAP
			//enable eth2 after detect new iface by wl_high module
			sleep(5);
			xstart("service", "wireless", "restart");
#endif

// shibby
// If we want restore backup of webmon from USB device,
// we have to wait for mount USB devices by hotplug
// and then reboot firewall service (webmon iptables rules) one more time.
		if( nvram_match( "log_wm", "1" ) && nvram_match( "webmon_bkp", "1" ) )
			xstart( "service", "firewall", "restart" );

	}
}

void stop_usb(void)
{
	int disabled = !nvram_get_int("usb_enable");

	// only find and kill the printer server we started (port 0)
	p9100d_sig(SIGTERM);
	modprobe_r(USBPRINTER_MOD);

	// only stop storage services if disabled
	if (disabled || !nvram_get_int("usb_storage")) {
		// Unmount all partitions
		remove_storage_main(0);

		// Stop storage services
		modprobe_r("ext2");
		modprobe_r("ext3");
		modprobe_r("jbd");
#ifdef LINUX26
		modprobe_r("mbcache");
#endif
		modprobe_r("vfat");
		modprobe_r("fat");
		modprobe_r("fuse");
#ifdef TCONFIG_HFS
		modprobe_r("hfs");
		modprobe_r("hfsplus");
#endif
		sleep(1);

#ifdef TCONFIG_SAMBASRV
		modprobe_r("nls_cp437");
		modprobe_r("nls_cp850");
		modprobe_r("nls_cp852");
		modprobe_r("nls_cp866");
#ifdef LINUX26
		modprobe_r("nls_cp932");
		modprobe_r("nls_cp936");
		modprobe_r("nls_cp949");
		modprobe_r("nls_cp950");
#endif
#endif
		modprobe_r(USBSTORAGE_MOD);
		modprobe_r(SD_MOD);
#ifdef LINUX26
		modprobe_r(SCSI_WAIT_MOD);
#endif
		modprobe_r(SCSI_MOD);
	}

#if defined(LINUX26) && defined(TCONFIG_MICROSD)
	if (disabled || !nvram_get_int("usb_storage") || nvram_get_int("usb_mmc") != 1) {
		modprobe_r("sdhci");
		modprobe_r("mmc_block");
		modprobe_r("mmc_core");
	}
#endif

	if (disabled || nvram_get_int("usb_ohci") != 1) modprobe_r(USBOHCI_MOD);
	if (disabled || nvram_get_int("usb_uhci") != 1) modprobe_r(USBUHCI_MOD);
	if (disabled || nvram_get_int("usb_usb2") != 1) modprobe_r(USB20_MOD);

#ifdef LINUX26
	modprobe_r("leds-usb");
	modprobe_r("ledtrig-usbdev");
	led(LED_USB, LED_OFF);
#endif

	// only unload core modules if usb is disabled
	if (disabled) {
		umount("/proc/bus/usb"); // unmount usb device filesystem
		modprobe_r(USBOHCI_MOD);
		modprobe_r(USBUHCI_MOD);
		modprobe_r(USB20_MOD);
		modprobe_r(USBCORE_MOD);
	}

#ifdef LINUX26
	if (nvram_get_int("idle_enable") == 0) {
		killall("sd-idle", SIGTERM);
	}

#ifdef TCONFIG_UPS
	stop_ups();
#endif

	if (nvram_match("3g_usb", "0") ) {
		if (nvram_match("3g_module", "sierra") ) {
			modprobe_r("sierra");
			modprobe_r("usbserial");
		}

		if (nvram_match("3g_module", "option") ) {
			modprobe_r("option");
			modprobe_r("usbserial");
		}
/*
		// shibby
		// when modem use usbserial module and we will try remove module, module will crash
		// the only solution at the moment is reboot router
		// FIX THIS
		if (nvram_match("3g_module", "usbserial") ) {
			modprobe_r("usbserial");
		}
*/

	if (nvram_match("boardtype", "0x052b")) { // Netgear WNR3500L v2 - disable USB port
		xstart("gpio", "disable", "20");
	}

	}
#endif

}


#define MOUNT_VAL_FAIL 	0
#define MOUNT_VAL_RONLY	1
#define MOUNT_VAL_RW 	2
#define MOUNT_VAL_EXIST	3

int mount_r(char *mnt_dev, char *mnt_dir, char *type)
{
	struct mntent *mnt;
	int ret;
	char options[140];
	char flagfn[128];
	int dir_made;

	if ((mnt = findmntents(mnt_dev, 0, NULL, 0))) {
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
			if (nvram_invmatch("usb_ext_opt", ""))
				sprintf(options, nvram_safe_get("usb_ext_opt"));
		}
		else if (strcmp(type, "vfat") == 0) {
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s", 
					isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "",
						nvram_get("smbd_cset"));
			if (nvram_invmatch("smbd_cpage", "")) {
				char *cp = nvram_safe_get("smbd_cpage");
				sprintf(options + strlen(options), ",codepage=%s" + (options[0] ? 0 : 1), cp);
				sprintf(flagfn, "nls_cp%s", cp);

				cp = nvram_get("smbd_nlsmod");
				if ((cp) && (*cp != 0) && (strcmp(cp, flagfn) != 0))
					modprobe_r(cp);

				modprobe(flagfn);
				nvram_set("smbd_nlsmod", flagfn);
			}
			sprintf(options + strlen(options), ",shortname=winnt" + (options[0] ? 0 : 1));
#ifdef LINUX26
			sprintf(options + strlen(options), ",flush" + (options[0] ? 0 : 1));
#endif
			if (nvram_invmatch("usb_fat_opt", ""))
				sprintf(options + strlen(options), "%s%s", options[0] ? "," : "", nvram_safe_get("usb_fat_opt"));
		}
		else if (strncmp(type, "ntfs", 4) == 0) {
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s",
					isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "",
						nvram_get("smbd_cset"));
			if (nvram_invmatch("usb_ntfs_opt", ""))
				sprintf(options + strlen(options), "%s%s", options[0] ? "," : "", nvram_safe_get("usb_ntfs_opt"));
		}

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

#ifdef TCONFIG_HFS
			if (ret != 0 && strncmp(type, "hfs", "") == 0) {
				ret = eval("mount", "-o", "noatime,nodev", mnt_dev, mnt_dir);
			}

			if (ret != 0 && strncmp(type, "hfsplus", "") == 0) {
				ret = eval("mount", "-o", "noatime,nodev", mnt_dev, mnt_dir);
			}
#endif

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


struct mntent *mount_fstab(char *dev_name, char *type, char *label, char *uuid)
{
	struct mntent *mnt = NULL;
#if 0
	if (eval("mount", "-a") == 0)
		mnt = findmntents(dev_name, 0, NULL, 0);
#else
	char spec[PATH_MAX+1];

	if (label && *label) {
		sprintf(spec, "LABEL=%s", label);
		if (eval("mount", spec) == 0)
			mnt = findmntents(dev_name, 0, NULL, 0);
	}

	if (!mnt && uuid && *uuid) {
		sprintf(spec, "UUID=%s", uuid);
		if (eval("mount", spec) == 0)
			mnt = findmntents(dev_name, 0, NULL, 0);
	}

	if (!mnt) {
		if (eval("mount", dev_name) == 0)
			mnt = findmntents(dev_name, 0, NULL, 0);
	}

	if (!mnt) {
		/* Still did not find what we are looking for, try absolute path */
		if (realpath(dev_name, spec)) {
			if (eval("mount", spec) == 0)
				mnt = findmntents(dev_name, 0, NULL, 0);
		}
	}
#endif

	if (mnt)
		syslog(LOG_INFO, "USB %s fs at %s mounted on %s", type, dev_name, mnt->mnt_dir);
	return (mnt);
}


/* Check if the UFD is still connected because the links created in /dev/discs
 * are not removed when the UFD is  unplugged.
 * The file to read is: /proc/scsi/usb-storage-#/#, where # is the host no.
 * We are looking for "Attached: Yes".
 */
static int usb_ufd_connected(int host_no)
{
	char proc_file[128];
#ifndef LINUX26
	char line[256];
#endif
	FILE *fp;

	sprintf(proc_file, "%s/%s-%d/%d", PROC_SCSI_ROOT, USB_STORAGE, host_no, host_no);
	fp = fopen(proc_file, "r");

	if (!fp) {
		/* try the way it's implemented in newer kernels: /proc/scsi/usb-storage/[host] */
		sprintf(proc_file, "%s/%s/%d", PROC_SCSI_ROOT, USB_STORAGE, host_no);
		fp = fopen(proc_file, "r");
	}

	if (fp) {
#ifdef LINUX26
		fclose(fp);
		return 1;
#else
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (strstr(line, "Attached: Yes")) {
				fclose(fp);
				return 1;
			}
		}
		fclose(fp);
#endif
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
int umount_partition(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags)
{
	sync();	/* This won't matter if the device is unplugged, though. */

	if (flags & EFH_HUNKNOWN) {
		/* EFH_HUNKNOWN flag is passed if the host was unknown.
		 * Only unmount disconnected drives in this case.
		 */
		if (usb_ufd_connected(host_num))
			return 0;
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

	/* Run user pre-unmount scripts if any. It might be too late if
	 * the drive has been disconnected, but we'll try it anyway.
 	 */
	if (nvram_get_int("usb_automount"))
		run_nvscript("script_usbumount", mnt->mnt_dir, 3);
	/* Run *.autostop scripts located in the root of the partition being unmounted if any. */
	run_userfile(mnt->mnt_dir, ".autostop", mnt->mnt_dir, 5);
	run_nvscript("script_autostop", mnt->mnt_dir, 5);

	count = 0;
	while ((ret = umount(mnt->mnt_dir)) && (count < 2)) {
		count++;
		/* If we could not unmount the drive on the 1st try,
		 * kill all NAS applications so they are not keeping the device busy -
		 * unless it's an unmount request from the Web GUI.
		 */
		if ((count == 1) && ((flags & EFH_USER) == 0))
			restart_nas_services(1, 0);
		sleep(1);
	}

	if (ret == 0)
		syslog(LOG_INFO, "USB partition unmounted from %s", mnt->mnt_dir);

	if (ret && ((flags & EFH_SHUTDN) != 0)) {
		/* If system is stopping (not restarting), and we couldn't unmount the
		 * partition, try to remount it as read-only. Ignore the return code -
		 * we can still try to do a lazy unmount.
		 */
		eval("mount", "-o", "remount,ro", mnt->mnt_dir);
	}

	if (ret && ((flags & EFH_USER) == 0)) {
		/* Make one more try to do a lazy unmount unless it's an unmount
		 * request from the Web GUI.
		 * MNT_DETACH will expose the underlying mountpoint directory to all
		 * except whatever has cd'ed to the mountpoint (thereby making it busy).
		 * So the unmount can't actually fail. It disappears from the ken of
		 * everyone else immediately, and from the ken of whomever is keeping it
		 * busy when they move away from it. And then it disappears for real.
		 */
		ret = umount2(mnt->mnt_dir, MNT_DETACH);
		syslog(LOG_INFO, "USB partition busy - will unmount ASAP from %s", mnt->mnt_dir);
	}

	if (ret == 0) {
		if ((unlink(flagfn) == 0)) {
			// Only delete the directory if it was auto-created
			rmdir(mnt->mnt_dir);
		}
	}
	return (ret == 0);
}


/* Mount this partition on this disc.
 * If the device is already mounted on any mountpoint, don't mount it again.
 * If this is a swap partition, try swapon -a.
 * If this is a regular partition, try mount -a.
 *
 * Before we mount any partitions:
 *	If the type is swap and /etc/fstab exists, do "swapon -a"
 *	If /etc/fstab exists, try mounting using fstab.
 *  We delay invoking mount because mount will probe all the partitions
 *	to read the labels, and we don't want it to do that early on.
 *  We don't invoke swapon until we actually find a swap partition.
 *
 * If the mount succeeds, execute the *.autorun scripts in the top
 * directory of the newly mounted partition.
 * Returns NZ for success, 0 if we did not mount anything.
 */
int mount_partition(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags)
{
	char the_label[128], mountpoint[128], uuid[40];
	int ret;
	char *type, *p;
	static char *swp_argv[] = { "swapon", "-a", NULL };
	struct mntent *mnt;

	if ((type = find_label_or_uuid(dev_name, the_label, uuid)) == NULL)
		return 0;

	if (f_exists("/etc/fstab")) {
		if (strcmp(type, "swap") == 0) {
			_eval(swp_argv, NULL, 0, NULL);
			return 0;
		}

		if (mount_r(dev_name, NULL, NULL) == MOUNT_VAL_EXIST)
			return 0;

		if ((mnt = mount_fstab(dev_name, type, the_label, uuid))) {
			strcpy(mountpoint, mnt->mnt_dir);
			ret = MOUNT_VAL_RW;
			goto done;
		}
	}

	if (*the_label != 0) {
		for (p = the_label; *p; p++) {
			if (!isalnum(*p) && !strchr("+-&.@", *p))
				*p = '_';
		}
		sprintf(mountpoint, "%s/%s", MOUNT_ROOT, the_label);
		if ((ret = mount_r(dev_name, mountpoint, type)))
			goto done;
	}

	/* Can't mount to /mnt/LABEL, so try mounting to /mnt/discDN_PN */
	sprintf(mountpoint, "%s/%s", MOUNT_ROOT, pt_name);
	ret = mount_r(dev_name, mountpoint, type);
done:
	if (ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW)
	{
		/* Run user *.autorun and post-mount scripts if any. */
		run_userfile(mountpoint, ".autorun", mountpoint, 3);
		if (nvram_get_int("usb_automount"))
			run_nvscript("script_usbmount", mountpoint, 3);
	}
	return (ret == MOUNT_VAL_RONLY || ret == MOUNT_VAL_RW);
}


#if 0 /* LINUX26 */

/* 
 * Finds SCSI Host number. Returns the host number >=0 if found, or (-1) otherwise.
 * The name and host number of scsi block device in kernel 2.6 (for attached devices) can be found as
 * 	/sys($DEVPATH)/host<host_no>/target<*>/<id>/block:[sda|sdb|...]
 * where $DEVPATH is passed to hotplug events, and looks like
 * 	/devices/pci0000:00/0000:00:04.1/usb1/1-1/1-1:1.2
 *
 * For printers this function finds a minor assigned to a printer
 *	/sys($DEVPATH)/usb:lp[0|1|2|...]
 */
int find_dev_host(const char *devpath)
{
	DIR *usb_devpath;
	struct dirent *dp;
	char buf[256];
	int host = -1;	/* Scsi Host */

	sprintf(buf, "/sys%s", devpath);
	if ((usb_devpath = opendir(buf))) {
		while ((dp = readdir(usb_devpath))) {
			errno = 0;
			if (strncmp(dp->d_name, "host", 4) == 0) {
				host = strtol(dp->d_name + 4, (char **)NULL, 10);
				if (errno)
					host = -1;
				else
					break;
			}
			else if (strncmp(dp->d_name, "usb:lp", 6) == 0) {
				host = strtol(dp->d_name + 6, (char **)NULL, 10);
				if (errno)
					host = -1;
				else
					break;
			}
			else
				continue;
		}
		closedir(usb_devpath);
	}
	return (host);
}

#endif	/* LINUX26 */

int dir_is_mountpoint(const char *root, const char *dir)
{
	char path[256];
	struct stat sb;
	int thisdev;

	snprintf(path, sizeof(path), "%s%s%s", root ? : "", root ? "/" : "", dir);

	/* Check if this is a directory */
	sb.st_mode = S_IFDIR;	/* failsafe */
	stat(path, &sb);

	if (S_ISDIR(sb.st_mode)) {

		/* If this dir & its parent dir are on the same device, it is not a mountpoint */
		strcat(path, "/.");
		stat(path, &sb);
		thisdev = sb.st_dev;
		strcat(path, ".");
		++sb.st_dev;	/* failsafe */
		stat(path, &sb);

		return (thisdev != sb.st_dev);
	}

	return 0;
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
				restart_nas_services(0, 1); // restart all NAS applications
			}
		}
	}
	else {
		if (nvram_get_int("usb_storage") || ((flags & EFH_USER) == 0)) {
			/* When unplugged, unmount the device even if
			 * usb storage is disabled in the GUI.
			 */
			exec_for_host(host_no, (flags & EFH_USER) ? 0x00 : 0x02, flags, umount_partition);
			/* Restart NAS applications (they could be killed by umount_mountpoint),
			 * or just re-read the configuration.
			 */
			restart_nas_services(0, 1);
		}
	}
}


/* This gets called at reboot or upgrade.  The system is stopping. */
void remove_storage_main(int shutdn)
{
	if (shutdn)
		restart_nas_services(1, 0);
	/* Unmount all partitions */
	exec_for_host(-1, 0x02, shutdn ? EFH_SHUTDN : 0, umount_partition);
}


/*******
 * All the complex locking & checking code was removed when the kernel USB-storage
 * bugs were fixed.
 * The crash bug was with overlapped I/O to different USB drives, not specifically
 * with mount processing.
 *
 * And for USB devices that are slow to come up.  The kernel now waits until the
 * USB drive has settled, and it correctly reads the partition table before calling
 * the hotplug agent.
 *
 * The kernel patch was cleaning up data structures on an unplug.  It
 * needs to wait until the disk is unmounted.  We have 20 seconds to do
 * the unmounts.
 *******/

#ifdef LINUX26
static inline void usbled_proc(char *device, int add)
{
	char *p;
	char param[32];

	if (do_led(LED_USB, LED_PROBE) != 255) {
		strncpy(param, device, sizeof(param));
		if ((p = strchr(param, ':')) != NULL)
			*p = 0;

		/* verify if we need to ignore this device (i.e. an internal SD/MMC slot ) */
		p = nvram_safe_get("usb_noled");
		if (strcmp(p, param) == 0)
			return;

		f_write_string(add ? "/proc/leds-usb/add" : "/proc/leds-usb/remove", param, 0, 0);
	}
}
#endif

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
#ifdef LINUX26
	char *device = getenv("DEVICENAME");
	int is_block = strcmp(getenv("SUBSYSTEM") ? : "", "block") == 0;
#else
	char *device = getenv("DEVICE");
#endif
	char *scsi_host = getenv("SCSI_HOST");

	_dprintf("%s hotplug INTERFACE=%s ACTION=%s PRODUCT=%s HOST=%s DEVICE=%s\n",
		getenv("SUBSYSTEM") ? : "USB", interface, action, product, scsi_host, device);

	if (!nvram_get_int("usb_enable")) return;
#ifdef LINUX26
	if (!action || ((!interface || !product) && !is_block))
#else
	if (!interface || !action || !product)	/* Hubs bail out here. */
#endif
		return;

	if (scsi_host)
		host = atoi(scsi_host);

	if (!wait_action_idle(10)) return;

	add = (strcmp(action, "add") == 0);
	if (add && (strncmp(interface ? : "", "TOMATO/", 7) != 0)) {
#ifdef LINUX26
		if (!is_block && device)
#endif
		syslog(LOG_DEBUG, "Attached USB device %s [INTERFACE=%s PRODUCT=%s]",
			device, interface, product);
#ifndef LINUX26
		/* To allow automount to be blocked on startup.
		 * In kernel 2.6 we still need to serialize mount/umount calls -
		 * so the lock is down below in the "block" hotplug processing.
		 */
		file_unlock(file_lock("usb"));
#endif
	}

	if (strncmp(interface ? : "", "TOMATO/", 7) == 0) {	/* web admin */
		if (scsi_host == NULL)
			host = atoi(product);	// for backward compatibility
		/* If host is negative, unmount all partitions of *all* hosts.
		 * If host == -1, execute "soft" unmount (do not kill NAS apps, no "lazy" umount).
		 * If host == -2, run "hard" unmount, as if the drive is unplugged.
		 * This feature can be used in custom scripts as following:
		 *
		 * # INTERFACE=TOMATO/1 ACTION=remove PRODUCT=-1 SCSI_HOST=-1 hotplug usb
		 *
		 * PRODUCT is required to pass the env variables verification.
		 */
		/* Unmount or remount all partitions of the host. */
		hotplug_usb_storage_device(host < 0 ? -1 : host, add ? -1 : 0,
			host == -2 ? 0 : EFH_USER);
	}
#ifdef LINUX26
	else if (is_block && strcmp(getenv("MAJOR") ? : "", "8") == 0 && strcmp(getenv("PHYSDEVBUS") ? : "", "scsi") == 0) {
		/* scsi partition */
		char devname[64];
		int lock;

		sprintf(devname, "/dev/%s", device);
		lock = file_lock("usb");
		if (add) {
			if (nvram_get_int("usb_storage") && nvram_get_int("usb_automount")) {
				int minor = atoi(getenv("MINOR") ? : "0");
				if ((minor % 16) == 0 && !is_no_partition(device)) {
					/* This is a disc, and not a "no-partition" device,
					 * like APPLE iPOD shuffle. We can't mount it.
					 */
					return;
				}
				if (mount_partition(devname, host, NULL, device, EFH_HP_ADD)) {
					restart_nas_services(0, 1); // restart all NAS applications
				}
			}
		}
		else {
			/* When unplugged, unmount the device even if usb storage is disabled in the GUI */
			umount_partition(devname, host, NULL, device, EFH_HP_REMOVE);
			/* Restart NAS applications (they could be killed by umount_mountpoint),
			 * or just re-read the configuration.
			 */
			restart_nas_services(0, 1);
		}
		file_unlock(lock);
	}
#endif
	else if (strncmp(interface ? : "", "8/", 2) == 0) {	/* usb storage */
#ifdef LINUX26
		usbled_proc(device, add);
#endif
		run_nvscript("script_usbhotplug", NULL, 2);
#ifndef LINUX26
		hotplug_usb_storage_device(host, add, (add ? EFH_HP_ADD : EFH_HP_REMOVE) | (host < 0 ? EFH_HUNKNOWN : 0));
#endif
	}
	else {	/* It's some other type of USB device, not storage. */
#ifdef LINUX26
		if (is_block) return;
#endif
#ifdef LINUX26
		if (strncmp(interface ? : "", "7/", 2) == 0)	/* printer */
			usbled_proc(device, add);
#endif
		/* Do nothing.  The user's hotplug script must do it all. */
		run_nvscript("script_usbhotplug", NULL, 2);
	}
}

