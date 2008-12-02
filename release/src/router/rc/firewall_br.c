

/*
 *********************************************************
 *   Copyright 2005, CyberTAN  Inc.  All Rights Reserved *
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <net/if.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>

#include <bcmnvram.h>
#include <shutils.h>
#include <rc.h>
#include <code_pattern.h>
#include <utils.h>
#include <cy_conf.h>
#ifdef SYMC_OUTBREAK_SUPPORT
#include <outbreak.h>
#endif
#include <errno.h>

#define EBTABLES_SAVE_FILE	"/tmp/.ebt"

static void save2file(const char *fmt,...)
{
    char buf[10240];
    va_list args;
    FILE *fp;

    if ((fp = fopen(EBTABLES_SAVE_FILE, "a")) == NULL) {
	printf("Can't open /tmp/.ebt\n");
	exit(1);
    }

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args); 
    va_start(args, fmt);
    fprintf(fp, "ebtables %s", buf);
    va_end(args);

    fclose(fp);
}

static void
broute_table(void)
{

}

static void
nat_table(void)
{

}

static void 
filter_input(void)
{
	// for fix wds bug by mark	
	if(nvram_invmatch("wl_macmode", "disabled") && nvram_invmatch("wl_mac_list", "")){
	     if (nvram_match("wl_macmode", "allow")||nvram_match("wl_macmode", "deny"))
	     {
		char *maclist,*if_list,*next;
		char mac[18], wdsifname[18], action[10];

		save2file("-N wds\n");
		maclist = nvram_safe_get("wl_mac_list");
		
		if(nvram_match("wds_ifname",""))
			if_list=nvram_safe_get("wl0_ifname");
		else
			if_list=nvram_safe_get("wds_ifname");
		
		if(nvram_match("wl_macmode", "allow"))
		{
			strcpy(action,"ACCEPT");
		        save2file("-P wds DROP\n");
		}
		else if(nvram_match("wl_macmode", "deny"))
		{
			strcpy(action,"DROP");
		        save2file("-P wds ACCESS\n");
		}    
		
		foreach(wdsifname,if_list, next) 
		{
			save2file("-A INPUT -i %s -j wds\n",wdsifname);
			foreach(mac,maclist , next) 
			{
    				save2file("-A wds -s %s -j %s\n",mac,action);		
			}
		}
	    }
    	 }
} //wds issue ...end
 
	   

static void 
filter_output(void)
{

}

static void 
filter_forward(void)
{
#ifdef SYMC_OUTBREAK_SUPPORT
	if(nvram_match("symc_outbreak", "1") &&
	   nvram_match("symc_block_lanlan", "1")) {
		char *active_dev = nvram_safe_get("symc_active_dev");
		int update;
		int ignore;
		int reason;
		char buf[254], *next;
		char mac[18];
		char lan[10];

		save2file("-N oba_block\n");

		foreach(lan, nvram_safe_get("lan_ifnames"), next) {
			save2file("-A oba_block -o %s -j DROP\n", lan);
		}

		foreach(buf, active_dev, next) {
			sscanf(buf, "%[^,],%d,%d,%d", mac, &update, &ignore, &reason);
			if(Is_Blocked(update, ignore)) {
				save2file("-A FORWARD -s %s -j oba_block\n", mac);
				save2file("-A FORWARD -d %s -j oba_block\n", mac);
			}
		}
	}
#endif
}

static void 
filter_table(void)
{
	save2file("-P INPUT ACCEPT\n");
	save2file("-P FORWARD ACCEPT\n");
	save2file("-P OUTPUT ACCEPT\n");

	filter_input();
	filter_output();
	filter_forward();
}

int
start_br_firewall(void)
{
	int ret;

	unlink(EBTABLES_SAVE_FILE);	

	broute_table();
	nat_table();
	filter_table();

	ret = eval("sh", EBTABLES_SAVE_FILE);

	return ret;	
}

int
stop_br_firewall(void)
{
	int ret;

	unlink(EBTABLES_SAVE_FILE);	

	save2file("-F\n");
	save2file("-Z\n");
	save2file("-X\n");

	ret = eval("sh", EBTABLES_SAVE_FILE);

	return ret;
}
