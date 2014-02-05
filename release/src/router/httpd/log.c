/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

/* Max number of log lines for GUI to display */
#define MAX_LOG_LINES	2000

static int logok(void)
{
	if (nvram_match("log_file", "1")) return 1;
	resmsg_set("Internal logging disabled");
	redirect("error.asp");
	return 0;
}

/* Figure out & return the logfile name. */
void get_logfilename(char *lfn)
{
	char *p;
	char cfg[256];
	char *nv;

	nv = "/var/log/messages";
	if (f_read_string("/etc/syslogd.cfg", cfg, sizeof(cfg)) > 0) {
		if ((p = strchr(cfg, '\n')))
			*p = 0;
		strtok(cfg, " \t");	// skip rotsize
		strtok(NULL, " \t");	// Skip backup cnt
		if ((p = strtok(NULL, " \t")) && (*p == '/')) {
			// check if we can write to the file
			if (f_write(p, cfg, 0, FW_APPEND, 0) >= 0) {
				nv = p; // nv is the configured log filename
			}
		}
	}
	if (lfn)
		strcpy(lfn, nv);
}

void wo_viewlog(char *url)
{
	char *p;
	char *c;
	char s[128];
	char t[128];
	int n;
	char lfn[256];

	if (!logok()) return;

	get_logfilename(lfn);
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
		sprintf(s, "grep -ih \"%s\" $(ls -1rv %s %s.*)", t, lfn, lfn);
		web_pipecmd(s, WOF_NONE);
		return;
	}

	if ((p = webcgi_get("which")) == NULL) return;

	if (strcmp(p, "all") == 0)
		n = MAX_LOG_LINES;
	else if ((n = atoi(p)) <= 0)
		return;

	send_header(200, NULL, mime_plain, 0);
	sprintf(s, "cat $(ls -1rv %s %s.*) | tail -n %d", lfn, lfn, n);
	web_pipecmd(s, WOF_NONE);
}

static void webmon_list(char *name, int webmon, int resolve, unsigned int maxcount)
{
	FILE *f;
	char s[512], ip[64], val[256];
	char *js, *jh;
	char comma;
	unsigned long time;
	unsigned int i;
	char host[NI_MAXHOST];

	web_printf("\nwm_%s = [", name);

	if (webmon) {
		sprintf(s, "/proc/webmon_recent_%s", name);
		if ((f = fopen(s, "r")) != NULL) {
			comma = ' ';
			i = 0;
			while ((!maxcount || i++ < maxcount) && fgets(s, sizeof(s), f)) {
				if (sscanf(s, "%lu\t%s\t%s", &time, ip, val) != 3) continue;
				jh = NULL;
				if (resolve) {
					if (resolve_addr(ip, host) == 0)
						jh = js_string(host);
				}
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

void wo_webmon(char *url)
{
	nvram_set("log_wmclear", webcgi_get("clear"));
	exec_service("firewall-restart");
	nvram_unset("log_wmclear");
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
	char lfn[256];
	char s[128];

	get_logfilename(lfn);
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
		sprintf(s, "cat $(ls -1rv %s %s.*)", lfn, lfn);
		web_pipecmd(s, WOF_NONE);
	}
}
