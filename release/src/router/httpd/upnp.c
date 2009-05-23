/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

void asp_upnpinfo(int argc, char **argv)
{
#ifdef USE_MINIUPNPD
	if (nvram_get_int("upnp_enable")) {
		f_write_string("/etc/upnp/info", "", 0, 0);
		if (killall("miniupnpd", SIGUSR2) == 0) {
			f_wait_notexists("/etc/upnp/info", 5);
		}
	
		web_puts("\nmupnp_data = '");
		web_putfile("/etc/upnp/data.info", WOF_JAVASCRIPT);
		web_puts("';\n");
	}
#else
	unlink("/var/spool/upnp.js");
	if (nvram_get_int("upnp_enable") == 1) {
		if (killall("upnp", SIGUSR2) == 0) {
			f_wait_exists("/var/spool/upnp.js", 5);
		}
	}

	web_puts("\nupnp_data = [\n");
	do_file("/var/spool/upnp.js");
	web_puts("];\n");
	unlink("/var/spool/upnp.js");
#endif
}

void wo_upnp(char *url)
{
#ifdef USE_MINIUPNPD
	char s[256];
	const char *proto;
	const char *eport;

	if (nvram_get_int("upnp_enable")) {
		if (((proto = webcgi_get("remove_proto")) != NULL) && (*proto) &&
			((eport = webcgi_get("remove_eport")) != NULL) && (*eport)) {
			sprintf(s, "%3s %6s\n", proto, eport);
			f_write_string("/etc/upnp/delete", s, 0, 0);
			if (killall("miniupnpd", SIGUSR2) == 0) {
				f_wait_notexists("/etc/upnp/delete", 5);
			}
		}
	}
	common_redirect();
#else
	char s[256];
	const char *proto;
	const char *port;

	if (nvram_get_int("upnp_enable") == 1) {
		if (((proto = webcgi_get("remove_proto")) != NULL) && (*proto) &&
			((port = webcgi_get("remove_ext_port")) != NULL) && (*port)) {

			sprintf(s, "%s %s\n", proto, port);
			f_write_string("/var/spool/upnp.delete", s, 0, 0);
			if (killall("upnp", SIGUSR2) == 0) {
				f_wait_notexists("/var/spool/upnp.delete", 5);
			}
		}
	}
	common_redirect();
#endif
}
