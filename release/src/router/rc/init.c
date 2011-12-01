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
#ifdef LINUX26
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#endif
#include <wlutils.h>
#include <bcmdevs.h>

#define SHELL "/bin/sh"

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
	SIGALRM,
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

int console_main(int argc, char *argv[])
{
	for (;;) run_shell(0, 0);

	return 0;
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
	sigprocmask(SIG_BLOCK, &ss, NULL);

	for (i = 30; i > 0; --i) {
		if (((act = check_action()) == ACT_IDLE) || (act == ACT_REBOOT)) break;
		_dprintf("Busy with %d. Waiting before shutdown... %d\n", act, i);
		sleep(1);
	}
	set_action(ACT_REBOOT);

	// Disconnect pppd - need this for PPTP/L2TP to finish gracefully
	stop_pptp();
	stop_l2tp();

	_dprintf("TERM\n");
	kill(-1, SIGTERM);
	sleep(3);
	sync();

	_dprintf("KILL\n");
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

/* Fixed the race condition & incorrect code by using sigwait()
 * instead of pause(). But SIGCHLD is a problem, since other
 * code: 1) messes with it and 2) depends on CHLD being caught so
 * that the pid gets immediately reaped instead of left a zombie.
 * Pidof still shows the pid, even though it's in zombie state.
 * So this SIGCHLD handler reaps and then signals the mainline by
 * raising ALRM.
 */
static void handle_reap(int sig)
{
	chld_reap(sig);
	raise(SIGALRM);
}

static int check_nv(const char *name, const char *value)
{
	const char *p;
	if (!nvram_match("manual_boot_nv", "1")) {
		if (((p = nvram_get(name)) == NULL) || (strcmp(p, value) != 0)) {
			_dprintf("Error: Critical variable %s is invalid. Resetting.\n", name);
			nvram_set(name, value);
			return 1;
		}
	}
	return 0;
}

static inline int invalid_mac(const char *mac)
{
	return (!mac || !(*mac) || strncasecmp(mac, "00:90:4c", 8) == 0);
}

static int find_sercom_mac_addr(void)
{
	FILE *fp;
	unsigned char m[6], s[18];

	sprintf(s, MTD_DEV(%dro), 0);
	if ((fp = fopen(s, "rb"))) {
		fseek(fp, 0x1ffa0, SEEK_SET);
		fread(m, sizeof(m), 1, fp);
		fclose(fp);
		sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X",
			m[0], m[1], m[2], m[3], m[4], m[5]);
		nvram_set("et0macaddr", s);
		return !invalid_mac(s);
	}
	return 0;
}

static int find_dir320_mac_addr(void)
{
	FILE *fp;
	char *buffer, s[18];
	int i, part, size, found = 0;

	if (!mtd_getinfo("board_data", &part, &size))
		goto out;
	sprintf(s, MTD_DEV(%dro), part);

	if ((fp = fopen(s, "rb"))) {
		buffer = malloc(size);
		memset(buffer, 0, size);
		fread(buffer, size, 1, fp);
		if (!memcmp(buffer, "RGCFG1", 6)) {
			for (i = 6; i < size - 24; i++) {
				if (!memcmp(buffer + i, "lanmac=", 7)) {
					memcpy(s, buffer + i + 7, 17);
					s[17] = 0;
					nvram_set("et0macaddr", s);
					found = 1;
				}
				else if (!memcmp(buffer + i, "wanmac=", 7)) {
					memcpy(s, buffer + i + 7, 17);
					s[17] = 0;
					nvram_set("il0macaddr", s);
					if (!found) {
						inc_mac(s, -1);
						nvram_set("et0macaddr", s);
					}
					found = 1;
				}
			}
		}
		free(buffer);
		fclose(fp);
	}
out:
	if (!found) {
		strcpy(s, nvram_safe_get("wl0_hwaddr"));
		inc_mac(s, -2);
		nvram_set("et0macaddr", s);
	}
	return 1;
}

static int init_vlan_ports(void)
{
	int dirty = 0;
	int model = get_model();

	switch (model) {
	case MODEL_WRT54G:
		switch (check_hw_type()) {
		case HW_BCM5352E:	// G v4, GS v3, v4
			dirty |= check_nv("vlan0ports", "3 2 1 0 5*");
			break;
		}
		break;
	case MODEL_WTR54GS:
		dirty |= check_nv("vlan0ports", "0 5*");
		dirty |= check_nv("vlan1ports", "1 5");
		dirty |= check_nv("vlan_enable", "1");
		break;
	case MODEL_WL500GP:
	case MODEL_WL500GE:
	case MODEL_WL500GPv2:
	case MODEL_WL520GU:
		if (nvram_match("vlan1ports", "0 5u"))	// 520GU or WL500GE?
			dirty |= check_nv("vlan1ports", "0 5");
		else if (nvram_match("vlan1ports", "4 5u"))
			dirty |= check_nv("vlan1ports", "4 5");
		break;
	case MODEL_WL500GD:
		dirty |= check_nv("vlan0ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan1ports", "0 5");
		break;
	case MODEL_DIR320:
	case MODEL_H618B:
		dirty |= (nvram_get("vlan2ports") != NULL);
		nvram_unset("vlan2ports");
		dirty |= check_nv("vlan1ports", "0 5");
		break;
	case MODEL_WRT310Nv1:
		dirty |= check_nv("vlan1ports", "1 2 3 4 8*");
		dirty |= check_nv("vlan2ports", "0 8");
		break;
	case MODEL_WL1600GL:
		dirty |= check_nv("vlan0ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan1ports", "4 5");
		break;
#ifdef CONFIG_BCMWL5
	case MODEL_WNR3500L:
	case MODEL_WRT320N:
	case MODEL_RTN16:
	case MODEL_RTN66U:
		dirty |= check_nv("vlan1ports", "4 3 2 1 8*");
		dirty |= check_nv("vlan2ports", "0 8");
		break;
	case MODEL_WNR2000v2:
		dirty |= check_nv("vlan0ports", "4 3 2 1 5*");
		dirty |= check_nv("vlan1ports", "0 5");
		break;
	case MODEL_RTN10:
		dirty |= check_nv("vlan1ports", "4 5");
		break;
	case MODEL_RTN12:
		dirty |= check_nv("vlan0ports", "3 2 1 0 5*");
		dirty |= check_nv("vlan1ports", "4 5");
		break;
	case MODEL_WRT610Nv2:
	case MODEL_F5D8235v3:
		dirty |= check_nv("vlan1ports", "1 2 3 4 8*");
		dirty |= check_nv("vlan2ports", "0 8");
		break;
	case MODEL_F7D3301:
	case MODEL_F7D4301:
		dirty |= check_nv("vlan1ports", "3 2 1 0 8*");
		dirty |= check_nv("vlan2ports", "4 8");
		break;
	case MODEL_F7D3302:
	case MODEL_F7D4302:
		dirty |= check_nv("vlan1ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_E4200:
		dirty |= check_nv("vlan1ports", "0 1 2 3 8*");
		dirty |= check_nv("vlan2ports", "4 8");
		break;
	case MODEL_WRT160Nv3:
		if (nvram_match("vlan1ports", "1 2 3 4 5*")) {
			// fix lan port numbering on CSE41, CSE51
			dirty |= check_nv("vlan1ports", "4 3 2 1 5*");
		}
		else if (nvram_match("vlan1ports", "1 2 3 4 8*")) {
			// WRT310Nv2 ?
			dirty |= check_nv("vlan1ports", "4 3 2 1 8*");
		}
		break;
#endif
	}

	return dirty;
}

static void check_bootnv(void)
{
	int dirty;
	int hardware;
	int model;
	char mac[18];

	model = get_model();
	dirty = check_nv("wl0_leddc", "0x640000") | check_nv("wl1_leddc", "0x640000");

	switch (model) {
	case MODEL_WTR54GS:
		dirty |= check_nv("vlan0hwname", "et0");
		dirty |= check_nv("vlan1hwname", "et0");
		break;
	case MODEL_WBRG54:
		dirty |= check_nv("wl0gpio0", "130");
		break;
	case MODEL_WR850GV1:
	case MODEL_WR850GV2:
		// need to cleanup some variables...
		if ((nvram_get("t_model") == NULL) && (nvram_get("MyFirmwareVersion") != NULL)) {
			nvram_unset("MyFirmwareVersion");
			nvram_set("restore_defaults", "1");
		}
		break;
	case MODEL_WL500W:
		/* fix WL500W mac adresses for WAN port */
		if (invalid_mac(nvram_get("et1macaddr"))) {
			strcpy(mac, nvram_safe_get("et0macaddr"));
			inc_mac(mac, 1);
			dirty |= check_nv("et1macaddr", mac);
		}
		dirty |= check_nv("wl0gpio0", "0x88");
		break;
	case MODEL_WL500GP:
		dirty |= check_nv("sdram_init", "0x0009");	// 32MB; defaults: 0x000b, 0x0009
		dirty |= check_nv("wl0gpio0", "136");
		break;
	case MODEL_WL500GPv2:
	case MODEL_WL520GU:
		dirty |= check_nv("wl0gpio1", "136");
		break;
	case MODEL_WL500GD:
		dirty |= check_nv("vlan0hwname", "et0");
		dirty |= check_nv("vlan1hwname", "et0");
		dirty |= check_nv("boardflags", "0x00000100"); // set BFL_ENETVLAN
		nvram_unset("wl0gpio0");
		break;
	case MODEL_DIR320:
		if (strlen(nvram_safe_get("et0macaddr")) == 12 ||
		    strlen(nvram_safe_get("il0macaddr")) == 12) {
			dirty |= find_dir320_mac_addr();
		}
		if (nvram_get("vlan2hwname") != NULL) {
			nvram_unset("vlan2hwname");
			dirty = 1;
		}
		dirty |= check_nv("wandevs", "vlan1");
		dirty |= check_nv("vlan1hwname", "et0");
		dirty |= check_nv("wl0gpio0", "8");
		dirty |= check_nv("wl0gpio1", "0");
		dirty |= check_nv("wl0gpio2", "0");
		dirty |= check_nv("wl0gpio3", "0");
	case MODEL_WL1600GL:
		if (invalid_mac(nvram_get("et0macaddr"))) {
			dirty |= find_sercom_mac_addr();
		}
		break;
	case MODEL_WRT160Nv1:
	case MODEL_WRT310Nv1:
	case MODEL_WRT300N:
		dirty |= check_nv("wl0gpio0", "8");
		break;
#ifdef CONFIG_BCMWL5
	case MODEL_WNR3500L:
		dirty |= check_nv("boardflags", "0x00000710"); // needed to enable USB
		dirty |= check_nv("vlan2hwname", "et0");
		dirty |= check_nv("ledbh0", "7");
		break;
	case MODEL_WNR2000v2:
		dirty |= check_nv("ledbh5", "8");
		break;
	case MODEL_WRT320N:
		dirty |= check_nv("reset_gpio", "5");
		dirty |= check_nv("ledbh0", "136");
		dirty |= check_nv("ledbh1", "11");
		/* fall through, same as RT-N16 */
	case MODEL_RTN16:
		dirty |= check_nv("vlan2hwname", "et0");
		break;
	case MODEL_WRT610Nv2:
		dirty |= check_nv("vlan2hwname", "et0");
		dirty |= check_nv("pci/1/1/ledbh2", "8");
		dirty |= check_nv("sb/1/ledbh1", "8");
		if (invalid_mac(nvram_get("pci/1/1/macaddr"))) {
			strcpy(mac, nvram_safe_get("et0macaddr"));
			inc_mac(mac, 3);
			dirty |= check_nv("pci/1/1/macaddr", mac);
		}
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		if (nvram_match("sb/1/macaddr", nvram_safe_get("et0macaddr"))) {
			strcpy(mac, nvram_safe_get("et0macaddr"));
			inc_mac(mac, 2);
			dirty |= check_nv("sb/1/macaddr", mac);
			inc_mac(mac, 1);
			dirty |= check_nv("pci/1/1/macaddr", mac);
		}
	case MODEL_E4200:
		dirty |= check_nv("vlan2hwname", "et0");
		if (invalid_mac(nvram_get("pci/1/1/macaddr")) == 0 ||
		    invalid_mac(nvram_get("sb/1/macaddr"))) {
			strcpy(mac, nvram_safe_get("et0macaddr"));
			inc_mac(mac, 2);
			dirty |= check_nv("sb/1/macaddr", mac);
			inc_mac(mac, 1);
			dirty |= check_nv("pci/1/1/macaddr", mac);
		}
		break;
	case MODEL_WRT160Nv3:
		dirty |= check_nv("vlan2hwname", "et0");
		break;
#endif

	case MODEL_WRT54G:
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
			_dprintf("Unable to find critical settings, erasing NVRAM\n");
			mtd_erase("nvram");
			goto REBOOT;
	}

	dirty |= check_nv("aa0", "3");
	dirty |= check_nv("wl0gpio0", "136");
	dirty |= check_nv("wl0gpio2", "0");
	dirty |= check_nv("wl0gpio3", "0");
	dirty |= check_nv("cctl", "0");
	dirty |= check_nv("ccode", "0");

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
		//dirty |= check_nv("pa0maxpwr", "0x4e");
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
		//dirty |= check_nv("pa0maxpwr", "0x4e");
		break;
	case HW_BCM5354G:
		dirty |= check_nv("pa0itssit", "62");
		dirty |= check_nv("pa0b0", "0x1326");
		dirty |= check_nv("pa0b1", "0xFB51");
		dirty |= check_nv("pa0b2", "0xFE87");
		//dirty |= check_nv("pa0maxpwr", "0x4e");
		break;
	case HW_BCM4704_BCM5325F:
		// nothing to do
		break;
	default:
		dirty |= check_nv("pa0itssit", "62");
		dirty |= check_nv("pa0b0", "0x170c");
		dirty |= check_nv("pa0b1", "0xfa24");
		dirty |= check_nv("pa0b2", "0xfe70");
		//dirty |= check_nv("pa0maxpwr", "0x48");
		break;
	}
	break;

	} // switch (model)

	dirty |= init_vlan_ports();

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
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("gpio2", "ses_button");
			nvram_set("reset_gpio", "7");
		}
		nvram_set("pa0itssit", "62");
		nvram_set("pa0b0", "0x1542");
		nvram_set("pa0b1", "0xfacb");
		nvram_set("pa0b2", "0xfec7");
		//nvram_set("pa0maxpwr", "0x4c");
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
#ifdef TCONFIG_USB
		nvram_set("usb_ohci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1 eth2 eth3");	// set to "vlan0 eth2" by DD-WRT; default: vlan0 eth1
		}
		break;
	case MODEL_WL500W:
		mfr = "Asus";
		name = "WL-500W";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_ohci", "-1");
#endif
		break;
	case MODEL_WL500GE:
		mfr = "Asus";
		name = "WL-550gE";
		//	features = ?
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
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
	case MODEL_WLA2G54L:
		mfr = "Buffalo";
		name = "WLA2-G54L";
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wan_ifname", "none");
		}
		break;
	case MODEL_TM2300:
		mfr = "Dell";
		name = "TrueMobile 2300";
		break;
	
	/*
	
	  ...
	
	*/

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
#ifdef CONFIG_BCMWL5
	case MODEL_RTN10:
		mfr = "Asus";
		name = "RT-N10";
		features = SUP_SES | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_RTN12:
		mfr = "Asus";
		name = "RT-N12";
		features = SUP_SES | SUP_BRAU | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_RTN16:
		mfr = "Asus";
		name = "RT-N16";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("vlan_enable", "1");
		}
		break;
	case MODEL_RTN66U:
		mfr = "Asus";
		name = "RT-N66U";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#if defined(LINUX26) && defined(TCONFIG_USB_EXTRAS)
		if (nvram_get_int("usb_mmc") == -1) nvram_set("usb_mmc", "0");
#endif
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wandevs", "vlan2");
#if defined(LINUX26) && defined(TCONFIG_USB)
			nvram_set("usb_noled", "1-1.4"); /* SD/MMC Card */
#endif
		}
		break;
	case MODEL_WNR3500L:
		mfr = "Netgear";
		name = "WNR3500L/U/v2";
		features = SUP_SES | SUP_AOSS_LED | SUP_80211N | SUP_1000ET;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("sromrev", "3");
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_WNR2000v2:
		mfr = "Netgear";
		name = "WNR2000 v2";
		features = SUP_SES | SUP_AOSS_LED | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_F7D3301:
	case MODEL_F7D3302:
	case MODEL_F7D4301:
	case MODEL_F7D4302:
	case MODEL_F5D8235v3:
		mfr = "Belkin";
		features = SUP_SES | SUP_80211N;
		switch (model) {
		case MODEL_F7D3301:
			name = "Share Max N300 (F7D3301/F7D7301) v1";
			break;
		case MODEL_F7D3302:
			name = "Share N300 (F7D3302/F7D7302) v1";
			break;
		case MODEL_F7D4301:
			name = "Play Max / N600 HD (F7D4301/F7D8301) v1";
			break;
		case MODEL_F7D4302:
			name = "Play N600 (F7D4302/F7D8302) v1";
			break;
		case MODEL_F5D8235v3:
			name = "N F5D8235-4 v3";
			break;
		}
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wandevs", "vlan2");
		}
		break;
	case MODEL_WRT160Nv3:
		// same as M10, M20, WRT310Nv2, E1000v1
		mfr = "Linksys";
		name = nvram_safe_get("boot_hw_model");
		if (strcmp(name, "E100") == 0)
			name = "E1000 v1";
		features = SUP_SES | SUP_80211N | SUP_WHAM_LED;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_WRT320N:
		mfr = "Linksys";
		name = nvram_match("boardrev", "0x1307") ? "E2000" : "WRT320N";
		features = SUP_SES | SUP_80211N | SUP_WHAM_LED | SUP_1000ET;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_WRT610Nv2:
		mfr = "Linksys";
		name = nvram_match("boot_hw_model", "E300") ? "E3000" : "WRT610N v2";
		features = SUP_SES | SUP_80211N | SUP_WHAM_LED | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_E4200:
		mfr = "Linksys";
		name = "E4200 v1";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
#endif	// CONFIG_BCMWL5
	case MODEL_WL500GPv2:
		mfr = "Asus";
		name = "WL-500gP v2";
		features = SUP_SES;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		break;
	case MODEL_WL520GU:
		mfr = "Asus";
		name = "WL-520GU";
		features = SUP_SES;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		break;
	case MODEL_WL500GD:
		mfr = "Asus";
		name = "WL-500g Deluxe";
		// features = SUP_SES;
#ifdef TCONFIG_USB
		nvram_set("usb_ohci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("wl_ifname", "eth1");
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_unset("wl0gpio0");
		}
		break;
	case MODEL_DIR320:
		mfr = "D-Link";
		name = "DIR-320";
		features = SUP_SES;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_H618B:
		mfr = "ZTE";
		name = "ZXV10 H618B";
		features = SUP_SES | SUP_AOSS_LED;
		break;
	case MODEL_WL1600GL:
		mfr = "Ovislink";
		name = "WL1600GL";
		features = SUP_SES;
		break;
#endif	// WL_BSS_INFO_VERSION >= 108
	case MODEL_WZRG300N:
		mfr = "Buffalo";
		name = "WZR-G300N";
		features = SUP_SES | SUP_AOSS_LED | SUP_BRAU | SUP_80211N;
		break;
	case MODEL_WRT160Nv1:
	case MODEL_WRT300N:
		mfr = "Linksys";
		name = (model == MODEL_WRT300N) ? "WRT300N v1" : "WRT160N v1";
		features = SUP_SES | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("wan_ifnameX", "eth1");
			nvram_set("lan_ifnames", "eth0 eth2");
		}
		break;
	case MODEL_WRT310Nv1:
		mfr = "Linksys";
		name = "WRT310N v1";
		features = SUP_SES | SUP_80211N | SUP_WHAM_LED | SUP_1000ET;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	}

	if (name) {
		nvram_set("t_fix1", name);
		sprintf(s, "%s %s", mfr, name);
	}
	else {
		snprintf(s, sizeof(s), "%s %d/%s/%s/%s/%s", mfr, check_hw_type(),
			nvram_safe_get("boardtype"), nvram_safe_get("boardnum"), nvram_safe_get("boardrev"), nvram_safe_get("boardflags"));
		s[64] = 0;
	}
	nvram_set("t_model_name", s);

	nvram_set("pa0maxpwr", "400");	// allow Tx power up tp 400 mW, needed for ND only

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

	nvram_set("wl_hwaddr", "");	// zzz- when disabling wireless, we must get null wireless mac ??

	//!!TB - do not force country code here to allow nvram override
	//nvram_set("wl_country", "JP");
	//nvram_set("wl_country_code", "JP");
	nvram_set("wan_get_dns", "");
	nvram_set("wan_get_domain", "");
	nvram_set("ppp_get_ip", "");
	nvram_set("action_service", "");
	nvram_set("jffs2_format", "0");
	nvram_set("rrules_radio", "-1");
	nvram_unset("https_crt_gen");
	nvram_unset("log_wmclear");
#ifdef TCONFIG_IPV6
	nvram_set("ipv6_get_dns", "");
#endif
#ifdef TCONFIG_MEDIA_SERVER
	nvram_unset("ms_rescan");
#endif
	if (nvram_get_int("http_id_gen") == 1) nvram_unset("http_id");

	nvram_unset("sch_rboot_last");
	nvram_unset("sch_rcon_last");
	nvram_unset("sch_c1_last");
	nvram_unset("sch_c2_last");
	nvram_unset("sch_c3_last");

	nvram_set("brau_state", "");
	if ((features & SUP_BRAU) == 0) nvram_set("script_brau", "");
	if ((features & SUP_SES) == 0) nvram_set("sesx_script", "");

	if ((features & SUP_1000ET) == 0) nvram_set("jumbo_frame_enable", "0");

	// compatibility with old versions
	if (nvram_match("wl_net_mode", "disabled")) {
		nvram_set("wl_radio", "0");
		nvram_set("wl_net_mode", "mixed");
	}

	return 0;
}

/* Get the special files from nvram and copy them to disc.
 * These were files saved with "nvram setfile2nvram <filename>".
 * Better hope that they were saved with full pathname.
 */
static void load_files_from_nvram(void)
{
	char *name, *cp;
	int ar_loaded = 0;
	char buf[NVRAM_SPACE];

	if (nvram_getall(buf, sizeof(buf)) != 0)
		return;

	for (name = buf; *name; name += strlen(name) + 1) {
		if (strncmp(name, "FILE:", 5) == 0) { /* This special name marks a file to get. */
			if ((cp = strchr(name, '=')) == NULL)
				continue;
			*cp = 0;
			syslog(LOG_INFO, "Loading file '%s' from nvram", name + 5);
			nvram_nvram2file(name, name + 5);
			if (memcmp(".autorun", cp - 8, 9) == 0) 
				++ar_loaded;
		}
	}
	/* Start any autorun files that may have been loaded into one of the standard places. */
	if (ar_loaded != 0)
		run_nvscript(".autorun", NULL, 3);
}

#if defined(LINUX26) && defined(TCONFIG_USB)
static inline void tune_min_free_kbytes(void)
{
	struct sysinfo info;

	memset(&info, 0, sizeof(struct sysinfo));
	sysinfo(&info);
	if (info.totalram >= 55 * 1024 * 1024) {
		// If we have 64MB+ RAM, tune min_free_kbytes
		// to reduce page allocation failure errors.
		f_write_string("/proc/sys/vm/min_free_kbytes", "8192", 0, 0);
	}
}
#endif

static void sysinit(void)
{
	static int noconsole = 0;
	static const time_t tm = 0;
	int hardware;
	int i;
	DIR *d;
	struct dirent *de;
	char s[256];
	char t[256];
	int model;

	mount("proc", "/proc", "proc", 0, NULL);
	mount("tmpfs", "/tmp", "tmpfs", 0, NULL);

#ifdef LINUX26
	mount("devfs", "/dev", "tmpfs", MS_MGC_VAL | MS_NOATIME, NULL);
	mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3));
	mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1));
	mount("sysfs", "/sys", "sysfs", MS_MGC_VAL, NULL);
	mkdir("/dev/shm", 0777);
	mkdir("/dev/pts", 0777);
	mknod("/dev/pts/ptmx", S_IRWXU|S_IFCHR, makedev(5, 2));
	mknod("/dev/pts/0", S_IRWXU|S_IFCHR, makedev(136, 0));
	mknod("/dev/pts/1", S_IRWXU|S_IFCHR, makedev(136, 1));
	mount("devpts", "/dev/pts", "devpts", MS_MGC_VAL, NULL);
#endif

	if (console_init()) noconsole = 1;

	stime(&tm);

	static const char *mkd[] = {
		"/tmp/etc", "/tmp/var", "/tmp/home", "/tmp/mnt",
		"/tmp/splashd", //!!Victek
		"/tmp/share", "/var/webmon", // !!TB
		"/var/log", "/var/run", "/var/tmp", "/var/lib", "/var/lib/misc",
		"/var/spool", "/var/spool/cron", "/var/spool/cron/crontabs",
		"/tmp/var/wwwext", "/tmp/var/wwwext/cgi-bin",	// !!TB - CGI support
		NULL
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
	f_write("/etc/fstab", NULL, 0, 0, 0644);			// !!TB - blank
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

#ifdef TCONFIG_SAMBASRV
	if ((d = opendir("/usr/codepages")) != NULL) {
		while ((de = readdir(d)) != NULL) {
			if (de->d_name[0] == '.') continue;
			snprintf(s, sizeof(s), "/usr/codepages/%s", de->d_name);
			snprintf(t, sizeof(t), "/usr/share/%s", de->d_name);
			symlink(s, t);
		}
		closedir(d);
	}
#endif

#ifdef LINUX26
	eval("hotplug2", "--coldplug");
	start_hotplug2();

	static const char *dn[] = {
		"null", "zero", "random", "urandom", "full", "ptmx", "nvram",
		NULL
	};
	for (i = 0; dn[i]; ++i) {
		snprintf(s, sizeof(s), "/dev/%s", dn[i]);
		chmod(s, 0666);
	}
	chmod("/dev/gpio", 0660);
#endif

	set_action(ACT_IDLE);

	for (i = 0; defenv[i]; ++i) {
		putenv(defenv[i]);
	}

	if (!noconsole) {
		printf("\n\nHit ENTER for console...\n\n");
		run_shell(1, 0);
	}

	check_bootnv();

#ifdef TCONFIG_IPV6
	// disable IPv6 by default on all interfaces
	f_write_string("/proc/sys/net/ipv6/conf/default/disable_ipv6", "1", 0, 0);
#endif

	for (i = 0; i < sizeof(fatalsigs) / sizeof(fatalsigs[0]); i++) {
		signal(fatalsigs[i], handle_fatalsigs);
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

#ifdef CONFIG_BCMWL5
	// ctf must be loaded prior to any other modules
	if (nvram_invmatch("ctf_disable", "1"))
		modprobe("ctf");
#endif

#ifdef TCONFIG_EMF
	modprobe("emf");
	modprobe("igs");
#endif

	switch (hardware = check_hw_type()) {
	case HW_BCM4785:
		modprobe("bcm57xx");
		break;
	default:
		modprobe("et");
		break;
	}

	modprobe("wl");

	config_loopback();

	eval("nvram", "defaults", "--initcheck");
	init_nvram();

	// set the packet size
	if (nvram_get_int("jumbo_frame_enable")) {
		// only set the size here - 'enable' flag is set by the driver
		// eval("et", "robowr", "0x40", "0x01", "0x1F"); // (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)
		eval("et", "robowr", "0x40", "0x05", nvram_safe_get("jumbo_frame_size"));
	}

	klogctl(8, NULL, nvram_get_int("console_loglevel"));

#if defined(LINUX26) && defined(TCONFIG_USB)
	tune_min_free_kbytes();
#endif
	setup_conntrack();
	set_host_domain_name();

	set_tz();

	eval("buttons");

	if (!noconsole) xstart("console");

	i = nvram_get_int("sesx_led");
	led(LED_AMBER, (i & 1) != 0);
	led(LED_WHITE, (i & 2) != 0);
	led(LED_AOSS, (i & 4) != 0);
	led(LED_BRIDGE, (i & 8) != 0);
	led(LED_DIAG, 1);
}

int init_main(int argc, char *argv[])
{
	int state, i;
	sigset_t sigset;

	sysinit();

	sigemptyset(&sigset);
	for (i = 0; i < sizeof(initsigs) / sizeof(initsigs[0]); i++) {
		sigaddset(&sigset, initsigs[i]);
	}
	sigprocmask(SIG_BLOCK, &sigset, NULL);

#if defined(DEBUG_NOISY)
	nvram_set("debug_logeval", "1");
	nvram_set("debug_cprintf", "1");
	nvram_set("debug_cprintf_file", "1");
	nvram_set("debug_ddns", "1");
#endif

	start_jffs2();

	state = SIGUSR2;	/* START */

	for (;;) {
		TRACE_PT("main loop signal/state=%d\n", state);

		switch (state) {
		case SIGUSR1:		/* USER1: service handler */
			exec_service();
			break;

		case SIGHUP:		/* RESTART */
		case SIGINT:		/* STOP */
		case SIGQUIT:		/* HALT */
		case SIGTERM:		/* REBOOT */
			led(LED_DIAG, 1);
			unlink("/var/notice/sysup");

			run_nvscript("script_shut", NULL, 10);

			stop_services();
			stop_wan();
			stop_lan();
			stop_vlan();
			stop_syslog();

			if ((state == SIGTERM /* REBOOT */) ||
			    (state == SIGQUIT /* HALT */)) {
				remove_storage_main(1);
				stop_usb();

				shutdn(state == SIGTERM /* REBOOT */);
				exit(0);
			}
			if (state == SIGINT /* STOP */) {
				break;
			}

			// SIGHUP (RESTART) falls through

		case SIGUSR2:		/* START */
			SET_LED(RELEASE_WAN_CONTROL);
			start_syslog();

			load_files_from_nvram();

			int fd = -1;
			fd = file_lock("usb");	// hold off automount processing
			start_usb();

			run_nvscript("script_init", NULL, 2);

			file_unlock(fd);	// allow to process usb hotplug events
#ifdef TCONFIG_USB
			/*
			 * On RESTART some partitions can stay mounted if they are busy at the moment.
			 * In that case USB drivers won't unload, and hotplug won't kick off again to
			 * remount those drives that actually got unmounted. Make sure to remount ALL
			 * partitions here by simulating hotplug event.
			 */
			if (state == SIGHUP /* RESTART */)
				add_remove_usbhost("-1", 1);
#endif

			create_passwd();
			start_vlan();
			start_lan();
			start_arpbind();
			start_wan(BOOT);
			start_services();
			start_wl();

#ifdef CONFIG_BCMWL5
			if (wds_enable()) {
				/* Restart NAS one more time - for some reason without
				 * this the new driver doesn't always bring WDS up.
				 */
				stop_nas();
				start_nas();
			}
#else
#ifdef TCONFIG_VLAN
			if (wl_security_on()) {
				stop_nas();
				start_nas();
			}
#endif /* TCONFIG_VLAN */
#endif

			syslog(LOG_INFO, "%s: Tomato %s", nvram_safe_get("t_model_name"), tomato_version);

			led(LED_DIAG, 0);
			notice_set("sysup", "");
			break;
		}

		chld_reap(0);		/* Periodically reap zombies. */
		check_services();
		sigwait(&sigset, &state);
	}

	return 0;
}

int reboothalt_main(int argc, char *argv[])
{
	int reboot = (strstr(argv[0], "reboot") != NULL);
	puts(reboot ? "Rebooting..." : "Shutting down...");
	fflush(stdout);
	sleep(1);
	kill(1, reboot ? SIGTERM : SIGQUIT);

	/* In the case we're hung, we'll get stuck and never actually reboot.
	 * The only way out is to pull power.
	 * So after 'reset_wait' seconds (default: 20), forcibly crash & restart.
	 */
	if (fork() == 0) {
		int wait = nvram_get_int("reset_wait") ? : 20;
		if ((wait < 10) || (wait > 120)) wait = 10;

		f_write("/proc/sysrq-trigger", "s", 1, 0 , 0); /* sync disks */
		sleep(wait);
		puts("Still running... Doing machine reset.");
		fflush(stdout);
		f_write("/proc/sysrq-trigger", "s", 1, 0 , 0); /* sync disks */
		sleep(1);
		f_write("/proc/sysrq-trigger", "b", 1, 0 , 0); /* machine reset */
	}

	return 0;
}

