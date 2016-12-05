/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"

#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <typedefs.h>
#include <sys/reboot.h>

//	#define DEBUG

#ifdef DEBUG
#define NVRAMCMD	"/tmp/nvram"
#else
#define NVRAMCMD	"nvram"
#endif

void wi_uploadsplash(char *url, int len, char *boundary)
{
	char *buf;
	char *p;
	const char *error;
	int ok;
	int n;
	char tmp[255];

//	check_id();


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
	n = n - strlen(boundary)-6;
	syslog(LOG_INFO, "boundary %s, len %d", boundary, strlen(boundary));
	if ((p = nvram_get("NC_DocumentRoot")) == NULL) p = "/tmp/splashd";
	sprintf(tmp, "%s/splash.html", p);
	if (f_write(tmp, buf, n, 0, 0600) != n) {
		error = "Error writing temporary file";
		goto ERROR;
	}

	nvram_set_file("NC_SplashFile", tmp, 8192);
	nvram_commit();
	rboot = 1;

#ifndef DEBUG
#endif
	error = NULL;

ERROR:
	free(buf);
	if (error != NULL) resmsg_set(error);
	web_eat(len);
}

void wo_uploadsplash(char *url)
{
        if (rboot) {
                redirect("/#advanced-splashd.asp");
		exit(0);
        }
        else {
              parse_asp("error.asp");
        }
}
