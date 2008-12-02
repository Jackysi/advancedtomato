#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <shutils.h>
#include <bcmnvram.h>
#include <rc.h>
#include <code_pattern.h>
#include <cy_conf.h>
#include <utils.h>
#include <bcmtimer.h>

#ifdef DEBUG_HEARTBEAT
#define MY_LOG syslog
#define MY_LOG1 eval
#else
#define MY_LOG(fmt, args...)
#define MY_LOG1(fmt, args...)
#endif

#define HB_TIMER	5

extern void timer_cancel(timer_t timerid);

int
start_heartbeat(int status)
{
	FILE *fp;
	int ret;
	char authserver[80];
	char authdomain[80];
	char buf[254];

	if(nvram_invmatch("wan_proto", "heartbeat"))	return 0;
	
	MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", "Start heartbeat daemon");
	MY_LOG(LOG_DEBUG, "heartbeat: hb_server_ip[%s] wan_get_domain[%s]\n", nvram_safe_get("hb_server_ip"), nvram_safe_get("wan_get_domain"));

	/* We must find out HB auth server from domain that get by dhcp if user don't input HB sever. */
	if(nvram_invmatch("hb_server_ip", "") && nvram_invmatch("hb_server_ip", "0.0.0.0")){
		MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", "tallest: Using user keyin HB server IP."); // by tallest debug 0210
		snprintf(authserver, sizeof(authserver), "%s", nvram_safe_get("hb_server_ip"));
		snprintf(authdomain, sizeof(authdomain), "%s", nvram_safe_get("wan_get_domain"));
	}
	else if(nvram_invmatch("wan_get_domain", " ")){            // NSW
		MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", "tallest: Using nsw.bigpond.net.au"); // by tallest debug 0210
		snprintf(authserver, sizeof(authserver), "sm-server.%s", nvram_safe_get("wan_get_domain"));
		snprintf(authdomain, sizeof(authdomain), "%s", nvram_safe_get("wan_get_domain"));
	}else{
		MY_LOG(LOG_DEBUG, "heartbeat: Could not get heartbeat domain.\n");
	}

	MY_LOG(LOG_DEBUG, "heartbeat: authserver[%s] authdomain[%s]\n", authserver, authdomain);
	
	snprintf(buf, sizeof(buf), "%s%c%s", authserver, !strcmp(authdomain,"") ? '\0' : '.', authdomain);
	nvram_set("hb_server_name", buf);
	
	MY_LOG(LOG_DEBUG, "heartbeat: Connect to HB server [%s]\n", buf);
	
	//syslog(LOG_DEBUG, "heartbeat: authserver[%s] authdomain[%s]\n", authserver, authdomain);
	if (!(fp = fopen("/tmp/bpalogin.conf", "w"))) {
		MY_LOG(LOG_ERR, "heartbeat: Cann't write %s\n", "/tmp/bpalogin.conf");
		perror("/tmp/bpalogin.conf");
		return errno;
	}
	fprintf(fp, "username %s\n", nvram_safe_get("ppp_username"));
	fprintf(fp, "password %s\n", nvram_safe_get("ppp_passwd"));
	fprintf(fp, "authserver %s\n", authserver);
	//fprintf(fp, "%cauthdomain %s\n", strcmp(authdomain,"") ? '\0' : '#', authdomain);
	fprintf(fp, "authdomain %s\n", authdomain);
	fprintf(fp, "localport 5050\n");
	fprintf(fp, "logging stdout\n");
	fprintf(fp, "debuglevel 4\n");
	fprintf(fp, "minheartbeatinterval 60\n");
	fprintf(fp, "maxheartbeatinterval 840\n");
	fprintf(fp, "connectedprog hb_connect\n"); 
	fprintf(fp, "disconnectedprog hb_disconnect\n");
	
	fclose(fp);
	
	mkdir("/tmp/ppp", 0777);
	if((fp = fopen("/tmp/hb_connect_success", "r"))){
		ret = eval("bpalogin", "-c", "/tmp/bpalogin.conf", "-t");
		fclose(fp);	
	}
	else
		ret = eval("bpalogin", "-c", "/tmp/bpalogin.conf");
	
	if (nvram_invmatch("ppp_demand", "1")) {
		if(status != REDIAL)
			start_redial();	
	}

	return ret;
}

int
stop_heartbeat(void)
{
	int ret;
	
	unlink("/tmp/ppp/link");
	ret = eval("killall", "bpalogin");
	ret += eval("killall", "-9", "bpalogin");
	
	dprintf("done\n");
	
	return ret;
}

/*
 *  Called when link comes up
 *  argv[1] : listenport
 *  argv[2] : pid
 */
int
hb_connect_main(int argc, char **argv)
{
	FILE *fp;
	char buf[254];
	
	MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", "hb_connect_main(): init");
	
	mkdir("/tmp/ppp", 0777);

	if (!(fp = fopen("/tmp/ppp/link", "a"))) {
                perror("/tmp/ppp/link");
                return errno;
        }
	fprintf(fp, "%s", argv[2]);
        fclose(fp);
	
	start_wan_done(nvram_safe_get("wan_ifname"));
	
	snprintf(buf, sizeof(buf), "iptables -I INPUT -d %s -i %s -p udp --dport %d -j %s", 
		   nvram_safe_get("wan_ipaddr"), 
		   nvram_safe_get("wan_ifname"),
		   5050, 
		   "ACCEPT");
	
	MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", buf);

	system(buf);

	return TRUE;	
}

/*
 *  * Called when link goes down
 *   */
int
hb_disconnect_main(int argc, char **argv)
{
	MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", "hb_disconnect_main(): init");

	if(check_wan_link(0))
	{
		MY_LOG1("logger", "-p", "daemon.debug", "-t", "bpalogin[]", "tallest: Stop_wan in hb_disconnect_main function");// by tallest debug 0210
		stop_heartbeat();
	}
	return 0;
}

int total_time = 0;
int last_num = 0;

void hb_main(timer_t t, int arg)
{
	FILE *fp;
	int num;
	char line[80];

	if(check_action() != ACT_IDLE)
		return ;
	
	if((fp = popen("iptables -L -n -v | grep lan2wan", "r"))){
		fgets(line, sizeof(line), fp);
		sscanf(line, "%d", &num);
		//cprintf("total_time=%d last_num=%d num=%d\n", total_time, last_num, num);

		if(num == last_num)
			total_time = total_time + HB_TIMER;
		else
			total_time = 0;
			
		last_num = num;

		if(total_time >= atoi(nvram_safe_get("ppp_idletime"))*60) {
			cprintf("The link is idle for %d seconds\n", atoi(nvram_safe_get("ppp_idletime"))*60);
			timer_cancel(t);	
			stop_heartbeat();
			stop_ntp();
			eval("listen", nvram_safe_get("lan_ifname"));
		}
		pclose(fp);	
	}
}
