/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <sys/statfs.h>
#include <netdb.h>
#include <net/route.h>

// !!TB
//#include <sys/mount.h>
#include <mntent.h>

#include <wlioctl.h>
#include <wlutils.h>

// to javascript-safe string
char *js_string(const char *s)
{
	unsigned char c;
	char *buffer;
	char *b;

	if ((buffer = malloc((strlen(s) * 4) + 1)) != NULL) {
		b = buffer;
		while ((c = *s++) != 0) {
			if ((c == '"') || (c == '\'') || (c == '\\') || (!isprint(c))) {
				b += sprintf(b, "\\x%02x", c);
			}
			else {
				*b++ = c;
			}
		}
		*b = 0;
	}
	return buffer;
}

// to html-safe string
char *html_string(const char *s)
{
	unsigned char c;
	char *buffer;
	char *b;

	if ((buffer = malloc((strlen(s) * 6) + 1)) != NULL) {
		b = buffer;
		while ((c = *s++) != 0) {
			if ((c == '&') || (c == '<') || (c == '>') || (c == '"') || (c == '\'') || (!isprint(c))) {
				b += sprintf(b, "&#%d;", c);
			}
			else {
				*b++ = c;
			}
		}
		*b = 0;
	}
	return buffer;
}

// removes \r
char *unix_string(const char *s)
{
	char *buffer;
	char *b;
	char c;

	if ((buffer = malloc(strlen(s) + 1)) != NULL) {
		b = buffer;
		while ((c = *s++) != 0)
			if (c != '\r') *b++ = c;
		*b = 0;
	}
	return buffer;
}

// # days, ##:##:##
char *reltime(char *buf, time_t t)
{
	int days;
	int m;

	if (t < 0) t = 0;
	days = t / 86400;
	m = t / 60;
	sprintf(buf, "%d day%s, %02d:%02d:%02d", days, ((days==1) ? "" : "s"), ((m / 60) % 24), (m % 60), (int)(t % 60));
	return buf;
}

int get_client_info(char *mac, char *ifname)
{
	FILE *f;
	char s[256];
	char ips[16];

/*
# cat /proc/net/arp
IP address       HW type     Flags       HW address            Mask     Device
192.168.0.1      0x1         0x2         00:01:02:03:04:05     *        vlan1
192.168.1.5      0x1         0x2         00:05:06:07:08:09     *        br0
*/

	if ((f = fopen("/proc/net/arp", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%15s %*s %*s %17s %*s %16s", ips, mac, ifname) == 3) {
				if (inet_addr(ips) == clientsai.sin_addr.s_addr) {
					fclose(f);
					return 1;
				}
			}
		}
		fclose(f);
	}
	return 0;
}





//	<% lanip(mode); %>
//	<mode>
//		1		return first 3 octets (192.168.1)
//		2		return last octet (1)
//		else	return full (192.168.1.1)

void asp_lanip(int argc, char **argv)
{
	char *nv, *p;
	char s[64];
	char mode;

	mode = argc ? *argv[0] : 0;

	if ((nv = nvram_get("lan_ipaddr")) != NULL) {
		strcpy(s, nv);
		if ((p = strrchr(s, '.')) != NULL) {
			*p = 0;
            web_puts((mode == '1') ? s : (mode == '2') ? (p + 1) : nv);
		}
	}
}

void asp_lipp(int argc, char **argv)
{
	char *one = "1";
	asp_lanip(1, &one);
}

//	<% psup(process); %>
//	returns 1 if process is running

void asp_psup(int argc, char **argv)
{
	if (argc == 1) web_printf("%d", pidof(argv[0]) > 0);
}

/*
# cat /proc/meminfo
        total:    used:    free:  shared: buffers:  cached:
Mem:  14872576 12877824  1994752        0  1236992  4837376
Swap:        0        0        0
MemTotal:        14524 kB
MemFree:          1948 kB
MemShared:           0 kB
Buffers:          1208 kB
Cached:           4724 kB
SwapCached:          0 kB
Active:           4364 kB
Inactive:         2952 kB
HighTotal:           0 kB
HighFree:            0 kB
LowTotal:        14524 kB
LowFree:          1948 kB
SwapTotal:           0 kB
SwapFree:            0 kB

*/

typedef struct {
	unsigned long total;
	unsigned long free;
	unsigned long shared;
	unsigned long buffers;
	unsigned long cached;
	unsigned long swaptotal;
	unsigned long swapfree;
	unsigned long maxfreeram;
} meminfo_t;

static int get_memory(meminfo_t *m)
{
	FILE *f;
	char s[128];
	int ok = 0;

	if ((f = fopen("/proc/meminfo", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (strncmp(s, "Mem:", 4) == 0) {
				if (sscanf(s + 6, "%ld %*d %ld %ld %ld %ld", &m->total, &m->free, &m->shared, &m->buffers, &m->cached) == 5)
					++ok;
			}
			else if (strncmp(s, "SwapTotal:", 10) == 0) {
				m->swaptotal = strtoul(s + 12, NULL, 10) * 1024;
				++ok;
			}
			else if (strncmp(s, "SwapFree:", 9) == 0) {
				m->swapfree = strtoul(s + 11, NULL, 10) * 1024;
				++ok;
				break;
			}
		}
		fclose(f);
	}
	if (ok != 3) {
		memset(m, 0, sizeof(*m));
		return 0;
	}
	m->maxfreeram = m->free;
	if (nvram_match("t_cafree", "1")) m->maxfreeram += m->cached;
	return 1;
}

void asp_sysinfo(int argc, char **argv)
{
	struct sysinfo si;
	char s[64];
	meminfo_t mem;

	web_puts("\nsysinfo = {\n");
	sysinfo(&si);
	get_memory(&mem);
	web_printf(
		"\tuptime: %ld,\n"
		"\tuptime_s: '%s',\n"
		"\tloads: [%ld, %ld, %ld],\n"
		"\ttotalram: %ld,\n"
		"\tfreeram: %ld,\n"
		"\tshareram: %ld,\n"
		"\tbufferram: %ld,\n"
		"\tcached: %ld,\n"
		"\ttotalswap: %ld,\n"
		"\tfreeswap: %ld,\n"
		"\ttotalfreeram: %ld,\n"
		"\tprocs: %d\n",
			si.uptime,
			reltime(s, si.uptime),
			si.loads[0], si.loads[1], si.loads[2],
			mem.total, mem.free,
			mem.shared, mem.buffers, mem.cached,
			mem.swaptotal, mem.swapfree,
			mem.maxfreeram,
			si.procs);
	web_puts("};\n");
}

void asp_activeroutes(int argc, char **argv)
{
	FILE *f;
	char s[512];
	char dev[17];
	unsigned long dest;
	unsigned long gateway;
	unsigned long flags;
	unsigned long mask;
	unsigned metric;
	struct in_addr ia;
	char s_dest[16];
	char s_gateway[16];
	char s_mask[16];
	int n;

	web_puts("\nactiveroutes = [");
	n = 0;
	if ((f = fopen("/proc/net/route", "r")) != NULL) {
		while (fgets(s, sizeof(s), f)) {
			if (sscanf(s, "%16s%lx%lx%lx%*s%*s%u%lx", dev, &dest, &gateway, &flags, &metric, &mask) != 6) continue;
			if ((flags & RTF_UP) == 0) continue;
			if (dest != 0) {
				ia.s_addr = dest;
				strcpy(s_dest, inet_ntoa(ia));
			}
			else {
				strcpy(s_dest, "default");
			}
			if (gateway != 0) {
				ia.s_addr = gateway;
				strcpy(s_gateway, inet_ntoa(ia));
			}
			else {
				strcpy(s_gateway, "*");
			}
			ia.s_addr = mask;
			strcpy(s_mask, inet_ntoa(ia));
			web_printf("%s['%s','%s','%s','%s',%u]", n ? "," : "", dev, s_dest, s_gateway, s_mask, metric);
			++n;
		}
		fclose(f);
	}
	web_puts("];\n");
}

void asp_cgi_get(int argc, char **argv)
{
	const char *v;
	int i;

	for (i = 0; i < argc; ++i) {
		v = webcgi_get(argv[i]);
		if (v) web_puts(v);
	}
}

void asp_time(int argc, char **argv)
{
	time_t t;
	char s[64];

	t = time(NULL);
	if (t < Y2K) {
		web_puts("Not Available");
	}
	else {
		strftime(s, sizeof(s), "%a, %d %b %Y %H:%M:%S %z", localtime(&t));
		web_puts(s);
	}
}

void asp_wanup(int argc, char **argv)
{
	web_puts(check_wanup() ? "1" : "0");
}

void asp_wanstatus(int argc, char **argv)
{
	const char *p;

	if ((using_dhcpc()) && (f_exists("/var/lib/misc/dhcpc.renewing"))) {
		p = "Renewing...";
	}
	else if (check_wanup()) {
		p = "Connected";
	}
	else if (f_exists("/var/lib/misc/wan.connecting")) {
		p = "Connecting...";
	}
	else {
		p = "Disconnected";
	}
	web_puts(p);
}

void asp_link_uptime(int argc, char **argv)
{
	struct sysinfo si;
	char buf[64];
	long uptime;

	buf[0] = '-';
	buf[1] = 0;
	if (check_wanup()) {
		sysinfo(&si);
		if (f_read("/var/lib/misc/wantime", &uptime, sizeof(uptime)) == sizeof(uptime)) {
			reltime(buf, si.uptime - uptime);
		}
	}
	web_puts(buf);
}

void asp_bandwidth(int argc, char **argv)
{
	char *name;
	int sig;

	if ((nvram_match("rstats_enable", "1")) && (argc == 1)) {
		if (strcmp(argv[0], "speed") == 0) {
			sig = SIGUSR1;
			name = "/var/spool/rstats-speed.js";
		}
		else {
			sig = SIGUSR2;
			name = "/var/spool/rstats-history.js";
		}
		unlink(name);
		killall("rstats", sig);
		f_wait_exists(name, 5);
		do_file(name);
		unlink(name);
	}
}

void asp_rrule(int argc, char **argv)
{
	char s[32];
	int i;

	i = nvram_get_int("rruleN");
	sprintf(s, "rrule%d", i);
	web_puts("\nrrule = '");
	web_putj(nvram_safe_get(s));
	web_printf("';\nrruleN = %d;\n", i);
}

void asp_compmac(int argc, char **argv)
{
	char mac[32];
	char ifname[32];

	if (get_client_info(mac, ifname)) {
		web_puts(mac);
	}
}

void asp_ident(int argc, char **argv)
{
	web_puth(nvram_safe_get("router_name"));
}

void asp_statfs(int argc, char **argv)
{
	struct statfs sf;

	if (argc != 2) return;

	// used for /cifs/, /jffs/... if it returns squashfs type, assume it's not mounted
	if ((statfs(argv[0], &sf) != 0) || (sf.f_type == 0x73717368))
		memset(&sf, 0, sizeof(sf));

	web_printf(
			"\n%s = {\n"
			"\tsize: %llu,\n"
			"\tfree: %llu\n"
			"};\n",
			argv[1],
			((uint64_t)sf.f_bsize * sf.f_blocks),
			((uint64_t)sf.f_bsize * sf.f_bfree));
}

void asp_notice(int argc, char **argv)
{
	char s[256];
	char buf[2048];

	if (argc != 1) return;
	snprintf(s, sizeof(s), "/var/notice/%s", argv[0]);
	if (f_read_string(s, buf, sizeof(buf)) <= 0) return;
	web_putj(buf);
}

void wo_wakeup(char *url)
{
	char *mac;
	char *p;
	char *end;

	if ((mac = webcgi_get("mac")) != NULL) {
		end = mac + strlen(mac);
		while (mac < end) {
			while ((*mac == ' ') || (*mac == '\t') || (*mac == '\r') || (*mac == '\n')) ++mac;
			if (*mac == 0) break;

			p = mac;
			while ((*p != 0) && (*p != ' ') && (*p != '\r') && (*p != '\n')) ++p;
			*p = 0;

			eval("ether-wake", "-i", nvram_safe_get("lan_ifname"), mac);
			mac = p + 1;
		}
	}
	common_redirect();
}

void asp_dns(int argc, char **argv)
{
	char s[128];
	int i;
	const dns_list_t *dns;

	dns = get_dns();	// static buffer
	strcpy(s, "\ndns = [");
	for (i = 0 ; i < dns->count; ++i) {
		sprintf(s + strlen(s), "%s'%s'", i ? "," : "", inet_ntoa(dns->dns[i]));
	}
	strcat(s, "];\n");
	web_puts(s);
}

void wo_resolve(char *url)
{
	char *p;
	char *ip;
	struct hostent *he;
	struct in_addr ia;
	char comma;
	char *js;

	comma = ' ';
	web_puts("\nresolve_data = [\n");
	if ((p = webcgi_get("ip")) != NULL) {
		while ((ip = strsep(&p, ",")) != NULL) {
			ia.s_addr = inet_addr(ip);
			he = gethostbyaddr(&ia, sizeof(ia), AF_INET);
			js = js_string(he ? he->h_name : "");
			web_printf("%c['%s','%s']", comma, inet_ntoa(ia), js);
			free(js);
			comma = ',';
		}
	}
	web_puts("];\n");
}


//!!TB - USB support

#define PROC_SCSI_ROOT	"/proc/scsi"
#define USB_STORAGE	"usb-storage"

/* this value should not match any of the existing EFH_ values in shared.h */
#define EFH_PRINT	0x00000080	/* output partition list to the web response */

int is_partition_mounted(char *dev_name, int host_num, int disc_num, int part_num, uint flags)
{
	char the_label[128];
	char *type;
	int is_mounted = 0;
	struct mntent *mnt;
	struct statfs s;
	unsigned long psize = 0;

	if (!find_label(dev_name, the_label)) {
		sprintf(the_label, "disc%d_%d", disc_num, part_num);
	}

	if (flags & EFH_PRINT) {
		if (flags & EFH_1ST_DISC) {
			// [disc_no, [partitions array]],...
			web_printf("]],[%d,[", disc_num);
		}
		// [partition_name, is_mounted, mount_point, type, opts, size],...
		web_printf("%s['%s',", (flags & EFH_1ST_DISC) ? "" : ",", the_label);
	}

	if ((mnt = findmntents(dev_name, 0, 0, 0))) {
		is_mounted = 1;
		if (flags & EFH_PRINT) {
			if (statfs(mnt->mnt_dir, &s) == 0) {
				psize = (s.f_blocks * (unsigned long long) s.f_bsize + 1024*1024/2) / (1024*1024);
			}
			web_printf("1,'%s','%s','%s','%ld']",
				mnt->mnt_dir, mnt->mnt_type, mnt->mnt_opts, psize);
		}
	}
	else if ((mnt = findmntents(dev_name, 1, 0, 0))) {
		is_mounted = 1;
		if (flags & EFH_PRINT) {
			web_printf("2,'','%s','','%ld']",
				"swap", (atoi(mnt->mnt_type) + 1023) / 1024);
		}
	}
	else {
		if (flags & EFH_PRINT) {
			type = detect_fs_type(dev_name);
			web_printf("0,'','%s','','0']", type ? type : "");
		}
	}

	return is_mounted;
}

int is_host_mounted(int host_no, int print_parts)
{
	if (print_parts) web_puts("[-1,[");

	int fd = nvram_get_int("usb_nolock") ? -1 : file_lock("usb");
	int mounted = exec_for_host(
		host_no,
		0x00,
		print_parts ? EFH_PRINT : 0,
		is_partition_mounted);
	file_unlock(fd);
	
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
	DIR *scsi_dir=NULL, *usb_dir=NULL;
	struct dirent *dp, *scsi_dirent;
	uint host_no;
	int i = 0, attached, mounted;
	FILE *fp;
	char line[128];
	char *tmp=NULL, g_usb_vendor[30], g_usb_product[30], g_usb_serial[30];

	web_puts("\nusbdev = [");

	if (!nvram_match("usb_enable", "1")) {
		web_puts("];\n");
		return;
	}

	/* find all attached USB storage devices */
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
					if (attached) {
						/* Host no. assigned by scsi driver for this UFD */
						host_no = atoi(dp->d_name);
						web_printf("%s['Storage','%d','%s','%s','%s', [", i ? "," : "",
							host_no, g_usb_vendor, g_usb_product, g_usb_serial);
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

	/* now look for printers */
	usb_dir = opendir("/proc/usblp");
	while (usb_dir && (dp = readdir(usb_dir)))
	{
		if (!strcmp(dp->d_name, "..") || !strcmp(dp->d_name, "."))
			continue;
		
		/*
		sprintf(line, "/dev/usb/%s", dp->d_name);
		if (!f_exists(line))
			continue;
		*/
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
				web_printf("%s['Printer','%s','%s','%s','']", i ? "," : "",
					dp->d_name, g_usb_vendor, g_usb_product);
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
