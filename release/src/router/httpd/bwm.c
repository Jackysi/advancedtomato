/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <sys/stat.h>
#include <sys/ioctl.h>

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

	check_id(url);

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
	int sfd;
	struct ifreq ifr;

	exclude = nvram_safe_get("rstats_exclude");
	web_puts("\n\nnetdev={");
	if ((f = fopen("/proc/net/dev", "r")) != NULL) {
		fgets(buf, sizeof(buf), f);	// header
		fgets(buf, sizeof(buf), f);	// "
		comma = ' ';

		if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
			_dprintf("[%s %d]: error opening socket %m\n", __FUNCTION__, __LINE__);
		}

		while (fgets(buf, sizeof(buf), f)) {
			if ((p = strchr(buf, ':')) == NULL) continue;
			*p = 0;
			if ((ifname = strrchr(buf, ' ')) == NULL) ifname = buf;
				else ++ifname;
//			if (strncmp(ifname, "ppp", 3) == 0) ifname = "ppp";
			if ((strcmp(ifname, "lo") == 0) || (find_word(exclude, ifname))) continue;

			// skip down interfaces
			if (sfd >= 0) {
				strcpy(ifr.ifr_name, ifname);
				if (ioctl(sfd, SIOCGIFFLAGS, &ifr) != 0) continue;
				if ((ifr.ifr_flags & IFF_UP) == 0) continue;
			}

			// <rx bytes, packets, errors, dropped, fifo errors, frame errors, compressed, multicast><tx ...>
			if (sscanf(p + 1, "%lu%*u%*u%*u%*u%*u%*u%*u%lu", &rx, &tx) != 2) continue;
			if (!strcmp(ifname, "imq1"))
				web_printf("%c'%s':{rx:0x0,tx:0x%lx}", comma, ifname, rx, tx);
			else if (!strcmp(ifname, "imq2"))
				web_printf("%c'%s':{rx:0x%lx,tx:0x0}", comma, ifname, rx, tx);
			else if (!strcmp(ifname, "imq3"))
				web_printf("%c'%s':{rx:0x0,tx:0x%lx}", comma, ifname, rx, tx);
			else if (!strcmp(ifname, "imq4"))
				web_printf("%c'%s':{rx:0x%lx,tx:0x0}", comma, ifname, rx, tx);
			else
				web_printf("%c'%s':{rx:0x%lx,tx:0x%lx}", comma, ifname, rx, tx);
				
			comma = ',';
		}

		if (sfd >= 0) close(sfd);
		fclose(f);
	}
	web_puts("};\n");
}

void asp_bandwidth(int argc, char **argv)
{
	char *name;
	int sig;

	if ((nvram_get_int("rstats_enable") == 1) && (argc == 1)) {
		if (strcmp(argv[0], "speed") == 0) {
			sig = SIGUSR1;
			name = "/var/spool/rstats-speed.js";
		}
		else {
			sig = SIGUSR2;
			name = "/var/spool/rstats-history.js";
		}
		unlink(name);
		killall("rstats", sig);
		f_wait_exists(name, 5);
		do_file(name);
		unlink(name);
	}
}
