
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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <signal.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <utils.h>
#include <cy_conf.h>
#include <rc.h>

#define QDISC_FILE	"/tmp/qdisc.sh"
#define WANRATE_TMP_FILE	"/tmp/voip"
//#define MAX_BANDWIDTH	102400
#define MAX_BANDWIDTH	15359	// S = (1500-64)*8/1024 (kbits)
				// S *1000 / (thelta t) * 1.369 (weight) = B
#define FIRST_PING_SIZE	22 //64-14(frame header)-20(IP header)-8(ICMP header)
#define SECOND_PING_SIZE	1458 //1500-14-20-8
#define PRIOMAP	"2 2 1 2 1 2 0 0 1 1 1 1 1 1 1 1"

#define qdisc_file(fmt, args...) do { \
	FILE *fp = fopen(QDISC_FILE, "a+"); \
        if (fp) { \
                fprintf(fp, fmt, ## args); \
                fclose(fp); \
        } \
	else \
		fprintf(stderr, fmt, ## args); \
} while (0)

static char wan_ifname[IFNAMSIZ] = "eth0";
#ifdef QDISC_PRIO 
static char lan_ifname[IFNAMSIZ] = "br0";
#endif

enum { WAN_RATE_SUCCESS, WAN_LINK_FAIL };

struct icmp_map_t{
	unsigned difference;
	unsigned rate;
} icmp_map[] = {
	{180, 64},
	{150, 96},
	{120, 128},
	{90, 192},
	{60, 256},
	{51, 320},
	{43, 384},
	{36, 448},
	{30, 512},
	{25, 576},
	{23, 640},
	{21, 708},
	{20, 800},
	//{4, 10240},
	//{0, MAX_BANDWIDTH},
};

#ifdef QOS_PHASE_II
#define EF_PRIO 0
#define AF1_PRIO 1 
#define AF2_PRIO 2
#define AF3_PRIO 3
#define AF4_PRIO 4
#define BE_PRIO 5

unsigned af1_rate, af2_rate, af3_rate, af4_rate, be_rate;

int init_rate(unsigned qos_rate)
{
	af1_rate = qos_rate * 0.8 ;
	af2_rate = qos_rate * 0.1;
	af3_rate = qos_rate * 0.05;
	af4_rate = qos_rate * 0.04;
	be_rate = qos_rate - af1_rate - af2_rate - af3_rate - af4_rate;
	return 0;
}
#endif

unsigned detect_qos_rate(unsigned ping64, unsigned ping1500)
{
	int i;
	int icmp_delay = (ping1500-ping64)/10;

	if ((ping1500 <= ping64) || (icmp_delay == 0))
		return MAX_BANDWIDTH;
	
	for (i = 0; i < STRUCT_LEN(icmp_map); i++)
		if (icmp_delay >= icmp_map[i].difference)
			return icmp_map[i].rate;
	
	return (unsigned)(MAX_BANDWIDTH/icmp_delay);
	//return 0;
}
		
void htb_prio_fifo_qdisc(unsigned qos_rate)
{
	//qdisc_file("#! /bin/sh\n");
#ifndef QOS_PHASE_II
#ifndef QDISC_PRIO 
    	unsigned low_rate=qos_rate*0.1;
    	unsigned high_rate=qos_rate*0.9;
	
	//strncpy(wan_ifname, get_wan_face(), IFNAMSIZ);
    	strncpy(wan_ifname, nvram_safe_get("wan_iface"), IFNAMSIZ);
	
	qdisc_file("tc qdisc add dev %s root handle 1: htb default 200\n", wan_ifname);
	qdisc_file("tc class add dev %s parent 1:0 classid 1:1 htb rate %dkbit\n", wan_ifname ,qos_rate);
	qdisc_file("tc class add dev %s parent 1:1 classid 1:100 htb rate %dkbit ceil %dkbit\n", wan_ifname, high_rate, qos_rate);
	qdisc_file("tc class add dev %s parent 1:1 classid 1:200 htb rate %dkbit ceil %dkbit\n", wan_ifname, low_rate, qos_rate);
	qdisc_file("tc qdisc add dev %s parent 1:100 pfifo limit 100\n", wan_ifname);
	qdisc_file("tc qdisc add dev %s parent 1:200 pfifo limit 100\n", wan_ifname);
	qdisc_file("tc filter add dev %s parent 1:0 prio 1 protocol ip u32 \\\n",wan_ifname);
	qdisc_file("match ip tos 0x10 0xff flowid 1:100\n");
#else
	//strncpy(wan_ifname, get_wan_face(), IFNAMSIZ);
    	strncpy(wan_ifname, nvram_safe_get("wan_iface"), IFNAMSIZ);
    	strncpy(lan_ifname, nvram_safe_get("lan_ifname"), IFNAMSIZ);
	
	qdisc_file("tc qdisc add dev %s root handle 1: htb\n", wan_ifname);
	qdisc_file("tc class add dev %s parent 1:0 classid 1:1 htb rate %dkbit\n", wan_ifname ,qos_rate);
	qdisc_file("tc qdisc add dev %s parent 1:1 handle 100: prio priomap %s\n", wan_ifname, PRIOMAP);
	qdisc_file("tc qdisc add dev %s parent 100:1 handle 200: pfifo limit 100\n", wan_ifname);
	qdisc_file("tc qdisc add dev %s parent 100:2 handle 300: pfifo limit 100\n", wan_ifname);
	qdisc_file("tc qdisc add dev %s parent 100:3 handle 400: pfifo limit 100\n", wan_ifname);
	qdisc_file("tc filter add dev %s parent 1:0 prio 1 protocol ip u32 match ip protocol 0x0 0x00 flowid 1:1\n", wan_ifname);

	qdisc_file("tc qdisc add dev %s root handle 1: prio priomap %s\n", lan_ifname, PRIOMAP);
	qdisc_file("tc qdisc add dev %s parent 1:1 handle 100: pfifo limit 100\n", lan_ifname);
	qdisc_file("tc qdisc add dev %s parent 1:2 handle 200: pfifo limit 100\n", lan_ifname);
	qdisc_file("tc qdisc add dev %s parent 1:3 handle 300: pfifo limit 100\n", lan_ifname);
#endif
#else
	init_rate(qos_rate);
	
	strncpy(wan_ifname, nvram_safe_get("wan_iface"), IFNAMSIZ);
    	strncpy(lan_ifname, nvram_safe_get("lan_ifname"), IFNAMSIZ);
	
	qdisc_file("tc qdisc add dev %s root handle 1: htb\n", wan_ifname);
	qdisc_file("tc class add dev %s parent 1:0 classid 1:1 htb rate %dkbit\n", wan_ifname, qos_rate);
	qdisc_file("tc qdisc add dev %s parent 1:1 handle 10: prio\n", wan_ifname);
	/* EF */
	qdisc_file("tc qdisc add dev %s parent 10:1 handle 100: pfifo limit 100\n", wan_ifname);
	//qdisc_file("tc qdisc add dev %s parent 10:1 handle 110:0 dsmark indices 8\n", wan_ifname);
	//qdisc_file("tc class change dev %s classid 110:1 dsmark mask 0x3 value 0xb8\n", wan_ifname);
	
	qdisc_file("tc qdisc add dev %s parent 10:2 handle 20:0 htb default 50\n", wan_ifname);
	qdisc_file("tc class add dev %s parent 20:0 classid 20:1 htb rate %dkbit ceil %dkbit\n", wan_ifname, qos_rate, qos_rate);
	/* AF1 */
	qdisc_file("tc class add dev %s parent 20:1 classid 20:10 htb rate %dkbit ceil %dkbit prio %d burst 6k\n", wan_ifname, af1_rate, qos_rate, AF1_PRIO);
	qdisc_file("tc qdisc add dev %s parent 20:10 handle 210: pfifo limit 100\n", wan_ifname);
	//qdisc_file("tc qdisc add dev %s parent 20:10 handle 210:0 dsmark indices 8\n", wan_ifname);
	//qdisc_file("tc class change dev %s classid 210:1 dsmark mask 0x3 value 0x20\n", wan_ifname);
	/* AF2 */
	qdisc_file("tc class add dev %s parent 20:1 classid 20:20 htb rate %dkbit ceil %dkbit prio %d burst 6k\n", wan_ifname, af2_rate, qos_rate, AF2_PRIO);
	qdisc_file("tc qdisc add dev %s parent 20:20 handle 220: pfifo limit 100\n", wan_ifname);
	//qdisc_file("tc qdisc add dev %s parent 20:20 handle 220:0 dsmark indices 8\n", wan_ifname);
	//qdisc_file("tc class change dev %s classid 220:1 dsmark mask 0x3 value 0x40\n", wan_ifname);
	/* AF3 */
	qdisc_file("tc class add dev %s parent 20:1 classid 20:30 htb rate %dkbit ceil %dkbit prio %d burst 6k\n", wan_ifname, af3_rate, qos_rate, AF3_PRIO);
	qdisc_file("tc qdisc add dev %s parent 20:30 handle 230: pfifo limit 100\n", wan_ifname);
	//qdisc_file("tc qdisc add dev %s parent 20:30 handle 230:0 dsmark indices 8\n", wan_ifname);
	//qdisc_file("tc class change dev %s classid 230:1 dsmark mask 0x3 value 0x60\n", wan_ifname);
	/* AF4 */
	qdisc_file("tc class add dev %s parent 20:1 classid 20:40 htb rate %dkbit ceil %dkbit prio %d burst 6k\n", wan_ifname, af4_rate, qos_rate, AF4_PRIO);
	qdisc_file("tc qdisc add dev %s parent 20:40 handle 240: pfifo limit 100\n", wan_ifname);
	//qdisc_file("tc qdisc add dev %s parent 20:40 handle 240:0 dsmark indices 8\n", wan_ifname);
	//qdisc_file("tc class change dev %s classid 240:1 dsmark mask 0x3 value 0x80\n", wan_ifname);
	/* BE */
	qdisc_file("tc class add dev %s parent 20:1 classid 20:50 htb rate %dkbit ceil %dkbit prio %d burst 6k\n", wan_ifname, be_rate, qos_rate, BE_PRIO);
	qdisc_file("tc qdisc add dev %s parent 20:50 handle 250: pfifo limit 100\n", wan_ifname);
	/* Filter */
	qdisc_file("tc filter add dev %s parent 1:0 prio 1 protocol ip u32 match ip protocol 0x0 0x00 flowid 1:1\n", wan_ifname);

	/* put the traffice with DSCP into appropriate queues*/

	/* EF  */
	qdisc_file("tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0xa0 0xe0 flowid 10:1\n", wan_ifname, EF_PRIO);
	/* AF Class 1 */
	qdisc_file("tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x20 0xe0 flowid 10:2\n",wan_ifname, AF1_PRIO);
	qdisc_file("tc filter add dev %s parent 20:0 protocol ip prio %d u32 match ip tos 0x20 0xe0 flowid 20:10\n",wan_ifname, AF1_PRIO);
	/* AF Class 2 */
	qdisc_file("tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x40 0xe0 flowid 10:2\n", wan_ifname, AF2_PRIO);
	qdisc_file("tc filter add dev %s parent 20:0 protocol ip prio %d u32 match ip tos 0x40 0xe0 flowid 20:20\n", wan_ifname, AF2_PRIO);
	/* AF Class 3 */
	qdisc_file("tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x60 0xe0 flowid 10:2\n", wan_ifname, AF3_PRIO);
	qdisc_file("tc filter add dev %s parent 20:0 protocol ip prio %d u32 match ip tos 0x60 0xe0 flowid 20:30\n", wan_ifname, AF3_PRIO);
	/* AF Class 4 */
	qdisc_file("tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x80 0xe0 flowid 10:2\n", wan_ifname, AF4_PRIO);
	qdisc_file("tc filter add dev %s parent 20:0 protocol ip prio %d u32 match ip tos 0x80 0xe0 flowid 20:40\n", wan_ifname, AF4_PRIO);
	/* BE Class  */
	qdisc_file("tc filter add dev %s parent 10:0 protocol ip prio %d u32 match ip tos 0x0 0xe0 flowid 10:2\n", wan_ifname, BE_PRIO);
	qdisc_file("tc filter add dev %s parent 20:0 protocol ip prio %d u32 match ip tos 0x0 0xe0 flowid 20:50\n", wan_ifname, BE_PRIO);
#endif
}
	
unsigned qos_get_wan_rate(void)
{
	int count=0; 
	unsigned ping64=0, ping1500=0, qos_rate=0;
	FILE *fp;
	char line[254];

	if ((fp = fopen(WANRATE_TMP_FILE, "r")) != NULL) {
		if( fgets(line, sizeof(line), fp) != NULL ) {
			count = sscanf(line, "%u %u", &ping64, &ping1500);
		}
		printf("\nping64=%u ping1500=%u count=%d\n", ping64, ping1500,count);
		fclose(fp);
	
		if (count == 0)
			goto RET_WAN_RATE;
		else if ( (count != 2)  || (ping64 == 0) || (ping1500 == 0))
			qos_rate = MAX_BANDWIDTH;
		else
			qos_rate = detect_qos_rate(ping64, ping1500);
	}

RET_WAN_RATE:
	printf("\nqos_rate= %u\n",qos_rate);
	return qos_rate;
}

int set_ratefile_byicmp(void)
{
	//char *ip = nvram_safe_get("wan_gateway");
	char *ip = nvram_safe_get("wan_get_dns");
	
	if((strchr(ip, ' ')) || (!strcmp(ip, "")))
	{
		struct dns_lists *dns_list = get_dns_list(0);
		int i;
		
		for(i=0 ; i<dns_list->num_servers ; i++){
			ip = dns_list->dns_server[i];
			if( (!strchr(ip, ' ')) && (strcmp(ip, "")))
				break;
		}
	}
	
	if((!check_wan_link(0)) || (strchr(ip, ' ')) || (!strcmp(ip, "")))
		return WAN_LINK_FAIL;
	else {
		//char *argv1[] = {"ping", "-c", "1", "-s", "56", "-v", WANRATE_TMP_FILE, ip, NULL};
		//char *argv2[] = {"ping", "-c", "1", "-s", "1492", "-v", WANRATE_TMP_FILE, ip, NULL};
		char cmd[80];
		snprintf(cmd, sizeof(cmd), "ping -c 1 -s %d -v %s %s", FIRST_PING_SIZE, WANRATE_TMP_FILE, ip);
		eval("killall", "ping");
		unlink(WANRATE_TMP_FILE);
		//_eval(argv1, NULL, 2, NULL);
	        sleep(1);
		system(cmd);    
		snprintf(cmd, sizeof(cmd), "ping -c 1 -s %d -v %s %s", SECOND_PING_SIZE, WANRATE_TMP_FILE, ip);
		eval("killall", "ping");
		//_eval(argv2, NULL, 2, NULL);
	        system(cmd);    
	}
	return WAN_RATE_SUCCESS;
}

static char *port_option_name[] = {
 	"port_priority_1",
	"port_flow_control_1",
	//{ "port_frame_type_1",
	"port_rate_limit_1",
	"port_priority_2",
	"port_flow_control_2",
	//{ "port_frame_type_2",
	"port_rate_limit_2",
	"port_priority_3",
	"port_flow_control_3",
	//{ "port_frame_type_3",
	"port_rate_limit_3",
	"port_priority_4",
	//{ "port_priority_4", PORT_CONFIG_4, PRIORITY_MASK, port_priority_content},
	"port_flow_control_4",
	//{ "port_frame_type_4",
	"port_rate_limit_4",
	"wan_speed",
	"QoS",
	NULL
};
	
void
qos_init(void)
{
	unsigned short i;
	char *value = NULL;
	
	for (i = 0; port_option_name[i]; i++)
	{
		printf("\n%s(%d) i=%d name=%s\n", __FUNCTION__, __LINE__, i, port_option_name[i]);
		if((value = nvram_get(port_option_name[i])))
		{
			printf("\n%s(%d) i=%d value=%s\n", __FUNCTION__, __LINE__, i, value);
			set_register_value(i, atoi(value));
		}
	}
	return;
}
	
int
do_qos(void)
{
	int wan_rate_status = WAN_RATE_SUCCESS;

	if (nvram_match("QoS","1"))
	{
		unsigned qos_rate = 0;
		
		if (nvram_match("rate_mode","0"))
		{	
			char *manual_rate = nvram_safe_get("manual_rate");
			if (strcmp(manual_rate, ""))
				qos_rate = atoi(manual_rate);
		}
		else
		{
			sleep(2);
			wan_rate_status += set_ratefile_byicmp();
			if (!wan_rate_status)
				qos_rate = (unsigned)(qos_get_wan_rate() * 0.8);
		}
		
		if (qos_rate)
		{
			htb_prio_fifo_qdisc(qos_rate);
			eval("sh", QDISC_FILE);
			//unlink(QDISC_FILE);
			//unlink(WANRATE_TMP_FILE);
		}
	}

	return wan_rate_status;
}

int start_voip_qos(void)
{
#ifndef DELAY_PING
	return do_qos();
#else
	int ret = 0;
	pid_t pid;
	char *qos_argv[] = {"qos", NULL};
	ret = _eval(qos_argv, NULL, 0, &pid);
	
	dprintf("done\n");
	return ret;
#endif
}

int 
stop_voip_qos(void)
{
	int ret = 0;
	
	//strncpy(wan_ifname, get_wan_face(), IFNAMSIZ);
    	strncpy(wan_ifname, nvram_safe_get("wan_iface"), IFNAMSIZ);

	unlink(QDISC_FILE);
	eval("tc", "qdisc", "del", "dev", wan_ifname, "root");

#ifdef QDISC_PRIO 
    	strncpy(lan_ifname, nvram_safe_get("lan_ifname"), IFNAMSIZ);
	eval("tc", "qdisc", "del", "dev", lan_ifname, "root");
#endif

	dprintf("done\n");
	return ret;
}

#ifdef DELAY_PING
int
qos_main(int argc, char *argv[])
{
	return do_qos();
}
#endif
