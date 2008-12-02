
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
#include <ctype.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <unistd.h>

#include <broadcom.h>

#define	STATUS_RETRY_COUNT	5
#define STATUS_REFRESH_TIME1	5	
#define STATUS_REFRESH_TIME2	60

//int retry_count = -1; //by tallest new status flash.
int refresh_time = STATUS_REFRESH_TIME2;

#if 1	//by tallest new status flash.
//=============================================================
struct status_parameter{
	char *wan_ipaddr;
	char *wan_netmask;
	char *wan_gateway;
	char *status1;
	char *status2;
	char *button1;
	char *hidden1;
	char *hidden2;
	int wan_link;
	int retry_count;
	int session_no;
	struct dns_lists *dns_list;
};
struct status_parameter button1_para = {"","","","","","","","",0,-1,0,NULL};
struct status_parameter button2_para = {"","","","","","","","",0,-1,1,NULL};
//============================================================================
static void
get_button_para(struct status_parameter *button_para){
	
	char *wan_proto = nvram_safe_get("wan_proto");
	char ipaddr[2][15]={"wan_ipaddr","wan_ipaddr_1"};
	char netmask[2][15]={"wan_netmask","wan_netmask_1"};
	char gateway[2][15]={"wan_gateway","wan_gateway_1"};

	button_para->wan_link = check_wan_link(button_para->session_no);
	button_para->dns_list = get_dns_list(button_para->session_no);
        if(nvram_match("wan_proto","pptp")){
                button_para->wan_ipaddr = button_para->wan_link ? nvram_safe_get("pptp_get_ip") : nvram_safe_get("wan_ipaddr");
                button_para->wan_netmask = button_para->wan_link ? "255.255.255.255" : nvram_safe_get("wan_netmask");
                button_para->wan_gateway = button_para->wan_link ? nvram_safe_get("wan_gateway") : nvram_safe_get("pptp_server_ip");
        }
	else if(nvram_match("wan_proto","l2tp")){
		button_para->wan_ipaddr =button_para->wan_link ? nvram_safe_get("l2tp_get_ip") : nvram_safe_get("wan_ipaddr");
		button_para->wan_netmask = button_para->wan_link ? "255.255.255.255" : nvram_safe_get("wan_netmask");
		button_para->wan_gateway = button_para->wan_link ? nvram_safe_get("wan_gateway") : nvram_safe_get("wan_gateway");
	}
        else if(!strcmp(nvram_safe_get("wan_proto"),"pppoe")
	){
		button_para->wan_ipaddr = button_para->wan_link ? nvram_safe_get(ipaddr[button_para->session_no]) : "0.0.0.0";
		button_para->wan_netmask = button_para->wan_link ? nvram_safe_get(netmask[button_para->session_no]) : "0.0.0.0";
		button_para->wan_gateway = button_para->wan_link ? nvram_safe_get(gateway[button_para->session_no]) : "0.0.0.0";
        }
        else if(!strcmp(nvram_safe_get("wan_proto"),"heartbeat")){
		button_para->wan_ipaddr = button_para->wan_link ? nvram_safe_get("wan_ipaddr") : "0.0.0.0";
		button_para->wan_netmask = button_para->wan_link ? nvram_safe_get("wan_netmask") : "0.0.0.0";
		button_para->wan_gateway = button_para->wan_link ? nvram_safe_get("wan_gateway") : "0.0.0.0";
        }
        else{
                button_para->wan_ipaddr = nvram_safe_get("wan_ipaddr");
                button_para->wan_gateway = nvram_safe_get("wan_gateway");
                button_para->wan_netmask = nvram_safe_get("wan_netmask");
        }
 
        if(!strcmp(wan_proto, "pppoe") || !strcmp(wan_proto, "pptp") || !strcmp(wan_proto, "heartbeat")
        || (!strcmp(wan_proto, "l2tp"))
	){
		button_para->hidden1 = "";
            	button_para->hidden2 = "";
		// submit_button old format is "Connect", new format is "Connect_pppoe" 
		//or "Connect_pptp" or "Connect_heartbeat".
            	if (button_para->wan_link == 0){
                	if(button_para->retry_count != -1){
                        	button_para->status1 = "Status";
                        	button_para->status2 = "Connecting";
                        	button_para->button1 = "Disconnect";
                	}
                	else{
                        	button_para->status1 = "Status";
                        	button_para->status2 = "Disconnected";
                        	button_para->button1 = "Connect";
                	}
            	}
            	else {
                	button_para->retry_count = -1;
                	button_para->status1 ="Status";
			button_para->status2 ="Connected";
                	button_para->status2 ="Connected";
                	button_para->button1 = "Disconnect";
            	}
        }
        else{
            	button_para->status1 ="Disable"; // only for nonbrand
		button_para->status2 ="&nbsp;";
            	button_para->status2 ="&nbsp;";
            	button_para->hidden1 = "<!--";
            	button_para->hidden2 = "-->";
        }

	return;
}
#endif

int
ej_show_status_setting(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;

	do_ej("Status_Router1.asp",wp);

	return ret;
}

/* Report time in RFC-822 format */
int
ej_localtime(int eid, webs_t wp, int argc, char_t **argv)
{
	time_t tm;

	time(&tm);

	if(time(0) > (unsigned long)60*60*24*365)
		return websWrite(wp, rfctime(&tm));
	else
		return websWrite(wp, "Not Available");
}	

int
ej_dhcp_remaining_time(int eid, webs_t wp, int argc, char_t **argv)
{
        int ret = 0;
	unsigned long now_time = 0L;
	unsigned long get_leases_time = 0L;
	unsigned long leases_time = 0L;
	unsigned long remain_time = 0L;
	struct sysinfo info;
	char string[80];
	int day=0, hour=0, min=0, sec=0;
	FILE *fp;

	if(nvram_invmatch("wan_proto","dhcp")
	)
		return ret;
	
	leases_time = atol(nvram_safe_get("wan_lease"));

	if ((fp = fopen("/tmp/udhcpc.expires", "r")) != NULL) {
		fscanf(fp,"%s",string);
		get_leases_time = atol(string) - leases_time;
		fclose(fp);
	}

	if(get_leases_time ==0 || leases_time == 0)
		return websWrite(wp, "0");

	sysinfo(&info);
	now_time = info.uptime;

	remain_time = leases_time - (now_time - get_leases_time);


	if(remain_time < 0)
		return websWrite(wp, "0");
		
	if(leases_time){
		if (remain_time > 60*60*24) {  //days
			day = (int)remain_time / (60*60*24);
			remain_time %= 60*60*24;
		}
		if (remain_time > 60*60) {   //hours
			hour = (int)remain_time / (60*60);
			remain_time %= 60*60;
		}

		if (remain_time > 60) {      //miniutes
			min = (int)remain_time / 60;
			remain_time %= 60;
		}			     //seconds

		sec = (int)remain_time;
	}

	if(day)
		ret += websWrite(wp, "%d days, ", day);
	ret += websWrite(wp, "%d:%02d:%02d", hour, min, sec);

	return ret;
}

#if 1 //by tallest new status flash.
int
ej_nvram_status_get(int eid, webs_t wp, int argc, char_t **argv)
{
        char *type, no;
	struct status_parameter *button_para = NULL;

        if (ejArgs(argc, argv, "%s %d", &type, &no) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }

	if(no != 1)
		no = 0;

	//button1_para.dns_list = get_dns_list();
	//button2_para.dns_list = get_dns_list();
	if(no == 0)
		button_para = &button1_para;
	else if(no == 1)
		button_para = &button2_para;
	
	if(!strcmp(type,"wan_ipaddr")){
                return websWrite(wp,"%s",button_para->wan_ipaddr);
        }
        else if(!strcmp(type,"wan_netmask")){
                return websWrite(wp,"%s",button_para->wan_netmask);
        }
        else if(!strcmp(type,"wan_gateway")){
                return websWrite(wp,"%s",button_para->wan_gateway);
        }
        else if(!strcmp(type,"wan_dns0")){
                return websWrite(wp,"%s",button_para->dns_list->dns_server[0]);
        }
        else if(!strcmp(type,"wan_dns1")){
                return websWrite(wp,"%s",button_para->dns_list->dns_server[1]);
        }
        else if(!strcmp(type,"wan_dns2")){
                return websWrite(wp,"%s",button_para->dns_list->dns_server[2]);
        }
        else if(!strcmp(type,"status1")){
                return websWrite(wp,"%s",button_para->status1);
        }
        else if(!strcmp(type,"status2")){
                return websWrite(wp,"%s",button_para->status2);
        }
        else if(!strcmp(type,"button1")){
                return websWrite(wp,"%s",button_para->button1);
        }
        else if(!strcmp(type,"hidden1")){
                return websWrite(wp,"%s",button_para->hidden1);
        }
        else if(!strcmp(type,"hidden2")){
                return websWrite(wp,"%s",button_para->hidden2);
        }
	if(button1_para.dns_list)	free(button1_para.dns_list);
	if(button2_para.dns_list)       free(button2_para.dns_list);

	return 1;	
}

int
ej_show_status(int eid, webs_t wp, int argc, char_t **argv)
{
	char *type;
	int ret = 0;
        char *wan_proto = nvram_safe_get("wan_proto");
	char *submit_type = websGetVar(wp, "submit_type", NULL);
	char buf[254];
	
        if(!strcmp(wan_proto,"pppoe") || !strcmp(wan_proto,"pptp") || !strcmp(wan_proto,"heartbeat")
        || (!strcmp(wan_proto, "l2tp"))
	){
		/* get type  [ refresh | reload ]*/
		if (ejArgs(argc, argv, "%s", &type) < 1) {
                	websError(wp, 400, "Insufficient args\n");
                	return -1;
             	}

		if(!strcmp(type,"init")){
			/* press [ Connect | Disconnect ] button */
			/* set retry count */
			if(gozila_action)
			{
				{
		    			button1_para.retry_count = STATUS_RETRY_COUNT;	// retry 3 times
		    		}
			}

	        	/* set refresh time 
			 * submit_type old format is "Disconnect", new format is 
			 * "Disconnect_pppoe" or "Disconnect_pptp" or "Disconnect_heartbeat"
			 * Disconnect always 60 seconds to refresh
			 */
			if(submit_type && !strncmp(submit_type, "Disconnect", 10)){
				button1_para.retry_count = -1;
			}

			get_button_para(&button1_para);
			{
				refresh_time = (button1_para.retry_count <= 0) ? STATUS_REFRESH_TIME2 : STATUS_REFRESH_TIME1;
			}
		}
		else if(!strcmp(type,"refresh_time")){

			ret += websWrite(wp,"%d",refresh_time*1000);
             	}
	     	else if(!strcmp(type,"onload")){
			//After refresh 2 times, if the status is disconnect, show Alert message.
	           	if(button1_para.retry_count == 0
			|| (!submit_type && !button1_para.wan_link && gozila_action)){
	               		ret += websWrite(wp,"ShowAlert(\"TIMEOUT\");");
	               		button1_para.retry_count = -1;
	  	   	}
		   	else if(file_to_buf("/tmp/ppp/log", buf, sizeof(buf))){
				ret += websWrite(wp, "ShowAlert(\"%s\");", buf);
                        	button1_para.retry_count = -1;
				unlink("/tmp/ppp/log");
	           	}
        	   	else{
	               		ret += websWrite(wp,"Refresh();");
		   	}

	           	if(button1_para.retry_count != -1)
	              		button1_para.retry_count--;
		}
	}
	else {
		get_button_para(&button1_para);	/* DHCP; Static IP */
	}
        return ret;
}

#else
int
ej_nvram_status_get(int eid, webs_t wp, int argc, char_t **argv)
{
        char *type;
	char *wan_ipaddr, *wan_netmask, *wan_gateway;
	char *status1="", *status2="", *hidden1, *hidden2, *button1="";
	char *wan_proto = nvram_safe_get("wan_proto");
	struct dns_lists *dns_list = NULL;
	int wan_link = check_wan_link(0);

        if (ejArgs(argc, argv, "%s", &type) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }
	if(nvram_match("wan_proto","pptp")){
		wan_ipaddr = wan_link ? nvram_safe_get("pptp_get_ip") : nvram_safe_get("wan_ipaddr");
		wan_netmask = wan_link ? "255.255.255.255" : nvram_safe_get("wan_netmask");
		wan_gateway = wan_link ? nvram_safe_get("wan_gateway") : nvram_safe_get("pptp_server_ip");
	}
	else if(nvram_match("wan_proto","l2tp")){
		wan_ipaddr = wan_link ? nvram_safe_get("l2tp_get_ip") : nvram_safe_get("wan_ipaddr");
		wan_netmask = wan_link ? "255.255.255.255" : nvram_safe_get("wan_netmask");
		wan_gateway = wan_link ? nvram_safe_get("wan_gateway") : nvram_safe_get("wan_gateway");
	}
	else if(!strcmp(nvram_safe_get("wan_proto"),"pppoe") 
	){
		wan_ipaddr = wan_link ? nvram_safe_get("wan_ipaddr") : "0.0.0.0";
		wan_netmask = wan_link ? nvram_safe_get("wan_netmask") : "0.0.0.0";
		wan_gateway = wan_link ? nvram_safe_get("wan_gateway") : "0.0.0.0";
	}
	else if(!strcmp(nvram_safe_get("wan_proto"),"heartbeat") ){
		wan_ipaddr = wan_link ? nvram_safe_get("wan_ipaddr") : "0.0.0.0";
		wan_netmask = wan_link ? nvram_safe_get("wan_netmask") : "0.0.0.0";
		wan_gateway = wan_link ? nvram_safe_get("wan_gateway") : "0.0.0.0";
	}
	else{
		wan_ipaddr = nvram_safe_get("wan_ipaddr");
		wan_gateway = nvram_safe_get("wan_gateway");
		wan_netmask = nvram_safe_get("wan_netmask");
	}

	dns_list = get_dns_list();

	if(!strcmp(wan_proto, "pppoe") || !strcmp(wan_proto, "pptp") || !strcmp(wan_proto, "l2tp") || !strcmp(wan_proto, "heartbeat")
	){
	    hidden1 = "";
	    hidden2 = "";
	    if (wan_link == 0){
		// submit_button old format is "Connect", new format is "Connect_pppoe" or "Connect_pptp" or "Connect_heartbeat"
		//if(submit_type && !strncmp(submit_type,"Connect",7) && retry_count != -1){
		if(retry_count != -1){
			status1 = "Status";
			status2 = "Connecting";
			button1 = "Disconnect";
		}
		else{
			status1 = "Status";
			status2 = "Disconnected";
			button1 = "Connect";
		}
	    }
	    else {
		retry_count = -1;
		status1 ="Status";
		status2 ="Connected";
		button1 = "Disconnect";
	    }
	}
	else{
	    status1 ="Disable";	// only for nonbrand
	    status2 ="&nbsp;";
	    hidden1 = "<!--";
	    hidden2 = "-->";
	}
	

	if(!strcmp(type,"wan_ipaddr"))
		return websWrite(wp,"%s",wan_ipaddr);
	else if(!strcmp(type,"wan_netmask"))
		return websWrite(wp,"%s",wan_netmask);
	else if(!strcmp(type,"wan_gateway"))
		return websWrite(wp,"%s",wan_gateway);
	else if(!strcmp(type,"wan_dns0"))
		return websWrite(wp,"%s",dns_list->dns_server[0]);
	else if(!strcmp(type,"wan_dns1"))
		return websWrite(wp,"%s",dns_list->dns_server[1]);
	else if(!strcmp(type,"wan_dns2"))
		return websWrite(wp,"%s",dns_list->dns_server[2]);
	else if(!strcmp(type,"status1"))
		return websWrite(wp,"%s",status1);
	else if(!strcmp(type,"status2"))
		return websWrite(wp,"%s",status2);
	else if(!strcmp(type,"button1"))
		return websWrite(wp,"%s",button1);
	else if(!strcmp(type,"hidden1"))
		return websWrite(wp,"%s",hidden1);
	else if(!strcmp(type,"hidden2"))
		return websWrite(wp,"%s",hidden2);

	if(dns_list)	free(dns_list);

	return 1;	
}

int
ej_show_status(int eid, webs_t wp, int argc, char_t **argv)
{
	char *type;
	int ret = 0;
        char *wan_proto = nvram_safe_get("wan_proto");
	char *submit_type = websGetVar(wp, "submit_type", NULL);
	int wan_link = 0;
	char buf[254];
	
        if(!strcmp(wan_proto,"pppoe") || !strcmp(wan_proto,"pptp") || !strcmp(wan_proto,"l2tp") || !strcmp(wan_proto,"heartbeat")
	){

	     /* get type  [ refresh | reload ]*/
             if (ejArgs(argc, argv, "%s", &type) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
             }
	         /* get ppp status , if /tmp/ppp/link exist, the connection is enabled */
		 wan_link = check_wan_link(0);


	     if(!strcmp(type,"init")){

	         /* press [ Connect | Disconnect ] button */
	         /* set retry count */
	         if(gozila_action)     
		    retry_count = STATUS_RETRY_COUNT;	// retry 3 times

	         /* set refresh time */
		// submit_type old format is "Disconnect", new format is "Disconnect_pppoe" or "Disconnect_pptp" or "Disconnect_heartbeat"
		if(submit_type && !strncmp(submit_type, "Disconnect", 10))	// Disconnect always 60 seconds to refresh
			retry_count = -1;

	         refresh_time = (retry_count <= 0) ? STATUS_REFRESH_TIME2 : STATUS_REFRESH_TIME1;

	     }
	     else if(!strcmp(type,"refresh_time")){

		ret += websWrite(wp,"%d",refresh_time*1000);
             }
	     else if(!strcmp(type,"onload")){
	           if(retry_count == 0 || (!submit_type && !wan_link && gozila_action)){    //After refresh 2 times, if the status is disconnect, show Alert message.
	               ret += websWrite(wp,"ShowAlert(\"TIMEOUT\");");
	               retry_count = -1;
	  	   }
		   else if(file_to_buf("/tmp/ppp/log", buf, sizeof(buf))){
			ret += websWrite(wp, "ShowAlert(\"%s\");", buf);
                        retry_count = -1;
			unlink("/tmp/ppp/log");
	           }
        	   else{
	               ret += websWrite(wp,"Refresh();");
		   }

	           if(retry_count != -1)
	              retry_count--;
	       }
       }
        return ret;
}
#endif

//======================== by tallest ============================
int
ej_show_wan_domain(int eid, webs_t wp, int argc, char_t **argv)
{
	char *wan_domain;

	if(strcmp(nvram_safe_get("wan_domain"),""))
		wan_domain = nvram_safe_get("wan_domain");
	else
		wan_domain = nvram_safe_get("wan_get_domain");
	return websWrite(wp,"%s",wan_domain);	
}
//================================================================

int
stop_ppp(webs_t wp)
{
	unlink("/tmp/ppp/log");
	return unlink("/tmp/ppp/link");
}



/* Return WAN link state */
/*
static int
ej_link(int eid, webs_t wp, int argc, char_t **argv)
{
        char *name;
        int s;
        struct ifreq ifr;
        struct ethtool_cmd ecmd;
        FILE *fp;

        if (ejArgs(argc, argv, "%s", &name) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }

        // PPPoE connection status 
        if (nvram_match("wan_proto", "pppoe")) {
                if ((fp = fopen("/tmp/ppp/link", "r"))) {
                        fclose(fp);
                       return websWrite(wp, "Connected");
                } else
                        return websWrite(wp, "Disconnected");
        }

        // Open socket to kernel 
        if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                websError(wp, 400, strerror(errno));
                return -1;
        }

        // Check for non-zero link speed 
        strncpy(ifr.ifr_name, nvram_safe_get(name), IFNAMSIZ);
        ifr.ifr_data = (void *) &ecmd;
        ecmd.cmd = ETHTOOL_GSET;
        if (ioctl(s, SIOCETHTOOL, &ifr) < 0) {
                close(s);
                websError(wp, 400, strerror(errno));
                return -1;
	 }

        // Cleanup 
        close(s);

        if (ecmd.speed)
                return websWrite(wp, "Connected");
        else
                return websWrite(wp, "Disconnected");
}

*/

