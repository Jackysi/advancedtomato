/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"

void asp_build_time(int argc, char **argv)
{
	web_puts(tomato_buildtime);
}

void asp_version(int argc, char **argv)
{
#if 0
	web_puts(tomato_version);
#else
	if (argc != 0) {
		web_puts(tomato_version);
	}
	else {
		web_write(tomato_version, strstr(tomato_version, "vpn") - tomato_version);
	}
#endif
}

