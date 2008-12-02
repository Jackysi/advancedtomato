

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
 * Router firewall 
 *
 * Copyright 2005, Cybertan Corporation
 * All Rights Reserved.
 *
 * Description:
 *   We use Netfilter to be our firewall. This program will generate a policy 
 *   file, and it is the same format as the output of "iptables-save". Thus, 
 *   we can use "iptables-restore" to insert the rules in one time.
 *   It's pretty fast ;-)
 *
 *   
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Cybertan Corporation;
 * the contents of this file may not be disclosed to third parties, copied or
 * duplicated in any form, in whole or in part, without the prior written
 * permission of Cybertan Corporation.
 *
 */


/* The macro 'DEVELOPE_ENV' is for developement only. If it is defined, you
 * can compile it in your (x86) host machine and run it locally. This speeds
 * up the developing.
 *
 * Tips :
 * shell> gcc firewall.c -o fw
 * shell> ./fw
 * shell> iptables-restore /tmp/.ipt
 */

//#define DEVELOPE_ENV
//#define XBOX_SUPPORT		/* Define Microsoft XBox, game machine, support , move to cy_configure*/
//#define AOL_SUPPORT		/* Define AOL support */
//#define SIP_ALG_SUPPORT
//#define FLOOD_PROTECT		/* Define flooding protection */
//#define REVERSE_RULE_ORDER	/* If it needs to reverse the rule's sequential. It is used 
//				   when the MARK match/target be using. */

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


#ifndef DEVELOPE_ENV
#include <bcmnvram.h>
#include <shutils.h>
#include <rc.h>
#include <code_pattern.h>
#include <utils.h>
#include <cy_conf.h>
#endif

//add by tanghui @ 2006-05-24
#include <errno.h>


/* Same as the file "linux/netfilter_ipv4/ipt_webstr.h" */
#define BLK_JAVA                0x01
#define BLK_ACTIVE              0x02
#define BLK_COOKIE              0x04
#define BLK_PROXY               0x08

/* possible files path */
#define IPTABLES_SAVE_FILE	"/tmp/.ipt"
#define CRONTAB			"/tmp/crontab"
#define IPTABLES_RULE_STAT	"/tmp/.rule"
#define IPTABLES_IN_RULE_STAT "/tmp/.in_rule"
#define IPTABLES_OUT_RULE_STAT "/tmp/.out_rule"

#define IPTABLES_MY_LOCK  "/tmp/.my_lock"
#define IPTABLES_RULE_STAT_LOCK	"/tmp/.rule.lock"
#define IPTABLES_INTERVAL	100000	/* 100000 Micro seconds */


/* Known port */
#define DNS_PORT		53	/* UDP */
#define TFTP_PORT		69	/* UDP */
#define ISAKMP_PORT		500	/* UDP */
#define RIP_PORT		520	/* UDP */
#define L2TP_PORT		1701	/* UDP */

/* tanghui add @ 2006-04-03 */
#define ESP_PPORT		0x32
#define AH_PPORT		0x33

#ifdef DHCP_FILTER_SUPPORT
#define DHCP_SERVER_PORT	67	/* UDP */
#define DHCP_HOST_PORT		68	/* UDP */
#endif

#define HTTP_PORT		80	/* TCP */
#define IDENT_PORT		113	/* TCP */
#define HTTPS_PORT		443	/* TCP */
#define PPTP_PORT		1723	/* TCP */

#define IP_MULTICAST		"224.0.0.0/4"

/* Limitation definition */
#define NR_RULES		10
#define NR_IPGROUPS		5
#define NR_MACGROUPS		5

/* MARK number in mangle table */
#define MARK_OFFSET		0x10
//#define MARK_MASK		0xf0
#define MARK_DROP		0x1e
//#define MARK_ACCEPT		0x1f
//#define MARK_HTTP 		0x30
#define MARK_LAN2WAN		0x100	/* For HotSpot */
#define MARK_NBSRELAY		0x89	/* Netbios protocol number 137 */

#ifdef AOL_SUPPORT
	#define AOL_TCP_PORT	"5190:5191 5200:5201 13784"
	#define AOL_UDP_PORT	"5190:5191"
	#define AOL_DHCP_UDP	"1701"
	#define MINUTE		60	/* seconds */
#endif

#ifdef FLOOD_PROTECT
	#define FLOOD_RATE	200
	#define LOG_FLOOD_RATE	"10/min"
	#define TARG_PASS	"limaccept"	/* limited of accepting chain */
	#define TARG_RST	"logreject"
#else
	#define TARG_PASS	"ACCEPT"
	#define TARG_RST	"REJECT --reject-with tcp-reset"
#endif

#if 0 
#define DEBUG printf
#else
#define DEBUG(format, args...) 
#endif

#define GOOGLE_DNS_FILE	"/tmp/dns.google"

static char *suspense;
static char *fw_suspense;
static unsigned int count = 0;
static unsigned int fw_count = 0;
static char log_accept[15];
static char log_drop[15];
static char log_reject[64];
static char wanface[IFNAMSIZ];
static char lanface[IFNAMSIZ];
static char lan_cclass[]="xxx.xxx.xxx.";
static char wanaddr[]="xxx.xxx.xxx.xxx";
static int web_lanport = HTTP_PORT; 

static FILE *ifd;	/* /tmp/.rule */
static FILE *cfd;	/* /tmp/crontab */

//add by xjj start
int deny_stat=0;
static int intime_stat[NR_RULES+1];
static int local_stat[NR_RULES+1]; 
static int time_Not_Available;
static int ntp_update_stat;
static int ntp_time_web_stat=1;

static int rule_run_stat[11];
static int rule_stat[11];
static unsigned int now_hrmin_last;
static unsigned int now_wday_last;
static int first_use_filtersync_main;
 int ntp_success;
 
static int filtersync_firewall;
//add by xjj end


static unsigned int now_wday, now_hrmin;
static int webfilter = 0;
static int dmzenable = 0;
static int remotemanage = 0;

#define DEL_IP_CONNTRACK_ENTRY 1
#ifdef DEL_IP_CONNTRACK_ENTRY
static int need_to_update_ip_conntrack = 0;
#endif
/******************************* DEVELOPE_ENV ***********************************************/
#ifdef DEVELOPE_ENV

#undef AOL_SUPPORT
#undef XBOX_SUPPORT 

#define diag_led(a, b) printf("........ diag_led()\n")
#define eval(args...) printf(".........eval()\n")
#define nvram_safe_get(name) (nvram_get(name) ? : "")
#define foreach(word, wordlist, next) \
for (next = &wordlist[strspn(wordlist, " ")], \
	strncpy(word, next, sizeof(word)), \
	word[strcspn(word, " ")] = '\0', \
	word[sizeof(word) - 1] = '\0', \
	next = strchr(next, ' '); \
	strlen(word); \
	next = next ? &next[strspn(next, " ")] : "", \
	strncpy(word, next, sizeof(word)), \
	word[strcspn(word, " ")] = '\0', \
	word[sizeof(word) - 1] = '\0', \
	next = strchr(next, ' '))

#define split(word, wordlist, next, delim) \
for (next = wordlist, \
	strncpy(word, next, sizeof(word)), \
	word[(next=strstr(next, delim)) ? strstr(word, delim) - word : sizeof(word) - 1] = '\0', \
	next = next ? next + sizeof(delim) - 1 : NULL ; \
	strlen(word); \
	next = next ? : "", \
	strncpy(word, next, sizeof(word)), \
	word[(next=strstr(next, delim)) ? strstr(word, delim) - word : sizeof(word) - 1] = '\0', \
	next = next ? next + sizeof(delim) - 1 : NULL)


static char *
nvram_get(const char *name)
{
    char *value;
    int i;

    struct nvram_pair {
	char *name;
	char *value;
    } nvp[] = {
	"wan_ipaddr",   "200.100.77.77",
	"lan_ipaddr",   "192.168.7.1",
	"dmz_ipaddr",   "192.168.7.100",
	"dmz_enable", "0",
	"http_wanport", "8080",
	"http_lanport", "80",
	//"forward_spec", "",
	"forward_port", "",
	"filter_port",  "",
	//"filter_ip",    "",
	//"filter_mac",   "",
	"lan_ifname",   "eth0",
	"log_level",    "0",
	"remote_upgrade", "",
	"wk_mode", "gateway",
	"block_wan", "1",
	"wan_proto", "dhcp",
	"mtu_enable", "0",
	//"pptp_pass", "",
	//"l2tp_pass", "0",
	//"ipsec_pass", "",
	//"hsiab_mode", "",
	"block_cookie", "1",
	"block_java", "",
	"block_activex", "",
	"block_proxy", "1",
	//"multicast_pass", "",
	"remote_management", "1",
	"dr_wan_rx", "0",
	//"aol_block_traffic", "",
	"port_trigger", "test1:on:both:10000-20000>1000-2000 test2:off:both:30000-40000>3000-4000",
	"filter_rule1", "$STAT:2$NAME:policy01$$",
	"filter_rule2", "$STAT:1$NAME:policy02$$",
	"filter_ip_grp1", "20 21 0 0 0 0 30-40 0-0",
	"filter_ip_grp2", "200 0 0 0 0 0 100-120 0-0",
	"filter_mac_grp1", "22:22:22:22:22:22 44:44:44:44:44:44",
	"filter_mac_grp2", "66:66:66:66:66:66 88:88:88:88:88:88",
	"filter_dport_grp1",  "tcp:50-60 both:600-600",
	"filter_dport_grp2",  "udp:2000-2000 both:600-600",
	"filter_tod1", "0:0 23:59 0-6",
	"filter_tod2", "0:0 23:59 0-6",
	"filter_web_host1", "hello<&nbsp;>world<&nbsp;>friend",
	"filter_web_host2", "",
	"filter_web_url1", "hello<&nbsp;>world<&nbsp;>friend",
	"filter_web_url2", "",
	"filter_port_grp1", "FTP<&nbsp;>Telnet G",
	"filter_port_grp2", "FTP<&nbsp;>Telnet G",
	"filter_services",  "$NAME:008:Telnet G$PROT:004:both$PORT:009:2323:2323<&nbsp;>$NAME:003:FTP$PROT:003:tcp$PORT:005:21:21",
	"filter", "on",
	"block_loopback", "0",
    };

    for (i=0; i < (sizeof(nvp)/sizeof(nvp[0])); i++) {
	if (strcmp(name, nvp[i].name) == 0)
	    return nvp[i].value;
    }

    return NULL;
}

static inline int
nvram_match(char *name, char *match)
{
    const char *value = nvram_get(name);
    return (value && !strcmp(value, match));
}

static inline int
nvram_invmatch(char *name, char *invmatch)
{
    const char *value = nvram_get(name);
    return (value && strcmp(value, invmatch));
}

static char * 
range(char *start, char *end)
{
    return start;
}

static char *
get_wan_face(void)
{
    return "eth1";
}

static int
get_single_ip(char *ipaddr, int which)
{
    int ip[4]={0,0,0,0};
    int ret;

    ret = sscanf(ipaddr,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]);

    return ip[which];	
}
#endif  /* DEVELOPE_ENV */
/******************************* DEVELOPE_ENV ***********************************************/

static void save2file(const char *fmt,...)
{
    char buf[10240];
    va_list args;
    FILE *fp;

    if ((fp = fopen(IPTABLES_SAVE_FILE, "a")) == NULL) {
	printf("Can't open /tmp/.ipt\n");
	exit(1);
    }

    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args); 
    va_start(args, fmt);
    fprintf(fp, "%s", buf);
    va_end(args);

    fclose(fp);
}

static int ip2cclass(char *ipaddr, char *new, int count)
{       
    int ip[4];  

    if(sscanf(ipaddr,"%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3]) != 4)
	return 0;

    return snprintf(new, count, "%d.%d.%d.",ip[0],ip[1],ip[2]);
}

static void parse_port_forward(char *wordlist)
{
    char var[256], *next;
    char *name, *enable, *proto, *port, *ip;
    char buff[256];
	char multiname[20];
    int  flag_dis = 0;
	int idx, flg = 0;

    /* name:enable:proto:port>ip name:enable:proto:port>ip */
    foreach(var, wordlist, next) {
	enable = var;
	name = strsep(&enable, ":");
	if (!name || !enable)
	    continue;
	proto = enable;
	enable = strsep(&proto, ":");
	if (!enable || !proto)
	    continue;
	port = proto;
	proto = strsep(&port, ":");
	if (!proto || !port)
	    continue;
	ip = port;
	port = strsep(&ip, ">");
	if (!port || !ip)
	    continue;

	/* skip if it's disabled */
	if( strcmp(enable, "off") == 0 )
	    flag_dis = 1;
	else
	    flag_dis = 0;

#ifdef DEL_IP_CONNTRACK_ENTRY
        if(flag_dis == 1)
        {
                need_to_update_ip_conntrack = 1;
                continue;
        }
#endif
	/* -A PREROUTING -i eth1 -p tcp -m tcp --dport 8899:88 -j DNAT 
	   --to-destination 192.168.1.88:0 
	   -A PREROUTING -i eth1 -p tcp -m tcp --dport 9955:99 -j DNAT 
	   --to-destination 192.168.1.99:0 */
	if( !strcmp(proto,"tcp") || !strcmp(proto,"both") )
	{
		bzero(buff, sizeof(buff));

		if( flag_dis == 0 )
		{
	   		 save2file("-A PREROUTING -p tcp -m tcp -d %s --dport %s "
		   			   "-j DNAT --to-destination %s%s\n"
		    		   , wanaddr, port, lan_cclass, ip);
	
	    	 snprintf(buff, sizeof(buff), "-A FORWARD -p tcp "
		    		   "-m tcp -d %s%s --dport %s -j %s\n"
		    		   , lan_cclass, ip, port, log_accept);
		}			
		else
		{
		    if( (!dmzenable) || (dmzenable && strcmp(ip , nvram_safe_get("dmz_ipaddr"))) )
		    {
		    	snprintf(buff, sizeof(buff), "-A FORWARD -p tcp "
			    		 "-m tcp -d %s%s --dport %s -j %s\n"
			   			 , lan_cclass, ip, port, log_drop);
         	}
		}
		
			if (nvram_match("filter","off") || nvram_match("wk_mode", "router")) 
			{
			    fw_count += strlen(buff) + 1;
			    fw_suspense = realloc(fw_suspense, fw_count );
			    strcat(fw_suspense, buff );
			}
			else
			{
	    		count += strlen(buff) + 1;
	   			suspense = realloc(suspense, count );
			    strcat(suspense, buff );
			}
	}

	if( !strcmp(proto,"udp") || !strcmp(proto,"both") )
	{
		bzero(buff, sizeof(buff));

		if( flag_dis == 0 )
		{
	   		save2file("-A PREROUTING -p udp -m udp -d %s --dport %s "
		    "-j DNAT --to-destination %s%s\n"
		    , wanaddr, port, lan_cclass, ip);

	    	snprintf(buff, sizeof(buff), "-A FORWARD -p udp "
		    "-m udp -d %s%s --dport %s -j %s\n"
		    , lan_cclass, ip, port, log_accept);
		}
		else
		{
		    if((!dmzenable) || (dmzenable && strcmp(ip , nvram_safe_get("dmz_ipaddr"))) )
		    {
		   		snprintf(buff, sizeof(buff), "-A FORWARD -p udp "
			    "-m udp -d %s%s --dport %s -j %s\n"
			    , lan_cclass, ip, port, log_drop);

		    }
		}
			if (nvram_match("filter","off") || nvram_match("wk_mode", "router")) 
			{
			    fw_count += strlen(buff) + 1;
			    fw_suspense = realloc(fw_suspense, fw_count );
			    strcat(fw_suspense, buff );
			}
			else
			{
	   			 count += strlen(buff) + 1;
	   			 suspense = realloc(suspense, count );
	   			 strcat(suspense, buff );
	   		}		
	}
	}
}

#ifdef ALG_FORWARD_SUPPORT
static void parse_alg_forward()
{
    char name[] = "forward_algXXXXXXXXXX", value[1000];
    char *wan_port0, *wan_port1, *lan_ipaddr, *lan_port0, *lan_port1, *proto;
    char *enable, *desc, *policy;
    char buff[256];
    int i;

//    if(nvram_invmatch("upnp_enable", "1"))
//	    return;

    /* Set wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1,proto,enable,desc */
    for(i=0 ; i<15 ; i++){
	snprintf(name, sizeof(name), "forward_alg%d", i);

	strncpy(value, nvram_safe_get(name), sizeof(value));

	/* Check for LAN IP address specification */
	lan_ipaddr = value;
	wan_port0 = strsep(&lan_ipaddr, ">");
	if (!lan_ipaddr)
	    continue;

	/* Check for LAN destination port specification */
	lan_port0 = lan_ipaddr;
	lan_ipaddr = strsep(&lan_port0, ":");
	if (!lan_port0)
	    continue;

	/* Check for protocol specification */
	proto = lan_port0;
	lan_port0 = strsep(&proto, ":,");
	if (!proto)
	    continue;

	/* Check for policy specification */
	policy = proto;
	proto = strsep(&policy, ":,");
	if (!policy)
	    continue;

	/* Check for enable specification */
	enable = policy;
	policy = strsep(&enable, ":,");
	if (!enable)
	    continue;

	/* Check for description specification (optional) */
	desc = enable;
	enable = strsep(&desc, ":,");

	/* Check for WAN destination port range (optional) */
	wan_port1 = wan_port0;
	wan_port0 = strsep(&wan_port1, "-");
	if (!wan_port1)
	    wan_port1 = wan_port0;

	/* Check for LAN destination port range (optional) */
	lan_port1 = lan_port0;
	lan_port0 = strsep(&lan_port1, "-");
	if (!lan_port1)
	    lan_port1 = lan_port0;

	/* skip if it's disabled */
	if( strcmp(enable, "off") == 0 )
	    continue;

	/* skip if it's illegal ip */
	if(get_single_ip(lan_ipaddr,3) == 0 || 
		get_single_ip(lan_ipaddr,3) == 255)
	    continue;

	/* -A PREROUTING -p tcp -m tcp --dport 823 -j DNAT 
	   --to-destination 192.168.1.88:23  */
	if( !strcmp(proto,"tcp") || !strcmp(proto,"both") ){
	    save2file("-A PREROUTING -i %s -p tcp -m tcp -d %s --dport %s "
		    "-j DNAT --to-destination %s%d:%s\n"
		    , wanface, nvram_safe_get("wan_ipaddr"), wan_port0, lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0);

	    snprintf(buff, sizeof(buff), "-A FORWARD -p tcp "
		    "-m tcp -d %s%d --dport %s -j %s\n"
		    , lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0, !strcmp(policy,"accept") ? log_accept : log_drop);

	    count += strlen(buff) + 1;
	    suspense = realloc(suspense, count );
	    strcat(suspense, buff );
	}
	if( !strcmp(proto,"udp") || !strcmp(proto,"both") ){
	    save2file("-A PREROUTING -i %s -p udp -m udp -d %s --dport %s "
		    "-j DNAT --to-destination %s%d:%s\n"
		    , wanface, nvram_safe_get("wan_ipaddr"), wan_port0, lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0);

	    snprintf(buff, sizeof(buff), "-A FORWARD -p udp "
		    "-m udp -d %s%d --dport %s -j %s\n"
		    , lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0, !strcmp(policy,"accept") ? log_accept : log_drop);

	    count += strlen(buff) + 1;
	    suspense = realloc(suspense, count );
	    strcat(suspense, buff); 
	}
    }
}
#endif

static void parse_upnp_forward()
{
    char name[] = "forward_portXXXXXXXXXX", value[1000];
    char name1[] = "forward_portsipXXXXXXXXXX", value1[1000];
    char *wan_port0, *wan_port1, *lan_ipaddr, *lan_port0, *lan_port1, *proto;
    char src_ipaddr[20];
    char *enable, *desc;
    char buff[256];
    int i;


	memset(value,0,1000);
	memset(value1,0,1000);
	memset(src_ipaddr,0,20);
	memset(buff,0,256);
	
    if(nvram_invmatch("upnp_enable", "1"))
	    return;

    /* Set wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1,proto,enable,desc */
    for(i=0 ; i<32 ; i++){
	snprintf(name, sizeof(name), "forward_port%d", i);
        snprintf(name1, sizeof(name1), "forward_portsip%d", i);

	strncpy(value, nvram_safe_get(name), sizeof(value));
        strncpy(value1, nvram_safe_get(name1), sizeof(value1));

	/* Check for LAN IP address specification */
	lan_ipaddr = value;
	wan_port0 = strsep(&lan_ipaddr, ">");
	if (!lan_ipaddr)
	    continue;

	/* Check for LAN destination port specification */
	lan_port0 = lan_ipaddr;
	lan_ipaddr = strsep(&lan_port0, ":");
	if (!lan_port0)
	    continue;

	/* Check for protocol specification */
	proto = lan_port0;
	lan_port0 = strsep(&proto, ":,");
	if (!proto)
	    continue;

	/* Check for enable specification */
	enable = proto;
	proto = strsep(&enable, ":,");
	if (!enable)
	    continue;

	/* Check for description specification (optional) */
	desc = enable;
	enable = strsep(&desc, ":,");

	/* Check for WAN destination port range (optional) */
	wan_port1 = wan_port0;
	wan_port0 = strsep(&wan_port1, "-");
	if (!wan_port1)
	    wan_port1 = wan_port0;

	/* Check for LAN destination port range (optional) */
	lan_port1 = lan_port0;
	lan_port0 = strsep(&lan_port1, "-");
	if (!lan_port1)
	    lan_port1 = lan_port0;

	/* skip if it's disabled */
	if( strcmp(enable, "off") == 0 )
	    continue;

	/* skip if it's illegal ip */
	if(get_single_ip(lan_ipaddr,3) == 0 || 
		get_single_ip(lan_ipaddr,3) == 255)
	    continue;

        if(!strcmp(value1, ""))
                snprintf(src_ipaddr, sizeof(src_ipaddr), "0.0.0.0");
        else
                snprintf(src_ipaddr, sizeof(src_ipaddr), value1);

	/* -A PREROUTING -p tcp -m tcp --dport 823 -j DNAT 
	   --to-destination 192.168.1.88:23  */
	if( !strcmp(proto,"tcp") || !strcmp(proto,"both") ){
	   // save2file("-A PREROUTING -i %s -p tcp -m tcp -d %s --dport %s "
            save2file("-A PREROUTING -i %s -p tcp -m tcp -s %s -d %s --dport %s "
		    "-j DNAT --to-destination %s%d:%s\n"
		   //, wanface, nvram_safe_get("wan_ipaddr"), wan_port0, lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0);
                    , wanface, src_ipaddr, nvram_safe_get("wan_ipaddr"), wan_port0, lan_cclass, get_single_ip(lan_ipaddr,3),
lan_port0);

	    snprintf(buff, sizeof(buff), "-A FORWARD -p tcp "
		    "-m tcp -d %s%d --dport %s -j %s\n"
		    , lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0, log_accept);

	    count += strlen(buff) + 1;
	    suspense = realloc(suspense, count );
	    strcat(suspense, buff );
	}
	if( !strcmp(proto,"udp") || !strcmp(proto,"both") ){
	   // save2file("-A PREROUTING -i %s -p udp -m udp -d %s --dport %s "
            save2file("-A PREROUTING -i %s -p udp -m udp -s %s -d %s --dport %s "
		    "-j DNAT --to-destination %s%d:%s\n"
		   //, wanface, nvram_safe_get("wan_ipaddr"), wan_port0, lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0);
                    , wanface, src_ipaddr, nvram_safe_get("wan_ipaddr"), wan_port0, lan_cclass, get_single_ip(lan_ipaddr,3),
lan_port0);

	    snprintf(buff, sizeof(buff), "-A FORWARD -p udp "
		    "-m udp -d %s%d --dport %s -j %s\n"
		    , lan_cclass, get_single_ip(lan_ipaddr,3), lan_port0, log_accept);

	    count += strlen(buff) + 1;
	    suspense = realloc(suspense, count );
	    strcat(suspense, buff); 
	}
    }
}

static void parse_spec_forward(char *wordlist)
{
    char var[256], *next;
    char *name, *enable, *proto, *from, *to, *ip;
    char buff[256];


    /* name:enable:proto:ext_port>ip:int_port name:enable:proto:ext_port>ip:int_port */
    foreach(var, wordlist, next) {
	enable = var;
	name = strsep(&enable, ":");
	if (!name || !enable)
	    continue;
	proto = enable;
	enable = strsep(&proto, ":");
	if (!enable || !proto)
	    continue;
	from = proto;
	proto = strsep(&from, ":");
	if (!proto || !from)
	    continue;
	to = from;
	from = strsep(&to, ">");
	if (!to || !from)
	    continue;
	ip = to;
	to = strsep(&ip, ":");
	if (!ip || !to)
	    continue;

	/* skip if it's disabled */
	if( strcmp(enable, "off") == 0 )
	    continue;

	/* -A PREROUTING -i eth1 -p tcp -m tcp -d 192.168.88.11 --dport 823 -j DNAT 
	   --to-destination 192.168.1.88:23  */
	if( !strcmp(proto,"tcp") || !strcmp(proto,"both") ){
	    save2file("-A PREROUTING -p tcp -m tcp -d %s --dport %s "
		      "-j DNAT --to-destination %s%s:%s\n",
		      wanaddr, from, lan_cclass, ip, to);

	    snprintf(buff, sizeof(buff), "-A FORWARD -p tcp "
		    "-m tcp -d %s%s --dport %s -j %s\n",
		    lan_cclass, ip, to, log_accept);

	    count += strlen(buff) + 1;
	    suspense = realloc(suspense, count );
	    strcat(suspense, buff );
	}
	if (!strcmp(proto,"udp") || !strcmp(proto,"both")) {
	    save2file("-A PREROUTING -p udp -m udp -d %s --dport %s "
		    "-j DNAT --to-destination %s%s:%s\n",
		    wanaddr, from, lan_cclass, ip, to);

	    snprintf(buff, sizeof(buff), "-A FORWARD -p udp "
		    "-m udp -d %s%s --dport %s -j %s\n",
		    lan_cclass, ip, to, log_accept);

	    count += strlen(buff) + 1;
	    suspense = realloc(suspense, count );
	    strcat(suspense, buff); 
	}
    }
}
/*****************************************
 * added by tanghui @ 2006-03-20
 * Name		: netmask_to_cidr *
 * Desc		: convert netmask string to CIDR
 * Input	: netmask string
 * Output	: CIDR
 * Return	: success with CIDR, fail with -1
 */
static int netmask_to_cidr(char *netmask)
{
	int a, flag, ret;
	int b[4];
	int i, j;

	sscanf(netmask, "%d.%d.%d.%d", &b[0], &b[1], &b[2], &b[3]);

	flag = 0;
	ret = 0;
	for(i = 0; i < 4; i++)
	{
		a = (1 << 7);
		for(j = 0; j < 8; j++)
		{
			if(0 == flag)
			{
				if((a & b[i]) == 0)
				{
					flag = 1;
				}
				else
				{
					ret ++;
					a = (a >> 1);
				}
			}
			else
			{
				if((a & b[i]) != 0)
					return -1;
			}
		}
	}
	return ret;
}

#ifdef GOOGLE_SUPPORT
static void google_nat_prerouting(void)
{
	char *pass_host = nvram_safe_get("google_pass_host");
	char *pass_mac = nvram_safe_get("google_pass_mac");
	char *next, *next1;
    char dnames[254], dname[254], proto[10];
    char ports[254], port[254];
	char mac[20];
	int count;
    struct ip_lists *ip_list = NULL;
    int i;

	/* Restore nvram to file */
	if(!nvram_match("google_dns", "") && !is_exist(GOOGLE_DNS_FILE)) {
		nvram2file("google_dns", GOOGLE_DNS_FILE);	
	}

	save2file("-A PREROUTING -i %s -j google\n", lanface);

	/* Forward all DNS query to dnsmasq */
        save2file("-A google -p udp -m udp  -i %s --dport %d "
                "-j DNAT --to-destination %s:%d\n",
                lanface, DNS_PORT,
                nvram_safe_get("lan_ipaddr"), DNS_PORT);

	/* Scan google_pass_host table */
        foreach(dnames, pass_host, next) {
                /* Format: name:port1,port2,port3,.... */
                sscanf(dnames, "%[^:]:%[^:]:%s", dname, proto, ports);
                _foreach(port, ports, next1, ",", ',') {
			ip_list = find_dns_ip(GOOGLE_DNS_FILE, dname, &count, FULL_SAME);
			/* Due to he domain name is not only, so we use IP address to replace domain name wiht IP. */
			/* The IP Address is from dnsmasq. */
			for(i=0;i<count;i++)
				save2file("-A google -p %s -m %s -i %s --dport %s -d %s -j ACCEPT\n", proto, proto, lanface, port, ip_list[i].ip);
			if(ip_list)     free(ip_list);
                }
        }

	/* Scan google_pass_mac table */
	foreach(mac, pass_mac, next) {
		save2file("-A google -i %s -p tcp -m mac --mac-source %s -j ACCEPT \n",
			lanface, mac);
	}

	/* Redirect all 80 port to local 80 port */
	save2file("-A google -i %s -p tcp --dport %d "
		"-j DNAT --to-destination  %s:%d \n",
		lanface, HTTP_PORT,
		nvram_safe_get("lan_ipaddr"), HTTP_PORT);

	/* Redirect all 5280 port to local 80 port */
	save2file("-A google -i %s -p tcp --dport %d "
		"-j DNAT --to-destination  %s:%d \n",
		lanface, 5280,
		nvram_safe_get("lan_ipaddr"), HTTP_PORT);
}
#endif

static void nat_prerouting(void)
{
    int i;
    char buf[256];
	char tmp[80];
    char *private_ip, *public_ip, *enable, *ifname, *ifup;
    char *ipaddr, *netmask, *gwmode;

    /* Drop incoming packets which destination IP address is to our LAN side directly */
    if (nvram_match("wk_mode", "gateway")
#ifdef UNNUMBERIP_SUPPORT
    && nvram_invmatch("wan_proto", "unnumberip")
#endif
    )
	save2file("-A PREROUTING -i %s -d %s%s -j DROP\n",
		  wanface, lan_cclass, "0/24");

    /* Enable remote management */
    if (remotemanage) 
     {	
         save2file("-A PREROUTING -p tcp -m tcp -d %s --dport %s "
		  "-j DNAT --to-destination %s:%d\n",
		  wanaddr, nvram_safe_get("http_wanport"),
		  nvram_safe_get("lan_ipaddr"), web_lanport);
     
       #ifdef __CONFIG_UTELNETD__
		if(strcmp(nvram_safe_get("telnet_enable"), "1") == 0)
		{
			save2file("-A PREROUTING -p tcp -m tcp -d %s --dport %s "
		  		"-j DNAT --to-destination %s:%s\n",
		  		wanaddr, nvram_safe_get("telnet_port"),
		  		nvram_safe_get("lan_ipaddr"), nvram_safe_get("telnet_port"));
		}
	#endif

	#ifdef __CONFIG_DROPBEAR__
		if(strcmp(nvram_safe_get("ssh_enable"), "1") == 0)
		{
			save2file("-A PREROUTING -p tcp -m tcp -d %s --dport %s "
		  		"-j DNAT --to-destination %s:%s\n",
		 	 	wanaddr, nvram_safe_get("ssh_port"),
		  		nvram_safe_get("lan_ipaddr"), nvram_safe_get("ssh_port"));
		}
	#endif






      }
    /* ICMP packets are always redirected to INPUT chains */
    save2file("-A PREROUTING -p icmp -d %s -j DNAT --to-destination %s\n",
	      wanaddr, nvram_safe_get("lan_ipaddr"));

#ifdef GOOGLE_SUPPORT
        if(nvram_match("google_enable", "1"))
                google_nat_prerouting();
#endif

    /* Enable remote upgrade */
    if (nvram_match("remote_upgrade", "1"))
	save2file("-A PREROUTING -p udp -m udp -d %s --dport %d "
		  "-j DNAT --to-destination %s\n",
		  wanaddr, TFTP_PORT, nvram_safe_get("lan_ipaddr"));

    /* Initiate suspense string for  parse_port_forward() */
    suspense = malloc(1);
    fw_suspense = malloc(1);
    *suspense = 0;
    *fw_suspense = 0;

    if (nvram_match("wk_mode", "gateway")
#ifdef UNNUMBERIP_SUPPORT
    && nvram_invmatch("wan_proto", "unnumberip")
#endif
    ){
	/* Port forwarding */
	parse_upnp_forward();
#ifdef ALG_FORWARD_SUPPORT
	parse_alg_forward();
#endif
	parse_spec_forward(nvram_safe_get("forward_spec"));
	parse_port_forward(nvram_safe_get("forward_port"));

#ifdef THROUGHPUT_TEST_SUPPORT
	if(nvram_match("throughput_test","0"))
#endif
#ifdef PORT_TRIGGER_SUPPORT
		/* Port triggering */
		save2file("-A PREROUTING -d %s -j TRIGGER --trigger-type DNAT\n",
			wanaddr);
#endif
    }

    /* DMZ forwarding */
    if (dmzenable)
	save2file("-A PREROUTING -d %s -j DNAT --to-destination %s%s\n",
		  wanaddr, lan_cclass, nvram_safe_get("dmz_ipaddr"));
}


static void nat_postrouting(void)
{
    int i;
    char buf[256];	
    char *private_ip, *public_ip;
    char *ipaddr, *netmask;

    if (nvram_match("wk_mode", "gateway")
#ifdef UNNUMBERIP_SUPPORT
    && nvram_invmatch("wan_proto", "unnumberip")
#endif
    ){
#ifdef SIP_ALG_SUPPORT
	save2file("-A POSTROUTING -p udp -m udp -o %s --sport 5060:5070 -j MASQUERADE "
			"--to-ports 5056-5071\n", wanface);
#endif
save2file("-A POSTROUTING -m mark --mark %d -j ACCEPT\n", MARK_NBSRELAY);
			

sprintf(buf, "lan_ipaddr");
		ipaddr = nvram_safe_get(buf);
		sprintf(buf, "lan_netmask");
		netmask = nvram_safe_get(buf);
		save2file("-A POSTROUTING -o %s -s %s/%d -j MASQUERADE\n", wanface, ipaddr, netmask_to_cidr(netmask));



if((nvram_match("wan_proto", "pptp") || nvram_match("wan_proto", "l2tp"))
		   && (strcmp(wanface, "ppp+") == 0))
		{
			sprintf(buf, "lan_ipaddr");
			ipaddr = nvram_safe_get(buf);
			sprintf(buf, "lan_netmask");
			netmask = nvram_safe_get(buf);
			save2file("-A POSTROUTING -o %s -s %s/%d -j MASQUERADE\n"
				, nvram_safe_get("wan_ifname"), ipaddr, netmask_to_cidr(netmask));
			
		}

	if (nvram_match("block_loopback", "0")) {
	    /* Allow "LAN to LAN" loopback connection.
	     * DMZ and port-forwarding could match it. */
	    save2file("-A POSTROUTING -o %s -s %s%s -d %s%s -j MASQUERADE\n",
		    lanface, lan_cclass, "0/24", lan_cclass, "0/24" );
	}
	else {
	    save2file("-A POSTROUTING -o %s -s %s%s -d %s%s -j DROP\n",
		    lanface, lan_cclass, "0/24", lan_cclass, "0/24" );
	}
    }
}

static void parse_port_filter(char *wordlist)
{
    char var[256], *next;
    char *protocol, *lan_port0, *lan_port1;


    /* Parse protocol:lan_port0-lan_port1 ... */
    foreach(var, wordlist, next) {
	lan_port0 = var;
	protocol = strsep(&lan_port0, ":");
	if (!protocol || !lan_port0)
	    continue;
	lan_port1 = lan_port0;
	lan_port0 = strsep(&lan_port1, "-");
	if (!lan_port0 || !lan_port1)
	    continue;

	if( !strcmp(protocol,"disable") )
	    continue;

	/* -A FORWARD -i br0 -p tcp -m tcp --dport 0:655 -j logdrop 
	   -A FORWARD -i br0 -p udp -m udp --dport 0:655 -j logdrop */
	if (!strcmp(protocol,"tcp") || !strcmp(protocol,"both")) {
	    save2file("-A FORWARD -i %s -p tcp -m tcp --dport %s:%s -j %s\n",
		      lanface, lan_port0, lan_port1, log_drop);
	}
	if (!strcmp(protocol,"udp") || !strcmp(protocol,"both")) {
	    save2file("-A FORWARD -i %s -p udp -m udp --dport %s:%s -j %s\n", 
		      lanface, lan_port0, lan_port1, log_drop);
	}
    }
}

/* Return 1 for match, 0 for accept, -1 for partial. */
static int find_pattern(const char *data, size_t dlen,
			const char *pattern, size_t plen,
			char term, 
			unsigned int *numoff,
			unsigned int *numlen)
{
    size_t i, j, k;

    //DEBUGP("find_pattern `%s': dlen = %u\n", pattern, dlen);
    if (dlen == 0)
	return 0;

    if (dlen <= plen) {
	/* Short packet: try for partial? */
	if (strncmp(data, pattern, dlen) == 0)
	    return -1;
	else return 0;
    }

    for(i=0; i<= (dlen - plen); i++){
	if( memcmp(data + i, pattern, plen ) != 0 ) continue;	

	/* patten match !! */	
	*numoff=i + plen;
	for (j=*numoff, k=0; data[j] != term; j++, k++)
	    if( j > dlen ) return -1 ;	/* no terminal char */

	*numlen = k;
	return 1;
    }

    return 0;
}

static int match_wday(char *wday)
{
    int wd[7]={0, 0, 0, 0, 0, 0, 0};
    char sep[]=",";
    char *token;
	char *token_2;
int st, end;	
    int i;
//cprintf("in match_wday wday==%s\n",wday);



	token = strtok(wday, sep);
    while( token != NULL ){
	//cprintf("in match_wday token==%s\n",token);
	if( sscanf(token, "%d-%d", &st, &end) == 2)
	    for(i=st ; i<= end ; i++)
		wd[i] = 1;
	else
	    wd[atoi(token)] = 1;

	token = strtok(NULL, sep);
    }







    DEBUG("week map=%d%d%d%d%d%d%d\n", wd[0], wd[1], wd[2], wd[3], wd[4], wd[5], wd[6]);
    DEBUG("now_wday=%d, match_wday()=%d\n", now_wday, wd[now_wday]);
//cprintf("week map=%d%d%d%d%d%d%d\n", wd[0], wd[1], wd[2], wd[3], wd[4], wd[5], wd[6]);
//    cprintf("now_wday=%d, match_wday()=%d\n", now_wday, wd[now_wday]);
	return wd[now_wday];
}	

static int match_hrmin(int hr_st, int mi_st, int hr_end, int mi_end) 
{
    unsigned int hm_st, hm_end;

    /* convert into %d%2d format */
    hm_st  = hr_st  * 100 + mi_st;
    hm_end = hr_end * 100 + mi_end;

//	cprintf("\n hr_st==%d, mi_st==%d, hr_end==%d, mi_end=%d\n",hr_st, mi_st,  hr_end, mi_end);

//	cprintf("now_hrmin==%d now_wday==%d\n",now_hrmin,now_wday);
    if( hm_st < hm_end ){
	if( now_hrmin < hm_st || now_hrmin > hm_end ) return 0;
    }
    else{		// time rotate
	if( now_hrmin < hm_st && now_hrmin > hm_end ) return 0;
    }

    return 1;
}

/*
 * PARAM - seq : Seqence number.
 *
 * RETURN - 0 : Data error or be disabled until in scheduled time.
 *          1 : Enabled.
 */
static int schedule_by_tod(int seq, int d)
{
	char todname[]="filter_todxxx";
	char *todvalue;
	char *temp_rule;
	char *temp_rule2;
	
	
	int sched = 0, allday = 0;
	int hr_st, hr_end;	/* hour */
	int mi_st, mi_end;	/* minute */
	char wday[128];
	char wday_2[128];
	int intime=0;

    /* Get the NVRAM data */
    sprintf(todname, "filter_tod%d", seq);
    todvalue = nvram_safe_get(todname);

	temp_rule = nvram_safe_get( "filter_rule1");
    
	//cprintf("xujinjin_sched_name==%s: value==%s,filter1==%s\n", todname, todvalue,temp_rule);

	



	
	DEBUG("%s: %s\n", todname, todvalue);
    if (strcmp(todvalue, "") == 0)
    {
		//cprintf("todvalue return\n");
		return 0;
    }
    /* Is it anytime or scheduled ? */
    if (strcmp(todvalue, "0:0 23:59 0-0") == 0 ||
        strcmp(todvalue, "0:0 23:59 0-6") == 0) 
      {

		sched = 0;		//24 hours and 7 days
		
	}
    else {
	sched = 1;
	//cprintf("enter 0177\n");
	if (strncmp(todvalue, "0:0 23:59",9) == 0)//modufy by xjj
	{

			//cprintf("allday change to 1\n");
			allday = 1;//24 hours
	}
	if (sscanf(todvalue, "%d:%d %d:%d %s", &hr_st, &mi_st,
		    &hr_end, &mi_end, wday) != 5)
		{
			
			return 0; /* error format */

		}
		else
		{
			int i;
			for(i=0;i<127;i++)
			wday_2[i]=wday[i];
		}
	}	

    DEBUG("sched=%d, allday=%d\n", sched, allday);
    /* Anytime */

	//cprintf("sched=%d, allday=%d\n", sched, allday);

	if (!sched) {
		
		intime_stat[seq]=1;
	
				
		return 1;
    	}
	
	
	/* Scheduled */
    if (allday) {	/* 24-hour, but not everyday */
	char wday_st[64]; 
	char	wday_end[64];	/* for crontab */
	int rotate = 0;		/* wday continugoue */
	char sep[]=",";		/* wday seperate character */
	char *token;		
	int st, end;	
 	wday_st[0]='\0';
	wday_end[0]='\0';
//	cprintf("wday_2==%s\n",wday_2);
	/* If its format looks like as "0-1,3,5-6" */
	if (*wday_2 == '0')
	{
	    if (*(wday_2 + strlen(wday_2) - 1) == '6')
		rotate = 1;
	}
	/* Parse the 'wday' format for crontab */
	token = strtok(wday_2, sep);
	//cprintf("1__token===%s\n",token);
	while (token != NULL) {
	    /* which type of 'wday' ? */
		//cprintf("token===%s\n",token);
	if (sscanf(token, "%d-%d", &st, &end) != 2)
		{
			end = atoi(token);
			st = end;
		}	

		//cprintf("strlen(wday_end)==%d,strlen(wday_st)==%d\n",strlen(wday_end),strlen(wday_st));
	    if (rotate == 1 && st == 0)
		sprintf(wday_end + strlen(wday_end), ",%d", end);
	    else if (rotate == 1 && end == 6)
		sprintf(wday_st  + strlen(wday_st),  ",%d", st );
	    else {
		sprintf(wday_st  + strlen(wday_st),  ",%d", st );
		sprintf(wday_end + strlen(wday_end), ",%d", end);
	    }
	//cprintf("st = =%d,end==%d\n",st,end);
	//cprintf("wday_st==%s,wday_end==%s\n",wday_st,wday_end);
	//cprintf("st==%d,end==%d,rotate ==%d,\n",st,end,rotate);
	    token = strtok(NULL, sep);
	}

	/* Write to crontab for triggering the event */
	/* "wday_xx + 1" can ignor the first character ',' */
	fprintf(cfd, "%02d %2d * * %s root filter add %d\n", 
		mi_st, hr_st, wday_st + 1, seq); 

	//cprintf("mi_st+1==%d,hr_st==%d,wday==%s,seq==%d\n",mi_st, hr_st, wday_st, seq);
	/************************************
	 * modify by tanghui @ 2006-05-29
	 ************************************/
	fprintf(cfd, "%02d %2d * * %s root filter del %d\n", 
		mi_end+1, hr_end, wday_end + 1, seq); 

	//cprintf("mi_end+1==%d,hr_st==%d,wday==%s,seq==%d\n",(mi_end+1), hr_end, wday_end, seq);
	/************************************/
	if (match_wday(wday_2))
	    intime=1;
    }
    else {		/* Nither 24-hour, nor everyday */
	/* Write to crontab for triggering the event */
	fprintf(cfd, "%02d %2d * * %s root filter add %d\n", 
		mi_st, hr_st, wday, seq); 
	//cprintf("mi_st+2==%d,hr_st==%d,wday==%s,seq==%d\n",mi_st, hr_st, wday, seq);
	
	/*************************************
	 * modify by tanghui @ 2006-05-29
	 *************************************/ 
	fprintf(cfd, "%02d %2d * * %s root filter del %d\n", 
		mi_end+1, hr_end, wday, seq); 
	/*************************************/
	//cprintf("mi_end+2==%d,hr_end==%d,wday==%s,seq==%d\n",mi_end+1, hr_end, wday, seq);
	if (match_wday(wday_2) && 
		match_hrmin(hr_st, mi_st, hr_end, mi_end))
	    intime=1;
    }

    /* Would it be enabled now ? */
    DEBUG("intime=%d\n", intime);
    if (intime) {
		
	intime_stat[seq]=1;
		
				
	return 1;
    }


    return 0;
}


static void macgrp_chain_2(int seq, unsigned int mark, int mode)
{
    char var[256], *next;
    char buf[100];
    char *wordlist;

    sprintf(buf, "filter_mac_grp%d", seq);
    wordlist = nvram_safe_get(buf);
    if (strcmp(wordlist, "") == 0)
	return;


	if(mode==1)//enter first in time
	{
	
    if (mark == MARK_DROP) {
	foreach(var, wordlist, next) {
			char grp[100];
			char input[100];
			sprintf(grp, "grp_%d", seq);
			sprintf(input, "input_%d", seq);
			eval("iptables", "-A", grp, "-m","mac","--mac-source", var,"-j",log_drop);
	//		save2file("-A grp_%d -m mac --mac-source %s -j %s\n"
	//	    ,seq, var, log_drop);
#if 1  //for filter issue(DNS relay will trigger PPPOE DOD)
eval("iptables","-I",input,"1","-p","udp","-m","udp","--dport","53","-m","mac","--mac-source", var,"-i","br0","-j",
log_drop);
//save2file("-I INPUT 1 -p udp -m udp --dport %d -m mac --mac-source %s -i br0 -j %s\n"
//		    ,DNS_PORT, var, log_drop);
#endif
	}
    }
    else {
	foreach(var, wordlist, next) {
			char grp[20];
			char advgrp[20];
			sprintf(grp, "grp_%d", seq);
			sprintf(advgrp, "advgrp_%d", seq);

			eval("iptables","-A",grp,"-m","mac","--mac-source",var,"-j",advgrp);

			

		//	save2file("-A grp_%d -m mac --mac-source %s -j advgrp_%d\n"
		//    ,seq, var, seq);

	    /*
	       mark = urlenable  ? mark : webfilter  ? MARK_HTTP : 0;
	       if (mark) {
	       save2file("-A macgrp_%d -p tcp -m tcp --dport %d -m mac "
	       "--mac-source %s -j MARK --set-mark %d\n"
	       , seq, HTTP_PORT, var, mark);
	       }
	       */
	}
    }
	}
	else
	{
	
    if (mark != MARK_DROP) {
	foreach(var, wordlist, next) {
			char grp[100];
			char input[100];
			
			sprintf(input, "input_%d", seq);
			sprintf(grp, "grp_%d", seq);
			eval("iptables", "-A", grp, "-m","mac","--mac-source", var,"-j",log_drop);
	//		save2file("-A grp_%d -m mac --mac-source %s -j %s\n"
	//	    ,seq, var, log_drop);


#if 1  //for filter issue(DNS relay will trigger PPPOE DOD)
eval("iptables","-I",input,"1","-p","udp","-m","udp","--dport","53","-m","mac","--mac-source", var,"-i","br0","-j",
log_drop);
//save2file("-I INPUT 1 -p udp -m udp --dport %d -m mac --mac-source %s -i br0 -j %s\n"
//		    ,DNS_PORT, var, log_drop);
#endif
	}
    }
    else {
	foreach(var, wordlist, next) {
			char grp[20];
			char advgrp[20];
			sprintf(grp, "grp_%d", seq);
			sprintf(advgrp, "advgrp_%d", seq);

			eval("iptables","-A",grp,"-m","mac","--mac-source",var,"-j",advgrp);
		

	//		eval("iptables","-I","INPUT","1","-p","udp","-m","udp","--dport","53","-m","mac","--mac-source", var,"-i","br0","-j",
//"DROP");
		//	save2file("-A grp_%d -m mac --mac-source %s -j advgrp_%d\n"
		//    ,seq, var, seq);

	    /*
	       mark = urlenable  ? mark : webfilter  ? MARK_HTTP : 0;
	       if (mark) {
	       save2file("-A macgrp_%d -p tcp -m tcp --dport %d -m mac "
	       "--mac-source %s -j MARK --set-mark %d\n"
	       , seq, HTTP_PORT, var, mark);
	       }
	       */
	}
    }
	}
}





static void portgrp_chain_2(int seq, unsigned int mark, int mode)
{
    char var[256], *next;
    char *wordlist;
    char buf[100];
    char target[100];
    char *protocol, *lan_port0, *lan_port1;


    sprintf(buf, "filter_dport_grp%d", seq);
    wordlist = nvram_safe_get(buf);
    if (strcmp(wordlist, "") == 0)
	return;

    /* Determine the filter target */
    if (mark == MARK_DROP) 
	strncpy(target, log_drop, sizeof(log_drop));
    else 
	sprintf(target, "advgrp_%d", seq);

    /* Parse protocol:lan_port0-lan_port1 ... */
    foreach(var, wordlist, next) {
	lan_port0 = var;
	protocol = strsep(&lan_port0, ":");
	if (!protocol || !lan_port0)
	    continue;
	lan_port1 = lan_port0;
	lan_port0 = strsep(&lan_port1, "-");
	if (!lan_port0 || !lan_port1)
	    continue;

	if (!strcmp(protocol,"disable"))
	    continue;

	/* -A grp_* -p tcp -m tcp --dport 0:655 -j logdrop 
	   -A grp_* -p udp -m udp --dport 0:655 -j logdrop */
if(mode==1){
	if (!strcmp(protocol,"tcp") || !strcmp(protocol,"both")) {
			char grp[20];
			sprintf(grp, "grp_%d", seq);
		eval("iptables","-A",grp,"-p","tcp","-m","tcp","--dport",lan_port0,":",lan_port1,"-j",target);
	//		save2file("-A grp_%d -p tcp -m tcp --dport %s:%s -j %s\n",
	//	    seq, lan_port0, lan_port1, target);
	}
	if (!strcmp(protocol,"udp") || !strcmp(protocol,"both")) {
		char grp[20];
			sprintf(grp, "grp_%d", seq);

eval("iptables","-A",grp,"-p","udp","-m","udp","--dport",lan_port0,":",lan_port1,"-j",target);
	//		save2file("-A grp_%d -p udp -m udp --dport %s:%s -j %s\n", 
	//	    seq, lan_port0, lan_port1, target);
	}
    }
    	}
}

static void advgrp_chain_2(int seq, unsigned int mark, int mode)
{
    char nvname[100];
    char *wordlist, word[1024], *next;
    char *services, srv[1024], *next2;
    char delim[] = "<&nbsp;>";
	int log_level=0;
    log_level = atoi(nvram_safe_get("log_level"));
 
   
    
  
    			  
    
/* filter_services=$NAME:006:My ICQ$PROT:002:17$PORT:009:5000:5010<&nbsp;>.. */
    services= nvram_safe_get("filter_services");

    /* filter_port_grp5=My ICQ<&nbsp;>Game boy */
    sprintf(nvname, "filter_port_grp%d", seq);
    wordlist = nvram_safe_get(nvname);
    split(word, wordlist, next, delim) {
	DEBUG("word=%s\n", word);

	split(srv, services, next2, delim) {
	    int len = 0;
	    char *name, *prot, *port;
	    char protocol[100], ports[100];

	    if ((name=strstr(srv, "$NAME:")) == NULL ||
		    (prot=strstr(srv, "$PROT:")) == NULL ||
		    (port=strstr(srv, "$PORT:")) == NULL) 
		continue;

	    /* $NAME */
	    if (sscanf(name, "$NAME:%3d:", &len) != 1 || strlen(word) != len) 
		continue;
	    if (memcmp(name + sizeof("$NAME:nnn:") - 1, word, len) != 0) 
		continue;

	    /* $PROT */
	    if (sscanf(prot, "$PROT:%3d:", &len) != 1) 
		continue;
	    strncpy(protocol, prot + sizeof("$PROT:nnn:") - 1, len);
	    protocol[len] = '\0';

	    /* $PORT */
	    if (sscanf(port, "$PORT:%3d:", &len) != 1) 
		continue;
	    strncpy(ports, port + sizeof("$PORT:nnn:") - 1, len);
	    ports[len] = '\0';

	    DEBUG("match:: name=%s, protocol=%s, ports=%s\n", 
		    word, protocol, ports);
	    if (!strcmp(protocol,"tcp") || !strcmp(protocol,"both"))
	    {
			char advgrp[20];
			sprintf(advgrp, "advgrp_%d", seq);
			eval("iptables","-A",advgrp,"-p","tcp","-m","tcp","--dport",ports,"-j",log_drop);
	//		save2file("-A advgrp_%d -p tcp -m tcp --dport %s -j %s\n",
	//		  seq, ports, log_drop);
	    	}
			if (!strcmp(protocol,"udp") || !strcmp(protocol,"both"))
			{
			char advgrp[20];
			sprintf(advgrp, "advgrp_%d", seq);
			eval("iptables","-A",advgrp,"-p","udp","-m","udp","--dport",ports,"-j",log_drop);

	//		save2file("-A advgrp_%d -p udp -m udp --dport %s -j %s\n", 
	//		  seq, ports, log_drop);
				}
			if (!strcmp(protocol,"icmp"))
				{
			char advgrp[20];
			sprintf(advgrp, "advgrp_%d", seq);
			eval("iptables","-A",advgrp,"-p","icmp","-j",log_drop);


				}
	}
    }
    /* filter_web_host2=hello<&nbsp;>world<&nbsp;>friend */
    sprintf(nvname, "filter_web_host%d", seq);
    wordlist = nvram_safe_get(nvname);
    if (strcmp(wordlist, "")) {
			char advgrp[20];
			sprintf(advgrp, "advgrp_%d", seq);


			//cprintf("log_level==%d",log_level);
			if( log_level & 1) 
			
			eval("iptables","-A",advgrp,"-p","tcp","-m","tcp","-m","webstr","--host",wordlist,"-j","logreject");

			else
			eval("iptables","-A",advgrp,"-p","tcp","-m","tcp","-m","webstr","--host",wordlist,"-j","REJECT", "--reject-with", "tcp-reset");



//	save2file("-A advgrp_%d -p tcp -m tcp -m webstr --host \"%s\" -j %s\n",
//	  seq, wordlist, log_reject);
    }
    /* filter_web_url3=hello<&nbsp;>world<&nbsp;>friend */
    sprintf(nvname, "filter_web_url%d", seq);
    wordlist = nvram_safe_get(nvname);
    if (strcmp(wordlist, "")) {

			char advgrp[20];
			sprintf(advgrp, "advgrp_%d", seq);
			if( log_level & 1) 
				eval("iptables","-A",advgrp,"-p","tcp","-m","tcp","-m","webstr","--url",wordlist,"-j","logreject");
			else
				eval("iptables","-A",advgrp,"-p","tcp","-m","tcp","-m","webstr","--url",wordlist,"-j","REJECT", "--reject-with", "tcp-reset");

	//save2file("-A advgrp_%d -p tcp -m tcp -m webstr --url \"%s\" -j %s\n",
	//	  seq, wordlist, log_reject);
    }

    // 2005-10-26 by kanki
/* 2006-03-03 by tanghui */
#if 0
    if (mark != MARK_DROP) {
	save2file("-A advgrp_%d -j ACCEPT\n", seq);
    }
#endif
}


static void macgrp_chain(int seq, unsigned int mark, int urlenable)
{
    char var[256], *next;
    char buf[100];
    char *wordlist;

    sprintf(buf, "filter_mac_grp%d", seq);
    wordlist = nvram_safe_get(buf);
    if (strcmp(wordlist, "") == 0)
	return;


	if((intime_stat[seq]==1)||(ntp_update_stat==30)||(ntp_time_web_stat==0))
	{
	
    if (mark == MARK_DROP) {
	foreach(var, wordlist, next) {
	    save2file("-A grp_%d -m mac --mac-source %s -j %s\n"
		    ,seq, var, log_drop);
#if 1  //for filter issue(DNS relay will trigger PPPOE DOD)
	    save2file("-I input_%d 1 -p udp -m udp --dport %d -m mac --mac-source %s -i br0 -j %s\n"
		    ,seq,DNS_PORT, var, log_drop);
#endif
	}
    }
    else {
	foreach(var, wordlist, next) {
	    save2file("-A grp_%d -m mac --mac-source %s -j advgrp_%d\n"
		    ,seq, var, seq);

	    /*
	       mark = urlenable  ? mark : webfilter  ? MARK_HTTP : 0;
	       if (mark) {
	       save2file("-A macgrp_%d -p tcp -m tcp --dport %d -m mac "
	       "--mac-source %s -j MARK --set-mark %d\n"
	       , seq, HTTP_PORT, var, mark);
	       }
	       */
	}
    }
	}
	else
	{
		{
	
    if (mark != MARK_DROP) {
	foreach(var, wordlist, next) {
	    save2file("-A grp_%d -m mac --mac-source %s -j %s\n"
		    ,seq, var, log_drop);
#if 1  //for filter issue(DNS relay will trigger PPPOE DOD)
	    save2file("-I input_%d 1 -p udp -m udp --dport %d -m mac --mac-source %s -i br0 -j %s\n"
		    ,seq,DNS_PORT, var, log_drop);
#endif
	}
    }
    else {
	foreach(var, wordlist, next) {
	    save2file("-A grp_%d -m mac --mac-source %s -j advgrp_%d\n"
		    ,seq, var, seq);

	    /*
	       mark = urlenable  ? mark : webfilter  ? MARK_HTTP : 0;
	       if (mark) {
	       save2file("-A macgrp_%d -p tcp -m tcp --dport %d -m mac "
	       "--mac-source %s -j MARK --set-mark %d\n"
	       , seq, HTTP_PORT, var, mark);
	       }
	       */
	}
    }
	}
	}
}

static void ipgrp_chain(int seq, unsigned int mark, int urlenable)
{
    char buf[256];
    char var1[256], *wordlist1, *next1;
    char var2[256], *wordlist2, *next2;
    char from[100], to[100];
    int  a1 = 0, a2 = 0;


    sprintf(buf, "filter_ip_grp%d", seq);
    wordlist1 = nvram_safe_get(buf);
    if (strcmp(wordlist1, "") == 0)
	return ;

    foreach (var1, wordlist1, next1) {
	if (sscanf(var1, "%d-%d", &a1, &a2) == 2) {
	    if (a1 == 0 && a2 == 0)	/* unset */
		continue;
	    if (a1 == 0)		/* from 1 */
		a1 = 1;

	    snprintf(from, sizeof(from), "%s%d", lan_cclass, a1);
	    snprintf(to,   sizeof(to),   "%s%d", lan_cclass, a2);
	    /* The return value of range() is global string array */

			//cprintf("lan_Class==%s\n",lan_cclass);
			wordlist2 = range(from, to);
	}
	else if (sscanf(var1, "%d", &a1) == 1) {
	    if (a1 == 0)		/* unset */
		continue;

	    snprintf(buf, sizeof(buf), "%s%d", lan_cclass, a1);
	    wordlist2 = buf;
	}
	else
	    continue;

	DEBUG("range=%s\n", wordlist2);

if((intime_stat[seq]==1)||(ntp_update_stat==30)||(ntp_time_web_stat==0)){
	if (mark == MARK_DROP) {
	    foreach(var2, wordlist2, next2) {
		save2file("-A grp_%d -s %s -j %s\n",
			seq, var2, log_drop);
#if 1	//for filter issue(DNS relay will trigger PPPOE DOD)
	        save2file("-I input_%d 1 -p udp -m udp --dport %d -s %s -i br0 -j %s\n"
		        ,seq,DNS_PORT, var2, log_drop);
#endif
	    }
	}
	else {
	    foreach(var2, wordlist2, next2) {
		save2file("-A grp_%d -s %s -j advgrp_%d\n",
			seq, var2, seq);
	    }
	}
    }
else
{
	{
	if (mark != MARK_DROP) {
	    foreach(var2, wordlist2, next2) {
		save2file("-A grp_%d -s %s -j %s\n",
			seq, var2, log_drop);
#if 1	//for filter issue(DNS relay will trigger PPPOE DOD)
	        save2file("-I input_%d 1 -p udp -m udp --dport %d -s %s -i br0 -j %s\n"
		        ,seq,DNS_PORT, var2, log_drop);
#endif
	    }
	}
	else {
	    foreach(var2, wordlist2, next2) {
		save2file("-A grp_%d -s %s -j advgrp_%d\n",
			seq, var2, seq);
	    }
	}
    }
}



	}
    // 2005-10-26 by kanki
/* 2006-03-03 by tanghui */
#if 0
    if (mark != MARK_DROP) {
	save2file("-A grp_%d -j DROP\n", seq);
    }
#endif
}


static void ipgrp_chain_2(int seq, unsigned int mark, int mode)
{
    char buf[256];
    char var1[256], *wordlist1, *next1;
    char var2[256], *wordlist2, *next2;
    char from[100], to[100];
    int  a1 = 0, a2 = 0;
	 char lan_cclass[13];
		char *lan_ipaddr;
//	 char wanaddr[]="xxx.xxx.xxx.xxx";
	lan_ipaddr=nvram_get("lan_ipaddr");
	//cprintf("lan_ipaddr==%s\n",lan_ipaddr);
	
	{
		int i,q;
		q=0;
		for(i=0;q<3;i++)
		{
		lan_cclass[i]=lan_ipaddr[i];
		if(lan_ipaddr[i]=='.')
		q++;
		}
		lan_cclass[i]='\0';
	}
	//cprintf("lan_cclass==%s\n",lan_cclass);
    sprintf(buf, "filter_ip_grp%d", seq);
    wordlist1 = nvram_safe_get(buf);
    if (strcmp(wordlist1, "") == 0)
	return ;
	//cprintf("no return\n");
    foreach (var1, wordlist1, next1) {
	if (sscanf(var1, "%d-%d", &a1, &a2) == 2) {
	    if (a1 == 0 && a2 == 0)	/* unset */
		continue;
	    if (a1 == 0)		/* from 1 */
		a1 = 1;

	    snprintf(from, sizeof(from), "%s%d", lan_cclass, a1);
	    snprintf(to,   sizeof(to),   "%s%d", lan_cclass, a2);
	    /* The return value of range() is global string array */
	    wordlist2 = range(from, to);
	}
	else if (sscanf(var1, "%d", &a1) == 1) {
	    if (a1 == 0)		/* unset */
		continue;

	    snprintf(buf, sizeof(buf), "%s%d", lan_cclass, a1);
	    wordlist2 = buf;
	}
	else
	    continue;

	DEBUG("range=%s\n", wordlist2);

if(mode==1){
	if (mark == MARK_DROP) {
	    foreach(var2, wordlist2, next2) {
		char grp[20];
		char input[50];
		sprintf(grp, "grp_%d", seq);
		sprintf(input, "input_%d", seq);
		
	eval("iptables", "-A", grp, "-s",var2,"-j",log_drop);
//		save2file("-A grp_%d -s %s -j %s\n",
//			seq, var2, "DROP");
#if 1	//for filter issue(DNS relay will trigger PPPOE DOD)
			eval("iptables","-I",input,"1","-p","udp","-m","udp","--dport","53","-s",var2,"-i","br0","-j",log_drop);

//save2file("-I INPUT 1 -p udp -m udp --dport %d -s %s -i br0 -j %s\n"
//		        ,DNS_PORT, var2, "DROP");
#endif
	    }

			//cprintf("add rule 111\n");
	}
	else {
	    foreach(var2, wordlist2, next2) {

		char grp[20];
		char advgrp[20];
		sprintf(grp, "grp_%d", seq);
		sprintf(advgrp, "advgrp_%d", seq);
		eval("iptables","-A",grp,"-s",var2,"-j",advgrp);
				
	//	save2file("-A grp_%d -s %s -j advgrp_%d\n",
	//		seq, var2, seq);

				//cprintf("save new rule 222\n");
			}
	}
    }
else
{
	{
	if (mark != MARK_DROP) {
	    foreach(var2, wordlist2, next2) {
		char grp[20];
		char input[50];
		sprintf(grp, "grp_%d", seq);
		sprintf(input, "input_%d", seq);
		
	eval("iptables", "-A", grp, "-s",var2,"-j",log_drop);
//		save2file("-A grp_%d -s %s -j %s\n",
//			seq, var2, "DROP");
#if 1	//for filter issue(DNS relay will trigger PPPOE DOD)
			eval("iptables","-I",input,"1","-p","udp","-m","udp","--dport","53","-s",var2,"-i","br0","-j",log_drop);

//save2file("-I INPUT 1 -p udp -m udp --dport %d -s %s -i br0 -j %s\n"
//		        ,DNS_PORT, var2, "DROP");
#endif
	    }

			//cprintf("add rule 111\n");
	}
	else {
	    foreach(var2, wordlist2, next2) {

		char grp[20];
		char advgrp[20];
		sprintf(grp, "grp_%d", seq);
		sprintf(advgrp, "advgrp_%d", seq);
		eval("iptables","-A",grp,"-s",var2,"-j",advgrp);
				
	//	save2file("-A grp_%d -s %s -j advgrp_%d\n",
	//		seq, var2, seq);

				//cprintf("save new rule 222\n");
			}
	}
    }
}



	}
    // 2005-10-26 by kanki
/* 2006-03-03 by tanghui */
#if 0
    if (mark != MARK_DROP) {
	save2file("-A grp_%d -j DROP\n", seq);
    }
#endif
}
static void portgrp_chain(int seq, unsigned int mark, int urlenable)
{
    char var[256], *next;
    char *wordlist;
    char buf[100];
    char target[100];
    char *protocol, *lan_port0, *lan_port1;


    sprintf(buf, "filter_dport_grp%d", seq);
    wordlist = nvram_safe_get(buf);
    if (strcmp(wordlist, "") == 0)
	return;

    /* Determine the filter target */
    if (mark == MARK_DROP) 
	strncpy(target, log_drop, sizeof(log_drop));
    else 
	sprintf(target, "advgrp_%d", seq);

    /* Parse protocol:lan_port0-lan_port1 ... */
    foreach(var, wordlist, next) {
	lan_port0 = var;
	protocol = strsep(&lan_port0, ":");
	if (!protocol || !lan_port0)
	    continue;
	lan_port1 = lan_port0;
	lan_port0 = strsep(&lan_port1, "-");
	if (!lan_port0 || !lan_port1)
	    continue;

	if (!strcmp(protocol,"disable"))
	    continue;

	/* -A grp_* -p tcp -m tcp --dport 0:655 -j logdrop 
	   -A grp_* -p udp -m udp --dport 0:655 -j logdrop */
if((intime_stat[seq]==1)||(ntp_update_stat==30)||(ntp_time_web_stat==0)){
	if (!strcmp(protocol,"tcp") || !strcmp(protocol,"both")) {
	    save2file("-A grp_%d -p tcp -m tcp --dport %s:%s -j %s\n",
		    seq, lan_port0, lan_port1, target);
	}
	if (!strcmp(protocol,"udp") || !strcmp(protocol,"both")) {
	    save2file("-A grp_%d -p udp -m udp --dport %s:%s -j %s\n", 
		    seq, lan_port0, lan_port1, target);
	}
    }
    	}
}

static void advgrp_chain(int seq, unsigned int mark, int urlenable)
{
    char nvname[100];
    char *wordlist, word[1024], *next;
    char *services, srv[1024], *next2;
    char delim[] = "<&nbsp;>";

    
    
  
    			  
    
/* filter_services=$NAME:006:My ICQ$PROT:002:17$PORT:009:5000:5010<&nbsp;>.. */
    services= nvram_safe_get("filter_services");

    /* filter_port_grp5=My ICQ<&nbsp;>Game boy */
    sprintf(nvname, "filter_port_grp%d", seq);
    wordlist = nvram_safe_get(nvname);
    split(word, wordlist, next, delim) {
	DEBUG("word=%s\n", word);

	split(srv, services, next2, delim) {
	    int len = 0;
	    char *name, *prot, *port;
	    char protocol[100], ports[100];

	    if ((name=strstr(srv, "$NAME:")) == NULL ||
		    (prot=strstr(srv, "$PROT:")) == NULL ||
		    (port=strstr(srv, "$PORT:")) == NULL) 
		continue;

	    /* $NAME */
	    if (sscanf(name, "$NAME:%3d:", &len) != 1 || strlen(word) != len) 
		continue;
	    if (memcmp(name + sizeof("$NAME:nnn:") - 1, word, len) != 0) 
		continue;

	    /* $PROT */
	    if (sscanf(prot, "$PROT:%3d:", &len) != 1) 
		continue;
	    strncpy(protocol, prot + sizeof("$PROT:nnn:") - 1, len);
	    protocol[len] = '\0';

	    /* $PORT */
	    if (sscanf(port, "$PORT:%3d:", &len) != 1) 
		continue;
	    strncpy(ports, port + sizeof("$PORT:nnn:") - 1, len);
	    ports[len] = '\0';

	    DEBUG("match:: name=%s, protocol=%s, ports=%s\n", 
		    word, protocol, ports);
	    if (!strcmp(protocol,"tcp") || !strcmp(protocol,"both"))
		save2file("-A advgrp_%d -p tcp -m tcp --dport %s -j %s\n",
			  seq, ports, log_drop);
	    if (!strcmp(protocol,"udp") || !strcmp(protocol,"both"))
		save2file("-A advgrp_%d -p udp -m udp --dport %s -j %s\n", 
			  seq, ports, log_drop);
	    if (!strcmp(protocol,"icmp"))
		save2file("-A advgrp_%d -p icmp -j %s\n", 
			  seq, log_drop);
	}
    }
    /* filter_web_host2=hello<&nbsp;>world<&nbsp;>friend */
    sprintf(nvname, "filter_web_host%d", seq);
    wordlist = nvram_safe_get(nvname);
    if (strcmp(wordlist, "")) {
	save2file("-A advgrp_%d -p tcp -m tcp -m webstr --host \"%s\" -j %s\n",
		  seq, wordlist, log_reject);
    }
    /* filter_web_url3=hello<&nbsp;>world<&nbsp;>friend */
    sprintf(nvname, "filter_web_url%d", seq);
    wordlist = nvram_safe_get(nvname);
    if (strcmp(wordlist, "")) {
	save2file("-A advgrp_%d -p tcp -m tcp -m webstr --url \"%s\" -j %s\n",
		  seq, wordlist, log_reject);
    }

    // 2005-10-26 by kanki
/* 2006-03-03 by tanghui */
#if 0
    if (mark != MARK_DROP) {
	save2file("-A advgrp_%d -j ACCEPT\n", seq);
    }
#endif
}

static int find_filter_rule(int cur, int d)
{
    FILE *fd;
    char buf[100];
    char sep[]=",";
    char *token;

    int i, k;
    int array[100];
    char * iptables_rule_stat;

	if(d == 0)
		iptables_rule_stat = IPTABLES_OUT_RULE_STAT;
	else if(d == 1)
		iptables_rule_stat = IPTABLES_IN_RULE_STAT;
	else
	{
		printf("Directory = %d is not correct!!!\n", d);
		return -1;
	}

    /* Read active-rule bitmap */
	if( (fd=fopen(iptables_rule_stat, "r")) == NULL )
	{
		printf("Can't open %s\n", iptables_rule_stat);
		return -1;
	}
	fgets(buf, sizeof(buf), fd);

	i = 1;
    token = strtok(buf, sep);
	while( token != NULL ){
		if((*token != '0') && (*token != '1') && (*token != '8'))
			break;

		array[i] = atoi(token);
		i++;
		token = strtok(NULL, sep);
	}

    fclose(fd);

	for(k = cur; k < i; k ++)
	{
		if(array[k] == 1)
			return k;
	}
	return 0; // no rule found.
}




static int find_pre_seq(int seq, int d)
{
    FILE *fd;
    char buf[100];
    char sep[]=",";
    char *token;

    int i, k;
    int array[100];
    char * iptables_rule_stat;

	if(d == 0)
		iptables_rule_stat = IPTABLES_OUT_RULE_STAT;
	else if(d == 1)
		iptables_rule_stat = IPTABLES_IN_RULE_STAT;
	else
	{
		printf("Directory = %d is not correct!!!\n", d);
		return -1;
	}

    /* Read active-rule bitmap */
	if( (fd=fopen(iptables_rule_stat, "r")) == NULL )
	{
		printf("Can't open %s\n", iptables_rule_stat);
		return -1;
	}
	fgets(buf, sizeof(buf), fd);

	i = 1;
    token = strtok(buf, sep);
	while( token != NULL ){
		if((*token != '0') && (*token != '1') && (*token != '8'))
			break;

		array[i] = atoi(token);
		i++;
		token = strtok(NULL, sep);
	}

    fclose(fd);

	k = 0;
	for(i = seq-1; i > 0; i--)
	{
		if(array[i] == 1)
		{
			k = i;
			break;
		}
	}
	return k;
}

static int find_post_seq(int seq, int d)
{
    FILE *fd;
    char buf[100];
    char sep[]=",";
    char *token;

    int i, k;
    int array[100];
    char * iptables_rule_stat;

	if(d == 0)
		iptables_rule_stat = IPTABLES_OUT_RULE_STAT;
	else if(d == 1)
		iptables_rule_stat = IPTABLES_IN_RULE_STAT;
	else
	{
		printf("Directory = %d is not correct!!!\n", d);
		return -1;
	}

    /* Read active-rule bitmap */
	if( (fd=fopen(iptables_rule_stat, "r")) == NULL )
	{
		printf("Can't open %s\n", iptables_rule_stat);
		return -1;
	}
	fgets(buf, sizeof(buf), fd);

	i = 1;
    token = strtok(buf, sep);
	while( token != NULL ){
		if((*token != '0') && (*token != '1') && (*token != '8'))
			break;

		array[i] = atoi(token);
		i++;
		token = strtok(NULL, sep);
	}

    fclose(fd);

	k = 0;
	for(i = seq+1; i <= NR_RULES; i++)
	{
		if(array[i] == 1)
		{
			k = i;
			break;
		}
	}
	return k;
}

static void adjust_advgrp_chains(int mode, int seq, int d)
{
	int pre_seq, post_seq;
	char pre_grp[20], pre_advgrp[20];
	char post_grp[20], post_advgrp[20];
	char cur_grp[20], cur_advgrp[20];

	if((pre_seq = find_pre_seq(seq, d)) < 0)
		return;
	if((post_seq = find_post_seq(seq, d)) < 0)
		return;

	if(pre_seq > 0)
	{
		sprintf(pre_grp, "grp_%d", pre_seq);
		sprintf(pre_advgrp, "advgrp_%d", pre_seq);
	}

	if(post_seq > 0)
	{
		sprintf(post_grp, "grp_%d", post_seq);
		sprintf(post_advgrp, "advgrp_%d", post_seq);
	}

	sprintf(cur_grp, "grp_%d", seq);
	sprintf(cur_advgrp, "advgrp_%d", seq);

	if(mode == 1)	// insert
	{
		// pre rule
		if(pre_seq > 0)
		{
			// first delete pre to post link
			if(post_seq > 0)
			{
				DEBUG("iptables -D %s -j %s\n", pre_advgrp, post_grp);
				eval("iptables", "-D", pre_advgrp, "-j", post_grp);

				DEBUG("iptables -A %s -j %s\n", pre_advgrp, cur_grp);
				eval("iptables", "-A", pre_advgrp, "-j", cur_grp);

				DEBUG("iptables -A %s -j %s\n", cur_advgrp, post_grp);
				eval("iptables", "-A", cur_advgrp, "-j", post_grp);
			}
			else
			{
				DEBUG("iptables -D %s -j ACCEPT\n", pre_advgrp);
				eval("iptables", "-D", pre_advgrp, "-j", "ACCEPT");
				DEBUG("iptables -D %s -j ACCEPT\n", pre_grp);
				eval("iptables", "-D", pre_grp, "-j", "ACCEPT");

				DEBUG("iptables -A %s -j %s\n", pre_advgrp, cur_grp);
				eval("iptables", "-A", pre_advgrp, "-j", cur_grp);

				DEBUG("iptables -A %s -j ACCEPT\n", cur_advgrp);
				eval("iptables", "-A", cur_advgrp, "-j", "ACCEPT");
				DEBUG("iptables -A %s -j ACCEPT\n", cur_grp);
				eval("iptables", "-A", cur_grp, "-j", "ACCEPT");
			}
		}
		else
		{
			if(post_seq > 0)
			{
				DEBUG("iptables -A %s -j %s\n", cur_advgrp, post_grp);
				eval("iptables", "-A", cur_advgrp, "-j", post_grp);
			}
			else
			{
				DEBUG("iptables -A %s -j ACCEPT\n", cur_advgrp);
				eval("iptables", "-A", cur_advgrp, "-j", "ACCEPT");
				DEBUG("iptables -A %s -j ACCEPT\n", cur_grp);
				eval("iptables", "-A", cur_grp, "-j", "ACCEPT");
			}
		}
	}
	else	//delete
	{
		// pre rule
		if(pre_seq > 0)
		{
			// first delete pre to cur link
			DEBUG("iptables -D %s -j %s\n", pre_advgrp, cur_grp);
			eval("iptables", "-D", pre_advgrp, "-j", cur_grp);

			// then add pre to post link
			if(post_seq > 0)
			{
				DEBUG("iptables -D %s -j %s\n", cur_advgrp, post_grp);
				eval("iptables", "-D", cur_advgrp, "-j", post_grp);

				DEBUG("iptables -A %s -j %s\n", pre_advgrp, post_grp);
				eval("iptables", "-A", pre_advgrp, "-j", post_grp);
			}
			else
			{
				DEBUG("iptables -D %s -j ACCEPT\n", cur_advgrp);
				eval("iptables", "-D", cur_advgrp, "-j", "ACCEPT");
				DEBUG("iptables -D %s -j ACCEPT\n", cur_grp);
				eval("iptables", "-D", cur_grp, "-j", "ACCEPT");
				
				DEBUG("iptables -A %s -j ACCEPT\n", pre_advgrp);
				eval("iptables", "-A", pre_advgrp, "-j", "ACCEPT");
				DEBUG("iptables -A %s -j ACCEPT\n", pre_grp);
				eval("iptables", "-A", pre_grp, "-j", "ACCEPT");
			}
		}
		else
		{
			if(post_seq > 0)
			{
				DEBUG("iptables -D %s -j %s\n", cur_advgrp, post_grp);
				eval("iptables", "-D", cur_advgrp, "-j", post_grp);
			}
			else
			{
				DEBUG("iptables -D %s -j ACCEPT\n", cur_advgrp);
				eval("iptables", "-D", cur_advgrp, "-j", "ACCEPT");
				DEBUG("iptables -D %s -j ACCEPT\n", cur_grp);
				eval("iptables", "-D", cur_grp, "-j", "ACCEPT");
			}
		}
	}
}

static void lan2wan_chains(void)
{
    time_t ct;      /* Calendar time */
    struct tm *bt;  /* Broken time */
    int seq;
    char buf[]="filter_rulexxx";
    char *data;
    int offset, len;
    unsigned int mark = 0;
    int up = 0;
    int  urlfilter = 1;
    //char urlhost[] ="filter_url_hostxxx";
    //char urlkeywd[]="filter_url_keywdxxx";
    int lock_fd;

	/* check if it's locked and lock it */
	do
	{
		lock_fd = open(IPTABLES_RULE_STAT_LOCK, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL);
		if(lock_fd == -1)
		{
			DEBUG("%s() try locking!!!\n", __FUNCTION__);
			DEBUG("%s\n", strerror(errno));
			usleep(IPTABLES_INTERVAL);
		}
		else
		{
			if(close(lock_fd) == -1)
			{
				DEBUG("%s():LOCK error!\n", __FUNCTION__);
				goto fail;
				//return ;
			}
		}
	} while(lock_fd == -1);

	DEBUG("%s():locked !!!\n", __FUNCTION__);

    /* Get local calendar time */
    time(&ct);
    bt=localtime(&ct);
{
	time_t tm;

	time(&tm);

	if(time(0) > (unsigned long)60*60*24*365)
		{
			ntp_update_stat=1;
			filtersync_firewall=0;
		}
	else
		{
		ntp_update_stat=30;//time not aviable
		
		filtersync_firewall=1;
		}
	
	
   }

    /* Convert to 3-digital format */
    now_hrmin = bt->tm_hour * 100 + bt->tm_min;
    now_wday = bt->tm_wday;

	//cprintf("in firewall now_hrmin==%d,now_wday==%d\n",now_hrmin,now_wday);

    /* keep the status using bitmap */
	if ((ifd=fopen(IPTABLES_OUT_RULE_STAT, "w")) == NULL) {
		printf("Can't open %s\n", IPTABLES_OUT_RULE_STAT);
		goto fail;
		//exit(1);
    }

    /* Open the crontab file for modification */
	//if ((cfd=fopen(CRONTAB, "w")) == NULL) {
	#if 0
	if ((cfd=fopen(CRONTAB, "a")) == NULL) {
		DEBUG("Can't open %s\n", CRONTAB);
		goto fail;
		//exit(1);
    }	
	//fprintf(cfd, "PATH=/sbin:/bin:/usr/sbin:/usr/bin\n\n");
	#endif
	{
		int temp_10;
		for(temp_10=1;temp_10<NR_RULES+1;temp_10++)
		intime_stat[temp_10]=0;

	}
#if defined(REVERSE_RULE_ORDER)
    for (seq=NR_RULES; seq >= 1; seq--) {
#else
    for (seq=1; seq <= NR_RULES; seq++) {
#endif

		/* check if it's an outgoing filter rule */
		sprintf(buf, "filter_in_out%d", seq);
		data = nvram_safe_get(buf);
		if(strcmp(data, "0"))
		{
			up = 8;
			fprintf(ifd, "%d,", up);
			continue;
		}

		sprintf(buf, "filter_rule%d", seq);
		data = nvram_safe_get(buf);

		if( strcmp( data, "") == 0)
	   	 		up = 0;
		else
		{

	/* Check if it is enabled */
				find_pattern(data, strlen(data), "$STAT:", 
					sizeof("$STAT:")-1, '$', &offset, &len);
		//cprintf("offset===%d,len==%d\n",offset,len);
			if (len < 1) 
		    	up = 0;	/* error format */
			else
			{

				strncpy(buf, data + offset, len);
				*(buf+len) = 0;
				DEBUG("STAT: %s\n", buf);
				//cprintf("STAT:%s\n",buf);
				switch (atoi(buf)) {
		    		case 1:		/* deny */
						deny_stat=1;
				
				case 2:	/*accept */
					/* URL checking */
					/********************************
		 			 * modify by tanghui @ 2006-05-10
					 ********************************/
		    			up = schedule_by_tod(seq, 0);
					/********************************/
						break;
					default:	/* jump to next iteration */
						up = 0;
						break;
				}// end of switch
			}// end of if(len < 1)
		}// end of if(strcmp...)

	fprintf(ifd, "%d,", up);
    }

    fclose(ifd);
    //fclose(cfd);

    for (seq=1; seq <= (NR_RULES); seq++)
    {
	sprintf(buf, "filter_rule%d", seq);				
	data = nvram_safe_get(buf);		
	if( strcmp(data , "") == 0)		    
	{
		continue;		
	}

	if( strncmp(data,"$STAT:0",7)==0)			
	{
		continue;
	}
	save2file("-A lan2wan -j grp_%d\n", seq);
	

    }

	#if 0
	{
		int log_level=0;
		 log_level= atoi(nvram_safe_get("log_level"));
		if(log_level & 2)
		save2file("-A grp_11 -j logaccept\n");


	}
	#endif

for (seq=1; seq <= NR_RULES; seq++) {






		/* check if it's an outgoing filter rule */
		sprintf(buf, "filter_in_out%d", seq);
		data = nvram_safe_get(buf);
		if(strcmp(data, "0"))
			continue;
		
		sprintf(buf, "filter_rule%d", seq);
		
		data = nvram_safe_get(buf);
		if( strcmp(data , "") == 0)
		    continue;

	/* Check if it is enabled */
		find_pattern(data, strlen(data), "$STAT:", 
			sizeof("$STAT:")-1, '$', &offset, &len);

		if (len < 1) 
		    continue;	/* error format */

		strncpy(buf, data + offset, len);
		*(buf+len) = 0;
		DEBUG("STAT: %s\n", buf);

		switch (atoi(buf)) {
		    case 1:		/* Drop it */
				mark = MARK_DROP;
				break;
		    case 2:		/* URL checking */
				mark = MARK_OFFSET + seq;
				break;
		    default:	/* jump to next iteration */
				continue;
		}


if(intime_stat[seq]==1)
{	rule_run_stat[seq]=1;		//for filtersync_main sync
}
else{
	if(mark==MARK_DROP)
	{
rule_run_stat[seq]=5;		// only refresh grp_n
		}
	else
		{
rule_run_stat[seq]=10;	}	//refresh grp_n and advgrp_n
}
	/*
	   sprintf(urlhost,  "filter_url_host%d",  seq);
	   sprintf(urlkeywd, "filter_url_keywd%d", seq);
	   if (nvram_match(urlhost, "") && nvram_match(urlkeywd, ""))
	   urlfilter = 0;

	   DEBUG("host=%s, keywd=%s\n", urlhost, urlkeywd);
	   */

//		if(time_Not_Available==1)
	//		intime_stat[seq]=0;
		macgrp_chain(seq, mark, urlfilter);
	        ipgrp_chain(seq, mark, urlfilter);
		portgrp_chain(seq, mark, urlfilter);
		
             if((intime_stat[seq]==1)||(ntp_update_stat==30)||(ntp_time_web_stat==0))
		advgrp_chain(seq, mark, urlfilter);//url match 
    }

 

	fail:
	/* unlock it now */
	if(unlink(IPTABLES_RULE_STAT_LOCK) == -1)
	{
		DEBUG("%s(): UNLOCK error!\n", __FUNCTION__);
	}
	DEBUG("%s(): unlocked!!!\n", __FUNCTION__);
}

#ifdef DHCP_FILTER_SUPPORT
static void dhcpfilter_chains(void)
{
    char var[256], *next;
    char *wordlist, *policy;

    wordlist = nvram_safe_get("dhcp_mac_list");
    policy = nvram_safe_get("dhcp_filter_policy");
    if (strcmp(wordlist, "") == 0)
	return;

    /* dhcp_filter_policy - 0: Disabled, 1: Deny, 2: Allow */
    if (!strcmp(policy, "1")) 
    {
	foreach(var, wordlist, next) {
	    save2file("-A dhcpfilter -m mac --mac-source %s -j %s\n",
		    var, log_drop);
	}
	/* Others will be accepted */
	save2file("-A dhcpfilter -j %s\n", log_accept);
    }
    else if (!strcmp(policy, "2"))
    {
	foreach(var, wordlist, next) {
	    save2file("-A dhcpfilter -m mac --mac-source %s -j %s\n",
		    var, log_accept);
	}
	/* Others will be dropped */
	save2file("-A dhcpfilter -j %s\n", log_drop);
    }
}
#endif


#ifdef PORT_TRIGGER_SUPPORT
static void parse_trigger_out(char *wordlist)
{
    char var[256], *next;
    char *name, *enable, *proto;
    char *wport0, *wport1, *lport0, *lport1;

    /* port_trigger=name:[on|off]:[tcp|udp|both]:wport0-wport1>lport0-lport1 */
    foreach(var, wordlist, next) {
	enable = var;
	name = strsep(&enable, ":");
	if (!name || !enable)
	    continue;
	proto = enable;
	enable = strsep(&proto, ":");
	if (!enable || !proto)
	    continue;
	wport0 = proto;
	proto = strsep(&wport0, ":");
	if (!proto || !wport0)
	    continue;
	wport1 = wport0;
	wport0 = strsep(&wport1, "-");
	if (!wport0 || !wport1)
	    continue;
	lport0 = wport1;
	wport1 = strsep(&lport0, ">");
	if (!wport1 || !lport0)
	    continue;
	lport1 = lport0;
	lport0 = strsep(&lport1, "-");
	if (!lport0 || !lport1)
	    continue;

	/* skip if it's disabled */
	if( strcmp(enable, "off") == 0 )
	    continue;

	if( !strcmp(proto,"tcp") || !strcmp(proto,"udp") ){
	    save2file("-A trigger_out -p %s -m %s --dport %s:%s "
		      "-j TRIGGER --trigger-type out --trigger-proto %s "
		      "--trigger-match %s-%s --trigger-relate %s-%s\n",
		      proto, proto, wport0, wport1, proto,
		      wport0, wport1, lport0, lport1);
	}
	else if(!strcmp(proto,"both") ){
	    save2file("-A trigger_out -p tcp -m tcp --dport %s:%s "
		      "-j TRIGGER --trigger-type out --trigger-proto all "
		      "--trigger-match %s-%s --trigger-relate %s-%s\n",
		      wport0, wport1, wport0, wport1, lport0, lport1);
	    save2file("-A trigger_out -p udp -m udp --dport %s:%s "
		      "-j TRIGGER --trigger-type out --trigger-proto all "
		      "--trigger-match %s-%s --trigger-relate %s-%s\n",
		      wport0, wport1, wport0, wport1, lport0, lport1);
	}
    }
}
#endif /* PORT_TRIGGER_SUPPORT */

static void filter_input(void)
{
	{
		int seq;
		char buf_private[32];
		char *data = NULL;

		memset(buf_private,'\0',32);
			
		for (seq=1; seq <= NR_RULES; seq++)
		{
      			 sprintf(buf_private, "filter_rule%d", seq);                             
      			 data = nvram_safe_get(buf_private);             
    			 if(strcmp(data , "") == 0)                 
      			{
               		continue;               
       		}
 
       		if( strncmp(data,"$STAT:0",7)==0)                       
     			{
              		continue;
       		}
 
    			save2file("-A INPUT -j input_%d\n", seq);
	
			memset(buf_private,'\0',32);
    		}
	}

#ifdef DHCP_FILTER_SUPPORT
    if (nvram_invmatch("dhcp_filter_policy", "0"))
    {
	save2file("-A INPUT -i %s -p udp -m udp --sport %d -j dhcpfilter\n",
		lanface,  DHCP_HOST_PORT);
	dhcpfilter_chains();
    }
#endif

    /* Filter known SPI state */
    save2file("-A INPUT -m state --state INVALID -j DROP\n"
	      "-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n"
	      "-A INPUT -i lo -m state --state NEW -j ACCEPT\n"
	      "-A INPUT -i %s -m state --state NEW -j %s\n"
	      , lanface, "ACCEPT");

    /* Routing protocol, RIP, accept */
    if (nvram_invmatch("dr_wan_rx", "0"))
	save2file("-A INPUT -p udp -m udp --dport %d -j %s\n"
		, RIP_PORT, TARG_PASS);

    /* Remote Management 
     * Use interface name, destination address, and port to make sure
     * that it's redirected from WAN */
    if (remotemanage)
	{
	save2file("-A INPUT -p tcp -m tcp -d %s --dport %d -j %s\n"
		, nvram_safe_get("lan_ipaddr"), web_lanport, log_accept); //2005-10-27 by kanki

		/******************************************
		 * add by tanghui @ 2006-05-17
		 * support telnet and ssh remotemanage
		 ******************************************/
	#ifdef __CONFIG_UTELNETD__
		if(strcmp(nvram_safe_get("telnet_enable"), "1") == 0)
		{
			save2file("-A INPUT -p tcp -m tcp -d %s --dport %s -j %s\n"
				, nvram_safe_get("lan_ipaddr"),  nvram_safe_get("telnet_port"), log_accept);
		}
	#endif

	#ifdef __CONFIG_DROPBEAR__
		if(strcmp(nvram_safe_get("ssh_enable"), "1") == 0)
		{
			save2file("-A INPUT -p tcp -m tcp -d %s --dport %s -j %s\n"
				, nvram_safe_get("lan_ipaddr"),  nvram_safe_get("ssh_port"), log_accept);
		}
	#endif
		/******************************************/
	}

    /* ICMP request from WAN interface */
    save2file("-A INPUT -p icmp -j %s\n", nvram_match("block_wan", "1") ? log_drop : TARG_PASS);

#ifdef MULTICAST_SUPPORT
    /* IGMP query from WAN interface */
    save2file("-A INPUT -p igmp -j %s\n", nvram_match("multicast_pass", "0") ? log_drop : TARG_PASS);
#endif

    /* Remote Upgrade */
    if( nvram_match("remote_upgrade", "1") )
	save2file("-A INPUT -p udp -m udp --dport %d -j %s\n", TFTP_PORT, TARG_PASS);

    /* Ident request backs by telnet or IRC server */
    if( nvram_match("ident_pass", "1") )
	save2file("-A INPUT -p tcp -m tcp --dport %d -j %s\n", IDENT_PORT, TARG_PASS);

    /* Drop those packets we are NOT recognizable */
    save2file( "-A INPUT -j %s\n", log_drop);
}


void filter_output(void)
{

}

#ifdef GOOGLE_SUPPORT
void google_filter_forward(void)
{
	char *pass_mac = nvram_safe_get("google_pass_mac");
	char *pass_host = nvram_safe_get("google_pass_host");
	char mac[20];
	char *next, *next1;
        char dnames[254], dname[254], proto[10];
        char ports[254], port[254];
	int count;
        struct ip_lists *ip_list = NULL;
        int i;

	save2file("-A FORWARD -i %s -j google\n", lanface);

	/* Scan google_pass_mac table */
        foreach(mac, pass_mac, next) {
		save2file("-A google -i %s -o %s -m mac --mac-source %s -j ACCEPT\n", lanface, wanface, mac);
        }

	/* Scan google_pass_host table */
        foreach(dnames, pass_host, next) {
                /* Format: name:proto:port1,port2,port3,.... */
                sscanf(dnames, "%[^:]:%[^:]:%s", dname, proto, ports);
                _foreach(port, ports, next1, ",", ',') {
			ip_list = find_dns_ip(GOOGLE_DNS_FILE, dname, &count, FULL_SAME);
			/* Due to he domain name is not only, so we use IP address to replace domain name wiht IP. */
			/* The IP Address is from dnsmasq. */
			for(i=0;i<count;i++)
				save2file("-A google -p %s -m %s --dport %s -d %s -j ACCEPT\n", proto, proto, port, ip_list[i].ip);
			if(ip_list)     free(ip_list);
                }
        }

	/* Allow DNS query */
        save2file("-A google -p udp -m udp --dport %d -j ACCEPT\n", DNS_PORT);

        /* Block all LAN to WAN packets except the above IP/PORT */
        save2file("-A google -j DROP\n");
}
#endif


void filter_forward(void)
{
  
 if(!(nvram_match("filter","off") || nvram_match("wk_mode", "router")
#ifdef UNNUMBERIP_SUPPORT
    || nvram_match("wan_proto", "unnumberip")
#endif
    ))
	{
	
    /* Accept the redirect, might be seen as INVALID, packets */
    save2file("-A FORWARD -i %s -o %s -j ACCEPT\n", lanface, lanface);

    /* Drop the wrong state, INVALID, packets */
    save2file("-A FORWARD -m state --state INVALID -j DROP\n");

#ifdef THROUGHPUT_TEST_SUPPORT
    if(nvram_match("throughput_test","0"))
#endif
    	/* Clamp TCP MSS to PMTU of WAN interface */
    	save2file("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss %d: -j TCPMSS "
		"--set-mss %d\n", atoi(nvram_safe_get("wan_run_mtu"))-39, atoi(nvram_safe_get("wan_run_mtu"))-40);

    /* DROP packets for PPTP pass through. */
    if (nvram_match("pptp_pass", "0")) 
	save2file("-A FORWARD -o %s -p tcp -m tcp --dport %d -j %s\n"
		, wanface, PPTP_PORT, log_drop);

    /* DROP packets for PPTP pass through. */
    if (nvram_match("l2tp_pass", "0")) 
	save2file("-A FORWARD -o %s -p udp -m udp --dport %d -j %s\n"
		, wanface, L2TP_PORT, log_drop);

    /* DROP packets for IPsec pass through */
    if (nvram_match("ipsec_pass", "0")) 
	{
	save2file("-A FORWARD -o %s -p udp -m udp --dport %d -j %s\n"
		, wanface, ISAKMP_PORT, log_drop);
		/***************************************
		 * modify by tanghui @ 2006-04-03
		 * should drop ESP and AH proto port packet after establishing IPSec tunnel
		 ***************************************/ 
		save2file("-A FORWARD -o %s -p %d -j %s\n"
			, wanface, ESP_PPORT, log_drop);
		save2file("-A FORWARD -o %s -p %d -j %s\n"
			, wanface, AH_PPORT, log_drop);
		/******** End of tanghui ****************/
	}

	/************************************
	 * add by tanghui @ 2006-05-10
	 * for incoming packets filter
	 ************************************/



#ifdef MULTICAST_SUPPORT
    /* ACCEPT packets for Multicast pass through */
    if (nvram_match("multicast_pass", "1")) 
	save2file("-A FORWARD -i %s -p udp -m udp --destination %s -j %s\n"
		, wanface, IP_MULTICAST, log_accept);
#endif

    /* Filter Web application */
    if (webfilter)
	{
	save2file("-A FORWARD -i %s -o %s -p tcp -m tcp --dport %d "
		"-m webstr --content %d -j %s\n", 
		lanface, wanface, HTTP_PORT, webfilter, log_reject);
		/*******************************************************************/
		/***************Alpha fixed for proxy filter 2006-7-28*********************/
		/*only support for filter port 80, 8080, 3128, can not work when use other ports**/
		save2file("-A FORWARD -i %s -o %s -p tcp -m tcp --dport 8080 "
		"-m webstr --content %d -j %s\n", 
		lanface, wanface, webfilter, log_reject);

		save2file("-A FORWARD -i %s -o %s -p tcp -m tcp --dport 3128 "
		"-m webstr --content %d -j %s\n", 
		lanface, wanface, webfilter, log_reject);
		/*******************************************************************/
    	}

//	save2file("-A FORWARD -i %s -o %s -p tcp -m tcp --dport %d "
//		"-m webstr --content %d -j %s\n", 
//		lanface, wanface, HTTP_PORT, webfilter, log_reject);

#ifdef THROUGHPUT_TEST_SUPPORT
    if(nvram_match("throughput_test","0"))
#endif
#ifdef PORT_TRIGGER_SUPPORT
    {
	/* Port trigger by user definition */
	/* Incoming connection will be accepted, if it match the port-ranges. */
	save2file("-A FORWARD -i %s -o %s -j TRIGGER --trigger-type in\n", wanface, lanface);
	save2file("-A FORWARD -i %s -j trigger_out\n", lanface);
    }
#endif

#ifdef GOOGLE_SUPPORT
        if(nvram_match("google_enable", "1"))
                google_filter_forward();
#endif

    /* Filter setting by user definition */
    save2file("-A FORWARD -i %s -j lan2wan\n", lanface);

    /* Filter by destination ports "filter_port" */
    parse_port_filter(nvram_safe_get("filter_port"));

    /* port-forwarding accepting rules */
    if (*suspense != 0)
	save2file( "%s", suspense);
    free(suspense);

    /* DMZ forwarding */
    if (dmzenable)
	save2file("-A FORWARD -o %s -d %s%s -j %s\n", lanface, lan_cclass
		, nvram_safe_get("dmz_ipaddr"), log_accept);
save2file("-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n");

     save2file("-A FORWARD -i %s -d %s -m state --state NEW -j %s\n"
	       , wanface, nvram_safe_get("lan_ipaddr"), log_accept);

    /* Accept those established/related connections, otherwise drop it */
    save2file("-A FORWARD -i %s -m state --state NEW -j %s\n"
	      "-A FORWARD -j %s\n", lanface, log_accept, log_drop);
   

}
    if ((cfd=fopen(CRONTAB, "w")) == NULL) {
		DEBUG("Can't open %s\n", CRONTAB);
    }	

	fprintf(cfd, "PATH=/sbin:/bin:/usr/sbin:/usr/bin\n\n");
	
    lan2wan_chains();
    	fclose(cfd);

 if(! (nvram_match("filter","off") || nvram_match("wk_mode", "router")
#ifdef UNNUMBERIP_SUPPORT
    || nvram_match("wan_proto", "unnumberip")
#endif
    ))
	{
		
#ifdef THROUGHPUT_TEST_SUPPORT
    if(nvram_match("throughput_test","0"))
#endif
#ifdef PORT_TRIGGER_SUPPORT
	parse_trigger_out(nvram_safe_get("port_trigger"));
#endif
}
}

#ifdef HW_QOS_SUPPORT
char *ftp_ext[]= {"ftp-data", NULL};

struct application_based_qos_t{
	char *port;
	char *nv_name;
	char *user_port;
	char **port_ext;
} application_based_qos[] = {
	/*{"ftp", "sel_qosftp", NULL, ftp_ext},
	{"http", "sel_qoshttp",},
	{"telnet", "sel_qostelnet",},
	{"smtp", "sel_qossmtp",},
	{"pop3", "sel_qospop3",},*/
	{ NULL, "sel_qosport1", "qos_appport1",},
	{ NULL, "sel_qosport2", "qos_appport2",},
	{ NULL, "sel_qosport3", "qos_appport3",},
	{ NULL, "sel_qosport4", "qos_appport4",},
	{ NULL, "sel_qosport5", "qos_appport5",},
	{ NULL, "sel_qosport6", "qos_appport6",},
	{ NULL, "sel_qosport7", "qos_appport7",},
	{ NULL, "sel_qosport8", "qos_appport8",}
};

/*struct range_port_t{
	unsigned start_port;
	unsigned end_port;
};*/

struct game_port_t{
	/*unsigned **single_port;
	struct range_port_t *range_port;*/
	char **single_port;
	char **range_port;
};

/*
struct range_port_t cs_tcp_range[]={{27030,27039}, {0,0}};
unsigned cs_udp_single[]={1200, 0};
struct range_port_t cs_udp_range[]={{27000,27015}, {0,0}};

unsigned aoe_single[]={6073, 0};
struct range_port_t aoe_range[]={{2302,2400}, {0,0}};

unsigned diablo_tcp_single[]={4000, 0};
struct range_port_t diablo_range[]={{6112,6119}, {0,0}};

unsigned everquest_single[]={7000, 0};
struct range_port_t everquest_range[]={{1024,6000}, {0,0}};

unsigned halflife_tcp_single[]={6003, 7002, 0};
unsigned halflife_udp_single[]={27005, 27010, 27011, 27015, 0};

unsigned quake2_single[]={27910, 0};

unsigned quake3_single[]={27660, 0};

unsigned RCW_udp_single[]={27950, 27960, 27965, 27952, 0};

unsigned unreal_single[]={8080, 27900, 0};
struct range_port_t unreal_range[]={{7777, 7783}, {0,0}};
*/

char *cs_tcp_range[]={"27030:27039", NULL};
char *cs_udp_single[]={"1200", NULL};
char *cs_udp_range[]={"27000:27015", NULL};

char *aoe_single[]={"6073", NULL};
char *aoe_range[]={"2302:2400", NULL};

char *diablo_tcp_single[]={"4000", NULL};
char *diablo_range[]={"6112:6119", NULL};

char *everquest_single[]={"7000", NULL};
char *everquest_range[]={"1024:6000", NULL};

char *halflife_tcp_single[]={"6003", "7002", NULL};
char *halflife_udp_single[]={"27005", "27010", "27011", "27015", NULL};

char *quake2_single[]={"27910", NULL};

char *quake3_single[]={"27660", NULL};

char *RCW_udp_single[]={"27950", "27960", "27965", "27952", NULL};

char *unreal_single[]={"8080", "27900", NULL};
char *unreal_range[]={"7777:7783", NULL};

struct game_port_t cs_port_tcp = {
	//{NULL}, {{27030, 27039}, NULL}
	NULL, cs_tcp_range
};

struct game_port_t cs_port_udp = {
	//{1200, NULL}, {{27000, 27015}, NULL}
	cs_udp_single, cs_udp_range
};

struct game_port_t aoe_port = {
	//{6073, NULL}, {{2302, 2400}, NULL}
	aoe_single, aoe_range
};

struct game_port_t diablo_port_tcp = {
	//{4000, NULL}, {{6112, 6119}, NULL}
	diablo_tcp_single, diablo_range
};

struct game_port_t diablo_port_udp = {
	//{NULL}, {{6112, 6119}, NULL}
	NULL, diablo_range
};

struct game_port_t everquest_port = {
	//{7000, NULL}, {{1024, 6000}, NULL}
	everquest_single, everquest_range
};

struct game_port_t halflife_port_tcp = {
	//{6003, 7002, NULL}, {NULL}
	halflife_tcp_single, NULL
};

struct game_port_t halflife_port_udp = {
	//{27005, 27010, 27011, 27015, NULL}, {NULL}
	halflife_udp_single, NULL
};

struct game_port_t quake2_port = {
	//{27910, NULL}, {NULL}
	quake2_single, NULL
};

struct game_port_t quake3_port = {
	//{27660, NULL}, {NULL}
	quake3_single, NULL
};

struct game_port_t RCW_port_tcp = {
	//{NULL}, {NULL}
	NULL, NULL
};

struct game_port_t RCW_port_udp = {
	//{27950, 27960, 27965,27952, NULL}, {NULL}
	RCW_udp_single, NULL
};

struct game_port_t unreal_port = {
	//{8080, 27900, NULL}, {{7777, 7783}, NULL}
	unreal_single, unreal_range
};

struct gaming_app_t{
	char *name;
	struct game_port_t *tcp_port;
	struct game_port_t *udp_port;
} gamming_app_table[] = {
	{"Counter Strike", &cs_port_tcp, &cs_port_udp},
	{"Age of Empires", &aoe_port, &aoe_port},
	{"Diablo II(Blizzard Battle.net)", &diablo_port_tcp, &diablo_port_udp},
	{"Everquest", &everquest_port, &everquest_port},
	{"Half life", &halflife_port_tcp, &halflife_port_udp},
	{"Quake2", &quake2_port, &quake2_port},
	{"Quake3", &quake3_port, &quake3_port},
	{"Return to Castle Wolfenstein", &RCW_port_tcp, &RCW_port_udp},
	{"Unreal Tournament", &unreal_port, &unreal_port}
};

char *dscp_class_map[] = {
	"BE",
	"AF31",
	"AF11",
	"EF",
};

enum { TCP, UDP };

void app_savefile(char *port, int direction, int protocol, int app_prio)
{
	char *interface = direction ? wanface :lanface;
	char *position = direction ? "--source-port" : "--destination-port";
	
	if (protocol == TCP)
	{
		save2file("-I PREROUTING -i %s -p tcp %s %s -j DSCP --set-dscp-class %s\n", interface, position, port, dscp_class_map[app_prio]);
	}
	else if (protocol == UDP)
	{
		char *argv[] = { "iptables", "-I", "PREROUTING", "-t", "mangle", "-i", interface, "-p", "udp", position, port, "-j", "DSCP", "--set-dscp-class", dscp_class_map[app_prio], NULL };
                pid_t pid;

		_eval(argv, NULL, 0, &pid); 
	}
}

void appitem_savefile(struct application_based_qos_t app_item, char *port, int direction, int protocol, int app_prio)
{
	int i = 0;
	
	app_savefile(port, direction, protocol, app_prio);
		
	if (app_item.port_ext)
		for (i=0; (port = app_item.port_ext[i]); i++)
			app_savefile(port, direction, protocol, app_prio);
}

int check_app_port(char *port, char *validated_port)
{
	char *dash_index;
	int valid_port = 0; 
	
	strcpy(validated_port, port);
	
	if(!strcmp(validated_port, ""))
		return valid_port;
	
	dash_index = strchr(validated_port, '-');
	
	if (dash_index){
		validated_port[dash_index-validated_port] = ':';	
		valid_port = 1;
	}
	else if (atoi(port))
		valid_port = 1;

	return valid_port;
}

void app_udp_settable(void)
{
    if (!strcmp(nvram_safe_get("QoS"),"1"))
    {
	int i = 0;
	char *port = NULL;
	struct application_based_qos_t app_item;
	
	for (i = 0; i<STRUCT_LEN(application_based_qos); i++)
	{
	    int app_prio = 0;
	    char validated_port[12];

	    app_item = application_based_qos[i];
	    app_prio = atoi(nvram_safe_get(app_item.nv_name));
	    
	    //if (!strcmp(nvram_safe_get(app_item.nv_name), "1"))
	    if (app_prio)
	    {
		if (!(port = app_item.port)) {
		    port = nvram_safe_get(app_item.user_port);
		    
		    if (!check_app_port(port, validated_port))
			continue;

		    port = validated_port;
		}

		appitem_savefile(app_item, port, 0, UDP, app_prio);//0:LAN 2 WAN
#if 0		
//#ifdef QDISC_PRIO		
		appitem_savefile(app_item, port, 1, UDP, app_prio);//1:WAN 2 LAN
#endif    		
	    }
	}

	if (nvram_match("enable_game", "1")){
		for (i = 0; i<STRUCT_LEN(gamming_app_table); i++){
			int j = 0;
			struct game_port_t *udp_item = gamming_app_table[i].udp_port;
			if (udp_item->single_port){
				for (j = 0; udp_item->single_port[j]; j++){
					app_savefile(udp_item->single_port[j], 0, UDP, 3);

				}
			}
			
			if (udp_item->range_port){
				for (j = 0; udp_item->range_port[j]; j++){
					app_savefile(udp_item->range_port[j], 0, UDP, 3);
					
				}

			}
		}
	
	}

	
    }		
}
#endif

/*
 *  Mangle table 
 */
static void mangle_table(void)
{
    save2file("*mangle\n"
	      ":PREROUTING ACCEPT [0:0]\n"
	      ":OUTPUT ACCEPT [0:0]\n");

    /* For PPPoE Connect On Demand, to reset idle timer. add by honor (2003-04-17) 
       Reference driver/net/ppp_generic.c					*/
    save2file("-I PREROUTING -i %s -j MARK --set-mark %d\n", lanface, MARK_LAN2WAN);
    
#ifdef HW_QOS_SUPPORT
    if (!strcmp(nvram_safe_get("QoS"),"1"))
    {
	int i = 0;
	char *port = NULL;
	struct application_based_qos_t app_item;
	char *qos_mac, qos_mac_name[] = "qos_devmacX";
	
	for (i=1 ;i<=2; i++)
	{
		int mac_prio = 0;
		snprintf(qos_mac_name, sizeof(qos_mac_name), "qos_devpri%d", i);
		mac_prio = atoi(nvram_safe_get(qos_mac_name));
		
		if (mac_prio)
		//if (nvram_match(qos_mac_name, "1"))
		{
			snprintf(qos_mac_name, sizeof(qos_mac_name), "qos_devmac%d", i);
			qos_mac = nvram_safe_get(qos_mac_name);
	 		printf("\n==qos_mac=%s\n", qos_mac);    
		
			if (strcmp(qos_mac, "") && strcmp(qos_mac, "00:00:00:00:00:00")) 
    			{
#if 0		
//#ifdef QDISC_PRIO		
				char *qos_ip = get_ip_from_mac(qos_mac);
#endif    		
				save2file("-I PREROUTING -i %s -m mac --mac-source %s -j DSCP --set-dscp-class %s\n", lanface, qos_mac, dscp_class_map[mac_prio]);
#if 0		
//#ifdef QDISC_PRIO		
				if (strcmp(qos_ip, "")) 
				{
					save2file("-I PREROUTING -i %s -d %s -j TOS --set-tos Minimize-Delay\n", wanface, qos_ip);
				
				}
#endif    		
			}
		}
	}

	
	for (i = 0; i<STRUCT_LEN(application_based_qos); i++)
	{
	    int app_prio = 0;
	    char validated_port[12] = "";
	    
	    app_item = application_based_qos[i];
	    app_prio = atoi(nvram_safe_get(app_item.nv_name));
	    
	    //if (!strcmp(nvram_safe_get(app_item.nv_name), "1"))
	    if (app_prio)
	    {
		if (!(port = app_item.port)) {
		    port = nvram_safe_get(app_item.user_port);

		    if (!check_app_port(port, validated_port))
			continue;
		    
		    port = validated_port;
		}

		appitem_savefile(app_item, port, 0, TCP, app_prio);//0:LAN 2 WAN
#if 0		
//#ifdef QDISC_PRIO		
		appitem_savefile(app_item, port, 1, TCP, app_prio);//1:WAN 2 LAN
#endif    		
	    }
	}
	
	if (nvram_match("enable_game", "1")){
		for (i = 0; i<STRUCT_LEN(gamming_app_table); i++){
			int j = 0;
			struct game_port_t *tcp_item = gamming_app_table[i].tcp_port;
			if (tcp_item->single_port){
				for (j = 0; tcp_item->single_port[j]; j++){
					app_savefile(tcp_item->single_port[j], 0, TCP, 3);

				}
			}
			
			if (tcp_item->range_port){
				for (j = 0; tcp_item->range_port[j]; j++){
					app_savefile(tcp_item->range_port[j], 0, TCP, 3);
				}

			}
		}
	
	}
    }	
#endif
    save2file("COMMIT\n");
}

/*
 *  NAT table 
 */
static void nat_table(void)
{
    save2file("*nat\n"
	      ":PREROUTING ACCEPT [0:0]\n"
	      ":POSTROUTING ACCEPT [0:0]\n"
#ifdef GOOGLE_SUPPORT
	      ":google - [0:0]\n"
#endif
	      ":OUTPUT ACCEPT [0:0]\n");
    nat_prerouting();
    nat_postrouting();
    save2file("COMMIT\n");
}

/*
 *  Filter table 
 */
static void filter_table(void)
{
    save2file("*filter\n"
	      ":INPUT ACCEPT [0:0]\n"
	      ":FORWARD ACCEPT [0:0]\n"
	      ":OUTPUT ACCEPT [0:0]\n"
	      ":logaccept - [0:0]\n"
	      ":logdrop - [0:0]\n"
	      ":logreject - [0:0]\n"
#ifdef FLOOD_PROTECT
	      ":limaccept - [0:0]\n"
#endif
#ifdef PORT_TRIGGER_SUPPORT
	      ":trigger_out - [0:0]\n"
#endif
#ifdef DHCP_FILTER_SUPPORT
	      ":dhcpfilter - [0:0]\n"
#endif
#ifdef GOOGLE_SUPPORT
          ":google - [0:0]\n"
#endif
	      ":lan2wan - [0:0]\n");

    /* Does it disable the filter? */
    if (nvram_match("filter","off") || nvram_match("wk_mode", "router")
#ifdef UNNUMBERIP_SUPPORT
    || nvram_match("wan_proto", "unnumberip")
#endif
    ) {
	/* Protect router itself as well as filtered */
	int seq;

	for (seq=1; seq <= NR_RULES; seq++) {
	    save2file(":grp_%d - [0:0]\n", seq);
	    save2file(":advgrp_%d - [0:0]\n", seq);
		
		{
			char buf_private[32];
             		char *data = NULL;

             		memset(buf_private,'\0',32);

			sprintf(buf_private, "filter_rule%d", seq);                             

			data = nvram_safe_get(buf_private);             

			if( strcmp(data , "") == 0)                 
       		{
               		continue;               
       		}
 
      			 if( strncmp(data,"$STAT:0",7)==0)                       
       		{
              		 continue;
       		}
       		save2file(":input_%d - [0:0]\n", seq);

       	}

 
		//save2file(":input_%d - [0:0]\n", seq);
	}
//	save2file(":grp_11 - [0:0]\n");

	filter_input();
	filter_output();

	filter_forward();
#ifdef THROUGHPUT_TEST_SUPPORT
    if(nvram_match("throughput_test","0"))
#endif
	/* Clamp TCP MSS to PMTU of WAN interface */
	save2file("-A FORWARD -p tcp --tcp-flags SYN,RST SYN -m tcpmss --mss %d: -j TCPMSS "
		    "--set-mss %d\n", atoi(nvram_safe_get("wan_run_mtu"))-39, atoi(nvram_safe_get("wan_run_mtu"))-40);
    	

  //    cprintf("fw_suspense==%s\n",fw_suspense);
   	if (*fw_suspense != 0)
	    save2file( "%s", fw_suspense);
	free(fw_suspense);
    }
    else {
	int seq;

	for (seq=1; seq <= NR_RULES; seq++) {
	    save2file(":grp_%d - [0:0]\n", seq);
	    save2file(":advgrp_%d - [0:0]\n", seq);
		{
			char buf_private[32];
             		char *data = NULL;

             		memset(buf_private,'\0',32);

			sprintf(buf_private, "filter_rule%d", seq);                             

			data = nvram_safe_get(buf_private);             

			if( strcmp(data , "") == 0)                 
       		{
               		continue;               
       		}
 
      			 if( strncmp(data,"$STAT:0",7)==0)                       
       		{
              		 continue;
       		}
       		save2file(":input_%d - [0:0]\n", seq);

       	}
		
	}
//	save2file(":grp_11 - [0:0]\n");
	filter_input();
	filter_output();
	filter_forward();
    }

    /* logaccept chain */
#ifdef FLOOD_PROTECT
    save2file("-A logaccept -i %s -m state --state NEW -m limit --limit %d -j LOG "
	    "--log-prefix \"ACCEPT \" --log-tcp-sequence --log-tcp-options --log-ip-options\n"
	    "-A logaccept -i %s -m state --state NEW -m limit --limit %d -j ACCEPT\n"
	    , wanface, FLOOD_RATE, wanface, FLOOD_RATE);
    save2file("-A logaccept -i %s -m state --state NEW -m limit --limit %s -j LOG "
	    "--log-prefix \"FLOOD \" --log-tcp-sequence --log-tcp-options --log-ip-options\n"
	    "-A logaccept -i %s -m state --state NEW -j DROP\n"
	    , wanface, LOG_FLOOD_RATE, wanface);
#endif
    save2file("-A logaccept -m state --state NEW -j LOG --log-prefix \"ACCEPT \" "
	    "--log-tcp-sequence --log-tcp-options --log-ip-options\n"
	    "-A logaccept -j ACCEPT\n");

    /* logdrop chain */
    save2file("-A logdrop -m state --state NEW -j LOG --log-prefix \"DROP \" "
	    "--log-tcp-sequence --log-tcp-options --log-ip-options\n"
	    "-A logdrop -j DROP\n");

    /* logreject chain */
    save2file("-A logreject -j LOG --log-prefix \"WEBDROP \" "
	    "--log-tcp-sequence --log-tcp-options --log-ip-options\n"
	    "-A logreject -p tcp -m tcp -j REJECT --reject-with tcp-reset\n");

#ifdef FLOOD_PROTECT
    /* limaccept chain */
    save2file("-A limaccept -i %s -m state --state NEW -m limit --limit %d -j ACCEPT\n"
    	    "-A limaccept -i %s -m state --state NEW -m limit --limit %s -j LOG "
	    "--log-prefix \"FLOOD \" --log-tcp-sequence --log-tcp-options --log-ip-options\n"
	    "-A limaccept -i %s -m state --state NEW -j DROP\n"
	    "-A limaccept -j ACCEPT\n"
	    , wanface, FLOOD_RATE, wanface, LOG_FLOOD_RATE, wanface);
#endif
    save2file("COMMIT\n");
}

static void create_restore_file(void)
{


#if 0
	{
		int lock_fd;
	
	do
		{
		
		lock_fd = open(IPTABLES_RULE_STAT_LOCK, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL);
		if(lock_fd == -1)
		{
			DEBUG("%s() try locking!!!\n", __FUNCTION__);
			DEBUG("%s\n", strerror(errno));
			usleep(IPTABLES_INTERVAL);
		}
		else
		{
			if(close(lock_fd) == -1)
			{
				DEBUG("%s():LOCK error!\n", __FUNCTION__);
				//goto fail;
				//return ;
			}
		}
	} while(lock_fd == -1);


}
#endif



#ifdef AOL_SUPPORT
    if (nvram_invmatch("aol_block_traffic", "0")){
	char var[256], *next;

	/***************** NAT table ******************/
	save2file("*nat\n"
		  ":PREROUTING ACCEPT [0:0]\n"
		  ":POSTROUTING ACCEPT [0:0]\n"
		  ":OUTPUT ACCEPT [0:0]\n");
	/* In AOL mode, it must be gateway mode */
	save2file("-A POSTROUTING -o %s -j MASQUERADE\n", wanface);
	save2file("COMMIT\n");

	/**************** Filter table ***************/
	save2file("*filter\n"
		  ":INPUT ACCEPT [0:0]\n"
		  ":FORWARD ACCEPT [0:0]\n"
		  ":OUTPUT ACCEPT [0:0]\n"
		  ":logaccept - [0:0]\n"
		  ":logdrop - [0:0]\n"
		  ":aol - [0:0]\n");

	/* INPUT chain */
	save2file("-A INPUT -m state --state INVALID -j DROP\n"
		  "-A INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT\n"
		  "-A INPUT -i lo -m state --state NEW -j ACCEPT\n"
		  "-A INPUT -i %s -m state --state NEW -j %s\n"
		  "-A INPUT -j %s\n", lanface, "ACCEPT", log_drop);

	/* Drop the wrong state, INVALID, packets */
	save2file("-A FORWARD -m state --state INVALID -j DROP\n");

	/* Clamp TCP MSS to PMTU of WAN interface */
	if (nvram_match("wan_proto", "pppoe") || nvram_match("mtu_enable", "1")
#ifdef WAN_AUTO_DETECT_SUPPORT
	|| nvram_match("wan_proto", "auto_pppoe")
#endif
	)
#ifdef THROUGHPUT_TEST_SUPPORT
		if(nvram_match("throughput_test","0"))
#endif
			save2file("-A FORWARD -p tcp -m tcp --tcp-flags SYN,RST SYN -j TCPMSS "
		      		"--clamp-mss-to-pmtu\n");

	/* Accept those established/related connections, otherwise drop it */
	save2file("-A FORWARD -i %s -m state --state NEW -j aol\n"
		  "-A FORWARD -m state --state RELATED,ESTABLISHED -j ACCEPT\n"
		  "-A FORWARD -j %s\n", lanface, log_drop);

	/* AOL chain */
	foreach (var, AOL_TCP_PORT, next) {
	    save2file("-A aol -p tcp -m tcp --dport %s -j %s\n", var, log_accept);
	}
	save2file("-A aol -p udp -m udp --dport %s -j %s\n", AOL_UDP_PORT, log_accept);
	save2file("-A aol -p udp -m udp --dport %d -j %s\n", DNS_PORT, log_accept);
	if (nvram_match("wan_proto", "dhcp")
#ifdef WAN_AUTO_DETECT_SUPPORT
	|| nvram_match("wan_proto", "auto_dhcp")
#endif
	)
	    save2file("-A aol -p udp -m udp --dport %s -j %s\n",
		      AOL_DHCP_UDP, log_accept);
	/* logaccept chain */
	save2file("-A logaccept -m state --state NEW -j LOG --log-prefix \"ACCEPT \" "
		  "--log-tcp-sequence --log-tcp-options --log-ip-options\n"
		  "-A logaccept -j ACCEPT\n");
	/* logdrop chain */
	save2file("-A logdrop -m state --state NEW -j LOG --log-prefix \"DROP \" "
		  "--log-tcp-sequence --log-tcp-options --log-ip-options\n"
		  "-A logdrop -j DROP\n");

	save2file("COMMIT\n");

	return ;
    }
#endif

    mangle_table();
    nat_table();
    filter_table();
#if 0
if(unlink(IPTABLES_RULE_STAT_LOCK) == -1)
	{
		DEBUG("%s(): UNLOCK error!\n", __FUNCTION__);
	}
	DEBUG("%s(): unlocked!!!\n", __FUNCTION__);

#endif
}

int
#ifdef DEVELOPE_ENV
main(void)
#else
start_firewall(void)
#endif
{
    DIR *dir;
    struct dirent *file;
    FILE *fp;
    char name[NAME_MAX];
    struct stat statbuff;
    int log_level = 0 ;

#ifdef HSIAB_SUPPORT
    if (nvram_match("hsiab_mode","1")) {
	eval("hsiab_fw");
	return 0;
    }
#endif

    /* Block obviously spoofed IP addresses */
    DEBUG("start firewall()...........\n");
  //cprintf("start firewall().....ffffffffffffffffffff......\n");
	/*****************************************
	 * add by tanghui @ 2006-06-02
	 * avoid ping status hold bug
	 *****************************************/
	set_ip_forward('0');
	/********** End of tanghui ***************/

    if (!(dir = opendir("/proc/sys/net/ipv4/conf")))
	perror("/proc/sys/net/ipv4/conf");
    while (dir && (file = readdir(dir))) {
	if (strncmp(file->d_name, ".", NAME_MAX) != 0 &&
		strncmp(file->d_name, "..", NAME_MAX) != 0) {
	    sprintf(name, "/proc/sys/net/ipv4/conf/%s/rp_filter", file->d_name);
	    if (!(fp = fopen(name, "r+"))) {
		perror(name);
		break;
	    }
	    fputc('1', fp);
	    fclose(fp);
	}
    }
    closedir(dir);
    /* Determine LOG level */
    DEBUG("start firewall()........1\n");
    log_level = atoi(nvram_safe_get("log_level"));
    sprintf(log_drop,   "%s", (log_level & 1) ? "logdrop" : "DROP");
    sprintf(log_accept, "%s", (log_level & 2) ? "logaccept" : TARG_PASS);
    sprintf(log_reject, "%s", (log_level & 1) ? "logreject" : TARG_RST);

    /* Get NVRAM value into globle variable */
    DEBUG("start firewall()........2\n");
    strncpy(lanface, nvram_safe_get("lan_ifname"), IFNAMSIZ);
    strncpy(wanface, get_wan_face(), IFNAMSIZ);

    if (nvram_match("wan_proto", "pptp"))
	strncpy(wanaddr, nvram_safe_get("pptp_get_ip"), sizeof(wanaddr));
#ifdef L2TP_SUPPORT
    else if (nvram_match("wan_proto", "l2tp"))
        strncpy(wanaddr, nvram_safe_get("l2tp_get_ip"), sizeof(wanaddr));
#endif
    else
	strncpy(wanaddr, nvram_safe_get("wan_ipaddr"), sizeof(wanaddr));

    ip2cclass(nvram_safe_get("lan_ipaddr"), &lan_cclass[0], sizeof(lan_cclass));

    /* Run Webfilter ? */
    webfilter = 0;		/* Reset, clear the late setting */
    if (nvram_match("block_cookie", "1"))
	webfilter |= BLK_COOKIE;
    if (nvram_match("block_java", "1"))
	webfilter |= BLK_JAVA;
    if (nvram_match("block_activex", "1"))
	webfilter |= BLK_ACTIVE;
    if (nvram_match("block_proxy", "1"))
	webfilter |= BLK_PROXY;

    /* Run DMZ forwarding ? */
    if (nvram_match("wk_mode", "gateway") && nvram_match("dmz_enable", "1") && 
	nvram_invmatch("dmz_ipaddr", "")  && nvram_invmatch("dmz_ipaddr", "0")
#ifdef UNNUMBERIP_SUPPORT
    && nvram_invmatch("wan_proto", "unnumberip")
#endif
)
	dmzenable = 1;
    else
	dmzenable = 0;

    /* Remote management */
    if (nvram_match("remote_management", "1") && 
	    nvram_invmatch("http_wanport", "") && nvram_invmatch("http_wanport", "0"))
	remotemanage = 1;
    else
	remotemanage = 0;

#ifdef HTTPS_SUPPORT
    if (nvram_match("remote_mgt_https", "1") && nvram_match("https_enable", "1"))
	web_lanport = HTTPS_PORT;
    else
#endif
	web_lanport = atoi(nvram_safe_get("http_lanport")) ? : HTTP_PORT;

    /* Remove existent file */
    DEBUG("start firewall()........3\n");
    if( stat(IPTABLES_SAVE_FILE, &statbuff) == 0 )
	unlink(IPTABLES_SAVE_FILE);

    /* Create file for iptables-restore */
    DEBUG("start firewall()........4\n");
//	cprintf("iptables -- restore 11  \n");
    create_restore_file();

#ifndef DEVELOPE_ENV
    /* Insert the rules into kernel */
    DEBUG("start firewall()........5\n");
//	cprintf("iptables -- restore  22\n");
    eval("iptables-restore", IPTABLES_SAVE_FILE);
    //unlink(IPTABLES_SAVE_FILE);
#ifdef HW_QOS_SUPPORT
    app_udp_settable();
#endif
#endif
#ifdef DEL_IP_CONNTRACK_ENTRY
        if(need_to_update_ip_conntrack == 1)
        {
                need_to_update_ip_conntrack = 0;
                system("echo 1 > /proc/net/del_ip_conntrack");
        }
#endif
    /* Turn on the DMZ-LED, if enabled.(from service.c) */
    if (dmzenable)
	diag_led(DMZ, START_LED);
    else
	diag_led(DMZ, STOP_LED);

#ifdef AOL_SUPPORT
    if ((fp=fopen("/proc/sys/net/ipv4/ip_conntrack_udp_timeouts", "r+"))) {
	if (nvram_invmatch("aol_block_traffic", "0"))
	    fprintf(fp, "%d %d", 90 * MINUTE, 90 * MINUTE);
	else
	    fprintf(fp, "%d %d", 30, 180);
	fclose(fp);
    } else
	perror("/proc/sys/net/ipv4/ip_conntrack_udp_timeouts");
#endif
#ifdef XBOX_SUPPORT
    if ((fp=fopen("/proc/sys/net/ipv4/ip_conntrack_udp_timeouts", "r+"))) {
	fprintf(fp, "%d %d", 65, 180);
	fclose(fp);
    } else
	perror("/proc/sys/net/ipv4/ip_conntrack_udp_timeouts");
#endif

//	if(nvram_match("wan_proto", "pppoe"))
	/******************************
	 * modify by tanghui @ 2006-04-10
	 * to solve the problem of forwarding before nat take effect.
	 ******************************/
	if(!strcmp("1", nvram_safe_get("ipt_block_lan")))
	{
		eval("iptables", "-D", "FORWARD", "-i", "br0", "-j", "DROP");
		nvram_set("ipt_block_lan", "0");
	}
	/******* End of tanghui *******/

    /* We don't forward packet until those policies are set. */
    DEBUG("start firewall()........6\n");
	set_ip_forward('1');
#if 0
    if ((fp=fopen("/proc/sys/net/ipv4/ip_forward", "r+"))) {
	fputc('1', fp);
	fclose(fp);
    } else
	perror("/proc/sys/net/ipv4/ip_forward");
#endif
    /* Enable multicast_pass_through */
    /*
       DEBUG("start firewall()........7\n");
       if( nvram_match("multicast_pass","1") ){
       if( (fp=fopen("/proc/sys/net/ipv4/multicast_pass_through", "r+")) ){
       fputc('1', fp);
       fclose(fp);
       } else
       perror("/proc/sys/net/ipv4/multicast_pass_through");
       }
       */
    dprintf("done\n");
    return 0;
}

int
stop_firewall(void)
{
    /* Make sure the DMZ-LED is off (from service.c) */
    diag_led(DMZ, STOP_LED);

    dprintf("done\n");
    return 0;
}







/****************** Below is for 'filter' command *******************/


/* 
 * update_bitmap:
 * 
 * Update bitmap file for activative rule when we insert/delete
 * rule. This file is for tracking the status of filter setting.
 *
 * PARAM - mode 0 : delete 
 *              1 : insert
 *
 * RETURN - The rule order.
 *
 * Example:
 *  mode = 1, seq = 7
 *  before = 0,1,1,0,1,0,0,0,1,1,
 *  after  = 0,1,1,0,1,0,1,0,1,1,
 *  return = 3
 */
static int update_bitmap(int mode, int seq, int d)
{
    FILE *fd;
    char buf[100];
    char sep[]=",";
    char *token;

    int k, i = 1, order = 0;
    int array[100];
	char * iptables_rule_stat;

	if(d == 0)
	{
		iptables_rule_stat = IPTABLES_OUT_RULE_STAT;
	}
	else if(d == 1)
	{
		iptables_rule_stat = IPTABLES_IN_RULE_STAT;
	}
	else
	{
		DEBUG("Directory = %d is not correct!!!\n", d);
		return -1;
	}

#if defined(REVERSE_RULE_ORDER)
    seq = (NR_RULES + 1) - seq ;
#endif
    /* Read active-rule bitmap */
	if( (fd=fopen(iptables_rule_stat, "r")) == NULL ){
		DEBUG("Can't open %s\n", iptables_rule_stat);
		return -1;
		//exit(1);
    }
    fgets(buf, sizeof(buf), fd);

    token = strtok(buf, sep);
    while( token != NULL ){
		if((*token != '0') && (*token != '1') && (*token != '8'))
	    break;

	array[i] = atoi(token);

		if((i < seq) && (*token != '8'))
	    order += array[i];
	i++;
	token = strtok(NULL, sep);
    }

    fclose(fd);

    /* Modify setting */
    if( mode == 1 ){	/* add */
		if( array[seq] != 0 )
	    return -1;
	array[seq] = 1;
    }
    else{			/* delete */
		if( array[seq] != 1 )
	    return -1;
	array[seq] = 0;
    }

    /* Write back active-rule bitmap */
	if( (fd=fopen(iptables_rule_stat, "w")) == NULL ){
	
	DEBUG("Can't open %s\n", iptables_rule_stat);
		return -1;
		//exit(1);
    }
    
    for( k=1; k< i; k++)
	fprintf(fd, "%d,", array[k]);

    fclose(fd);

    return order;
}


/*
 *
 * mode 0 : delete
 *      1 : insert
 */
static int
update_filter_new(int mode, int seq)
{
	


{


		
		int log_level;
		log_level = atoi(nvram_safe_get("log_level"));
    		sprintf(log_drop,   "%s", (log_level & 1) ? "logdrop" : "DROP");
    		sprintf(log_accept, "%s", (log_level & 2) ? "logaccept" : TARG_PASS);
    		sprintf(log_reject, "%s", (log_level & 1) ? "logreject" : TARG_RST);



		//cprintf("\n update_filter mode==%d,seq==%d\n",mode,seq);


		char buf[]="filter_rulexxx";
		char *data;
		int offset,len;
		unsigned int mark;
		char direction[12];
		char direction_2[12];
		char direction_3[50];
		int urlfilter=1;
		sprintf(buf, "filter_in_out%d", seq);
		data = nvram_safe_get(buf);
		if(strcmp(data, "0"))
			goto my_error;
		
		sprintf(buf, "filter_rule%d", seq);
		
		sprintf(direction, "grp_%d", seq);
	
		sprintf(direction_2, "advgrp_%d", seq);		
		sprintf(direction_3, "input_%d", seq);	
		data = nvram_safe_get(buf);
		if( strcmp(data , "") == 0)
		    goto my_error;

	/* Check if it is enabled */
		find_pattern(data, strlen(data), "$STAT:", 
			sizeof("$STAT:")-1, '$', &offset, &len);

		if (len < 1) 
		    goto my_error;;	/* error format */

		strncpy(buf, data + offset, len);
		*(buf+len) = 0;
		DEBUG("STAT: %s\n", buf);
		
		switch (atoi(buf)) {
		    case 1:		/* Drop it */
				mark = MARK_DROP;
				break;
		    case 2:		/* URL checking */
				mark = MARK_OFFSET + seq;
				break;
		    default:	/* jump to next iteration */
				goto my_error;
		}
		//cprintf("in filter uodate  type==%d\n",mark);


			eval("iptables", "-F", direction);
    		eval("iptables", "-F", direction_2);
			eval("iptables", "-F", direction_3);
		//cprintf("eval after1\n");
		macgrp_chain_2(seq, mark, mode);
		//cprintf("eval after2\n");

			ipgrp_chain_2(seq, mark, mode);
		//cprintf("eval after3\n");
		portgrp_chain_2(seq, mark, mode);
		
             if(mode==1)
		advgrp_chain_2(seq, mark, mode);//url match 
    



}


my_error:

return 1;

}

int
filter_add_new(int seq)
{

		
	int lock_fd;

	do
	{
		lock_fd = open(IPTABLES_MY_LOCK, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL);
		//cprintf("my lock_fd==%d\n",lock_fd);
		if(lock_fd == -1)
		{
			DEBUG("%s() try locking!!!\n", __FUNCTION__);
			DEBUG("%s\n", strerror(errno));
			//usleep(IPTABLES_INTERVAL);
		}
		
	} while(lock_fd == -1);

		DEBUG("filter_add:\n");
	
	 update_filter_new(1, seq);


	close(lock_fd);
	
		
if(unlink(IPTABLES_MY_LOCK) == -1)
	{
		DEBUG("%s(): UNLOCK error!\n", __FUNCTION__);
	}
 	return 1;
	
}

int
filter_del_new(int seq)
{
    
	int lock_fd;

	do
	{
		lock_fd = open(IPTABLES_MY_LOCK, O_CREAT|O_WRONLY|O_TRUNC|O_EXCL);
		//cprintf(" my lock_fd==%d\n",lock_fd);
		if(lock_fd == -1)
		{
			DEBUG("%s() try locking!!!\n", __FUNCTION__);
			DEBUG("%s\n", strerror(errno));
			//usleep(IPTABLES_INTERVAL);
		}
		
		
	} while(lock_fd == -1);
	

	 update_filter_new(0, seq);

	close(lock_fd);

	if(unlink(IPTABLES_MY_LOCK) == -1)
	{
		DEBUG("%s(): UNLOCK error!\n", __FUNCTION__);
	}

	return 1;
	

	
}

/*
 * PARAM - seq : Seqence number.
 *
 * RETURN - 0 : It's not in time. 
 *          1 : in time and anytime
 *          2 : in time
 */
static int if_tod_intime(int seq)
{
    char todname[]="filter_todxxx";
    char *todvalue;
    int sched = 0, allday = 0;
    int hr_st, hr_end;	/* hour */
    int mi_st, mi_end;	/* minute */
    char wday[128];
	  char wday_2[128];
int intime=0;

    /* Get the NVRAM data */
    sprintf(todname, "filter_tod%d", seq);
    todvalue = nvram_safe_get(todname);

    DEBUG("%s: %s\n", todname, todvalue);
    if (strcmp(todvalue, "") == 0)
	return 0;

    /* Is it anytime or scheduled ? */
    if (strcmp(todvalue, "0:0 23:59 0-0") == 0 ||
        strcmp(todvalue, "0:0 23:59 0-6") == 0) {
	sched = 0;
    }
    else {
	sched = 1;
	if (strcmp(todvalue, "0:0 23:59") == 0)
	{
		allday = 1;
	}
	if (sscanf(todvalue, "%d:%d %d:%d %s", &hr_st, &mi_st,
		    &hr_end, &mi_end, wday) != 5)
	{
			return 0; /* error format */
	}
	else
	{
		int i;
		for(i=0;i<127;i++)
			wday_2[i]=wday[i];
	}
}	
    DEBUG("sched=%d, allday=%d\n", sched, allday);

    /* Anytime */
    if (!sched)
	return 1;

    /* Scheduled */
    if (allday) {	/* 24-hour, but not everyday */
	if (match_wday(wday_2))
	    intime=1;
    }
    else {		/* Nither 24-hour, nor everyday */
	if (match_wday(wday_2) && 
		match_hrmin(hr_st, mi_st, hr_end, mi_end))
	    intime=1;
    }
    DEBUG("intime=%d\n", intime);

    /* Would it be enabled now ? */
    if (intime)
	return 2;

    return 0;
}

static int schedule_by_tod_2(int seq, int d)
{
	char todname[]="filter_todxxx";
	char *todvalue;
	char *temp_rule;
	char *temp_rule2;
	
	
	int sched = 0, allday = 0;
	int hr_st, hr_end;	/* hour */
	int mi_st, mi_end;	/* minute */
	char wday[128];
	char wday_2[128];
	int intime=0;

    /* Get the NVRAM data */
    sprintf(todname, "filter_tod%d", seq);
    todvalue = nvram_safe_get(todname);

	temp_rule = nvram_safe_get( "filter_rule1");
    
	//cprintf("xujinjin_sched_name==%s: value==%s,filter1==%s\n", todname, todvalue,temp_rule);

	
	local_stat[seq]=0;

	

	
	DEBUG("%s: %s\n", todname, todvalue);
    if (strcmp(todvalue, "") == 0)
    {
		//cprintf("todvalue return\n");
		return 0;
    }
    /* Is it anytime or scheduled ? */
    if (strcmp(todvalue, "0:0 23:59 0-0") == 0 ||
        strcmp(todvalue, "0:0 23:59 0-6") == 0) 
      {

		sched = 0;		//24 hours and 7 days
		//cprintf("seq_%d==%d   24h 7day\n",seq,sched);
	}
    else {
	sched = 1;
	//cprintf("enter 0177\n");
	if (strncmp(todvalue, "0:0 23:59",9) == 0)//modufy by xjj
	{

		//	cprintf("allday change to 1\n");
			allday = 1;//24 hours
		//	cprintf("seq_%d==%d   24h not 7day\n",seq,sched);
	}
	if (sscanf(todvalue, "%d:%d %d:%d %s", &hr_st, &mi_st,
		    &hr_end, &mi_end, wday) != 5)
		{
			cprintf("error format\n");
			return 0; /* error format */

		}
		else
		{
			int i;
			for(i=0;i<127;i++)
			wday_2[i]=wday[i];
		}
	}	

    DEBUG("sched=%d, allday=%d\n", sched, allday);
    /* Anytime */

	//cprintf("sched=%d, allday=%d\n", sched, allday);

	if (!sched) {
		
		local_stat[seq]=1;
	
			//cprintf("seq_%d==%d   24h and 7day 2\n",seq,sched);
		return 1;
    	}
	
	
	/* Scheduled */
    if (allday) {	/* 24-hour, but not everyday */
	char wday_st[64], wday_end[64];	/* for crontab */
	int rotate = 0;		/* wday continugoue */
	char sep[]=",";		/* wday seperate character */
	char *token;		
	int st, end;	

	/* If its format looks like as "0-1,3,5-6" */
	if (*wday == '0')
	    if (*(wday + strlen(wday) - 1) == '6')
		rotate = 1;

	/* Parse the 'wday' format for crontab */
	token = strtok(wday, sep);
	//cprintf("1__token===%s\n",token);
	while (token != NULL) {
	    /* which type of 'wday' ? */
		//cprintf("token===%s\n",token);
	if (sscanf(token, "%d-%d", &st, &end) != 2)
		st = end = atoi(token);

	    if (rotate == 1 && st == 0)
		sprintf(wday_end + strlen(wday_end), ",%d", end);
	    else if (rotate == 1 && end == 6)
		sprintf(wday_st  + strlen(wday_st),  ",%d", st );
	    else {
		sprintf(wday_st  + strlen(wday_st),  ",%d", st );
		sprintf(wday_end + strlen(wday_end), ",%d", end);
	    }

	    token = strtok(NULL, sep);
	}

	/* Write to crontab for triggering the event */
	/* "wday_xx + 1" can ignor the first character ',' */

	/************************************/
	if (match_wday(wday_2))
	    intime=1;
    }
    else {		
	if (match_wday(wday_2) && 
		match_hrmin(hr_st, mi_st, hr_end, mi_end))
	    intime=1;
    }

    /* Would it be enabled now ? */
    DEBUG("intime=%d\n", intime);
    if (intime) {
		
	local_stat[seq]=1;
		
				//cprintf("seq_%d==%d   in time\n",seq,sched);
	return 1;
    }


    return 0;
}

int filtersync_main(void) //ntp get current time so we may refresh our firewall rule
{
    time_t ct;      /* Calendar time */
    struct tm *bt;  /* Broken time */
    int seq;
    int ret;
		int lock_fd;


#ifdef AOL_SUPPORT
    if (nvram_invmatch("aol_block_traffic", "0"))
	return 1;
#endif
    /* Get local calendar time */
//	if(nv)

	
	//cprintf("I douht this is the reason ntp get the time it will run here \n");
/*
    for (seq=1; seq <= NR_RULES; seq++) {
	if (if_tod_intime(seq) > 0)
	    ret = filter_add(seq);
	else																		
	    ret = filter_del(seq);
	DEBUG("seq=%d, ret=%d\n", seq, ret);
    }
*/


/*
if(( now_hrmin_last==0)&&(now_wday_last==0)&&(first_use_filtersync_main==0))
{
		first_use_filtersync_main=1;
		now_hrmin_last=now_hrmin;
		now_wday_last=now_wday;
}
else 
	first_use_filtersync_main=0x10;
*/


time(&ct);
    bt=localtime(&ct);
    /* Convert to 3-digital format */
    now_hrmin = bt->tm_hour * 100 + bt->tm_min;
    now_wday = bt->tm_wday;
//cprintf("filter sync  now_wday==%d,now_hrmin==%d\n",now_wday,now_hrmin);
	
for(seq=1;seq<=NR_RULES;seq++)
{
	schedule_by_tod_2(seq, 0);
}
	for(seq=1;seq<=NR_RULES;seq++)
		{
			//cprintf("seq_%d==%d\n",seq,local_stat[seq]);
			if(local_stat[seq]==1)
			{
				filter_add_new(seq);
			}
			if	(local_stat[seq]==0)
			{
				filter_del_new(seq);
			}
		}






	return 1;
}

