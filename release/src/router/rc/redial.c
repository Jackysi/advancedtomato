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


int start_redial(void)
{
	stop_redial();

	xstart("redial");
	return 0;
}

int stop_redial(void)
{
	while (killall("redial", SIGKILL) == 0) {
		sleep(1);
	}
	return 0;
}

int redial_main(int argc, char **argv)
{
	int tm;
	int count;
	int proto;

	proto = get_wan_proto();
	if (proto == WP_PPPOE || proto == WP_PPP3G || proto == WP_PPTP || proto == WP_L2TP) {
		if (nvram_get_int("ppp_demand") != 0) return 0;
	}

	tm = nvram_get_int("ppp_redialperiod") ? : 30;
	if (tm < 5) tm = 5;
	
	syslog(LOG_INFO, "Started. Time: %d", tm);

	count = 0;
	sleep(10);

	while (1) {
		while (1) {
			sleep(tm);
			if (!check_wanup()) break;
			count = 0;
		}

#if 0
		long ut;
		if ((count < 3) && (get_wan_proto() == WP_PPPOE) || (get_wan_proto() == WP_PPP3G)) {
			if (f_read("/var/lib/misc/pppoe-disc", &ut, sizeof(ut)) == sizeof(ut)) {
				ut = (get_uptime() - ut);
				if (ut <= 15) {
					syslog(LOG_INFO, "PPPoE reconnect in progress (%ld)", ut);
					++count;
					continue;
				}
			}
		}
#endif

		if ((!wait_action_idle(10)) || (check_wanup())) continue;

		if (!nvram_match("action_service", "wan-restart")) {
			syslog(LOG_INFO, "WAN down. Reconnecting...");
			xstart("service", "wan", "restart");
			break;
		}
		else {
			syslog(LOG_INFO, "WAN down. Reconnect is already in progress...");
		}
	}
	
	return 0;
}
