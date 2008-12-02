
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
#include <wait.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>

#include <ntp.h>
#include <cy_conf.h>
#include <utils.h>

#define NTP_M_TIMER "3600" 
#define NTP_N_TIMER "30" 

extern void timer_cancel(timer_t timerid);

extern int ntp_success;

static int ntp_success_count=1;
/* for NTP */
int do_ntp(void)
{
        char default_servers[] = "209.81.9.7 207.46.130.100 192.36.144.23";
	char servers[100];

        char buf[20],buf2[4],buf4[20];
	int TimeZone;  
        struct timeval tv;
        struct timezone tz;
        struct tm tm;
        int i,j,ret;
	char  startMonth;
        char  endMonth;
        char  diffMonth;
       	float time_zone;

	if(!nvram_match("ntp_enable", "1")) {
		dprintf("Disable NTP Client");
		return 1;
	}

	if(!check_wan_link(0)){
		cprintf("Don't exec ntp\n");
		return 1;
	}

        if (nvram_match("ntp_mode", "manual") && nvram_invmatch("ntp_server", ""))
	       strcpy(servers, nvram_safe_get("ntp_server"));
	else
               strcpy(servers, default_servers);

        char *ntpclient_argv[] = { "ntpclient", "-h", servers, "-l", "-s", "-i", "5", "-c", "1", NULL };
	
        ret = -1;
	ret = _eval(ntpclient_argv, NULL, 20, NULL);
	
	cprintf("return code=%d, from ntpclient\n", ret);

	if(ret == 0)	// Update Successfully
	{
		
		strcpy(buf4,nvram_safe_get("time_zone"));
		strcpy(buf,nvram_safe_get("time_zone"));
		strcpy(buf2,strtok(buf," "));
		time_zone = atof(buf2);
		
		cprintf("\n%s,%s,%s\n",buf,buf2,buf4);
		cprintf("Time update successfully, adjust time. (adjust:%f)\n", time_zone);

   	        gettimeofday(&tv,&tz);
         	tv.tv_sec = tv.tv_sec+ time_zone*3600;
         	settimeofday(&tv,&tz);

		/* DL */
        	gettimeofday(&tv,&tz);
		memcpy(&tm, localtime(&tv.tv_sec), sizeof(struct tm));
	
		//dprintf("\nYear:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	
		//syslog(LOG_INFO,"\nYear:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	

		cprintf("\nYear:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	
		i=0;
		TimeZone=0;;  
		while(strncmp(buf4,tzEntry[i].name,strlen(tzEntry[i].name))!=0) 
			i++;
		//printf("\ntzEntrySize=%d\n",tzEntrySize);
		if(i==tzEntrySize) return -1; /* fail */

		TimeZone=i;
        	startMonth = dstEntry[(int)tzEntry[TimeZone].dstFlag].startMonth;
        	endMonth = dstEntry[(int)tzEntry[TimeZone].dstFlag].endMonth;
        	diffMonth = dstEntry[(int)tzEntry[TimeZone].dstFlag].diffMonth;

		//j=tm.tm_year+1900-2002; /** marcel 2007.02.26 [update new daylight saving time of US and Canada]*/
		j=tm.tm_year+1900-2007; /** marcel 2007.02.26 [update new daylight saving time of US and Canada]*/
 		cprintf("\nTimeZone:%d,i=%d,startm=%d,endmonth=%d,diffm=%d,j=%d\n",TimeZone,i,startMonth,endMonth,diffMonth,j);
 		//syslog(LOG_INFO,"\nTimeZone:%d,i=%d,startm=%d,endmonth=%d,diffm=%d,j=%d\n",TimeZone,i,startMonth,endMonth,diffMonth,j);
 		//dprintf("\nflag:%d,dstBias:%d\n",tzEntry[TimeZone].dstFlag,dstEntry[tzEntry[TimeZone].dstFlag].dstBias);
 		//syslog(LOG_INFO,"\nflag:%d,dstBias:%d\n",tzEntry[TimeZone].dstFlag,dstEntry[tzEntry[TimeZone].dstFlag].dstBias);

 	 //	dprintf("\n,Year:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	
	 	//syslog(LOG_INFO,"\n,Year:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	
		if(atoi(nvram_safe_get("daylight_time"))){
			cprintf("Adjust daylight\n");
  	     		if (tzEntry[TimeZone].dstFlag && (((diffMonth == 0) &&
       			((tm.tm_mon+1 == startMonth && tm.tm_mday >= dstEntry[(int)tzEntry[TimeZone].dstFlag].startDay[j]) || 
			 (tm.tm_mon+1 == endMonth && tm.tm_mday < dstEntry[(int)tzEntry[TimeZone].dstFlag].endDay[j]) || 
			 (tm.tm_mon+1 > startMonth && tm.tm_mon+1 < endMonth))) || 
			 ((diffMonth == 1) && ((tm.tm_mon+1 == startMonth && tm.tm_mday >= dstEntry[(int)tzEntry[TimeZone].dstFlag].startDay[j]) ||						 (tm.tm_mon+1 == endMonth && tm.tm_mday < dstEntry[(int)tzEntry[TimeZone].dstFlag].endDay[j]) || 
			  (tm.tm_mon+1 > startMonth || tm.tm_mon+1 < endMonth)))))
       			{

         			tv.tv_sec=tv.tv_sec+dstEntry[(int)tzEntry[TimeZone].dstFlag].dstBias;
	 			cprintf("\ndstBias:%d\n",dstEntry[(int)tzEntry[TimeZone].dstFlag].dstBias);
       	 			settimeofday(&tv,&tz);
       	 			gettimeofday(&tv,&tz);
	 			memcpy(&tm, localtime(&tv.tv_sec), sizeof(struct tm));
	
	 	 		cprintf("\n,Year:%d,Month:%d,Day:%d,Hour:%d,Min:%d,Sec:%d\n",tm.tm_year+1900,tm.tm_mon+1,tm.tm_mday,tm.tm_hour,tm.tm_min,tm.tm_sec);	
	       		}
		}
		/* firewall.c : synchronize the filter rules by TOD scheduling */
	
		
		nvram_set("timer_interval", NTP_M_TIMER);

	if(ntp_success_count==1)
	{	

		cprintf("eval filtersync ntp successful\n ");
		eval("filtersync");
	
		ntp_success_count=0;
	}
	
	//	cprintf("ntp success ntp_success_count==%d\n",ntp_success_count);
		ntp_success=1;
	} /* get gmt time successfully */
	else
	{
		cprintf("Time update failed\n");
		ntp_success=0;
		nvram_set("timer_interval", NTP_N_TIMER);
	}
        return ret;
}

void ntp_main(timer_t t, int arg)
{
	int ret = 0;
	
	if(check_action() == ACT_IDLE && check_wan_link(0)){	// Don't execute during upgrading
		if(nvram_invmatch("ntp_enable","1"))
			return;	
		stop_ntp();
		ret = do_ntp();
		if(ret == 0 && arg == FIRST){
			cprintf("Cancel first ntp timer\n");
			timer_cancel(t);
		}
	}
	else
		fprintf(stderr, "ntp: nothing to do...\n");
}



int
stop_ntp(void)
{
        int ret = eval("killall","-9", "ntpclient");

        dprintf("done\n");
        return ret;
}

