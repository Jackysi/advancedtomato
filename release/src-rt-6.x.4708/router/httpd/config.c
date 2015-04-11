/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <typedefs.h>
#include <sys/reboot.h>

//#define DEBUG

#ifdef DEBUG
#define NVRAMCMD	"/tmp/nvram"
#else
#define NVRAMCMD	"nvram"
#endif

void wo_defaults(char *url)
{
	const char *v;
	int mode;

	if ((v = webcgi_get("mode")) != NULL) {
		mode = atoi(v);
		if ((mode == 1) || (mode == 2)) {
			prepare_upgrade();

			parse_asp("reboot-default.asp");
			web_close();

			// disconnect ppp - need this for PPTP/L2TP/PPPOE to finish gracefully
			killall("xl2tpd", SIGTERM);
			killall("pppd", SIGTERM);

			led(LED_DIAG, 1);
			sleep(2);

			if (mode == 1) {
				//	eval(NVRAMCMD, "defaults", "--yes");
				nvram_set("restore_defaults", "1");
				nvram_commit();
			}
			else {
				eval("mtd-erase2", "nvram");
			}

			set_action(ACT_REBOOT);
			//	kill(1, SIGTERM);
			reboot(RB_AUTOBOOT);
			exit(0);
		}
	}

	redirect("/admin-config.asp");
}

void wo_backup(char *url)
{
	char tmp[64];
	char msg[64];
//	static char *args[] = {
//		NVRAMCMD, "save"
//	};

	char *args[3];
	args[0] = NVRAMCMD;
	args[1] = "save";

	strcpy(tmp, "/tmp/backupXXXXXX");
	mktemp(tmp);
	args[2] = tmp;

	sprintf(msg, ">%s.msg", tmp);

	if (_eval(args, msg, 0, NULL) == 0) {
		eval(args, msg);
		send_header(200, NULL, mime_binary, 0);
		do_file(tmp);
		unlink(tmp);
	}
	else {
		resmsg_fread(msg + 1);
		send_header(200, NULL, mime_html, 0);
		parse_asp("error.asp");
	}

	unlink(msg + 1);
}

void wi_restore(char *url, int len, char *boundary)
{
	char *buf;
	const char *error;
	int ok;
	int n;
	char tmp[64];

	check_id(url);

	tmp[0] = 0;
	buf = NULL;
	error = "Error reading file";
	ok = 0;

	if (!skip_header(&len)) {
		goto ERROR;
	}

	if ((len < 64) || (len > (NVRAM_SPACE * 2))) {
		error = "Invalid file";
		goto ERROR;
	}

	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

	n = web_read(buf, len);
	len -= n;

	strcpy(tmp, "/tmp/restoreXXXXXX");
	mktemp(tmp);
	if (f_write(tmp, buf, n, 0, 0600) != n) {
		error = "Error writing temporary file";
		goto ERROR;
	}

	rboot = 1;
	prepare_upgrade();

	char msg[64];

	char *args[3];
	args[0] = NVRAMCMD;
	args[1] = "restore";
	args[2] = tmp;

	sprintf(msg, ">%s.msg", tmp);

	if (_eval(args, msg, 0, NULL) != 0) {
		resmsg_fread(msg + 1);
	}
	nvram_commit();
#ifndef DEBUG
	unlink(msg + 1);
#endif
	error = NULL;

ERROR:
	free(buf);
	if (error != NULL) resmsg_set(error);
	web_eat(len);
#ifndef DEBUG
	if (tmp[0]) unlink(tmp);
#endif
}


void wo_restore(char *url)
{
	if (rboot) {
		parse_asp("reboot.asp");
		web_close();

		// disconnect ppp - need this for PPTP/L2TP/PPPOE to finish gracefully
		killall("xl2tpd", SIGTERM);
		killall("pppd", SIGTERM);
		sleep(2);

#ifdef DEBUG
		cprintf("---reboot=%d\n", rboot);
#else
		set_action(ACT_REBOOT);
		//	kill(1, SIGTERM);
		sync();
		reboot(RB_AUTOBOOT);
#endif
		exit(0);
	}

	parse_asp("error.asp");
}
