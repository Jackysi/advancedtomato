/*

  Tomato Firmware
  Copyright (C) 2006-2008 Jonathan Zarate
  rate limit & connection limit by conanxu
*/

#include "rc.h"
#include <arpa/inet.h>

//#include <sys/stat.h>

// read nvram into files
void start_arpbind(void)
{
	FILE *f;
	char *p, *q, *e;
	char *ipaddr;//ip address
	char *macaddr;//mac address
	char *s = "/tmp/start_arpbind.sh";
	char *argv[3];
	int pid;
	int i;
	char lan[24];
	const char *router_ip;
	int host[256];
	char buf_arp[512];
	char ipbuf[32];
	int ipn, length;

	//arpbind is enabled?
	if (!nvram_get_int("arpbind_enable")) return;

	//read static dhcp list from nvram
	p = nvram_safe_get("dhcpd_static");

	//read arpbind_list into file 
	if ((f = fopen(s, "w")) == NULL) return;
	fprintf(f,
		"#!/bin/sh\n"
		"for HOST in `cat /proc/net/arp |sed -n 's/\\([0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*\\).*/\\1/p'`; do arp -d $HOST; done\n"
	);
	memset(host, 0, sizeof(host));

	//get network ip prefix
	router_ip = nvram_safe_get("lan_ipaddr");
	strlcpy(lan, router_ip, sizeof(lan));
	if ((p = strrchr(lan, '.')) != NULL) {
	host[atoi(p+1)] = 1;
	*p = '\0';
	}

	// 00:aa:bb:cc:dd:ee<123<xxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 53 w/ delim
	// 00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 85 w/ delim
	// 00:aa:bb:cc:dd:ee,00:aa:bb:cc:dd:ee<123.123.123.123<xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx.xyz> = 106 w/ delim
	p = nvram_safe_get("dhcpd_static");
	while ((e = strchr(p, '>')) != NULL) {
		length = (e - p);
		if (length > 105) {
			p = e + 1;
			continue;
		}

		strncpy(buf_arp, p, length);
		buf_arp[length] = 0;
		p = e + 1;

		/* get the MAC address */
		if ((e = strchr(buf_arp, '<')) == NULL) continue;
		*e = 0;
		ipaddr = e + 1;
		macaddr = buf_arp;
		if ((e = strchr(macaddr, ',')) != NULL){
			*e = 0;
		}
		//cprintf ("mac address %s\n", macaddr);

		/* get the IP adddres */
		if ((e = strchr(ipaddr, '<')) == NULL) continue;
		*e = 0;
		if (strchr(ipaddr, '.') == NULL) {
			ipn = atoi(ipaddr);
			if ((ipn <= 0) || (ipn > 255)) continue;
			sprintf(ipbuf, "%s%d", lan, ipn);
			ipaddr = ipbuf;
		}
		else {
			if (inet_addr(ipaddr) == INADDR_NONE) continue;
		}
		//cprintf ("ip address %s\n", ipaddr);

		/* add static arp */ 
		if ((*macaddr != 0) && (strcmp(macaddr, "00:00:00:00:00:00") != 0)) {
			fprintf(f, "arp -s %s %s\n", ipaddr, macaddr);
			//cprintf ("arp -s %s %s\n", ipaddr, macaddr);
			if ((q = strrchr(ipaddr, '.')) != NULL) {
				*q = '\0';
				if (!strcmp(ipaddr, lan)) host[atoi(q+1)] = 1;
			}
		}
	}

	if (nvram_get_int("arpbind_only")) {
		for (i = 1; i < 255; i++) {
			if (!host[i]) {
				fprintf(f, "arp -s %s.%d 00:00:00:00:00:00\n", lan, i);
			}
		}
	}

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
	}
      
	chdir("/");
}

void stop_arpbind(void)
{
	FILE *f;
	char *s = "/tmp/stop_arpbind.sh";
	char *argv[3];
	int pid;

	if (nvram_get_int("arpbind_enable")) return;

	if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f,
		"#!/bin/sh\n"
		"for HOST in `cat /proc/net/arp |sed -n 's/\\([0-9]*\\.[0-9]*\\.[0-9]*\\.[0-9]*\\).*/\\1/p'`; do arp -d $HOST; done\n"
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
	}
      
	chdir("/");
}
