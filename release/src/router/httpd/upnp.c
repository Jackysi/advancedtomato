/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"

void asp_upnpinfo(int argc, char **argv)
{
	unlink("/var/spool/upnp.js");
	if (nvram_match("upnp_enable", "1")) {
		if (killall("upnp", SIGUSR2) == 0) {
			wait_file_exists("/var/spool/upnp.js", 5, 0);
		}
	}

	web_puts("\nupnp_data = [\n");
	do_file("/var/spool/upnp.js");
	web_puts("];\n");
	unlink("/var/spool/upnp.js");
}

void wo_upnp(char *url)
{
	char s[256];
	const char *proto;
	const char *port;

	if (nvram_match("upnp_enable", "1")) {
		if (((proto = webcgi_get("remove_ext_proto")) != NULL) && (*proto) &&
			((port = webcgi_get("remove_ext_port")) != NULL) && (*port)) {

			sprintf(s, "%s %s\n", proto, port);
			f_write_string("/var/spool/upnp.delete", s, 0, 0);
			if (killall("upnp", SIGUSR2) == 0) {
				wait_file_exists("/var/spool/upnp.delete", 5, 1);
			}
		}
	}
	common_redirect();
}
