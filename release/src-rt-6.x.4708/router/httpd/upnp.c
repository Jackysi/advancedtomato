/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

void asp_upnpinfo(int argc, char **argv)
{
	if (nvram_get_int("upnp_enable")) {
		f_write_string("/etc/upnp/info", "", 0, 0);
		if (killall("miniupnpd", SIGUSR2) == 0) {
			f_wait_notexists("/etc/upnp/info", 5);
		}
	
		web_puts("\nmupnp_data = '");
		web_putfile("/etc/upnp/data.info", WOF_JAVASCRIPT);
		web_puts("';\n");
	}
}

void wo_upnp(char *url)
{
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
}
