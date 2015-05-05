/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#ifndef __TOMATO_H_
#define __TOMATO_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <typedefs.h>
#include <syslog.h>
#include <signal.h>

#include <bcmutils.h>
#include <bcmnvram.h>
#include <utils.h>
#include <shutils.h>
#include <shared.h>

#include <tomato_profile.h>
#include <tomato_config.h>


#include "httpd.h"


//	#define BLACKHOLE		// for testing


extern int rboot;

extern void exec_service(const char *action);
extern void wi_generic(char *url, int len, char *boundary);
extern void common_redirect(void);
extern char* get_wl_tempsense(char *);

extern const char *resmsg_get(void);
extern void resmsg_set(const char *msg);
extern int resmsg_fread(const char *fname);



// nvram.c
extern void asp_nvram(int argc, char **argv);
extern void asp_nvramseq(int argc, char **argv);
extern void asp_nv(int argc, char **argv);
extern void asp_nvstat(int argc, char **argv);

// misc.c
extern char *js_string(const char *s);
extern char *html_string(const char *s);
extern char *unix_string(const char *s);
extern char *reltime(char *buf, time_t t);
extern int get_client_info(char *mac, char *ifname);
extern int resolve_addr(const char *ip, char *host);

extern void asp_lipp(int argc, char **argv);
extern void asp_activeroutes(int argc, char **argv);
extern void asp_cgi_get(int argc, char **argv);
extern void asp_time(int argc, char **argv);
extern void asp_wanup(int argc, char **argv);
extern void asp_wanstatus(int argc, char **argv);
extern void asp_link_uptime(int argc, char **argv);
extern void asp_rrule(int argc, char **argv);
extern void asp_compmac(int argc, char **argv);
extern void asp_ident(int argc, char **argv);
extern void asp_lanip(int argc, char **argv);
extern void asp_psup(int argc, char **argv);
#ifdef TCONFIG_OPENVPN
extern void wo_vpn_status(char *url);
#endif
extern void asp_sysinfo(int argc, char **argv);
extern void asp_jiffies(int argc, char **argv);
extern void asp_statfs(int argc, char **argv);
extern void asp_notice(int argc, char **argv);
#ifdef TCONFIG_SDHC
extern void asp_mmcid(int argc, char **argv);
#endif
extern void asp_etherstates(int argc, char **argv);
extern void asp_anonupdate(int argc, char **argv);
extern void wo_wakeup(char *url);
extern void asp_dns(int argc, char **argv);
extern void wo_resolve(char *url);

#ifdef TCONFIG_IPV6
extern void asp_calc6rdlocalprefix(int argc, char **argv);
#endif

// usb.c
#ifdef TCONFIG_USB
extern void asp_usbdevices(int argc, char **argv);
extern void wo_usbcommand(char *url);
#endif

//pptpd.c
#ifdef TCONFIG_PPTPD
extern void asp_pptpd_userol(int argc, char **argv);
extern void wo_pptpdcmd(char *url);
#endif

// devlist.c
extern void asp_arplist(int argc, char **argv);
extern void asp_devlist(int argc, char **argv);

// ctnf.c
extern void asp_ctcount(int argc, char **argv);
extern void asp_ctdump(int argc, char **argv);
extern void asp_ctrate(int argc, char **argv);
extern void asp_qrate(int argc, char **argv);
extern void asp_layer7(int argc, char **argv);
extern void wo_expct(char *url);

// wl.c
extern void asp_wlscan(int argc, char **argv);
extern void wo_wlradio(char *url);
extern void asp_wlnoise(int argc, char **argv);
extern void wo_wlmnoise(char *url);
extern void asp_wlstats(int argc, char **argv);
extern void asp_wlclient(int argc, char **argv);
extern void asp_wlchannels(int argc, char **argv);
extern void asp_wlbands(int argc, char **argv);
extern void asp_wlifaces(int argc, char **argv);
extern void asp_wlcountries(int argc, char **argv);

// dhcp.c
extern void asp_dhcpc_time(int argc, char **argv);
extern void wo_dhcpd(char *url);
extern void wo_dhcpc(char *url);

// version.c
extern void asp_build_time(int argc, char **argv);
extern void asp_version(int argc, char **argv);

// traceping.c
extern void wo_trace(char *url);
extern void wo_ping(char *url);

// log.c
extern void wo_viewlog(char *url);
extern void wo_syslog(char *url);
extern void asp_webmon(int argc, char **argv);
extern void wo_webmon(char *url);

// ddns.c
extern void asp_ddnsx(int argc, char **argv);
extern void asp_ddnsx_ip(int argc, char **argv);
extern void asp_ddnsx_msg(int argc, char **argv);

// upgrade.c
extern void prepare_upgrade(void);
extern void wi_upgrade(char *url, int len, char *boundary);
extern void wo_flash(char *url);

// config.c
extern void wo_backup(char *url);
extern void wi_restore(char *url, int len, char *boundary);
extern void wo_restore(char *url);
extern void wo_defaults(char *url);

// parser.c
extern void wo_asp(char *path);

// blackhole.c
extern void wi_blackhole(char *url, int len, char *boundary);

// upnp.c
extern void asp_upnpinfo(int argc, char **argv);
extern void wo_upnp(char *url);

// bwm.c
extern void wo_bwmbackup(char *url);
extern void wi_bwmrestore(char *url, int len, char *boundary);
extern void wo_bwmrestore(char *url);
extern void asp_netdev(int argc, char **argv);
extern void asp_bandwidth(int argc, char **argv);
extern void ctvbuf(FILE *f);

extern void wo_iptbackup(char *url);
extern void wi_iptrestore(char *url, int len, char *boundary);
extern void wo_iptrestore(char *url);

extern void asp_ipt_bandwidth(int argc, char **argv);
extern void asp_iptmon(int argc, char **argv);
extern void asp_iptraffic(int argc, char **argv);

#ifdef TCONFIG_NOCAT
// nocat.c
extern void wi_uploadsplash(char *url, int len, char *boundary);
extern void wo_uploadsplash(char *url);
#endif

#if TOMATO_SL
// share.c
extern void asp_sharelist(int argc, char **argv);
extern void wo_umount(char *url);
extern void wo_usb(char *url);
#endif

// utf8.c
extern char *utf8_to_js_string(const char *ins);
extern char *utf8_to_html_string(const char *ins);

#endif
