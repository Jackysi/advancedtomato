
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
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wait.h>
#include <syslog.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <utils.h>
#include <code_pattern.h>
#include <rc.h>
#include <cy_conf.h>

struct mon {
        char *name;		// Process name
	int count;		// Process conut, 0 means don't check
	int type;		// LAN or WAN
	int (*stop)(void);	// stop function
	int (*start)(void);	// start function
};

enum {M_LAN, M_WAN};

struct mon mons[] = {
	{ "tftpd",	1,	M_LAN,	stop_tftpd,		start_tftpd},
	{ "process_monitor",	1,	M_WAN,	stop_process_monitor,		start_process_monitor},
	{ "httpd",	2,	M_LAN,	stop_httpd,		start_httpd},
	//{ "udhcpd",	1,	M_LAN,	stop_dhcpd,		start_dhcpd},
	//{ "dnsmasq",	1,	M_LAN,	stop_dns,		start_dns},
};

int
search_process(char *name, int count)
{
	int *pidList = NULL;
	int c = 0;

	pidList = find_all_pid_by_ps(name);
	if(pidList && *pidList > 0){
	    for(; pidList && *pidList!=0; pidList++) {
		dprintf("Find %s which pid is %d\n", name, *pidList);
		c ++;
	    }
	}

	if(!c){
		cprintf("Cann't find %s\n", name);
		return 0;
	}
	else{
		dprintf("Find %s which count is %d\n", name, c);
		//if(count && c != count){
		//	cprintf("%s count is not match\n", name);
		//	return 0;
		//}
		//else
			return 1;
	}
}

int
do_mon(void)
{
	struct mon *v;
	
	for(v = mons ; v < &mons[sizeof(mons)/sizeof(mons[0])] ; v++) {
		if(v->type == M_WAN)
			if(!check_wan_link(0))	continue;

		if(!search_process(v->name, v->count)){
			ct_logger(LOG_INFO, "system[%d]: Maybe %s had died, we need to re-exec it\n", getpid(), v->name);
			cprintf("Maybe %s had died, we need to re-exec it\n", v->name);
			if(v->stop)
				v->stop();
			sleep(1);
			if(v->start)
				v->start();
		}
   	}

	return 1;
}

int
check_ps_main(int argc, char**argv)
{
	pid_t pid;

	if(check_action() != ACT_IDLE){	// Don't execute during upgrading
		cprintf("check_ps: nothing to do...\n");
		return 1;
	}

	pid = fork();
	switch(pid)	
	{
		case -1:
			perror("fork failed");
			exit(1);
			break;
		case 0:
			do_mon();
			exit(0);
			break;
		default:
			_exit(0);
			break;
	}
}
