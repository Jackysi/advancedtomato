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

//	#define DEBUG_IPTFILE
//	#define DEBUG_RCTEST
//	#define DEBUG_NOISY

#ifdef DEBUG_NOISY
#define TRACE_PT(args...) do { _dprintf("[%d:%s +%ld] ", getpid(), __FUNCTION__, get_uptime()); _dprintf(args); } while(0)
#else
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

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
#define sin6_addr(s) (((struct sockaddr_in6 *)(s))->sin6_addr)

#define IPT_V4	0x01
#define IPT_V6	0x02
#define IPT_ANY_AF	(IPT_V4 | IPT_V6)
#define IPT_AF_IS_EMPTY(f)	((f & IPT_ANY_AF) == 0)

// init.c
extern int init_main(int argc, char *argv[]);
extern int reboothalt_main(int argc, char *argv[]);
extern int console_main(int argc, char *argv[]);

// interface.c
extern int _ifconfig(const char *name, int flags, const char *addr, const char *netmask, const char *dstaddr);
#define ifconfig(name, flags, addr, netmask) _ifconfig(name, flags, addr, netmask, NULL)
extern int route_add(char *name, int metric, char *dst, char *gateway, char *genmask);
extern void route_del(char *name, int metric, char *dst, char *gateway, char *genmask);
extern void config_loopback(void);
extern int start_vlan(void);
extern int stop_vlan(void);
extern int config_vlan(void);
extern void config_loopback(void);
#ifdef TCONFIG_IPV6
extern int ipv6_mapaddr4(struct in6_addr *addr6, int ip6len, struct in_addr *addr4, int ip4mask);
#endif

// listen.c
extern int listen_main(int argc, char **argv);

// ppp.c
extern int ipup_main(int argc, char **argv);
extern int ipdown_main(int argc, char **argv);
extern int pppevent_main(int argc, char **argv);
#ifdef TCONFIG_IPV6
extern int ip6up_main(int argc, char **argv);
extern int ip6down_main(int argc, char **argv);
#endif

// rc.c
extern void restore_defaults(void);

// redial.c
extern int start_redial(void);
extern int stop_redial(void);
extern int redial_main(int argc, char **argv);

// wan.c
extern void start_pptp(int mode);
extern void stop_pptp(void);
extern void start_pppoe(int);
extern void stop_pppoe(void);
extern void start_l2tp(void);
extern void stop_l2tp(void);
extern void start_wan(int mode);
extern void start_wan_done(char *ifname);
extern char *wan_gateway(void);
#ifdef TCONFIG_IPV6
extern void start_wan6_done(const char *wan_ifname);
#endif
extern void stop_wan(void);
extern void force_to_dial(void);
extern void do_wan_routes(char *ifname, int metric, int add);
extern void preset_wan(char *ifname, char *gw, char *netmask);

// network.c
extern void set_host_domain_name(void);
extern void set_et_qos_mode(int sfd);
extern void start_lan(void);
extern void stop_lan(void);
extern void hotplug_net(void);
extern void do_static_routes(int add);
extern int radio_main(int argc, char *argv[]);
extern int wldist_main(int argc, char *argv[]);
extern void stop_wireless(void);
extern void start_wireless(void);
extern void start_wl(void);
extern void unload_wl(void);
extern void load_wl(void);
#ifdef TCONFIG_IPV6
extern void enable_ipv6(int enable);
extern void accept_ra(const char *ifname);
#else
#define enable_ipv6(enable) do {} while (0)
#define accept_ra(ifname) do {} while (0)
#endif

// dhcpc.c
extern int dhcpc_event_main(int argc, char **argv);
extern int dhcpc_release_main(int argc, char **argv);
extern int dhcpc_renew_main(int argc, char **argv);
extern void start_dhcpc(void);
extern void stop_dhcpc(void);
#ifdef TCONFIG_IPV6
extern int dhcp6c_state_main(int argc, char **argv);
extern void start_dhcp6c(void);
extern void stop_dhcp6c(void);
#endif

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
extern void start_udpxy(void);
extern void stop_udpxy(void);
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
#ifdef TCONFIG_USB
extern void restart_nas_services(int stop, int start);
#else
#define restart_nas_services(args...) do { } while(0)
#endif
#ifdef LINUX26
extern void start_hotplug2();
extern void stop_hotplug2(void);
#endif
#ifdef TCONFIG_IPV6
extern void start_ipv6_tunnel(void);
extern void stop_ipv6_tunnel(void);
extern void start_6rd_tunnel(void);
extern void stop_6rd_tunnel(void);
extern void start_radvd(void);
extern void stop_radvd(void);
extern void start_ipv6(void);
extern void stop_ipv6(void);
#endif

// !!TB - USB Support
// usb.c
#ifdef TCONFIG_USB
extern void start_usb(void);
extern void stop_usb(void);
extern int dir_is_mountpoint(const char *root, const char *dir);
extern void hotplug_usb(void);
extern void remove_storage_main(int shutdn);
#else
#define start_usb(args...) do { } while(0)
#define stop_usb(args...) do { } while(0)
#define dir_is_mountpoint(args...) (0)
#define hotplug_usb(args...) do { } while(0)
#define remove_storage_main(args...) do { } while(0)
#endif

// wnas.c
extern int wds_enable(void);
extern int wl_security_on(void);
extern void start_nas(void);
extern void stop_nas(void);
extern void notify_nas(const char *ifname);

// firewall.c
extern wanface_list_t wanfaces;
extern char lanface[];
#ifdef TCONFIG_IPV6
extern char wan6face[];
#endif
extern char lan_cclass[];
extern const char *chain_in_accept;
extern const char *chain_out_drop;
extern const char *chain_out_accept;
extern const char *chain_out_reject;
extern char **layer7_in;

extern void enable_ip_forward(void);
extern void enable_ip6_forward(void);
extern void ipt_write(const char *format, ...);
extern void ip6t_write(const char *format, ...);
#if defined(TCONFIG_IPV6) && defined(LINUX26)
#define ip46t_write(args...) do { ipt_write(args); ip6t_write(args); } while(0)
//#define ip46t_flagged_write(do_ip4t, do_ip6t, args...) do { if (do_ip4t) ipt_write(args); if (do_ip6t) ip6t_write(args); } while(0)
#define ip46t_flagged_write(do_ip46t, args...) do { if (do_ip46t & IPT_V4) ipt_write(args); if (do_ip46t & IPT_V6) ip6t_write(args); } while(0)
#define ip46t_cond_write(do_ip6t, args...) do { if (do_ip6t) ip6t_write(args); else ipt_write(args); } while(0)
#else
#define ip46t_write ipt_write
//#define ip46t_flagged_write(do_ip4t, do_ip6t, args...) do { if (do_ip4t) ipt_write(args); } while(0)
#define ip46t_flagged_write(do_ip46t, args...) do { if (do_ip46t & IPT_V4) ipt_write(args); } while(0)
#define ip46t_cond_write(do_ip6t, args...) ipt_write(args)
#endif
extern void ipt_log_unresolved(const char *addr, const char *addrtype, const char *categ, const char *name);
extern int ipt_addr(char *addr, int maxlen, const char *s, const char *dir, int af, int strict, const char *categ, const char *name);
extern int ipt_dscp(const char *v, char *opt);
extern int ipt_ipp2p(const char *v, char *opt);
extern int ipt_layer7(const char *v, char *opt);
extern void ipt_layer7_inbound(void);
extern int start_firewall(void);
extern int stop_firewall(void);
#ifdef DEBUG_IPTFILE
extern void create_test_iptfile(void);
#endif
#ifdef LINUX26
extern void allow_fastnat(const char *service, int allow);
extern void try_enabling_fastnat(void);
#endif

// forward.c
extern void ipt_forward(ipt_table_t table);
extern void ipt_triggered(ipt_table_t table);

#ifdef TCONFIG_IPV6
extern void ip6t_forward(void);
#endif

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
#define modprobe(mod, args...) ({ char *argv[] = { "modprobe", "-s", mod, ## args, NULL }; _eval(argv, NULL, 0, NULL); })
extern int modprobe_r(const char *mod);
#define xstart(args...)	_xstart(args, NULL)
extern int _xstart(const char *cmd, ...);
extern void run_nvscript(const char *nv, const char *arg1, int wtime);
extern void run_userfile (char *folder, char *extension, const char *arg1, int wtime);
extern void setup_conntrack(void);
extern int host_addr_info(const char *name, int af, struct sockaddr_storage *buf);
extern int host_addrtypes(const char *name, int af);
extern void inc_mac(char *mac, int plus);
extern void set_mac(const char *ifname, const char *nvname, int plus);
extern const char *default_wanif(void);
//	extern const char *default_wlif(void);
#define vstrsep(buf, sep, args...) _vstrsep(buf, sep, args, NULL)
extern int _vstrsep(char *buf, const char *sep, ...);
extern void simple_unlock(const char *name);
extern void simple_lock(const char *name);
extern void killall_tk(const char *name);
extern int mkdir_if_none(const char *path);
extern long fappend(FILE *out, const char *fname);
extern long fappend_file(const char *path, const char *fname);

// telssh.c
extern void create_passwd(void);
extern void start_sshd(void);
extern void stop_sshd(void);
extern void start_telnetd(void);
extern void stop_telnetd(void);

// mtd.c
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

#ifdef TCONFIG_PPTPD
// pptp_client.c
extern void start_pptp_client(void);
extern void stop_pptp_client(void);
extern int write_pptpvpn_resolv(FILE*);
extern void clear_pptp_route(void);
#else
#define write_pptpvpn_resolv(f) (0)
#endif

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

// transmission.c
#ifdef TCONFIG_BT
extern void start_bittorrent();
extern void stop_bittorrent();
#endif

// nfs.c
#ifdef TCONFIG_NFS
extern void start_nfs();
extern void stop_nfs();
#endif

// snmp.c
#ifdef TCONFIG_SNMP
extern void start_snmp();
extern void stop_snmp();
#endif

//tor.c
#ifdef TCONFIG_TOR
extern void start_tor();
extern void stop_tor();
#endif

// apcupsd.c
#ifdef TCONFIG_UPS
extern void start_ups();
extern void stop_ups();
#endif

// pptp.c
#ifdef TCONFIG_PPTPD
extern void start_pptpd(void);
extern void stop_pptpd(void);
extern void write_pptpd_dnsmasq_config(FILE* f);
#endif

// vpn.c
#ifdef TCONFIG_OPENVPN
extern void start_vpnclient(int clientNum);
extern void stop_vpnclient(int clientNum);
extern void start_vpnserver(int serverNum);
extern void stop_vpnserver(int serverNum);
extern void start_vpn_eas();
extern void stop_vpn_eas();
extern void run_vpn_firewall_scripts();
extern void write_vpn_dnsmasq_config(FILE*);
extern int write_vpn_resolv(FILE*);
#else
/*
static inline void start_vpnclient(int clientNum) {}
static inline void stop_vpnclient(int clientNum) {}
static inline void start_vpnserver(int serverNum) {}
static inline void stop_vpnserver(int serverNum) {}
static inline void run_vpn_firewall_scripts() {}
static inline void write_vpn_dnsmasq_config(FILE*) {}
*/
static inline void start_vpn_eas() { }
static inline void stop_vpn_eas() { }
#define write_vpn_resolv(f) (0)
#endif

// new_qoslimit.c
extern void ipt_qoslimit(int chain);
extern void new_qoslimit_start(void);
extern void new_qoslimit_stop(void);

// arpbind.c
extern void start_arpbind(void);
extern void stop_arpbind(void);

// mmc.c
#ifdef TCONFIG_SDHC
extern void start_mmc(void);
extern void stop_mmc(void);
#endif

#ifdef TCONFIG_NOCAT
// nocat.c 
extern void start_nocat(); 
extern void stop_nocat(); 
extern void reset_nocat(); 
#endif

// tomatoanon.c
extern void start_tomatoanon(); 
extern void stop_tomatoanon(); 

#endif


