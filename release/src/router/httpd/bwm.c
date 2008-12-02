/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"

#include <sys/stat.h>

/*
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <typedefs.h>
*/

static const char *hfn = "/var/lib/misc/rstats-history.gz";

void wo_bwmbackup(char *url)
{
	struct stat st;
	time_t t;
	int i;
	
	if (stat(hfn, &st) == 0) {
		t = st.st_mtime;
		sleep(1);
	}
	else {
		t = 0;
	}
	killall("rstats", SIGHUP);
	for (i = 10; i > 0; --i) {
		if ((stat(hfn, &st) == 0) && (st.st_mtime != t)) break;
		sleep(1);
	}
	if (i == 0) {
		send_error(500, NULL, NULL);
		return;
	}
	send_header(200, NULL, mime_binary, 0);
	do_file((char *)hfn);
}

void wi_bwmrestore(char *url, int len, char *boundary)
{
	char *buf;
	const char *error;
	int ok;
	int n;
	char tmp[64];
	
	check_id();
	
	tmp[0] = 0;
	buf = NULL;
	error = "Error reading file";
	ok = 0;

	if (!skip_header(&len)) {
		goto ERROR;
	}

	if ((len < 64) || (len > 10240)) {
		goto ERROR;
	}

	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

	n = web_read(buf, len);
	len -= n;

	sprintf(tmp, "%s.new", hfn);
	if (f_write(tmp, buf, n, 0, 0600) != n) {
		unlink(tmp);
		error = "Error writing temporary file";
		goto ERROR;
	}
	f_write("/var/tmp/rstats-load", NULL, 0, 0, 0600);
	killall("rstats", SIGHUP);
	sleep(1);

	error = NULL;
	rboot = 1;	// used as "ok"

ERROR:
	free(buf);
	web_eat(len);
	if (error != NULL) resmsg_set(error);
}

void wo_bwmrestore(char *url)
{
	if (rboot) {
		redirect("/bwm-daily.asp");
	}
	else {
		parse_asp("error.asp");
	}
}

void asp_netdev(int argc, char **argv)
{
	FILE *f;
	char buf[256];
	unsigned long rx, tx;
	char *p;
	char *ifname;
	char comma;
	char *exclude;
	
	exclude = nvram_safe_get("rstats_exclude");
	web_puts("\n\nnetdev={");
	if ((f = fopen("/proc/net/dev", "r")) != NULL) {
		fgets(buf, sizeof(buf), f);	// header
		fgets(buf, sizeof(buf), f);	// "
		comma = ' ';
		while (fgets(buf, sizeof(buf), f)) {
			if ((p = strchr(buf, ':')) == NULL) continue;
			*p = 0;
			if ((ifname = strrchr(buf, ' ')) == NULL) ifname = buf;
				else ++ifname;
//			if (strncmp(ifname, "ppp", 3) == 0) ifname = "ppp";
			if ((strcmp(ifname, "lo") == 0) || (find_word(exclude, ifname))) continue;
			
			// <rx bytes, packets, errors, dropped, fifo errors, frame errors, compressed, multicast><tx ...>
			if (sscanf(p + 1, "%lu%*u%*u%*u%*u%*u%*u%*u%lu", &rx, &tx) != 2) continue;
			web_printf("%c'%s':{rx:0x%lx,tx:0x%lx}", comma, ifname, rx, tx);
			comma = ',';
		}
		fclose(f);
	}
	web_puts("};\n");
}
