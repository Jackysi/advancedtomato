/*

	Tomato Firmware
	PPTP Server Support
	Copyright (C) 2012 Augusto Bott

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

*/

#include "tomato.h"

#ifndef PPTP_CONNECTED
#define PPTP_CONNECTED	"/tmp/pptp_connected"
#endif

#ifndef IF_SIZE
#define IF_SIZE 8
#endif

/*
#ifndef IFNAMSIZ
#define IFNAMSIZ 16
#endif
*/

void asp_pptpd_userol(int argc, char **argv) {
	char comma;
	char line[128];
	FILE *fp;

	char clientusername[32+1];
	char clientlocalip[INET6_ADDRSTRLEN+1];
	char clientremoteip[INET6_ADDRSTRLEN+1];
	char interface[IF_SIZE+1];
	int ppppid;
	int clientuptime;

	web_puts("\n\npptpd_online=[");
	comma = ' ';

	fp = fopen(PPTP_CONNECTED, "r");
	if (fp) {
		while (fgets(line, sizeof(line), fp) != NULL) {
			if (sscanf(line, "%d %s %s %s %s %d", &ppppid, interface, clientlocalip, clientremoteip, clientusername, &clientuptime) != 6) continue;
			web_printf("%c['%d', '%s', '%s', '%s', '%s', '%d']", 
				comma, ppppid, interface, clientlocalip, clientremoteip, clientusername, clientuptime);
			comma = ',';
		}
		fclose(fp);
	}

	web_puts("];\n");
}

void wo_pptpdcmd(char *url) {
	char *p;
	int n = 10;
	// do we really need to output anything?
	web_puts("\npptd_result = [\n");
	if ((p = webcgi_get("disconnect")) != NULL) {
		while ((kill(atoi(p), SIGTERM) == 0) && (n > 1)) {
			sleep(1);
			n--;
		}
	}
	web_puts("];\n");
}

