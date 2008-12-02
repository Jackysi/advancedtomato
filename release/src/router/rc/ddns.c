
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
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <syslog.h>
#include <sys/wait.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <utils.h>
#include <rc.h>

char service[10];
char disable_ip[20];
char _username[] = "ddns_username_X";
char _passwd[] = "ddns_passwd_X";
char _hostname[] = "ddns_hostname_X";
int get_public_ip(void);

int
init_ddns(void)
{
	int flag = 0;

	if(nvram_match("ddns_enable","0")) {			// disable from ui or default
		if(nvram_match("ddns_enable_buf","1")){		// before disable is dyndns, so we want to disable dyndns
			if(!nvram_get("ddns_service"))
				strcpy(service,"dyndns");
			else
				strcpy(service, nvram_safe_get("ddns_service"));
			strcpy(disable_ip,"192.168.1.1");	// send this address to disable dyndns
			flag = 1;
		}
		else if(nvram_match("ddns_enable_buf","2")){	// before disable is tzo, so we want to disable tz
			strcpy(service,"tzo");
			strcpy(disable_ip,"0.0.0.0");
			flag = 2;
		}
		else return -1;					// default 
	}
	else if(nvram_match("ddns_enable","1")){
		if(!nvram_get("ddns_service"))
			strcpy(service,"dyndns");
		else
			strcpy(service, nvram_safe_get("ddns_service"));
		flag = 1;
	}
	else if(nvram_match("ddns_enable","2")){
		strcpy(service,"tzo");
		flag = 2;
	}

	if(flag == 1){
		snprintf(_username, sizeof(_username),"%s","ddns_username");
		snprintf(_passwd, sizeof(_passwd),"%s","ddns_passwd");
		snprintf(_hostname, sizeof(_hostname),"%s","ddns_hostname");
	}
	else{
		snprintf(_username, sizeof(_username),"ddns_username_%d", flag);
		snprintf(_passwd, sizeof(_passwd),"ddns_passwd_%d", flag);
		snprintf(_hostname, sizeof(_hostname),"ddns_hostname_%d", flag);
	}

	return 0;
}

int
start_ddns(void)
{
	int ret;
	FILE *fp;
	pid_t pid;
	char string[80]="";

	/* Get correct username, password and hostname */
	if(init_ddns() < 0)
		return -1;

	/* We don't want to update, if user don't input below field */
	if(nvram_match(_username,"")||
	   nvram_match(_passwd,"")||
	   nvram_match(_hostname,""))
		return -1;

	
	/* We want to re-update if user change some value from UI */
//	if(strcmp(nvram_safe_get("ddns_enable_buf"),nvram_safe_get("ddns_enable")) ||	// ddns mode change
//            strcmp(nvram_safe_get("ddns_username_buf"),nvram_safe_get(_username)) ||	// ddns username chane
//            strcmp(nvram_safe_get("ddns_passwd_buf"),nvram_safe_get(_passwd)) ||	// ddns password change
//            strcmp(nvram_safe_get("ddns_hostname_buf"),nvram_safe_get(_hostname))){  // ddns hostname change
//	    cprintf("Some value had been changed , need to update\n");
	    
	    if(nvram_match("action_service", "ddns") || !file_to_buf("/tmp/ddns_msg", string, sizeof(string))
                ){
		    cprintf("Upgrade from UI or first time\n");
		    nvram_unset("ddns_cache");	// The will let program to re-update
		    unlink("/tmp/ddns_msg");	// We want to get new message
	    }
//	}

	/* Some message we want to stop to update */
	if(file_to_buf("/tmp/ddns_msg", string, sizeof(string))){
		cprintf("string=[%s]\n", string);
		if(strcmp(string, "") && 
		   !strstr(string, "_good") && 
		   !strstr(string, "noupdate") &&
		   !strstr(string, "nochg") &&
		   !strstr(string, "all_")){
			cprintf("Last update have error message : %s, don't re-update\n", string);
			return -1;
		}
	}

	if(nvram_match("ddns_enable", "0") && nvram_invmatch("action_service", "ddns"))
		return -1;
	
	/* Generate ddns configuration file */
       	if ((fp = fopen("/tmp/ddns.conf", "w"))) {
     		fprintf(fp, "service-type=%s\n",service); 
     	  	fprintf(fp, "user=%s:%s\n",nvram_safe_get(_username),nvram_safe_get(_passwd)); 
       		fprintf(fp, "host=%s\n",nvram_safe_get(_hostname)); 

		if(nvram_match("ddns_enable","0")){
			fprintf(fp, "address=%s\n",disable_ip);	// send error ip address
		}
		else {
			if(nvram_match("ddns_enable","2")) {
				//if(nvram_match("public_ip", "") || first_time())	
					get_public_ip();
	       			fprintf(fp, "address=%s\n",nvram_safe_get("public_ip")); 
			}
			else
                        {
                             /******** modify by zg 2006.11.14 for cdrouter v3.3 dyndns module bugs in pptp mode ********/
                             if(nvram_match("wan_proto","pptp"))
                             {
                                 fprintf(fp, "address=%s\n",nvram_safe_get("pptp_get_ip"));
                             }
                             else
                             {
	       			fprintf(fp, "address=%s\n",nvram_safe_get("wan_ipaddr")); 
                             }
                             /******** end by zg 2006.11.14 for cdrouter v3.3 dyndns module bugs in pptp mode ********/
                        }
		}

		if(nvram_match("ddns_enable", "1")) {	// For DynDNS
			if(nvram_get("ddns_mx") && !nvram_match("ddns_mx", ""))
				fprintf(fp, "mx=%s\n", nvram_safe_get("ddns_mx"));
	
			if(nvram_get("ddns_backmx") && nvram_match("ddns_backmx", "YES"))
				fprintf(fp, "backmx=YES\n");
			else
				fprintf(fp, "backmx=NO\n");

			if(nvram_get("ddns_wildcard") && nvram_match("ddns_wildcard","ON"))
				fprintf(fp, "wildcard\n");
		}

		fclose(fp);
	}
	else{
        	perror("/tmp/ddns.conf");
        	return -1;
	}

	/* Restore cache data to file */
	if(nvram_invmatch("ddns_enable", ""))	
		nvram2file("ddns_cache", "/tmp/ddns.cache");
	
	{
		char *argv[] = {"ez-ipupdate",
		      //"-i", nvram_safe_get("wan_ifname"),
		      "-D",
		      //"-P", "3600",
		      "-e", "ddns_success",
		      "-c", "/tmp/ddns.conf",
		      "-b", "/tmp/ddns.cache", 
		      NULL };

		ret = _eval(argv, ">/dev/console", 0, &pid);
	}


	dprintf("done\n");
	
	return ret;
}

int
stop_ddns(void)
{
        int ret;
	
        ret = eval("killall","-9","ez-ipupdate");

        dprintf("done\n");

        return ret;
}

int
ddns_success_main(int argc, char *argv[])
{
	char buf[80];
	
	init_ddns();

	snprintf(buf, sizeof(buf), "%ld,%s", time(NULL), argv[1]);
	cprintf("DDNS update successfully, save [%s] to ddns_cache\n", buf);
	
	nvram_set("ddns_cache", buf);
	nvram_set("ddns_status", "1");
	nvram_set("ddns_enable_buf", nvram_safe_get("ddns_enable"));
	nvram_set("ddns_username_buf", nvram_safe_get(_username));
	nvram_set("ddns_passwd_buf", nvram_safe_get(_passwd));
	nvram_set("ddns_hostname_buf", nvram_safe_get(_hostname));
	nvram_set("ddns_change", "");

	if(nvram_match("ddns_enable","2"))
	{	
	//	buf_to_file("/tmp/ddns_msg", "tzo_good");
	}
	nvram_commit();

	dprintf("done\n");

	return 0;
}

int
get_public_ip(void)
{
	int ret = 0;

	ret = system("/sbin/ddns_checkip -t tzo-echo -n public_ip -w 3 -d");
	// 0: Success
	// -1: Failure
	printf("ret=%d\n", ret);

	if(ret == 0) {
		char *ddns_cache = nvram_safe_get("ddns_cache");
		char *public_ip = nvram_safe_get("public_ip");		
		char ip[20];
		char buf[20];

		sscanf(ddns_cache, "%*d,%s", ip);
		printf("ip=[%s] public_ip=[%s]\n", ip, public_ip);
		if(strcmp(ip, public_ip))	// Different public ip
			return 1;

		file_to_buf("/tmp/ddns_msg", buf, sizeof(buf));	
		printf("buf=[%s]\n", buf);
		if(!strcmp(buf, "dyn_strange")) {
			printf("buf=[%s]\n", buf);
			return 1;
		}
	}

	return ret;
}

void 
ddns_check_main(timer_t t, int arg)
{
	if(check_action() == ACT_IDLE && check_wan_link(0)) {	// Don't execute during upgrading
		if(nvram_match("ddns_enable", "2")) {	// Only for TZO
			if(get_public_ip() == 1) {
				nvram_set("action_service","ddns");
                        	kill(1,SIGUSR1);
			}
		}
	}
}
