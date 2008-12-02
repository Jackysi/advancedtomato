
/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

 This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 the contents of this file may not be disclosed to third parties,
 copied or duplicated in any form without the prior written
 permission of CyberTAN Inc.

 This software should be used as a reference only, and it not
 intended for production use!


 THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

/*
 * Router rc control script
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: rc.c,v 1.103.10.4.2.6 2006/07/19 07:19:13 honor Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <string.h>
#include <sys/klog.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <net/if_arp.h>
#include <dirent.h>

#include <epivers.h>
#include <bcmnvram.h>
#include <mtd.h>
#include <shutils.h>
#include <rc.h>
#include <netconf.h>
#include <nvparse.h>
#include <bcmdevs.h>
#include <bcmparams.h>
#include <wlutils.h>

#include <utils.h>
#include <cyutils.h>
#include <code_pattern.h>
#include <cy_conf.h>
#include <support.h>
#include <ezc.h>
#include <cymac.h>

static void restore_defaults(void);
static void sysinit(void);
static void rc_signal(int sig);

static int check_cfe_nv(void);
static int check_pmon_nv(void);
static void unset_nvram(void);
static int init_nvram(void);
static int check_image(void);

extern struct nvram_tuple router_defaults[];

static int
build_ifnames(char *type, char *names, int *size)
{
	char name[32], *next;
	int len = 0;
	int s;

	/* open a raw scoket for ioctl */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
       		return -1;

	/*
	 * go thru all device names (wl<N> il<N> et<N> vlan<N>) and interfaces to 
	 * build an interface name list in which each i/f name coresponds to a device
	 * name in device name list. Interface/device name matching rule is device
	 * type dependant:
	 *
	 *	wl:	by unit # provided by the driver, for example, if eth1 is wireless
	 *		i/f and its unit # is 0, then it will be in the i/f name list if
	 *		wl0 is in the device name list.
	 *	il/et:	by mac address, for example, if et0's mac address is identical to
	 *		that of eth2's, then eth2 will be in the i/f name list if et0 is 
	 *		in the device name list.
	 *	vlan:	by name, for example, vlan0 will be in the i/f name list if vlan0
	 *		is in the device name list.
	 */
	foreach (name, type, next) {
		struct ifreq ifr;
		int i, unit;
		char var[32], *mac, ea[ETHER_ADDR_LEN];
		
		/* vlan: add it to interface name list */
		if (!strncmp(name, "vlan", 4)) {
			/* append interface name to list */
			len += snprintf(&names[len], *size - len, "%s ", name);
			continue;
		}

		/* others: proceed only when rules are met */
		for (i = 1; i <= DEV_NUMIFS; i ++) {
			/* ignore i/f that is not ethernet */
			ifr.ifr_ifindex = i;
			if (ioctl(s, SIOCGIFNAME, &ifr))
				continue;
			if (ioctl(s, SIOCGIFHWADDR, &ifr))
				continue;
			if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
				continue;
			if (!strncmp(ifr.ifr_name, "vlan", 4))
				continue;
				
			/* wl: use unit # to identify wl */
			if (!strncmp(name, "wl", 2)) {
				if (wl_probe(ifr.ifr_name) ||
				    wl_ioctl(ifr.ifr_name, WLC_GET_INSTANCE, &unit, sizeof(unit)) ||
				    unit != atoi(&name[2]))
					continue;
			}
			/* et/il: use mac addr to identify et/il */
			else if (!strncmp(name, "et", 2) || !strncmp(name, "il", 2)) {
				snprintf(var, sizeof(var), "%smacaddr", name);
				if (!(mac = nvram_get(var)) || !ether_atoe(mac, ea) ||
				    bcmp(ea, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN))
					continue;
			}
			/* mac address: compare value */
			else if (ether_atoe(name, ea) && !bcmp(ea, ifr.ifr_hwaddr.sa_data, ETHER_ADDR_LEN))
				;
			/* others: ignore */
			else
				continue;

			/* append interface name to list */
			len += snprintf(&names[len], *size - len, "%s ", ifr.ifr_name);
		}
	}
	
	close(s);

	*size = len;
	return 0;
}	

#ifdef __CONFIG_SES__
static void
ses_cleanup(void)
{
	/* well known event to cleanly initialize state machine */
	nvram_set("ses_event", "2");

	/* Delete lethal dynamically generated variables */
	nvram_unset("ses_bridge_disable");
}

static void
ses_restore_defaults(void)
{
	char tmp[100], prefix[] = "wlXXXXXXXXXX_ses_";
	int i;

	/* Delete dynamically generated variables */
	for (i = 0; i < MAX_NVPARSE; i++) {
		sprintf(prefix, "wl%d_ses_", i);
		nvram_unset(strcat_r(prefix, "ssid", tmp));
		nvram_unset(strcat_r(prefix, "closed", tmp));
		nvram_unset(strcat_r(prefix, "wpa_psk", tmp));
		nvram_unset(strcat_r(prefix, "auth", tmp));
		nvram_unset(strcat_r(prefix, "wep", tmp));
		nvram_unset(strcat_r(prefix, "auth_mode", tmp));
		nvram_unset(strcat_r(prefix, "crypto", tmp));
		nvram_unset(strcat_r(prefix, "akm", tmp));
	}
}
#endif /* __CONFIG_SES__ */

static void
restore_defaults(void)
{
	struct nvram_tuple generic[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "eth0 eth2 eth3 eth4", 0 },
		{ "wan_ifname", "eth1", 0 },
		{ "wan_ifnames", "eth1", 0 },
		{ 0, 0, 0 }
	};
#ifdef __CONFIG_VLAN__
	struct nvram_tuple vlan[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "vlan0 eth1 eth2 eth3", 0 },
		{ "wan_ifname", "vlan1", 0 },
		{ "wan_ifnames", "vlan1", 0 },
		{ 0, 0, 0 }
	};
#endif	/* __CONFIG_VLAN__ */
	struct nvram_tuple dyna[] = {
		{ "lan_ifname", "br0", 0 },
		{ "lan_ifnames", "", 0 },
		{ "wan_ifname", "", 0 },
		{ "wan_ifnames", "", 0 },
		{ 0, 0, 0 }
	};

	struct nvram_tuple *linux_overrides;
	struct nvram_tuple *t, *u;
	int restore_defaults;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif	/* __CONFIG_VLAN_ */
	char *landevs, *wandevs;
	char lan_ifnames[128], wan_ifnames[128];
	char wan_ifname[32], *next;
	int len;
	int ap = 0;

	/* Restore defaults if told to or OS has changed */
	restore_defaults = !nvram_match("restore_defaults", "0") || nvram_invmatch("os_name", "linux");
	if (restore_defaults)
		cprintf("Restoring defaults...");

	/* Delete dynamically generated variables */
#ifdef BRCM
	if (restore_defaults) {
		char tmp[100], prefix[] = "wlXXXXXXXXXX_";
		int i;
		for (i = 0; i < MAX_NVPARSE; i++) {
#ifdef __CONFIG_NAT__
			del_filter_client(i);
			del_forward_port(i);
			del_autofw_port(i);
#endif	/* __CONFIG_NAT__ */
			snprintf(prefix, sizeof(prefix), "wl%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wl_", 3))
					nvram_unset(strcat_r(prefix, &t->name[3], tmp));
			}
#ifdef __CONFIG_NAT__
			snprintf(prefix, sizeof(prefix), "wan%d_", i);
			for (t = router_defaults; t->name; t ++) {
				if (!strncmp(t->name, "wan_", 4))
					nvram_unset(strcat_r(prefix, &t->name[4], tmp));
			}
#endif	/* __CONFIG_NAT__ */
		}
#ifdef __CONFIG_SES__
		ses_restore_defaults();
#endif /* __CONFIG_SES__ */
	}
#endif

	/* 
	 * Build bridged i/f name list and wan i/f name list from lan device name list
	 * and wan device name list. Both lan device list "landevs" and wan device list
	 * "wandevs" must exist in order to preceed.
	 */
	//if ((landevs = nvram_get("landevs")) && (wandevs = nvram_get("wandevs"))) {
	if (0) {
		/* build bridged i/f list based on nvram variable "landevs" */
		len = sizeof(lan_ifnames);
		if (!build_ifnames(landevs, lan_ifnames, &len) && len)
			dyna[1].value = lan_ifnames;
		else
			goto canned_config;
		/* build wan i/f list based on nvram variable "wandevs" */
		len = sizeof(wan_ifnames);
		if (!build_ifnames(wandevs, wan_ifnames, &len) && len) {
			dyna[3].value = wan_ifnames;
			foreach (wan_ifname, wan_ifnames, next) {
				dyna[2].value = wan_ifname;
				break;
			}
		}
		else
			ap = 1;
		linux_overrides = dyna;
	}
	/* override lan i/f name list and wan i/f name list with default values */
	else {
canned_config:
#ifdef __CONFIG_VLAN__
		boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
		if (boardflags & BFL_ENETVLAN || check_hw_type() == BCM4712_CHIP)
			linux_overrides = vlan;
		else
#endif	/* __CONFIG_VLAN__ */
			linux_overrides = generic;
	}

	/* Restore defaults */
	for (t = router_defaults; t->name; t++) {
		if (restore_defaults || !nvram_get(t->name)) {
			for (u = linux_overrides; u && u->name; u++) {
				if (!strcmp(t->name, u->name)) {
					nvram_set(u->name, u->value);
					break;
				}
			}
			if (!u || !u->name)
				nvram_set(t->name, t->value);
		}
	}

	/* Force to AP */
	if (ap)
		nvram_set("router_disable", "1");

	/* Always set OS defaults */
	nvram_set("os_name", "linux");
	nvram_set("os_version", EPI_ROUTER_VERSION_STR);
	nvram_set("os_date", __DATE__);

	nvram_set("is_modified", "0");
	nvram_set("ezc_version", EZC_VERSION_STR);
	
	if(check_hw_type()==BCM4712_CHIP) //Barry Adds for SW SES on 4712 BOARD, 20050323 
	{
		nvram_set("gpio2","adm_eecs");
		nvram_set("gpio3","adm_eesk");
		nvram_set("gpio5","adm_eedi");
		nvram_set("gpio6","adm_rc");
		nvram_unset("gpio4");
	}
	else	
		if(check_hw_type()==BCM4702_CHIP) //Barry Adds for SW SES on 4712 BOARD, 20050323 
		{
			nvram_unset("gpio2");
			nvram_unset("gpio3");
			nvram_unset("gpio4");
			nvram_unset("gpio5");
			nvram_unset("gpio6");
		}

	if(check_now_boot() == CFE_BOOT)
		check_cfe_nv();
	else if(check_now_boot() == PMON_BOOT)
		check_pmon_nv();
	

	/* Commit values */
	if (restore_defaults) {
		int i;

		unset_nvram();
#ifdef __CONFIG_SES__
		ses_restore_defaults();
#endif /* __CONFIG_SES__ */
		nvram_commit();
		cprintf("done\n");
#ifdef HW_QOS_SUPPORT
		qos_init();
#endif

		for(i=0;i<MAX_NVPARSE;i++)
			del_wds_wsec(0,i);
	}
}

static int noconsole = 0;

static void
sysinit(void)
{
	char buf[PATH_MAX];
	struct utsname name;
	struct stat tmp_stat;
	time_t tm = 0;
	int match = 0;

	/* /proc */
	mount("proc", "/proc", "proc", MS_MGC_VAL, NULL);

	/* /tmp */
	mount("ramfs", "/tmp", "ramfs", MS_MGC_VAL, NULL);

	/* /var */
	mkdir("/tmp/var", 0777);
	mkdir("/var/lock", 0777);
	mkdir("/var/log", 0777);
	mkdir("/var/run", 0777);
	mkdir("/var/tmp", 0777);

#ifdef MULTILANG_SUPPORT
	int ret;
	/* /tmp/www */
	mkdir("/tmp/www", 0777);
//	ret = mount("/dev/mtdblock/3", "/tmp/www", "cramfs", MS_RDONLY, NULL);
	ret = mount("/dev/mtdblock/3", "/tmp/www", "squashfs", MS_RDONLY, NULL);
	cprintf("ret = %d\n", ret);
	if(ret != 0) {
		rmdir("/tmp/www");
		cprintf("www -> /www\n");
		if(is_exist("/www/lang_pack/language") && file_to_buf("/www/lang_pack/language", buf, sizeof(buf)))
			match = 1;
		perror("mount");
	}
	else
	{
		cprintf("www -> /tmp/www\n");
		if(is_exist("/tmp/www/lang_pack/language") && file_to_buf("/tmp/www/lang_pack/language", buf, sizeof(buf)))
			match = 1;
	}
#endif

	/* Setup console */
	if (console_init())
		noconsole = 1;
	klogctl(8, NULL, atoi(nvram_safe_get("console_loglevel")));

#ifdef MULTILANG_SUPPORT
	cprintf("Language Package: %s\n", chomp(buf));
	if(match)
		nvram_set("language", chomp(buf));
	else
		nvram_set("language", "EN");
#endif

	/* Modules */
	uname(&name);
	snprintf(buf, sizeof(buf), "/lib/modules/%s", name.release);
	if (stat("/proc/modules", &tmp_stat) == 0 &&
	    stat(buf, &tmp_stat) == 0) {
		char module[80], *modules[10], *next, buf[254]="";
		int i=0;
		if(check_hw_type() == BCM4702_CHIP) {
			modules[i++] = "4702et";
			modules[i++] = "diag";
		}
		else
			modules[i++] = "et";

		modules[i++] = "ctmisc";

#if defined(HW_QOS_SUPPORT) || defined(PERFORMANCE_SUPPORT)
		if(check_hw_type() == BCM4712_CHIP)
			modules[i++] = "port_based_qos_mod";
#endif

#ifdef WIRELESS_SUPPORT
		modules[i++] = "wl";
#endif
		modules[i++] = NULL;

		bzero(buf, sizeof(buf));

		if(nvram_invmatch("ct_modules", "")) {
			strcpy(buf, nvram_safe_get("ct_modules"));
		}
		else {
			for(i=0 ; modules[i] ; i++) {
                	        snprintf(buf+strlen(buf), sizeof(buf), "%s ", modules[i]);
				cprintf("modules[%d]=%s buf=[%s]\n", i, modules[i], buf);
			}
		}
		cprintf("Needed modules: %s\n", buf);
		foreach(module, buf, next)
		{
			int not_success;
			not_success = eval("insmod", module);
			//cprintf("tallest:=====(canwork=%d)=====\n",not_success);
			if(not_success && (check_hw_type() == BCM4704_BCM5325F_CHIP))
				diag_led(DIAG, MALFUNCTION_LED);
				
		}
	}

	/* Set a sane date */
	stime(&tm);

	dprintf("done\n");
}

/* States */
enum {
	RESTART,
	STOP,
	START,
	TIMER,
	USER,
	IDLE,
};
static int state = START;
static int signalled = -1;

/* Signal handling */
static void
rc_signal(int sig)
{
	if (state == IDLE) {	
		if (sig == SIGHUP) {
			printf("signalling RESTART\n");
			signalled = RESTART;
		}
		else if (sig == SIGUSR2) {
			printf("signalling START\n");
			signalled = START;
		}
		else if (sig == SIGINT) {
			printf("signalling STOP\n");
			signalled = STOP;
		}
		else if (sig == SIGALRM) {
			printf("signalling TIMER\n");
			signalled = TIMER;
		}
		else if (sig == SIGUSR1) {		// Receive from WEB
			printf("signalling USER1\n");
			signalled = USER;
                }
		 
	}
}

/* Timer procedure */
int
do_timer(void)
{
#ifdef BRCM
	int interval = atoi(nvram_safe_get("timer_interval"));
	time_t now;
	struct tm gm, local;
	struct timezone tz;

	dprintf("%d\n", interval);

	if (interval == 0)
		return 0;

	/* Report stats */
	if (nvram_invmatch("stats_server", "")) {
		char *stats_argv[] = { "stats", nvram_get("stats_server"), NULL };
		_eval(stats_argv, NULL, 5, NULL);
	}

	/* Sync time */
	start_ntpc();

	/* Update kernel timezone */
	setenv("TZ", nvram_safe_get("time_zone"), 1);
	time(&now);
	gmtime_r(&now, &gm);
	localtime_r(&now, &local);
	tz.tz_minuteswest = (mktime(&gm) - mktime(&local)) / 60;
	settimeofday(NULL, &tz);

	alarm(interval);
#else
	//do_ntp();
#endif
	return 0;
}

/* Main loop */
static void
main_loop(void)
{
	sigset_t sigset;
	pid_t shell_pid = 0;
#ifdef __CONFIG_VLAN__
	uint boardflags;
#endif
        /******** add by zg 2006-11-10 for modifying ip_conntrack_max ********/
        int max_conntrack=1024;

	FILE *fp;
	
	/* Basic initialization */
	sysinit();

	/* Setup signal handlers */
	signal_init();
	signal(SIGHUP, rc_signal);
	signal(SIGUSR1, rc_signal);	// Start single service from WEB, by honor
	signal(SIGUSR2, rc_signal);
	signal(SIGINT, rc_signal);
	signal(SIGALRM, rc_signal);
	sigemptyset(&sigset);

	/* Give user a chance to run a shell before bringing up the rest of the system */
	if (!noconsole)
		run_shell(1, 0);
	
	/* Get boardflags to see if VLAN is supported */
#ifdef __CONFIG_VLAN__
	boardflags = strtoul(nvram_safe_get("boardflags"), NULL, 0);
#endif	/* __CONFIG_VLAN__ */

	/* Add loopback */
	config_loopback();
#ifdef BRCM
	/* Convert deprecated variables */
	convert_deprecated();
#endif

	/* init nvram , by honor*/
        init_nvram();

#ifdef __CONFIG_SES__
	/* ses cleanup of variables left half way through */
	ses_cleanup();
#endif /* __CONFIG_SES__ */

	/* Restore defaults if necessary */
	restore_defaults();

	/* Update boot from embedded boot.bin. If we find that boot have serious bug, we need to do this. by honor */
        check_image();

#ifdef TINYLOGIN_SUPPORT
	init_login_account();
#endif	

	/***************************************
	 * add by tanghui@2006-09-15
	 * for CDRouter NAT test
     ***************************************/

    /******** add by zg 2006-11-10 for modifying ip_conntrack_max ********/
    if ((fp=fopen("/proc/meminfo", "r")))
    {
        while (!feof (fp))
        {
            int memtotal=0; 
            char buf[1024];
            if (fgets (buf, sizeof(buf), fp))
            {
                if (!strncmp(buf, "MemTotal:", 9))
                {
                    sscanf(buf, "MemTotal: %u", &memtotal);
                    if (memtotal > 1024*8 && memtotal <= 1024*16)
                        max_conntrack = 2048;
                    else if(memtotal > 1024*16) 
                        max_conntrack = 8192;
                    break;
                }
            }
        }
        fclose(fp);
    }	
    if ((fp=fopen("/proc/sys/net/ipv4/ip_conntrack_max", "w+")))
    {
        fprintf(fp, "%u", max_conntrack);
        fclose(fp);
    }
    /******** end by zg 2006-11-10 for modifying ip_conntrack_max ********/

    if ((fp=fopen("/proc/sys/net/ipv4/ip_conntrack_tcp_timeouts", "w+")))
    {
        fprintf(fp, "300 600 120 60 120 120 10 60 30 120");
        fclose(fp);
    }
    if ((fp=fopen("/proc/sys/net/ipv4/tcp_syncookies", "w+")))
    {
        fprintf(fp, "1");
        fclose(fp);
    }
    /***************************************/
	stop_httpd();
	/* Loop forever */
	for (;;) {
		switch (state) {
		case USER:		// Restart single service from WEB or tftpd, by honor
                        dprintf("USER1\n");
                        start_single_service();
                        state = IDLE;
                        break;
		case RESTART:
#ifdef EZC_SUPPORT
			//Added by Daniel(2004-07-30)
			if(nvram_match("wl_auth_mode","psk")&&nvram_match("is_modified","1")) {
			    nvram_set("security_mode","psk");
			    nvram_set("security_mode2","wpa_personal");
			}
#endif
			dprintf("RESTART\n");
			/* Fall through */
		case STOP:
			dprintf("STOP\n");
			/***************************************
			 * remove by tanghui @ 2006-10-09
			 ***************************************/
			//stop_wan();
			/***************************************/
			stop_services();
			stop_wan();
			stop_lan();
#ifdef __CONFIG_VLAN__
                        if (boardflags & BFL_ENETVLAN)
                                stop_vlan();
#endif  /* __CONFIG_VLAN__ */
			stop_resetbutton();
			if (state == STOP) {
				state = IDLE;
				break;
			}
			/* Fall through */
		case START:
			dprintf("START\n");
			start_resetbutton();
#ifdef __CONFIG_VLAN__
			if (boardflags & BFL_ENETVLAN)
				start_vlan();
#endif	/* __CONFIG_VLAN__ */
			start_lan();
			start_services();
			start_wan(BOOT);
			diag_led(DIAG, STOP_LED);
			SET_LED(RELEASE_WAN_CONTROL);
			start_nas("wan");

			if(check_hw_type() == BCM4704_BCM5325F_CHIP) {
				//Barry for testing , 4306 on 4704 board
        	                eval("wl","antdiv","0");
                	        eval("wl","txant","3");
                	        eval("wl","antdiv");
                	        eval("wl","txant");
			}
	
			/* Fall through */
		case TIMER:
			dprintf("TIMER\n");
			do_timer();
			/* Fall through */
		case IDLE:
			dprintf("IDLE\n");
			state = IDLE;
			/* Wait for user input or state change */
			while (signalled == -1) {
				if (!noconsole && (!shell_pid || kill(shell_pid, 0) != 0))
					shell_pid = run_shell(0, 1);
				else
					sigsuspend(&sigset);
			}
			state = signalled;
			signalled = -1;
			break;
		default:
			dprintf("UNKNOWN\n");
			return;
		}
	}
}


#define CONVERT_NV(old, new) \
	if(nvram_get(old)) \
		nvram_set(new, nvram_safe_get(old));


static int
init_nvram(void)
{
	int i;

#ifdef WIRELESS_SUPPORT
	/* broadcom 3.11.48.7 change some nvram name */
	CONVERT_NV("d11g_channel","wl_channel");
	CONVERT_NV("d11g_rateset","wl_rateset");
	CONVERT_NV("d11g_rts","wl_rts");
	CONVERT_NV("d11g_bcn","wl_bcn");
	CONVERT_NV("d11g_mode","wl_gmode");
	CONVERT_NV("d11g_rate","wl_rate");
	CONVERT_NV("d11g_frag","wl_frag");
	CONVERT_NV("d11g_dtim","wl_dtim");

	nvram_set("wl0_hwaddr","");	// When disbale wireless, we must get null wireless mac */

	if(nvram_match("wl_gmode","5"))	// Mixed mode had been changed to 5
		nvram_set("wl_gmode","1");

	if(nvram_match("wl_gmode","4")) // G-ONLY mode had been changed to 2, after 1.40.1 for WiFi G certication
                nvram_set("wl_gmode","2");
	nvram_set("wl_country","ALL");	// The country always all
	nvram_set("wl_country_code","ALL");	// The country_code always all

	/* The tkip and aes already are changed to wl_crypto from v3.63.3.0 */
	if(nvram_match("wl_wep", "tkip"))
		nvram_set("wl_crypto", "tkip");
	else if(nvram_match("wl_wep", "aes"))
		nvram_set("wl_crypto", "tkip");
	else if(nvram_match("wl_wep", "tkip+aes"))
		nvram_set("wl_crypto", "tkip");

	if(nvram_match("wl_wep", "restricted"))	
		nvram_set("wl_wep", "enabled");	// the nas need this value, the "restricted" is no longer need. (20040624 by honor)

	/* Adjust WPA/WPA2 GUI, add security_mode2 2005-05-25 by honor */
	if(!nvram_get("security_mode2")) {
		if(nvram_match("security_mode", "disabled")) {
			nvram_set("security_mode2", "disabled");
			nvram_set("wl_akm", "");		// Old version doesn't 
			nvram_set("wl_auth_mode", "none");
		}
		else if(nvram_match("security_mode", "wep")) {
			nvram_set("security_mode2", "wep");
			nvram_set("wl_akm", "");
			nvram_set("wl_auth_mode", "none");
		}
		else if(nvram_match("security_mode", "radius")) {
			nvram_set("security_mode2", "radius");
			nvram_set("wl_akm", "");
			nvram_set("wl_auth_mode", "radius");
		}
		else if(nvram_match("security_mode", "psk")) {
			nvram_set("security_mode2", "wpa_personal");
			nvram_set("wl_akm", "psk");
			nvram_set("wl_auth_mode", "none");
		}
		else if(nvram_match("security_mode", "psk2") || 
		       (nvram_match("security_mode", "psk psk2"))) {
			nvram_set("security_mode2", "wpa2_personal");
			nvram_set("wl_auth_mode", "none");
		}
		else if(nvram_match("security_mode", "wpa")) {
			nvram_set("security_mode2", "wpa_enterprise"); 
			nvram_set("wl_akm", "wpa");
			nvram_set("wl_auth_mode", "none");
		}
		else if(nvram_match("security_mode", "wpa2") ||
		       (nvram_match("security_mode", "wpa wpa2"))) {
			nvram_set("security_mode2", "wpa2_enterprise");
			nvram_set("wl_auth_mode", "none");
		}
	}

	if(check_hw_type() == BCM4704_BCM5325F_CHIP) { // Fixed wireless power issue for 4318 EEPROM
		nvram_set("wl_country","US");
		nvram_set("wl_country_code","US");
	}
	else if(check_hw_type() == BCM5352E_CHIP)
	{
		nvram_set("opo","0x0008");
		nvram_set("ag0","0x02");
	}

	nvram_set("wds_ifname","");
#endif

	nvram_set("wan_get_dns","");
	nvram_set("filter_id","1");
	nvram_set("wl_active_add_mac","0");
	nvram_set("ddns_change","");
	nvram_set("action_service","");
	nvram_set("ddns_interval","60");
	nvram_set("wan_get_domain","");

#ifndef SPEED_BOOSTER_SUPPORT
	/* wl_net_mode is added from WRT54GS */
	if(!strcmp(nvram_safe_get("wl_gmode"), "0"))
		nvram_set("wl_net_mode", "b-only");
	else if(!strcmp(nvram_safe_get("wl_gmode"), "1"))
		nvram_set("wl_net_mode", "mixed");
	else if(!strcmp(nvram_safe_get("wl_gmode"), "2"))
		nvram_set("wl_net_mode", "g-only");
	else if(!strcmp(nvram_safe_get("wl_gmode"), "-1"))
		nvram_set("wl_net_mode", "disabled");
#endif

#ifndef AOL_SUPPORT
#else
	nvram_set("aol_block_traffic","0");
	nvram_set("aol_block_traffic1","0");
	nvram_set("aol_block_traffic2","0");
#endif
	nvram_set("ping_ip","");
	nvram_set("ping_times","");
	nvram_set("traceroute_ip","");

	nvram_set("filter_port", "");	// The name have been disbaled from 1.41.3

	/* Let HW1.x users can communicate with WAP54G without setting to factory default */
#ifdef SPEED_BOOSTER_SUPPORT
	if(nvram_match("wl_net_mode", "speedbooster"))
		nvram_safe_set("wl_lazywds", "0");
	else
#endif
	{
#ifdef WL_WDS_SUPPORT
#else
		nvram_safe_set("wl_lazywds", "1");
#endif
	}

	if(nvram_invmatch("restore_defaults", "1")){
		for (i = 0; i < MAX_NVPARSE; i++) {
			char name[] = "forward_portXXXXXXXXXX";
			snprintf(name, sizeof(name), "forward_port%d", i);
			if(nvram_get(name) && strstr(nvram_safe_get(name), "msmsgs")){
				cprintf("unset MSN value %d..........\n", i);
				nvram_unset(name);
			}
		}
	}
	nvram_set("upnp_wan_proto", "");

#ifdef FOR_WAN_PORT
	if(check_hw_type() == BCM4712_CHIP) //Barry 20040901
	{
        	nvram_set("vlan0ports","0 2 3 4 5*"); // DPN
        	nvram_set("vlan1ports","1 5");
	}
#endif

#ifdef WRITE_MAC_SUPPORT
	eval("misc", "-t", "get_mac", "-w", "3");		// write mac value and index
#endif
#ifdef EOU_SUPPORT
	eval("misc", "-t", "get_eou", "-w", "2");		// write eou index
	nvram_unset("eou_ie");
#endif
#ifdef WRITE_SN_SUPPORT
	eval("misc", "-t", "get_sn", "-w", "3");		// write sn value and index
#endif
#ifndef CFI_FLASH_OP
	eval("misc", "-t", "get_flash_type", "-w", "1");
#endif
/* For DDNS */
#if LOCALE == EUROPE
	int flag = 0;
#ifdef DDNS3322_SUPPORT
	flag ++;
#endif
#ifdef PEANUTHULL_SUPPORT	
	flag ++;
#endif
	// Only English and Chinese Simplified support 3322 and PeanutHull
	if(flag == 2) {	// Support 3322 and PeanutHull
		if(nvram_match("language", "EN") || nvram_match("language", "SC")) {
			if(nvram_match("ddns_enable", "1") || nvram_match("ddns_enable", "2")) {
				nvram_set("ddns_enable","0");
				nvram_set("ddns_service","");
			}
		}
		else {
			if(nvram_match("ddns_enable", "3") || nvram_match("ddns_enable", "4")) {
				nvram_set("ddns_enable","0");
				nvram_set("ddns_service","");
			}
		}
	}
#endif

	return 0;
}

static void
unset_nvram(void)
{
	int i;
#ifndef MPPPOE_SUPPORT
	nvram_safe_unset("ppp_username_1");
	nvram_safe_unset("ppp_passwd_1");
	nvram_safe_unset("ppp_idletime_1");
	nvram_safe_unset("ppp_demand_1");
	nvram_safe_unset("ppp_redialperiod_1");
	nvram_safe_unset("ppp_service_1");
	nvram_safe_unset("mpppoe_enable");
	nvram_safe_unset("mpppoe_dname");
#endif
#ifndef HTTPS_SUPPORT
        nvram_safe_unset("remote_mgt_https");
#endif
#ifndef HSIAB_SUPPORT
        nvram_safe_unset("hsiab_mode");
        nvram_safe_unset("hsiab_provider");
        nvram_safe_unset("hsiab_device_id");
        nvram_safe_unset("hsiab_device_password");
        nvram_safe_unset("hsiab_admin_url");
        nvram_safe_unset("hsiab_registered");
        nvram_safe_unset("hsiab_configured");
        nvram_safe_unset("hsiab_register_ops");
        nvram_safe_unset("hsiab_session_ops");
        nvram_safe_unset("hsiab_config_ops");
        nvram_safe_unset("hsiab_manual_reg_ops");
        nvram_safe_unset("hsiab_proxy_host");
        nvram_safe_unset("hsiab_proxy_port");
        nvram_safe_unset("hsiab_conf_time");
        nvram_safe_unset("hsiab_stats_time");
        nvram_safe_unset("hsiab_session_time");
        nvram_safe_unset("hsiab_sync");
        nvram_safe_unset("hsiab_config");
#endif
	
#ifndef HEARTBEAT_SUPPORT
        nvram_safe_unset("hb_server_ip");
        nvram_safe_unset("hb_server_domain");
#endif
			
#ifndef PARENTAL_CONTROL_SUPPORT
        nvram_safe_unset("artemis_enable");
        nvram_safe_unset("artemis_SVCGLOB");
        nvram_safe_unset("artemis_HB_DB");
        nvram_safe_unset("artemis_provisioned");
#endif

#ifndef WL_STA_SUPPORT
        nvram_safe_unset("wl_ap_ssid");
        nvram_safe_unset("wl_ap_ip");
#endif

	for (i = 0; i < MAX_NVPARSE; i++) {
		del_forward_port(i);
	}
	
#ifdef SYSLOG_SUPPORT
	{
	struct support_list *s;
	char buf[80];
	for(s = supports ; s < &supports[SUPPORT_COUNT] ; s++) {
		snprintf(buf, sizeof(buf), "LOG_%s", s->name);
		nvram_safe_set(buf, s->log_level);
	}
	}
#endif
	
#ifndef SYSLOG_SUPPORT
	{
	struct support_list *s;
	char buf[80];
	for(s = supports ; s < &supports[SUPPORT_COUNT] ; s++) {
		snprintf(buf, sizeof(buf), "LOG_%s", s->name);
		if(nvram_get(buf)){
			nvram_safe_unset(buf);
		}
	}
	}
	nvram_safe_unset("log_show_all");
	nvram_safe_unset("log_show_type");
#endif
        //Alpha add 2007-06-13 to unset upnp port forward nvram value when factory default.
#if 0   // remove by tanghui @ 2007-07-04, do it in del_forward_port();
        char name[] = "forward_portXXXXXXXXXX";
#ifndef UPNP_FORWARD_SUPPOR
        for(i = 0; i < 10; i++)
        {
                snprintf(name, sizeof(name), "forward_port%d", i);
                nvram_safe_unset(name);
        }
#endif
        for(i = 10; i < 32; i++)
        {
                snprintf(name, sizeof(name), "forward_port%d", i);
                nvram_safe_unset(name);
        }
        for(i = 0; i < 32; i++)
        {
                snprintf(name, sizeof(name), "forward_portsip%d", i);
                nvram_safe_unset(name);
        }
#endif
}

static int
check_nv(char *name, char *value)
{
	int ret = 0;

	if(nvram_match("manual_boot_nv", "1"))
		return;
	
	if(!nvram_get(name)){
		cprintf("ERR: Cann't find %s !.......................\n", name);
		nvram_set(name, value);
		ret ++; 
	}
	else if (nvram_invmatch(name, value)){
		cprintf("ERR: The %s is %s, not %s !.................\n", name, nvram_safe_get(name), value);
		nvram_set(name, value);
		ret ++;
	}

	return ret;
}

static int
check_cfe_nv(void)
{
	int ret = 0;

	if(!nvram_get("boardtype") ||
	   !nvram_get("boardnum") ||
           !nvram_get("boardflags") ||
	   !nvram_get("clkfreq") ||
	   !nvram_get("os_flash_addr") ||
	   !nvram_get("dl_ram_addr") ||
	   !nvram_get("os_ram_addr") ||
	   !nvram_get("scratch") ||
	   !nvram_get("et0macaddr") ||
	   (check_hw_type() != BCM4704_BCM5325F_CHIP && (!nvram_get("vlan0ports") || !nvram_get("vlan0hwname")))) {
		cprintf("ERR, Cann't find some important names, set to factory default!\n");
		mtd_erase("nvram");
		kill(1, SIGTERM);
		exit(0);
	}

	if(check_hw_type() == BCM5325E_CHIP) {
		/* Lower the DDR ram drive strength , the value will be stable for all boards 
		   Latency 3 is more stable for all ddr 20050420 by honor */
		ret += check_nv("sdram_init", "0x010b");
		ret += check_nv("sdram_config", "0x0062");
#if LINKSYS_MODEL == WRT54G
		/* Only for 16M sdram need to override cpu clock */
		ret += check_nv("clkfreq", "216");
#endif
		if(ret) {
			nvram_set("sdram_ncdl", "0x0");
		}
		ret += check_nv("pa0itssit", "62");
		ret += check_nv("pa0b0", "0x15eb");
		ret += check_nv("pa0b1", "0xfa82");
		ret += check_nv("pa0b2", "0xfe66");
		ret += check_nv("pa0maxpwr", "0x4e");
	}
	else if(check_hw_type() == BCM4704_BCM5325F_CHIP) {
		// nothing to do
	}
	else if(check_hw_type() == BCM5352E_CHIP) {
		ret += check_nv("sdram_init", "0x010b");
		ret += check_nv("sdram_config", "0x0062");
		if(ret) {
			nvram_set("sdram_ncdl", "0x0");
		}
		ret += check_nv("pa0itssit", "62");
		ret += check_nv("pa0b0", "0x168b");
		ret += check_nv("pa0b1", "0xfabf");
		ret += check_nv("pa0b2", "0xfeaf");
		ret += check_nv("pa0maxpwr", "0x4e");
		ret += check_nv("vlan0ports", "3 2 1 0 5*");
	}
	else {
		ret += check_nv("pa0itssit", "62");
		ret += check_nv("pa0b0", "0x170c");
		ret += check_nv("pa0b1", "0xfa24");
		ret += check_nv("pa0b2", "0xfe70");
		ret += check_nv("pa0maxpwr", "0x48");
	}

	if(ret) {
		nvram_commit();
		kill(1, SIGTERM);
           	exit(0);
	}

	return ret;
}

static int
check_pmon_nv(void)
{
	return 0;
}

static int
check_image(void)
{
	int ret = 0;
#ifdef EMBEDDED_BOOT_SUPPORT	
	FILE *fp;
	if((fp = fopen("/bin/boot.bin","r"))){
		if(!nvram_get("boot_ver")){
			printf("The boot.bin isn't yet updated, need to update.\n");
			ret = write_boot("/bin/boot.bin","boot");
		}
		else if(strcmp(nvram_safe_get("boot_ver"), BOOT_VERSION) < 0){
			printf("The boot(%s) is older than boot.bin(%s), need to update.\n",nvram_safe_get("boot_ver"),BOOT_VERSION);
			ret = write_boot("/bin/boot.bin","boot");
		}
		else if(strcmp(nvram_safe_get("boot_ver"), BOOT_VERSION) > 0){
			printf("The boot(%s) is newer than boot.bin(%s), not need to update.\n",nvram_safe_get("boot_ver"),BOOT_VERSION);
		}
		else if(strcmp(nvram_safe_get("boot_ver"), BOOT_VERSION) == 0){
			printf("The boot(%s) is equal boot.bin(%s), not need to update.\n",nvram_safe_get("boot_ver"),BOOT_VERSION);
		}
		fclose(fp);
	}
#endif 
	eval("insmod","writemac","flag=get_flash"); //Barry adds for set flash_type 2003 09 08
        nvram_set("firmware_version",CYBERTAN_VERSION);
        nvram_set("Intel_firmware_version",INTEL_FLASH_SUPPORT_VERSION_FROM);
        nvram_set("bcm4712_firmware_version",BCM4712_CHIP_SUPPORT_VERSION_FROM);
	eval("rmmod","writemac");

	return ret;
}

int
main(int argc, char **argv)
{
	char *base = strrchr(argv[0], '/');
	
	base = base ? base + 1 : argv[0];

	/* init */
	if (strstr(base, "init")) {
		main_loop();
		return 0;
	}

	/* Set TZ for all rc programs */
	setenv("TZ", nvram_safe_get("time_zone"), 1);

	/* rc [stop|start|restart ] */
	if (strstr(base, "rc")) {
		if (argv[1]) {
			if (strncmp(argv[1], "start", 5) == 0)
				return kill(1, SIGUSR2);
			else if (strncmp(argv[1], "stop", 4) == 0)
				return kill(1, SIGINT);
			else if (strncmp(argv[1], "restart", 7) == 0)
				return kill(1, SIGHUP);
		} else {
			fprintf(stderr, "usage: rc [start|stop|restart]\n");
			return EINVAL;
		}
	}
	
	/* ppp */
	else if (strstr(base, "ip-up"))
		return ipup_main(argc, argv);
	else if (strstr(base, "ip-down"))
		return ipdown_main(argc, argv);
	else if (strstr(base, "set-pppoepid")) //tallest 1219
                return set_pppoepid_to_nv_main(argc, argv);
        else if (strstr(base, "disconnected_pppoe")) //by tallest 0407
                return disconnected_pppoe_main(argc, argv);
        else if (strstr(base, "ppp_event")) 
                return pppevent_main(argc, argv);

	/* udhcpc [ deconfig bound renew ] */
	else if (strstr(base, "udhcpc"))
		return udhcpc_main(argc, argv);
#ifdef BRCM
	/* ldhclnt [ deconfig bound renew ] */
	else if (strstr(base, "ldhclnt"))
		return udhcpc_lan(argc, argv);

	/* stats [ url ] */
	else if (strstr(base, "stats"))
		return http_stats(argv[1] ? : nvram_safe_get("stats_server"));
#endif
	/* erase [device] */
	else if (strstr(base, "erase")) {
		if (argv[1])
			return mtd_erase(argv[1]);
		else {
			fprintf(stderr, "usage: erase [device]\n");
			return EINVAL;
		}
	}

	/* write [path] [device] */
	else if (strstr(base, "write")) {
		if (argc >= 3)
			return mtd_write(argv[1], argv[2]);
		else {
			fprintf(stderr, "usage: write [path] [device]\n");
			return EINVAL;
		}
	}
#ifdef BACKUP_RESTORE_SUPPORT
	/* restore [path] nvram */
	else if (strstr(base, "restore")) {
		return nvram_restore(argv[1], argv[2]);
	}
#endif
#ifdef WIRELESS_SUPPORT
	/* hotplug [event] */
	else if (strstr(base, "hotplug")) {
		if (argc >= 2) {
			if (!strcmp(argv[1], "net"))
				return hotplug_net();
		} else {
			fprintf(stderr, "usage: hotplug [event]\n");
			return EINVAL;
		}
	}
#endif
	/* rc [stop|start|restart ] */
	else if (strstr(base, "rc")) {
		if (argv[1]) {
			if (strncmp(argv[1], "start", 5) == 0)
				return kill(1, SIGUSR2);
			else if (strncmp(argv[1], "stop", 4) == 0)
				return kill(1, SIGINT);
			else if (strncmp(argv[1], "restart", 7) == 0)
				return kill(1, SIGHUP);
		} else {
			fprintf(stderr, "usage: rc [start|stop|restart]\n");
			return EINVAL;
		}
	}

	//////////////////////////////////////////////////////
	//
	else if (strstr(base, "filtersync"))
		return filtersync_main();
        /* filter [add|del] number */
        else if (strstr(base, "filter")) {
                if (argv[1] && argv[2]) {
                        int num=0;
                        if( (num=atoi(argv[2])) > 0 ){
                                if (strcmp(argv[1], "add") == 0)
                                	{
                                						cprintf("in rc add filter\n");
																return filter_add_new(num);
                                	}
                                else if (strcmp(argv[1], "del") == 0)
                                	{
																	cprintf("in rc del filter\n");
																	return filter_del_new(num);
                                	}
                        }
                }
                else {
        	        fprintf(stderr, "usage: filter [add|del] number\n");
                        return EINVAL;
		}
        }
	else if (strstr(base, "redial"))
		return redial_main(argc, argv);
		
	else if (strstr(base, "resetbutton"))
                return resetbutton_main(argc, argv);

	else if (strstr(base, "write_boot"))
		return write_boot("/tmp/boot.bin","boot");
	
#ifdef DEBUG_IPTABLE
        else if (strstr(base, "iptable_range"))
                return range_main(argc, argv);
        else if (strstr(base, "iptable_rule"))
                return rule_main(argc, argv);
#endif
			
#ifdef HEARTBEAT_SUPPORT
	 else if (strstr(base, "hb_connect"))
                 return hb_connect_main(argc, argv);
         else if (strstr(base, "hb_disconnect"))
                 return hb_disconnect_main(argc, argv);
#endif
	
	else if (strstr(base, "gpio"))
		return gpio_main(argc, argv);
	else if (strstr(base, "listen"))
		return listen_main(argc, argv);
	else if (strstr(base, "check_ps"))
		return check_ps_main(argc, argv);
	else if (strstr(base, "ddns_success"))
		return ddns_success_main(argc, argv);
	else if (strstr(base, "ddns_checkip"))
		return ddns_checkip_main(argc, argv);
        else if (strstr(base, "process_monitor"))
                return process_monitor_main();
#ifdef WL_STA_SUPPORT
        else if (strstr(base, "site_suvery"))
                return site_suvery_main();
#endif
#ifdef EOU_SUPPORT
        else if (strstr(base, "eou_status"))
                return eou_status_main(argc, argv);
#endif
#ifdef DELAY_PING
        else if (strstr(base, "qos"))
                return qos_main(argc, argv);
#endif
	else if (strstr(base, "misc"))
		return misc_main(argc, argv);
	else if (strstr(base, "detectwan"))
		return detectwan_main(argc, argv);
#ifdef SES_SUPPORT
	else if (strstr(base, "sendudp"))
		return sendudp_main(argc, argv);
	else if (strstr(base, "check_ses_led"))
		return check_ses_led_main(argc, argv);
	else if (strstr(base, "ses_led"))
		return ses_led_main(argc, argv);
#endif
								
	return EINVAL;
}
