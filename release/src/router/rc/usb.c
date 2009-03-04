/*

	USB Support

*/
#include "rc.h"

#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>

#include <sys/klog.h>
#include <sys/mount.h>
#include <mntent.h>
#include <dirent.h>


void start_usb(void)
{
	_dprintf("%s\n", __FUNCTION__);
	simple_unlock("usbhp");

	if (nvram_match("usb_enable", "1")) {
//		led(LED_AOSS, LED_ON);
		modprobe("usbcore");

		/* if enabled, force USB2 before USB1.1 */
		if (nvram_match("usb_usb2", "1")) {
			modprobe("ehci-hcd");
		}

		if (nvram_match("usb_uhci", "1")) {
			modprobe("usb-uhci");
		}

		if (nvram_match("usb_ohci", "1")) {
			modprobe("usb-ohci");
		}

		/* mount usb device filesystem */
        	mount("usbdevfs", "/proc/bus/usb", "usbdevfs", MS_MGC_VAL, NULL);

		if (nvram_match("usb_storage", "1")) {
			modprobe("scsi_mod");
			modprobe("sd_mod");
			modprobe("usb-storage");

			if (nvram_match("usb_fs_ext3", "1")) {
				modprobe("ext2");
				modprobe("jbd");
				modprobe("ext3");
			}

			if (nvram_match("usb_fs_fat", "1")) {
				modprobe("fat");
				modprobe("vfat");
			}

			probe_usb_mass(NULL, 1);
		}

		if (nvram_match("usb_printer", "1")) {
			modprobe("printer");
			// start printer server
			xstart("p910nd",
				nvram_match("usb_printer_bidirect", "1") ? "-b" : "", //bidirectional
				"-f", "/dev/usb/lp0", // device
				"0" // listen port
			);
			symlink("/dev/usb/lp0", "/dev/printers/0");
		}
	}
	else {
//		led(LED_AOSS, LED_OFF);
	}
}

void stop_usb(void)
{
	// Only stop printing service here, since there might be mounted USB partitions
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


#define MOUNT_VAL_FAIL 	0
#define MOUNT_VAL_RONLY	1
#define MOUNT_VAL_RW 	2
#define MOUNT_VAL_EXIST	3

int mount_r(char *mnt_dev, char *mnt_dir)
{
	struct mntent *mnt;
	char *type;
	int ret;
	char options[40];
	char flagfn[128];
	int dir_made;
	
	if ((mnt = findmntent(mnt_dev))) {
		syslog(LOG_INFO, "USB partition at %s already mounted on %s",
			mnt_dev, mnt->mnt_dir);
		return MOUNT_VAL_EXIST;
	}

	options[0] = 0;
	
	if ((type = detect_fs_type(mnt_dev))) 
	{
		unsigned long flags = MS_NOATIME;

		if (strcmp(type, "swap") == 0 || strcmp(type, "mbr") == 0) {
			/* not a mountable partition */
			flags = 0;
		}
		if (strcmp(type, "ext2") == 0 || strcmp(type, "ext3") == 0) {
			flags = flags | MS_NODIRATIME;
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
#endif
		else if (strcmp(type, "ntfs") == 0)
		{
			flags = MS_RDONLY;
#ifdef TCONFIG_SAMBASRV
			if (nvram_invmatch("smbd_cset", ""))
				sprintf(options, "iocharset=%s%s", 
					isdigit(nvram_get("smbd_cset")[0]) ? "cp" : "",
						nvram_get("smbd_cset"));
#endif
		}

		if (flags) {
			if ((dir_made = mkdir_if_none(mnt_dir))) {
				/* Create the flag file for remove the directory on dismount. */
				sprintf(flagfn, "%s/.autocreated-dir", mnt_dir);
				f_write(flagfn, NULL, 0, 0, 0);
			}

			ret = mount(mnt_dev, mnt_dir, type, flags, options[0] ? options : "");
			if (ret != 0) /* give it another try - guess fs */
				ret = eval("mount", "-o", "noatime", mnt_dev, mnt_dir);
			
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
 * The file to read is: /proc/scsi/usb-storage-#/#, where # is
 * the # from the symlink "../scsi/host#/busN/targetN/lunN"
 * that the /dev/discs/discDISK_NO is softlinked to.
 * We are looking for "Attached: Yes".
 */
static int usb_ufd_connected(int disc_no)
{
	char proc_file[128], line[256];
	FILE *fp;
	char *cp;
	int len;
	int host_no = disc_no;

	sprintf(proc_file, "%s/disc%d", DEV_DISCS_ROOT, disc_no);
	len = readlink(proc_file, line, sizeof(line)-1);
	if (len > 0) {
		line[len] = 0;
		cp = strstr(line, "/scsi/host");
		if (cp)
			host_no = atoi(cp + 10);
	}

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


/* Probe USB drive(s) to force usbdev/scsi subsystem to read the partition table(s).
 * 
 *   /dev/disks/discN/anything-non-existant
 *
 * There should be some way to associate the PRODUCT with a disc#, so we can
 * probe just that one newly-inserted or newly-removed drive.  Haven't found it yet, though.
 * It can be done by comparing things.  It's a lot of work, though, and this works just fine.
 * So it's not worth doing.
 * The only problem is that when probing all detached hosts, kernel emits a whole lot of
 * messages to the syslog. Don't know how to suppress them...
 *
 * This needs to be called *after* the umount.
 * This should called *before* the mount.
 * If it is already mounted or open, the driver will reject the revalidation request
 * with "Device busy for revalidation (usage=#)"
 * All kinds of funny things do (and don't!) manage to get the scsi subsystem to
 * read the device's partition table.  This makes _sure_ it gets done.
 */
void probe_usb_mass(char *product, int connected_only)
{
	DIR *usb_dev_disc;
	struct dirent *dp;
	char bf[128];
	struct stat statbuf;
	int i;

	if ((usb_dev_disc = opendir(DEV_DISCS_ROOT))) {
		while (usb_dev_disc && (dp = readdir(usb_dev_disc))) {
			if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
				continue;
			
			if (!connected_only || usb_ufd_connected(atoi(dp->d_name + 4))) {
				sprintf(bf, "%s/%s/part0", DEV_DISCS_ROOT, dp->d_name);
				i = stat(bf, &statbuf);	/* Force the partition table read. */
			}
		}
		closedir(usb_dev_disc);
	}
}


#define MNT_DETACH 0x00000002 /* from linux/fs.h */

/* Unmount.
 * If the special flagfile is now revealed, delete it and [attempt to] delete the directory.
 */
int umountdir(char *umount_dir, int lazy)
{
	int ret, count;
	char flagfn[128];
 
	sprintf(flagfn, "%s/.autocreated-dir", umount_dir);
	count=0;
	while ((ret = umount(umount_dir)) && (count < 2)) {
		count++;
		sleep(1);
	}

	if (ret && lazy) {
		/* make one more try to do a lazy unmount */
		ret = umount2(umount_dir, MNT_DETACH);
	}

	if (!ret) {
		syslog(LOG_INFO, "USB partition unmounted from %s", umount_dir);
		if ((unlink(flagfn) == 0)) {
			// Only delete the directory if it was auto-created
			rmdir(umount_dir);
		}
	}
	return ret;
}


int try_automount(void)
{
	if (f_exists("/tmp/etc/fstab")) {
		char *argv[] = { NULL, "-a", NULL };
		argv[0] = "swapon";
		_eval(argv, NULL, 0, NULL);
		argv[0] = "mount";
		return _eval(argv, NULL, 0, NULL);
	}
	return 0;
}


/* Loop through all USB partitions and either
 *	- try to mount them
 *	- try to unmount
 *	- consider only one specific device.
 * parameters:
 *   do_mount:
 *	> 0  = mount.
 *	= 0  = dismount.
 *	< 0  = dismount, but only for discs for which the host adapted is not connected.
 *
 *   do_all
 *	Non-Zero = Mount or unmount all partitions of all hosts.
 *	Zero = Do all partitions of the single host that matches 'host'.
 *   
 *   host: host no to unmount or remount.
 *   user: whether or not the process is initiated by a user from the Web GUI.
 *
 * Returns the # of devices mounted.
 */

int process_all_usb_part(int do_mount, int do_all, int host, int user)
{
	DIR *usb_dev_disc, *usb_dev_part;
	char usb_disc[128], mnt_dev[128], mnt_dir[128];
	char the_label[128], mnt_dir_label[128];
	struct dirent *dp, *dp_disc;
	struct mntent *mnt;
	int is_mounted = 0, is_unmounted, connected;
	int disc_no, host_no;
	int is_disc, i, mnt_r;
	char *cp;
	struct stat statbuf;

	usb_dev_disc = usb_dev_part = NULL;

	simple_lock("usbhp");	/* serialize using breakable file lock */

	if ((usb_dev_disc = opendir(DEV_DISCS_ROOT))) {

		/* First try to mount using /etc/fstab
		 * No need to increment is_mounted since this
		 * will not create new directories.
		 */
		if (do_mount > 0) try_automount();

		while ((dp = readdir(usb_dev_disc))) {
			if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
				continue;

			/* Disc no. assigned by scsi driver for this UFD when it is first plugged in.
			 * In the case of devices with multiple luns, each lun will be one disk, all
			 * on the same scsi host no.
			 */
			disc_no = atoi(dp->d_name + 4);
			host_no = disc_no;
			/* Find the actual host no in case of multiple luns */
			sprintf(usb_disc, "%s/disc%d", DEV_DISCS_ROOT, disc_no);
			i = readlink(usb_disc, the_label, sizeof(the_label)-1);
			if (i > 0) {
				the_label[i] = 0;
				cp = strstr(the_label, "/scsi/host");
				if (cp)
					host_no = atoi(cp + 10);
			}
			is_unmounted = 0;

			// Files created when the UFD is inserted are not removed when
			// it is removed. Verify the host device is still inserted. Strip
			// the "disc" and pass the rest of the string.
			connected = usb_ufd_connected(disc_no);

			//cprintf("Disc # %d is %s connected.  do_mount: %d\n", disc_no, connected?"": "not", do_mount);
			if (do_mount > 0 && !connected)
				/* Can't mount a device that isn't attached. */
				continue;
			if (do_mount < 0 && connected)
				/* Don't dismount devices that are still connected. */
				continue;
			if (!do_all && (host != host_no))
				/* If this request is for a specific host, ignore others */
				continue;

			/* For all the partitions on this disc.... */
			sprintf(usb_disc, "%s/%s", DEV_DISCS_ROOT, dp->d_name);

			if ((usb_dev_part = opendir(usb_disc))) {
				/* The first call to readdir makes the kernel realize "Disk
				 * change detected".  Except sometime the kernel does it even
	   			 * before the hotplug event gets send to us.  I think for a
				 * new device that it has never seen, it does it before
				 * sending the event.  But if it is a device that it has seen
				 * before, it doesn't read the partition table immediately,
				 * but waits until we poke it and then realizes the disk
				 * change has happened, and *then* reads it.
				 */
				while ((dp_disc = readdir(usb_dev_part))) {
					if (!strcmp(dp_disc->d_name, "..") || !strcmp(dp_disc->d_name, "."))
						continue;

					sprintf(mnt_dev, "%s/%s/%s", DEV_DISCS_ROOT, dp->d_name, dp_disc->d_name);

					/* Derive the fallback mountpoint directory name. */
					is_disc = !strcmp(dp_disc->d_name, "disc");
					if (is_disc)
						sprintf(mnt_dir, "%s/%s", MOUNT_ROOT, dp->d_name);
					else 
						sprintf(mnt_dir, "%s/%s_%s", MOUNT_ROOT, dp->d_name,
							dp_disc->d_name + (strncmp(dp_disc->d_name, "part", 4) ? 0 : 4));

					if (do_mount > 0) {	/* MOUNTING */
#if 0
						/* First "mount -a" call may fail if the device is plugged in
						 * for the first time, or if it was manually unmounted by calling
						 * umount command. However, there should be no need in this 
						 * workaround anymore after probing the device.
						 */
						if (automnt)
							automnt = try_automount();
#endif
						if (find_label(mnt_dev, the_label)) {
							sprintf(mnt_dir_label,"%s/%s", MOUNT_ROOT, the_label);
							if ((mnt_r = mount_r(mnt_dev, mnt_dir_label))) {
								if (mnt_r != MOUNT_VAL_EXIST)
									is_mounted++;
								continue;
							}
						}
						/* If it didn't mount as "label", fall thru and use derived name. */
						if ((mnt_r = mount_r(mnt_dev, mnt_dir)) && mnt_r != MOUNT_VAL_EXIST)
							is_mounted++;
					}
					else {	/* DISMOUNTING */
						/* There was a problem here.
						 * Under certain weird conditions, the "partN" is no longer in the /proc/partitions,
						 * but the disc is still mounted as "..../part1".  
						 * It won't get unmounted because no match.
						 */
						mnt = findmntent(mnt_dev);
						if (mnt) {
							if (user) {
								// unmount from Web
								// kill all NAS apps here so they are not keeping the device busy
								restart_nas_services(0);
							}
							umountdir(mnt->mnt_dir, (!user));
							is_unmounted++;
						}
					}
				}
				closedir(usb_dev_part);
			}
			if (is_unmounted) {
				/* If we just unmounted the disc, do a probe on it
				 * so we won't need to call probe_usb_mass() for all hosts
				 */
				sprintf(usb_disc, "%s/%s/part0", DEV_DISCS_ROOT, dp->d_name);
				i = stat(usb_disc, &statbuf);	/* Force the partition table read. */
			}
		}
		closedir(usb_dev_disc);

		/* Check for dangling devices. The partN entry went away before the umount got done.
		 * It won't necessarily be *this* disc, it might be *any* disc.
		 * Do this only _after_ normal processing is completed for all discs.
		 */
		if (do_mount <= 0) {
			while ((mnt = findmntent(""))) {
				//cprintf("dangling mnt_dev has mnt_dir: %s\n", mnt->mnt_dir);
				umountdir(mnt->mnt_dir, 1);
			}
		}
	}

	simple_unlock("usbhp");
	return is_mounted;
}


/* On an occurrance, multiple hotplug events may be fired off.
 * For example, if a hub is plugged or unplugged, an event
 * will be generated for everything downstream of it, plus one for
 * the hub itself.  These are fired off simultaneously, not serially.
 * This means that many many hotplug processes will be running at
 * the same time.
 * There is no reason that we should have to serialize this code, but
 * we have to.  Otherwise we will occasionally get a kernel Oops! when
 * we repeatedly plug and unplug a hub with several USB storage devices
 * connected to it.  This is undoubtedly a bug in the kernel.  Kinda
 * hard to debug, though, since we don't have a console on the router.
 * And nobody in kernel development will care, since all
 * the usb code & methodology has changed in 2.6.
 * Oh well-----serializing this code avoids the problem.
 */

/* The hotplug event generated by the kernel gives us several pieces
   of information:
   PRODUCT is vendorid/productid/rev#.
   DEVICE is /proc/bus/usb/bus#/dev#
   ACTION is add or remove

   I believe that we could somehow use DEVICE to figure out what
   /dev/discs/disc# is being reported.  This code ignores everything
   but ACTION, which is either "add" or "remove".  On an add (with
   automount enabled) we mount every partition on every USB mass
   storage device that is attached.  However, we decline to re-mount a
   device that is already mounted.  On a remove, we unmount every
   mounted device that is not attached.

   Note that when we get a hotplug add event, the USB susbsystem has
   *not* yet tried to read the partition table of the device.  And on
   a remove, the partition info has not yet been expunged.  The
   partitions show up as /dev/discs/disc#/part#, and /proc/partitions.
   Somewhere along the line, we do _something_ that causes the kernel
   to re-validate the device and update the partition table info.  To
   avoid mysterious problems, this code now explictly makes this
   happen.
   
   But this opens up a timing window wider.  If multiple events occur
   at once, and especially if adds and removes happen all at the same time
   (rare, but I now know how to make it happen), then the stuff that
   happens during the add processing will erase the /.../part# entries
   of the just-removed disc(s) and we now have a dangling mount device.
   We could't unmount it because we couldn't find the matching /proc/part#
   device---because it had already been deleted.

   To make this happen: plug in a USB stick and a USB camera card
   adapter.  Some (all??) adapters only generate a hotplug event when
   the adapter is plugged in, and do *not* generate an eveen when a
   new camera card is inserted into the adapter while the adapter is
   still plugged in.  So......the new card doesn't get mounted.  Now
   unplug the USB stick.  This generates a hotplug event.  The probing
   that we do will cause the kernel to revalidate all the devices.
   This will update the partition information of the camera card, and
   REMOVE the partition info of the USB stick.  Voila! We jabe a
   dangling device.  When we go to umount the USB stick (because its
   controller is not attached), we can't find the matching partition,
   so don't realize which one we should umount.  The /proc/mounts
   shows a mount to a device that doesn't exist anymore.  That's fixed
   now, though, by making a check for that.

   All-in-all, I'm still not too happy with this code.  I really would
   like to see it mess only with the one device that the event is
   reporting, rather than mess with (mount or umount) every device.
   Maybe later.....
 */

/* insert usb mass storage */
void hotplug_usb_mass(char *product)
{	
	_dprintf("%s %s product=%s\n", __FILE__, __FUNCTION__, product);
	if (!nvram_match("usb_automount", "1")) return;

	if (process_all_usb_part(1, 1, 0, 0)) {	/* Mount all partitions. */
		// restart all NAS applications
		restart_nas_services(1);
	}

	//run post-mount script if any
	run_nvscript("script_usbmount", product, 5);
}


/* This gets called at reboot or upgrade.  The system is stopping. */
void remove_storage_main(void)
{
	if (nvram_match("usb_enable", "1") && nvram_match("usb_storage", "1")) {
		if (nvram_match("usb_automount", "1")) {
			// run pre-unmount script if any
			run_nvscript("script_usbumount", NULL, 5);
		}
	}
	/* Unmount all partitions */
	process_all_usb_part(0, 1, 0, 0);
}


/* Plugging or removing usb device
 *
 * The $INTERFACE is "class/subclass/protocol"
 * Some interesting classes:
 *	8 = mass storage
 *	7 = printer
 *	3 = HID.   3/1/2 = mouse.
 *	6 = still image (6/1/1 = Digital camera Camera)
 *	9 = Hub
 *
 * Observed:
 *	Hub seems to have no INTERFACE (null), and TYPE of "9/0/0"
 *	Flash disk seems to have INTERFACE of "8/6/80", and TYPE of "0/0/0"
 *
 * Special values for Web Administration to unmount or remount
 * all partitions of the host:
 *	INTERFACE=TOMATO/...
 *	ACTION=add/remove
 *	PRODUCT=<host_no>
 * If host_no is negative, we unmount all partions of *all* hosts.
 */
void hotplug_usb(void)
{
	int add;
	int host = 0;
	char *interface = getenv("INTERFACE");
	char *action = getenv("ACTION");
	char *product = getenv("PRODUCT");

	_dprintf("USB hotplug INTERFACE=%s ACTION=%s PRODUCT=%s\n", interface, action, product);

	if (!nvram_match("usb_enable", "1")) return;
	if (!interface || !action || !product)	/* Hubs bail out here. */
		return;

	add = (strcmp(action, "add") == 0);
	if (add && (strncmp(interface, "TOMATO/", 7) != 0)) {
		syslog(LOG_INFO, "usb-hotplug: waiting for device to settle before scanning");
		sleep(2);
	}

	if (strncmp(interface, "TOMATO/", 7) == 0) {	/* web admin */
		host = atoi(product);
		/* Unmount or remount all partitions of the host. */
		if (!add) {	/* Dismounting */
			if (nvram_match("usb_storage", "1")) {
				// run pre-unmount script if any
				run_nvscript("script_usbumount", NULL, 5);
			}
			/* If host is negative, unmount all partitions of *all* hosts.
			 * This feature can be used in custom scripts as following:
			 *
			 * # INTERFACE=TOMATO/1 ACTION=remove PRODUCT=-1 hotplug usb
			 */
			process_all_usb_part(0, host < 0 ? 1 : 0, host, 1);
			restart_nas_services(1);
		}
		else {	/* Remounting a single host */
			if (process_all_usb_part(1, 0, host, 1))
				restart_nas_services(1);
			if (nvram_match("usb_storage", "1")) {
				//run post-mount script if any
				run_nvscript("script_usbmount", NULL, 5);
			}
		}
	}
	else if (strncmp(interface, "8/", 2) == 0) {	/* usb storage */
		if (add)
			probe_usb_mass(product, 1);	/* so the mount command can work */
		run_nvscript("script_usbhotplug", NULL, 2);

		if (add) {
			if (nvram_match("usb_storage", "1")) hotplug_usb_mass(product);
		}
		else {
			// unmount the device even if usb storage is disabled in the GUI
			process_all_usb_part(-1, 1, 0, 0);	// unmount all partitions on non-attached discs
			restart_nas_services(1);
		}
	}
	else {	/* It's some other type of USB device, not storage. */
		/* For now, do nothing.  The user's hotplug script must do it all. */
		run_nvscript("script_usbhotplug", NULL, 2);
	}
}
