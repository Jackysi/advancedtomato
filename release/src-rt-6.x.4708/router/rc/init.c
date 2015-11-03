/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/

#include "rc.h"
#include "shared.h"

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

extern struct nvram_tuple router_defaults[];

void
restore_defaults_module(char *prefix)
{
	struct nvram_tuple *t;

	for (t = router_defaults; t->name; t++) {
		if(strncmp(t->name, prefix, sizeof(prefix))!=0) continue;
		nvram_set(t->name, t->value);
	}
}
#ifdef TCONFIG_BCM7
extern struct nvram_tuple bcm4360ac_defaults[];

static void set_bcm4360ac_vars(void)
{
	struct nvram_tuple *t;

	/* Restore defaults */
	dbg("Restoring bcm4360ac vars...\n");
	for (t = bcm4360ac_defaults; t->name; t++) {
		if (!nvram_get(t->name))
			nvram_set(t->name, t->value);
	}
}

void
bsd_defaults(void)
{
	char extendno_org[14];
	int ext_num;
	char ext_commit_str[8];
	struct nvram_tuple *t;

	dbg("Restoring bsd settings...\n");

	if (!strlen(nvram_safe_get("extendno_org")) ||
	    nvram_match("extendno_org", nvram_safe_get("extendno")))
		return;

	strcpy(extendno_org, nvram_safe_get("extendno_org"));
	if (!strlen(extendno_org) ||
	    sscanf(extendno_org, "%d-g%s", &ext_num, ext_commit_str) != 2)
		return;


	for (t = router_defaults; t->name; t++)
		if (strstr(t->name, "bsd"))
			nvram_set(t->name, t->value);
}

/* assign none-exist value */
void
wl_defaults(void)
{
	struct nvram_tuple *t;
	char prefix[]="wlXXXXXX_", tmp[100], tmp2[100];
	char pprefix[]="wlXXXXXX_", *pssid;
	char word[256], *next;
	int unit, subunit;
	char wlx_vifnames[64], wl_vifnames[64], lan_ifnames[128];
	int subunit_x = 0;
	unsigned int max_mssid;

	memset(wlx_vifnames, 0, sizeof(wlx_vifnames));
	memset(wl_vifnames, 0, sizeof(wl_vifnames));
	memset(lan_ifnames, 0, sizeof(lan_ifnames));

	dbg("Restoring wireless vars ...\n");

	if (!nvram_get("wl_country_code"))
		nvram_set("wl_country_code", "");

	unit = 0;
	foreach (word, nvram_safe_get("wl_ifnames"), next) {
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);

	dbg("Restoring wireless vars - in progress ...\n");

		for (t = router_defaults; t->name; t++) {
#ifdef CONFIG_BCMWL5
			if (!strncmp(t->name, "wl", 2) && strncmp(t->name, "wl_", 3) && strncmp(t->name, "wlc", 3) && !strcmp(&t->name[4], "nband"))
				nvram_set(t->name, t->value);
#endif
			if (strncmp(t->name, "wl_", 3)!=0) continue;
#ifdef CONFIG_BCMWL5
			if (!strcmp(&t->name[3], "nband") && nvram_match(strcat_r(prefix, &t->name[3], tmp), "-1"))
				nvram_set(strcat_r(prefix, &t->name[3], tmp), t->value);
#endif
			if (!nvram_get(strcat_r(prefix, &t->name[3], tmp))) {
				/* Add special default value handle here */
#ifdef TCONFIG_EMF
				/* Wireless IGMP Snooping */
				if (strncmp(&t->name[3], "igs", sizeof("igs")) == 0) {
					char *value = nvram_get(strcat_r(prefix, "wmf_bss_enable", tmp2));
					nvram_set(tmp, (value && *value) ? value : t->value);
				} else
#endif
					nvram_set(tmp, t->value);
			}
		}

		unit++;
	}
	dbg("Restoring wireless vars - done ...\n");
}

#endif


static void
restore_defaults(void)
{
	struct nvram_tuple *t;
	int restore_defaults;
	char prefix[] = "usb_pathXXXXXXXXXXXXXXXXX_", tmp[100];
	int unit;
	int model;

	/* Restore defaults if told to or OS has changed */
	if(!restore_defaults)
		restore_defaults = !nvram_match("restore_defaults", "0");

	if (restore_defaults)
		fprintf(stderr, "\n## Restoring defaults... ##\n");

	/* Restore defaults */
	for (t = router_defaults; t->name; t++) {
		if (restore_defaults || !nvram_get(t->name)) {
			nvram_set(t->name, t->value);
		}
	}

	nvram_set("os_name", "linux");
	nvram_set("os_version", tomato_version);
	nvram_set("os_date", tomato_buildtime);
}


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
	case MODEL_WL330GE:
		if (nvram_match("vlan1ports", "0 5u"))	// 520GU or 330GE or WL500GE?
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
		dirty |= check_nv("vlan0ports", "1 2 3 4 5*");
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
	case MODEL_WNR3500LV2:
	case MODEL_RTN16:
	case MODEL_RTN66U:
		dirty |= check_nv("vlan1ports", "4 3 2 1 8*");
		dirty |= check_nv("vlan2ports", "0 8");
		break;
	case MODEL_W1800R:
		dirty |= check_nv("vlan1ports", "1 2 3 4 8*");
		dirty |= check_nv("vlan2ports", "0 8");
		dirty |= check_nv("boot_wait", "on");
		dirty |= check_nv("wait_time", "5");
		break;
	case MODEL_D1800H:
		dirty |= check_nv("vlan1ports", "1 2 3 4 8*");
		dirty |= check_nv("vlan2ports", "0 8");
		dirty |= check_nv("ledbh0", "11");
		dirty |= check_nv("ledbh1", "11");
		dirty |= check_nv("ledbh2", "11");
		dirty |= check_nv("ledbh11", "136");
		// must flash tt through tftp.
		dirty |= check_nv("boot_wait", "on");
		dirty |= check_nv("wait_time", "5");
		break;
	case MODEL_RTN53:
		dirty |= check_nv("vlan2ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan1ports", "4 5");
		break;
	case MODEL_RTN53A1:
		dirty |= check_nv("vlan1ports", "4 5");
		dirty |= check_nv("vlan2ports", "3 2 1 0 5*");
		break;
	case MODEL_WNR2000v2:
		dirty |= check_nv("vlan1ports", "4 3 2 1 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_HG320:
	case MODEL_H218N:
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_RG200E_CA:
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_RTN10:
		dirty |= check_nv("vlan1ports", "4 5");
		break;
	case MODEL_RTN10U:
	case MODEL_CW5358U:
		dirty |= check_nv("vlan0ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan1ports", "0 5");
		break;
	case MODEL_RTN10P:
	case MODEL_RTN12:
	case MODEL_RTN12B1:
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
	case MODEL_E900:
	case MODEL_E1500:
	case MODEL_E1550:
	case MODEL_E2500:
	case MODEL_F7D3302:
	case MODEL_F7D4302:
	case MODEL_DIR620C1:
		dirty |= check_nv("vlan1ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_E1000v2:
	case MODEL_L600N:
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_RTN15U:
	case MODEL_E3200:
	case MODEL_E4200:
		dirty |= check_nv("vlan1ports", "0 1 2 3 8*");
		dirty |= check_nv("vlan2ports", "4 8");
		break;
	case MODEL_TDN60:
		dirty |= check_nv("vlan1ports", "4 3 2 1 8*");
		dirty |= check_nv("vlan2ports", "4 8");
		dirty |= check_nv("boot_wait", "on");
		dirty |= check_nv("wait_time", "5");
		break;
	case MODEL_TDN6: //bwq518
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		dirty |= check_nv("boot_wait", "on");
		dirty |= check_nv("wait_time", "5");
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
#ifdef CONFIG_BCMWL6A
	case MODEL_R6250:
	case MODEL_R6300v2:
		dirty |= check_nv("vlan1ports", "3 2 1 0 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_RTAC56U:
	case MODEL_DIR868L:
		dirty |= check_nv("vlan1ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_R7000:
	case MODEL_RTN18U:
	case MODEL_RTAC68U:
	case MODEL_RTAC3200:
	case MODEL_WS880:
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_EA6700:
	case MODEL_WZR1750:
		dirty |= check_nv("vlan1ports", "0 1 2 3 5*");
		dirty |= check_nv("vlan2ports", "4 5");
		break;
	case MODEL_EA6900:
		dirty |= check_nv("vlan1ports", "1 2 3 4 5*");
		dirty |= check_nv("vlan2ports", "0 5");
		break;
	case MODEL_R1D:
		dirty |= check_nv("vlan1ports", "0 2 5*");
		dirty |= check_nv("vlan2ports", "4 5");
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
	case MODEL_WL330GE:
		dirty |= check_nv("wl0gpio1", "0x02");
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
		break;
	case MODEL_H618B:
		dirty |= check_nv("wandevs", "vlan1");
		dirty |= check_nv("vlan0hwname", "et0");
		dirty |= check_nv("vlan1hwname", "et0");
		break;
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
	case MODEL_WNR3500LV2:
		dirty |= check_nv("vlan2hwname", "et0");
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
	case MODEL_HG320:
	case MODEL_RG200E_CA:
	case MODEL_H218N:
		dirty |= check_nv("vlan1hwname", "et0");
		dirty |= check_nv("vlan2hwname", "et0");
		dirty |= check_nv("boardflags", "0x710"); // set BFL_ENETVLAN, enable VLAN
		dirty |= check_nv("reset_gpio", "30");
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
	case MODEL_EA6500V1:
		dirty |= check_nv("vlan2hwname", "et0");
		if (strncasecmp(nvram_safe_get("pci/2/1/macaddr"), "00:90:4c", 8) == 0) {
			strcpy(mac, nvram_safe_get("et0macaddr"));
			inc_mac(mac, 3);
			dirty |= check_nv("pci/2/1/macaddr", mac);
		}
		break;
	case MODEL_E4200:
		dirty |= check_nv("vlan2hwname", "et0");
		if (strncasecmp(nvram_safe_get("pci/1/1/macaddr"), "00:90:4c", 8) == 0 ||
		    strncasecmp(nvram_safe_get("sb/1/macaddr"), "00:90:4c", 8) == 0) {
			strcpy(mac, nvram_safe_get("et0macaddr"));
			inc_mac(mac, 2);
			dirty |= check_nv("sb/1/macaddr", mac);
			inc_mac(mac, 1);
			dirty |= check_nv("pci/1/1/macaddr", mac);
		}
		break;
	case MODEL_E900:
	case MODEL_E1000v2:
	case MODEL_E1500:
	case MODEL_E1550:
	case MODEL_E2500:
	case MODEL_E3200:
	case MODEL_WRT160Nv3:
	case MODEL_L600N:
	case MODEL_DIR620C1:
		dirty |= check_nv("vlan2hwname", "et0");
		break;
#endif
#ifdef CONFIG_BCMWL6
	case MODEL_R7000:
	case MODEL_R6250:
	case MODEL_R6300v2:
		nvram_unset("et1macaddr");
		dirty |= check_nv("wl0_ifname", "eth1");
		dirty |= check_nv("wl1_ifname", "eth2");
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
	const char *ver;
	char s[256];
	unsigned long bf;
	unsigned long n;
	
	model = get_model();
	sprintf(s, "%d", model);
	nvram_set("t_model", s);

	mfr = "Broadcom";
	name = NULL;
	ver = NULL;
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
			if ( nvram_is_empty("wlx_hpamp") || nvram_match("wlx_hpamp", "")) {
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
	case MODEL_L600N:
		mfr = "Rosewill";
		name = "L600N";
		features = SUP_SES | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
#ifdef TCONFIG_USBAP
			nvram_set("wl1_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("ehciirqt", "3");
			nvram_set("qtdc_pid", "48407");
			nvram_set("qtdc_vid", "2652");
			nvram_set("qtdc0_ep", "4");
			nvram_set("qtdc0_sz", "0");
			nvram_set("qtdc1_ep", "18");
			nvram_set("qtdc1_sz", "10");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wl0_ifname", "wl0");
			nvram_set("wl1_ifname", "wl1");
#else
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("landevs", "vlan1 wl0");
#endif
			nvram_set("wl_ifname", "eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
		}
		break;
	case MODEL_DIR620C1:
		mfr = "D-Link";
		name = "Dir-620 C1";
		features = SUP_SES | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
#ifdef TCONFIG_USBAP
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
#else
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("landevs", "vlan1 wl0");
#endif
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");

		}
		break;
	case MODEL_CW5358U:
		mfr = "Catchtech";
		name = "CW-5358U";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifname", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_HG320:
		mfr = "FiberHome";
		name = "HG320";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifname", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_RG200E_CA:
		mfr = "ChinaNet";
		name = "RG200E-CA";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifname", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_H218N:
		mfr = "ZTE";
		name = "H218N";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifname", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_TDN60:
		mfr = "Tenda";
		name = "N60";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","1l");
			nvram_set("wl1_bw_cap","7");
			nvram_set("wl1_chanspec","36/80");
			nvram_set("blink_5g_interface","eth2");
			//nvram_set("landevs", "vlan1 wl0 wl1");
			//nvram_set("wandevs", "vlan2");

			// fix WL mac`s
			nvram_set("wl0_hwaddr", nvram_safe_get("sb/1/macaddr"));
			nvram_set("wl1_hwaddr", nvram_safe_get("0:macaddr"));
		}
		break;
	case MODEL_TDN6:
		mfr = "Tenda";
		name = "N6";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","1l");
			nvram_set("wl1_bw_cap","7");
			nvram_set("wl1_chanspec","36/80");
			nvram_set("blink_5g_interface","eth2");

		// fix WL mac`s
		nvram_set("wl0_hwaddr", nvram_safe_get("sb/1/macaddr"));
		nvram_set("wl1_hwaddr", nvram_safe_get("0:macaddr"));
		}
		break;
	case MODEL_RTN10:
	case MODEL_RTN10P:
		mfr = "Asus";
		name = nvram_match("boardrev", "0x1153") ? "RT-N10P" : "RT-N10";
		features = SUP_SES | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_RTN10U:
		mfr = "Asus";
		name = "RT-N10U";
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
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
	case MODEL_RTN12B1:
		mfr = "Asus";
		name = "RT-N12 B1";
		features = SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_RTN15U:
		mfr = "Asus";
		name = "RT-N15U";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_iface", "vlan2");
			nvram_set("wan_ifname", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wan_ifnames", "vlan2");
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
	case MODEL_RTN53:
	case MODEL_RTN53A1:
		mfr = "Asus";
		name = nvram_match("boardrev", "0x1446") ? "RT-N53 A1" : "RT-N53";
		features = SUP_SES | SUP_80211N;
#if defined(LINUX26) && defined(TCONFIG_USBAP)
		if (nvram_get_int("usb_storage") == 1) nvram_set("usb_storage", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
#ifdef TCONFIG_USBAP
			nvram_set("wl1_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("ehciirqt", "3");
			nvram_set("qtdc_pid", "48407");
			nvram_set("qtdc_vid", "2652");
			nvram_set("qtdc0_ep", "4");
			nvram_set("qtdc0_sz", "0");
			nvram_set("qtdc1_ep", "18");
			nvram_set("qtdc1_sz", "10");
			nvram_set("lan_ifnames", "vlan2 eth1 eth2");
			nvram_set("landevs", "vlan2 wl0 wl1");
			nvram_set("wl1_ifname", "eth2");
#else
			nvram_set("lan_ifnames", "vlan2 eth1");
			nvram_set("landevs", "vlan2 wl0");
#endif
			nvram_set("lan_ifname", "br0");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wandevs", "vlan1");
			nvram_unset("vlan0ports");
		}
		break;
#ifdef CONFIG_BCMWL6A
	case MODEL_RTN18U:
		mfr = "Asus";
		name = "RT-N18U";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0");
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");

			// fix WL mac`s
			nvram_set("wl0_hwaddr", nvram_safe_get("0:macaddr"));

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");
		}
		break;
	case MODEL_RTAC56U:
		mfr = "Asus";
		name = "RT-AC56U";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");

			// fix WL mac`s
			nvram_set("wl0_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("wl1_hwaddr", nvram_safe_get("1:macaddr"));

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// force wl1 settings
			nvram_set("wl1_bw", "3");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "149/80");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
			nvram_set("wl_country", "SG");
			nvram_set("wl_country_code", "SG");

			nvram_set("1:ledbh6", "136");   // fixup 5 ghz led - from dd-wrt
			nvram_unset("1:ledbh10");       // fixup 5 ghz led - from dd-wrt
		}
		break;
	case MODEL_RTAC68U:
		mfr = "Asus";
		name = nvram_match("boardrev", "0x1103") ? "RT-AC68P" : "RT-AC68R/U";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");

			// fix WL mac`s
			nvram_set("wl0_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("wl1_hwaddr", nvram_safe_get("1:macaddr"));

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// force wl1 settings
			nvram_set("wl1_bw", "3");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "149/80");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("0:ccode", "SG");
			nvram_set("1:ccode", "SG");
			nvram_set("wl_country", "SG");
			nvram_set("wl_country_code", "SG");
		}
		break;
	case MODEL_RTAC3200:
		mfr = "Asus";
		name = "RT-AC3200";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1 wl2");
			nvram_set("lan_ifnames", "vlan1 eth2 eth1 eth3");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth2 eth1 eth3");
			nvram_set("wl_ifname", "eth2");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");
			nvram_set("wl2_ifname", "eth3");
			nvram_set("wl0_vifnames", "wl0.1 wl0.2 wl0.3");
			nvram_set("wl1_vifnames", "wl1.1 wl1.2 wl1.3");
			nvram_set("wl2_vifnames", "wl2.1 wl2.2 wl2.3");

			// fix WL mac`s
			nvram_set("wl0_hwaddr", nvram_safe_get("1:macaddr"));
			nvram_set("wl1_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("wl2_hwaddr", nvram_safe_get("2:macaddr"));

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// force wl settings
			// wl0 (1:) - 2,4GHz
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","6u");
			nvram_set("wl0_channel","6");
			nvram_set("wl0_nbw","40");
			nvram_set("wl0_nctrlsb", "upper");
			nvram_set("1:ccode", "SG");
			nvram_set("1:regrev", "0");
			// wl1 (0:) - 5GHz low
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "48l");
			nvram_set("wl1_channel", "48");
			nvram_set("wl1_nbw","80");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("0:ccode", "SG");
			nvram_set("0:regrev", "0");
			// wl2 (2:) - 5GHz high
			nvram_set("wl2_bw_cap", "7");
			nvram_set("wl2_chanspec", "100u");
			nvram_set("wl2_channel", "100");
			nvram_set("wl2_nbw","80");
			nvram_set("wl2_nctrlsb", "upper");
			nvram_set("2:ccode", "SG");
			nvram_set("2:regrev", "0");

			wl_defaults();
			bsd_defaults();
			set_bcm4360ac_vars();
		}
		break;
	case MODEL_R6250:
	case MODEL_R6300v2:
	case MODEL_R7000:
		mfr = "Netgear";
		if(nvram_match("board_id", "U12H245T00_NETGEAR")) //R6250
			name = "R6250";
		else
			name = model == MODEL_R7000 ? "R7000" : "R6300v2"; //R7000 or R6300v2

		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			if (get_model() == MODEL_R6300v2) {
				nvram_set("lan_invert", "1");
			}
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");

			//disable second *fake* LAN interface
			nvram_unset("et1macaddr");

			// fix WL mac for 2,4G
			nvram_set("pci/1/1/macaddr", nvram_safe_get("et0macaddr"));

			// fix WL mac for 5G
			strcpy(s, nvram_safe_get("pci/1/1/macaddr"));
			inc_mac(s, +1);
			nvram_set("pci/2/1/macaddr", s);

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// force wl1 settings
			nvram_set("wl1_bw", "3");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "149/80");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("wl_country", "SG");
			nvram_set("wl_country_code", "SG");
			nvram_set("blink_wl", "1");

			// bcm4360ac_defaults - fix problem of loading driver failed with code 21
			nvram_set("pci/1/1/aa2g", "7");
			nvram_set("pci/1/1/agbg0", "71");
			nvram_set("pci/1/1/agbg1", "71");
			nvram_set("pci/1/1/agbg2", "71");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags", "0x1000");
			nvram_set("pci/1/1/boardflags2", "0x100002");
			nvram_set("pci/1/1/boardflags3", "0x10000003");
			nvram_set("pci/1/1/boardnum", "57359");
			nvram_set("pci/1/1/boardrev", "0x1150");
			nvram_set("pci/1/1/boardtype", "0x661");
			nvram_set("pci/1/1/cckbw202gpo", "0");
			nvram_set("pci/1/1/cckbw20ul2gpo", "0");
			nvram_set("pci/1/1/ccode", "EU");
			nvram_set("pci/1/1/devid", "0x43a1");
			nvram_set("pci/1/1/dot11agduphrpo", "0");
			nvram_set("pci/1/1/dot11agduplrpo", "0");
			nvram_set("pci/1/1/dot11agofdmhrbw202gpo", "0xCA86");
			nvram_set("pci/1/1/epagain2g", "0");
			nvram_set("pci/1/1/femctrl", "3");
			nvram_set("pci/1/1/gainctrlsph", "0");
//			nvram_set("pci/1/1/macaddr", "E4:F4:C6:01:47:7C");
			nvram_set("pci/1/1/maxp2ga0", "106");
			nvram_set("pci/1/1/maxp2ga1", "106");
			nvram_set("pci/1/1/maxp2ga2", "106");
			nvram_set("pci/1/1/mcsbw202gpo", "0xA976A600");
			nvram_set("pci/1/1/mcsbw402gpo", "0xA976A600");
			nvram_set("pci/1/1/measpower", "0x7f");
			nvram_set("pci/1/1/measpower1", "0x7f");
			nvram_set("pci/1/1/measpower2", "0x7f");
			nvram_set("pci/1/1/noiselvl2ga0", "31");
			nvram_set("pci/1/1/noiselvl2ga1", "31");
			nvram_set("pci/1/1/noiselvl2ga2", "31");
			nvram_set("pci/1/1/ofdmlrbw202gpo", "0");
			nvram_set("pci/1/1/pa2ga0", "0xFF32,0x1C30,0xFCA3");
			nvram_set("pci/1/1/pa2ga1", "0xFF35,0x1BE3,0xFCB0");
			nvram_set("pci/1/1/pa2ga2", "0xFF33,0x1BE1,0xFCB0");
			nvram_set("pci/1/1/papdcap2g", "0");
			nvram_set("pci/1/1/pdgain2g", "14");
			nvram_set("pci/1/1/pdoffset2g40ma0", "15");
			nvram_set("pci/1/1/pdoffset2g40ma1", "15");
			nvram_set("pci/1/1/pdoffset2g40ma2", "15");
			nvram_set("pci/1/1/pdoffset2g40mvalid", "1");
			nvram_set("pci/1/1/pdoffset40ma0", "0");
			nvram_set("pci/1/1/pdoffset40ma1", "0");
			nvram_set("pci/1/1/pdoffset40ma2", "0");
			nvram_set("pci/1/1/pdoffset80ma0", "0");
			nvram_set("pci/1/1/pdoffset80ma1", "0");
			nvram_set("pci/1/1/pdoffset80ma2", "0");
			nvram_set("pci/1/1/regrev", "66");
			nvram_set("pci/1/1/rpcal2g", "0x5f7");
			nvram_set("pci/1/1/rxgainerr2ga0", "63");
			nvram_set("pci/1/1/rxgainerr2ga1", "31");
			nvram_set("pci/1/1/rxgainerr2ga2", "31");
			nvram_set("pci/1/1/rxgains2gelnagaina0", "3");
			nvram_set("pci/1/1/rxgains2gelnagaina1", "3");
			nvram_set("pci/1/1/rxgains2gelnagaina2", "3");
			nvram_set("pci/1/1/rxgains2gtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains2gtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains2gtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains2gtrisoa0", "7");
			nvram_set("pci/1/1/rxgains2gtrisoa1", "7");
			nvram_set("pci/1/1/rxgains2gtrisoa2", "7");
			nvram_set("pci/1/1/sar2g", "18");
			nvram_set("pci/1/1/sromrev", "11");
			nvram_set("pci/1/1/subband5gver", "0x4");
			nvram_set("pci/1/1/subvid", "0x14e4");
			nvram_set("pci/1/1/tssifloor2g", "0x3ff");
			nvram_set("pci/1/1/tssiposslope2g", "1");
			nvram_set("pci/1/1/tworangetssi2g", "0");
			nvram_set("pci/1/1/xtalfreq", "65535");
			nvram_set("pci/2/1/aa2g", "7");
			nvram_set("pci/2/1/aa5g", "0");
			nvram_set("pci/2/1/aga0", "0");
			nvram_set("pci/2/1/aga1", "0");
			nvram_set("pci/2/1/aga2", "0");
			nvram_set("pci/2/1/agbg0", "0");
			nvram_set("pci/2/1/agbg1", "0");
			nvram_set("pci/2/1/agbg2", "0");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x30000000");
			nvram_set("pci/2/1/boardflags2", "0x300002");
			nvram_set("pci/2/1/boardflags3", "0x10000000");
			nvram_set("pci/2/1/boardnum", "20507");
			nvram_set("pci/2/1/boardrev", "0x1451");
			nvram_set("pci/2/1/boardtype", "0x621");
			nvram_set("pci/2/1/cckbw202gpo", "0");
			nvram_set("pci/2/1/cckbw20ul2gpo", "0");
			nvram_set("pci/2/1/ccode", "SG");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/dot11agofdmhrbw202gpo", "0");
			nvram_set("pci/2/1/epagain2g", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/femctrl", "3");
			nvram_set("pci/2/1/gainctrlsph", "0");
//			nvram_set("pci/2/1/macaddr", "E4:F4:C6:01:47:7B");
			nvram_set("pci/2/1/maxp2ga0", "76");
			nvram_set("pci/2/1/maxp2ga1", "76");
			nvram_set("pci/2/1/maxp2ga2", "76");
			nvram_set("pci/2/1/maxp5ga0", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga1", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga2", "106,106,106,106");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcsbw1605hpo", "0");
			nvram_set("pci/2/1/mcsbw202gpo", "0");
			nvram_set("pci/2/1/mcsbw205ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw205glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw205gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw402gpo", "0");
			nvram_set("pci/2/1/mcsbw405ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl2ga0", "31");
			nvram_set("pci/2/1/noiselvl2ga1", "31");
			nvram_set("pci/2/1/noiselvl2ga2", "31");
			nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
			nvram_set("pci/2/1/ofdmlrbw202gpo", "0");
			nvram_set("pci/2/1/pa2ga0", "0xfe72,0x14c0,0xfac7");
			nvram_set("pci/2/1/pa2ga1", "0xfe80,0x1472,0xfabc");
			nvram_set("pci/2/1/pa2ga2", "0xfe82,0x14bf,0xfad9");
			nvram_set("pci/2/1/pa5ga0", "0xFF4C,0x1808,0xFD1B,0xFF4C,0x18CF,0xFD0C,0xFF4A,0x1920,0xFD08,0xFF4C,0x1949,0xFCF6");
			nvram_set("pci/2/1/pa5ga1", "0xFF4A,0x18AC,0xFD0B,0xFF44,0x1904,0xFCFF,0xFF56,0x1A09,0xFCFC,0xFF4F,0x19AB,0xFCEF");
			nvram_set("pci/2/1/pa5ga2", "0xFF4C,0x1896,0xFD11,0xFF43,0x192D,0xFCF5,0xFF50,0x19EE,0xFCF1,0xFF52,0x19C6,0xFCF1");
			nvram_set("pci/2/1/papdcap2g", "0");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pdgain2g", "4");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/pdoffset2g40ma0", "15");
			nvram_set("pci/2/1/pdoffset2g40ma1", "15");
			nvram_set("pci/2/1/pdoffset2g40ma2", "15");
			nvram_set("pci/2/1/pdoffset2g40mvalid", "1");
			nvram_set("pci/2/1/pdoffset40ma0", "4369");
			nvram_set("pci/2/1/pdoffset40ma1", "4369");
			nvram_set("pci/2/1/pdoffset40ma2", "4369");
			nvram_set("pci/2/1/pdoffset80ma0", "0");
			nvram_set("pci/2/1/pdoffset80ma1", "0");
			nvram_set("pci/2/1/pdoffset80ma2", "0");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
			nvram_set("pci/2/1/regrev", "66");
			nvram_set("pci/2/1/rpcal2g", "0");
			nvram_set("pci/2/1/rpcal5gb0", "0x610c");
			nvram_set("pci/2/1/rpcal5gb1", "0x6a09");
			nvram_set("pci/2/1/rpcal5gb2", "0x5eff");
			nvram_set("pci/2/1/rpcal5gb3", "0x700c");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr2ga0", "63");
			nvram_set("pci/2/1/rxgainerr2ga1", "31");
			nvram_set("pci/2/1/rxgainerr2ga2", "31");
			nvram_set("pci/2/1/rxgainerr5ga0", "63,63,63,63");
			nvram_set("pci/2/1/rxgainerr5ga1", "31,31,31,31");
			nvram_set("pci/2/1/rxgainerr5ga2", "31,31,31,31");
			nvram_set("pci/2/1/rxgains2gelnagaina0", "0");
			nvram_set("pci/2/1/rxgains2gelnagaina1", "0");
			nvram_set("pci/2/1/rxgains2gelnagaina2", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa0", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa1", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa2", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa0", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa1", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa2", "0");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/2/1/sar2g", "18");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sb20in40hrpo", "0");
			nvram_set("pci/2/1/sb20in40lrpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80hr5glpo", "0");
			nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80lr5glpo", "0");
			nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/subvid", "0x14e4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempthresh", "255");
			nvram_set("pci/2/1/tssifloor2g", "0x3ff");
			nvram_set("pci/2/1/tssifloor5g", "0x3ff,0x3ff,0x3ff,0x3ff");
			nvram_set("pci/2/1/tssiposslope2g", "1");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi2g", "0");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/xtalfreq", "65535");

		}
		break;
	case MODEL_DIR868L:
		mfr = "D-Link";
		name = "DIR868L";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");

			// fix WL mac for 5G
			strcpy(s, nvram_safe_get("pci/1/1/macaddr"));
			inc_mac(s, +1);
			nvram_set("pci/2/1/macaddr", s);

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// force wl1 settings
			nvram_set("wl1_bw", "3");
			nvram_set("wl1_bw_cap", "7");
			nvram_set("wl1_chanspec", "149/80");
			nvram_set("wl1_nctrlsb", "lower");
			nvram_set("wl_country", "SG");
			nvram_set("wl_country_code", "SG");

			// bcm4360ac_defaults - fix problem of loading driver failed with code 21
			nvram_set("pci/1/1/aa2g", "7");
			nvram_set("pci/1/1/agbg0", "71");
			nvram_set("pci/1/1/agbg1", "71");
			nvram_set("pci/1/1/agbg2", "71");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags", "0x1000");
			nvram_set("pci/1/1/boardflags2", "0x100002");
			nvram_set("pci/1/1/boardflags3", "0x10000003");
			nvram_set("pci/1/1/boardnum", "57359");
			nvram_set("pci/1/1/boardrev", "0x1150");
			nvram_set("pci/1/1/boardtype", "0x661");
			nvram_set("pci/1/1/cckbw202gpo", "0");
			nvram_set("pci/1/1/cckbw20ul2gpo", "0");
			nvram_set("pci/1/1/ccode", "EU");
			nvram_set("pci/1/1/devid", "0x43a1");
			nvram_set("pci/1/1/dot11agduphrpo", "0");
			nvram_set("pci/1/1/dot11agduplrpo", "0");
			nvram_set("pci/1/1/dot11agofdmhrbw202gpo", "0xCA86");
			nvram_set("pci/1/1/epagain2g", "0");
			nvram_set("pci/1/1/femctrl", "3");
			nvram_set("pci/1/1/gainctrlsph", "0");
//			nvram_set("pci/1/1/macaddr", "E4:F4:C6:01:47:7C");
			nvram_set("pci/1/1/maxp2ga0", "106");
			nvram_set("pci/1/1/maxp2ga1", "106");
			nvram_set("pci/1/1/maxp2ga2", "106");
			nvram_set("pci/1/1/mcsbw202gpo", "0xA976A600");
			nvram_set("pci/1/1/mcsbw402gpo", "0xA976A600");
			nvram_set("pci/1/1/measpower", "0x7f");
			nvram_set("pci/1/1/measpower1", "0x7f");
			nvram_set("pci/1/1/measpower2", "0x7f");
			nvram_set("pci/1/1/noiselvl2ga0", "31");
			nvram_set("pci/1/1/noiselvl2ga1", "31");
			nvram_set("pci/1/1/noiselvl2ga2", "31");
			nvram_set("pci/1/1/ofdmlrbw202gpo", "0");
			nvram_set("pci/1/1/pa2ga0", "0xFF32,0x1C30,0xFCA3");
			nvram_set("pci/1/1/pa2ga1", "0xFF35,0x1BE3,0xFCB0");
			nvram_set("pci/1/1/pa2ga2", "0xFF33,0x1BE1,0xFCB0");
			nvram_set("pci/1/1/papdcap2g", "0");
			nvram_set("pci/1/1/pdgain2g", "14");
			nvram_set("pci/1/1/pdoffset2g40ma0", "15");
			nvram_set("pci/1/1/pdoffset2g40ma1", "15");
			nvram_set("pci/1/1/pdoffset2g40ma2", "15");
			nvram_set("pci/1/1/pdoffset2g40mvalid", "1");
			nvram_set("pci/1/1/pdoffset40ma0", "0");
			nvram_set("pci/1/1/pdoffset40ma1", "0");
			nvram_set("pci/1/1/pdoffset40ma2", "0");
			nvram_set("pci/1/1/pdoffset80ma0", "0");
			nvram_set("pci/1/1/pdoffset80ma1", "0");
			nvram_set("pci/1/1/pdoffset80ma2", "0");
			nvram_set("pci/1/1/regrev", "66");
			nvram_set("pci/1/1/rpcal2g", "0x5f7");
			nvram_set("pci/1/1/rxgainerr2ga0", "63");
			nvram_set("pci/1/1/rxgainerr2ga1", "31");
			nvram_set("pci/1/1/rxgainerr2ga2", "31");
			nvram_set("pci/1/1/rxgains2gelnagaina0", "3");
			nvram_set("pci/1/1/rxgains2gelnagaina1", "3");
			nvram_set("pci/1/1/rxgains2gelnagaina2", "3");
			nvram_set("pci/1/1/rxgains2gtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains2gtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains2gtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains2gtrisoa0", "7");
			nvram_set("pci/1/1/rxgains2gtrisoa1", "7");
			nvram_set("pci/1/1/rxgains2gtrisoa2", "7");
			nvram_set("pci/1/1/sar2g", "18");
			nvram_set("pci/1/1/sromrev", "11");
			nvram_set("pci/1/1/subband5gver", "0x4");
			nvram_set("pci/1/1/subvid", "0x14e4");
			nvram_set("pci/1/1/tssifloor2g", "0x3ff");
			nvram_set("pci/1/1/tssiposslope2g", "1");
			nvram_set("pci/1/1/tworangetssi2g", "0");
			nvram_set("pci/1/1/xtalfreq", "65535");
			nvram_set("pci/2/1/aa2g", "7");
			nvram_set("pci/2/1/aa5g", "0");
			nvram_set("pci/2/1/aga0", "0");
			nvram_set("pci/2/1/aga1", "0");
			nvram_set("pci/2/1/aga2", "0");
			nvram_set("pci/2/1/agbg0", "0");
			nvram_set("pci/2/1/agbg1", "0");
			nvram_set("pci/2/1/agbg2", "0");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x30000000");
			nvram_set("pci/2/1/boardflags2", "0x300002");
			nvram_set("pci/2/1/boardflags3", "0x10000000");
			nvram_set("pci/2/1/boardnum", "20507");
			nvram_set("pci/2/1/boardrev", "0x1451");
			nvram_set("pci/2/1/boardtype", "0x621");
			nvram_set("pci/2/1/cckbw202gpo", "0");
			nvram_set("pci/2/1/cckbw20ul2gpo", "0");
			nvram_set("pci/2/1/ccode", "SG");
			nvram_set("pci/2/1/devid", "0x43a2");
			nvram_set("pci/2/1/dot11agduphrpo", "0");
			nvram_set("pci/2/1/dot11agduplrpo", "0");
			nvram_set("pci/2/1/dot11agofdmhrbw202gpo", "0");
			nvram_set("pci/2/1/epagain2g", "0");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/femctrl", "3");
			nvram_set("pci/2/1/gainctrlsph", "0");
//			nvram_set("pci/2/1/macaddr", "E4:F4:C6:01:47:7B");
			nvram_set("pci/2/1/maxp2ga0", "76");
			nvram_set("pci/2/1/maxp2ga1", "76");
			nvram_set("pci/2/1/maxp2ga2", "76");
			nvram_set("pci/2/1/maxp5ga0", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga1", "106,106,106,106");
			nvram_set("pci/2/1/maxp5ga2", "106,106,106,106");
			nvram_set("pci/2/1/mcsbw1605ghpo", "0");
			nvram_set("pci/2/1/mcsbw1605glpo", "0");
			nvram_set("pci/2/1/mcsbw1605gmpo", "0");
			nvram_set("pci/2/1/mcsbw1605hpo", "0");
			nvram_set("pci/2/1/mcsbw202gpo", "0");
			nvram_set("pci/2/1/mcsbw205ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw205glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw205gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw402gpo", "0");
			nvram_set("pci/2/1/mcsbw405ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw405gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805ghpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805glpo", "0xBA768600");
			nvram_set("pci/2/1/mcsbw805gmpo", "0xBA768600");
			nvram_set("pci/2/1/mcslr5ghpo", "0");
			nvram_set("pci/2/1/mcslr5glpo", "0");
			nvram_set("pci/2/1/mcslr5gmpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl2ga0", "31");
			nvram_set("pci/2/1/noiselvl2ga1", "31");
			nvram_set("pci/2/1/noiselvl2ga2", "31");
			nvram_set("pci/2/1/noiselvl5ga0", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga1", "31,31,31,31");
			nvram_set("pci/2/1/noiselvl5ga2", "31,31,31,31");
			nvram_set("pci/2/1/ofdmlrbw202gpo", "0");
			nvram_set("pci/2/1/pa2ga0", "0xfe72,0x14c0,0xfac7");
			nvram_set("pci/2/1/pa2ga1", "0xfe80,0x1472,0xfabc");
			nvram_set("pci/2/1/pa2ga2", "0xfe82,0x14bf,0xfad9");
			nvram_set("pci/2/1/pa5ga0", "0xFF4C,0x1808,0xFD1B,0xFF4C,0x18CF,0xFD0C,0xFF4A,0x1920,0xFD08,0xFF4C,0x1949,0xFCF6");
			nvram_set("pci/2/1/pa5ga1", "0xFF4A,0x18AC,0xFD0B,0xFF44,0x1904,0xFCFF,0xFF56,0x1A09,0xFCFC,0xFF4F,0x19AB,0xFCEF");
			nvram_set("pci/2/1/pa5ga2", "0xFF4C,0x1896,0xFD11,0xFF43,0x192D,0xFCF5,0xFF50,0x19EE,0xFCF1,0xFF52,0x19C6,0xFCF1");
			nvram_set("pci/2/1/papdcap2g", "0");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/pdgain2g", "4");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/pdoffset2g40ma0", "15");
			nvram_set("pci/2/1/pdoffset2g40ma1", "15");
			nvram_set("pci/2/1/pdoffset2g40ma2", "15");
			nvram_set("pci/2/1/pdoffset2g40mvalid", "1");
			nvram_set("pci/2/1/pdoffset40ma0", "4369");
			nvram_set("pci/2/1/pdoffset40ma1", "4369");
			nvram_set("pci/2/1/pdoffset40ma2", "4369");
			nvram_set("pci/2/1/pdoffset80ma0", "0");
			nvram_set("pci/2/1/pdoffset80ma1", "0");
			nvram_set("pci/2/1/pdoffset80ma2", "0");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
			nvram_set("pci/2/1/regrev", "66");
			nvram_set("pci/2/1/rpcal2g", "0");
			nvram_set("pci/2/1/rpcal5gb0", "0x610c");
			nvram_set("pci/2/1/rpcal5gb1", "0x6a09");
			nvram_set("pci/2/1/rpcal5gb2", "0x5eff");
			nvram_set("pci/2/1/rpcal5gb3", "0x700c");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr2ga0", "63");
			nvram_set("pci/2/1/rxgainerr2ga1", "31");
			nvram_set("pci/2/1/rxgainerr2ga2", "31");
			nvram_set("pci/2/1/rxgainerr5ga0", "63,63,63,63");
			nvram_set("pci/2/1/rxgainerr5ga1", "31,31,31,31");
			nvram_set("pci/2/1/rxgainerr5ga2", "31,31,31,31");
			nvram_set("pci/2/1/rxgains2gelnagaina0", "0");
			nvram_set("pci/2/1/rxgains2gelnagaina1", "0");
			nvram_set("pci/2/1/rxgains2gelnagaina2", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa0", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa1", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa2", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa0", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa1", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa2", "0");
			nvram_set("pci/2/1/rxgains5gelnagaina0", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5ghelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina1", "3");
			nvram_set("pci/2/1/rxgains5ghelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina0", "3");
			nvram_set("pci/2/1/rxgains5gmelnagaina1", "4");
			nvram_set("pci/2/1/rxgains5gmelnagaina2", "4");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/2/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/2/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/2/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/2/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/2/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/2/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/2/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/2/1/sar2g", "18");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sb20in40hrpo", "0");
			nvram_set("pci/2/1/sb20in40lrpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/2/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80hr5glpo", "0");
			nvram_set("pci/2/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/2/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/2/1/sb40and80lr5glpo", "0");
			nvram_set("pci/2/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/subvid", "0x14e4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempthresh", "255");
			nvram_set("pci/2/1/tssifloor2g", "0x3ff");
			nvram_set("pci/2/1/tssifloor5g", "0x3ff,0x3ff,0x3ff,0x3ff");
			nvram_set("pci/2/1/tssiposslope2g", "1");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/tworangetssi2g", "0");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/xtalfreq", "65535");

		}
		break;
	case MODEL_WS880:
		mfr = "Huawei";
		name = "WS880";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");

			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			inc_mac(s, +2);
			nvram_set("0:macaddr", s);
			inc_mac(s, +4);
			nvram_set("1:macaddr", s);

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// misc settings
			nvram_set("boot_wait", "off");
			nvram_set("wait_time", "1");
	
			// 2.4GHz module defaults
			nvram_set("devpath0", "pci/1/1");
			nvram_set("0:aa2g", "7");
			nvram_set("0:ag0", "0");
			nvram_set("0:ag1", "0");
			nvram_set("0:ag2", "0");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags2", "0x00100000");
			nvram_set("0:boardflags", "0x80001200");
			nvram_set("0:boardtype", "0x59b");
			nvram_set("0:boardvendor", "0x14e4");
			nvram_set("0:cckbw202gpo", "0x0000");
			nvram_set("0:cckbw20ul2gpo", "0x0000");
			nvram_set("0:ccode", "#a");
			nvram_set("0:devid", "0x4332");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:ledbh0", "11");
			// nvram_set("0:ledbh1", "11");
			nvram_set("0:ledbh2", "14");
			nvram_set("0:ledbh3", "1");
			nvram_set("0:ledbh12", "11");
			nvram_set("0:leddc", "0xffff");
			nvram_set("0:legofdm40duppo", "0x0");
			nvram_set("0:legofdmbw202gpo", "0x88888888");
			nvram_set("0:legofdmbw20ul2gpo", "0x88888888");
			nvram_set("0:maxp2ga0", "0x46");
			nvram_set("0:maxp2ga1", "0x46");
			nvram_set("0:maxp2ga2", "0x46");
			nvram_set("0:mcs32po", "0x0");
			nvram_set("0:mcsbw202gpo", "0x88888888");
			nvram_set("0:mcsbw20ul2gpo", "0x88888888");
			nvram_set("0:mcsbw402gpo", "0x88888888");
			nvram_set("0:pa2gw0a0", "0xfe63");
			nvram_set("0:pa2gw0a1", "0xfe78");
			nvram_set("0:pa2gw0a2", "0xfe65");
			nvram_set("0:pa2gw1a0", "0x1dfd");
			nvram_set("0:pa2gw1a1", "0x1e4a");
			nvram_set("0:pa2gw1a2", "0x1e74");
			nvram_set("0:pa2gw2a0", "0xf8c7");
			nvram_set("0:pa2gw2a1", "0xf8d8");
			nvram_set("0:pa2gw2a2", "0xf8b9");
			nvram_set("0:parefldovoltage", "35");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:regrev", "0");
			nvram_set("0:rxchain", "7");
			nvram_set("0:sromrev", "9");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:triso2g", "3");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "7");
			nvram_set("0:venid", "0x14e4");
			nvram_set("0:xtalfreq", "20000");

			// 5GHz module defaults
			nvram_set("devpath1", "pci/2/1");
			nvram_set("1:aa2g", "0");
			nvram_set("1:aa5g", "7");
			nvram_set("1:agbg0", "71");
			nvram_set("1:agbg1", "71");
			nvram_set("1:agbg2", "133");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags2", "0x00300002");
			nvram_set("1:boardflags3", "0x0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:boardnum", "20771");
			nvram_set("1:boardrev", "0x1402");
			nvram_set("1:boardtype", "0x621");
			nvram_set("1:boardvendor", "0x14e4");
			nvram_set("1:cckbw202gpo", "0");
			nvram_set("1:cckbw20ul2gpo", "0");
			nvram_set("1:ccode", "#a");
			nvram_set("1:devid", "0x43a2");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain2g", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:maxp2ga0", "76");
			nvram_set("1:maxp2ga1", "76");
			nvram_set("1:maxp2ga2", "76");
			nvram_set("1:maxp5ga0", "70,70,86,86");
			nvram_set("1:maxp5ga1", "70,70,86,86");
			nvram_set("1:maxp5ga2", "70,70,86,86");
			nvram_set("1:mcsbw402gpo", "0");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw402gpo", "0");
			nvram_set("1:mcsbw1605glpo", "0x00222222");
			nvram_set("1:mcsbw205ghpo", "0xaa880000");
			nvram_set("1:mcsbw205glpo", "0x66666666");
			nvram_set("1:mcsbw205gmpo", "0x66666666");
			nvram_set("1:mcsbw405ghpo", "0xaa880000");
			nvram_set("1:mcsbw405glpo", "0x22222222"); 
			nvram_set("1:mcsbw405gmpo", "0x22222222");
			nvram_set("1:mcsbw805ghpo", "0x88880000");
			nvram_set("1:mcsbw805glpo", "0x00222222");
			nvram_set("1:mcsbw805gmpo", "0x00222222");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:measpower1", "0x7f");
			nvram_set("1:measpower2", "0x7f");
			nvram_set("1:measpower", "0x7f");
			nvram_set("1:noiselvl2ga0", "31");
			nvram_set("1:noiselvl2ga1", "31");
			nvram_set("1:noiselvl2ga2", "31");
			nvram_set("1:noiselvl5ga0", "31,31,31,31");
			nvram_set("1:noiselvl5ga1", "31,31,31,31");
			nvram_set("1:noiselvl5ga2", "31,31,31,31");
			nvram_set("1:ofdmlrbw202gpo", "0");
			nvram_set("1:pa2ga0", "0xfe72,0x14c0,0xfac7");
			nvram_set("1:pa2ga1", "0xfe80,0x1472,0xfabc");
			nvram_set("1:pa2ga2", "0xfe82,0x14bf,0xfad9");
			nvram_set("1:pa5ga0", "0xff31,0x1a56,0xfcc7,0xff35,0x1a8f,0xfcc1,0xff35,0x18d4,0xfcf4,0xff2d,0x18d5,0xfce8");
			nvram_set("1:pa5ga1", "0xff30,0x190f,0xfce6,0xff38,0x1abc,0xfcc0,0xff0f,0x1762,0xfcef,0xff18,0x1648,0xfd23");
			nvram_set("1:pa5ga2", "0xff32,0x18f6,0xfce8,0xff36,0x195d,0xfcdf,0xff28,0x16ae,0xfd1e,0xff28,0x166c,0xfd2b");
			nvram_set("1:papdcap2g", "0");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pcieingress_war", "15");
			nvram_set("1:pdgain2g", "4");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0");
			nvram_set("1:pdoffset80ma1", "0");
			nvram_set("1:pdoffset80ma2", "0xfecc");
			nvram_set("1:phycal_tempdelta", "255");
			nvram_set("1:rawtempsense", "0x1ff");
			nvram_set("1:regrev", "0");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgainerr2ga0", "63");
			nvram_set("1:rxgainerr2ga1", "31");
			nvram_set("1:rxgainerr2ga2", "31");
			nvram_set("1:rxgainerr5ga0", "63,63,63,63");
			nvram_set("1:rxgainerr5ga1", "31,31,31,31");
			nvram_set("1:rxgainerr5ga2", "31,31,31,31");
			nvram_set("1:rxgains2gelnagaina0", "0");
			nvram_set("1:rxgains2gelnagaina1", "0");
			nvram_set("1:rxgains2gelnagaina2", "0");
			nvram_set("1:rxgains2gtrisoa0", "0");
			nvram_set("1:rxgains2gtrisoa1", "0");
			nvram_set("1:rxgains2gtrisoa2", "0");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar2g", "18");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:subvid", "0x14e4");
			nvram_set("1:tempcorrx", "0x3f");
			nvram_set("1:tempoffset", "255");
			nvram_set("1:temps_hysteresis", "15");
			nvram_set("1:temps_period", "15");
			nvram_set("1:tempsense_option", "0x3");
			nvram_set("1:tempsense_slope", "0xff");
			nvram_set("1:tempthresh", "255");
			nvram_set("1:tssiposslope2g", "1");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi2g", "0");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14e4");
			nvram_set("1:xtalfreq", "40000");
		}
		break;

	case MODEL_R1D:
		mfr = "Xiaomi";
		name = "MiWiFi";
		features = SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif

		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "10");
			nvram_set("router_name", "X-R1D");

			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth2");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");

			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			inc_mac(s, +1);
			nvram_set("pci/2/1/macaddr", s);
			nvram_set("wl1_hwaddr", s);
			inc_mac(s, +1);
			nvram_set("pci/1/1/macaddr", s);
			nvram_set("wl0_hwaddr", s);

			// force wl0 settings
			nvram_set("wl0_bw", "3");
			nvram_set("wl0_bw_cap", "7");
			nvram_set("wl0_chanspec", "149/80");
			nvram_set("wl0_nctrlsb", "lower");
			nvram_set("pci/1/1/ccode", "SG");
			nvram_set("pci/2/1/ccode", "SG");
			nvram_set("pci/1/1/regrev", "0");
			nvram_set("pci/2/1/regrev", "0");
			nvram_set("wl_country", "SG");
			nvram_set("wl_country_code", "SG");
			nvram_set("wl_ssid", "MiWiFi_5G");
			nvram_set("wl1_ssid", "MiWiFi");

			// usb settings
			nvram_set("usb_usb3", "0");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");
	
			// 2.4GHz module defaults
			nvram_set("pci/2/1/aa2g", "3");
			nvram_set("pci/2/1/ag0", "2");
			nvram_set("pci/2/1/ag1", "2");
			nvram_set("pci/2/1/antswctl2g", "0");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/boardflags", "0x80001000");
			nvram_set("pci/2/1/boardflags2", "0x00000800");
			nvram_set("pci/2/1/boardrev", "0x1301");
			nvram_set("pci/2/1/bw40po", "0");
			nvram_set("pci/2/1/bwduppo", "0");
			nvram_set("pci/2/1/bxa2g", "0");
			nvram_set("pci/2/1/cck2gpo", "0x8880");
			nvram_set("pci/2/1/cddpo", "0");
			nvram_set("pci/2/1/devid", "0x43A9");
			nvram_set("pci/2/1/elna2g", "2");
			nvram_set("pci/2/1/extpagain2g", "3");
			nvram_set("pci/2/1/freqoffset_corr", "0");
			nvram_set("pci/2/1/hw_iqcal_en", "0");
			nvram_set("pci/2/1/iqcal_swp_dis", "0");
			nvram_set("pci/2/1/itt2ga1", "32");
			nvram_set("pci/2/1/ledbh0", "255");
			nvram_set("pci/2/1/ledbh1", "255");
			nvram_set("pci/2/1/ledbh2", "255");
			nvram_set("pci/2/1/ledbh3", "131");
			nvram_set("pci/2/1/leddc", "65535");
			nvram_set("pci/2/1/maxp2ga0", "0x2072");
			nvram_set("pci/2/1/maxp2ga1", "0x2072");
			nvram_set("pci/2/1/mcs2gpo0", "0x8888");
			nvram_set("pci/2/1/mcs2gpo1", "0x8888");
			nvram_set("pci/2/1/mcs2gpo2", "0x8888");
			nvram_set("pci/2/1/mcs2gpo3", "0xDDB8");
			nvram_set("pci/2/1/mcs2gpo4", "0x8888");
			nvram_set("pci/2/1/mcs2gpo5", "0xA988");
			nvram_set("pci/2/1/mcs2gpo6", "0x8888");
			nvram_set("pci/2/1/mcs2gpo7", "0xDDC8");
			nvram_set("pci/2/1/measpower1", "0");
			nvram_set("pci/2/1/measpower2", "0");
			nvram_set("pci/2/1/measpower", "0");
			nvram_set("pci/2/1/ofdm2gpo", "0xAA888888");
			nvram_set("pci/2/1/opo", "68");
			nvram_set("pci/2/1/pa2gw0a0", "0xFE77");
			nvram_set("pci/2/1/pa2gw0a1", "0xFE76");
			nvram_set("pci/2/1/pa2gw1a0", "0x1C37");
			nvram_set("pci/2/1/pa2gw1a1", "0x1C5C");
			nvram_set("pci/2/1/pa2gw2a0", "0xF95D");
			nvram_set("pci/2/1/pa2gw2a1", "0xF94F");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/pdetrange2g", "3");
			nvram_set("pci/2/1/phycal_tempdelta", "0");
			nvram_set("pci/2/1/rawtempsense", "0");
			nvram_set("pci/2/1/rssisav2g", "0");
			nvram_set("pci/2/1/rssismc2g", "0");
			nvram_set("pci/2/1/rssismf2g", "0");
			nvram_set("pci/2/1/rxchain", "3");
			nvram_set("pci/2/1/rxpo2g", "0");
			nvram_set("pci/2/1/sromrev", "8");
			nvram_set("pci/2/1/stbcpo", "0");
			nvram_set("pci/2/1/tempcorrx", "0");
			nvram_set("pci/2/1/tempoffset", "0");
			nvram_set("pci/2/1/temps_hysteresis", "0");
			nvram_set("pci/2/1/temps_period", "0");
			nvram_set("pci/2/1/tempsense_option", "0");
			nvram_set("pci/2/1/tempsense_slope", "0");
			nvram_set("pci/2/1/tempthresh", "120");
			nvram_set("pci/2/1/triso2g", "4");
			nvram_set("pci/2/1/tssipos2g", "1");
			nvram_set("pci/2/1/txchain", "3");

			// 5GHz module defaults
			nvram_set("pci/1/1/aa5g", "7");
			nvram_set("pci/1/1/aga0", "01");
			nvram_set("pci/1/1/aga1", "01");
			nvram_set("pci/1/1/aga2", "133");
			nvram_set("pci/1/1/antswitch", "0");
			nvram_set("pci/1/1/boardflags", "0x30000000");
			nvram_set("pci/1/1/boardflags2", "0x00300002");
			nvram_set("pci/1/1/boardflags3", "0x00000000");
			nvram_set("pci/1/1/boardvendor", "0x14E4");
			nvram_set("pci/1/1/devid", "0x43B3");
			nvram_set("pci/1/1/dot11agduphrpo", "0");
			nvram_set("pci/1/1/dot11agduplrpo", "0");
			nvram_set("pci/1/1/epagain5g", "0");
			nvram_set("pci/1/1/femctrl", "3");
			nvram_set("pci/1/1/gainctrlsph", "0");
			nvram_set("pci/1/1/maxp5ga0", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("pci/1/1/maxp5ga1", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("pci/1/1/maxp5ga2", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("pci/1/1/mcsbw1605ghpo", "0");
			nvram_set("pci/1/1/mcsbw1605glpo", "0");
			nvram_set("pci/1/1/mcsbw1605gmpo", "0");
			nvram_set("pci/1/1/mcsbw205ghpo", "0x55540000");
			nvram_set("pci/1/1/mcsbw205glpo", "0x88642222");
			nvram_set("pci/1/1/mcsbw205gmpo", "0x88642222");
			nvram_set("pci/1/1/mcsbw405ghpo", "0x85542000");
			nvram_set("pci/1/1/mcsbw405glpo", "0xA8842222");
			nvram_set("pci/1/1/mcsbw405gmpo", "0xA8842222");
			nvram_set("pci/1/1/mcsbw805ghpo", "0x85542000");
			nvram_set("pci/1/1/mcsbw805glpo", "0xAA842222");
			nvram_set("pci/1/1/mcsbw805gmpo", "0xAA842222");
			nvram_set("pci/1/1/mcslr5ghpo", "0");
			nvram_set("pci/1/1/mcslr5glpo", "0");
			nvram_set("pci/1/1/mcslr5gmpo", "0");
			nvram_set("pci/1/1/measpower1", "0x7F");
			nvram_set("pci/1/1/measpower2", "0x7F");
			nvram_set("pci/1/1/measpower", "0x7F");
			nvram_set("pci/1/1/pa5ga0", "0xFF90,0x1E37,0xFCB8,0xFF38,0x189B,0xFD00,0xFF33,0x1A66,0xFCC4,0xFF2F,0x1748,0xFD21");
			nvram_set("pci/1/1/pa5ga1", "0xFF1B,0x18A2,0xFCB6,0xFF34,0x183F,0xFD12,0xFF37,0x1AA1,0xFCC0,0xFF2F,0x1889,0xFCFB");
			nvram_set("pci/1/1/pa5ga2", "0xFF1D,0x1653,0xFD33,0xFF38,0x1A2A,0xFCCE,0xFF35,0x1A93,0xFCC1,0xFF3A,0x1ABD,0xFCC0");
			nvram_set("pci/1/1/papdcap5g", "0");
			nvram_set("pci/1/1/pcieingress_war", "15");
			nvram_set("pci/1/1/pdgain5g", "4");
			nvram_set("pci/1/1/pdoffset40ma0", "0x1111");
			nvram_set("pci/1/1/pdoffset40ma1", "0x1111");
			nvram_set("pci/1/1/pdoffset40ma2", "0x1111");
			nvram_set("pci/1/1/pdoffset80ma0", "0");
			nvram_set("pci/1/1/pdoffset80ma1", "0");
			nvram_set("pci/1/1/pdoffset80ma2", "0");
			nvram_set("pci/1/1/phycal_tempdelta", "255");
			nvram_set("pci/1/1/rawtempsense", "0x1FF");
			nvram_set("pci/1/1/rxchain", "7");
			nvram_set("pci/1/1/rxgains5gelnagaina0", "1");
			nvram_set("pci/1/1/rxgains5gelnagaina1", "1");
			nvram_set("pci/1/1/rxgains5gelnagaina2", "1");
			nvram_set("pci/1/1/rxgains5ghelnagaina0", "2");
			nvram_set("pci/1/1/rxgains5ghelnagaina1", "2");
			nvram_set("pci/1/1/rxgains5ghelnagaina2", "3");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5ghtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5ghtrisoa0", "5");
			nvram_set("pci/1/1/rxgains5ghtrisoa1", "4");
			nvram_set("pci/1/1/rxgains5ghtrisoa2", "4");
			nvram_set("pci/1/1/rxgains5gmelnagaina0", "2");
			nvram_set("pci/1/1/rxgains5gmelnagaina1", "2");
			nvram_set("pci/1/1/rxgains5gmelnagaina2", "3");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5gmtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5gmtrisoa0", "5");
			nvram_set("pci/1/1/rxgains5gmtrisoa1", "4");
			nvram_set("pci/1/1/rxgains5gmtrisoa2", "4");
			nvram_set("pci/1/1/rxgains5gtrelnabypa0", "1");
			nvram_set("pci/1/1/rxgains5gtrelnabypa1", "1");
			nvram_set("pci/1/1/rxgains5gtrelnabypa2", "1");
			nvram_set("pci/1/1/rxgains5gtrisoa0", "7");
			nvram_set("pci/1/1/rxgains5gtrisoa1", "6");
			nvram_set("pci/1/1/rxgains5gtrisoa2", "5");
			nvram_set("pci/1/1/sar5g", "15");
			nvram_set("pci/1/1/sb20in40hrpo", "0");
			nvram_set("pci/1/1/sb20in40lrpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5ghpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5glpo", "0");
			nvram_set("pci/1/1/sb20in80and160hr5gmpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5ghpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5glpo", "0");
			nvram_set("pci/1/1/sb20in80and160lr5gmpo", "0");
			nvram_set("pci/1/1/sb40and80hr5ghpo", "0");
			nvram_set("pci/1/1/sb40and80hr5glpo", "0");
			nvram_set("pci/1/1/sb40and80hr5gmpo", "0");
			nvram_set("pci/1/1/sb40and80lr5ghpo", "0");
			nvram_set("pci/1/1/sb40and80lr5glpo", "0");
			nvram_set("pci/1/1/sb40and80lr5gmpo", "0");
			nvram_set("pci/1/1/sromrev", "11");
			nvram_set("pci/1/1/subband5gver", "4");
			nvram_set("pci/1/1/tempcorrx", "0x3F");
			nvram_set("pci/1/1/tempoffset", "255");
			nvram_set("pci/1/1/temps_hysteresis", "15");
			nvram_set("pci/1/1/temps_period", "15");
			nvram_set("pci/1/1/tempsense_option", "3");
			nvram_set("pci/1/1/tempsense_slope", "255");
			nvram_set("pci/1/1/tempthresh", "255");
			nvram_set("pci/1/1/tssiposslope5g", "1");
			nvram_set("pci/1/1/tworangetssi5g", "0");
			nvram_set("pci/1/1/txchain", "7");
			nvram_set("pci/1/1/venid", "0x14E4");
			nvram_set("pci/1/1/xtalfreq", "0x40000");
		}
		break;
	case MODEL_EA6700:
		mfr = "Cisco Linksys";
		if (strstr(nvram_safe_get("modelNumber"), "EA6500") != NULL)
			name = "EA6500v2";
		else
			name = "EA6700";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;

#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			//nvram_set("wl0_country_code", "Q2");
			//nvram_set("wl1_country_code", "Q2");
			//nvram_set("wl0_country_rev", "33");
			//nvram_set("wl1_country_rev", "33");
			//fix wifi channels
			nvram_set("0:ccode", "#a");
			nvram_set("1:ccode", "#a");
			nvram_set("0:regrev", "0");
			nvram_set("1:regrev", "0");
			nvram_set("wl0_country_code", "#a");
			nvram_set("wl0_country_rev", "0");
			nvram_set("wl0_reg_mode", "off");
			nvram_set("wl1_country_code", "#a");
			nvram_set("wl1_country_rev", "0");
			nvram_set("wl1_reg_mode", "off");
			
			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			inc_mac(s, +2);
			nvram_set("0:macaddr", s);
			inc_mac(s, +4);
			nvram_set("1:macaddr", s);

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// misc settings
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "1");
	
			// 2.4GHz module defaults
			//nvram_set("devpath0", "pci/1/1");
			nvram_set("0:aa2g", "7");
			nvram_set("0:ag0", "0");
			nvram_set("0:ag1", "0");
			nvram_set("0:ag2", "0");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags2", "0x00100000");
			nvram_set("0:boardflags", "0x80001200");
			nvram_set("0:cckbw202gpo", "0x4444");
			nvram_set("0:cckbw20ul2gpo", "0x4444");
			nvram_set("0:devid", "0x4332");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:ledbh0", "11");
			nvram_set("0:ledbh1", "11");
			nvram_set("0:ledbh2", "11");
			nvram_set("0:ledbh3", "11");
			nvram_set("0:ledbh12", "2");
			nvram_set("0:leddc", "0xFFFF");
			nvram_set("0:legofdm40duppo", "0x0");
			nvram_set("0:legofdmbw202gpo", "0x55553300");
			nvram_set("0:legofdmbw20ul2gpo", "0x55553300");
			nvram_set("0:maxp2ga0", "0x60");
			nvram_set("0:maxp2ga1", "0x60");
			nvram_set("0:maxp2ga2", "0x60");
			nvram_set("0:mcs32po", "0x0006");
			nvram_set("0:mcsbw202gpo", "0xAA997755");
			nvram_set("0:mcsbw20ul2gpo", "0xAA997755");
			nvram_set("0:mcsbw402gpo", "0xAA997755");
			nvram_set("0:pa2gw0a0", "0xFE7C");
			nvram_set("0:pa2gw0a1", "0xFE85");
			nvram_set("0:pa2gw0a2", "0xFE82");
			nvram_set("0:pa2gw1a0", "0x1E9B");
			nvram_set("0:pa2gw1a1", "0x1EA5");
			nvram_set("0:pa2gw1a2", "0x1EC5");
			nvram_set("0:pa2gw2a0", "0xF8B4");
			nvram_set("0:pa2gw2a1", "0xF8C0");
			nvram_set("0:pa2gw2a2", "0xF8B8");
			nvram_set("0:parefldovoltage", "60");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:rxchain", "7");
			nvram_set("0:sromrev", "9");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "7");
			nvram_set("0:venid", "0x14E4");
			nvram_set("0:xtalfreq", "20000");

			// 5GHz module defaults
			nvram_set("1:aa2g", "7");
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "0");
			nvram_set("1:aga1", "0");
			nvram_set("1:aga2", "0");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags2", "0x00200002");
			nvram_set("1:boardflags3", "0x0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:devid", "0x43A2");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh0", "11");
			nvram_set("1:ledbh1", "11");
			nvram_set("1:ledbh2", "11");
			nvram_set("1:ledbh3", "11");
			nvram_set("1:ledbh10", "2");
			nvram_set("1:leddc", "0xFFFF");
			nvram_set("1:maxp5ga0", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga1", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga2", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605glpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw205ghpo", "0xDD553300");
			nvram_set("1:mcsbw205glpo", "0xDD553300");
			nvram_set("1:mcsbw205gmpo", "0xDD553300");
			nvram_set("1:mcsbw405ghpo", "0xEE885544");
			nvram_set("1:mcsbw405glpo", "0xEE885544"); 
			nvram_set("1:mcsbw405gmpo", "0xEE885544");
			nvram_set("1:mcsbw805ghpo", "0xEE885544");
			nvram_set("1:mcsbw805glpo", "0xEE885544");
			nvram_set("1:mcsbw805gmpo", "0xEE885544");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:pa5ga0", "0xff2b,0x1898,0xfcf2,0xff2c,0x1947,0xfcda,0xff33,0x18f9,0xfcec,0xff2d,0x18ef,0xfce4");
			nvram_set("1:pa5ga1", "0xff31,0x1930,0xfce3,0xff30,0x1974,0xfcd9,0xff31,0x18db,0xfcee,0xff37,0x194e,0xfce1");
			nvram_set("1:pa5ga2", "0xff2e,0x193c,0xfcde,0xff2c,0x1831,0xfcf9,0xff30,0x18c6,0xfcef,0xff30,0x1942,0xfce0");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0");
			nvram_set("1:pdoffset80ma1", "0");
			nvram_set("1:pdoffset80ma2", "0");
			nvram_set("1:phycal_tempdelta", "0");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar2g", "18");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempoffset", "0");
			nvram_set("1:temps_hysteresis", "5");
			nvram_set("1:temps_period", "5");
			nvram_set("1:tempthresh", "120");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "40000");
		}
		nvram_set("acs_2g_ch_no_ovlp", "1");
		nvram_set("acs_2g_ch_no_restrict", "1");
		
		nvram_set("devpath0", "pci/1/1/");
		nvram_set("devpath1", "pci/2/1/");
		nvram_set("partialboots", "0");
		break;

	case MODEL_EA6900:
		mfr = "Linksys";
		name = "EA6900";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;

#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			//nvram_set("wl0_country_code", "Q2");
			//nvram_set("wl1_country_code", "Q2");
			//nvram_set("wl0_country_rev", "33");
			//nvram_set("wl1_country_rev", "33");
			//fix wifi channels
			nvram_set("0:ccode", "#a");
			nvram_set("1:ccode", "#a");
			nvram_set("0:regrev", "0");
			nvram_set("1:regrev", "0");
			nvram_set("wl0_country_code", "#a");
			nvram_set("wl0_country_rev", "0");
			nvram_set("wl0_reg_mode", "off");
			nvram_set("wl1_country_code", "#a");
			nvram_set("wl1_country_rev", "0");
			nvram_set("wl1_reg_mode", "off");
			
			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			inc_mac(s, +2);
			nvram_set("0:macaddr", s);
			inc_mac(s, +4);
			nvram_set("1:macaddr", s);

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// misc settings
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "1");
	
			// 2.4GHz module defaults
			nvram_set("0:aa2g", "7");
			nvram_set("0:agbg0", "0x47");
			nvram_set("0:agbg1", "0x47");
			nvram_set("0:agbg2", "0x47");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags", "0x00001000");
			nvram_set("0:boardflags2", "0x00100002");
			nvram_set("0:boardflags3", "0x00000003");
			nvram_set("0:cckbw202gpo", "0x0");
			nvram_set("0:cckbw20ul2gpo", "0x0");
			nvram_set("0:devid", "0x43A1");
			nvram_set("0:dot11agduphrpo", "0x0");
			nvram_set("0:dot11agduplrpo", "0x0");
			nvram_set("0:dot11agofdmhrbw202gpo", "0x6666");
			nvram_set("0:epagain2g", "0");
			nvram_set("0:femctrl", "3");
			nvram_set("0:gainctrlsph", "0");
			nvram_set("0:ledbh0", "0xFF");
			nvram_set("0:ledbh1", "0xFF");
			nvram_set("0:ledbh10", "2");
			nvram_set("0:ledbh2", "0xFF");
			nvram_set("0:ledbh3", "0xFF");
			nvram_set("0:leddc", "0xFFFF");
			nvram_set("0:maxp2ga0", "0x62");
			nvram_set("0:maxp2ga1", "0x62");
			nvram_set("0:maxp2ga2", "0x62");
			nvram_set("0:mcsbw202gpo", "0xCC666600");
			nvram_set("0:mcsbw402gpo", "0xCC666600");
			nvram_set("0:ofdmlrbw202gpo", "0x0");
			nvram_set("0:pa2ga0", "0xff22,0x1a4f,0xfcc1");
			nvram_set("0:pa2ga1", "0xff22,0x1a71,0xfcbb");
			nvram_set("0:pa2ga2", "0xff1f,0x1a21,0xfcc2");
			nvram_set("0:papdcap2g", "0");
			nvram_set("0:parefldovoltage", "35");
			nvram_set("0:pdgain2g", "14");
			nvram_set("0:pdoffset2g40ma0", "0x3");
			nvram_set("0:pdoffset2g40ma1", "0x3");
			nvram_set("0:pdoffset2g40ma2", "0x3");
			nvram_set("0:phycal_tempdelta", "0");
			//nvram_set("0:rpcal2g", "47823");
			nvram_set("0:rpcal2g", "53985");
			nvram_set("0:rxchain", "7");
			nvram_set("0:rxgains2gelnagaina0", "4");
			nvram_set("0:rxgains2gelnagaina1", "4");
			nvram_set("0:rxgains2gelnagaina2", "4");
			nvram_set("0:rxgains2gtrelnabypa0", "1");
			nvram_set("0:rxgains2gtrelnabypa1", "1");
			nvram_set("0:rxgains2gtrelnabypa2", "1");
			nvram_set("0:rxgains2gtrisoa0", "7");
			nvram_set("0:rxgains2gtrisoa1", "7");
			nvram_set("0:rxgains2gtrisoa2", "7");
			nvram_set("0:sb20in40hrpo", "0x0");
			nvram_set("0:sb20in40lrpo", "0x0");
			nvram_set("0:sromrev", "11");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:temps_hysteresis", "5");
			nvram_set("0:temps_period", "5");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:tssiposslope2g", "1");
			nvram_set("0:tworangetssi2g", "0");
			nvram_set("0:txchain", "7");
			nvram_set("0:venid", "0x14E4");
			nvram_set("0:xtalfreq", "40000");

			// 5GHz module defaults
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "0");
			nvram_set("1:aga1", "0");
			nvram_set("1:aga2", "0");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:boardflags2", "0x00200002");
			nvram_set("1:boardflags3", "0x00000000");
			nvram_set("1:devid", "0x43A2");
			nvram_set("1:dot11agduphrpo", "0x0");
			nvram_set("1:dot11agduplrpo", "0x0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh0", "11");
			nvram_set("1:ledbh1", "11");
			nvram_set("1:ledbh10", "2");
			nvram_set("1:ledbh2", "11");
			nvram_set("1:ledbh3", "11");
			nvram_set("1:leddc", "0xFFFF");
			nvram_set("1:maxp5ga0", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga1", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:maxp5ga2", "0x5C,0x5C,0x5C,0x5C");
			nvram_set("1:mcsbw205ghpo", "0xBB555500");
			nvram_set("1:mcsbw205glpo", "0xBB555500");
			nvram_set("1:mcsbw205gmpo", "0xBB555500");
			nvram_set("1:mcsbw405ghpo", "0xBB777700");
			nvram_set("1:mcsbw405glpo", "0xBB777700");
			nvram_set("1:mcsbw405gmpo", "0xBB777700");
			nvram_set("1:mcsbw805ghpo", "0xBB777700");
			nvram_set("1:mcsbw805glpo", "0xBB777733");
			nvram_set("1:mcsbw805gmpo", "0xBB777700");
			nvram_set("1:mcslr5ghpo", "0x0");
			nvram_set("1:mcslr5glpo", "0x0");
			nvram_set("1:mcslr5gmpo", "0x0");
			nvram_set("1:pa5ga0", "0xff2e,0x185a,0xfcfc,0xff37,0x1903,0xfcf1,0xff4b,0x197f,0xfcff,0xff37,0x180f,0xfd12");
			nvram_set("1:pa5ga1", "0xff33,0x1944,0xfce5,0xff30,0x18c6,0xfcf5,0xff40,0x19c7,0xfce5,0xff38,0x18cc,0xfcf9");
			nvram_set("1:pa5ga2", "0xff34,0x1962,0xfce1,0xff35,0x193b,0xfceb,0xff38,0x1921,0xfcf1,0xff39,0x188f,0xfd00");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:parefldovoltage", "35");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0xEEEE");
			nvram_set("1:pdoffset80ma1", "0xEEEE");
			nvram_set("1:pdoffset80ma2", "0xEEEE");
			nvram_set("1:phycal_tempdelta", "0");
			//nvram_set("1:rpcal5gb0", "32015");
			//nvram_set("1:rpcal5gb3", "35617");
			nvram_set("1:rpcal5gb0", "41773");
			nvram_set("1:rpcal5gb3", "42547");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sb20in40hrpo", "0x0");
			nvram_set("1:sb20in40lrpo", "0x0");
			nvram_set("1:sb20in80and160hr5ghpo", "0x0");
			nvram_set("1:sb20in80and160hr5glpo", "0x0");
			nvram_set("1:sb20in80and160hr5gmpo", "0x0");
			nvram_set("1:sb20in80and160lr5ghpo", "0x0");
			nvram_set("1:sb20in80and160lr5glpo", "0x0");
			nvram_set("1:sb20in80and160lr5gmpo", "0x0");
			nvram_set("1:sb40and80hr5ghpo", "0x0");
			nvram_set("1:sb40and80hr5glpo", "0x0");
			nvram_set("1:sb40and80hr5gmpo", "0x0");
			nvram_set("1:sb40and80lr5ghpo", "0x0");
			nvram_set("1:sb40and80lr5glpo", "0x0");
			nvram_set("1:sb40and80lr5gmpo", "0x0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempoffset", "0");
			nvram_set("1:temps_hysteresis", "5");
			nvram_set("1:temps_period", "5");
			nvram_set("1:tempthresh", "120");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "40000");
		}
		nvram_set("acs_2g_ch_no_ovlp", "1");
		nvram_set("acs_2g_ch_no_restrict", "1");
		
		nvram_set("devpath0", "pci/1/1/");
		nvram_set("devpath1", "pci/2/1/");
		nvram_set("partialboots", "0");
		break;

	case MODEL_WZR1750:
		mfr = "Buffalo";
		name = "WZR-1750DHP";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;

#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("vlan1hwname", "et0");
			nvram_set("vlan2hwname", "et0");
			nvram_set("lan_ifname", "br0");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnames", "vlan2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wandevs", "vlan2");
			nvram_set("wl_ifnames", "eth2 eth1");
			nvram_set("wl_ifname", "eth2");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");

			// force wl settings
			nvram_set("wl0_bw", "3");
			nvram_set("wl0_bw_cap", "7");
			nvram_set("wl0_chanspec", "149/80");
			nvram_set("wl0_nctrlsb", "lower");
			nvram_set("1:ccode", "SG");
			nvram_set("0:ccode", "SG");
			nvram_set("1:regrev", "0");
			nvram_set("0:regrev", "0");
			nvram_set("wl_country", "SG");
			nvram_set("wl_country_code", "SG");

			// fix WL mac for 2,4G
			nvram_set("wl0_hwaddr", nvram_safe_get("0:macaddr"));

			// fix WL mac for 5G
			strcpy(s, nvram_safe_get("0:macaddr"));
			inc_mac(s, +1);
			nvram_set("wl1_hwaddr", s);

			// usb3.0 settings
			nvram_set("usb_usb3", "1");
			nvram_set("xhci_ports", "1-1");
			nvram_set("ehci_ports", "2-1 2-2");
			nvram_set("ohci_ports", "3-1 3-2");

			// misc settings
			nvram_set("boot_wait", "on");
			nvram_set("wait_time", "1");

			// 2.4GHz module defaults
			nvram_set("devpath0", "pci/2/1");
			nvram_set("0:aa2g", "3");
			nvram_set("0:ag0", "2");
			nvram_set("0:ag1", "2");
			nvram_set("0:antswctl2g", "0");
			nvram_set("0:antswitch", "0");
			nvram_set("0:boardflags", "0x80001000");
			nvram_set("0:boardflags2", "0x00000800");
			nvram_set("0:boardrev", "0x1301");
			nvram_set("0:bw40po", "0");
			nvram_set("0:bwduppo", "0");
			nvram_set("0:bxa2g", "0");
			nvram_set("0:cck2gpo", "0x8880");
			nvram_set("0:cddpo", "0");
			nvram_set("0:devid", "0x43A9");
			nvram_set("0:elna2g", "2");
			nvram_set("0:extpagain2g", "3");
			nvram_set("0:freqoffset_corr", "0");
			nvram_set("0:hw_iqcal_en", "0");
			nvram_set("0:iqcal_swp_dis", "0");
			nvram_set("0:itt2ga1", "32");
			nvram_set("0:ledbh0", "255");
			nvram_set("0:ledbh1", "255");
			nvram_set("0:ledbh2", "255");
			nvram_set("0:ledbh3", "131");
			nvram_set("0:ledbh12", "7");
			nvram_set("0:leddc", "65535");
			nvram_set("0:maxp2ga0", "0x2072");
			nvram_set("0:maxp2ga1", "0x2072");
			nvram_set("0:mcs2gpo0", "0x8888");
			nvram_set("0:mcs2gpo1", "0x8888");
			nvram_set("0:mcs2gpo2", "0x8888");
			nvram_set("0:mcs2gpo3", "0xDDB8");
			nvram_set("0:mcs2gpo4", "0x8888");
			nvram_set("0:mcs2gpo5", "0xA988");
			nvram_set("0:mcs2gpo6", "0x8888");
			nvram_set("0:mcs2gpo7", "0xDDC8");
			nvram_set("0:measpower1", "0");
			nvram_set("0:measpower2", "0");
			nvram_set("0:measpower", "0");
			nvram_set("0:ofdm2gpo", "0xAA888888");
			nvram_set("0:opo", "68");
			nvram_set("0:pa2gw0a0", "0xFE77");
			nvram_set("0:pa2gw0a1", "0xFE76");
			nvram_set("0:pa2gw1a0", "0x1C37");
			nvram_set("0:pa2gw1a1", "0x1C5C");
			nvram_set("0:pa2gw2a0", "0xF95D");
			nvram_set("0:pa2gw2a1", "0xF94F");
			nvram_set("0:pcieingress_war", "15");
			nvram_set("0:pdetrange2g", "3");
			nvram_set("0:phycal_tempdelta", "0");
			nvram_set("0:rawtempsense", "0");
			nvram_set("0:rssisav2g", "0");
			nvram_set("0:rssismc2g", "0");
			nvram_set("0:rssismf2g", "0");
			nvram_set("0:rxchain", "3");
			nvram_set("0:rxpo2g", "0");
			nvram_set("0:sromrev", "8");
			nvram_set("0:stbcpo", "0");
			nvram_set("0:tempcorrx", "0");
			nvram_set("0:tempoffset", "0");
			nvram_set("0:temps_hysteresis", "0");
			nvram_set("0:temps_period", "0");
			nvram_set("0:tempsense_option", "0");
			nvram_set("0:tempsense_slope", "0");
			nvram_set("0:tempthresh", "120");
			nvram_set("0:triso2g", "4");
			nvram_set("0:tssipos2g", "1");
			nvram_set("0:txchain", "3");

			// 5GHz module defaults
			nvram_set("devpath1", "pci/1/1");
			nvram_set("1:aa5g", "7");
			nvram_set("1:aga0", "01");
			nvram_set("1:aga1", "01");
			nvram_set("1:aga2", "133");
			nvram_set("1:antswitch", "0");
			nvram_set("1:boardflags", "0x30000000");
			nvram_set("1:boardflags2", "0x00300002");
			nvram_set("1:boardflags3", "0x00000000");
			nvram_set("1:boardvendor", "0x14E4");
			nvram_set("1:devid", "0x43B3");
			nvram_set("1:dot11agduphrpo", "0");
			nvram_set("1:dot11agduplrpo", "0");
			nvram_set("1:epagain5g", "0");
			nvram_set("1:femctrl", "3");
			nvram_set("1:gainctrlsph", "0");
			nvram_set("1:ledbh10", "7");
			nvram_set("1:maxp5ga0", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("1:maxp5ga1", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("1:maxp5ga2", "0x5E,0x5E,0x5E,0x5E");
			nvram_set("1:mcsbw1605ghpo", "0");
			nvram_set("1:mcsbw1605glpo", "0");
			nvram_set("1:mcsbw1605gmpo", "0");
			nvram_set("1:mcsbw205ghpo", "0x55540000");
			nvram_set("1:mcsbw205glpo", "0x88642222");
			nvram_set("1:mcsbw205gmpo", "0x88642222");
			nvram_set("1:mcsbw405ghpo", "0x85542000");
			nvram_set("1:mcsbw405glpo", "0xA8842222");
			nvram_set("1:mcsbw405gmpo", "0xA8842222");
			nvram_set("1:mcsbw805ghpo", "0x85542000");
			nvram_set("1:mcsbw805glpo", "0xAA842222");
			nvram_set("1:mcsbw805gmpo", "0xAA842222");
			nvram_set("1:mcslr5ghpo", "0");
			nvram_set("1:mcslr5glpo", "0");
			nvram_set("1:mcslr5gmpo", "0");
			nvram_set("1:measpower1", "0x7F");
			nvram_set("1:measpower2", "0x7F");
			nvram_set("1:measpower", "0x7F");
			nvram_set("1:pa5ga0", "0xFF90,0x1E37,0xFCB8,0xFF38,0x189B,0xFD00,0xFF33,0x1A66,0xFCC4,0xFF2F,0x1748,0xFD21");
			nvram_set("1:pa5ga1", "0xFF1B,0x18A2,0xFCB6,0xFF34,0x183F,0xFD12,0xFF37,0x1AA1,0xFCC0,0xFF2F,0x1889,0xFCFB");
			nvram_set("1:pa5ga2", "0xFF1D,0x1653,0xFD33,0xFF38,0x1A2A,0xFCCE,0xFF35,0x1A93,0xFCC1,0xFF3A,0x1ABD,0xFCC0");
			nvram_set("1:papdcap5g", "0");
			nvram_set("1:pcieingress_war", "15");
			nvram_set("1:pdgain5g", "4");
			nvram_set("1:pdoffset40ma0", "0x1111");
			nvram_set("1:pdoffset40ma1", "0x1111");
			nvram_set("1:pdoffset40ma2", "0x1111");
			nvram_set("1:pdoffset80ma0", "0");
			nvram_set("1:pdoffset80ma1", "0");
			nvram_set("1:pdoffset80ma2", "0");
			nvram_set("1:phycal_tempdelta", "255");
			nvram_set("1:rawtempsense", "0x1FF");
			nvram_set("1:rxchain", "7");
			nvram_set("1:rxgains5gelnagaina0", "1");
			nvram_set("1:rxgains5gelnagaina1", "1");
			nvram_set("1:rxgains5gelnagaina2", "1");
			nvram_set("1:rxgains5ghelnagaina0", "2");
			nvram_set("1:rxgains5ghelnagaina1", "2");
			nvram_set("1:rxgains5ghelnagaina2", "3");
			nvram_set("1:rxgains5ghtrelnabypa0", "1");
			nvram_set("1:rxgains5ghtrelnabypa1", "1");
			nvram_set("1:rxgains5ghtrelnabypa2", "1");
			nvram_set("1:rxgains5ghtrisoa0", "5");
			nvram_set("1:rxgains5ghtrisoa1", "4");
			nvram_set("1:rxgains5ghtrisoa2", "4");
			nvram_set("1:rxgains5gmelnagaina0", "2");
			nvram_set("1:rxgains5gmelnagaina1", "2");
			nvram_set("1:rxgains5gmelnagaina2", "3");
			nvram_set("1:rxgains5gmtrelnabypa0", "1");
			nvram_set("1:rxgains5gmtrelnabypa1", "1");
			nvram_set("1:rxgains5gmtrelnabypa2", "1");
			nvram_set("1:rxgains5gmtrisoa0", "5");
			nvram_set("1:rxgains5gmtrisoa1", "4");
			nvram_set("1:rxgains5gmtrisoa2", "4");
			nvram_set("1:rxgains5gtrelnabypa0", "1");
			nvram_set("1:rxgains5gtrelnabypa1", "1");
			nvram_set("1:rxgains5gtrelnabypa2", "1");
			nvram_set("1:rxgains5gtrisoa0", "7");
			nvram_set("1:rxgains5gtrisoa1", "6");
			nvram_set("1:rxgains5gtrisoa2", "5");
			nvram_set("1:sar5g", "15");
			nvram_set("1:sb20in40hrpo", "0");
			nvram_set("1:sb20in40lrpo", "0");
			nvram_set("1:sb20in80and160hr5ghpo", "0");
			nvram_set("1:sb20in80and160hr5glpo", "0");
			nvram_set("1:sb20in80and160hr5gmpo", "0");
			nvram_set("1:sb20in80and160lr5ghpo", "0");
			nvram_set("1:sb20in80and160lr5glpo", "0");
			nvram_set("1:sb20in80and160lr5gmpo", "0");
			nvram_set("1:sb40and80hr5ghpo", "0");
			nvram_set("1:sb40and80hr5glpo", "0");
			nvram_set("1:sb40and80hr5gmpo", "0");
			nvram_set("1:sb40and80lr5ghpo", "0");
			nvram_set("1:sb40and80lr5glpo", "0");
			nvram_set("1:sb40and80lr5gmpo", "0");
			nvram_set("1:sromrev", "11");
			nvram_set("1:subband5gver", "4");
			nvram_set("1:tempcorrx", "0x3F");
			nvram_set("1:tempoffset", "255");
			nvram_set("1:temps_hysteresis", "15");
			nvram_set("1:temps_period", "15");
			nvram_set("1:tempsense_option", "3");
			nvram_set("1:tempsense_slope", "255");
			nvram_set("1:tempthresh", "255");
			nvram_set("1:tssiposslope5g", "1");
			nvram_set("1:tworangetssi5g", "0");
			nvram_set("1:txchain", "7");
			nvram_set("1:venid", "0x14E4");
			nvram_set("1:xtalfreq", "0x40000");
		}
		break;

#endif
	case MODEL_RTN66U:
		mfr = "Asus";
#ifdef TCONFIG_AC66U
		name = "RT-AC66U";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#else
		name = "RT-N66U";
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#if defined(LINUX26) && defined(TCONFIG_MICROSD)
		if (nvram_get_int("usb_mmc") == -1) nvram_set("usb_mmc", "1");
#endif
#endif

#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
#ifdef TCONFIG_AC66U
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
#endif
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wandevs", "vlan2");
#ifndef TCONFIG_AC66U
#if defined(LINUX26) && defined(TCONFIG_USB)
			nvram_set("usb_noled", "1-1.4"); /* SD/MMC Card */
#endif
#else
			nvram_set("wl1_bw_cap","7");
			nvram_set("wl1_chanspec","36/80");
			nvram_set("wl0_bw_cap","3");
			nvram_set("wl0_chanspec","1l");
			nvram_set("blink_5g_interface","eth2");

			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			inc_mac(s, +2);
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +1);
			nvram_set("wl1_hwaddr", s);

			// bcm4360ac_defaults
			nvram_set("pci/2/1/aa2g", "0");
			nvram_set("pci/2/1/aa5g", "7");
			nvram_set("pci/2/1/aga0", "71");
			nvram_set("pci/2/1/aga1", "71");
			nvram_set("pci/2/1/aga2", "71");
			nvram_set("pci/2/1/agbg0", "133");
			nvram_set("pci/2/1/agbg1", "133");
			nvram_set("pci/2/1/agbg2", "133");
			nvram_set("pci/2/1/antswitch", "0");
			nvram_set("pci/2/1/cckbw202gpo", "0");
			nvram_set("pci/2/1/cckbw20ul2gpo", "0");
			nvram_set("pci/2/1/dot11agofdmhrbw202gpo", "0");
			nvram_set("pci/2/1/femctrl", "3");
			nvram_set("pci/2/1/papdcap2g", "0");
			nvram_set("pci/2/1/tworangetssi2g", "0");
			nvram_set("pci/2/1/pdgain2g", "4");
			nvram_set("pci/2/1/epagain2g", "0");
			nvram_set("pci/2/1/tssiposslope2g", "1");
			nvram_set("pci/2/1/gainctrlsph", "0");
			nvram_set("pci/2/1/papdcap5g", "0");
			nvram_set("pci/2/1/tworangetssi5g", "0");
			nvram_set("pci/2/1/pdgain5g", "4");
			nvram_set("pci/2/1/epagain5g", "0");
			nvram_set("pci/2/1/tssiposslope5g", "1");
			nvram_set("pci/2/1/maxp2ga0", "76");
			nvram_set("pci/2/1/maxp2ga1", "76");
			nvram_set("pci/2/1/maxp2ga2", "76");
			nvram_set("pci/2/1/mcsbw202gpo", "0");
			nvram_set("pci/2/1/mcsbw402gpo", "0");
			nvram_set("pci/2/1/measpower", "0x7f");
			nvram_set("pci/2/1/measpower1", "0x7f");
			nvram_set("pci/2/1/measpower2", "0x7f");
			nvram_set("pci/2/1/noiselvl2ga0", "31");
			nvram_set("pci/2/1/noiselvl2ga1", "31");
			nvram_set("pci/2/1/noiselvl2ga2", "31");
			nvram_set("pci/2/1/noiselvl5gha0", "31");
			nvram_set("pci/2/1/noiselvl5gha1", "31");
			nvram_set("pci/2/1/noiselvl5gha2", "31");
			nvram_set("pci/2/1/noiselvl5gla0", "31");
			nvram_set("pci/2/1/noiselvl5gla1", "31");
			nvram_set("pci/2/1/noiselvl5gla2", "31");
			nvram_set("pci/2/1/noiselvl5gma0", "31");
			nvram_set("pci/2/1/noiselvl5gma1", "31");
			nvram_set("pci/2/1/noiselvl5gma2", "31");
			nvram_set("pci/2/1/noiselvl5gua0", "31");
			nvram_set("pci/2/1/noiselvl5gua1", "31");
			nvram_set("pci/2/1/noiselvl5gua2", "31");
			nvram_set("pci/2/1/ofdmlrbw202gpo", "0");
			nvram_set("pci/2/1/pa2ga0", "0xfe72,0x14c0,0xfac7");
			nvram_set("pci/2/1/pa2ga1", "0xfe80,0x1472,0xfabc");
			nvram_set("pci/2/1/pa2ga2", "0xfe82,0x14bf,0xfad9");
			nvram_set("pci/2/1/pcieingress_war", "15");
			nvram_set("pci/2/1/phycal_tempdelta", "255");
			nvram_set("pci/2/1/rawtempsense", "0x1ff");
			nvram_set("pci/2/1/rxchain", "7");
			nvram_set("pci/2/1/rxgainerr2g", "0xffff");
			nvram_set("pci/2/1/rxgainerr5g", "0xffff,0xffff,0xffff,0xffff");
			nvram_set("pci/2/1/rxgains2gelnagaina0", "0");
			nvram_set("pci/2/1/rxgains2gelnagaina1", "0");
			nvram_set("pci/2/1/rxgains2gelnagaina2", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa0", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa1", "0");
			nvram_set("pci/2/1/rxgains2gtrelnabypa2", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa0", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa1", "0");
			nvram_set("pci/2/1/rxgains2gtrisoa2", "0");
			nvram_set("pci/2/1/sar2g", "18");
			nvram_set("pci/2/1/sar5g", "15");
			nvram_set("pci/2/1/sromrev", "11");
			nvram_set("pci/2/1/subband5gver", "0x4");
			nvram_set("pci/2/1/tempcorrx", "0x3f");
			nvram_set("pci/2/1/tempoffset", "255");
			nvram_set("pci/2/1/temps_hysteresis", "15");
			nvram_set("pci/2/1/temps_period", "15");
			nvram_set("pci/2/1/tempsense_option", "0x3");
			nvram_set("pci/2/1/tempsense_slope", "0xff");
			nvram_set("pci/2/1/tempthresh", "255");
			nvram_set("pci/2/1/txchain", "7");
			nvram_set("pci/2/1/ledbh0", "2");
			nvram_set("pci/2/1/ledbh1", "5");
			nvram_set("pci/2/1/ledbh2", "4");
			nvram_set("pci/2/1/ledbh3", "11");
			nvram_set("pci/2/1/ledbh10", "7");

			//force EU country for eth2
			nvram_set("pci/2/1/ccode", "EU");
#endif // TCONFIG_AC66U
		}
		break;
#ifdef CONFIG_BCMWL6
	case MODEL_W1800R:
		mfr = "Tenda";
		name = "W1800R";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");
			nvram_set("wl0_bw_cap","7");
			nvram_set("wl0_chanspec","36/80");
			nvram_set("wl1_bw_cap","3");
			nvram_set("wl1_chanspec","1l");
			nvram_set("blink_5g_interface","eth1");
			//nvram_set("landevs", "vlan1 wl0 wl1");
			//nvram_set("wandevs", "vlan2");

			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			nvram_set("wl0_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("wl1_hwaddr", nvram_safe_get("1:macaddr"));

			// fix ssid according to 5G(eth2) and 2.4G(eth1) 
			nvram_set("wl_ssid","Tomato50");
			nvram_set("wl0_ssid","Tomato50");
			nvram_set("wl1_ssid","Tomato24");
		}
		break;
	case MODEL_D1800H:
		mfr = "Buffalo";
		if (nvram_match("product", "WLI-H4-D1300")) {
			name = "WLI-H4-D1300";
		}
		else if (nvram_match("product", "WZR-D1100H")) {
			name = "WZR-D1100H";
		}
		else {
			name = "WZR-D1800H";
		}
		features = SUP_SES | SUP_AOSS_LED | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth2");
			nvram_set("wl1_ifname", "eth1");
			nvram_set("wl0_bw_cap","7");
			nvram_set("wl0_chanspec","36/80");
			nvram_set("wl1_bw_cap","3");
			nvram_set("wl1_chanspec","1l");
			nvram_set("blink_5g_interface","eth1");

			// fix WL mac`s
			strcpy(s, nvram_safe_get("et0macaddr"));
			trimstr(s);
			int i;
			for (i = 0; i < strlen(s); i ++) if ( s[i] == '-') s[i] = ':';
			nvram_set("et0macaddr",s);
			inc_mac(s, +2);
			nvram_set("wl0_hwaddr", s);
			inc_mac(s, +1);
			nvram_set("wl1_hwaddr", s);

			// fix ssid according to 5G(eth2) and 2.4G(eth1) 
			nvram_set("wl_ssid","Tomato50");
			nvram_set("wl0_ssid","Tomato50");
			nvram_set("wl1_ssid","Tomato24");

			nvram_set("pci/2/1/maxp2ga0", "0x70");
			nvram_set("pci/2/1/maxp2ga1", "0x70");
			nvram_set("pci/2/1/maxp2ga2", "0x70");
			nvram_set("pci/2/1/maxp5ga0", "0x6A");
			nvram_set("pci/2/1/maxp5ga1", "0x6A");
			nvram_set("pci/2/1/maxp5ga2", "0x6A");
			nvram_set("pci/2/1/cckbw202gpo", "0x5555");
			nvram_set("pci/2/1/cckbw20ul2gpo", "0x5555");
			nvram_set("pci/2/1/legofdmbw202gpo", "0x97555555");
			nvram_set("pci/2/1/legofdmbw20ul2gpo", "0x97555555");
			nvram_set("pci/2/1/mcsbw202gpo", "0xDA755555");
			nvram_set("pci/2/1/mcsbw20ul2gpo", "0xDA755555");
			nvram_set("pci/2/1/mcsbw402gpo", "0xFC965555");
			nvram_set("pci/2/1/cckbw205gpo", "0x5555");
			nvram_set("pci/2/1/cckbw20ul5gpo", "0x5555");
			nvram_set("pci/2/1/legofdmbw205gpo", "0x97555555");
			nvram_set("pci/2/1/legofdmbw20ul5gpo", "0x97555555");
			nvram_set("pci/2/1/legofdmbw205gmpo", "0x77777777");
			nvram_set("pci/2/1/legofdmbw20ul5gmpo", "0x77777777");
			nvram_set("pci/2/1/legofdmbw205ghpo", "0x77777777");
			nvram_set("pci/2/1/legofdmbw20ul5ghpo", "0x77777777");
			nvram_set("pci/2/1/mcsbw205ghpo", "0x77777777");
			nvram_set("pci/2/1/mcsbw20ul5ghpo", "0x77777777");
			nvram_set("pci/2/1/mcsbw205gpo", "0xDA755555");
			nvram_set("pci/2/1/mcsbw20ul5gpo", "0xDA755555");
			nvram_set("pci/2/1/mcsbw405gpo", "0xFC965555");
			nvram_set("pci/2/1/mcsbw405ghpo", "0x77777777");
			nvram_set("pci/2/1/mcsbw405ghpo", "0x77777777");
			nvram_set("pci/2/1/mcs32po", "0x7777");
			nvram_set("pci/2/1/legofdm40duppo", "0x0000");
			nvram_set("pci/1/1/maxp5ga0", "104,104,104,104");
			nvram_set("pci/1/1/maxp5ga1", "104,104,104,104");
			nvram_set("pci/1/1/maxp5ga2", "104,104,104,104");
			nvram_set("pci/1/1/mcsbw205glpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw405glpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw805glpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw205gmpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw405gmpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw805gmpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw205ghpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw405ghpo", "0xBB975311");
			nvram_set("pci/1/1/mcsbw805ghpo", "0xBB975311");

			//force US country for 5G eth1, modified by bwq518
			nvram_set("pci/1/1/ccode", "US");
			nvram_set("wl1_country_code", "US");
			nvram_set("regulation_domain_5G", "US");
		}
		break;
	case MODEL_EA6500V1:
		mfr = "Linksys";
		name = "EA6500v1";
		features = SUP_SES | SUP_80211N | SUP_1000ET | SUP_80211AC;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifnames", "eth1 eth2");
			nvram_set("wl_ifname", "eth1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
			nvram_set("wl0_bw_cap","7");
			nvram_set("wl0_chanspec","36/80");
			nvram_set("wl1_bw_cap","3");
			nvram_set("wl1_chanspec","1l");
			nvram_set("blink_5g_interface","eth1");

			// fix ssid according to 5G(eth1) and 2.4G(eth2) 
			nvram_set("wl_ssid","Tomato50");
			nvram_set("wl0_ssid","Tomato50");
			nvram_set("wl1_ssid","Tomato24");

			//force US country for 5G eth1, modified by bwq518
			nvram_set("pci/1/1/ccode", nvram_safe_get("ccode"));
			nvram_set("regulation_domain_5G", nvram_safe_get("ccode"));
		}
		break;
#endif // CONFIG_BCMWL6
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
	case MODEL_WNR3500LV2:
		mfr = "Netgear";
		name = "WNR3500L v2";
		features = SUP_SES | SUP_AOSS_LED | SUP_80211N | SUP_1000ET;
		if (!nvram_match("t_fix1", (char *)name)) {
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
	case MODEL_E900:
	case MODEL_E1500:
		mfr = "Linksys";
		name = nvram_safe_get("boot_hw_model");
		ver = nvram_safe_get("boot_hw_ver");
		features = SUP_SES | SUP_80211N;
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_E1550:
		mfr = "Linksys";
		name = nvram_safe_get("boot_hw_model");
		ver = nvram_safe_get("boot_hw_ver");
		features = SUP_SES | SUP_80211N;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");
		}
		break;
	case MODEL_E2500:
		mfr = "Linksys";
		name = nvram_safe_get("boot_hw_model");
		ver = nvram_safe_get("boot_hw_ver");
		features = SUP_SES | SUP_80211N;
#if defined(LINUX26) && defined(TCONFIG_USBAP)
		if (nvram_get_int("usb_storage") == 1) nvram_set("usb_storage", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
#ifdef TCONFIG_USBAP
			nvram_set("wl1_hwaddr", nvram_safe_get("0:macaddr"));
			nvram_set("ehciirqt", "3");
			nvram_set("qtdc_pid", "48407");
			nvram_set("qtdc_vid", "2652");
			nvram_set("qtdc0_ep", "4");
			nvram_set("qtdc0_sz", "0");
			nvram_set("qtdc1_ep", "18");
			nvram_set("qtdc1_sz", "10");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
#else
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("landevs", "vlan1 wl0");
#endif
			nvram_set("wan_ifnameX", "vlan2");
			nvram_set("wl_ifname", "eth1");

		}
		break;
	case MODEL_E3200:
		mfr = "Linksys";
		name = nvram_safe_get("boot_hw_model");
		ver = nvram_safe_get("boot_hw_ver");
		features = SUP_SES | SUP_80211N | SUP_1000ET;
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
#ifdef TCONFIG_USBAP
			nvram_set("wl1_hwaddr", nvram_safe_get("usb/0xBD17/macaddr"));
			nvram_set("ehciirqt", "3");
			nvram_set("qtdc_pid", "48407");
			nvram_set("qtdc_vid", "2652");
			nvram_set("qtdc0_ep", "4");
			nvram_set("qtdc0_sz", "0");
			nvram_set("qtdc1_ep", "18");
			nvram_set("qtdc1_sz", "10");
			nvram_set("lan_ifnames", "vlan1 eth1 eth2");
			nvram_set("landevs", "vlan1 wl0 wl1");
			nvram_set("wl0_ifname", "eth1");
			nvram_set("wl1_ifname", "eth2");
#else
			nvram_set("lan_ifnames", "vlan1 eth1");
			nvram_set("landevs", "vlan1 wl0");
#endif
			nvram_set("wl_ifname", "eth1");
			nvram_set("wan_ifnameX", "vlan2");
		}
		break;
	case MODEL_E1000v2:
	case MODEL_WRT160Nv3:
		// same as M10, M20, WRT310Nv2, E1000v1
		mfr = "Linksys";
		name = nvram_safe_get("boot_hw_model");
		ver = nvram_safe_get("boot_hw_ver");
		if (nvram_match("boot_hw_model", "E100")){
			name = "E1000";
		}
		if (nvram_match("boot_hw_model", "M10") || nvram_match("boot_hw_model", "M20")){
			mfr = "Cisco";
		}
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

	case MODEL_WL330GE:
		mfr = "Asus";
		name = "WL-330gE";
		// The 330gE has only one wired port which can act either as WAN or LAN.
		// Failsafe mode is to have it start as a LAN port so you can get an IP
		// address via DHCP and access the router config page.
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("wl_ifname", "eth1");
			nvram_set("lan_ifnames", "eth1");
			nvram_set("wan_ifnameX", "eth0");
			nvram_set("wan_islan", "1");
			nvram_set("wan_proto", "disabled");
		}
		break;
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
#ifdef TCONFIG_USB
		nvram_set("usb_uhci", "-1");
#endif
		if (!nvram_match("t_fix1", (char *)name)) {
			nvram_set("lan_ifnames", "vlan0 eth1");
			nvram_set("wan_ifname", "vlan1");
			nvram_set("wan_ifnames", "vlan1");
			nvram_set("wan_ifnameX", "vlan1");
			nvram_set("wl_ifname", "eth1");
		}
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
		if (ver && strcmp(ver, "")) {
			sprintf(s, "%s %s v%s", mfr, name, ver);
		} else {
			sprintf(s, "%s %s", mfr, name);
		}
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

#ifndef TCONFIG_BCMARM
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
#endif

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

#ifdef LINUX26
	eval("hotplug2", "--coldplug");
	start_hotplug2();
#endif

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

	//load after initnvram as Broadcom Wl require pci/x/1/devid and pci/x/1/macaddr nvram to be set first for DIR-865L
	//else 5G interface will not start!
	//must be tested on other routers to determine if loading nvram 1st will cause problems!
	//load_wl();

	//config_loopback();

//	eval("nvram", "defaults", "--initcheck");
	restore_defaults(); // restore default if necessary

	init_nvram();

	// set the packet size
	if (nvram_get_int("jumbo_frame_enable")) {
		// only set the size here - 'enable' flag is set by the driver
		// eval("et", "robowr", "0x40", "0x01", "0x1F"); // (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4)
		eval("et", "robowr", "0x40", "0x05", nvram_safe_get("jumbo_frame_size"));
	}

	//load after init_nvram
	load_wl();

	config_loopback();

	klogctl(8, NULL, nvram_get_int("console_loglevel"));

#if defined(LINUX26) && defined(TCONFIG_USB)
	tune_min_free_kbytes();
#endif
	setup_conntrack();
	set_host_domain_name();

	set_tz();

	eval("buttons");

#ifdef CONFIG_BCMWL6
	eval("blink_5g");
#endif

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

// AB - failsafe?
	nvram_unset("debug_rc_svc");

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

			if( nvram_match( "webmon_bkp", "1" ) )
				xstart( "/usr/sbin/webmon_bkp", "hourly" ); // make a copy before halt/reboot router

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

#ifndef TCONFIG_BCMARM
			load_files_from_nvram();
#endif

			int fd = -1;
			fd = file_lock("usb");	// hold off automount processing
			start_usb();

			xstart("/usr/sbin/mymotd", "init");
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

