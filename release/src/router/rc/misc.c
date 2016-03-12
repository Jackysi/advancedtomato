/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

#include <stdarg.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>
#include <netdb.h>

#include <bcmdevs.h>
#include <wlutils.h>
#include <dirent.h>
#include <sys/wait.h>


void usage_exit(const char *cmd, const char *help)
{
	fprintf(stderr, "Usage: %s %s\n", cmd, help);
	exit(1);
}

#if 0 // replaced by #define in rc.h
int modprobe(const char *mod)
{
#if 1
	return eval("modprobe", "-s", (char *)mod);
#else
	int r = eval("modprobe", "-s", (char *)mod);
	cprintf("modprobe %s = %d\n", mod, r);
	return r;
#endif
}
#endif // 0

int modprobe_r(const char *mod)
{
#if 1
	return eval("modprobe", "-r", (char *)mod);
#else
	int r = eval("modprobe", "-r", (char *)mod);
	cprintf("modprobe -r %s = %d\n", mod, r);
	return r;
#endif
}

#ifndef ct_modprobe
#ifdef LINUX26
#define ct_modprobe(mod, args...) ({ \
		modprobe("nf_conntrack_"mod, ## args); \
		modprobe("nf_nat_"mod); \
})
#else
#define ct_modprobe(mod, args...) ({ \
		modprobe("ip_conntrack_"mod, ## args); \
		modprobe("ip_nat_"mod, ## args); \
})
#endif
#endif

#ifndef ct_modprobe_r
#ifdef LINUX26
#define ct_modprobe_r(mod) ({ \
	modprobe_r("nf_nat_"mod); \
	modprobe_r("nf_conntrack_"mod); \
})
#else
#define ct_modprobe_r(mod) ({ \
	modprobe_r("ip_nat_"mod); \
	modprobe_r("ip_conntrack_"mod); \
})
#endif
#endif

/* 
 * The various child job starting functions:
 * _eval()
 *	Start the child. If ppid param is NULL, wait until the child exits.
 *	Otherwise, store the child's pid in ppid and return immediately.
 * eval()
 *	Call _eval with a NULL ppid, to wait for the child to exit.
 * xstart()
 *	Call _eval with a garbage ppid (to not wait), then return.
 * runuserfile
 *	Execute each executable in a directory that has the specified extention.
 *	Call _eval with a ppid (to not wait), then check every second for the child's pid.
 *	After wtime seconds or when the child has exited, return.
 *	If any such filename has an '&' character in it, then do *not* wait at
 *	all for the child to exit, regardless of the wtime.
 */

#define MAX_XSTART_ARGC 16
int _xstart(const char *cmd, ...)
{
	va_list ap;
	char *argv[MAX_XSTART_ARGC];
	int argc;
	int pid;

	argv[0] = (char *)cmd;
	argc = 1;
	va_start(ap, cmd);
	while ((argv[argc++] = va_arg(ap, char *)) != NULL) {
		if (argc >= MAX_XSTART_ARGC) {
			_dprintf("%s: too many parameters\n", __FUNCTION__);
			break;
		}
	}
	va_end(ap);

	return _eval(argv, NULL, 0, &pid);
}

static int endswith(const char *str, char *cmp)
{
	int cmp_len, str_len, i;

	cmp_len = strlen(cmp);
	str_len = strlen(str);
	if (cmp_len > str_len)
		return 0;
	for (i = 0; i < cmp_len; i++) {
		if (str[(str_len - 1) - i] != cmp[(cmp_len - 1) - i])
			return 0;
	}
	return 1;
}

static void execute_with_maxwait(char *const argv[], int wtime)
{
	pid_t pid;

	if (_eval(argv, NULL, 0, &pid) != 0)
		pid = -1;
	else {
		while (wtime-- > 0) {
			waitpid(pid, NULL, WNOHANG);	/* Reap the zombie if it has terminated. */
			if (kill(pid, 0) != 0) break;
			sleep(1);
		}
		_dprintf("%s killdon:   errno: %d    pid %d\n", argv[0], errno, pid);
	}
}

/* This is a bit ugly.  Why didn't they allow another parameter to filter???? */
static char *filter_extension;
static int endswith_filter(const struct dirent *entry)
{
	return endswith(entry->d_name, filter_extension);
}

/* If the filename has an '&' character in it, don't wait at all. */
void run_userfile(char *folder, char *extension, const char *arg1, int wtime)
{
	unsigned char buf[PATH_MAX + 1];
	char *argv[] = { buf, (char *)arg1, NULL };
	struct dirent **namelist;
	int i, n;

	/* Do them in sorted order. */
	filter_extension = extension;
	n = scandir(folder, &namelist, endswith_filter, alphasort);
	if (n >= 0) {
		for (i = 0; i < n; ++i) {
			sprintf(buf, "%s/%s", folder, namelist[i]->d_name);
			execute_with_maxwait(argv,
				strchr(namelist[i]->d_name, '&') ? 0 : wtime);
			free(namelist[i]);
		}
		free(namelist);
	}
}

/* Run user-supplied script(s), with 1 argument.
 * Return when the script(s) have finished,
 * or after wtime seconds, even if they aren't finished.
 *
 * Extract NAME from nvram variable named as "script_NAME".
 *
 * The sole exception to the nvram item naming rule is sesx.
 * That one is "sesx_script" rather than "script_sesx", due
 * to historical accident.
 *
 * The other exception is time-scheduled commands.
 * These have names that start with "sch_".
 * No directories are searched for corresponding user scripts.
 *
 * Execute in this order:
 *	nvram item: nv (run as a /bin/sh script)
 *		(unless nv starts with a dot)
 *	All files with a suffix of ".NAME" in these directories:
 *	/etc/config/
 *	/jffs/etc/config/
 *	/opt/etc/config/
 *	/mmc/etc/config/
 *	/tmp/config/
 */
/*
At this time, the names/events are:
   (Unless otherwise noted, there are no parameters.  Otherwise, one parameter).
   sesx		SES/AOSS Button custom script.  Param: ??
   brau		"bridge/auto" button pushed.  Param: mode (bridge/auto/etc)
   fire		When firewall service has been started or re-started.
   shut		At system shutdown, just before wan/lan/usb/etc. are stopped.
   init		At system startup, just before wan/lan/usb/etc. are started.
		The root filesystem and /jffs are mounted, but not any USB devices.
   usbmount	After an auto-mounted USB drive is mounted.
   usbumount	Before an auto-mounted USB drive is unmounted.
   usbhotplug	When any USB device is attached or removed.
   wanup	After WAN has come up.
   autostop	When a USB partition gets un-mounted.  Param: the mount-point (directory).
		If unmounted from the GUI, the directory is still mounted and accessible.
		If the USB drive was unplugged, it is still mounted but not accessible.

User scripts -- no directories are searched.  One parameter.
   autorun	When a USB disk partition gets auto-mounted. Param: the mount-point (directory).
		But not if the partition was already mounted.
		Only the files in that directory will be run.
*/
void run_nvscript(const char *nv, const char *arg1, int wtime)
{
	FILE *f;
	char *script;
	char s[PATH_MAX + 1];
	char *argv[] = { s, (char *)arg1, NULL };
	int check_dirs = 1;

	if (nv[0] == '.') {
		strcpy(s, nv);
	}
	else {
		script = nvram_get(nv);

		if ((script) && (*script != 0)) {
			sprintf(s, "/tmp/%s.sh", nv);
			if ((f = fopen(s, "w")) != NULL) {
				fputs("#!/bin/sh\n", f);
				fputs(script, f);
				fputs("\n", f);
				fclose(f);
				chmod(s, 0700);
				chdir("/tmp");

				_dprintf("Running: '%s %s'\n", argv[0], argv[1]? argv[1]: "");
				execute_with_maxwait(argv, wtime);
				chdir("/");
			}
		}

		sprintf(s, ".%s", nv);
		if (strncmp("sch_c", nv, 5) == 0) {
			check_dirs = 0;
		}
		else if (strncmp("sesx_", nv, 5) == 0) {
			s[5] = 0;
		}
		else if (strncmp("script_", nv, 7) == 0) {
			strcpy(&s[1], &nv[7]);
		}
	}

	if (nvram_match("userfiles_disable", "1")) {
		// backdoor to disable user scripts execution
		check_dirs = 0;
	}

	if ((check_dirs) && strcmp(s, ".") != 0) {
		_dprintf("checking for user scripts: '%s'\n", s);
		run_userfile("/etc/config", s, arg1, wtime);
		run_userfile("/jffs/etc/config", s, arg1, wtime);
		run_userfile("/opt/etc/config", s, arg1, wtime);
		run_userfile("/mmc/etc/config", s, arg1, wtime);
		run_userfile("/tmp/config", s, arg1, wtime);
	}
}

static void write_ct_timeout(const char *type, const char *name, unsigned int val)
{
	unsigned char buf[128];
	char v[16];

	sprintf(buf, "/proc/sys/net/ipv4/netfilter/ip_conntrack_%s_timeout%s%s",
		type, (name && name[0]) ? "_" : "", name ? name : "");
	sprintf(v, "%u", val);

	f_write_string(buf, v, 0, 0);
}

#ifndef write_tcp_timeout
#define write_tcp_timeout(name, val) write_ct_timeout("tcp", name, val)
#endif

#ifndef write_udp_timeout
#define write_udp_timeout(name, val) write_ct_timeout("udp", name, val)
#endif

static unsigned int read_ct_timeout(const char *type, const char *name)
{
	unsigned char buf[128];
	unsigned int val = 0;
	char v[16];

	sprintf(buf, "/proc/sys/net/ipv4/netfilter/ip_conntrack_%s_timeout%s%s",
		type, (name && name[0]) ? "_" : "", name ? name : "");
	if (f_read_string(buf, v, sizeof(v)) > 0)
		val = atoi(v);

	return val;
}

#ifndef read_tcp_timeout
#define read_tcp_timeout(name) read_ct_timeout("tcp", name)
#endif

#ifndef read_udp_timeout
#define read_udp_timeout(name) read_ct_timeout("udp", name)
#endif

void setup_conntrack(void)
{
	unsigned int v[10];
	const char *p;
	char buf[70];
	int i;

	p = nvram_safe_get("ct_tcp_timeout");
	if (sscanf(p, "%u%u%u%u%u%u%u%u%u%u",
		&v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9]) == 10) {	// lightly verify
		write_tcp_timeout("established", v[1]);
		write_tcp_timeout("syn_sent", v[2]);
		write_tcp_timeout("syn_recv", v[3]);
		write_tcp_timeout("fin_wait", v[4]);
		write_tcp_timeout("time_wait", v[5]);
		write_tcp_timeout("close", v[6]);
		write_tcp_timeout("close_wait", v[7]);
		write_tcp_timeout("last_ack", v[8]);
	}
	else {
		v[1] = read_tcp_timeout("established");
		v[2] = read_tcp_timeout("syn_sent");
		v[3] = read_tcp_timeout("syn_recv");
		v[4] = read_tcp_timeout("fin_wait");
		v[5] = read_tcp_timeout("time_wait");
		v[6] = read_tcp_timeout("close");
		v[7] = read_tcp_timeout("close_wait");
		v[8] = read_tcp_timeout("last_ack");
		sprintf(buf, "0 %u %u %u %u %u %u %u %u 0",
			v[1], v[2], v[3], v[4], v[5], v[6], v[7], v[8]);
		nvram_set("ct_tcp_timeout", buf);
	}

	p = nvram_safe_get("ct_udp_timeout");
	if (sscanf(p, "%u%u", &v[0], &v[1]) == 2) {
		write_udp_timeout(NULL, v[0]);
		write_udp_timeout("stream", v[1]);
	}
	else {
		v[0] = read_udp_timeout(NULL);
		v[1] = read_udp_timeout("stream");
		sprintf(buf, "%u %u", v[0], v[1]);
		nvram_set("ct_udp_timeout", buf);
	}

	p = nvram_safe_get("ct_timeout");
	if (sscanf(p, "%u%u", &v[0], &v[1]) == 2) {
		write_ct_timeout("generic", NULL, v[0]);
		write_ct_timeout("icmp", NULL, v[1]);
	}
	else {
		v[0] = read_ct_timeout("generic", NULL);
		v[1] = read_ct_timeout("icmp", NULL);
		sprintf(buf, "%u %u", v[0], v[1]);
		nvram_set("ct_timeout", buf);
	}

#ifdef LINUX26
	p = nvram_safe_get("ct_hashsize");
	i = atoi(p);
	if (i >= 127) {
		f_write_string("/sys/module/nf_conntrack/parameters/hashsize", p, 0, 0);
	}
	else if (f_read_string("/sys/module/nf_conntrack/parameters/hashsize", buf, sizeof(buf)) > 0) {
		if (atoi(buf) > 0) nvram_set("ct_hashsize", buf);
	}
#endif

	p = nvram_safe_get("ct_max");
	i = atoi(p);
	if (i >= 128) {
		f_write_string("/proc/sys/net/ipv4/netfilter/ip_conntrack_max", p, 0, 0);
	}
	else if (f_read_string("/proc/sys/net/ipv4/netfilter/ip_conntrack_max", buf, sizeof(buf)) > 0) {
		if (atoi(buf) > 0) nvram_set("ct_max", buf);
	}

	if (!nvram_match("nf_rtsp", "0")) {
		ct_modprobe("rtsp");
	}
	else {
		ct_modprobe_r("rtsp");
	}

	if (!nvram_match("nf_h323", "0")) {
		ct_modprobe("h323");
	}
	else {
		ct_modprobe_r("h323");
	}

#ifdef LINUX26
	if (!nvram_match("nf_sip", "0")) {
		ct_modprobe("sip");
	}
	else {
		ct_modprobe_r("sip");
	}
#endif

	// !!TB - FTP Server
#ifdef TCONFIG_FTP
	i = nvram_get_int("ftp_port");
	if (nvram_match("ftp_enable", "1") && (i > 0) && (i != 21))
	{
		char ports[32];

		sprintf(ports, "ports=21,%d", i);
		ct_modprobe("ftp", ports);
	}
	else 
#endif
	if (!nvram_match("nf_ftp", "0")
#ifdef TCONFIG_FTP
		|| nvram_match("ftp_enable", "1")	// !!TB - FTP Server
#endif
		) {
		ct_modprobe("ftp");
	}
	else {
		ct_modprobe_r("ftp");
	}

	if (!nvram_match("nf_pptp", "0")) {
		ct_modprobe("proto_gre");
		ct_modprobe("pptp");
	}
	else {
		ct_modprobe_r("pptp");
		ct_modprobe_r("proto_gre");
	}
}

int host_addr_info(const char *name, int af, struct sockaddr_storage *buf)
{
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;
	int err;
	int addrtypes = 0;

	memset(&hints, 0, sizeof hints);
#ifdef TCONFIG_IPV6
	switch (af & (IPT_V4 | IPT_V6)) {
	case IPT_V4:
		hints.ai_family = AF_INET;
		break;
	case IPT_V6:
		hints.ai_family = AF_INET6;
		break;
	//case (IPT_V4 | IPT_V6):
	//case 0: // error?
	default:
		hints.ai_family = AF_UNSPEC;
	}
#else
	hints.ai_family = AF_INET;
#endif
	hints.ai_socktype = SOCK_RAW;

	if ((err = getaddrinfo(name, NULL, &hints, &res)) != 0) {
		return addrtypes;
	}

	for(p = res; p != NULL; p = p->ai_next) {
		switch(p->ai_family) {
		case AF_INET:
			addrtypes |= IPT_V4;
			break;
		case AF_INET6:
			addrtypes |= IPT_V6;
			break;
		}
		if (buf && (hints.ai_family == p->ai_family) && res->ai_addrlen) {
			memcpy(buf, res->ai_addr, res->ai_addrlen);
		}
	}

	freeaddrinfo(res);
	return (addrtypes & af);
}

inline int host_addrtypes(const char *name, int af)
{
	return host_addr_info(name, af, NULL);
}

void inc_mac(char *mac, int plus)
{
	unsigned char m[6];
	int i;

	for (i = 0; i < 6; i++)
		m[i] = (unsigned char) strtol(mac + (3 * i), (char **)NULL, 16);
	while (plus != 0) {
		for (i = 5; i >= 3; --i) {
			m[i] += (plus < 0) ? -1 : 1;
			if (m[i] != 0) break;	// continue if rolled over
		}
		plus += (plus < 0) ? 1 : -1;
	}
	sprintf(mac, "%02X:%02X:%02X:%02X:%02X:%02X",
		m[0], m[1], m[2], m[3], m[4], m[5]);
}

void set_mac(const char *ifname, const char *nvname, int plus)
{
	int sfd;
	struct ifreq ifr;
	int up;
	int j;

	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
		return;
	}

	strcpy(ifr.ifr_name, ifname);

	up = 0;
	if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0) {
		if ((up = ifr.ifr_flags & IFF_UP) != 0) {
			ifr.ifr_flags &= ~IFF_UP;
			if (ioctl(sfd, SIOCSIFFLAGS, &ifr) != 0) {
				_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
			}
		}
	}
	else {
		_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
	}

	if (!ether_atoe(nvram_safe_get(nvname), (unsigned char *)&ifr.ifr_hwaddr.sa_data)) {
		if (!ether_atoe(nvram_safe_get("et0macaddr"), (unsigned char *)&ifr.ifr_hwaddr.sa_data)) {

			// goofy et0macaddr, make something up
			nvram_set("et0macaddr", "00:01:23:45:67:89");
			ifr.ifr_hwaddr.sa_data[0] = 0;
			ifr.ifr_hwaddr.sa_data[1] = 0x01;
			ifr.ifr_hwaddr.sa_data[2] = 0x23;
			ifr.ifr_hwaddr.sa_data[3] = 0x45;
			ifr.ifr_hwaddr.sa_data[4] = 0x67;
			ifr.ifr_hwaddr.sa_data[5] = 0x89;
		}

		while (plus-- > 0) {
			for (j = 5; j >= 3; --j) {
				ifr.ifr_hwaddr.sa_data[j]++;
				if (ifr.ifr_hwaddr.sa_data[j] != 0) break;	// continue if rolled over
			}
		}
	}

	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	if (ioctl(sfd, SIOCSIFHWADDR, &ifr) == -1) {
		_dprintf("Error setting %s address\n", ifname);
	}

	if (up) {
		if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0) {
			ifr.ifr_flags |= IFF_UP|IFF_RUNNING;
			if (ioctl(sfd, SIOCSIFFLAGS, &ifr) == -1) {
				_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
			}
		}
		else {
			_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
		}
	}

	close(sfd);
}

/*
const char *default_wanif(void)
{
	return ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) ||
		(check_hw_type() == HW_BCM4712)) ? "vlan1" : "eth1";
}
*/

/*
const char *default_wlif(void)
{
	switch (check_hw_type()) {
	case HW_BCM4702:
	case HW_BCM4704_BCM5325F:
	case HW_BCM4704_BCM5325F_EWC:
		return "eth2";
	}
	return "eth1";

}
*/

int _vstrsep(char *buf, const char *sep, ...)
{
	va_list ap;
	char **p;
	int n;

	n = 0;
	va_start(ap, sep);
	while ((p = va_arg(ap, char **)) != NULL) {
		if ((*p = strsep(&buf, sep)) == NULL) break;
		++n;
	}
	va_end(ap);
	return n;
}

void simple_unlock(const char *name)
{
	char fn[256];

	snprintf(fn, sizeof(fn), "/var/lock/%s.lock", name);
	f_write(fn, NULL, 0, 0, 0600);
}

void simple_lock(const char *name)
{
	int n;
	char fn[256];

	n = 5 + (getpid() % 10);
	snprintf(fn, sizeof(fn), "/var/lock/%s.lock", name);
	while (unlink(fn) != 0) {
		if (--n == 0) {
			syslog(LOG_DEBUG, "Breaking %s", fn);
			break;
		}
		sleep(1);
	}
}

void killall_tk(const char *name)
{
	int n;

	if (killall(name, SIGTERM) == 0) {
		n = 50;
		while ((killall(name, 0) == 0) && (n-- > 0)) {
			_dprintf("%s: waiting name=%s n=%d\n", __FUNCTION__, name, n);
			usleep(100 * 1000);
		}
		if (n < 0) {
			n = 100;
			while ((killall(name, SIGKILL) == 0) && (n-- > 0)) {
				_dprintf("%s: SIGKILL name=%s n=%d\n", __FUNCTION__, name, n);
				usleep(100 * 1000);
			}
		}
	}
}

int kill_pidfile_s(char *pidfile, int sig)
{
	char tmp[100];
	int pid;
	
	if (f_read_string(pidfile, tmp, sizeof(tmp)) > 0) {
		if ((pid = atoi(tmp)) > 1) {
			return kill(pid, sig);
		}
	}	
	return -1;
}

/*
 * Return non-zero if we created the directory,
 * and zero if it already existed.
 */
int mkdir_if_none(const char *path)
{
	DIR *dp;

	if (!(dp = opendir(path))) {
		eval("mkdir", "-m", "0777", "-p", (char *)path);
		return 1;
	}
	closedir(dp);
	return 0;
}

long fappend(FILE *out, const char *fname)
{
	FILE *in;
	char buf[1024];
	int n;
	long r;

	if ((in = fopen(fname, "r")) == NULL) return -1;
	r = 0;
	while ((n = fread(buf, 1, sizeof(buf), in)) > 0) {
		if (fwrite(buf, 1, n, out) != n) {
			r = -1;
			break;
		}
		else {
			r += n;
		}
	}
	fclose(in);
	return r;
}

long fappend_file(const char *path, const char *fname)
{
	FILE *f;
	int r = -1;

	if (f_exists(fname) && (f = fopen(path, "a")) != NULL) {
		r = fappend(f, fname);
		fclose(f);
	}
	return r;
}
