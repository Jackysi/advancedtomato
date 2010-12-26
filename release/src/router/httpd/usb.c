/*

	Tomato Firmware
	USB Support

*/

#include "tomato.h"

#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/statfs.h>
#include <mntent.h>

#ifndef BLKGETSIZE
#define BLKGETSIZE _IO(0x12,96)
#endif
#ifndef BLKGETSIZE64
#define BLKGETSIZE64 _IOR(0x12,114,size_t)
#endif

static uint64_t get_psize(char *dev)
{
	uint64_t bytes = 0;
	unsigned long sectors;
	int fd;

	if ((fd = open(dev, O_RDONLY)) >= 0) {
		if (ioctl(fd, BLKGETSIZE64, &bytes) < 0) {
			bytes = 0;
			/* Can't get bytes, try 512 byte sectors */
			if (ioctl(fd, BLKGETSIZE, &sectors) >= 0)
				bytes = (uint64_t)sectors << 9;
		}
		close(fd);
	}

	return bytes;
}

#define PROC_SCSI_ROOT	"/proc/scsi"
#define USB_STORAGE	"usb-storage"

int is_partition_mounted(char *dev_name, int host_num, char *dsc_name, char *pt_name, uint flags)
{
	char the_label[128];
	char *type, *js;
	int is_mounted = 0;
	struct mntent *mnt;
	struct statfs sf;
	uint64_t size, fsize;

	type = find_label_or_uuid(dev_name, the_label, NULL);
	if (*the_label == 0) {
		strncpy(the_label, pt_name, sizeof(the_label));
	}

	if (flags & EFH_PRINT) {
		if (flags & EFH_1ST_DISC) {
			// [disc_name, [partitions array]],...
			web_printf("]],['%s',[", dsc_name);
		}
		// [partition_name, is_mounted, mount_point, type, opts, size, free],...
		js = js_string(the_label);
		web_printf("%s['%s',", (flags & EFH_1ST_DISC) ? "" : ",", js ? : "");
		free(js);
	}

	if ((mnt = findmntents(dev_name, 0, 0, 0))) {
		is_mounted = 1;
		if (flags & EFH_PRINT) {
			if (statfs(mnt->mnt_dir, &sf) == 0) {
				size = (uint64_t)sf.f_bsize * sf.f_blocks;
				fsize = (uint64_t)sf.f_bsize * sf.f_bfree;
			}
			else {
				size = get_psize(dev_name);
				fsize = 0;
			}
			web_printf("1,'%s','%s','%s',%llu,%llu]",
				mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts, size, fsize);
		}
	}
	else if ((mnt = findmntents(dev_name, 1, 0, 0))) {
		is_mounted = 1;
		if (flags & EFH_PRINT) {
			web_printf("2,'','swap','',%llu,0]",
				(uint64_t)atoi(mnt->mnt_type) * 1024);
		}
	}
	else {
		if (flags & EFH_PRINT) {
			web_printf("0,'','%s','',%llu,0]", type ? : "", get_psize(dev_name));
		}
	}

	return is_mounted;
}

int is_host_mounted(int host_no, int print_parts)
{
	if (print_parts) web_puts("[-1,[");

	int mounted = exec_for_host(
		host_no,
		0x00,
		print_parts ? EFH_PRINT : 0,
		is_partition_mounted);
	
	if (print_parts) web_puts("]]");

	return mounted;
}

/*
 * The disc # doesn't correspond to the host#, since there may be more than
 * one partition on a disk.
 * Nor does either correspond to the scsi host number.
 * And if the user plugs or unplugs a usb storage device after bringing up the
 * NAS:USB support page, the numbers won't match anymore, since "part#"s
 * may be added or deleted to the /dev/discs* or /dev/scsi**.
 *
 * But since we only need to support the devices list and mount/unmount 
 * functionality on the host level, the host# shoudl work ok. Just make sure
 * to always pass and use the _host#_, and not the disc#.
 */
void asp_usbdevices(int argc, char **argv)
{
	DIR *usb_dir;
	struct dirent *dp;
	uint host_no;
	int last_hn = -1;
	char *p, *p1;
	int i = 0, mounted;
	FILE *fp;
	char line[128];
	char *tmp, *js_vend, *js_prod;
	char g_usb_vendor[30], g_usb_product[30];

	web_puts("\nusbdev = [");

	if (!nvram_match("usb_enable", "1")) {
		web_puts("];\n");
		return;
	}

	/* find all attached USB storage devices */
#if 1	// NZ = Get the info from the SCSI subsystem.
	fp = fopen(PROC_SCSI_ROOT"/scsi", "r");
	if (fp) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			p = strstr(line, "Host: scsi");
			if (p) {
				host_no = atoi(p + 10);
				if (host_no == last_hn)
					continue;
				last_hn = host_no;
				if (fgets(line, sizeof(line), fp) != NULL) {
					memset(g_usb_vendor, 0, sizeof(g_usb_vendor));
					memset(g_usb_product, 0, sizeof(g_usb_product));
					p = strstr(line, "  Vendor: ");
					p1 = strstr(line + 10 + 8, " Model: ");
					if (p && p1) {
						strncpy(g_usb_vendor, p + 10, 8);
						strncpy(g_usb_product, p1 + 8, 16);
						js_vend = js_string(g_usb_vendor);
						js_prod = js_string(g_usb_product);
						web_printf("%s['Storage','%d','%s','%s','', [", i ? "," : "",
							host_no, js_vend ? : "", js_prod ? : "");
						free(js_vend);
						free(js_prod);
						mounted = is_host_mounted(host_no, 1);
						web_printf("], %d]", mounted);
						++i;
					}
				}
			}
		}
		fclose(fp);
	}
#else	// Get the info from the usb/storage subsystem.
	DIR *scsi_dir;
	struct dirent *scsi_dirent;
	char *g_usb_serial[30];
	int attached;

	scsi_dir = opendir(PROC_SCSI_ROOT);
	while (scsi_dir && (scsi_dirent = readdir(scsi_dir)))
	{
		if (!strncmp(USB_STORAGE, scsi_dirent->d_name, strlen(USB_STORAGE)))
		{
			sprintf(line, "%s/%s", PROC_SCSI_ROOT, scsi_dirent->d_name);
			usb_dir = opendir(line);
			while (usb_dir && (dp = readdir(usb_dir)))
			{
				if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
					continue;
				sprintf(line, "%s/%s/%s", PROC_SCSI_ROOT, scsi_dirent->d_name, dp->d_name);

				fp = fopen(line, "r");
				if (fp) {
					attached = 0;
					g_usb_vendor[0] = 0;
					g_usb_product[0] = 0;
					g_usb_serial[0] = 0;
					tmp = NULL;

					while (fgets(line, sizeof(line), fp) != NULL) {
						if (strstr(line, "Attached: Yes")) {
							attached = 1;
						}
						else if (strstr(line, "Vendor")) {
							tmp = strtok(line, " ");
							tmp = strtok(NULL, "\n");
							strncpy(g_usb_vendor, tmp, sizeof(g_usb_vendor) - 1);
							tmp = NULL;
						}
						else if (strstr(line, "Product")) {
							tmp = strtok(line, " ");
							tmp = strtok(NULL, "\n");
							strncpy(g_usb_product, tmp, sizeof(g_usb_product) - 1);
							tmp = NULL;
						}
						else if (strstr(line, "Serial Number")) {
							tmp = strtok(line, " ");
							tmp = strtok(NULL, " ");
							tmp = strtok(NULL, "\n");
							strncpy(g_usb_serial, tmp, sizeof(g_usb_serial) - 1);
							tmp = NULL;
						}
					}
					fclose(fp);
#ifdef LINUX26
					attached = (strlen(g_usb_product) > 0) || (strlen(g_usb_vendor) > 0);
#endif
					if (attached) {
						/* Host no. assigned by scsi driver for this UFD */
						host_no = atoi(dp->d_name);
						js_vend = js_string(g_usb_vendor);
						js_prod = js_string(g_usb_product);
						web_printf("%s['Storage','%d','%s','%s','%s', [", i ? "," : "",
							host_no, js_vend ? : "", js_prod ? : "", g_usb_serial);
						free(js_vend);
						free(js_prod);
						mounted = is_host_mounted(host_no, 1);
						web_printf("], %d]", mounted);
						++i;
					}
				}

			}
			if (usb_dir)
				closedir(usb_dir);
		}
	}
	if (scsi_dir)
		closedir(scsi_dir);
#endif

	/* now look for printers */
	usb_dir = opendir("/proc/usblp");
	while (usb_dir && (dp = readdir(usb_dir)))
	{
		if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
			continue;

		sprintf(line, "/proc/usblp/%s", dp->d_name);
		if ((fp = fopen(line, "r"))) {
			g_usb_vendor[0] = 0;
			g_usb_product[0] = 0;
			tmp = NULL;

			while (fgets(line, sizeof(line), fp) != NULL) {
				if (strstr(line, "Manufacturer")) {
					tmp = strtok(line, "=");
					tmp = strtok(NULL, "\n");
					strncpy(g_usb_vendor, tmp, sizeof(g_usb_vendor) - 1);
					tmp = NULL;
				}
				else if (strstr(line, "Model")) {
					tmp = strtok(line, "=");
					tmp = strtok(NULL, "\n");
					strncpy(g_usb_product, tmp, sizeof(g_usb_product) - 1);
					tmp = NULL;
				}
			}
			if ((strlen(g_usb_product) > 0) || (strlen(g_usb_vendor) > 0)) {
				js_vend = js_string(g_usb_vendor);
				js_prod = js_string(g_usb_product);
				web_printf("%s['Printer','%s','%s','%s','']", i ? "," : "",
					dp->d_name, js_vend ? : "", js_prod ? : "");
				free(js_vend);
				free(js_prod);
				++i;
			}

			fclose(fp);
		}
	}
	if (usb_dir)
		closedir(usb_dir);

	web_puts("];\n");
}

void wo_usbcommand(char *url)
{
	char *p;
	int add = 0;

	web_puts("\nusb = [\n");
	if ((p = webcgi_get("remove")) != NULL) {
		add = 0;
	}
	else if ((p = webcgi_get("mount")) != NULL) {
		add = 1;
	}
	if (p) {
		add_remove_usbhost(p, add);
		web_printf("%d", is_host_mounted(atoi(p), 0));
	}
	web_puts("];\n");
}
