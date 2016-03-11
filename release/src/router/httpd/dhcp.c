/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/types.h>

void asp_dhcpc_time(int argc, char **argv)
{
	long exp;
	struct sysinfo si;
	long n;
	int r;
	char buf[32];
	char prefix[] = "wanXX";

	if(argc > 0){
		strcpy(prefix, argv[0]); } 
	else{
		strcpy(prefix, "wan"); }

	char expires_file[256];
	memset(expires_file, 0, 256);
	sprintf(expires_file, "/var/lib/misc/dhcpc-%s.expires", prefix);
	
	if (using_dhcpc(prefix)) {
		exp = 0;
		r = f_read_string(expires_file, buf, sizeof(buf));
		if (r > 0) {
			n = atol(buf);
			if (n > 0) {
				sysinfo(&si);
				exp = n - si.uptime;
			}
		}
		web_puts(reltime(buf, exp));
	}
}

void wo_dhcpc(char *url)
{
	char *p;
	char *argv[] = { NULL, NULL };
	int pid;

	if ((p = webcgi_get("exec")) != NULL) {
		if (strcmp(p, "release") == 0){
			argv[0] = "dhcpc-release";
		}
		else if (strcmp(p, "renew") == 0){
			argv[0] = "dhcpc-renew";
		}
		argv[1] = webcgi_get("prefix");
		_eval(argv, NULL, 0, &pid);
	}
	common_redirect();
}


// -----------------------------------------------------------------------------


void wo_dhcpd(char *url)
{
	char *p;

	if ((p = webcgi_get("remove")) != NULL) {
		f_write_string("/var/tmp/dhcp/delete", p, FW_CREATE|FW_NEWLINE, 0666);
		killall("dnsmasq", SIGUSR2);
		f_wait_notexists("/var/tmp/dhcp/delete", 5);
	}
	web_puts("{}");
}
