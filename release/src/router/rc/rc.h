/*
 * Router rc control script
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: rc.h,v 1.39 2005/03/29 02:00:06 honor Exp $
 */

#ifndef _rc_h_
#define _rc_h_

#include <bcmconfig.h>
#include <netinet/in.h>
#include <fcntl.h>
#ifdef __CONFIG_BUSYBOX__
#include <Config.h>
#endif

//#include <bcmconfig.h>
//#include <code_pattern.h>
#include <cy_conf.h>

/* udhcpc scripts */
extern int udhcpc_wan(int argc, char **argv);
extern int udhcpc_lan(int argc, char **argv);

/* ppp scripts */
extern int ipup_main(int argc, char **argv);
extern int ipdown_main(int argc, char **argv);
extern int set_pppoepid_to_nv_main(int argc, char **argv); // tallest 1219
extern int disconnected_pppoe_main(int argc, char **argv); //by tallest 0407

/* http functions */
extern int http_get(const char *server, char *buf, size_t count, off_t offset);
extern int http_post(const char *server, char *buf, size_t count);
extern int http_stats(const char *url);

/* init */
extern int console_init(void);
extern pid_t run_shell(int timeout, int nowait);
extern void signal_init(void);
extern void fatal_signal(int sig);

/* interface */
extern int ifconfig(char *ifname, int flags, char *addr, char *netmask);
extern int route_add(char *name, int metric, char *dst, char *gateway, char *genmask);
extern int route_del(char *name, int metric, char *dst, char *gateway, char *genmask);
extern void config_loopback(void);
extern int start_vlan(void);
extern int stop_vlan(void);

/* network */
extern void start_lan(void);
extern void stop_lan(void);
extern void start_wan(int status);
extern void start_wan_done(char *ifname);
extern void stop_wan(void);
extern int hotplug_net(void);
/* services */
extern int start_dhcpd(void);
extern int stop_dhcpd(void);
extern int start_dns(void);
extern int stop_dns(void);
extern int start_ntpc(void);
extern int stop_ntpc(void);
extern int start_services(void);
extern int stop_services(void);

extern int config_vlan(void);
extern void config_loopback(void);

extern int start_nas(char *type);

/* firewall */
#ifdef __CONFIG_NETCONF__
extern int start_firewall(void);
extern int stop_firewall(void);
#else
#define start_firewall() do {} while (0)
#define stop_firewall() do {} while (0)
#endif

/* routes */
extern int set_routes(void);

////////////////////////////////////////////////////////////
#define BOOT 0 
#define REDIAL 1
#define CONNECTING	2
#define PPPOE0		0
#define PPPOE1		1

//==================================================
/*
#define GOT_IP			"0x01"
#define RELEASE_IP		"0x02"
#define	GET_IP_ERROR		"0x03"
#define RELEASE_WAN_CONTROL	"0x04"
#define SET_LED(val) \
{ \
	FILE *filep; \
	if (!(filep = fopen("/proc/sys/diag", "w"))) { \
		perror("/proc/sys/diag"); \
	}else \
	{ \
		fprintf(filep, val); \
		fclose(filep); \
	} \
}
*/
#define GOT_IP			0x01
#define RELEASE_IP		0x02
#define	GET_IP_ERROR		0x03
#define RELEASE_WAN_CONTROL	0x04
#define SET_LED(val) \
{ \
	int filep; \
	if(check_hw_type() == BCM4704_BCM5325F_CHIP) { \
		if ((filep = open("/dev/ctmisc", O_RDWR,0))) \
		{ \
			ioctl(filep, val, 0); \
			close(filep); \
		} \
	} \
}
//==================================================

#define QDISC_PRIO 
#define DELAY_PING 
#define QOS_PHASE_II 

extern int start_resetbutton(void);  
extern int stop_resetbutton(void);  

extern int start_tftpd(void);
extern int stop_tftpd(void);

extern int start_cron(void);
extern int stop_cron(void);

extern int start_zebra(void);
extern int stop_zebra(void);

extern int start_redial(void);
extern int stop_redial(void);

extern int start_ddns(void);
extern int stop_ddns(void);

extern int start_upnp(void);
extern int stop_upnp(void);

extern int start_ntp(void);
extern int stop_ntp(void);

extern int start_pptp(int status);
extern int stop_pptp(void);

extern int start_syslog(void);
extern int stop_syslog(void);

extern int start_process_monitor(void);
extern int stop_process_monitor(void);

extern int start_dhcpc(char *);
extern int stop_dhcpc(void);

extern int start_pppoe(int);
extern int stop_pppoe(void);

extern int start_l2tp(int status);
extern int stop_l2tp(void);

extern int start_eou(void);
extern int stop_eou(void);

extern int start_igmp_proxy(void);
extern int stop_igmp_proxy(void);




extern int start_httpd(void);
extern int stop_httpd(void);

extern int filtersync_main(void);
extern int filter_add_new(int seq);
extern int filter_del_new(int seq);
extern int resetbutton_main(int argc, char **argv);
//extern int ntp_main(int argc, char **argv);
extern int ipupdate_main(int argc, char **argv);
extern int gpio_main(int argc, char **argv);
extern int redial_main(int argc, char **argv);
extern int pppevent_main(int argc, char **argv);
extern int udhcpc_main(int argc, char **argv);
extern int eou_status_main(int argc, char **argv);
extern int qos_main(int argc, char **argv);
extern int misc_main(int argc, char **argv);
extern int detectwan_main(int argc, char **argv);
extern int sendudp_main(int argc, char **argv);
extern int check_ses_led_main(int argc, char **argv);

extern int del_routes(char *route);
extern void start_tmp_ppp(int num);

extern int start_single_service(void);

extern int write_boot(const char *path, const char *mtd);

extern int init_mtu(char *wan_proto);
//extern int force_to_dial(void);
extern int force_to_dial(char *whichone);
extern char *range(char *start, char *end);


extern int start_heartbeat(int status);
extern int stop_heartbeat(void);
extern int hb_connect_main(int argc, char **argv);
extern int hb_disconnect_main(int argc, char **argv);
extern int check_ps_main(int argc, char **argv);
extern int listen_main(int argc, char **argv);
extern int ddns_success_main(int argc, char **argv);
extern int process_monitor_main(void);
void cfe_default(void);

extern int nvram_restore(const char *path, char *mtd);

extern int start_voip_qos(void);
extern int stop_voip_qos(void);
extern void qos_init(void);

extern int stop_singe_pppoe(int pppoe_num);

#endif /* _rc_h_ */
