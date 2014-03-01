/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>


static int check_addr(const char *addr, int max)
{
	const char *p;
	char c;

	if ((addr == NULL) || (addr[0] == 0)) return 0;
	p = addr;
	while (*p) {
		c = *p;
		if ((!isalnum(c)) && (c != '.') && (c != '-') && (c != ':')) return 0;	//Give IPv6 address a chance
		++p;
	}
	return((p - addr) <= max);
}

void wo_trace(char *url)
{
	char cmd[256];
	const char *addr;

	addr = webcgi_get("addr");
	if (!check_addr(addr, 64)) return;

	killall("traceroute", SIGTERM);

	web_puts("\ntracedata = '");
	sprintf(cmd, "traceroute -I -m %u -w %u %s", atoi(webcgi_safeget("hops", "0")), atoi(webcgi_safeget("wait", "0")), addr);
	web_pipecmd(cmd, WOF_JAVASCRIPT);
	web_puts("';");
}

void wo_ping(char *url)
{
	char cmd[256];
	const char *addr;

	addr = webcgi_get("addr");
	if (!check_addr(addr, 64)) return;

	killall("ping", SIGTERM);

	web_puts("\npingdata = '");
	sprintf(cmd, "ping -c %d -s %d %s", atoi(webcgi_safeget("count", "0")), atoi(webcgi_safeget("size", "0")), addr);
	web_pipecmd(cmd, WOF_JAVASCRIPT);
	web_puts("';");
}


/*
#include <regex.h>

int main(int argc, char **argv)
{
	FILE *f;
	char s[1024];
	int n;
	char domain[512];
	char ip[32];
	char min[32];
	regex_t re;
	regmatch_t rm[10];
	int i;

	if ((f = popen("traceroute -I 192.168.0.1", "r")) == NULL) {
		perror("popen");
		return 1;
	}
// 2  192.168.0.1 (192.168.0.1)  1.908 ms  1.812 ms  1.688 ms

	while (fgets(s, sizeof(s), f)) {

		//
		if (regcomp(&re, "^ +[0-9]+ +(.+?) +\\((.+?)\\) +(.+?) ms +(.+?) ms +(.+?) ms", REG_EXTENDED) != 0) {
			printf("error: regcomp\n");
			return 1;
		}
		if ((regexec(&re, s, sizeof(rm) / sizeof(rm[0]), rm, 0) == 0) && (re.re_nsub == 5)) {
			printf("[");
			for (i = 1; i < 6; ++i) {
				s[rm[i].rm_eo] = 0;
				printf("'%s'%c", s + rm[i].rm_so, (i == 5) ? ' ' : ',');
			}
			printf("]\n");
//			printf("%d = %d = [%s]\n", i, rm[i].rm_so, s + rm[i].rm_so);
		}
		regfree(&re);

//		sscanf(s, "%d %s (%s) %s ms", &n, domain, ip, min);
//		printf("[%s] %s %s\n", ip, domain, min);
	}
	pclose(f);

	return 0;
}

*/
