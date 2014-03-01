/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include <epivers.h>
#include "tomato.h"

void asp_build_time(int argc, char **argv)
{
	web_puts(tomato_buildtime);
}

void asp_version(int argc, char **argv)
{
#if 0
	if (argc != 0) {
		web_puts(tomato_version);
	}
	else {
		web_write(tomato_version, strrchr(tomato_version, '.') - tomato_version);
	}
#else
	if (argc != 0) {
		switch (atoi(argv[0])) {
		case 2:
			// kernel version
			web_pipecmd("uname -r", WOF_NONE);
			break;
		case 3:
			// wl driver version
			web_puts(EPI_VERSION_STR);
			break;
		default:
			// tomato version
			web_puts(tomato_version);
			break;
		}
	}
	else {
		web_puts(tomato_shortver);
	}
#endif
}

