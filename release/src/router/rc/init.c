/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/

#include "rc.h"

#include <termios.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <time.h>
#include <errno.h>
#include <paths.h>
#include <sys/wait.h>
#include <sys/reboot.h>
#include <sys/klog.h>
#include <wlutils.h>
#include <bcmdevs.h>

#define SHELL "/bin/sh"


enum {
	RESTART,
	STOP,
	START,
	USER1,
	IDLE,
	REBOOT,
	HALT,
	INIT
};

static int fatalsigs[] = {
	SIGILL,
	SIGABRT,
	SIGFPE,
	SIGPIPE,
	SIGBUS,
	SIGSYS,
	SIGTRAP,
	SIGPWR
};

static int initsigs[] = {
	SIGHUP,
	SIGUSR1,
	SIGUSR2,
	SIGINT,
	SIGQUIT,
	SIGTERM
};

static char *defenv[] = {
	"TERM=vt100",
	"HOME=/",
	"PATH=/usr/bin:/bin:/usr/sbin:/sbin",
	"SHELL=" SHELL,
	"USER=root",
	NULL
};

static int noconsole = 0;
static volatile int state = INIT;
static volatile int signaled = -1;


/* Set terminal settings to reasonable defaults */
static void set_term(int fd)
{
	struct termios tty;

	tcgetattr(fd, &tty);

	/* set control chars */
	tty.c_cc[VINTR]  = 3;	/* C-c */
	tty.c_cc[VQUIT]  = 28;	/* C-\ */
	tty.c_cc[VERASE] = 127; /* C-? */
	tty.c_cc[VKILL]  = 21;	/* C-u */
	tty.c_cc[VEOF]   = 4;	/* C-d */
	tty.c_cc[VSTART] = 17;	/* C-q */
	tty.c_cc[VSTOP]  = 19;	/* C-s */
	tty.c_cc[VSUSP]  = 26;	/* C-z */

	/* use line dicipline 0 */
	tty.c_line = 0;

	/* Make it be sane */
	tty.c_cflag &= CBAUD|CBAUDEX|CSIZE|CSTOPB|PARENB|PARODD;
	tty.c_cflag |= CREAD|HUPCL|CLOCAL;


	/* input modes */
	tty.c_iflag = ICRNL | IXON | IXOFF;

	/* output modes */
	tty.c_oflag = OPOST | ONLCR;

	/* local modes */
	tty.c_lflag =
		ISIG | ICANON | ECHO | ECHOE | ECHOK | ECHOCTL | ECHOKE | IEXTEN;

	tcsetattr(fd, TCSANOW, &tty);
}

static int console_init(void)
{
	int fd;

	/* Clean up */
	ioctl(0, TIOCNOTTY, 0);
	close(0);
	close(1);
	close(2);
	setsid();

	/* Reopen console */
	if ((fd = open(_PATH_CONSOLE, O_RDWR)) < 0) {
		/* Avoid debug messages is redirected to socket packet if no exist a UART chip, added by honor, 2003-12-04 */
		open("/dev/null", O_RDONLY);
		open("/dev/null", O_WRONLY);
		open("/dev/null", O_WRONLY);
		perror(_PATH_CONSOLE);
		return errno;
	}
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);

	ioctl(0, TIOCSCTTY, 1);
	tcsetpgrp(0, getpgrp());
	set_term(0);

	return 0;
}

/*
 * Waits for a file descriptor to change status or unblocked signal
 * @param	fd	file descriptor
 * @param	timeout	seconds to wait before timing out or 0 for no timeout
 * @return	1 if descriptor changed status or 0 if timed out or -1 on error
 */
static int waitfor(int fd, int timeout)
{
	fd_set rfds;
	struct timeval tv = { timeout, 0 };

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);
	return select(fd + 1, &rfds, NULL, NULL, (timeout > 0) ? &tv : NULL);
}

static pid_t run_shell(int timeout, int nowait)
{
	pid_t pid;
	int sig;

	/* Wait for user input */
	if (waitfor(STDIN_FILENO, timeout) <= 0) return 0;

	switch (pid = fork()) {
	case -1:
		perror("fork");
		return 0;
	case 0:
		/* Reset signal handlers set for parent process */
		for (sig = 0; sig < (_NSIG-1); sig++)
			signal(sig, SIG_DFL);

		/* Reopen console */
		console_init();
		printf("\n\nTomato %s\n\n", tomato_version);

		/* Now run it.  The new program will take over this PID,
		 * so nothing further in init.c should be run. */
		execve(SHELL, (char *[]) { SHELL, NULL }, defenv);

		/* We're still here?  Some error happened. */
		perror(SHELL);
		exit(errno);
	default:
		if (nowait) {
			return pid;
		}
		else {
			waitpid(pid, NULL, 0);
			return 0;
		}
	}
}

static void shutdn(int rb)
{
	int i;
	int act;
	sigset_t ss;

	_dprintf("shutdn rb=%d\n", rb);

	sigemptyset(&ss);
	for (i = 0; i < sizeof(fatalsigs) / sizeof(fatalsigs[0]); i++)
		sigaddset(&ss, fatalsigs[i]);
	for (i = 0; i < sizeof(initsigs) / sizeof(initsigs[0]); i++)
		sigaddset(&ss, initsigs[i]);
	sigaddset(&ss, SIGCHLD);
	sigprocmask(SIG_BLOCK, &ss, NULL);

	for (i = 30; i > 0; --i) {
		if (((act = check_action()) == ACT_IDLE) || (act == ACT_REBOOT)) break;
		cprintf("Busy with %d. Waiting before shutdown... %d\n", act, i);
		sleep(1);
	}
	set_action(ACT_REBOOT);

	cprintf("TERM\n");
	kill(-1, SIGTERM);
	sleep(2);
	sync();

	cprintf("KILL\n");
	kill(-1, SIGKILL);
	sleep(1);
	sync();

	umount("/jffs");	// may hang if not
	sleep(1);

	if (rb != -1) {
		led(LED_WLAN, 0);
		if (rb == 0) {
			for (i = 4; i > 0; --i) {
				led(LED_DMZ, 1);
				led(LED_WHITE, 1);
				usleep(250000);
				led(LED_DMZ, 0);
				led(LED_WHITE, 0);
				usleep(250000);
			}
		}
	}

	reboot(rb ? RB_AUTOBOOT : RB_HALT_SYSTEM);

	do {
		sleep(1);
	} while (1);
}

static void handle_fatalsigs(int sig)
{
	_dprintf("fatal sig=%d\n", sig);
	shutdn(-1);
}

void handle_reap(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {
		//
	}
}

static void handle_initsigs(int sig)
{
//	TRACE_PT("sig=%d state=%d, signaled=%d\n", sig, state, signaled);

	switch (sig) {
	case SIGHUP:
		signaled = RESTART;
		break;
	case SIGUSR1:
		signaled = USER1;
		break;
	case SIGUSR2:
		signaled = START;
		break;
	case SIGINT:
        signaled = STOP;
		break;
	case SIGTERM:
		signaled = REBOOT;
		break;
	case SIGQUIT:
		signaled = HALT;
		break;
	}
}

static int check_nv(const char *name, const char *value)
{
	const char *p;
	if (!nvram_match("manual_boot_nv", "1")) {
		if (((p = nvram_get(name)) == NULL) || (strcmp(p, value) != 0)) {
//			cprintf("Error: Critical variable %s is invalid. Resetting.\n", name);
			nvram_set(name, value);
			return 1;
		}
	}
	return 0;
}

static void check_bootnv(void)
{
	int dirty;
	int hardware;

	if (get_model() != MODEL_WRT54G) return;
	if (strncmp(nvram_safe_get("pmon_ver"), "CFE", 3) != 0) return;

	hardware = check_hw_type();
	if (!nvram_get("boardtype") ||
		!nvram_get("boardnum") ||
		!nvram_get("boardflags") ||
		!nvram_get("clkfreq") ||
		!nvram_get("os_flash_addr") ||
		!nvram_get("dl_ram_addr") ||
		!nvram_get("os_ram_addr") ||
		!nvram_get("scratch") ||
		!nvram_get("et0macaddr") ||
		((hardware != HW_BCM4704_BCM5325F) && (!nvram_get("vlan0ports") || !nvram_get("vlan0hwname")))) {
			cprintf("Unable to find critical settings, erasing NVRAM\n");
			mtd_erase("nvram");
			goto REBOOT;
	}

	dirty = 0;

	switch (hardware) {
	case HW_BCM5325E:
		/* Lower the DDR ram drive strength , the value will be stable for all boards
		   Latency 3 is more stable for all ddr 20050420 by honor */
		dirty |= check_nv("sdram_init", "0x010b");
		dirty |= check_nv("sdram_config", "0x0062");
		if (!nvram_match("debug_clkfix", "0")) {
			dirty |= check_nv("clkfreq", "216");
		}
		if (dirty) {
			nvram_set("sdram_ncdl", "0x0");
		}
		dirty |= check_nv("pa0itssit", "62");
		dirty |= check_nv("pa0b0", "0x15eb");
		dirty |= check_nv("pa0b1", "0xfa82");
		dirty |= check_nv("pa0b2", "0xfe66");
		dirty |= check_nv("pa0maxpwr", "0x4e");
		break;
	case HW_BCM5352E:	// G v4, GS v3, v4
		dirty |= check_nv("sdram_init", "0x010b");
		dirty |= check_nv("sdram_config", "0x0062");
		if (dirty) {
			nvram_set("sdram_ncdl", "0x0");
		}
		dirty |= check_nv("pa0itssit", "62");
		dirty |= check_nv("pa0b0", "0x168b");
		dirty |= check_nv("pa0b1", "0xfabf");
		dirty |= check_nv("pa0b2", "0xfeaf");
		dirty |= check_nv("pa0maxpwr", "0x4e");
		dirty |= check_nv("vlan0ports", "3 2 1 0 5*");
		break;
	case HW_BCM5354G:
		dirty |= check_nv("pa0itssit", "62");
		dirty |= check_nv("pa0b0", "0x1326");
		dirty |= check_nv("pa0b1", "0xFB51");
		dirty |= check_nv("pa0b2", "0xFE87");
		dirty |= check_nv("pa0maxpwr", "0x4e");
		break;
	case HW_BCM4704_BCM5325F:
		// nothing to do
		break;
	default:
		dirty |= check_nv("pa0itssit", "62");
		dirty |= check_nv("pa0b0", "0x170c");
		dirty |= check_nv("pa0b1", "0xfa24");
		dirty |= check_nv("pa0b2", "0xfe70");
		dirty |= check_nv("pa0maxpwr", "0x48");
		break;
	}

	if (dirty) {
		nvram_commit();
REBOOT:	// do a simple reboot
		sync();
		reboot(RB_AUTOBOOT);
        exit(0);
	}
}

static int init_nvram(void)
{
	unsigned long features;
	int model;
	const char *mfr;
	const char *name;
	char s[256];
	unsigned long bf;
	unsigned long n;
	
	model = get_model();
	sprintf(s, "%d", model);
	nvram_set("t_model", s);

	mfr = "Broadcom";
	name = NULL;
	features = 0;
	switch (model) {
	case MODEL_WRT54G:
		mfr = "Linksys";
		name = "WRT54G/GS/GL";
		switch (check_hw_type()) {
		case HW_BCM4712:
			nvram_set("gpio2", "adm_eecs");
			nvram_set("gpio3", "adm_eesk");
			nvram_unset("gpio4");
			nvram_set("gpio5", "adm_eedi");
			nvram_set("gpio6", "adm_rc");
			break;
		case HW_BCM4702:
			nvram_unset("gpio2");
			nvram_unset("gpio3");
			nvram_unset("gpio4");
			nvram_unset("gpio5");
			nvram_unset("gpio6");
			break;
		case HW_BCM5352E:
			nvram_set("opo", "0x0008");
			nvram_set("ag0", "0x02");
			// drop
		default:
			nvram_set("gpio2", "ses_led");
			nvram_set("gpio3", "ses_led2");
			nvram_set("gpio4", "ses_button");
			features = SUP_SES | SUP_WHAM_LED;
			break;
		}
		break;
	case MODEL_WTR54GS:
		mfr = "Linksys";
		name = "WTR54GS";
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set ("vlan0hwname", "et0");
			nvram_set ("vlan1hwname", "et0");
			nvram_set("vlan0ports", "0 5*");
			nvram_set("vlan1ports", "1 5");
			nvram_set("vlan_enable", "1");
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("gpio2", "ses_button");
			nvram_set("reset_gpio", "7");

		}
		nvram_set("pa0itssit", "62");
		nvram_set("pa0b0", "0x1542");
		nvram_set("pa0b1", "0xfacb");
		nvram_set("pa0b2", "0xfec7");
		nvram_set("pa0maxpwr", "0x4c");
		features = SUP_SES;
		break;
	case MODEL_WRTSL54GS:
		mfr = "Linksys";
		name = "WRTSL54GS";
		features = SUP_SES | SUP_WHAM_LED;
		break;
	case MODEL_WHRG54S:
		mfr = "Buffalo";
		name = "WHR-G54S";
		features = SUP_SES | SUP_AOSS_LED | SUP_BRAU;
		break;
	case MODEL_WHRHPG54:
	case MODEL_WZRRSG54HP:
	case MODEL_WZRHPG54:
		mfr = "Buffalo";
		features = SUP_SES | SUP_AOSS_LED | SUP_HPAMP;
		switch (model) {
		case MODEL_WZRRSG54HP:
			name = "WZR-RS-G54HP";
			break;
		case MODEL_WZRHPG54:
			name = "WZR-HP-G54";
			break;
		default:
			name = "WHR-HP-G54";
			features = SUP_SES | SUP_AOSS_LED | SUP_BRAU | SUP_HPAMP;
			break;
		}

		bf = strtoul(nvram_safe_get("boardflags"), NULL, 0);
		switch (bf) {
		case 0x0758:
		case 0x1758:
		case 0x2758:
		case 0x3758:
			if (nvram_match("wlx_hpamp", "")) {
				if (nvram_get_int("wl_txpwr") > 10) nvram_set("wl_txpwr", "10");
				nvram_set("wlx_hpamp", "1");
				nvram_set("wlx_hperx", "0");
			}

			n = bf;
			if (nvram_match("wlx_hpamp", "0")) {
				n &= ~0x2000UL;
			}
			else {
				n |= 0x2000UL;
			}
			if (nvram_match("wlx_hperx", "0")) {
				n |= 0x1000UL;
			}
			else {
				n &= ~0x1000UL;
			}
			if (bf != n) {
				sprintf(s, "0x%lX", n);
				nvram_set("boardflags", s);
			}
			break;
		default:
			syslog(LOG_WARNING, "Unexpected: boardflag=%lX", bf);
			break;
		}
		break;
	case MODEL_WBRG54:
		mfr = "Buffalo";
		name = "WBR-G54";
		nvram_set("wl0gpio0", "130");
		break;
	case MODEL_WBR2G54:
		mfr = "Buffalo";
		name = "WBR2-G54";
		features = SUP_SES | SUP_AOSS_LED;
		break;
	case MODEL_WHR2A54G54:
		mfr = "Buffalo";
		name = "WHR2-A54G54";
		features = SUP_SES | SUP_AOSS_LED | SUP_BRAU;
		break;
	case MODEL_WHR3AG54:
		mfr = "Buffalo";
		name = "WHR3-AG54";
		features = SUP_SES | SUP_AOSS_LED;
		break;
	case MODEL_WZRG54:
		mfr = "Buffalo";
		name = "WZR-G54";
		features = SUP_SES | SUP_AOSS_LED;
		break;
	case MODEL_WZRRSG54:
		mfr = "Buffalo";
		name = "WZR-RS-G54";
		features = SUP_SES | SUP_AOSS_LED;
		break;
	case MODEL_WVRG54NF:
		mfr = "Buffalo";
		name = "WVR-G54-NF";
		features = SUP_SES;
		break;
	case MODEL_WZRG108:
		mfr = "Buffalo";
		name = "WZR-G108";
		features = SUP_SES | SUP_AOSS_LED;
		break;
	case MODEL_RT390W:
	    mfr = "Fuji";
		name = "RT390W";
        break;
	case MODEL_WR850GV1:
		mfr = "Motorola";
		name = "WR850G v1";
		features = SUP_NONVE;
		break;
	case MODEL_WR850GV2:
		mfr = "Motorola";
		name = "WR850G v2/v3";
		features = SUP_NONVE;
		break;
	case MODEL_WL500GP:
		mfr = "Asus";
		name = "WL-500gP";
		features = SUP_SES;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("t_fix1", name);
			nvram_set("sdram_init", "0x0009");	// 32MB; defaults: 0x000b, 0x0009
			nvram_set("vlan1ports", "0 5");		// default: 0 5u
			nvram_set("lan_ifnames", "vlan0 eth1 eth2 eth3");	// set to "vlan0 eth2" by DD-WRT; default: vlan0 eth1
			// !!TB - WLAN LED fix
			nvram_set("wl0gpio0", "136");
		}		
		break;
	case MODEL_WL500GE:
		mfr = "Asus";
		name = "WL-500gE";
		//	features = ?
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("t_fix1", name);
			nvram_set("vlan1ports", "0 5");			// default: 0 5u
		}
		break;
	case MODEL_WX6615GT:
		mfr = "SparkLAN";
		name = "WX-6615GT";
		//	features = ?
		break;
	case MODEL_MN700:
		mfr = "Microsoft";
		name = "MN-700";
		break;
	case MODEL_WR100:
		mfr = "Viewsonic";
		name = "WR100";
		break;
#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif
#if WL_BSS_INFO_VERSION >= 108
/*
	case MODEL_WRH54G:
		mfr = "Linksys";
		name = "WRH54G";

		nvram_set("opo", "12");
		break;
*/
	case MODEL_WHRG125:
		mfr = "Buffalo";
		name = "WHR-G125";
		features = SUP_SES | SUP_AOSS_LED | SUP_BRAU;

		nvram_set("opo", "0x0008");
		nvram_set("ag0", "0x0C");
		break;
	case MODEL_WL500GPv2:
		mfr = "Asus";
		name = "WL-500gP v2";
		//	features = ?;
		if (!nvram_match("t_fix1", (char *)name)) {
			if (nvram_match("vlan1ports", "4 5u")) {
				nvram_set("vlan1ports", "4 5");
			}
			else if (nvram_match("vlan1ports", "0 5u")) {	// 520GU?
				nvram_set("vlan1ports", "0 5");
			}
		}
		break;
	case MODEL_WL520GU:
		mfr = "Asus";
		name = "WL-520GU";
		features = SUP_SES;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("t_fix1", name);
			nvram_set("vlan1ports", "0 5");
			// !!TB - LED fix
			nvram_set("wl0gpio0", "0");
			nvram_set("wl0gpio1", "136");
			nvram_set("wl0gpio2", "0");
			nvram_set("wl0gpio3", "0");
		}
		break;
#endif
#if TOMATO_N
	case MODEL_WZRG300N:
		mfr = "Buffalo";
		name = "WZR-G300N";
		features = SUP_SES | SUP_AOSS_LED | SUP_BRAU | SUP_80211N;
		break;
	case MODEL_WRT300N:
		mfr = "Linksys";
		name = "WRT300N";
		features = SUP_SES | SUP_80211N;
		break;
#endif
	}

	if (name) {
		sprintf(s, "%s %s", mfr, name);
	}
	else {
		snprintf(s, sizeof(s), "%s %d/%s/%s/%s/%s", mfr, check_hw_type(),
			nvram_safe_get("boardtype"), nvram_safe_get("boardnum"), nvram_safe_get("boardrev"), nvram_safe_get("boardflags"));
		s[64] = 0;
	}
	nvram_set("t_model_name", s);

	sprintf(s, "0x%lX", features);
	nvram_set("t_features", s);


	/*
	
		note: set wan_ifnameX if wan_ifname needs to be overriden
	
	*/
	if (nvram_is_empty("wan_ifnameX")) {
#if 1
		nvram_set("wan_ifnameX", ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) ||
				(check_hw_type() == HW_BCM4712)) ? "vlan1" : "eth1");
#else
		p = nvram_safe_get("wan_ifname");
		if ((*p == 0) || (nvram_match("wl_ifname", p))) {
			p = ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) ||
				(check_hw_type() == HW_BCM4712)) ? "vlan1" : "eth1";
		}
		nvram_set("wan_ifnameX", p);
#endif
	}


	nvram_set("wl_hwaddr", "");				// when disabling wireless, we must get null wireless mac 	??
	nvram_set("wl_country", "JP");
	nvram_set("wl_country_code", "JP");
	nvram_set("wan_get_dns", "");
	nvram_set("wan_get_domain", "");
	nvram_set("pppoe_pid0", "");
	nvram_set("action_service", "");
	nvram_set("jffs2_format", "0");
	nvram_set("rrules_radio", "-1");
	nvram_unset("https_crt_gen");
	if (nvram_get_int("http_id_gen") == 1) nvram_unset("http_id");

	nvram_unset("sch_rboot_last");
	nvram_unset("sch_rcon_last");
	nvram_unset("sch_c1_last");
	nvram_unset("sch_c2_last");
	nvram_unset("sch_c3_last");

	nvram_set("brau_state", "");
	if ((features & SUP_BRAU) == 0) nvram_set("script_brau", "");
	if ((features & SUP_SES) == 0) nvram_set("sesx_script", "");

	if (nvram_match("wl_net_mode", "disabled")) {
		nvram_set("wl_radio", "0");
		nvram_set("wl_net_mode", "mixed");
	}

	return 0;
}

static void sysinit(void)
{
	static const time_t tm = 0;
	int hardware;
	int i;
	DIR *d;
	struct dirent *de;
	char s[256];
	char t[256];
	int model;

	mount("", "/proc", "proc", 0, NULL);
	mount("", "/tmp", "ramfs", 0, NULL);

	if (console_init()) noconsole = 1;

	stime(&tm);

	static const char *mkd[] = {
		"/tmp/etc", "/tmp/var", "/tmp/home", "/tmp/mnt",
		"/var/log", "/var/run", "/var/tmp", "/var/lib", "/var/lib/misc",
		"/var/spool", "/var/spool/cron", "/var/spool/cron/crontabs", NULL
	};
	umask(0);
	for (i = 0; mkd[i]; ++i) {
		mkdir(mkd[i], 0755);
	}
	mkdir("/var/lock", 0777);
	mkdir("/var/tmp/dhcp", 0777);
	mkdir("/home/root", 0700);
	chmod("/tmp", 0777);
	f_write("/etc/hosts", NULL, 0, 0, 0644);			// blank
	simple_unlock("cron");
	simple_unlock("firewall");
	simple_unlock("restrictions");
	umask(022);

	if ((d = opendir("/rom/etc")) != NULL) {
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') continue;
			snprintf(s, sizeof(s), "%s/%s", "/rom/etc", de->d_name);
			snprintf(t, sizeof(t), "%s/%s", "/etc", de->d_name);
			symlink(s, t);
		}
		closedir(d);
	}
	symlink("/proc/mounts", "/etc/mtab");

	set_action(ACT_IDLE);

	for (i = 0; defenv[i]; ++i) {
		putenv(defenv[i]);
	}

	if (!noconsole) {
		printf("\n\nHit ENTER for console...\n\n");
		run_shell(1, 0);
	}

	check_bootnv();

	for (i = 0; i < sizeof(fatalsigs) / sizeof(fatalsigs[0]); i++) {
		signal(fatalsigs[i], handle_fatalsigs);
	}
	for (i = 0; i < sizeof(initsigs) / sizeof(initsigs[0]); i++) {
		signal(initsigs[i], handle_initsigs);
	}
	signal(SIGCHLD, handle_reap);

	switch (model = get_model()) {
	case MODEL_WR850GV1:
	case MODEL_WR850GV2:
		// need to cleanup some variables...
		if ((nvram_get("t_model") == NULL) && (nvram_get("MyFirmwareVersion") != NULL)) {
			nvram_unset("MyFirmwareVersion");
			nvram_set("restore_defaults", "1");
		}
		break;
	}

	system("nvram defaults --initcheck");
	init_nvram();

	klogctl(8, NULL, nvram_get_int("console_loglevel"));

	setup_conntrack();

	hardware = check_hw_type();
#if WL_BSS_INFO_VERSION >= 108
	modprobe("et");
#else
	if ((hardware == HW_BCM4702) && (model != MODEL_WR850GV1)) {
		modprobe("4702et");
		modprobe("diag");
	}
	else {
		modprobe("et");
	}
#endif
	modprobe("wl");
	modprobe("tomato_ct");

	set_host_domain_name();
	config_loopback();

	eval("buttons");

	start_jffs2();

	set_tz();

	led(LED_DIAG, 1);
}

int init_main(int argc, char *argv[])
{
	pid_t shell_pid = 0;

	sysinit();

	state = START;
	signaled = -1;

#if defined(DEBUG_NOISY)
	nvram_set("debug_logeval", "1");
	nvram_set("debug_cprintf", "1");
	nvram_set("debug_cprintf_file", "1");
	nvram_set("debug_ddns", "1");
#endif

	for (;;) {
//		TRACE_PT("main loop state=%d\n", state);

		switch (state) {
		case USER1:
			exec_service();
			state = IDLE;
			break;
		case RESTART:
		case STOP:
		case HALT:
		case REBOOT:
			led(LED_DIAG, 1);

			run_nvscript("script_shut", NULL, 10);

			stop_services();
			stop_wan();
			stop_lan();
			stop_vlan();

			if ((state == REBOOT) || (state == HALT)) {
				shutdn(state == REBOOT);
				exit(0);
			}
			if (state == STOP) {
				state = IDLE;
				break;
			}

			// RESTART falls through

		case START:
			SET_LED(RELEASE_WAN_CONTROL);

			run_nvscript("script_init", NULL, 2);

			start_vlan();
			start_lan();
			start_wan(BOOT);
			start_services();

			syslog(LOG_INFO, "Tomato %s", tomato_version);
			syslog(LOG_INFO, "%s", nvram_safe_get("t_model_name"));

			led(LED_DIAG, 0);

			state = IDLE;

			// fall through

		case IDLE:
			while (signaled == -1) {
				check_services();
				if ((!noconsole) && ((!shell_pid) || (kill(shell_pid, 0) != 0))) {
					shell_pid = run_shell(0, 1);
				}
				else {
					pause();
				}
			}
			state = signaled;
			signaled = -1;
			break;
		}
	}
}

int reboothalt_main(int argc, char *argv[])
{
	int reboot = (strstr(argv[0], "reboot") != NULL);
	puts(reboot ? "Rebooting..." : "Shutting down...");
	fflush(stdout);
	sleep(1);
	kill(1, reboot ? SIGTERM : SIGQUIT);
	return 0;
}

