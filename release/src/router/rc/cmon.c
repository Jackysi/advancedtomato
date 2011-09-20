/*

  Tomato Firmware
  Copyright (C) 2006-2008 Jonathan Zarate
  rate limit & connection limit by conanxu


TOASTMAN - client monitor

Files used with the clientmon function:

	httpd/bwm.c			// write to the correct in/out stats display
	httpd/tomato.c
	nvram/defaults.c
	rc/Makefile
	rc/cmon.c (this file)
	rc/rc.h
	rc/services.c
	rc/wan.c
	www/about.asp
	bwm-common.js			// gives names to the imq interfaces
	www/bwm-realtime.asp		





*/

#include "rc.h"
#include <arpa/inet.h>

//#include <sys/stat.h>

static char old_ip_address[16]="none";

void start_cmon(void)
{
	FILE *f;
	char *p;
	char *s = "/tmp/start_cmon.sh";
	char *argv[3];
	int pid;



	//	Is cmon enabled?
	//	This nvram variable should be either 0 (off) or 1 (on)

	if (!nvram_get_int("cmon_enable")) return;

	//read ip address to monitor from nvram
	p = nvram_safe_get("cmon_ipaddr");

	if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f, 
		"#!/bin/sh\n"
	);


	if (strcmp(old_ip_address, "none"))
	{
		fprintf(f, 			
			"iptables -t mangle -D PREROUTING -s %s -j IMQ --todev 3\n"
			"iptables -t mangle -D POSTROUTING -d %s -j IMQ --todev 4\n",
			old_ip_address, old_ip_address
		);
	}

	fprintf(f, 
		"ip link set imq3 up txqueuelen 100\n"
		"ip link set imq4 up txqueuelen 100\n"
		"iptables -t mangle -A PREROUTING -s %s -j IMQ --todev 3\n"
		"iptables -t mangle -A POSTROUTING -d %s -j IMQ --todev 4\n"	
		"\n", p, p
	);

	fclose(f);
	chmod(s, 0700);
	chdir("/tmp");

	argv[0] = s;
	argv[1] = NULL;
	argv[2] = NULL;
	if (_eval(argv, NULL, 0, &pid) != 0) {
		pid = -1;
	}
	else {
		kill(pid, 0);
		strcpy(old_ip_address, p);
	}
      
	chdir("/");
}


/*

This is working!  All above is OK    -    Test the stop now...

*/



void stop_cmon(void)
{
	FILE *f;
	char *s = "/tmp/stop_cmon.sh";
	char *argv[3];
	int pid;

	if (nvram_get_int("cmon_enable")) return;

	if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f,
		"#!/bin/sh\n"
	);
	if (strcmp(old_ip_address, "none"))
	{
		fprintf(f, 
			"iptables -t mangle -D PREROUTING -s %s -j IMQ --todev 3\n"
			"iptables -t mangle -D POSTROUTING -d %s -j IMQ --todev 4\n",
			old_ip_address, old_ip_address);
	}
	fprintf(f,
		"ip link set imq3 down\n"
		"ip link set imq4 down\n"
		"\n"
	);

	fclose(f);
	chmod(s, 0700);
	chdir("/tmp");

	argv[0] = s;
	argv[1] = NULL;
	argv[2] = NULL;
	if (_eval(argv, NULL, 0, &pid) != 0) {
		pid = -1;
	}
	else {
		kill(pid, 0);
		strcpy(old_ip_address, "none");
	}
      
	chdir("/");
}
