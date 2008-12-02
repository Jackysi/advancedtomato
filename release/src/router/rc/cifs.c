/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "rc.h"

#include <sys/mount.h>
#include <sys/stat.h>


void start_cifs(void)
{
	xstart("mount-cifs", "-m");
}

void stop_cifs(void)
{
	killall("mount-cifs", SIGTERM);
	eval("mount-cifs", "-u");
}

int mount_cifs_main(int argc, char *argv[])
{
	char s[512];
	char opt[512];
	char mpath[32];
	int i, j;
	int try;
	int first;
	char *on, *unc, *user, *pass, *dom, *exec;
	int done[3];
	char *exargv[3];
	int pid;

	if (argc == 2) {
		if (strcmp(argv[1], "-m") == 0) {
			done[1] = 0;
			done[2] = 0;
			first = 1;
			for (try = 60; try > 0; --try) {
				for (i = 1; i <= 2; ++i) {
					if (done[i]) continue;

					done[i] = 2;

					sprintf(s, "cifs%d", i);
					strlcpy(s, nvram_safe_get(s), sizeof(s));
					if ((vstrsep(s, "<", &on, &unc, &user, &pass, &dom, &exec) != 6) || (*on != '1')) continue;

					done[i] = 0;

					if (first) {
						notice_set("cifs", "Mounting...");
						modprobe("cifs");
						first = 0;
					}

					j = sprintf(opt, "sep=<unc=%s", unc);
					if (*user) j += sprintf(opt + j, "<user=%s", user);
					if (*pass) j += sprintf(opt + j, "<pass=%s", pass);
					if (*dom) j += sprintf(opt + j, "<dom=%s", dom);

					sprintf(mpath, "/cifs%d", i);
					umount(mpath);
					if (mount("-", mpath, "cifs", MS_NOATIME|MS_NODIRATIME, opt) != 0) continue;
					done[i] = 1;

					if (*exec) {
						chdir(mpath);
						exargv[0] = exec;
						exargv[1] = NULL;
						_eval(exargv, NULL, 0, &pid);
					}
				}
				if ((done[1]) && (done[2])) break;
				sleep(2);
			}

			s[0] = 0;
			for (i = 1; i <= 2; ++i) {
				if (done[i] == 0) sprintf(s + strlen(s), "Error mounting CIFS %d. ", i);
			}
			notice_set("cifs", s);
			return 1;
		}
		if (strcmp(argv[1], "-u") == 0) {
			for (i = 1; i <= 2; ++i) {
				sprintf(mpath, "/cifs%d", i);
				umount(mpath);
			}
			modprobe_r("cifs");
			notice_set("cifs", "");
			return 0;
		}
	}

	usage_exit(argv[0], "-m|-u");
	return 1;
}
