/*

	Tomato Firmware modified - AKA: ugly hacks
	Copyright (C) 2007 Augusto Bott
	Modified by Tomasz S³odkowicz for SDHC/MMC driver v2.0.0

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
	char s[256];
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
		snprintf(s, sizeof(s), "cs=%s clk=%s din=%s dout=%s", mmc_cs, mmc_cl, mmc_di, mmc_do);
	} else *s=0;
	if (eval("modprobe -s mmc major=121 dbg=1", s) != 0) {
		error("loading module for");
		return;
	}
	if (((mmc_fs_type = nvram_get("mmc_fs_type")) != NULL) && (*mmc_fs_type != 0) &&
    	    ((mmc_part_number = nvram_get("mmc_fs_partition")) != NULL) && (*mmc_part_number != 0)) {
		modprobe(mmc_fs_type);
		snprintf(s, sizeof(s), "/dev/mmc/disc0/part%s", mmc_part_number);
		if (mount(s, "/mmc", mmc_fs_type, MS_NOATIME|MS_NODIRATIME, "") != 0) {
			modprobe_r(mmc_fs_type);
			modprobe_r("mmc");
			error("mounting");
			return;
		}
		if (((mmc_exec_mount = nvram_get("mmc_exec_mount")) != NULL) && (*mmc_exec_mount != 0)) {
			chdir("/mmc");
			xstart(mmc_exec_mount);
			chdir("/");
		}
	}
}

void stop_mmc(void)
{
	if (!wait_action_idle(10)) return;
	eval(nvram_safe_get("mmc_exec_umount"));
	notice_set("mmc", "");
	eval("/bin/sync");
	umount ("/mmc");
	modprobe_r("mmc");
	modprobe_r(nvram_safe_get("mmc_fs_type"));
}

