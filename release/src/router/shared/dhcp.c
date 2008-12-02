
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

#define DHCP_MAX_COUNT 254

/* Dump leases in <tr><td>hostname</td><td>MAC</td><td>IP</td><td>expires</td></tr> format */
int
ej_dumpleases(int eid, webs_t wp, int argc, char_t **argv)
{
	FILE *fp;
	struct lease_t lease;
	int i;
	struct in_addr addr;
	unsigned long expires;
	char sigusr1[] = "-XX";
	int ret = 0;
	int count = 0;
	char *ipaddr, mac[20]="", expires_time[50]="";


        /* Write out leases file */
        sprintf(sigusr1, "-%d", SIGUSR1);
        eval("killall", sigusr1, "udhcpd");


	/* Parse leases file */
	if ((fp = fopen("/tmp/udhcpd.leases", "r"))) {
	    while (fread(&lease, sizeof(lease), 1, fp)) {
		strcpy(mac,"");

		for (i = 0; i < 6; i++) {
			sprintf(mac+strlen(mac),"%02X", lease.chaddr[i]);
			if (i != 5) sprintf(mac+strlen(mac),":");
		}
		mac[17] = '\0';
		if(!strcmp(mac,"00:00:00:00:00:00"))
			continue;
		
		addr.s_addr = lease.yiaddr;

		ipaddr = inet_ntoa(addr);

		expires = ntohl(lease.expires);

		strcpy(expires_time,"");
		if (!expires){
			continue;
			strcpy(expires_time,"expired");
		}
		else {
			if (expires > 60*60*24) {
				if(nvram_match("language", "DE"))
					sprintf(expires_time+strlen(expires_time),"%ld Tag, ",expires / (60*60*24));
				else
					sprintf(expires_time+strlen(expires_time),"%ld days, ",expires / (60*60*24));
				expires %= 60*60*24;
			}
			if (expires > 60*60) {
				sprintf(expires_time+strlen(expires_time),"%02ld:",expires / (60*60));	// hours
				expires %= 60*60;
			}
			else{
				sprintf(expires_time+strlen(expires_time),"00:");	// no hours
			}
			if (expires > 60) {
				sprintf(expires_time+strlen(expires_time),"%02ld:",expires / 60);	// minutes
				expires %= 60;
			}
			else{
				sprintf(expires_time+strlen(expires_time),"00:");	// no minutes
			}

			sprintf(expires_time+strlen(expires_time),"%02ld:",expires);		// seconds

			expires_time[strlen(expires_time)-1]='\0';
		}
ret += websWrite(wp, "%c'%s','%s','%s','%s','%d'\n",count ? ',' : ' ', 
			!*lease.hostname ? "&nbsp;" : lease.hostname, 
			ipaddr, 
			mac, 
			expires_time, 
			get_single_ip(inet_ntoa(addr),3));
                count++;
	   }
	fclose(fp);
	}

	return ret;
}

/* Delete leases */
int
delete_leases(webs_t wp)
{
        FILE *fp_w;
        char sigusr1[] = "-XX";
        int i;
        int ret = 0;

	if(nvram_match("lan_proto","static"))
		return ret;

	unlink("/tmp/.delete_leases");

        if (!(fp_w = fopen("/tmp/.delete_leases", "w"))) {
                websError(wp, 400, "Write leases error\n");
                return -1;
        }

	for(i = 0 ; i < DHCP_MAX_COUNT ; i ++){
		char name[] = "d_XXX";
		char *value;
                snprintf(name, sizeof(name), "d_%d", i);
                value = websGetVar(wp, name, NULL);
		if(!value)	continue;
		fprintf(fp_w,"%d.%d.%d.%s\n",get_single_ip(nvram_safe_get("lan_ipaddr"),0),
					     get_single_ip(nvram_safe_get("lan_ipaddr"),1),
					     get_single_ip(nvram_safe_get("lan_ipaddr"),2),
					     value);	

	}
	fclose(fp_w);
	
      	sprintf(sigusr1, "-%d", SIGUSR2);
       	eval("killall", sigusr1, "udhcpd");	// call udhcpd to delete ip from lease table

        return ret;
}


void
dhcp_check(webs_t wp, char *value, struct variable *v)
{
	return ;	// The udhcpd can valid lease table when re-load udhcpd.leases.	by honor 2003-08-05
}

int
dhcp_renew(webs_t wp)
{
	int ret;
        char sigusr[] = "-XX";

	sprintf(sigusr, "-%d", SIGUSR1);
	ret = eval("killall", sigusr, "udhcpc");

	return ret;
}

int
dhcp_release(webs_t wp)
{
	int ret = 0;

	nvram_set("wan_ipaddr","0.0.0.0");
	nvram_set("wan_netmask","0.0.0.0");
	nvram_set("wan_gateway","0.0.0.0");
	nvram_set("wan_get_dns","");
	nvram_set("wan_lease","0");
	
	unlink("/tmp/get_lease_time");
	unlink("/tmp/lease_time");
	
	return ret;
}



