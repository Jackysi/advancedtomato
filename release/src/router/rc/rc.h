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

#ifndef __RC_H__
#define __RC_H__

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h> // !!TB
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <net/if.h>

#include <bcmnvram.h>
#include <utils.h>
#include <shutils.h>
#include <shared.h>

#include <tomato_profile.h>
#include <tomato_config.h>

#define USE_MINIUPNPD

//	#define DEBUG_IPTFILE
//	#define DEBUG_RCTEST
//	#define DEBUG_NOISY

#ifdef DEBUG_NOISY
#define _dprintf(args...) cprintf(args)
#define TRACE_PT(args...) do { cprintf("[%d:%s +%ld] ", getpid(), __FUNCTION__, get_uptime()); cprintf(args); } while(0)
#else
#define _dprintf(args...) do { } while(0)
#define TRACE_PT(args...) do { } while(0)
#endif

#define MOUNT_ROOT	"/tmp/mnt"
#define PROC_SCSI_ROOT	"/proc/scsi"
#define USB_STORAGE	"usb-storage"
 
#define BOOT		0
#define REDIAL		1
#define CONNECTING	2

#define PPPOE0		0
#define PPPOE1		1

#define GOT_IP			0x01
#define RELEASE_IP		0x02
#define	GET_IP_ERROR		0x03
#define RELEASE_WAN_CONTROL	0x04
#define USB_DATA_ACCESS		0x05	//For WRTSL54GS
#define USB_CONNECT		0x06	//For WRTSL54GS
#define USB_DISCONNECT		0x07	//For WRTSL54GS

/*
// ?
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
*/

#define SET_LED(val)	do { } while(0)


typedef enum { IPT_TABLE_NAT, IPT_TABLE_FILTER, IPT_TABLE_MANGLE } ipt_table_t;


// init.c
extern void handle_reap(int sig);
extern int init_main(int argc, char *argv[]);
extern int reboothalt_main(int argc, char *argv[]);

// interface.c
extern int ifconfig(const char *ifname, int flags, const char *addr, const char *netmask);
extern int route_add(char *name, int metric, char *dst, char *gateway, char *genmask);
extern int route_del(char *name, int metric, char *dst, char *gateway, char *genmask);
extern void config_loopback(void);
extern int start_vlan(void);
extern int stop_vlan(void);
extern int config_vlan(void);
extern void config_loopback(void);

// listen.c
extern int listen_main(int argc, char **argv);

// ppp.c
extern int ipup_main(int argc, char **argv);
extern int ipdown_main(int argc, char **argv);
extern int pppevent_main(int argc, char **argv);
extern int set_pppoepid_main(int argc, char **argv);	// by tallest 1219
extern int pppoe_down_main(int argc, char **argv);		// by tallest 0407

// rc.c
extern void restore_defaults(void);

// redial.c
extern int start_redial(void);
extern int stop_redial(void);
extern int redial_main(int argc, char **argv);

// wan.c
extern int start_pptp(int mode);
extern int stop_pptp(void);
extern void start_pppoe(int);
extern void stop_pppoe(void);
extern void stop_singe_pppoe(int num);
extern void start_l2tp(void);
extern void stop_l2tp(void);
extern void start_wan(int mode);
extern void start_wan_done(char *ifname);
extern void stop_wan(void);
extern void force_to_dial(void);

// network.c
extern void set_host_domain_name(void);
extern void start_lan(void);
extern void stop_lan(void);
extern void hotplug_net(void);
extern void do_static_routes(int add);
extern int radio_main(int argc, char *argv[]);
extern int wldist_main(int argc, char *argv[]);

// dhcpc.c
extern int dhcpc_event_main(int argc, char **argv);
extern int dhcpc_release_main(int argc, char **argv);
extern int dhcpc_renew_main(int argc, char **argv);
extern void start_dhcpc(void);
extern void stop_dhcpc(void);

// services.c
extern void start_cron(void);
extern void stop_cron(void);
extern void start_zebra(void);
extern void stop_zebra(void);
extern void start_upnp(void);
extern void stop_upnp(void);
extern void start_syslog(void);
extern void stop_syslog(void);
extern void start_igmp_proxy(void);
extern void stop_igmp_proxy(void);
extern void start_httpd(void);
extern void stop_httpd(void);
extern void clear_resolv(void);
extern void dns_to_resolv(void);
extern void start_dnsmasq(void);
extern void stop_dnsmasq(void);
extern void set_tz(void);
extern void start_ntpc(void);
extern void stop_ntpc(void);
extern void check_services(void);
extern void exec_service(void);
extern int service_main(int argc, char *argv[]);
extern void start_service(const char *name);
extern void stop_service(const char *name);
extern void restart_service(const char *name);
extern void start_services(void);
extern void stop_services(void);
// !!TB - USB and NAS
extern int mkdir_if_none(char *dir);
extern void restart_nas_services(int start);

// !!TB - USB Support
// usb.c
extern void start_usb(void);
extern void stop_usb(void);
extern void hotplug_usb(void);
extern void remove_storage_main(void);

// wnas.c
extern void start_nas(void);
extern void stop_nas(void);
extern void notify_nas(const char *ifname);

// firewall.c
extern char wanface[IFNAMSIZ];
extern char lanface[IFNAMSIZ];
extern char wanaddr[];
extern char lan_cclass[];
extern const char *chain_in_accept;
extern const char *chain_out_drop;
extern const char *chain_out_accept;
extern const char *chain_out_reject;
extern char **layer7_in;

extern void enable_ip_forward(void);
extern void ipt_write(const char *format, ...);
extern int ipt_ipp2p(const char *v, char *opt);
extern int ipt_layer7(const char *v, char *opt);
extern void ipt_layer7_inbound(void);
extern int start_firewall(void);
extern int stop_firewall(void);
#ifdef DEBUG_IPTFILE
extern void create_test_iptfile(void);
#endif

// forward.c
extern void ipt_forward(ipt_table_t table);
extern void ipt_triggered(ipt_table_t table);

// restrict.c
extern int rcheck_main(int argc, char *argv[]);
extern void ipt_restrictions(void);
extern void sched_restrictions(void);

// qos.c
extern void ipt_qos(void);
extern void start_qos(void);
extern void stop_qos(void);

// cifs.c
#ifdef TCONFIG_CIFS
extern void start_cifs(void);
extern void stop_cifs(void);
extern int mount_cifs_main(int argc, char *argv[]);
#else
static inline void start_cifs(void) { };
static inline void stop_cifs(void) { };
#endif

// jffs2.c
#ifdef TCONFIG_JFFS2
extern void init_jffs2(void);
extern void start_jffs2(void);
extern void stop_jffs2(void);
#else
static inline void init_jffs2(void) { };
static inline void start_jffs2(void) { };
static inline void stop_jffs2(void) { };
#endif

// ddns.c
#ifdef TCONFIG_DDNS
extern void start_ddns(void);
extern void stop_ddns(void);
extern int ddns_update_main(int argc, char **argv);
#else
static inline void start_ddns(void) { };
static inline void stop_ddns(void) { };
#endif

// misc.c
extern void usage_exit(const char *cmd, const char *help) __attribute__ ((noreturn));
extern int modprobe(const char *mod);
extern int modprobe_r(const char *mod);
#define xstart(args...)	_xstart(args, NULL)
extern int _xstart(const char *cmd, ...);
extern void run_nvscript(const char *nv, const char *arg1, int wtime);
extern void setup_conntrack(void);
extern void set_mac(const char *ifname, const char *nvname, int plus);
extern const char *default_wanif(void);
//	extern const char *default_wlif(void);
#define vstrsep(buf, sep, args...) _vstrsep(buf, sep, args, NULL)
extern int _vstrsep(char *buf, const char *sep, ...);
extern void simple_unlock(const char *name);
extern void simple_lock(const char *name);
extern void killall_tk(const char *name);
long fappend(FILE *out, const char *fname);

// telssh.c
extern void create_passwd(void);
extern void start_sshd(void);
extern void stop_sshd(void);
extern void start_telnetd(void);
extern void stop_telnetd(void);

// mtd.c
extern int mtd_getinfo(const char *mtdname, int *part, int *size);
extern int mtd_erase(const char *mtdname);
extern int mtd_unlock(const char *mtdname);
extern int mtd_write_main(int argc, char *argv[]);
extern int mtd_unlock_erase_main(int argc, char *argv[]);

// buttons.c
extern int buttons_main(int argc, char *argv[]);

// led.c
extern int led_main(int argc, char *argv[]);

// gpio.c
extern int gpio_main(int argc, char *argv[]);

// sched.c
extern int sched_main(int argc, char *argv[]);
extern void start_sched(void);
extern void stop_sched(void);

//nvram
extern int nvram_file2nvram(const char *name, const char *filename);
extern int nvram_nvram2file(const char *name, const char *filename);

#ifdef TOMATO_SL
// usb.c
extern void hotplug_usb(void);
extern int usbevent_main(int argc, char *argv[]);
extern void start_usbevent(void);
extern void stop_usbevent(void);
extern int usbrescan_main(int argc, char *argv[]);
extern int hotdiskadd_main(int argc, char *argv[]);
extern int hotdiskremove_main(int argc, char *argv[]);
extern int hotdiskerror_main(int argc, char *argv[]);
extern int umountx_main(int argc, char *argv[]);

void start_test_1(void);
void stop_test_1(void);

// samba.c
extern void start_smbd(void);
extern void stop_smbd(void);
#endif


#endif
