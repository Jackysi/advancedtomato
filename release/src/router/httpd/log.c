/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

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

static void webmon_list(char *name, int webmon, int resolve, unsigned int maxcount)
{
	FILE *f;
	char s[512], ip[16], val[256];
	char *js, *jh;
	struct hostent *he;
	struct in_addr ia;
	char comma;
	unsigned long time;
	unsigned int i;

	web_printf("\nwm_%s = [", name);

	if (webmon) {
		sprintf(s, "/proc/webmon_recent_%s", name);
		if ((f = fopen(s, "r")) != NULL) {
			comma = ' ';
			i = 0;
			while ((!maxcount || i++ < maxcount) && fgets(s, sizeof(s), f)) {
				if (sscanf(s, "%lu %15s %s", &time, ip, val) != 3) continue;
				if (resolve) {
					ia.s_addr = inet_addr(ip);
					he = gethostbyaddr(&ia, sizeof(ia), AF_INET);
					jh = js_string(he ? he->h_name : "");
				} else
					jh = NULL;
				js = utf8_to_js_string(val);
				web_printf("%c['%lu','%s','%s', '%s']", comma,
					time, ip, js ? : "", jh ? : "");
				free(js);
				free(jh);
				comma = ',';
			}
			fclose(f);
		}
	}

	web_puts("];\n");
}

void asp_webmon(int argc, char **argv)
{
	int webmon = nvram_get_int("log_wm");
	int maxcount = (argc > 0) ? atoi(argv[0]) : 0;
	int resolve = (argc > 1) ? atoi(argv[1]) : 0;

	webmon_list("domains", webmon, resolve, maxcount);
	webmon_list("searches", webmon, resolve, maxcount);
}

static int webmon_ok(int searches)
{
	if (nvram_get_int("log_wm") && nvram_get_int(searches ? "log_wmsmax" : "log_wmdmax") > 0) return 1;
	resmsg_set("Web Monitoring disabled");
	redirect("error.asp");
	return 0;
}

void wo_syslog(char *url)
{
	if (strncmp(url, "webmon_", 7) == 0) {
		// web monitor
		char file[64];
		snprintf(file, sizeof(file), "/proc/%s", url);
		if (!webmon_ok(strstr(url, "searches") != NULL)) return;
		send_header(200, NULL, mime_binary, 0);
		do_file(file);
	}
	else {
		// syslog
		if (!logok()) return;
		send_header(200, NULL, mime_binary, 0);
		do_file("/var/log/messages.0");
		do_file("/var/log/messages");
	}
}
