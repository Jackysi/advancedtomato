/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "rc.h"

#include <sys/mount.h>
#include <sys/stat.h>
#include <errno.h>

//	#define TEST_INTEGRITY


static void error(const char *message)
{
	char s[512];

	snprintf(s, sizeof(s), "Error %s JFFS2. Check the logs to see if they contain more details about this error.", message);
	notice_set("jffs2", s);
}

void start_jffs2(void)
{
	if (!nvram_match("jffs2_on", "1")) {
		notice_set("jffs2", "");
		return;
	}

	int format = 0;
	char s[256];
	int size;
	int part;
	const char *p;
	
	if (!wait_action_idle(10)) return;

	if (!mtd_getinfo("jffs2", &part, &size)) return;
	
	if (nvram_match("jffs2_format", "1")) {
		nvram_set("jffs2_format", "0");

		if (!mtd_erase("jffs2")) {
			error("formatting");
			return;
		}
		
		format = 1;
	}
	
	sprintf(s, "%d", size);
	p = nvram_get("jffs2_size");	
	if ((p == NULL) || (strcmp(p, s) != 0)) {
		if (format) {
			nvram_set("jffs2_size", s);
			nvram_commit();
		}
		else if ((p != NULL) && (*p != 0)) {
			error("verifying known size of");
			return;
		}
	}

	if (!mtd_unlock("jffs2")) {
		error("unlocking");
		return;
	}

	modprobe("jffs2");

	sprintf(s, "/dev/mtdblock/%d", part);
	if (mount(s, "/jffs", "jffs2", MS_NOATIME|MS_NODIRATIME, "") != 0) {
		modprobe_r("jffs2");
		error("mounting");
		return;
	}

#ifdef TEST_INTEGRITY
	int test;
			
	if (format) {
		if (f_write("/jffs/.tomato_do_not_erase", &size, sizeof(size), 0, 0) != sizeof(size)) {
			stop_jffs2();
			error("setting integrity test for");
			return;
		}
	}
			
	if ((f_read("/jffs/.tomato_do_not_erase", &test, sizeof(test)) != sizeof(test)) || (test != size)) {
		stop_jffs2();
		error("testing integrity of");
		return;
	}
#endif
	
	notice_set("jffs2", format ? "Formatted." : "");

	if (((p = nvram_get("jffs2_exec")) != NULL) && (*p != 0)) {
		chdir("/jffs");
		xstart(p);
		chdir("/");
	}
}

void stop_jffs2(void)
{
	if (!wait_action_idle(10)) return;

	notice_set("jffs2", "");
	umount("/jffs");
	modprobe_r("jffs2");
}
