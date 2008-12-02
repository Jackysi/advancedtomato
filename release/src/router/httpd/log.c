/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>

static int logok(void)
{
	if (nvram_match("log_file", "1")) return 1;
	resmsg_set("Internal logging disabled");
	redirect("error.asp");
	return 0;
}

void wo_viewlog(char *url)
{
	char *p;
	char *c;
	char s[128];
	char t[128];
	int n;

	if (!logok()) return;
	
	if ((p = webcgi_get("find")) != NULL) {
		send_header(200, NULL, mime_plain, 0);
		if (strlen(p) > 64) return;
		c = t;
		while (*p) {
			switch (*p) {
			case '<':
			case '>':
			case '|':
			case '"':
			case '\\':
				*c++ = '\\';
				*c++ = *p;
				break;
			default:
				if (isprint(*p)) *c++ = *p;
				break;
			}
			++p;
		}
		*c = 0;
		sprintf(s, "cat %s %s | grep -i \"%s\"", "/var/log/messages.0", "/var/log/messages", t);
		web_pipecmd(s, WOF_NONE);
		return;
	}
	
	if ((p = webcgi_get("which")) == NULL) return;
	if (strcmp(p, "all") == 0) {
		send_header(200, NULL, mime_plain, 0);
		do_file("/var/log/messages.0");
		do_file("/var/log/messages");
		return;
	}
	if ((n = atoi(p)) > 0) {
		send_header(200, NULL, mime_plain, 0);
		sprintf(s, "cat %s %s | tail -n %d", "/var/log/messages.0", "/var/log/messages", n);
		web_pipecmd(s, WOF_NONE);
	}
}

void wo_syslog(char *url)
{
	if (!logok()) return;	
	send_header(200, NULL, mime_binary, 0);
	do_file("/var/log/messages.0");
	do_file("/var/log/messages");
}
