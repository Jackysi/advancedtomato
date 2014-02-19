/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/wait.h>
#include <typedefs.h>
#include <sys/reboot.h>

#if 1
#define MTD_WRITE_CMD	"mtd-write2"
#else
#define DEBUG_TEST
#define MTD_WRITE_CMD	"/tmp/mtd-write"
#endif

void prepare_upgrade(void)
{
	int n;

	// stop non-essential stuff & free up some memory
	exec_service("upgrade-start");
	for (n = 30; n > 0; --n) {
		sleep(1);
		if (nvram_match("action_service", "")) break;	// this is cleared at the end
	}
	unlink("/var/log/messages");
	unlink("/var/log/messages.0");
	sync();
}

void wi_upgrade(char *url, int len, char *boundary)
{
	uint8 buf[1024];
	const char *error = "Error reading file";
	int ok = 0;
	int n;
	int reset;

	check_id(url);
	reset = (strcmp(webcgi_safeget("_reset", "0"), "1") == 0);

#ifdef TCONFIG_JFFS2
	// quickly check if JFFS2 is mounted by checking if /jffs/ is not squashfs
	struct statfs sf;
	if ((statfs("/jffs", &sf) != 0) || (sf.f_type != 0x71736873 && sf.f_type != 0x73717368)) {
		error = "JFFS2 is currently in use. Since an upgrade may overwrite the "
			"JFFS2 partition, please backup the contents, disable JFFS2, then reboot the router";
		goto ERROR;
	}
#endif

	// skip the rest of the header
	if (!skip_header(&len)) goto ERROR;

	if (len < (1 * 1024 * 1024)) {
		error = "Invalid file";
		goto ERROR;
	}

	// -- anything after here ends in a reboot --

	rboot = 1;

	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);

	prepare_upgrade();
	system("cp reboot.asp /tmp");	// copy to memory

	led(LED_DIAG, 1);

	char fifo[] = "/tmp/flashXXXXXX";
	int pid = -1;
	FILE *f = NULL;

	if ((mktemp(fifo) == NULL) ||
		(mkfifo(fifo, S_IRWXU) < 0)) {
		error = "Unable to create a fifo";
		goto ERROR2;
	}

	char *wargv[] = { MTD_WRITE_CMD, fifo, "linux", NULL };
	if (_eval(wargv, ">/tmp/.mtd-write", 0, &pid) != 0) {
		error = "Unable to start flash program";
		goto ERROR2;
	}

	if ((f = fopen(fifo, "w")) == NULL) {
		error = "Unable to start pipe for mtd write";
		goto ERROR2;
	}

	// !!! This will actually write the boundary. But since mtd-write
	// uses trx length... -- zzz

	while (len > 0) {
		 if ((n = web_read(buf, MIN(len, sizeof(buf)))) <= 0) {
			 goto ERROR2;
		 }
		 len -= n;
		 if (safe_fwrite(buf, 1, n, f) != n) {
			 error = "Error writing to pipe";
			 goto ERROR2;
		 }
	}

	error = NULL;
	ok = 1;

ERROR2:
	rboot = 1;

	if (f) fclose(f);
	if (pid != -1) waitpid(pid, &n, 0);

	if (error == NULL && reset) {
		set_action(ACT_IDLE);
		eval("mtd-erase2", "nvram");
	}
	set_action(ACT_REBOOT);

	if (resmsg_fread("/tmp/.mtd-write"))
		error = NULL;
ERROR:
	if (error) resmsg_set(error);
	web_eat(len);
}

void wo_flash(char *url)
{
	if (rboot) {
		parse_asp("/tmp/reboot.asp");
		web_close();

#ifdef DEBUG_TEST
		printf("\n\n -- reboot -- \n\n");
		set_action(ACT_IDLE);
#else
		// disconnect ppp - need this for PPTP/L2TP/PPPOE to finish gracefully
		killall("xl2tpd", SIGTERM);
		killall("pppd", SIGTERM);

		sleep(2);
		//	kill(1, SIGTERM);
		reboot(RB_AUTOBOOT);
#endif
		exit(0);
	}

	parse_asp("error.asp");
}
