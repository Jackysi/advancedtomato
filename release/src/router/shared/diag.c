
/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

 This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
 the contents of this file may not be disclosed to third parties,
 copied or duplicated in any form without the prior written
 permission of CyberTAN Inc.

 This software should be used as a reference only, and it not
 intended for production use!


 THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
 KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
 SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/socket.h>

#include <broadcom.h>


int
diag_ping_start(webs_t wp)
{
	int ret = 0;
	char *ip = websGetVar(wp, "ping_ip", NULL);
	char *times = websGetVar(wp, "ping_times", NULL);
	
	if(!ip || !times || !strcmp(ip,""))	return ret;

	if(strcmp(times, "0") &&
           strcmp(times, "5") &&
           strcmp(times, "10")) {
		return ret;
	}

	unlink(PING_TMP);
	nvram_set("ping_ip",ip);
	nvram_set("ping_times",times);
	
	// The web will hold, so i move to service.c
	//snprintf(cmd, sizeof(cmd), "ping -c %s %s &", times, ip);
	//cprintf("cmd=[%s]\n",cmd);
	//system(cmd);
	
	return ret;
}

int
diag_ping_stop(webs_t wp)
{
	return eval("killall","-9","ping");
}

int
diag_ping_clear(webs_t wp)
{
	return unlink(PING_TMP);
}

int 
ping_onload(webs_t wp, char *arg)
{
	int ret = 0;
	int pid;	
	char *type = websGetVar(wp, "submit_type", "");

	pid = find_pid_by_ps("ping");

	if(pid>0 && strncmp(type, "stop", 4)){	// pinging
		ret += websWrite(wp, arg);	
	}

	return ret;
}

int
diag_traceroute_start(webs_t wp)
{
	int ret = 0;
	char *ip = websGetVar(wp, "traceroute_ip", NULL);

	if(!ip || !strcmp(ip,""))	return ret;

	unlink(TRACEROUTE_TMP);
	nvram_set("traceroute_ip",ip);

	return ret;
}

int
diag_traceroute_stop(webs_t wp)
{
	return eval("killall","-9","traceroute");
}

int
diag_traceroute_clear(webs_t wp)
{
	return unlink(TRACEROUTE_TMP);
}

int 
traceroute_onload(webs_t wp, char *arg)
{
	int ret = 0;
	int pid;	
	char *type = websGetVar(wp, "submit_type", "");

	pid = find_pid_by_ps("traceroute");

	if(pid>0 && strncmp(type, "stop", 4)){	// tracerouting
		ret += websWrite(wp, arg);	
	}

	return ret;
}

int
ej_dump_ping_log(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret=0, count=0;
	FILE *fp;
	char line[254];

	if ((fp = fopen(PING_TMP, "r")) != NULL) {		// show result
		while( fgets(line, sizeof(line), fp) != NULL ) {
			line[strlen(line)-1] = '\0';
			if(!strcmp(line,""))	continue;
			ret += websWrite(wp,"%c\"%s\"\n",count ? ',' : ' ', line);
			count ++;
		}
		fclose(fp);
	}

	return ret;
}

int
ej_dump_traceroute_log(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret=0, count=0;
	FILE *fp;
	char line[254];

	if ((fp = fopen(TRACEROUTE_TMP, "r")) != NULL) {		// show result
		while( fgets(line, sizeof(line), fp) != NULL ) {
			line[strlen(line)-1] = '\0';
			if(!strcmp(line,""))	continue;
			ret += websWrite(wp,"%c\"%s\"\n",count ? ',' : ' ', line);
			count ++;
		}
		fclose(fp);
	}

	return ret;
}
