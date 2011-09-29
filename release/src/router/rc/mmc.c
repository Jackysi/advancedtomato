/*

	Tomato Firmware modified - AKA: ugly hacks
	Copyright (C) 2007 Augusto Bott
	Modified by Tomasz S³odkowicz for SDHC/MMC driver v2.0.1

*/

#include "rc.h"

#include <sys/mount.h>

static void error(const char *message)
{
	char s[512];

	snprintf(s, sizeof(s), "Error %s SDHC/MMC. Check the logs to see if they contain more details about this error.", message);
	notice_set("mmc", s);
}

void start_mmc(void)
{
	char s[32];
	char p[4][10];
	const char *mmc_cs;
	const char *mmc_cl;
	const char *mmc_di;
	const char *mmc_do;
	const char *mmc_fs_type;
	const char *mmc_part_number;
	const char *mmc_exec_mount;
 
	if (!nvram_match("mmc_on", "1")) {
		notice_set("mmc", "");
		return;
	}
	if (((mmc_cs = nvram_get("mmc_cs")) != NULL) && (*mmc_cs != 0) &&
	    ((mmc_cl = nvram_get("mmc_clk")) != NULL) && (*mmc_cl != 0) &&
            ((mmc_di = nvram_get("mmc_din")) != NULL) && (*mmc_di != 0) &&
	    ((mmc_do = nvram_get("mmc_dout")) != NULL) && (*mmc_do != 0)) {
		snprintf(p[0], sizeof(p[0]), "cs=%s", mmc_cs);
		snprintf(p[1], sizeof(p[1]), "clk=%s", mmc_cl);
		snprintf(p[2], sizeof(p[2]), "din=%s", mmc_di);
		snprintf(p[3], sizeof(p[3]), "dout=%s", mmc_do);
	} else *p[0]=*p[1]=*p[2]=*p[3]=0;
	if (eval("modprobe", "-s", "mmc", "major=121", "dbg=1", p[0], p[1], p[2], p[3]) != 0) {
		error("loading module for");
		return;
	}
	if (((mmc_fs_type = nvram_get("mmc_fs_type")) != NULL) && (*mmc_fs_type != 0) &&
    	    ((mmc_part_number = nvram_get("mmc_fs_partition")) != NULL) && (*mmc_part_number != 0)) {
		if (modprobe(mmc_fs_type) != 0 ) {
			modprobe_r("mmc");
			error("loading filesystem module for");
			return;
		}
		snprintf(s, sizeof(s), "/dev/mmc/disc0/part%s", mmc_part_number);
		if (mount(s, "/opt", mmc_fs_type, MS_NOATIME|MS_NODIRATIME, "") != 0) {
			modprobe_r(mmc_fs_type);
			modprobe_r("mmc");
			error("mounting");
			return;
		}
		if (((mmc_exec_mount = nvram_get("mmc_exec_mount")) != NULL) && (*mmc_exec_mount != 0)) {
			chdir("/opt");
			xstart(mmc_exec_mount);
			chdir("/");
		}
	}
	notice_set("mmc", "");
}

void stop_mmc(void)
{
	if (!wait_action_idle(10)) return;
	eval(nvram_safe_get("mmc_exec_umount"));
	notice_set("mmc", "");
	eval("/bin/sync");
	if (umount ("/opt") != 0) {
		error("unmounting");
		return;
	} else {
		modprobe_r("mmc");
		modprobe_r(nvram_safe_get("mmc_fs_type"));
	}
}

