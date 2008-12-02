
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
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <syslog.h>
#include <sys/stat.h>
#include <time.h>

#include <shutils.h>
#include <code_pattern.h>
#include <typedefs.h>
#include <bcmnvram.h>
#include <rc.h>
#include <cy_conf.h>
#include <utils.h>

#ifdef HSIAB_SUPPORT
extern int get_monitor_ip_session_time(void);
extern int get_conf_update_interval(void);
extern int get_stats_update_interval(void);
#endif
extern void ntp_main(timer_t t, int arg);
#ifdef HEARTBEAT_SUPPORT
extern void hb_main(timer_t t, int arg);
#endif
extern void do_redial(timer_t t, int arg);
extern int do_ntp(void);
extern void ddns_check_main(timer_t t, int arg);

extern void init_event_queue(int n);
extern int timer_connect(timer_t timerid, void (*routine)(timer_t, int), int arg);

#define NTP_M_TIMER 3600
#define NTP_N_TIMER 30 
#define HB_TIMER    5
#define DDNS_CHECK_TIMER 600
//#define DDNS_CHECK_TIMER 400

#ifdef HSIAB_SUPPORT
void 
configmon(timer_t t, int arg)
{
	start_configmon();
}

void 
statsmon(timer_t t, int arg)
{
	start_statsmon();
}

void 
monitor_ip(timer_t t, int arg)
{
	start_monitor_ip();
}

int
get_conf_update_interval(void)
{
	int default_update_interval = 300;
	int update_time;

	update_time = atoi(nvram_safe_get("hsiab_conf_time"));

	if(!update_time)    update_time = default_update_interval;

	cprintf("Every %d seconds to retrieve config\n", update_time);

	return update_time;
}

int
get_stats_update_interval(void)
{
	int default_update_interval = 300;
	int update_time;
	
	update_time = atoi(nvram_safe_get("hsiab_stats_time"));

	if(!update_time)    update_time = default_update_interval;

	cprintf("Every %d seconds to send stats\n", update_time);

	return update_time;
}

int
get_monitor_ip_idle_time(void)
{
	int default_idle_time = 60;
	int idle_time = 0;

	idle_time = atoi(nvram_safe_get("hsiab_idle_time"));

	if(!idle_time)
		idle_time = default_idle_time;

	return idle_time;
}
int
get_monitor_ip_session_time(void)
{
	int default_session_time = 60;
	int session_time = 0;
	
	session_time = atoi(nvram_safe_get("hsiab_session_time"));

	if(!session_time)
		session_time = default_session_time;

	cprintf("Every %d seconds to check session\n", session_time);
	
	return session_time;
}
#endif
int
process_monitor_main(void)
{
	struct itimerspec t4, t5, t6, t7;
        timer_t ntp1_id, ntp2_id, hb_id, ddns_id;
	int time;

        init_event_queue(40);
#if 0
	if(nvram_match("wan_proto", "pppoe") || nvram_match("wan_proto", "pptp") || nvram_match("wan_proto", "heartbeat")){
		time = atoi(nvram_safe_get("ppp_redialperiod"));
		if(!time || time < 20)
			time = 20;
	
	        memset(&t6, 0, sizeof(t6));
	        t6.it_interval.tv_sec = time;
	        t6.it_value.tv_sec = time;

	        timer_create(CLOCK_REALTIME, NULL, (timer_t *) &redial_id);
	        timer_connect(redial_id,  do_redial, FIRST);
	        timer_settime(redial_id, 0, &t5, NULL);
			
	}
#endif	
	if(check_wan_link(0)){
		/* init ntp timer */
		if(do_ntp() != 0){
			cprintf("Last update failed, we need to re-update after %d seconds\n", NTP_N_TIMER);
			time = NTP_N_TIMER;
	
		        memset(&t4, 0, sizeof(t4));
			t4.it_interval.tv_sec = time;
		        t4.it_value.tv_sec = time;

		        timer_create(CLOCK_REALTIME, NULL, (timer_t *) &ntp1_id);
		        timer_connect(ntp1_id,  ntp_main, FIRST);
		        timer_settime(ntp1_id, 0, &t4, NULL);
		}

		cprintf("We need to re-update after %d seconds\n", NTP_M_TIMER);
		
		time = NTP_M_TIMER;
	
	        memset(&t5, 0, sizeof(t5));
	        t5.it_interval.tv_sec = time;
	        t5.it_value.tv_sec = time;

	        timer_create(CLOCK_REALTIME, NULL, (timer_t *) &ntp2_id);
	        timer_connect(ntp2_id,  ntp_main, SECOND);
	        timer_settime(ntp2_id, 0, &t5, NULL);

		//=================================================================================

		if(nvram_get("ddns_check_time"))
			time = atoi(nvram_safe_get("ddns_check_time"));
		else
			time = DDNS_CHECK_TIMER;
	
	        memset(&t7, 0, sizeof(t7));
	        t7.it_interval.tv_sec = time;
	        t7.it_value.tv_sec = time;

	        timer_create(CLOCK_REALTIME, NULL, (timer_t *) &ddns_id);
	        timer_connect(ddns_id,  ddns_check_main, 0);
	        timer_settime(ddns_id, 0, &t7, NULL);

		//=================================================================================

#ifdef HEARTBEAT_SUPPORT
		if(nvram_match("wan_proto", "heartbeat") && nvram_match("ppp_demand", "1")) {
			time = HB_TIMER;

	                memset(&t6, 0, sizeof(t6));
	                t6.it_interval.tv_sec = time;
	                t6.it_value.tv_sec = time;

	                timer_create(CLOCK_REALTIME, NULL, (timer_t *) &hb_id);
	                timer_connect(hb_id,  hb_main, 0);
	                timer_settime(hb_id, 0, &t6, NULL);
		}
#endif														
#ifdef HSIAB_SUPPORT
	/* init monitor_ip timer */
		if(nvram_match("hsiab_mode", "1") && 
		   nvram_invmatch("hsiab_provider", "0") && 
		   nvram_match("hsiab_registered", "1") &&
		   is_exist("/var/run/hsiabd.pid")){
			struct itimerspec t1, t2, t3, t4, t5;
			timer_t session_id, config1_id, config2_id, stats_id, wl_id;
			    
			time = get_monitor_ip_session_time();
			cprintf("The idle time is %d\n", get_monitor_ip_idle_time());

			memset(&t1, 0, sizeof(t1));
			t1.it_interval.tv_sec = time;
			t1.it_value.tv_sec = time;

			timer_create(CLOCK_REALTIME, NULL, (timer_t *) &session_id);
			timer_connect(session_id,  monitor_ip, (int) NULL);
			timer_settime(session_id, 0, &t1, NULL);
#if 0
			/* init configmon timer */
			time = 5;
			memset(&t2, 0, sizeof(t2));
			t2.it_interval.tv_sec = time;
			t2.it_value.tv_sec = time;

			timer_create(CLOCK_REALTIME, NULL, (timer_t *) &config1_id);
			timer_connect(config1_id,  configmon, FIRST);
			timer_settime(config1_id, 0, &t2, NULL);
#endif			

			time = get_conf_update_interval();
	
			memset(&t3, 0, sizeof(t3));
			t3.it_interval.tv_sec = time;
			t3.it_value.tv_sec = time;

			timer_create(CLOCK_REALTIME, NULL, (timer_t *) &config2_id);
			timer_connect(config2_id,  configmon, SECOND);
			timer_settime(config2_id, 0, &t3, NULL);

			/* init configmon timer */
			time = get_stats_update_interval();
	
			memset(&t4, 0, sizeof(t4));
			t4.it_interval.tv_sec = time;
			t4.it_value.tv_sec = time;

			timer_create(CLOCK_REALTIME, NULL, (timer_t *) &stats_id);
			timer_connect(stats_id,  statsmon, (int) NULL);
			timer_settime(stats_id, 0, &t4, NULL);
#if 0			
			/* init monitor_wl timer */
			time = 2;
	
			memset(&t5, 0, sizeof(t5));
			t5.it_interval.tv_sec = time;
			t5.it_value.tv_sec = time;

			timer_create(CLOCK_REALTIME, NULL, (timer_t *) &wl_id);
			timer_connect(wl_id,  monitor_wl, (int) NULL);
			timer_settime(wl_id, 0, &t5, NULL);
#endif
		}
#endif
	}

	while(1){
		sleep(3600);
	}

        return 1;
}

int
start_process_monitor(void)
{
	pid_t pid;
#ifdef THROUGHPUT_TEST_SUPPORT
        if(nvram_match("throughput_test","1"))
                return 0;
#endif
	char *argv[] = {"process_monitor", NULL};
	int ret = _eval(argv, NULL, 0, &pid);

	dprintf("done");

	return ret;
}

int
stop_process_monitor(void)
{
	int ret;
	
	ret = eval("killall","process_monitor");
	
	dprintf("done\n");

	return ret;
}
