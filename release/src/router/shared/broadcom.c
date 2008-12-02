
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

/*
 * Broadcom Home Gateway Reference Design
 * Web Page Configuration Support Routines
 *
 * Copyright 2001-2003, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 * $Id: broadcom.c,v 1.127.10.4.2.7 2006/07/04 06:10:20 honor Exp $
 */

#ifdef WEBS
#include <webs.h>
#include <uemf.h>
#include <ej.h>
#else /* !WEBS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <httpd.h>
#include <errno.h>
#endif /* WEBS */

#include <proto/ethernet.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/klog.h>
#include <sys/wait.h>

#include <broadcom.h>
#include <cyutils.h>
#include <support.h>
#include <cy_conf.h>
//#ifdef EZC_SUPPORT
#include <ezc.h>
//#endif

int gozila_action = 0;
int error_value = 0;
int browser_method ;

#ifdef DEBUG_WEB
int debug_value = 1;
#else
int debug_value = 0;
#endif

#ifdef HSIAB_SUPPORT
extern int register_status;
extern int new_device;
#endif

//#if defined(linux)

#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/klog.h>
#include <sys/wait.h>
#define service_restart() kill(1, SIGUSR1)
#define sys_restart() kill(1, SIGHUP)
#define sys_reboot() kill(1, SIGTERM)
#define sys_stats(url) eval("stats", (url))

/* Example:
 * ISDIGIT("", 0); return true;
 * ISDIGIT("", 1); return false;
 * ISDIGIT("123", 1); return true;
 */
int
ISDIGIT(char *value, int flag)
{
	int i, tag = TRUE;

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): len=(%d)",__FUNCTION__,value,strlen(value));
#endif

	if(!strcmp(value,"")){
		if (flag) return 0;   // null
		else	return 1;
	}

	for(i=0 ; *(value+i) ; i++){
		if(!isdigit(*(value+i))){
			tag = FALSE;
			break;
		}
	}
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): (%s)",__FUNCTION__,value,tag ? "TRUE" : "FALSE");
#endif
	return tag;
}

/* Example:
 * ISASCII("", 0); return true;
 * ISASCII("", 1); return false;
 * ISASCII("abc123", 1); return true;
 */
int
ISASCII(char *value, int flag)
{
	int i, tag = TRUE;
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): len=(%d)",__FUNCTION__,value,strlen(value));
#endif

#if COUNTRY == JAPAN
	return tag;	// don't check for japan version
#endif

	if(!strcmp(value,"")){
		if (flag) return 0;   // null
		else	return 1;
	}

	for(i=0 ; *(value+i) ; i++){
		if(!isascii(*(value+i))){
			tag = FALSE;
			break;
		}
	}
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): (%s)",__FUNCTION__,value,tag ? "TRUE" : "FALSE");
#endif
	return tag;
}

/* Example:
 * legal_hwaddr("00:11:22:33:44:aB"); return true;
 * legal_hwaddr("00:11:22:33:44:5"); return false;
 * legal_hwaddr("00:11:22:33:44:HH"); return false;
 */
int
legal_hwaddr(char *value)
{
	unsigned int hwaddr[6];
	int tag = TRUE;
	int i,count;

	/* Check for bad, multicast, broadcast, or null address */
	for(i=0,count=0 ; *(value+i) ; i++){
		if(*(value+i) == ':'){
			if((i+1)%3 != 0){
				tag = FALSE;
				break;
			}	
			count++;
		}
		else if(isxdigit(*(value+i))) /* one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F */
			continue;
		else{
			tag = FALSE;
			break;
		}
	}
	
	if (!tag || i != 17 || count != 5)		/* must have 17's characters and 5's ':' */
		tag = FALSE;	
	else if (sscanf(value, "%x:%x:%x:%x:%x:%x",
		   &hwaddr[0], &hwaddr[1], &hwaddr[2],
		   &hwaddr[3], &hwaddr[4], &hwaddr[5]) != 6 ){
	    //(hwaddr[0] & 1) ||		// the bit 7 is 1 
	    //(hwaddr[0] & hwaddr[1] & hwaddr[2] & hwaddr[3] & hwaddr[4] & hwaddr[5]) == 0xff ){ // FF:FF:FF:FF:FF:FF
	    //(hwaddr[0] | hwaddr[1] | hwaddr[2] | hwaddr[3] | hwaddr[4] | hwaddr[5]) == 0x00){ // 00:00:00:00:00:00
		tag = FALSE;
	}
	else
		tag = TRUE;
	
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): (%s)",__FUNCTION__,value,tag?"TRUE":"FALSE");
#endif

	return tag;
}

/* Example:
 * 255.255.255.0  (111111111111111111111100000000)  is a legal netmask
 * 255.255.0.255  (111111111111110000000011111111)  is an illegal netmask
 */
int
legal_netmask(char *value)
{
	struct in_addr ipaddr;
	int ip[4]={0,0,0,0};
	int i,j;
	int match0 = -1;
	int match1 = -1;
	int ret, tag;

	ret=sscanf(value,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);

	if (ret == 4 && inet_aton(value, &ipaddr)) {
		for(i=3;i>=0;i--){
			for(j=1;j<=8;j++){
				if((ip[i] % 2) == 0)   match0 = (3-i)*8 + j;
				else if(((ip[i] % 2) == 1) && match1 == -1)   match1 = (3-i)*8 + j;
				ip[i] = ip[i] / 2;
			}
		}
	}

	if (match0 >= match1)
		tag = FALSE;
	else
		tag = TRUE;

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): match0=[%d] match1=[%d] (%s)",__FUNCTION__,value,match0,match1,tag?"TRUE":"FALSE");
#endif

	return tag;
}


/* Example:
 * legal_ipaddr("192.168.1.1"); return true;
 * legal_ipaddr("192.168.1.1111"); return false;
 */
int
legal_ipaddr(char *value)
{
	struct in_addr ipaddr;
	int ip[4];
	int ret, tag;

	ret = sscanf(value,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);

	if (ret != 4 || !inet_aton(value, &ipaddr))
		tag = FALSE;
	else
		tag = TRUE;

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): ret=[%d] ip[%d][%d][%d][%d] (%s)",__FUNCTION__,value,ret,ip[0],ip[1],ip[2],ip[3],tag?"TRUE":"FALSE");
#endif

	return tag;
}

/* Example:
 * legal_ip_netmask("192.168.1.1","255.255.255.0","192.168.1.100"); return true;
 * legal_ip_netmask("192.168.1.1","255.255.255.0","192.168.2.100"); return false;
 */
int
legal_ip_netmask(char *sip, char *smask, char *dip)
{
	struct in_addr ipaddr, netaddr, netmask;
	int tag;

	inet_aton(nvram_safe_get(sip), &netaddr);
	inet_aton(nvram_safe_get(smask), &netmask);
	inet_aton(dip, &ipaddr);

	netaddr.s_addr &= netmask.s_addr;

	if (netaddr.s_addr != (ipaddr.s_addr & netmask.s_addr))
		tag = FALSE;
	else
		tag = TRUE;

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): sip=[%s] smask=[%s] dip=[%s] (%s)",__FUNCTION__,nvram_safe_get(sip),nvram_safe_get(smask),dip,tag?"TRUE":"FALSE");
#endif
	
	return tag;
}


/* Example:
 * wan_dns = 1.2.3.4 10.20.30.40 15.25.35.45
 * get_dns_ip("wan_dns", 1, 2); produces "20"
 */
int
get_dns_ip(char *name, int which, int count)
{
        static char word[256];
        char *next;
	int ip;

        foreach(word, nvram_safe_get(name), next) {
                if (which-- == 0){
			ip = get_single_ip(word,count);
#ifdef MY_DEBUG
	//LOG(LOG_DEBUG,"%s(): ip=[%d]",__FUNCTION__,ip);
#endif
                	return ip;
                }
        }
        return 0;
}


/* Example:
 * wan_mac = 00:11:22:33:44:55
 * get_single_mac("wan_mac", 1); produces "11"
 */
int
get_single_mac(char *macaddr, int which)
{
	int mac[6]={0,0,0,0,0,0};
	int ret;

	ret = sscanf(macaddr,"%2X:%2X:%2X:%2X:%2X:%2X",&mac[0],&mac[1],&mac[2],&mac[3],&mac[4],&mac[5]);
	return mac[which];	
}


/* Example:
 * lan_ipaddr_0 = 192
 * lan_ipaddr_1 = 168
 * lan_ipaddr_2 = 1
 * lan_ipaddr_3 = 1
 * get_merge_ipaddr("lan_ipaddr", ipaddr); produces ipaddr="192.168.1.1"
 */
int
get_merge_ipaddr(char *name, char *ipaddr)
{
	char ipname[30];
	int i;
	
	strcpy(ipaddr,"");	

	for(i=0 ; i<4 ; i++){
		snprintf(ipname,sizeof(ipname),"%s_%d",name,i);
		strcat(ipaddr, websGetVar(wp, ipname , "0"));
		if(i<3)	strcat(ipaddr, ".");
	}

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): ipaddr=[%s]",__FUNCTION__,ipaddr);
#endif
	return 1;

}


/* Example:
 * wan_mac_0 = 00
 * wan_mac_1 = 11
 * wan_mac_2 = 22
 * wan_mac_3 = 33
 * wan_mac_4 = 44
 * wan_mac_5 = 55
 * get_merge_mac("wan_mac",mac); produces mac="00:11:22:33:44:55"
 */
int 
get_merge_mac(char *name, char *macaddr)
{
	char macname[30];
	char *mac;
	int i;

	strcpy(macaddr,"");

	for(i=0 ; i<6 ; i++){
		snprintf(macname,sizeof(macname),"%s_%d",name,i);
		mac = websGetVar(wp, macname , "00");
		if(strlen(mac) == 1)	strcat(macaddr,"0");
		strcat(macaddr,mac);
		if(i<5)	strcat(macaddr,":");
	}

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): macaddr=[%s]",__FUNCTION__,macaddr);
#endif
	return 1;

}

struct onload onloads[] = {
	//{ "Filters", filter_onload },
	{ "WL_ActiveTable", wl_active_onload },
	{ "MACClone", macclone_onload },
	{ "FilterSummary", filtersummary_onload },
#ifdef DIAG_SUPPORT
	{ "Ping", ping_onload },
	{ "Traceroute", traceroute_onload },
#endif
};

int
ej_onload(int eid, webs_t wp, int argc, char_t **argv)
{
	char *type, *arg;
	int ret = 0;
	struct onload *v;

        if (ejArgs(argc, argv, "%s %s", &type, &arg) < 2) {
             websError(wp, 400, "Insufficient args\n");
             return -1;
        }

	for(v = onloads ; v < &onloads[STRUCT_LEN(onloads)] ; v++) {
#ifdef MY_DEBUG
   		LOG(LOG_DEBUG,"%s(%s): onload=[%s]\n",__FUNCTION__,type,v->name);
#endif
   		if(!strcmp(v->name,type)){
      			ret = v->go(wp, arg);
			return ret;
		}
   	}

        return ret;
}

/* Meta tag command that will no allow page cached by browsers.
 * The will force the page to be refreshed when visited.
 */
int
ej_no_cache(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret;

	ret = websWrite(wp,"<meta http-equiv=\"expires\" content=\"0\">\n");
	ret += websWrite(wp,"<meta http-equiv=\"cache-control\" content=\"no-cache\">\n");
	ret += websWrite(wp,"<meta http-equiv=\"pragma\" content=\"no-cache\">\n");

#ifndef MULTILANG_SUPPORT
	ret += websWrite(wp,"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=%s\">", HTTP_CHARSET);
#endif	

	return ret;
}
#ifdef MULTILANG_SUPPORT
/* Meta tag command that will no allow page cached by browsers.
 * The will force the page to be refreshed when visited.
 */
int
ej_langpack(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;
//	do_ej("lang_pack/capsec.js",wp);
//	do_ej("lang_pack/capapp.js",wp);
//	do_ej("lang_pack/capasg.js",wp);
//	do_ej("lang_pack/capsetup.js",wp);
//	do_ej("lang_pack/help.js",wp);
	do_ej("lang_pack/share.js",wp);
	do_ej("lang_pack/capadmin.js",wp);
	do_ej("lang_pack/capwrt54g.js",wp);
	do_ej("common.js",wp);
	cprintf("ej_langpack\r\n");
	return ret;
}
int
ej_charset(int eid, webs_t wp, int argc, char_t **argv)
{
        do_ej("lang_pack/charset.js",wp);
}

#endif	

/*
 * Example: 
 * lan_ipaddr=192.168.1.1
 * <% prefix_ip_get("lan_ipaddr",1); %> produces "192.168.1."
 */
int
ej_prefix_ip_get(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name;
	int type;
	int ret = 0;

        if (ejArgs(argc, argv, "%s %d", &name,&type) < 2) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s,%d): ",__FUNCTION__,name,type);	
#endif
	
	if(type == 1)
		ret = websWrite(wp,"%d.%d.%d.",get_single_ip(nvram_safe_get("lan_ipaddr"),0),
					       get_single_ip(nvram_safe_get("lan_ipaddr"),1),
					       get_single_ip(nvram_safe_get("lan_ipaddr"),2));

	return ret;
}

/* Deal with side effects before committing */
int
sys_commit(void)
{
	return nvram_commit();
}


char *
rfctime(const time_t *timep)
{
	static char s[201];
	struct tm tm;

	setenv("TZ", nvram_safe_get("time_zone"), 1);
	memcpy(&tm, localtime(timep), sizeof(struct tm));
	//strftime(s, 200, "%a, %d %b %Y %H:%M:%S %z", &tm);
 	if (!strcmp(nvram_safe_get("language"), "DE"))
                strftime(s, 200, "%a, %Y-%m-%d %H:%M:%S", &tm);
        else
                strftime(s, 200, "%a, %d %b %Y %H:%M:%S", &tm);
        return s;
}

static char *
reltime(unsigned int seconds)
{
	static char s[] = "XXXXX days, XX hours, XX minutes, XX seconds";
	char *c = s;

	if (seconds > 60*60*24) {
		c += sprintf(c, "%d days, ", seconds / (60*60*24));
		seconds %= 60*60*24;
	}
	if (seconds > 60*60) {
		c += sprintf(c, "%d hours, ", seconds / (60*60));
		seconds %= 60*60;
	}
	if (seconds > 60) {
		c += sprintf(c, "%d minutes, ", seconds / 60);
		seconds %= 60;
	}
	c += sprintf(c, "%d seconds", seconds);

	return s;
}

int
_websWrite(webs_t wp, char *value)
{
	char *c;	
	int ret = 0;
	
	for (c = value; *c; c++) {
		if (isprint((int) *c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>' && *c != '\'')
			ret += websWrite(wp, "%c", *c);
		else
			ret += websWrite(wp, "&#%d;", *c);
	}
	return ret;
}

/*
 * Example: 
 * lan_ipaddr = 192.168.1.1
 * <% nvram_get("lan_ipaddr"); %> produces "192.168.1.1"
 */
static int
ej_nvram_get(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *c;
	int ret = 0;

	if (ejArgs(argc, argv, "%s", &name) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

#if COUNTRY == JAPAN
	ret = websWrite(wp, "%s",nvram_safe_get(name));
#else
	for (c = nvram_safe_get(name); *c; c++) {
		if (isprint((int) *c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>' && *c != '\'')
			ret += websWrite(wp, "%c", *c);
		else
			ret += websWrite(wp, "&#%d;", *c);
	}
#endif
	return ret;
}

static int
ej_nvram_get_len(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *c;
	int length = 0, ret = 0, count = 0;

	if (ejArgs(argc, argv, "%s %d", &name, &length) < 2) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

#if COUNTRY == JAPAN
	ret = websWrite(wp, "%s",nvram_safe_get(name));
#else
	for (c = nvram_safe_get(name); *c; c++) {
		if (isprint((int) *c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>' && *c != '\'')
			ret += websWrite(wp, "%c", *c);
		else
			ret += websWrite(wp, "&#%d;", *c);
		count++;
		if(count > length)
		{
			ret += websWrite(wp, "<br>");
			count =0;
		}
	}
#endif
	return ret;
}


/*
 * Example: 
 * lan_ipaddr = 192.168.1.1, gozila_action = 0
 * <% nvram_selget("lan_ipaddr"); %> produces "192.168.1.1"
 * lan_ipaddr = 192.168.1.1, gozila_action = 1, websGetVar(wp, "lan_proto", NULL) = 192.168.1.2;
 * <% nvram_selget("lan_ipaddr"); %> produces "192.168.1.2"
 */
static int
ej_nvram_selget(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *c;
	int ret = 0;

	if (ejArgs(argc, argv, "%s", &name) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	if(gozila_action){
		char *buf = websGetVar(wp, name, NULL);
		if(buf)
			return websWrite(wp, "%s", buf);
	}

	for (c = nvram_safe_get(name); *c; c++) {
		if (isprint((int) *c) &&
		    *c != '"' && *c != '&' && *c != '<' && *c != '>')
			ret += websWrite(wp, "%c", *c);
		else
			ret += websWrite(wp, "&#%d", *c);
	}

	return ret;
}

/*
 * Example: 
 * wan_mac = 00:11:22:33:44:55
 * <% nvram_mac_get("wan_mac"); %> produces "00-11-22-33-44-55"
 */
static int
ej_nvram_mac_get(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *c;
	int ret = 0;
	char *mac;
	int i;

	if (ejArgs(argc, argv, "%s", &name) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	c = nvram_safe_get(name);

	if(c){
		mac = strdup(c);
		for(i=0 ; *(mac+i) ; i++){
			if(*(mac+i) == ':')	*(mac+i) = '-';
		}
		ret = websWrite(wp, "%s",mac );
	}

	return ret;

}

/*
 * Example: 
 * wan_proto = dhcp; gozilla = 0;
 * <% nvram_gozila_get("wan_proto"); %> produces "dhcp"
 *
 * wan_proto = dhcp; gozilla = 1; websGetVar(wp, "wan_proto", NULL) = static;
 * <% nvram_gozila_get("wan_proto"); %> produces "static"
 */
static int
ej_nvram_gozila_get(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *type;

	if (ejArgs(argc, argv, "%s", &name) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	type = GOZILA_GET(name);

	return websWrite(wp, "%s", type);
}

static int
ej_webs_get(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *value;
	int ret = 0;

	if (ejArgs(argc, argv, "%s", &name) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	value = websGetVar(wp, name, NULL);

	if(value)
		ret = websWrite(wp, "%s", value);

	return ret;
}

/*
 * Example: 
 * lan_ipaddr = 192.168.1.1
 * <% get_single_ip("lan_ipaddr","1"); %> produces "168"
 */
static int
ej_get_single_ip(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *c;
	int ret = 0;
	int which;

	if (ejArgs(argc, argv, "%s %d", &name, &which) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	c = nvram_safe_get(name);
	if(c){
		if(!strcmp(c, PPP_PSEUDO_IP) || !strcmp(c, PPP_PSEUDO_GW))
			c = "0.0.0.0";
		else if (!strcmp(c, PPP_PSEUDO_NM))
			c = "255.255.255.0";

		ret += websWrite(wp, "%d", get_single_ip(c,which));
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): name=[%s] which=[%d] ip=[%d]",__FUNCTION__,name,which,get_single_ip(c,which));
#endif
	}
	else
		ret += websWrite(wp, "0");

	return ret;
}

/*
 * Example: 
 * wan_mac = 00:11:22:33:44:55
 * <% get_single_mac("wan_mac","1"); %> produces "11"
 */
static int
ej_get_single_mac(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *c;
	int ret = 0;
	int which;
	int mac;

	if (ejArgs(argc, argv, "%s %d", &name, &which) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	c = nvram_safe_get(name);
	if(c){
		mac = get_single_mac(c,which);
		ret += websWrite(wp, "%02X", mac);
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): name=[%s] which=[%d] mac=[%02X]",__FUNCTION__,name,which,mac);
#endif
	}
	else
		ret += websWrite(wp, "00");

	return ret;
}

/*
 * Example: 
 * wan_proto = dhcp; gozilla = 0;
 * <% nvram_selmatch("wan_proto", "dhcp", "selected"); %> produces "selected"
 *
 * wan_proto = dhcp; gozilla = 1; websGetVar(wp, "wan_proto", NULL) = static;
 * <% nvram_selmatch("wan_proto", "static", "selected"); %> produces "selected"
 */
int
ej_nvram_selmatch(int eid, webs_t wp, int argc, char_t **argv)
{
        char *name, *match, *output;
        char *type;

        if (ejArgs(argc, argv, "%s %s %s", &name, &match, &output) < 3) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }

        type = GOZILA_GET(name);

        if (!type){
           if (nvram_match(name, match)){
                return websWrite(wp, output);
           }
        }
        else{
           if(!strcmp(type, match)
#ifdef WAN_AUTO_DETECT_SUPPORT
		|| ( (!strcmp(match,"auto")) && (!strcmp(type,"auto_pppoe") || !strcmp(type,"auto_dhcp")) )
#endif
		){
                return websWrite(wp, output);
           }
        }

        return 0;
}       

int
ej_nvram_else_selmatch(int eid, webs_t wp, int argc, char_t **argv)
{
        char *name, *match, *output1, *output2;
        char *type;

        if (ejArgs(argc, argv, "%s %s %s %s", &name, &match, &output1, &output2) < 4) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }

        type = GOZILA_GET(name);
        
        if (!type){
           if (nvram_match(name, match)){
                return websWrite(wp, output1);
           }
	   else
                return websWrite(wp, output2);
        }
        else{
           if(!strcmp(type, match)){
                return websWrite(wp, output1);
           }
	   else
                return websWrite(wp, output2);
        }

        return 0;
}       

/*
 * Example: 
 * wan_proto=dhcp
 * <% nvram_else_match("wan_proto", "dhcp", "0","1"); %> produces "0"
 * <% nvram_else_match("wan_proto", "static", "0","1"); %> produces "1"
 */
static int
ej_nvram_else_match(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *match, *output1, *output2;

	if (ejArgs(argc, argv, "%s %s %s %s", &name, &match, &output1, &output2) < 4) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (nvram_match(name, match))
		return websWrite(wp, output1);
	else
		return websWrite(wp, output2);

	return 0;
}	

/*
 * Example: 
 * wan_proto=dhcp
 * <% nvram_match("wan_proto", "dhcp", "selected"); %> produces "selected"
 * <% nvram_match("wan_proto", "static", "selected"); %> does not produce
 */
static int
ej_nvram_match(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *match, *output;

	if (ejArgs(argc, argv, "%s %s %s", &name, &match, &output) < 3) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (nvram_match(name, match))
		return websWrite(wp, output);

	return 0;
}	

/*
 * Example: 
 * wan_proto=dhcp
 * <% nvram_invmatch("wan_proto", "dhcp", "disabled"); %> does not produce
 * <% nvram_invmatch("wan_proto", "static", "disabled"); %> produces "disabled"
 */
static int
ej_nvram_invmatch(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *invmatch, *output;

	if (ejArgs(argc, argv, "%s %s %s", &name, &invmatch, &output) < 3) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if (nvram_invmatch(name, invmatch))
		return websWrite(wp, output);

	return 0;
}	

/*
 * Example: 
 * HEARTBEAT_SUPPORT = 1
 * <% support_match("HEARTBEAT_SUPPORT", "0", "selected"); %> does not produce
 * <% support_match("HEARTBEAT_SUPPORT", "1", "selected"); %> produces "selected"
 */
static int
ej_support_match(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *value, *output;
	struct support_list *v;

	if (ejArgs(argc, argv, "%s %s %s", &name, &value, &output) < 3) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	
	if(!strcmp(name, "WL_STA_SUPPORT") ||
	   !strcmp(name, "SYSLOG_SUPPORT"))	return 1;
	
	for(v = supports ; v < &supports[SUPPORT_COUNT] ; v++) {
   		if(!strcmp(v->supp_name, name) && !strcmp(v->supp_value, value)){
			return websWrite(wp, output);
		}
   	}
	
	return 1;	
}	


/*
 * Example: 
 * HEARTBEAT_SUPPORT = 1
 * <% support_invmatch("HEARTBEAT_SUPPORT", "1", "<!--"); %> does not produce
 * HEARTBEAT_SUPPORT = 0
 * <% support_invmatch("HEARTBEAT_SUPPORT", "1", "-->"); %> produces "-->"
 */
static int
ej_support_invmatch(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *value, *output;
	struct support_list *v;

	if (ejArgs(argc, argv, "%s %s %s", &name, &value, &output) < 3) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	
	if(!strcmp(name, "WL_STA_SUPPORT") ||
	   !strcmp(name, "SYSLOG_SUPPORT"))
		return websWrite(wp, output);

	for(v = supports ; v < &supports[SUPPORT_COUNT] ; v++) {
   		if(!strcmp(v->supp_name, name)){
			if (strcmp(v->supp_value, value)){
				return websWrite(wp, output);
			}
			else
				return 1;
		}
   	}
	
	return websWrite(wp, output);
}

/*
 * Example: 
 * HEARTBEAT_SUPPORT = 1
 * <% support_elsematch("HEARTBEAT_SUPPORT", "1", "black", "red"); %> procude "black"
 */
static int
ej_support_elsematch(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name, *value, *output1, *output2;
	struct support_list *v;

	if (ejArgs(argc, argv, "%s %s %s %s", &name, &value, &output1, &output2) < 3) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if(!strcmp(name, "WL_STA_SUPPORT") ||
	   !strcmp(name, "SYSLOG_SUPPORT"))
		return websWrite(wp, output2);
	
	for(v = supports ; v < &supports[SUPPORT_COUNT] ; v++) {
   		if(!strcmp(v->supp_name, name) && !strcmp(v->supp_value, value)){
			return websWrite(wp, output1);
		}
   	}
	
	return websWrite(wp, output2);	
}	

static int
ej_scroll(int eid, webs_t wp, int argc, char_t **argv)
{
	char *type;
	int y;

	if (ejArgs(argc, argv, "%s %d", &type, &y) < 2) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}
	if(gozila_action)
		websWrite(wp, "%d", y);
	else
		websWrite(wp, "0");
		
	return 0;
}
/*
 * Example: 
 * filter_mac=00:12:34:56:78:00 00:87:65:43:21:00
 * <% nvram_list("filter_mac", 1); %> produces "00:87:65:43:21:00"
 * <% nvram_list("filter_mac", 100); %> produces ""
 */
static int
ej_nvram_list(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name;
	int which;
	char word[256], *next;
	int ret = 0;

	if (ejArgs(argc, argv, "%s %d", &name, &which) < 2) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	foreach(word, nvram_safe_get(name), next) {
		if (which-- == 0)
			ret += websWrite(wp, word);
	}

	return ret;
}

/* Example: 
 * wan_dns = 168.95.1.1 210.66.161.125 168.95.192.1
 * <% get_dns_ip("wan_dns", "1", "2"); %> produces "161"
 * <% get_dns_ip("wan_dns", "2", "3"); %> produces "1"
 */
int
ej_get_dns_ip(int eid, webs_t wp, int argc, char_t **argv)
{
	char *name;
	int count, which;
	char word[256], *next;

	if (ejArgs(argc, argv, "%s %d %d", &name, &which, &count) < 3) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	foreach(word, nvram_safe_get(name), next) {
		if (which-- == 0)
			return websWrite(wp, "%d", get_single_ip(word,count));
	}

	return websWrite(wp, "0");	// not find
}


static unsigned long
inet_atoul(char *cp)
{
	struct in_addr in;

	(void) inet_aton(cp, &in);
	return in.s_addr;
}

int
valid_wep_key(webs_t wp, char *value, struct variable *v)
{
	int i;

	switch(strlen(value)){
		case 5:
		case 13: 
			for(i=0 ; *(value+i) ; i++){
                                if(isascii(*(value+i))){ 
                                        continue;
                                }
                                else{ 
                                        websDebugWrite(wp, "Invalid <b>%s</b> %s: must be ascii code<br>",
                                            v->longname, value);
                                        return FALSE;
                                }
                         }
			break;
		case 10:
		case 26:
			for(i=0 ; *(value+i) ; i++){
		                if(isxdigit(*(value+i))){ /* one of 0 1 2 3 4 5 6 7 8 9 a b c d e f A B C D E F */
                		        continue;
          		        }
         		        else{   
                    		        websDebugWrite(wp, "Invalid <b>%s</b> %s: must be hexadecimal digits<br>",
                              		    v->longname, value);
                     			return FALSE;
               			}
       			 }
			 break;

		default: websDebugWrite(wp, "Invalid <b>%s</b>: must be 5 or 13 ASCII characters or 10 or 26 hexadecimal digits<br>", v->longname); return FALSE;

	}

/*
	for(i=0 ; *(value+i) ; i++){
		if(isxdigit(*(value+i))){ 
			continue;
		}
		else{
			websDebugWrite(wp, "Invalid <b>%s</b> %s: must be hexadecimal digits<br>",
				  v->longname, value);
			return FALSE;
		}
	}

	if (i != length) {
		websDebugWrite(wp, "Invalid <b>%s</b> %s: must be %d characters<br>",
			  v->longname, value,length);
		return FALSE;
	}
*/
	return TRUE;
}

int
valid_netmask(webs_t wp, char *value, struct variable *v)
{

	if(!legal_netmask(value)){
		websDebugWrite(wp, "Invalid <b>%s</b> %s: not a legal netmask<br>",
			  v->longname, value);
		return FALSE;
	}

	return TRUE;

}

static void
validate_netmask(webs_t wp, char *value, struct variable *v)
{
	if (valid_netmask(wp, value, v))
		nvram_set(v->name, value);
}

static void
validate_merge_netmask(webs_t wp, char *value, struct variable *v)
{
	char *netmask, maskname[30];
	char *mask;
	int i;

	netmask = malloc(20);
	strcpy(netmask,"");
	for(i=0 ; i<4 ; i++){
		snprintf(maskname,sizeof(maskname),"%s_%d",v->name,i);
		mask = websGetVar(wp, maskname , NULL);
		if(mask){
			strcat(netmask,mask);
			if(i<3)	strcat(netmask,".");
		}
		else
			return ;
	}

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): netmask=[%s]",__FUNCTION__,netmask);
#endif

	if(valid_netmask(wp, netmask, v))
		nvram_set(v->name, netmask);

	if(netmask)	free(netmask);
}

//Added by Daniel(2004-07-29) for EZC
char webs_buf[5000];
int webs_buf_offset=0;

static void
validate_list(webs_t wp, char *value, struct variable *v,
	      int (*valid)(webs_t, char *, struct variable *))
{
	int n, i;
	char name[100];
	char buf[1000] = "", *cur = buf;

	n = atoi(value);

	for (i = 0; i < n; i++) {
		snprintf(name, sizeof(name), "%s%d", v->name, i);
#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%d): [%s]=[%s]", __FUNCTION__, i, name, websGetVar(wp, name, NULL));
#endif
		if (!(value = websGetVar(wp, name, NULL)))
			return;
		if (!*value && v->nullok)
			continue;
		if (!valid(wp, value, v))
			continue;
		cur += snprintf(cur, buf + sizeof(buf) - cur, "%s%s",
			cur == buf ? "" : " ", value);
	}
	nvram_set(v->name, buf);

}	

int
valid_ipaddr(webs_t wp, char *value, struct variable *v)
{
	struct in_addr netaddr, netmask;
	
	if (!legal_ipaddr(value)) {
		websDebugWrite(wp, "Invalid <b>%s</b> %s: not an IP address<br>",
			  v->longname, value);
		return FALSE;
	}

	if (v->argv) {
		if(!strcmp(v->argv[0],"lan")){
			if(*(value+strlen(value)-2)=='.' && *(value+strlen(value)-1)=='0'){
				websDebugWrite(wp, "Invalid <b>%s</b> %s: not an IP address<br>",
			 		 v->longname, value);
				return FALSE;
			}
		}
		
		else if(!legal_ip_netmask(v->argv[0],v->argv[1],value)){
			(void) inet_aton(nvram_safe_get(v->argv[0]), &netaddr);
			(void) inet_aton(nvram_safe_get(v->argv[1]), &netmask);
			websDebugWrite(wp, "Invalid <b>%s</b> %s: not in the %s/",
				  v->longname, value, inet_ntoa(netaddr));
			websDebugWrite(wp, "%s network<br>", inet_ntoa(netmask));
			return FALSE;
		}
	}

	return TRUE;
}

static void
validate_ipaddr(webs_t wp, char *value, struct variable *v)
{
	if (valid_ipaddr(wp, value, v))
		nvram_set(v->name, value);
}

static void
validate_ipaddrs(webs_t wp, char *value, struct variable *v)
{
	validate_list(wp, value, v, valid_ipaddr);
}

int
valid_merge_ip_4(webs_t wp, char *value, struct variable *v)
{
	char *ipaddr;

	if(atoi(value) == 255){
			websDebugWrite(wp, "Invalid <b>%s</b> %s: out of range 0 - 254 <br>",
				  v->longname, value);
			return FALSE;
	}

	ipaddr = malloc(20);
	sprintf(ipaddr,"%d.%d.%d.%s",get_single_ip(nvram_safe_get("lan_ipaddr"),0),
				     get_single_ip(nvram_safe_get("lan_ipaddr"),1),
				     get_single_ip(nvram_safe_get("lan_ipaddr"),2),
				     value);

	if (!valid_ipaddr(wp, ipaddr, v)){
		free(ipaddr);
		return FALSE;
	}
	
	if(ipaddr) free(ipaddr);
	
	return TRUE;
}

static void
validate_merge_ip_4(webs_t wp, char *value, struct variable *v)
{
	if(!strcmp(value,"")){
		nvram_set(v->name, "0");
		return ;
	}	

	if(valid_merge_ip_4(wp, value, v))
		nvram_set(v->name, value);
}

static void
validate_merge_ipaddrs(webs_t wp, char *value, struct variable *v)
{
	char ipaddr[20];

	get_merge_ipaddr(v->name, ipaddr);

	if(valid_ipaddr(wp, ipaddr, v))
		nvram_set(v->name, ipaddr);
}

static void
validate_merge_mac(webs_t wp, char *value, struct variable *v)
{
	char macaddr[20];

	get_merge_mac(v->name, macaddr);

	if(valid_hwaddr(wp, macaddr, v))
		nvram_set(v->name, macaddr);

}

static void
validate_dns(webs_t wp, char *value, struct variable *v)
{
	char buf[100] = "", *cur = buf;
	char ipaddr[20], ipname[30];
	char *ip;
	int i, j;

	for(j=0; j<3 ; j++){
		strcpy(ipaddr,"");
		for(i=0 ; i<4 ; i++){
			snprintf(ipname,sizeof(ipname),"%s%d_%d",v->name,j,i);
			ip = websGetVar(wp, ipname , NULL);
			if(ip){
				strcat(ipaddr,ip);
				if(i<3)	strcat(ipaddr,".");
			}
			else
				return ;
		}

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): ipaddr=[%s]",__FUNCTION__,ipaddr);
#endif
		if(!strcmp(ipaddr,"0.0.0.0"))
			continue;
		if(!valid_ipaddr(wp, ipaddr, v))
			continue;
		cur += snprintf(cur, buf + sizeof(buf) - cur, "%s%s",
				cur == buf ? "" : " ", ipaddr);
	}
	nvram_set(v->name, buf);

	dns_to_resolv();
}

int
valid_choice(webs_t wp, char *value, struct variable *v)
{
	char **choice;

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s):",__FUNCTION__,value);
#endif
	for (choice = v->argv; *choice; choice++) {
		if (!strcmp(value, *choice))
			return TRUE;
	}

	websDebugWrite(wp, "Invalid <b>%s</b> %s: not one of ", v->longname, value);
	for (choice = v->argv; *choice; choice++)
		websDebugWrite(wp, "%s%s", choice == v->argv ? "" : "/", *choice);
	websDebugWrite(wp, "<br>");
	return FALSE;
}

void
validate_choice(webs_t wp, char *value, struct variable *v)
{
	if (valid_choice(wp, value, v))
		nvram_set(v->name, value);
}

int
valid_range(webs_t wp, char *value, struct variable *v)
{
	int n, start, end;

	n = atoi(value);
	start = atoi(v->argv[0]);
	end = atoi(v->argv[1]);

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): start=[%s] end=[%s]",__FUNCTION__,value,v->argv[0],v->argv[1]);
#endif

	if (!ISDIGIT(value,1) || n < start || n > end) {
		websDebugWrite(wp, "Invalid <b>%s</b> %s: out of range %d-%d<br>",
			  v->longname, value, start, end);
		return FALSE;
	}

	return TRUE;
}

static void
validate_range(webs_t wp, char *value, struct variable *v)
{
	char buf[20];
	int range;
	if (valid_range(wp, value, v)){
		range = atoi(value);
		snprintf(buf, sizeof(buf),"%d", range);
		nvram_set(v->name, buf);
	}
}

int
valid_name(webs_t wp, char *value, struct variable *v)
{
	int n, max;

	n = atoi(value);
	max = atoi(v->argv[0]);

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(%s): max=[%d]",__FUNCTION__,value,max);
#endif

	if (!ISASCII(value,1)){
		websDebugWrite(wp, "Invalid <b>%s</b> %s: NULL or have illegal characters<br>",
			  v->longname, value);
		return FALSE;
	}
	if (strlen(value) > max) {
		websDebugWrite(wp, "Invalid <b>%s</b> %s: out of range 1-%d characters<br>",
			  v->longname, value,max);
		return FALSE;
	}

	return TRUE;
}

static void
validate_name(webs_t wp, char *value, struct variable *v)
{
	if (valid_name(wp, value, v))
	    nvram_set(v->name, value);
}

#ifdef MULTIPLE_LOGIN_SUPPORT
static void
validate_http_name(webs_t wp, char *value, struct variable *v)
{
	char buf[1000] = "", *cur = buf;
	char word[1024], *next, *accounts;
	char delim[] = "<&nbsp;>";
	char *name,*psw,*mod;
	int count = 0;

	accounts = nvram_safe_get("http_login");
	if (valid_name(wp, value, v))
	{
	    split(word, accounts, next, delim){
		int len = 0;

		if ((name=strstr(word, "$NAME:")) == NULL ||
		    (psw=strstr(word, "$PSW:")) == NULL ||
		    (mod=strstr(word, "$MOD:")) == NULL)
		    continue;

		if(count == 0)
		{
			strcpy(name, value);
		}
		else
		{
			/* $NAME */
			if (sscanf(name, "$NAME:%3d:", &len) != 1)
			    continue;
			strncpy(name, name + sizeof("$NAME:nnn:") - 1, len);
                        name[len] = '\0';
		}

		/* $PSW */
		if (sscanf(psw, "$PSW:%3d:", &len) != 1)
		    continue;
		strncpy(psw, psw + sizeof("$PSW:nnn:") - 1, len);
		psw[len] = '\0';

		/* $MOD */
		if (sscanf(mod, "$MOD:%3d:", &len) != 1)
		    continue;
		strncpy(mod, mod + sizeof("$MOD:nnn:") - 1, len);
		mod[len] = '\0';

		cur += snprintf(cur, buf + sizeof(buf) - cur, "%s$NAME:%03d:%s$PSW:%03d:%s$MOD:%03d:%s",
			cur == buf ? "" : "<&nbsp;>",
			strlen(name), name,
			strlen(psw), psw,
			strlen(mod), mod);
		count ++;
	    }
	    nvram_set("http_login", buf);
	    nvram_set("http_username", value);
#ifdef TINYLOGIN_SUPPORT
	    set_tinylogin_info();
#endif
	}
}

static void
validate_http_password(webs_t wp, char *value, struct variable *v)
{
	char buf[1000] = "", *cur = buf;
	char word[1024], *next, *accounts;
	char delim[] = "<&nbsp;>";
	char *name,*psw,*mod;
	int count = 0;

	if (strcmp(value, TMP_PASSWD) && valid_name(wp, value, v))
	{
	accounts = nvram_safe_get("http_login");
	split(word, accounts, next, delim){
		int len = 0;

		if ((name=strstr(word, "$NAME:")) == NULL ||
		    (psw=strstr(word, "$PSW:")) == NULL ||
		    (mod=strstr(word, "$MOD:")) == NULL)
		    continue;

		/* $NAME */
		if (sscanf(name, "$NAME:%3d:", &len) != 1)
		    continue;
		strncpy(name, name + sizeof("$NAME:nnn:") - 1, len);
                       name[len] = '\0';

		if(count == 0)
		{
			strcpy(psw, value);
		}
		else
		{
			/* $PSW */
			if (sscanf(psw, "$PSW:%3d:", &len) != 1)
			    continue;
			strncpy(psw, psw + sizeof("$PSW:nnn:") - 1, len);
			psw[len] = '\0';
		}
		/* $MOD */
		if (sscanf(mod, "$MOD:%3d:", &len) != 1)
		    continue;
		strncpy(mod, mod + sizeof("$MOD:nnn:") - 1, len);
		mod[len] = '\0';

		cur += snprintf(cur, buf + sizeof(buf) - cur, "%s$NAME:%03d:%s$PSW:%03d:%s$MOD:%03d:%s",
			cur == buf ? "" : "<&nbsp;>",
			strlen(name), name,
			strlen(psw), psw,
			strlen(mod), mod);
		count ++;
		}
		nvram_set("http_login", buf);
	    	nvram_set("http_passwd", value);
#ifdef TINYLOGIN_SUPPORT
	    set_tinylogin_info();
#endif
#ifdef DDM_SUPPORT
		nvram_set("DDM_pass_flag","1");
#endif
	}
}

static void
validate_http_user_password(webs_t wp, char *value, struct variable *v)
{

	char buf[1000] = "", *cur = buf;
	char word[1024], *next, *accounts, *userid;
	char delim[] = "<&nbsp;>";
	char *name,*psw,*mod;

	if (strcmp(value, TMP_PASSWD) && valid_name(wp, value, v))
	{
	accounts = nvram_safe_get("http_login");
	userid = nvram_safe_get("current_login_name");

	split(word, accounts, next, delim){
		int len = 0;

		if ((name=strstr(word, "$NAME:")) == NULL ||
		    (psw=strstr(word, "$PSW:")) == NULL ||
		    (mod=strstr(word, "$MOD:")) == NULL)
		    continue;

		/* $NAME */
		if (sscanf(name, "$NAME:%3d:", &len) != 1)
		    continue;
		strncpy(name, name + sizeof("$NAME:nnn:") - 1, len);
                       name[len] = '\0';

		if(!strcmp(userid , name))
			strcpy(psw, value);
		else{
			if (sscanf(psw, "$PSW:%3d:", &len) != 1)
			    continue;
			strncpy(psw, psw + sizeof("$PSW:nnn:") - 1, len);
			psw[len] = '\0';
		}

		/* $MOD */
		if (sscanf(mod, "$MOD:%3d:", &len) != 1)
		    continue;
		strncpy(mod, mod + sizeof("$MOD:nnn:") - 1, len);
		mod[len] = '\0';

		cur += snprintf(cur, buf + sizeof(buf) - cur, "%s$NAME:%03d:%s$PSW:%03d:%s$MOD:%03d:%s",
			cur == buf ? "" : "<&nbsp;>",
			strlen(name), name,
			strlen(psw), psw,
			strlen(mod), mod);
		}
		nvram_set("http_login", buf);
#ifdef TINYLOGIN_SUPPORT
	    set_tinylogin_info();
#endif
#ifdef DDM_SUPPORT
		nvram_set("DDM_pass_flag","1");
#endif
	}
}
#endif

#ifdef SNMP_SUPPORT
static void
validate_name_snmp(webs_t wp, char *value, struct variable *v)
{
 	if (valid_name(wp, value, v))
 	    nvram_set(v->name, value);
// 	fprintf(stderr,"exit validate_name_snmp\r\n");
}
#endif
/* the html always show "d6nw5v1x2pc7st9m"
 * so we must filter it.
 */
static void
validate_password(webs_t wp, char *value, struct variable *v)
{
	if (strcmp(value, TMP_PASSWD) && valid_name(wp, value, v))
	{	
		nvram_set(v->name, value);
#ifdef DDM_SUPPORT
		if( !strcmp(v->name,"http_passwd") )
		{	
			nvram_set("DDM_pass_flag","1");		
		}	
#endif		
	}	
}


int
valid_hwaddr(webs_t wp, char *value, struct variable *v)
{
	/* Make exception for "NOT IMPLELEMENTED" string */
	if (!strcmp(value,"NOT_IMPLEMENTED")) 
		return(TRUE);

	/* Check for bad, multicast, broadcast, or null address */
	if (!legal_hwaddr(value)){
		websDebugWrite(wp, "Invalid <b>%s</b> %s: not a legal MAC address<br>",
			  v->longname, value);
		return FALSE;
	}

	return TRUE;
}

static void
validate_hwaddr(webs_t wp, char *value, struct variable *v)
{
	if (valid_hwaddr(wp, value, v))
		nvram_set(v->name, value);
}

static void
validate_hwaddrs(webs_t wp, char *value, struct variable *v)
{
	validate_list(wp, value, v, valid_hwaddr);
}

int
ej_get_http_prefix(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;
	char http[10];
	char ipaddr[20];
	char port[10];

	char *http_enable = websGetVar(wp, "http_enable", NULL);
#ifdef HTTPS_SUPPORT
	char *https_enable = websGetVar(wp, "https_enable", NULL);
#endif
	char *action = websGetVar(wp, "action", NULL);

#ifdef HTTPS_SUPPORT	
	if(do_ssl && !http_enable && !https_enable) {
			strcpy(http, "https");
	}
	else if(do_ssl && http_enable && https_enable) {
		if(atoi(https_enable) && atoi(http_enable))
			strcpy(http, "https");
		else if(atoi(https_enable) && !atoi(http_enable))
			strcpy(http, "https");
		else	// !atoi(https_enable) && atoi(http_enable)
			strcpy(http, "http");
	}
	else if(do_ssl && !http_enable && !https_enable) {
			strcpy(http, "http");
	}
	else if(!do_ssl && http_enable && https_enable) {
		if(atoi(https_enable) && atoi(http_enable))
			strcpy(http, "http");
		else if(atoi(https_enable) && !atoi(http_enable))
			strcpy(http, "https");
		else	// !atoi(https_enable) && atoi(http_enable)
			strcpy(http, "http");
	}
	else
#endif
		strcpy(http, "http");

	if(browser_method == USE_LAN || !action) { // Use LAN to browser
		if(nvram_match("restore_defaults", "1")) {
			strcpy(ipaddr, "192.168.1.1");	// Deafult IP
			strcpy(http, "http");
		}
		else
			strcpy(ipaddr, nvram_safe_get("lan_ipaddr"));
		strcpy(port, "");
	}
	else {
		strcpy(ipaddr, nvram_safe_get("wan_ipaddr"));
		sprintf(port, ":%s", nvram_safe_get("http_wanport"));
	}

	ret = websWrite(wp, "%s://%s%s/", http, ipaddr, port);

	return ret;
}

int ej_get_mtu(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;
	struct mtu_lists *mtu_list;
	char *type;
	char *proto = GOZILA_GET("wan_proto");

	if (ejArgs(argc, argv, "%s", &type) < 1) {
                websError(wp, 400, "Insufficient args\n");
                return -1;
        }

	mtu_list = get_mtu(proto);

	if(!strcmp(type, "min"))
		ret = websWrite(wp, "%s", mtu_list->min);
	else if(!strcmp(type, "max"))
		ret = websWrite(wp, "%s", mtu_list->max);

	return ret;
}


/* 
 * Variables are set in order (put dependent variables later). Set
 * nullok to TRUE to ignore zero-length values of the variable itself.
 * For more complicated validation that cannot be done in one pass or
 * depends on additional form components or can throw an error in a
 * unique painful way, write your own validation routine and assign it
 * to a hidden variable (e.g. filter_ip).
 */
struct variable variables[] = {
	/* for index */
	{ "lan_ipaddr", "LAN IP Address", validate_lan_ipaddr, ARGV("lan"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "lan_netmask", "LAN Subnet Mask", validate_choice, ARGV("255.255.255.0","255.255.255.128","255.255.255.192","255.255.255.224","255.255.255.240","255.255.255.248","255.255.255.252"), FALSE },
	{ "router_name", "Routert Name", validate_name, ARGV("255"), TRUE, 0 },
	{ "wan_hostname","WAN Host Name", validate_name, ARGV("255"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wan_domain", "WAN Domain Name", validate_name, ARGV("255"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wan_ipaddr", "WAN IP Address", validate_wan_ipaddr, NULL, FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_ipaddr", "WAN IP Address", validate_merge_ipaddrs, NULL, FALSE },
	//{ "wan_netmask", "WAN Subnet Mask", validate_merge_netmask, FALSE },
	//{ "wan_gateway", "WAN Gateway", validate_merge_ipaddrs, ARGV("wan_ipaddr","wan_netmask"), FALSE },
	{ "wan_proto", "WAN Protocol", validate_choice, ARGV("dhcp", "static", "pppoe", "pptp", "l2tp", "heartbeat","auto"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "ntp_server", "NTP Server", NULL, NULL, TRUE, 0 },  // not use 
	{ "ntp_mode", "NTP Mode", validate_choice, ARGV("manual","auto"), TRUE, 0 },
	{ "daylight_time", "Daylight", validate_choice, ARGV("0", "1"), TRUE, 0 },
	{ "time_zone", "Time Zone", validate_choice, ARGV("-12 1 0","-11 1 0","-10 1 0","-09 1 1","-08 1 1","-07 1 0","-07 2 1","-06 1 0","-06 2 1","-05 1 0","-05 2 1","-04 1 0","-04 2 1","-03.5 1 1","-03 1 0","-03 2 1","-02 1 0","-01 1 2","+00 1 0","+00 2 2","+01 1 0","+01 2 2","+02 1 0","+02 2 2","+03 1 0","+04 1 0","+05 1 0","+05.5 1 0","+06 1 0","+07 1 0","+08 1 0","+08 2 0","+09 1 0","+10 1 0","+10 2 4","+11 1 0","+12 1 0","+12 2 4"), FALSE, 0 },
	//{ "pptp_server_ip", "WAN Gateway", validate_merge_ipaddrs, ARGV("wan_ipaddr","wan_netmask"), FALSE },
	{ "ppp_username", "Username", validate_name, ARGV("63"), FALSE, 0 },
	{ "ppp_passwd", "Password", validate_password, ARGV("63"), TRUE, 0 },
	{ "ppp_keepalive", "Keep Alive", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ppp_demand", "Connect on Demand", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ppp_idletime", "Max Idle Time", validate_range, ARGV("1", "9999"), FALSE, 0 },
	{ "ppp_redialperiod", "Redial Period", validate_range, ARGV("1", "9999"), FALSE, 0 },
	{ "ppp_service", "Service Name", validate_name, ARGV("63"), TRUE, 0 },	// 2003-03-19 by honor
	{ "ppp_static", "Enable /Disable Static IP", validate_choice, ARGV("0", "1"), TRUE, 0 },
	{ "ppp_static_ip", "Static IP", validate_merge_ipaddrs, NULL, FALSE, 0 },
#ifdef MPPPOE_SUPPORT
	{ "mpppoe_enable", "Multi PPPoE", validate_choice, ARGV("0","1"), FALSE, 0 },
	{ "mpppoe_dname", "Match Doamin Name", validate_name, ARGV("63"), FALSE, 0 },
	{ "ppp_username_1", "Username", validate_name, ARGV("63"), FALSE, 0 },
	{ "ppp_passwd_1", "Password", validate_password, ARGV("63"), TRUE, 0 },
	{ "ppp_keepalive_1", "Keep Alive", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ppp_demand_1", "Connect on Demand", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ppp_idletime_1", "Max Idle Time", validate_range, ARGV("1", "9999"), FALSE, 0 },
	{ "ppp_redialperiod_1", "Redial Period", validate_range, ARGV("1", "9999"), FALSE, 0 },
	{ "ppp_service_1", "Service Name", validate_name, ARGV("63"), TRUE, 0 },	// 2003-03-19 by honor
#endif
	/* for index, dhcp */
	{ "wan_dns", "WAN DNS Server", validate_dns, NULL, FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	/* for dhcp */
#ifdef VERIZON_LAN_SUPPORT
	{ "lan_proto", "LAN Protocol", validate_lan_proto, ARGV("dhcp", "static", "dhcprelay"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
#else
	{ "lan_proto", "LAN Protocol", validate_choice, ARGV("dhcp", "static"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
#endif
	{ "dhcp_check", "DHCP check", dhcp_check, NULL, FALSE, 0 },
	{ "dhcp_start", "DHCP Server LAN IP Address Range", validate_range, ARGV("0","255"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "dhcp_start", "DHCP Server LAN IP Address Range", validate_merge_ip_4, NULL, FALSE },
	{ "dhcp_num", "DHCP Users", validate_range, ARGV("1","253"), FALSE, 0 },
	{ "dhcp_lease", "DHCP Client Lease Time", validate_range, ARGV("0","99999"), FALSE, 0 },
	{ "wan_wins", "WAN WINS Server", validate_merge_ipaddrs, NULL, FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	/* for password */
#ifdef MULTIPLE_LOGIN_SUPPORT //r
	{ "http_username", "Router Username", validate_http_name, ARGV("63"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "http_passwd", "Router Password", validate_http_password, ARGV("63"), TRUE, EZC_FLAGS_WRITE },
	{ "http_user_passwd", "Router Password", validate_http_user_password, ARGV("63"), TRUE, EZC_FLAGS_WRITE },
#else
	{ "http_username", "Router Username", validate_name, ARGV("63"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "http_passwd", "Router Password", validate_password, ARGV("63"), TRUE, EZC_FLAGS_WRITE },
#endif
	{ "http_method", "http method", validate_choice, ARGV("post","get"), TRUE, 0 },
	{ "upnp_enable", "UPnP", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "upnp_ssdp_interval", "UPnP SSDP Interval", validate_range, ARGV("0", "9999"), FALSE, 0 },
	{ "upnp_max_age", "Max Age", validate_range, ARGV("0", "9999"), FALSE, 0 },
	{ "web_wl_filter", "Wireless Access Web", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "http_enable", "HTTP Server", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "https_enable", "HTTPS Server", validate_choice, ARGV("0", "1"), FALSE, 0 },
	/* for log */
	{ "log_level", "Connection Logging", validate_range, ARGV("0", "3"), FALSE, 0 },
	{ "log_enable", "Access log", validate_choice, ARGV("0", "1"), FALSE, 0 },
#ifdef SYSLOG_SUPPORT
	{ "log_ipaddr", "Log server ip", validate_range, ARGV("0","255"), FALSE, 0 },
	{ "log_settings", "Log settings", validate_log_settings, NULL, FALSE, 0 },
	{ "log_show_all", "Show all log messages", validate_choice, ARGV("0", "1"), FALSE, 0 },
#endif
	/* for filter */
	{ "filter", "Firewall Protection", validate_choice, ARGV("on", "off"), FALSE, 0 },
	{ "filter_policy", "Filter", validate_filter_policy, NULL, FALSE, 0 },
	{ "filter_ip_value", "TCP/UDP IP Filter", validate_filter_ip_grp, NULL, FALSE, 0 },
	{ "filter_port", "TCP/UDP Port Filter", validate_filter_port, NULL, FALSE, 0 },
	{ "filter_dport_value", "TCP/UDP Port Filter", validate_filter_dport_grp, NULL, FALSE, 0 },
	{ "blocked_service", "TCP/UDP Port Filter", validate_blocked_service, NULL, FALSE, 0 },
	{ "filter_mac_value", "TCP/UDP MAC Filter", validate_filter_mac_grp, NULL, FALSE, 0 },
	{ "filter_web", "Website Filter", validate_filter_web, NULL, FALSE, 0 },
	{ "block_wan", "Block WAN Request", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ident_pass", "IDENT passthrough", validate_choice, ARGV("0", "1"), TRUE, 0 },
	{ "block_loopback", "Filter Internet NAT redirection", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "block_proxy", "Block Proxy", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "block_java", "Block Java", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "block_activex", "Block ActiveX", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "block_cookie", "Block Cookie", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "multicast_pass", "Multicast Pass Through", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ipsec_pass", "IPSec Pass Through", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "pptp_pass", "PPTP Pass Through", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "l2tp_pass", "L2TP Pass Through", validate_choice, ARGV("0", "1"), FALSE, 0 },
#ifdef PPPOE_RELAY_SUPPORT
	{ "pppoe_pass", "PPPoE Pass Through", validate_choice, ARGV("0", "1"), FALSE, 0 },
#endif
	{ "remote_management", "Remote Management", validate_choice, ARGV("0", "1"), FALSE, 0 },
#ifdef HTTPS_SUPPORT
	{ "remote_mgt_https", "Remote Management use https", validate_choice, ARGV("0", "1"), FALSE, 0 },
#endif
	{ "http_wanport", "Router WAN Port", validate_range, ARGV("0", "65535"), TRUE, 0 },
	{ "remote_upgrade", "Remote Upgrade", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "mtu_enable", "MTU enable", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "wan_mtu", "WAN MTU", validate_range, ARGV("576","1500"), FALSE, 0 },
	/* for forward */
	{ "forward_port", "TCP/UDP Port Forward", validate_forward_proto, NULL, FALSE, 0 },
#ifdef UPNP_FORWARD_SUPPORT
	{ "forward_upnp", "TCP/UDP UPnP Forward", validate_forward_upnp, NULL, FALSE, 0 },
#endif
#ifdef ALG_FORWARD_SUPPORT
	{ "forward_alg", "TCP/UDP ALG Forward", validate_forward_alg, NULL, FALSE, 0 },
#endif
#ifdef SPECIAL_FORWARD_SUPPORT
	{ "forward_spec", "TCP/UDP special Forward", validate_forward_spec, NULL, FALSE, 0 },
#endif
#ifdef PORT_TRIGGER_SUPPORT
	   /* for port trigger */
	{ "port_trigger", "TCP/UDP Port Trigger", validate_port_trigger, NULL, FALSE, 0 },
#endif
	/* for static route */
	{ "static_route", "Static Route", validate_static_route, NULL, FALSE, 0 },
	/* for dynamic route */
	{ "wk_mode", "Working Mode", validate_dynamic_route, ARGV("gateway", "router"), FALSE, 0 },
	//{ "dr_setting", "Dynamic Routing", validate_choice, ARGV("0", "1", "2", "3"), FALSE },
	//{ "dr_lan_tx", "Dynamic Routing LAN TX", validate_choice, ARGV("0","1 2"), FALSE },
	//{ "dr_lan_rx", "Dynamic Routing LAN RX", validate_choice, ARGV("0","1 2"), FALSE },
	//{ "dr_wan_tx", "Dynamic Routing WAN TX", validate_choice, ARGV("0","1 2"), FALSE },
	//{ "dr_wan_rx", "Dynamic Routing WAN RX", validate_choice, ARGV("0","1 2"), FALSE },
	/* for dmz */
	{ "dmz_enable", "DMZ enable", validate_choice, ARGV("0", "1"), FALSE, 0  },
	{ "dmz_ipaddr", "DMZ LAN IP Address", validate_range, ARGV("0","255"), FALSE, 0  },
	/* for MAC Clone */
	{ "mac_clone_enable", "User define WAN MAC Address", validate_choice, ARGV("0","1"), TRUE, 0 },
	{ "def_hwaddr", "User define WAN MAC Address", validate_merge_mac, NULL, TRUE, 0 },
	/* for upgrade */
	{ "upgrade_enable", "Tftp upgrade", validate_choice, ARGV("0", "1"), FALSE, 0 },
	/* for wireless */	
	{ "wl_enable", "Enable Wireless", validate_choice, ARGV("0","1"), TRUE, 0 },
	{ "wl_ssid", "Network Name (SSID)", validate_name, ARGV("32"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
        { "wl_closed", "Network Type", validate_choice, ARGV("0", "1"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
        { "wl_ap_isolate", "AP Isolate", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "wl_country", "Country", validate_choice, ARGV("Worldwide", "Thailand", "Israel", "Jordan", "China", "Japan", "USA", "Europe", "USA Low", "Japan High", "All"), FALSE, 0 },
        { "wl_mode", "AP Mode", validate_choice, ARGV("ap", "sta", "wds"), FALSE, 0 },
        { "wl_lazywds", "Bridge Restrict", validate_choice, ARGV("0", "1"), FALSE, 0 },
        { "wl_wds", "Remote Bridges", validate_hwaddrs, NULL, TRUE, 0 },
	{ "wl_wds_timeout", "Link Timeout Interval", NULL, NULL, TRUE, 0 },
        /* Order and index matters for WEP keys so enumerate them separately */
        { "wl_WEP_key", "Network Key Index", validate_wl_wep_key, NULL, FALSE, 0 },
        //{ "wl_passphrase", "Network Passphrase", validate_name, ARGV("20"), FALSE },
        //{ "wl_key", "Network Key Index", validate_range, ARGV("1","4"), FALSE },
        //{ "wl_key1", "Network Key 1", validate_wl_key, NULL, TRUE },
        //{ "wl_key2", "Network Key 2", validate_wl_key, NULL, TRUE },
        //{ "wl_key3", "Network Key 3", validate_wl_key, NULL, TRUE },
        //{ "wl_key4", "Network Key 4", validate_wl_key, NULL, TRUE },
        //{ "wl_wep_bit", "WEP Mode", validate_choice, ARGV("64", "128"), FALSE },
        { "wl_wep", "WEP Mode", validate_wl_wep, ARGV("off", "on", "restricted","tkip","aes","tkip+aes"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wl_crypto", "Crypto Mode", validate_choice, ARGV("tkip","aes","tkip+aes"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
        { "wl_auth", "Authentication Mode", validate_wl_auth, ARGV("0", "1"), FALSE, 0 },
	{ "wl_akm", "Authenticated Key Management", validate_wl_akm, NULL, FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
        { "wl_macmode1", "MAC Restrict Mode", validate_macmode, NULL , FALSE, 0 },
        //{ "wl_mac", "Allowed MAC Address", validate_hwaddrs, NULL, TRUE },
	{ "wl_radio", "Radio Enable", validate_choice, ARGV("0", "1"), FALSE, 0 }, //from 11.9
        { "wl_mac_list", "Filter MAC Address", validate_wl_hwaddrs, NULL, FALSE, 0 },
        //{ "wl_active_mac", "Active MAC Address", validate_wl_active_mac, NULL, FALSE },
        { "wl_channel", "802.11g Channel", validate_range, ARGV("1","14"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
        { "wl_rate", "802.11g Rate", validate_choice, ARGV("0", "1000000", "2000000", "5500000", "6000000", "9000000", "11000000", "12000000", "18000000", "24000000", "36000000", "48000000", "54000000"), FALSE, 0 },
        { "wl_rateset", "802.11g Supported Rates", validate_choice, ARGV("all", "default","12"), FALSE, 0 },
        { "wl_frag", "802.11g Fragmentation Threshold", validate_range, ARGV("256", "2346"), FALSE, 0 },
        { "wl_rts", "802.11g RTS Threshold", validate_range, ARGV("0", "2347"), FALSE, 0 },
        { "wl_dtim", "802.11g DTIM Period", validate_range, ARGV("1", "255"), FALSE, 0 },
        { "wl_bcn", "802.11g Beacon Interval", validate_range, ARGV("1", "65535"), FALSE, 0 },
        { "wl_gmode", "802.11g mode", validate_wl_gmode, ARGV("-1", "0", "1", "2", "4", "5", "6"), FALSE, 0 },
        { "wl_net_mode", "802.11g mode", validate_wl_net_mode, ARGV("disabled", "mixed", "b-only", "g-only", "speedbooster"), FALSE, 0 },
	{ "wl_gmode_protection", "54g Protection", validate_choice, ARGV("off", "auto"), FALSE, 0 },
	{ "wl_frameburst", "Frame Bursting", validate_choice, ARGV("off", "on"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wl_plcphdr", "Preamble Type", validate_choice, ARGV("long", "short"), FALSE, 0 },
	{ "wl_maxassoc", "Max Assocation Limit", validate_range, ARGV("1", "256"), FALSE, 0 },
	{ "wl_phytype", "Radio Band", validate_choice, ARGV("a", "b", "g"), TRUE, 0 },
	/* WPA parameters */
	{ "wl_wpa_psk", "WPA Pre-Shared Key", validate_wpa_psk, ARGV("64"), TRUE, EZC_FLAGS_WRITE },
	{ "wl_wpa_gtk_rekey", "WPA GTK Rekey Timer", validate_range, ARGV("0","99999"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wl_radius_ipaddr", "RADIUS Server", validate_merge_ipaddrs, NULL, TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wl_radius_port", "RADIUS Port", validate_range, ARGV("0", "65535"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	{ "wl_radius_key", "RADIUS Shared Secret", validate_name, ARGV("255"), TRUE, EZC_FLAGS_WRITE },
	//{ "wl_auth_mode", "Network Authentication", validate_wl_auth_mode, ARGV("disabled", "radius", "wpa", "psk"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE }, //Masked by Daniel(2004-08-06)
	{ "wl_auth_mode", "Network Authentication", validate_wl_auth_mode, ARGV("radius", "none"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE }, //Modified by Daniel(2004-08-06) for SES
	{ "security_mode", "Security Mode", validate_security_mode, ARGV("disabled", "radius", "wpa", "psk","wep", "wpa2", "psk2", "wpa wpa2", "psk psk2"), FALSE, 0 },
	{ "security_mode2", "Security Mode", validate_security_mode2, ARGV("disabled", "radius", "wep", "wpa_personal", "wpa_enterprise","wpa2_personal", "wpa2_enterprise"), FALSE, 0 },
	{ "wl_net_reauth", "Network Re-auth Interval", NULL, NULL, TRUE, 0 },
        { "wl_preauth", "Network Preauthentication Support", validate_wl_preauth, ARGV("disabled", "enabled"), FALSE, 0 },
	/* MUST leave this entry here after all wl_XXXX variables */
	{ "wl_unit", "802.11 Instance", wl_unit, NULL, TRUE, 0 },
	{ "wl_wme", "WME Support", validate_choice, ARGV("off", "on"), FALSE },
	{ "wl_wme_no_ack", "No-Acknowledgement", validate_noack, ARGV("off", "on"), FALSE},
	{ "wl_wme_ap_bk", "WME AP BK", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_ap_be", "WME AP BE", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_ap_vi", "WME AP VI", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_ap_vo", "WME AP VO", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_sta_bk", "WME STA BK", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_sta_be", "WME STA BE", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_sta_vi", "WME STA VI", validate_wl_wme_params, NULL, TRUE },
	{ "wl_wme_sta_vo", "WME STA VO", validate_wl_wme_params, NULL, TRUE },
	
#ifdef WL_STA_SUPPORT
	{ "wl_ap_ssid", "SSID of associating AP", validate_name, ARGV("32"), TRUE, 0 },
	{ "wl_ap_ip", "Default IP of associating AP", validate_merge_ipaddrs, NULL, TRUE, 0 },
#endif

	/* for DDNS */
	//{ "ddns_enable", "DDNS", validate_choice, ARGV("0", "1"), FALSE },
	//{ "ddns_username", "DDNS username", validate_name, ARGV("63"), FALSE },
	//{ "ddns_passwd", "DDNS password", validate_password, ARGV("63"), FALSE },
	//{ "ddns_hostname", "DDNS hostname", validate_name, ARGV("255"), TRUE },
        //{ "ddns_server", "DDNS server", validate_choice,ARGV("ath.cx","dnsalias.com","dnsalias.net","dnsalias.org","dyndns.biz","dyndns.info","dyndns.org","dyndns.tv","gotdns.com","gotdns.org","homedns.org","homeftp.net","homeftp.org","homeip.net","homelinux.com","homelinux.net","homelinux.org","homeunix.com","homeunix.net","homeunix.org","kicks-ass.net","kicks-ass.org","merseine.nu","mine.nu","serveftp.net"), FALSE },
#ifdef AOL_SUPPORT
	/* for AOL */
	{ "aol_block_traffic2", "AOL Parental Control", validate_choice, ARGV("0","1"), FALSE, 0 },
#endif
#ifdef HSIAB_SUPPORT
	//{ "hsiab_mode", "HSIAB mode", validate_choice, ARGV("1","0"), FALSE },
	//{ "hsiab_provider", "HSIAB provider", validate_choice, ARGV("0","1","2"), FALSE },
	//{ "hsiab_sync", "Sync Config", validate_choice, ARGV("0","1"), FALSE },
#endif
	/* for verizon */
#ifdef VERIZON_LAN_SUPPORT
	{ "dhcrelay_ipaddr", "LAN DHCP Server", validate_dhcrelay_ipaddr, NULL, FALSE },
#endif
#ifdef UDHCPD_STATIC_SUPPORT
	{ "dhcp_statics", "DHCP Static Leases", validate_dhcp_statics, NULL, FALSE },
#endif
#ifdef DHCP_FILTER_SUPPORT
	{ "dhcp_mac_list", "DHCP Filter MAC Address", validate_DHCP_hwaddrs, NULL, FALSE, 0 },
#endif

#ifdef L2TP_SUPPORT
	{ "l2tp_server_ip", "L2TP Server", validate_merge_ipaddrs, NULL, FALSE, 0 },
#endif
#ifdef HEARTBEAT_SUPPORT
	//{ "hb_server_ip", "Heart Beat Server", validate_merge_ipaddrs, NULL, FALSE },		//by tallest
	{ "hb_server_ip", "Heart Beat Server", validate_name, ARGV("63"), TRUE, 0 },	
#endif
#ifdef PARENTAL_CONTROL_SUPPORT
	{ "artemis_enable", "Parental Control", validate_choice, ARGV("0","1"), FALSE, 0 },
#endif
	/* Internal variables */
	{ "os_server", "OS Server", NULL, NULL, TRUE, 0 },
	{ "stats_server", "Stats Server", NULL, NULL, TRUE, 0 },
	/* for radius */
#ifdef HW_QOS_SUPPORT
#if 1 //Added by crazy 20070717 - Fixed issue id 7684, 7693
	/*
	   After clicked the Save Settings button on webpage 
	   'Qos.asp' some times, the DUT will crash when testing 
	   throughput. 
	*/
	{ "ip_forward_disable", "Disable Forward", validate_ip_forward, ARGV("0", "1"), FALSE, 0},
#endif
	{ "wan_speed", "WAN Port switch rate", validate_port_qos, ARGV("0", "1", "2", "3", "4"), FALSE, 0},
	{ "QoS", "QoS switch", validate_port_qos, ARGV("0", "1"), FALSE, 0},
	{ "rate_mode", "QoS Rate Mode", validate_choice, ARGV("0", "1"), FALSE, 0},
	{ "manual_rate", "Manual Rate", validate_range, ARGV("0", "1024000"), FALSE, 0},
	/*{ "sel_qosftp", "Protocol switch", validate_choice, ARGV("0", "1"), FALSE, 0},
	{ "sel_qoshttp", "Protocol switch", validate_choice, ARGV("0", "1"), FALSE, 0},
	{ "sel_qostelnet", "Protocol switch", validate_choice, ARGV("0", "1"), FALSE, 0},
	{ "sel_qossmtp", "Protocol switch", validate_choice, ARGV("0", "1"), FALSE, 0},
	{ "sel_qospop3", "Protocol switch", validate_choice, ARGV("0", "1"), FALSE, 0},*/
	{ "sel_qosport1", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport2", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport3", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport4", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport5", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport6", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport7", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "sel_qosport8", "Protocol switch", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "qos_appname1", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname2", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname3", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname4", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname5", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname6", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname7", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appname8", "App name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_appport1", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport2", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport3", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport4", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport5", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport6", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport7", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_appport8", "App port", validate_name, ARGV("11"), FALSE, 0},
	{ "qos_devname1", "Device name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_devname2", "Device name", validate_name, ARGV("7"), TRUE, 0},
	{ "qos_devmac1", "Device MAC", validate_merge_mac, NULL, TRUE, 0},
	{ "qos_devmac2", "Device MAC", validate_merge_mac, NULL, TRUE, 0},
	{ "qos_devpri1", "Device Pri", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "qos_devpri2", "Device Pri", validate_choice, ARGV("0", "1", "2", "3"), FALSE, 0},
	{ "port_priority_1", "Port Priority", validate_port_qos, ARGV("0", "1", "2"), FALSE, 0},
	{ "port_flow_control_1", "Port flow control", validate_port_qos, ARGV("0", "1"), FALSE, 0},
	{ "port_rate_limit_1", "Port rate limit", validate_port_qos, ARGV("0", "1", "2", "3", "4", "5", "6", "7", "8"), FALSE, 0},
	{ "port_priority_2", "Port Priority", validate_port_qos, ARGV("0", "1", "2"), FALSE, 0},
	{ "port_flow_control_2", "Port flow control", validate_port_qos, ARGV("0", "1"), FALSE, 0},
	{ "port_rate_limit_2", "Port rate limit", validate_port_qos, ARGV("0", "1", "2", "3", "4", "5", "6", "7", "8"), FALSE, 0},
	{ "port_priority_3", "Port Priority", validate_port_qos, ARGV("0", "1", "2"), FALSE, 0},
	{ "port_flow_control_3", "Port flow control", validate_port_qos, ARGV("0", "1"), FALSE, 0},
	{ "port_rate_limit_3", "Port rate limit", validate_port_qos, ARGV("0", "1", "2", "3", "4", "5", "6", "7", "8"), FALSE, 0},
	{ "port_priority_4", "Port Priority", validate_port_qos, ARGV("0", "1", "2"), FALSE, 0},
	{ "port_flow_control_4", "Port flow control", validate_port_qos, ARGV("0", "1"), FALSE, 0},
	{ "port_rate_limit_4", "Port rate limit", validate_port_qos, ARGV("0", "1", "2", "3", "4", "5", "6", "7", "8"), FALSE, 0},
	{ "enable_game", "Enable Gaming", validate_port_qos, ARGV("0", "1"), FALSE, 0},
#if 1 //Added by crazy 20070717 - Fixed issue id 7684, 7693
	/*
	   After clicked the Save Settings button on webpage 
	   'Qos.asp' some times, the DUT will crash when testing 
	   throughput. 
	*/
	{ "ip_forward_enable", "Enable Forward", validate_ip_forward, ARGV("0", "1"), FALSE, 0},
#endif
#endif
         /* for snmp*/
#ifdef SNMP_SUPPORT	
 	{ "snmp_contact", "snmp contact", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "snmp_location", "snmp location", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "snmp_getcom", "snmp getcom", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "snmp_setcom", "snmp setcom", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "snmp_trust", "snmp trust", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "trap_com", "trap com", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "trap_dst", "trap dst", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "snmpv3_username", "snmpv3_username", validate_name_snmp, ARGV("255"), TRUE, 0 },
 	{ "snmpv3_passwd", "snmpv3_passwd", validate_password, ARGV("63"), TRUE, 0 },
#endif
	/* SES Settings */
	{ "ses_enable", "SES", validate_choice, ARGV("0", "1"), FALSE, 0 },
	{ "ses_event", "SES Event", validate_choice, ARGV("0", "3", "4", "6", "7"), FALSE, 0 },
#ifdef GOOGLE_SUPPORT
    // Google secure access
	{ "google_enable", "GSA", validate_google,ARGV("0", "1"), FALSE, 0},
#endif
#ifdef EZC_SUPPORT
	{ "ezc_enable", "EZConfig", validate_choice, ARGV("0", "1"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE},
	//{ "fw_disable", "Firewall", validate_choice, ARGV("0", "1"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "lan_stp", "Spanning Tree Protocol", validate_choice, ARGV("0", "1"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "lan_lease", "DHCP Server Lease Time", validate_range, ARGV("1", "604800"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_desc", "Description", validate_name, ARGV("0", "255"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_hwaddr", "MAC Address", validate_hwaddr, NULL, TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_netmask", "Subnet Mask", validate_ipaddr, NULL, FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_gateway", "Default Gateway", validate_ipaddr, NULL, TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_username", "PPPoE Username", validate_name, ARGV("0", "255"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_passwd", "PPPoE Password", validate_name, ARGV("0", "255"), TRUE, EZC_FLAGS_WRITE },
	//{ "wan_pppoe_service", "PPPoE Service Name", validate_name, ARGV("0", "255"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_ac", "PPPoE Access Concentrator", validate_name, ARGV("0", "255"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_keepalive", "PPPoE Keep Alive", validate_choice, ARGV("0", "1"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_demand", "PPPoE Connect on Demand", validate_choice, ARGV("0", "1"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_idletime", "PPPoE Max Idle Time", validate_range, ARGV("1", "3600"), TRUE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_mru", "PPPoE MRU", validate_range, ARGV("128", "16384"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wan_pppoe_mtu", "PPPoE MTU", validate_range, ARGV("128", "16384"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wl_country_code", "Country Code", validate_country, NULL, FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wl_afterburner", "AfterBurner Technology", validate_wl_afterburner, ARGV("off", "auto"), FALSE, EZC_FLAGS_READ | EZC_FLAGS_WRITE },
	//{ "wl_key", "Network Key Index", validate_range, ARGV("1", "4"), FALSE, EZC_FLAGS_WRITE },
	//{ "wl_key1", "Network Key 1", validate_wl_key, NULL, TRUE, EZC_FLAGS_WRITE},
	//{ "wl_key2", "Network Key 2", validate_wl_key, NULL, TRUE, EZC_FLAGS_WRITE},
	//{ "wl_key3", "Network Key 3", validate_wl_key, NULL, TRUE, EZC_FLAGS_WRITE},
	//{ "wl_key4", "Network Key 4", validate_wl_key, NULL, TRUE, EZC_FLAGS_WRITE},
	
#endif

};

//Added by Daniel(2004-07-29) for EZC
int 
variables_arraysize(void)
{
	return ARRAYSIZE(variables);
}

static void
validate_cgi(webs_t wp)
{
	struct variable *v;
	char *value;

	/* Validate and set variables in table order */
	for (v = variables; v < &variables[STRUCT_LEN(variables)]; v++) {
		value = websGetVar(wp, v->name, NULL);
#ifdef MY_DEBUG
	if(value)
		LOG(LOG_DEBUG,"%s(): [%s]=[%s] nullok=[%d] (%s)",__FUNCTION__,v->name,value ? value :"",v->nullok,!value ? "" : "have");
#endif
		if (!value)
			continue;
		if ((!*value && v->nullok) || !v->validate)
			nvram_set(v->name, value);
		else
			v->validate(wp, value, v);
	}
}

enum {
	NOTHING,
	REBOOT,
	RESTART,
	SERVICE_RESTART,
	REFRESH,
};

struct gozila_action gozila_actions[] = {
	/* bellow for setup page */
	{ "index",		"clone_mac",		"",		1,	REFRESH,		clone_mac},
	{ "WanMAC",		"clone_mac",		"",		1,	REFRESH,		clone_mac},	// for cisco style
	{ "DHCPTable",		"delete",		"",		2,	REFRESH,		delete_leases},
	{ "Status",		"release",		"dhcp_release",	0,	SERVICE_RESTART,	dhcp_release},
	{ "Status",             "renew",		"",             3,      REFRESH,                dhcp_renew},
	{ "Status",		"Connect",		"start_pppoe",	1,	RESTART,		NULL},
	{ "Status_Router",	"release",		"dhcp_release",	0,	SERVICE_RESTART,	dhcp_release},	// for cisco style
	{ "Status_Router",      "renew",		"",             3,      REFRESH,                dhcp_renew},	// for cisco style
	{ "Status",		"Disconnect",		"stop_pppoe",	2,	SERVICE_RESTART,	stop_ppp},
	{ "Status",		"Connect_pppoe",	"start_pppoe",	1,	RESTART,		NULL},
	{ "Status",		"Disconnect_pppoe",	"stop_pppoe",	2,	SERVICE_RESTART,	stop_ppp},
	{ "Status",		"Connect_pptp",		"start_pptp",	1,	RESTART,		NULL},
	{ "Status",		"Disconnect_pptp",	"stop_pptp",	2,	SERVICE_RESTART,	stop_ppp},
	{ "Status",		"Connect_heartbeat",	"start_heartbeat",	1,	RESTART,	NULL},
	{ "Status",		"Disconnect_heartbeat",	"stop_heartbeat",	2,	SERVICE_RESTART,stop_ppp},
	{ "Status_Router",	"Disconnect",		"stop_pppoe",	2,	SERVICE_RESTART,	stop_ppp},	// for cisco style
	{ "Status_Router",	"Connect_pppoe",	"start_pppoe",	1,	SERVICE_RESTART,	NULL},	// for cisco style
	{ "Status_Router",	"Disconnect_pppoe",	"stop_pppoe",	2,	SERVICE_RESTART,	stop_ppp},	// for cisco style
#ifdef MPPPOE_SUPPORT
        { "Status_Router",      "Connect_pppoe_1",      "start_pppoe_1",1,      SERVICE_RESTART,	NULL},  // for cisco style
        { "Status_Router",      "Disconnect_pppoe_1",   "stop_pppoe_1", 2,      SERVICE_RESTART,        stop_ppp_1},    // for cisco style
#endif
#ifdef UNNUMBERIP_SUPPORT
        { "Status_Router",      "Connect_unnumberip",	"start_pppoe",  1,      SERVICE_RESTART,	NULL}, // for cisco style
        { "Status_Router",      "Disconnect_unnumberip","stop_pppoe",   2,      SERVICE_RESTART,        stop_ppp}, // for cisco style
#endif
	{ "Status_Router",	"Connect_pptp",		"start_pptp",	1,	RESTART,		NULL},	// for cisco style
	{ "Status_Router",	"Disconnect_pptp",	"stop_pptp",	2,	SERVICE_RESTART,	stop_ppp},	// for cisco style
	{ "Status_Router",	"Connect_l2tp",		"start_l2tp",	1,	RESTART,		NULL},	// for cisco style
	{ "Status_Router",	"Disconnect_l2tp",	"stop_l2tp",	2,	SERVICE_RESTART,	stop_ppp},	// for cisco style
	{ "Status_Router",	"Connect_heartbeat",	"start_heartbeat",	1,	RESTART,	NULL},	// for cisco style
	{ "Status_Router",	"Disconnect_heartbeat",	"stop_heartbeat",	2,	SERVICE_RESTART,stop_ppp},	// for cisco style
#ifdef SES_BUTTON_SUPPORT
	{ "Wireless_Basic",	"Reset_SES",		"",		1,      SERVICE_RESTART,	reset_ses},
	{ "Wireless_Basic",	"Set_SES_Long_Push",	"",		1,      SERVICE_RESTART,	set_ses_long_push},
	{ "Wireless_Basic",	"Set_SES_Short_Push",	"ses_led",	1,      SERVICE_RESTART,	set_ses_short_push},
#endif
#ifdef SNMP_SUPPORT
	{ "Management",		"save",			"start_snmp",	1,	SERVICE_RESTART,	NULL},
#endif	
	{ "Filters",		"save",			"filters",	1,	SERVICE_RESTART,	save_policy},
	{ "Filters",		"delete",		"filters",	1,	SERVICE_RESTART,	single_delete_policy},
	{ "FilterSummary",	"delete",		"filters",	1,	SERVICE_RESTART,	summary_delete_policy},
	{ "Routing",		"del",			"static_route_del",	1,	SERVICE_RESTART,	delete_static_route},
	{ "RouteStatic",	"del",			"static_route_del",	1,	SERVICE_RESTART,	delete_static_route},
	{ "WL_WEPTable",	"key_64",		"",		1,	REFRESH,		generate_key_64},
	{ "WL_WEPTable",	"key_128",		"",		1,	REFRESH,		generate_key_128},
	{ "WL_WPATable",	"key_64",		"",		1,	REFRESH,		generate_key_64},
	{ "WL_WPATable",	"key_128",		"",		1,	REFRESH,		generate_key_128},
	{ "WL_ActiveTable",	"add_mac",		"",		1,	REFRESH,		add_active_mac},
	{ "Port_Services",	"save_services",	"filters",	2,	SERVICE_RESTART,	save_services_port},
#ifdef MULTIPLE_LOGIN_SUPPORT
	{ "User_Login",		"save_accounts",	"filters",	1,	SERVICE_RESTART,	save_user_account},
#endif
#ifdef EMI_TEST
	{ "emi_test",		"exec",			"",		1,	REFRESH,		emi_test_exec},
	{ "emi_test",		"del",			"",		1,	REFRESH,		emi_test_del},
#endif
#ifdef DIAG_SUPPORT
	{ "Ping",		"start",		"start_ping",	1,	SERVICE_RESTART,	diag_ping_start},
	{ "Ping",		"stop",			"",		0,	REFRESH,		diag_ping_stop},
	{ "Ping",		"clear",		"",		0,	REFRESH,		diag_ping_clear},
	{ "Traceroute",		"start",		"start_traceroute",		1,	SERVICE_RESTART,		diag_traceroute_start},
	{ "Traceroute",		"stop",			"",		0,	REFRESH,		diag_traceroute_stop},
	{ "Traceroute",		"clear",		"",		0,	REFRESH,		diag_traceroute_clear},
#endif
#ifdef DNS_SUPPORT
	{ "Dns_Table",		"apply",		"dns",		1,	SERVICE_RESTART,	apply_dns},
#endif
#ifdef TINYLOGIN_SUPPORT
	{ "Accounts",		"apply",		"",		1,	REFRESH,		apply_accounts},
	{ "Accounts",		"del_user",		"",		1,	REFRESH,		del_user},

#endif
#ifdef SYSLOG_SUPPORT
	{ "Log_all",		""	,		"",		0,	REFRESH,		set_log_type},
#endif
#ifdef WAKE_ON_LAN_SUPPORT
        { "Wol",		"wakeup",		"",		0,	REFRESH,		wol_wakeup},
#endif
#ifdef SAMBA_SUPPORT
	{ "SMBTable",		"get_share",		"",		0,	REFRESH,		smb_getshare},
	{ "SMBShare",		"get_dir",		"",		0,	REFRESH,		smb_getdir},
        { "SMBDir",		"get_dir",		"",		0,	REFRESH,		smb_getdir},
        { "SMBDir",		"get_file",		"",		0,	REFRESH,		smb_getfile},
#endif	
};

struct gozila_action *
handle_gozila_action(char *name, char *type)
{
	struct gozila_action *v;

	if(!name || !type)
		return NULL;

	for(v = gozila_actions ; v < &gozila_actions[STRUCT_LEN(gozila_actions)]; v++) {
   		if(!strcmp(v->name, name) && !strcmp(v->type, type)){
			return v;
		}
   	}
        return NULL;
}


char my_next_page[30]="";
int 
gozila_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, char_t *url, char_t *path, char_t *query)
{
	char *submit_button, *submit_type,*next_page;
	int action = REFRESH;
	int sleep_time;
	struct gozila_action *act;
	int ret;

	gozila_action = 1;
	my_next_page[0]='\0';

	submit_button = websGetVar(wp, "submit_button", NULL);	/* every html must have the name */
	submit_type = websGetVar(wp, "submit_type", NULL);	/* add, del, renew, release ..... */

	nvram_set("action_service","");
	nvram_set("action_service_arg1","");

	cprintf("submit_button=[%s] submit_type=[%s]\n",submit_button, submit_type);
	act = handle_gozila_action(submit_button, submit_type);

	if(act){
		cprintf("name=[%s] type=[%s] service=[%s] sleep=[%d] action=[%d]\n",act->name, act->type, act->service, act->sleep_time, act->action);
		nvram_set("action_service",act->service);
		sleep_time = act->sleep_time;
		action = act->action;
		if(act->go)
			ret=act->go(wp);
	}
	else{
		sleep_time = 0;
		action = REFRESH;
	}
#ifdef SAMBA_SUPPORT
	if (ret==SAMBA_FORK) goto skip_web; // for fork httpd
#endif	
	if(action == REFRESH)
		sleep(sleep_time);
	else if(action == SERVICE_RESTART){
		sys_commit();
		service_restart();
		sleep(sleep_time);
	}
	else if (action == RESTART){
		sys_commit();
		sys_restart();
	}
	if (my_next_page[0]!='\0') {
		sprintf(path,"%s",my_next_page);
	} else {
		next_page = websGetVar(wp, "next_page", NULL);
		if (next_page)
			sprintf(path,"%s",next_page);
		else
			sprintf(path,"%s.asp",submit_button);
	}
	do_ej(path,wp);       //refresh
	websDone(wp, 200);

#ifdef SAMBA_SUPPORT
   skip_web:	
#endif
	gozila_action = 0;    //reset gozila_action
	generate_key = 0;
	clone_wan_mac = 0;

	return 1;
}

struct apply_action apply_actions[] = {
	/* bellow for setup page */
#if OEM == LINKSYS
	{ "index",		"index",	0,	RESTART		,	NULL},
#else
	{ "OnePage",		"",		0,	RESTART		,	NULL},	// same as index
	{ "Expose",		"filters",	0,	SERVICE_RESTART	,	NULL},  // same as DMZ 
	{ "VServer",		"forward",	0,	SERVICE_RESTART	,	NULL},	// same as Forward
#endif
	{ "Security",		"",		1,	RESTART		,	NULL},
	{ "System",		"",		0,	RESTART		,	NULL},
	{ "DHCP",		"dhcp",		0,	SERVICE_RESTART	,	NULL},
#ifdef UDHCPD_STATIC_SUPPORT
	{ "DHCP_Static",	"dhcp",		0,	SERVICE_RESTART	,	NULL},
#endif
#ifdef DHCP_FILTER_SUPPORT
	{ "DHCP_FilterTable",	"filters",	0,	SERVICE_RESTART	,	NULL},
#endif
	{ "WL_WEPTable",	"",		0,	RESTART		,	NULL},
	{ "WL_WPATable",	"wireless",	0,	SERVICE_RESTART	,	NULL},
	/* bellow for advanced page */
	{ "DMZ",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "Filters",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "FilterIPMAC",	"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "FilterIP",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "FilterMAC",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "FilterPort",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "VPN",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "Firewall",		"filters",	0,	SERVICE_RESTART	,	NULL},
	{ "Forward",		"forward",	0,	SERVICE_RESTART	,	NULL},
	{ "Management",		"management",	4,	SERVICE_RESTART	,	NULL},
#ifdef PORT_TRIGGER_SUPPORT
	{ "Triggering",		"filters",	0,	SERVICE_RESTART	,	NULL},
#endif
#ifdef UPNP_FORWARD_SUPPORT
	{ "Forward_UPnP",	"forward_upnp",	0,	SERVICE_RESTART	,	NULL},
#endif
#ifdef ALG_FORWARD_SUPPORT
	{ "Forward_ALG",	"filters",	0,	SERVICE_RESTART	,	NULL},
#endif
	{ "Routing",		"",		0,	RESTART		,	NULL},
	{ "DDNS",		"ddns",		4,	SERVICE_RESTART	,	ddns_save_value},
	{ "Wireless",		"wireless",	0,	SERVICE_RESTART	,	NULL},
	{ "Wireless_Basic",	"wireless",	0,	RESTART	,		NULL},
	{ "Wireless_Advanced",	"wireless",	0,	SERVICE_RESTART	,	NULL},
	{ "Wireless_MAC",	"wireless",	0,	SERVICE_RESTART	,	NULL},
	{ "WL_FilterTable",	"wireless",	0,	SERVICE_RESTART	,	NULL},
#ifdef HSIAB_SUPPORT
	{ "HotSpot_Admin",      "hsiab_register",     2,      SERVICE_RESTART ,	hsiab_register},
	{ "HotSpot_Register_ok","hsiab_register_ok",  2,      RESTART ,		NULL},
	{ "finish_registration","hsiab_register",     2,      REFRESH ,		hsiab_finish_registration},
#endif
#ifdef SYSLOG_SUPPORT
	{ "Log_settings",    	"",		0,      RESTART ,		NULL},
#endif
};

struct apply_action *
handle_apply_action(char *name)
{
	struct apply_action *v;

	if(!name)
		return NULL;

	for(v = apply_actions ; v < &apply_actions[STRUCT_LEN(apply_actions)] ; v++) {
   		if(!strcmp(v->name, name)){
			return v;
		}
   	}
        return NULL;
}

int
apply_cgi(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, 
	  char_t *url, char_t *path, char_t *query)
{
	int action = NOTHING;
	char *value;
	char *submit_button;
	int sleep_time = 0;
	int need_commit = 1;
	int need_reboot = atoi(websGetVar(wp, "need_reboot", "0"));
#ifdef MUST_CHANGE_PWD_SUPPORT
	int is_not_first_access = atoi(websGetVar(wp, "is_not_first_access", "0"));
#endif
#ifdef THROUGHPUT_TEST_SUPPORT 
	int throughput = atoi(websGetVar(wp, "throughput_test", "0"));
#endif
	int ret_code;
	int i;

	error_value = 0;
	ret_code = -1;

        /********************/
        value = websGetVar(wp, "change_action", "");

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): change_action=[%s]",__FUNCTION__,value);
#endif

        if(value && !strcmp(value,"gozila_cgi")){     
             gozila_cgi(wp, urlPrefix, webDir, arg, url, path, query);
             return 1;
        }
        /********************/
	submit_button = websGetVar(wp, "submit_button", "");

	if (!query)
		goto footer;

	if(legal_ip_netmask("lan_ipaddr", "lan_netmask", nvram_safe_get("http_client_ip")) == TRUE)
		browser_method = USE_LAN;
	else
		browser_method = USE_WAN;

#ifdef MUST_CHANGE_PWD_SUPPORT
	if(is_not_first_access)
		nvram_set("is_not_first_access","1");
#endif
#ifdef THROUGHPUT_TEST_SUPPORT 
	if(throughput)
	{
		nvram_set("throughput_test","1");
	}
	else
	{
		nvram_set("throughput_test","0");
	}
#endif

#if 0
        if(check_hw_type() == BCM4702_CHIP) /* barry add for 4712 or 4702 RF test */
	        {
	          printf("\nBoard is 4702 \n");
	          value = websGetVar(wp, "StartContinueTx", NULL);
	          if(value){
	                    StartContinueTx_4702(value);
	                    goto footer;
	                    }

                  value = websGetVar(wp, "StopContinueTx", NULL);
                  if(value){
                            StopContinueTx_4702(value);
			    goto footer;
			    }
	       }
        else
        {
#endif
		value = websGetVar(wp, "StartContinueTx", NULL);
		printf("\nBarry StartContinueTx,value=%s\n",value);
		if(value){
			StartContinueTx(value);
			goto footer;
		}
	
		value = websGetVar(wp, "StopContinueTx", NULL);
		printf("\nBarry StopContinueTx,value=%s\n",value);
		if(value){
			StopContinueTx(value);
			goto footer;
		}
		/* 1030 */
		value = websGetVar(wp, "WL_atten_bb", NULL);
		printf("\nBarry WL_atten_bb,value=%s\n",value);
		if(value){
			Check_TSSI(value);
			goto footer;
		}
		/* 1030 */
		value = websGetVar(wp, "WL_tssi_enable", NULL);
		printf("\nBarry WL_tssi_enable,value=%s\n",value);
		if(value){
			Enable_TSSI(value);
			goto footer;
		}
		/* 1216 */
		value = websGetVar(wp, "ChangeANT", NULL);
		printf("\nBarry ChangeANT,value=%s\n",value);
		if(value){
			Change_Ant(value);
			goto footer;
		}
		value = websGetVar(wp, "StartEPI", NULL);
		printf("\nBarry StartEPI,value=%s\n",value);
		if(value){
			Start_EPI(value);
			goto footer;
		}
#if 0
	}
#endif
	value = websGetVar(wp, "skip_amd_check", NULL);
	if(value){
		if(atoi(value) == 0 || atoi(value) == 1){
			nvram_set("skip_amd_check", value);
			sys_commit();
		}
		goto footer;
	}

	need_commit = atoi(websGetVar(wp, "commit", "1"));
	
	value = websGetVar(wp, "action", "");

	/* Apply values */
	if (!strcmp(value, "Apply")) {
		struct apply_action *act;
		validate_cgi(wp);

		act = handle_apply_action(submit_button);
#ifdef EZC_SUPPORT
	        //If web page configuration is changed, the EZC configuration function should be disabled.(2004-07-29)
		nvram_set("is_default", "0");
		nvram_set("is_modified", "1");
#endif

		if(act){
			cprintf("submit_button=[%s] service=[%s] sleep_time=[%d] action=[%d]\n", 
				 act->name, act->service, act->sleep_time, act->action);
			if(act->action == SERVICE_RESTART)
				nvram_set("action_service",act->service);
			else
				nvram_set("action_service","");

			action = act->action;

			if(act->go)
				ret_code = act->go(wp);
#ifdef HSIAB_SUPPORT
			if(ret_code == DISABLE_HSIAB)
				action = RESTART;
			else if(ret_code == RESTART_HSIAB)
				action = RESTART;
#endif
		}
		else{
			nvram_set("action_service","");
			sleep_time = 1;
			action = RESTART;
		}
		
		if(need_commit){
#ifdef EOU_SUPPORT
		        //If web page configuration is changed, the EoU function should be disabled.(2004-05-06)
			nvram_set("eou_configured", "1");
#endif
#ifdef EZC_SUPPORT
		        //If web page configuration is changed, the EZC configuration function should be disabled.(2004-07-29)
			//nvram_set("is_default", "0");
			//nvram_set("is_modified", "1");
#endif
			sys_commit();
                } 
	}
	/* Restore defaults */
	else if (!strncmp(value, "Restore", 7)) {
		ACTION("ACT_SW_RESTORE"); 
		//eval("erase","nvram");
		nvram_set("restore_defaults", "1");
		eval("killall", "-9", "udhcpc");
		sys_commit();
		action = REBOOT;
	}

	/* Reboot */
	else if (!strncmp(value, "Reboot", 7)) {
		action = REBOOT;
	}
#ifdef BRCM
	/* Release lease */
	else if (!strcmp(value, "Release")) {
		websWrite(wp, "Releasing lease...");
		//sys_release();
		websWrite(wp, "done<br>");
		action = NOTHING;
	}

	/* Renew lease */
	else if (!strcmp(value, "Renew")) {
		websWrite(wp, "Renewing lease...");
		//sys_renew();
		websWrite(wp, "done<br>");
		action = NOTHING;
	}
#endif	
#ifdef __CONFIG_SES__
	/* SES events */
	else if (!strcmp(value, "NewSesNW")) {
		websWrite(wp, "Creating SES Network");
		nvram_set("ses_event", "3");
		action = NOTHING;
	}
	else if (!strcmp(value, "OpenWindow")) {
		/* verify that we are a psk/tkip network */
		if (nvram_match("wl0_crypto", "tkip") && 
		    (nvram_match("wl0_akm", "psk") || nvram_match("wl0_akm", "psk "))) {
			websWrite(wp, "Opening SES Window");
			nvram_set("ses_event", "4");
		}
		else {
			websWrite(wp, "Failed to open SES window; configuration incorrect");
		}
		action = NOTHING;
	}
	else if (!strcmp(value, "NewSesNWAndOW")) {
		websWrite(wp, "Creating SES Network and Opening SES Window");
		nvram_set("ses_event", "6");
		action = NOTHING;
	}
	else if (!strcmp(value, "ResetNWToDefault")) {
		websWrite(wp, "Restoring Network to Default");
		nvram_set("ses_event", "7");
		action = NOTHING;
	}
#endif /* __CONFIG_SES__ */
	/* Invalid action */
	else
		websDebugWrite(wp, "Invalid action %s<br>", value);


 footer:

#if OEM == LINKSYS
#ifdef HSIAB_SUPPORT
	if(!strcmp(submit_button, "HotSpot_Admin") && ret_code != DISABLE_HSIAB && ret_code != RESTART_HSIAB){
		if(register_status == -1 )
	        	do_ej("HotSpot_Register_fail.asp",wp);
		else{
			if(new_device == 1)
	        		do_ej("HotSpot_New_device.asp",wp);
			else{
	        		do_ej("HotSpot_Old_device.asp",wp);
				nvram_set("action_service", "");
				action = REFRESH;
			}
		}
	}
	else if(!strcmp(submit_button,"finish_registration")){
		do_ej("HotSpot_Register_ok.asp",wp);
	}
	else
#endif
	{
		if(!error_value){
			if(websGetVar(wp, "small_screen", NULL))
        			do_ej("Success_s.asp",wp);
			else
        			do_ej("Success.asp",wp);
		}
		else{
			if(websGetVar(wp, "small_screen", NULL))
        			do_ej("Fail_s.asp",wp);
			else
        			do_ej("Fail.asp",wp);
		}
	}
#else
	do_ej("Success.asp",wp); 
#endif

	websDone(wp, 200);

	nvram_set("upnp_wan_proto", "");
	/* The will let PC to re-get a new IP Address automatically */
	if(lan_ip_changed || need_reboot)	action = REBOOT;

	if (action == RESTART)
		sys_restart();
	else if (action == REBOOT)
		sys_reboot();
	else if (action == SERVICE_RESTART){
		service_restart();
	}

	for(i=sleep_time ; i>0 ; i--)
		sleep(1);

	return 1;
}

#ifdef WEBS

void
initHandlers(void)
{
	websAspDefine("nvram_get", ej_nvram_get);
	websAspDefine("nvram_match", ej_nvram_match);
	websAspDefine("nvram_invmatch", ej_nvram_invmatch);
	websAspDefine("nvram_list", ej_nvram_list);
	websAspDefine("filter_ip", ej_filter_ip);
	websAspDefine("filter_port", ej_filter_port);
	websAspDefine("forward_port", ej_forward_port);
	websAspDefine("static_route", ej_static_route);
	websAspDefine("localtime", ej_localtime);
	websAspDefine("dumplog", ej_dumplog);
	websAspDefine("dumpleases", ej_dumpleases);
	websAspDefine("ppplink", ej_ppplink);

	websUrlHandlerDefine("/apply.cgi", NULL, 0, apply_cgi, 0);
	websUrlHandlerDefine("/internal.cgi", NULL, 0, internal_cgi, 0);

	websSetPassword(nvram_safe_get("http_passwd"));

	websSetRealm("Broadcom Home Gateway Reference Design");
}

#else /* !WEBS */

static void
do_auth(char *userid, char *passwd, char *realm)
{
#ifdef MULTIPLE_LOGIN_SUPPORT //roger for multiple login 2004-11-30
	strncpy(userid, nvram_safe_get("http_login"), AUTH_MAX);
	strncpy(passwd, nvram_safe_get("http_login"), AUTH_MAX);
	strncpy(realm, MODEL_NAME, AUTH_MAX);
#else
	strncpy(userid, nvram_safe_get("http_username"), AUTH_MAX);
	strncpy(passwd, nvram_safe_get("http_passwd"), AUTH_MAX);
	strncpy(realm, MODEL_NAME, AUTH_MAX);
#endif
}
#ifdef EZC_SUPPORT
char ezc_version[128];
#endif
#ifdef GET_POST_SUPPORT

char post_buf[10000] = { 0 };
extern int post;

void	// support GET and POST 2003-08-22
do_apply_post(char *url, webs_t stream, int len, char *boundary)
{
   char buf[1024];
   int count;

   if (post==1) {
	if(len > sizeof(post_buf)-1) {
		cprintf("The POST data exceed length limit!\n");
		return;
	}

        /* Get query */
        if (!(count = wfread(post_buf, 1, len, stream)))
                return;
	post_buf[count] = '\0';;
        len -= strlen(post_buf);

        /* Slurp anything remaining in the request */
        while (--len>0)
#if defined(HTTPS_SUPPORT)
	     if(do_ssl)	
		     BIO_gets((BIO *)stream,buf,1);
	     else
#endif
	    	     (void) fgetc(stream);
	init_cgi(post_buf); //Added by Daniel(2004-07-29)
   }
}

static void
do_apply_cgi(char *url, webs_t stream)
{
	char *path, *query;

        if (post==1) {
                query=post_buf;
                path=url;
        }
        else {
                query=url;
                path = strsep(&query, "?") ? : url;
		init_cgi(query); //Added by Daniel(2004-07-29)
        }

	if(!query)
		return;	
	//init_cgi(query);  //Masked by Daniel(2004-07-29)
	apply_cgi(stream, NULL, NULL, 0, url, path, query);
	
	init_cgi(NULL); //Added by Daniel(2004-07-29) for new cgi.c
}

#else	// only support GET
static void
//do_apply_cgi(char *url, FILE *stream)
do_apply_cgi(char *url, webs_t stream) // jimmy, https, 8/4/2003
{
	char *path, *query;

	query = url;
	path = strsep(&query, "?") ? : url;

	init_cgi(query);
	apply_cgi(stream, NULL, NULL, 0, url, path, query);
	init_cgi(NULL); //Added by Daniel(2004-07-29) for new cgi.c
}
#endif

#ifdef BRCM
static void
do_internal_cgi(char *url, FILE *stream)
{
	char *path, *query;

	query = url;
	path = strsep(&query, "?") ? : url;
	init_cgi(query);
	internal_cgi(stream, NULL, NULL, 0, url, path, query);
	init_cgi(NULL); //Added by Daniel(2004-07-29) for new cgi.c
}
#endif

int
ej_get_http_method(int eid, webs_t wp, int argc, char_t **argv)
{
#ifdef GET_POST_SUPPORT
	if(nvram_match("http_method", "get"))
		return websWrite(wp, "%s", "get");
	else
		return websWrite(wp, "%s", "post");
#else
	return websWrite(wp, "%s", "get");
#endif
}


//Masked by Daniel(2004-07-29) for EZC
//static char no_cache[] =
//Modify by Daniel(2004-07-29) for EZC
char no_cache[] =
"Cache-Control: no-cache\r\n"
"Pragma: no-cache\r\n"
"Expires: 0"
;

#ifdef SAMBA_SUPPORT
extern char samba_mime_type[], samba_mime_extra[];
#endif

struct mime_handler mime_handlers[] = {
#ifdef EZC_SUPPORT
	{ "ezconfig.asp", "text/html", ezc_version, do_apply_ezconfig_post, do_ezconfig_asp, NULL },
#endif
	{ "**.asp", "text/html", no_cache, NULL, do_ej, do_auth },
	{ "SysInfo.htm*", "text/plain", no_cache, NULL, do_ej, do_auth },
	{ "SysInfo1.htm*", "text/plain", no_cache, NULL, do_ej, do_auth },
	{ "wlaninfo.htm*", "text/plain", no_cache, NULL, do_ej, do_auth },
	{ "**.css", "text/css", NULL, NULL, do_file, do_auth },
	{ "**.gif", "image/gif", NULL, NULL, do_file, do_auth },
	{ "**.jpg", "image/jpeg", NULL, NULL, do_file, do_auth },
#ifdef MULTILANG_SUPPORT
	{ "**.js", "text/javascript", NULL, NULL, do_ej, do_auth },
#else
	{ "**.js", "text/javascript", NULL, NULL, do_file, do_auth },
#endif
#ifdef GET_POST_SUPPORT
	{ "apply.cgi*", "text/html", no_cache, do_apply_post, do_apply_cgi, do_auth },
#else
	{ "apply.cgi*", "text/html", no_cache, NULL, do_apply_cgi, do_auth },
#endif
#ifdef BRCM
	{ "internal.cgi*", "text/html", no_cache, NULL, do_internal_cgi, do_auth },
#endif
#ifdef WCN_SUPPORT
        { "**.WFC", "octet-stream", no_cache, do_wcn_post, do_wcn_cgi, NULL },
#endif
	{ "upgrade.cgi*", "text/html", no_cache, do_upgrade_post, do_upgrade_cgi, do_auth },
#ifdef SETUP_WIZARD_SUPPORT
	{ "Gozila.cgi*", "text/html", no_cache, NULL, do_setup_wizard, do_auth },	// for setup wizard
#endif
#ifdef BACKUP_RESTORE_SUPPORT
	{ "**.cfg", "application/octet-stream", no_cache, NULL, do_backup, do_auth },
	{ "restore.cgi**", "text/html", no_cache, do_upgrade_post, do_upgrade_cgi, do_auth },
#endif
#ifdef SAMBA_SUPPORT
	{ "samba.cgi*", samba_mime_type, samba_mime_extra, do_samba_file, do_samba_cgi, do_auth },
#endif
	{ "test.bin**", "application/octet-stream", no_cache, NULL, do_file, do_auth },
//for ddm 
#ifdef DDM_SUPPORT
	{ "verizon/page_info.xml*", "text/xml", no_cache, NULL, do_ej, NULL },
	{ "verizon/get_admin_info.xml*", "text/xml", no_cache, NULL, do_ej, NULL },
	{ "verizon/get_wan_ip_address_assigment_info.xml*", "text/xml", no_cache, NULL, do_ej, NULL },
	{ "verizon/get*.xml*", "text/xml", no_cache, NULL, do_ej, do_auth },
	{ "verizon/set**.xml", "text/xml", no_cache, do_ddm_post, do_ej, do_auth },
#endif	
#ifdef INTEL_VIIV_SUPPORT
        { "viiv.cgi*", "text/html", no_cache, do_viiv_post, do_viiv_cgi, do_auth },
        { "viiv.htm*", "text/plain", no_cache, NULL, do_viiv_html, do_auth },
        { "viiv.xml*", "text/xml", no_cache, NULL, do_viiv_xml, do_auth },
        { "viiv_general.xml*", "text/xml", no_cache, NULL, do_viiv_general_xml, NULL },
        //{ "router-info.htm*", "text/plain", no_cache, NULL, do_router_info, NULL },
#endif
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

struct ej_handler ej_handlers[] = {
	/* for all */
	{ "nvram_get", ej_nvram_get },
	{ "nvram_get_len", ej_nvram_get_len },
	{ "nvram_selget", ej_nvram_selget },
	{ "nvram_match", ej_nvram_match },
	{ "nvram_invmatch", ej_nvram_invmatch },
	{ "nvram_selmatch", ej_nvram_selmatch },
	{ "nvram_else_selmatch", ej_nvram_else_selmatch },
	{ "nvram_else_match", ej_nvram_else_match },
	{ "nvram_list", ej_nvram_list },
	{ "nvram_mac_get", ej_nvram_mac_get },
	{ "nvram_gozila_get", ej_nvram_gozila_get },
	{ "nvram_status_get", ej_nvram_status_get },
	{ "webs_get", ej_webs_get },
	{ "support_match", ej_support_match },
	{ "support_invmatch", ej_support_invmatch },
	{ "support_elsematch", ej_support_elsematch },
	{ "get_firmware_version", ej_get_firmware_version },
	{ "get_firmware_title", ej_get_firmware_title },
	{ "get_model_name", ej_get_model_name },
	{ "get_single_ip", ej_get_single_ip },
	{ "get_single_mac", ej_get_single_mac },
	{ "prefix_ip_get", ej_prefix_ip_get },
	{ "no_cache", ej_no_cache },
	{ "scroll", ej_scroll },
	{ "get_dns_ip", ej_get_dns_ip },
	{ "onload", ej_onload },
	{ "get_web_page_name", ej_get_web_page_name },
	{ "show_logo", ej_show_logo },
	{ "get_clone_mac", ej_get_clone_mac },
	/* for index */
	{ "show_index_setting", ej_show_index_setting },
	{ "compile_date", ej_compile_date },
	{ "compile_time", ej_compile_time },
	{ "get_wl_max_channel", ej_get_wl_max_channel },
	{ "get_wl_domain", ej_get_wl_domain },
	/* for status */
	{ "show_status", ej_show_status },
	{ "show_domain", ej_show_wan_domain },
	{ "show_status_setting", ej_show_status_setting },
	{ "localtime", ej_localtime },
	{ "dhcp_remaining_time", ej_dhcp_remaining_time },
#ifdef ARP_TABLE_SUPPORT
	{ "dump_arp_table", ej_dump_arp_table },
#endif
#ifdef SES_BUTTON_SUPPORT   
	{ "get_ses_status", ej_get_ses_status },
#endif 
#ifdef WAKE_ON_LAN_SUPPORT
        { "get_wol_mac", ej_get_wol_mac },
#endif
#ifdef SAMBA_SUPPORT
	{ "dumpsamba", ej_dumpsamba },
#endif	
	/* for dhcp */
	{ "dumpleases", ej_dumpleases },
	/* for ddm */
#ifdef DDM_SUPPORT	
	{ "ddmdumpleases", ej_ddm_dumpleases },
	{ "ddm_check_passwd", ej_ddm_check_passwd },
	{ "ddm_error_no", ej_ddm_error_no },
	{ "ddm_error_desc", ej_ddm_error_desc },
	{ "ddm_show_ipend", ej_ddm_show_ipend },
	{ "ddm_show_wanproto", ej_ddm_show_wanproto },
	{ "ddm_show_lanproto", ej_ddm_show_lanproto },
	{ "ddm_show_idletime", ej_ddm_show_idletime },
#endif
#ifdef MULTIPLE_LOGIN_SUPPORT //roger multi-login
	{ "http_user_name_get", ej_http_user_name_get },
	{ "user_account_get", ej_user_account_get },
#endif
	{ "http_name_get", ej_http_name_get },//r modify for MULTIPLE_LOGIN
#ifdef DNS_SUPPORT
	{ "dump_dns_entry", ej_dump_dns_entry },
#endif
#ifdef TINYLOGIN_SUPPORT
	{ "dump_account", ej_dump_account },
#endif
	/* for log */
	{ "dumplog", ej_dumplog },
#ifdef SYSLOG_SUPPORT
	{ "dump_syslog", ej_dump_syslog },
	{ "dump_log_settings", ej_dump_log_settings },
#endif
	/* for filter */
	{ "filter_init", ej_filter_init },
	{ "filter_summary_show", ej_filter_summary_show },
	{ "filter_ip_get", ej_filter_ip_get },
	{ "filter_port_get", ej_filter_port_get },
	{ "filter_dport_get", ej_filter_dport_get },
	{ "filter_mac_get", ej_filter_mac_get },
	{ "filter_policy_select", ej_filter_policy_select },
	{ "filter_policy_get", ej_filter_policy_get },
	{ "filter_tod_get", ej_filter_tod_get },
	{ "filter_web_get", ej_filter_web_get },
	{ "filter_port_services_get", ej_filter_port_services_get },
	/* for forward */
	{ "port_forward_table", ej_port_forward_table },
#ifdef SPECIAL_FORWARD_SUPPORT
	{ "spec_forward_table", ej_spec_forward_table },
#endif
#ifdef PORT_TRIGGER_SUPPORT
	{ "port_trigger_table", ej_port_trigger_table },
#endif
#ifdef UPNP_FORWARD_SUPPORT
	{ "forward_upnp", ej_forward_upnp },
#endif
#ifdef ALG_FORWARD_SUPPORT
	{ "forward_alg", ej_forward_alg },
#endif
	/* for route */
	{ "static_route_table", ej_static_route_table },
	{ "static_route_setting", ej_static_route_setting },
	{ "dump_route_table", ej_dump_route_table },
	/* for ddns */
	{ "show_ddns_status", ej_show_ddns_status },
	{ "show_ddns_ip", ej_show_ddns_ip },
	{ "show_ddns_setting", ej_show_ddns_setting },
	/* for wireless */
	{ "wireless_active_table", ej_wireless_active_table },
	{ "wireless_filter_table", ej_wireless_filter_table },
	{ "show_wl_wep_setting", ej_show_wl_wep_setting },
	{ "get_wep_value", ej_get_wep_value },
	{ "get_wl_active_mac", ej_get_wl_active_mac },
	{ "get_wl_value", ej_get_wl_value },
	{ "show_wpa_setting", ej_show_wpa_setting },
	{ "show_wpa_setting2", ej_show_wpa_setting2 },
	/* for test */
	{ "wl_packet_get", ej_wl_packet_get },
	{ "wl_ioctl", ej_wl_ioctl },
#ifdef AOL_SUPPORT
	/* for aol */
	{ "aol_value_get", ej_aol_value_get },
	{ "aol_settings_show", ej_aol_settings_show },
#endif
#ifdef EMI_TEST
	{ "dump_emi_test_log", ej_dump_emi_test_log },
#endif
#ifdef DIAG_SUPPORT
	{ "dump_ping_log", ej_dump_ping_log },
	{ "dump_traceroute_log", ej_dump_traceroute_log },
#endif
#ifdef HSIAB_SUPPORT
        { "get_hsiab_value", ej_get_hsiab_value },
        { "show_hsiab_setting", ej_show_hsiab_setting },
        { "show_hsiab_config", ej_show_hsiab_config },
        { "dump_hsiab_db", ej_dump_hsiab_db },
        { "dump_hsiab_msg", ej_dump_hsiab_msg },
#endif  
	/* for verizon */
#ifdef VERIZON_LAN_SUPPORT
	{ "show_dhcp_setting", ej_show_dhcp_setting },
#endif
#ifdef UDHCPD_STATIC_SUPPORT
	{ "dhcp_static_get", ej_dhcp_static_get },
#endif
#ifdef DHCP_FILTER_SUPPORT
	{ "DHCP_filter_MAC_table", ej_DHCP_filter_MAC_table },
#endif

        { "show_sysinfo", ej_show_sysinfo },
        { "show_miscinfo", ej_show_miscinfo },
        { "get_http_method", ej_get_http_method },
#ifdef BACKUP_RESTORE_SUPPORT
        { "get_backup_name", ej_get_backup_name },
        { "view_config", ej_view_config },
#endif
#ifdef HW_QOS_SUPPORT
	{ "per_port_option", ej_per_port_option},	
#endif
#ifdef PERFORMANCE_SUPPORT
	{ "perform", ej_perform},	
	{ "show_wl_status", ej_show_wl_status},	
#endif
	{ "get_http_prefix", ej_get_http_prefix },
#ifdef WL_STA_SUPPORT
	{ "dump_site_suvery", ej_dump_site_suvery },
#endif
	{ "get_mtu", ej_get_mtu },
	{ "get_url", ej_get_url },
	{ "wme_match_op", ej_wme_match_op },
#ifdef MULTILANG_SUPPORT
	{ "langpack", ej_langpack },
        { "charset", ej_charset },
#endif
#ifdef WCN_SUPPORT
//For WCN; Daniel(2004-12-06)
        { "get_wcn_output_file_name", ej_get_wcn_output_file_name },
        { "get_wcn_output_file_path_name", ej_get_wcn_output_file_path_name },
#endif
	{ NULL, NULL }
};
#endif /* !WEBS */
