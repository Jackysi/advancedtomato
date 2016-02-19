/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/

#include "rc.h"

#include <sys/ioctl.h>
#include <wait.h>


// used in keepalive mode (ppp_demand=0)


int start_redial(char *prefix)
{
	stop_redial(prefix);
	char cmd[64];
	sprintf(cmd, "redial %s", prefix);

	xstart(cmd);
	return 0;
}

int stop_redial(char *prefix)
{
	char tmp[100];
	int pid;
	pid = nvram_get_int(strcat_r(prefix, "_ppp_redialpid", tmp));
	if(pid > 1){
		while (kill(pid, SIGKILL) == 0) {
		sleep(1);
		}
	}
	return 0;
}

int redial_main(int argc, char **argv)
{
	int tm;
	int count;
	int proto;
	char c_pid[10];
	char tmp[100];
	memset(c_pid, 0, 10);
	sprintf(c_pid, "%d", getpid());
	char prefix[] = "wanXXXXXXXXXX_";
	if(argc > 1){
		strcpy(prefix, argv[1]); } 
	else{
		strcpy(prefix, "wan"); }

	proto = get_wanx_proto(prefix);
	if (proto == WP_PPPOE || proto == WP_PPP3G || proto == WP_PPTP || proto == WP_L2TP) {
		if (nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp)) != 0) return 0;
	}

	nvram_set(strcat_r(prefix, "_ppp_redialpid", tmp), c_pid);
	tm = nvram_get_int(strcat_r(prefix, "_ppp_redialperiod", tmp)) ? : 30;
	if (tm < 5) tm = 5;
	
	syslog(LOG_INFO, "Started. Time: %d", tm);

	count = 0;
	sleep(10);

	while (1) {
		while (1) {
			sleep(tm);
			if (!check_wanup(prefix)) break;
			count = 0;
		}

#if 0
		long ut;
		char pppdisc_file[256];
		if ((count < 3) && (get_wanx_proto(prefix) == WP_PPPOE) || (get_wanx_proto(prefix) == WP_PPP3G)) {
			memset(pppdisc_file, 0, 256);
			sprintf(pppdisc_file, "/var/lib/misc/%s_pppoe-disc", prefix);
			if (f_read(pppdisc_file, &ut, sizeof(ut)) == sizeof(ut)) {
				ut = (get_uptime() - ut);
				if (ut <= 15) {
					syslog(LOG_INFO, "%s PPPoE reconnect in progress (%ld)", prefix, ut);
					++count;
					continue;
				}
			}
		}
#endif

		if ((!wait_action_idle(10)) || (check_wanup(prefix))) continue;

		if (!nvram_match("action_service", "wan1-restart")
			|| !nvram_match("action_service", "wan2-restart")
#ifdef TCONFIG_MULTIWAN
			|| !nvram_match("action_service", "wan3-restart")
			|| !nvram_match("action_service", "wan4-restart")
#endif
			) {
			syslog(LOG_INFO, "%s down. Reconnecting...", prefix);
			xstart("service", (char *)prefix, "restart"); //Mabye bugs. arctic
			break;
		}
		else {
			syslog(LOG_INFO, "%s down. Reconnect is already in progress...", prefix);
		}
	}
	
	return 0;
}
