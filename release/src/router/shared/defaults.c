
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
 * Router default NVRAM values
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: defaults.c,v 1.104.10.1.2.3 2006/02/07 08:08:32 honor Exp $
 */

#include <epivers.h>
#include <string.h>
#include <bcmnvram.h>
#include <typedefs.h>
#include <wlioctl.h>
#include <stdio.h>
#include <ezc.h>
#include <bcmconfig.h>

#include <code_pattern.h>
#include <cy_conf.h>

#define XSTR(s) STR(s)
#define STR(s) #s

struct nvram_tuple router_defaults[] = {
	/* OS parameters */
	{ "os_name", "", 0 },			/* OS name string */
	{ "os_version", EPI_VERSION_STR, 0 },	/* OS revision */
	{ "os_date", __DATE__, 0 },		/* OS date */
	{ "ct_modules", "", 0 },		/* CyberTAN kernel modules */

	/* Miscellaneous parameters */

	{ "timer_interval", "3600", 0 },	/* Timer interval in seconds */
	{ "ntp_server", "", 0 },		/* NTP server */	/* Modify */
	{ "ntp_enable", "1", 0 },
	//{ "time_zone", "PST8PDT", 0 },	/* Time zone (GNU TZ format) */
#if OEM == LINKSYS
#if COUNTRY == JAPAN
	{ "time_zone", "+09 1 0", 0 },		/* Time zone (GNU TZ format) Japan */
	{ "daylight_time", "0", 0 },		/* Automatically adjust clock for daylight */
#else
	{ "time_zone", "-08 1 1", 0 },		/* Time zone (GNU TZ format) USA */
	{ "daylight_time", "1", 0 },		/* Automatically adjust clock for daylight */
#endif
#elif OEM == PCI && LANGUAGE == ENGLISH
	{ "time_zone", "+08 2 0", 0 },		/* Time zone (GNU TZ format) (2003-03-19 by honor) */
	{ "daylight_time", "0", 0 },		/* Automatically adjust clock for daylight */
#elif OEM == ELSA
	{ "time_zone", "+01 2 2", 0 },		/* Time zone (GNU TZ format) Germany */
	{ "daylight_time", "1", 0 },		/* Automatically adjust clock for daylight */
#elif COUNTRY == KOREA
	{ "time_zone", "+09 1 0", 0 },		/* Time zone (GNU TZ format) Japan */
	{ "daylight_time", "0", 0 },		/* Automatically adjust clock for daylight */
#else
	{ "time_zone", "-08 1 1", 0 },		/* Time zone (GNU TZ format) USA */
	{ "daylight_time", "1", 0 },		/* Automatically adjust clock for daylight */
#endif

	{ "log_level", "0", 0 },		/* Bitmask 0:off 1:denied 2:accepted */
#ifdef VERIZON_WAN_SUPPORT
	{ "upnp_enable", "0", 0 },		/* 0:Disable 1:Enable */
#else
	{ "upnp_enable", "1", 0 },		/* 0:Disable 1:Enable */
#endif
	{ "upnp_ssdp_interval", "60", 0 },	/* SSDP interval */
	{ "upnp_max_age", "180", 0 },		/* MAX age time */
#ifdef THROUGHPUT_TEST_SUPPORT
	{ "throughput_test", "0", 0 },		/* throughtput test */
#endif
	{ "ezc_enable", "1", 0 },		/* Enable EZConfig updates */
	{ "ezc_version", EZC_VERSION_STR, 0 },	/* EZConfig version */
	{ "is_default", "1", 0 },		/* is it default setting: 1:yes 0:no*/
	{ "os_server", "", 0 },			/* URL for getting upgrades */
	{ "stats_server", "", 0 },		/* URL for posting stats */
	{ "console_loglevel", "1", 0 },		/* Kernel panics only */

	/* Big switches */
	{ "router_disable", "0", 0 },		/* lan_proto=static lan_stp=0 wan_proto=disabled */
	{ "fw_disable", "0", 0 },		/* Disable firewall (allow new connections from the WAN) */

	/* TCP/IP parameters */
	{ "log_enable", "0", 0 },		/* 0:Disable 1:Eanble */	/* Add */
	{ "log_ipaddr", "0", 0 },		/* syslog recipient */
#ifdef SYSLOG_SUPPORT
	{ "log_show_all", "0", 0 },		/* show all message */
	{ "log_show_type", "ALL", 0 },		/* show log type */
#endif

	/* LAN H/W parameters */
	{ "lan_ifname", "", 0 },		/* LAN interface name */
	{ "lan_ifnames", "", 0 },		/* Enslaved LAN interfaces */
	{ "lan_hwnames", "", 0 },		/* LAN driver names (e.g. et0) */
	{ "lan_hwaddr", "", 0 },		/* LAN interface MAC address */

	/* LAN TCP/IP parameters */
	{ "lan_dhcp", "0", 0 },			/* DHCP client [static|dhcp] */
        { "lan_proto", "dhcp", 0 },             /* DHCP server [static|dhcp] */  //Barry add 2004 09 16
	{ "lan_ipaddr", "192.168.1.1", 0 },	/* LAN IP address */
	{ "lan_netmask", "255.255.255.0", 0 },	/* LAN netmask */
	{ "lan_wins", "", 0 },			/* x.x.x.x x.x.x.x ... */
	{ "lan_domain", "", 0 },		/* LAN domain name */
	{ "lan_lease", "86400", 0 },		/* LAN lease time in seconds */
	{ "lan_stp", "0", 0 },			/* LAN spanning tree protocol */
	{ "lan_route", "", 0 },			/* Static routes (ipaddr:netmask:gateway:metric:ifname ...) */
#ifdef VERIZON_LAN_SUPPORT
	{ "dhcrelay_ipaddr", "0.0.0.0", 0 },	/* LAN DHCP Server IP address (for dhcp relay) */
#endif
#ifdef UDHCPD_STATIC_SUPPORT
	{ "dhcp_statics0" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics1" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics2" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics3" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics4" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics5" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics6" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics7" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics8" , "00:00:00:00:00:00 0 off" , 0 } , 
	{ "dhcp_statics9" , "00:00:00:00:00:00 0 off" , 0 } , 
#endif
#ifdef DHCP_FILTER_SUPPORT
	{ "dhcp_mac_list", "", 0 },		/* DHCP server filter mac list */
	{ "dhcp_filter_policy", "0", 0 },	/* DHCP server filter policy */	
#endif

	/* WAN H/W parameters */
	{ "wan_ifname", "", 0 },		/* WAN interface name */
	{ "wan_ifnames", "", 0 },		/* WAN interface names */
	{ "wan_hwname", "", 0 },		/* WAN driver name (e.g. et1) */
	{ "wan_hwaddr", "", 0 },		/* WAN interface MAC address */

	/* WAN TCP/IP parameters */
	{ "wan_proto", "dhcp", 0 },		/* [static|dhcp|pppoe|disabled] */
	{ "wan_ipaddr", "0.0.0.0", 0 },		/* WAN IP address */
	{ "wan_netmask", "0.0.0.0", 0 },	/* WAN netmask */
	{ "wan_gateway", "0.0.0.0", 0 },	/* WAN gateway */
	{ "wan_dns", "", 0 },			/* x.x.x.x x.x.x.x ... */
	{ "wan_wins", "", 0 },			/* x.x.x.x x.x.x.x ... */
	{ "wan_hostname", "", 0 },		/* WAN hostname */
	{ "wan_domain", "", 0 },		/* WAN domain name */
	{ "wan_lease", "86400", 0 },		/* WAN lease time in seconds */
	{ "static_route", "", 0 },		/* Static routes (ipaddr:netmask:gateway:metric:ifname ...) */
	{ "static_route_name", "", 0 },		/* Static routes name ($NAME:name) */

#ifdef BRCM_3_51_8
	/* PPPoE parameters */
	{ "wan_pppoe_ifname", "", 0 },		/* PPPoE enslaved interface */
	{ "wan_pppoe_username", "", 0 },	/* PPP username */
	{ "wan_pppoe_passwd", "", 0 },		/* PPP password */
	{ "wan_pppoe_idletime", "60", 0 },	/* Dial on demand max idle time (seconds) */
	{ "wan_pppoe_keepalive", "0", 0 },	/* Restore link automatically */
	{ "wan_pppoe_demand", "0", 0 },		/* Dial on demand */
	{ "wan_pppoe_mru", "1492", 0 },		/* Negotiate MRU to this value */
	{ "wan_pppoe_mtu", "1492", 0 },		/* Negotiate MTU to the smaller of this value or the peer MRU */
	{ "wan_pppoe_service", "", 0 },		/* PPPoE service name */
	{ "wan_pppoe_ac", "", 0 },		/* PPPoE access concentrator name */

	/* Misc WAN parameters */
	{ "wan_desc", "", 0 },			/* WAN connection description */
	{ "wan_route", "", 0 },			/* Static routes (ipaddr:netmask:gateway:metric:ifname ...) */
#endif
	{ "wan_primary", "1", 0 },		/* Primary wan connection */
	{ "wan_unit", "0", 0 },			/* Last configured connection */

	/* Filters */
	{ "filter_maclist", "", 0 },		/* xx:xx:xx:xx:xx:xx ... */
	{ "filter_macmode", "deny", 0 },	/* "allow" only, "deny" only, or "disabled" (allow all) */
	{ "filter_client0", "", 0 },		/* [lan_ipaddr0-lan_ipaddr1|*]:lan_port0-lan_port1,proto,enable,day_start-day_end,sec_start-sec_end,desc */

	{ "filter", "on", 0 },			/* [on | off] Firewall Protection */
	{ "filter_port", "", 0 },		/* [lan_ipaddr|*]:lan_port0-lan_port1 */
	{ "filter_rule1", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule2", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule3", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule4", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule5", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule6", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule7", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule8", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule9", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_rule10", "", 0 },		/* $STAT: $NAME:$$ */
	{ "filter_tod1", "", 0 },		/* Filter Time of the day */
	{ "filter_tod2", "", 0 },		/* Filter Time of the day */
	{ "filter_tod3", "", 0 },		/* Filter Time of the day */
	{ "filter_tod4", "", 0 },		/* Filter Time of the day */
	{ "filter_tod5", "", 0 },		/* Filter Time of the day */
	{ "filter_tod6", "", 0 },		/* Filter Time of the day */
	{ "filter_tod7", "", 0 },		/* Filter Time of the day */
	{ "filter_tod8", "", 0 },		/* Filter Time of the day */
	{ "filter_tod9", "", 0 },		/* Filter Time of the day */
	{ "filter_tod10", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf1", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf2", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf3", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf4", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf5", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf6", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf7", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf8", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf9", "", 0 },		/* Filter Time of the day */
	{ "filter_tod_buf10", "", 0 },		/* Filter Time of the day */
	{ "filter_ip_grp1", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp2", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp3", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp4", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp5", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp6", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp7", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp8", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp9", "", 0 },		/* Filter IP group 1 */
	{ "filter_ip_grp10", "", 0 },		/* Filter IP group 1 */
	
	{ "filter_in_out1", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out2", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out3", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out4", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out5", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out6", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out7", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out8", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out9", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	{ "filter_in_out10", "0", 0 },		/* Filter traffic direction; 0 : outgoing, 1 : incoming */
	
	


	{ "filter_mac_grp1", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp2", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp3", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp4", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp5", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp6", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp7", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp8", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp9", "", 0 },		/* Filter MAC group 1 */
	{ "filter_mac_grp10", "", 0 },		/* Filter MAC group 1 */
	{ "filter_web_host1", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host2", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host3", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host4", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host5", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host6", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host7", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host8", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host9", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_host10", "", 0 },		/* Website Blocking by URL Address */
	{ "filter_web_url1", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url2", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url3", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url4", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url5", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url6", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url7", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url8", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url9", "", 0 },		/* Website Blocking by keyword */
	{ "filter_web_url10", "", 0 },		/* Website Blocking by keyword */
	{ "filter_port_grp1", "", 0 },		/* Blocked Services */
	{ "filter_port_grp2", "", 0 },		/* Blocked Services */
	{ "filter_port_grp3", "", 0 },		/* Blocked Services */
	{ "filter_port_grp4", "", 0 },		/* Blocked Services */
	{ "filter_port_grp5", "", 0 },		/* Blocked Services */
	{ "filter_port_grp6", "", 0 },		/* Blocked Services */
	{ "filter_port_grp7", "", 0 },		/* Blocked Services */
	{ "filter_port_grp8", "", 0 },		/* Blocked Services */
	{ "filter_port_grp9", "", 0 },		/* Blocked Services */
	{ "filter_port_grp10", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp1", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp2", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp3", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp4", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp5", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp6", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp7", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp8", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp9", "", 0 },		/* Blocked Services */
	{ "filter_dport_grp10", "", 0 },	/* Blocked Services */
	{ "filter_services", "$NAME:003:DNS$PROT:003:udp$PORT:005:53:53<&nbsp;>$NAME:004:Ping$PROT:004:icmp$PORT:003:0:0<&nbsp;>$NAME:004:HTTP$PROT:003:tcp$PORT:005:80:80<&nbsp;>$NAME:005:HTTPS$PROT:003:tcp$PORT:007:443:443<&nbsp;>$NAME:003:FTP$PROT:003:tcp$PORT:005:21:21<&nbsp;>$NAME:004:POP3$PROT:003:tcp$PORT:007:110:110<&nbsp;>$NAME:004:IMAP$PROT:003:tcp$PORT:007:143:143<&nbsp;>$NAME:004:SMTP$PROT:003:tcp$PORT:005:25:25<&nbsp;>$NAME:004:NNTP$PROT:003:tcp$PORT:007:119:119<&nbsp;>$NAME:006:Telnet$PROT:003:tcp$PORT:005:23:23<&nbsp;>$NAME:004:SNMP$PROT:003:udp$PORT:007:161:161<&nbsp;>$NAME:004:TFTP$PROT:003:udp$PORT:005:69:69<&nbsp;>$NAME:003:IKE$PROT:003:udp$PORT:007:500:500<&nbsp;>", 0 },		/* Services List */

	/* Port forwards */
	{ "dmz_enable", "0", 0 },		/* Enable (1) or Disable (0) */
	{ "dmz_ipaddr", "0", 0 },		/* x.x.x.x (equivalent to 0-60999>dmz_ipaddr:0-60999) */
	{ "autofw_port0", "", 0 },		/* out_proto:out_port,in_proto:in_port0-in_port1>to_port0-to_port1,enable,desc */

	/* DHCP server parameters */
#ifdef VERIZON_LAN_SUPPORT
	{ "dhcp_start", "64", 0 },		/* First assignable DHCP address */
	{ "dhcp_num", "191", 0 },		/* Number of DHCP Users */	/* Add */
#else
	{ "dhcp_start", "100", 0 },		/* First assignable DHCP address */
	//{ "dhcp_end", "150", 0 },		/* Last assignable DHCP address */	/* Remove */
	{ "dhcp_num", "50", 0 },		/* Number of DHCP Users */	/* Add */
#endif
#if OEM == LINKSYS
	{ "dhcp_lease", "0", 0 },		/* LAN lease time in minutes */
#else
	{ "dhcp_lease", "60", 0 },		/* LAN lease time in minutes */ /* Add */
#endif
	{ "dhcp_domain", "wan", 0 },		/* Use WAN domain name first if available (wan|lan) */
	{ "dhcp_wins", "wan", 0 },		/* Use WAN WINS first if available (wan|lan) */
	{ "wan_get_dns", "", 0 },		/* DNS IP address which get by dhcpc */ /* Add */

	/* Web server parameters */
#ifdef MUST_CHANGE_PWD_SUPPORT
	{ "is_not_first_access", "0", 0 },               /* DNS IP address which get by dhcpc */ /* Add */
#endif

#ifdef DDM_SUPPORT
	{ "http_username", "admin", 0 },		/* Username */
#else
	{ "http_username", "", 0 },		/* Username */
#endif
#ifdef MULTIPLE_LOGIN_SUPPORT //roger for multiple login 2004-11-30
	{ "http_login", "$NAME:005:admin$PSW:008:password$MOD:005:admin<&nbsp;>$NAME:004:user$PSW:004:user$MOD:004:user<&nbsp;>", 0 }, /* Services List */
	{ "current_login_name", "", 0 },
#endif
#if OEM == LINKSYS

#ifdef DDM_SUPPORT
	{ "http_passwd", "password", 0 },		/* DDM_SUPPORT Password */
#else
	{ "http_passwd", "admin", 0 },		/* Password */
#endif

#elif OEM == PCI && LANGUAGE == ENGLISH
	{ "http_passwd", "0000", 0 },		/* Password */
#elif OEM == PCI && LANGUAGE == JAPANESE
	{ "http_passwd", "password", 0 },		/* Password */
#else
	{ "http_passwd", "admin", 0 },		/* Password */
#endif
	{ "http_wanport", "8080", 0 },		/* WAN port to listen on */
	{ "http_lanport", "80", 0 },		/* LAN port to listen on */
	
	{ "http_enable", "1", 0 },		/* HTTP server enable/disable */
#ifdef HTTPS_SUPPORT
	{ "https_enable", "0", 0 },		/* HTTPS server enable/disable */
#endif

#ifdef GET_POST_SUPPORT
	{ "http_method", "post", 0 },		/* HTTP method */
#else
	{ "http_method", "get", 0 },		/* HTTP method */
#endif
#ifdef WIRELESS_SUPPORT
	{ "web_wl_filter", "0", 0 },		/* Allow/Deny Wireless Access Web */
#endif
	/* PPPoE parameters */
	{ "pppoe_ifname", "", 0 },		/* PPPoE enslaved interface */
	{ "ppp_username", "", 0 },		/* PPP username */
	{ "ppp_passwd", "", 0 },		/* PPP password */
#ifdef VERIZON_WAN_SUPPORT
	{ "ppp_idletime", "30", 0 },		/* Dial on demand max idle time (mins) */
	{ "enable_tftpd", "0", 0 },		/* Dial on demand max idle time (mins) */
#else
	{ "ppp_idletime", "5", 0 },		/* Dial on demand max idle time (mins) */
#endif
	{ "ppp_keepalive", "0", 0 },		/* Restore link automatically */
#if OEM == LINKSYS
#if COUNTRY == EUROPE
	{ "ppp_demand", "1", 0 },		/* Dial on demand */
#elif COUNTRY == GERMANY
	{ "ppp_demand", "1", 0 },		/* Dial on demand */
#elif COUNTRY == FRANCE
	{ "ppp_demand", "1", 0 },		/* Dial on demand */
#else
	{ "ppp_demand", "0", 0 },		/* Dial on demand */
#endif
#elif OEM == ALLNET
	{ "ppp_demand", "1", 0 },		/* Dial on demand */
#else
	{ "ppp_demand", "0", 0 },		/* Dial on demand */
#endif
	{ "ppp_redialperiod", "30", 0 },	/* Redial Period  (seconds)*/
	{ "ppp_mru", "1500", 0 },		/* Negotiate MRU to this value */
	{ "ppp_mtu", "1500", 0 },		/* Negotiate MTU to the smaller of this value or the peer MRU */
	{ "ppp_service", "", 0 },		/* PPPoE service name */
	{ "ppp_ac", "", 0 },			/* PPPoE access concentrator name */
	{ "ppp_static", "0", 0 },		/* Enable / Disable Static IP  */
	{ "ppp_static_ip", "", 0 },		/* PPPoE Static IP */
	{ "ppp_get_ac", "", 0 },		/* PPPoE Server ac name */
	{ "ppp_get_srv", "", 0 },		/* PPPoE Server service name */

#ifdef MPPPOE_SUPPORT
	{ "ppp_username_1", "", 0 },		/* PPP username */
	{ "ppp_passwd_1", "", 0 },		/* PPP password */
	{ "ppp_idletime_1", "5", 0 },		/* Dial on demand max idle time (mins) */
	{ "ppp_demand_1", "0", 0 },		/* Dial on demand */
	{ "ppp_redialperiod_1", "30", 0 },	/* Redial Period  (seconds)*/
	{ "ppp_service_1", "", 0 },		/* PPPoE Service Name */
	{ "mpppoe_enable", "0", 0 },		/* PPPoE Service Name */
	{ "mpppoe_dname", "", 0 },		/* PPPoE Service Name */
#endif
#ifdef WIRELESS_SUPPORT
	/* Wireless parameters */
	{ "wl_ifname", "", 0 },			/* Interface name */
	{ "wl_hwaddr", "", 0 },			/* MAC address */
	{ "wl_phytype", "g", 0 },		/* Current wireless band ("a" (5 GHz), "b" (2.4 GHz), or "g" (2.4 GHz)) */	/* Modify */
	{ "wl_corerev", "", 0 },		/* Current core revision */
	{ "wl_phytypes", "", 0 },		/* List of supported wireless bands (e.g. "ga") */
	{ "wl_radioids", "", 0 },		/* List of radio IDs */
#if OEM == LINKSYS
	{ "wl_ssid", "linksys", 0 },		/* Service set ID (network name) */
#elif OEM == PCI && LANGUAGE == JAPANESE
	{ "wl_ssid", "BLW-04G", 0 },		/* Service set ID (network name) */
#elif OEM == PCI && LANGUAGE == ENGLISH
	{ "wl_ssid", "bRoadLannerWave", 0 },	/* Service set ID (network name) (2003-03-19 by honor) */
#else
	{ "wl_ssid", "wireless", 0 },		/* Service set ID (network name) */
#endif
	{ "wl_country", "Worldwide", 0 },		/* Country (default obtained from driver) */
	{ "wl_radio", "1", 0 },			/* Enable (1) or disable (0) radio */
	{ "wl_closed", "0", 0 },		/* Closed (hidden) network */
        { "wl_ap_isolate", "0", 0 },            /* AP isolate mode */
	{ "wl_mode", "ap", 0 },			/* AP mode (ap|sta|wds) */
	{ "wl_lazywds", "1", 0 },		/* Enable "lazy" WDS mode (0|1) */
	{ "wl_wds", "", 0 },			/* xx:xx:xx:xx:xx:xx ... */
	{ "wl_wds_timeout", "1", 0 },		/* WDS link detection interval defualt 1 sec*/
	{ "wds_ifname", "", 0 },		/* WDS link detection interval defualt 1 sec*/
	{ "wl_wep", "disabled", 0 },		/* WEP data encryption (enabled|disabled) */
	{ "wl_auth", "0", 0 },			/* Shared key authentication optional (0) or required (1) */
	{ "wl_key", "1", 0 },			/* Current WEP key */
	{ "wl_key1", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key2", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key3", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_key4", "", 0 },			/* 5/13 char ASCII or 10/26 char hex */
	{ "wl_maclist", "", 0 },		/* xx:xx:xx:xx:xx:xx ... */
	{ "wl_macmode", "disabled", 0 },	/* "allow" only, "deny" only, or "disabled" (allow all) */
	{ "wl_macmode1", "disabled", 0 },	/* "disabled" or "other" for WEBB */	/* Add */
#if OEM == LINKSYS
#if COUNTRY == EUROPE
	{ "wl_channel", "11", 0 },		/* Channel number */
#elif COUNTRY == FRANCE
	{ "wl_channel", "11", 0 },		/* Channel number */
#elif COUNTRY == GERMANY
	{ "wl_channel", "11", 0 },		/* Channel number */
#else
	{ "wl_channel", "6", 0 },		/* Channel number */
#endif
#else
	{ "wl_channel", "6", 0 },		/* Channel number */
#endif
	{ "wl_rate", "0", 0 },			/* Rate (bps, 0 for auto) */
	{ "wl_mrate", "0", 0 },			/* Mcast Rate (bps, 0 for auto) */
	{ "wl_rateset", "default", 0 },		/* "default" or "all" or "12" */
	{ "wl_frag", "2346", 0 },		/* Fragmentation threshold */
	{ "wl_rts", "2347", 0 },		/* RTS threshold */
	{ "wl_dtim", "1", 0 },			/* DTIM period (3.11.5)*/	/* It is best value for WiFi test */
	{ "wl_bcn", "100", 0 },			/* Beacon interval */
	{ "wl_plcphdr", "long", 0 },		/* 802.11b PLCP preamble type */
	{ "wl_net_mode", "mixed", 0 },		/* Wireless mode (mixed|g-only|b-only|disable) */
#endif
#ifdef SPEED_BOOSTER_SUPPORT
	{ "wl_gmode", "6", 0 },			/* 54g mode */
	//{ "wl_afterburner_override", "-1", 0 },	/* afterburner override (0|1|-1) */
	{ "wl_gmode_protection", "off", 0 },	/* 802.11g RTS/CTS protection (off|auto) */
	{ "wl_afterburner", "auto", 0 },	/* AfterBurner */
	{ "wl_frameburst", "on", 0 },		/* BRCM Frambursting mode (off|on) */
#else
	{ "wl_gmode", "1", 0 },			/* 54g mode */
	{ "wl_gmode_protection", "off", 0 },	/* 802.11g RTS/CTS protection (off|auto) */
	{ "wl_afterburner", "off", 0 },		/* AfterBurner */
	{ "wl_frameburst", "off", 0 },		/* BRCM Frambursting mode (off|on) */
#endif
#ifdef WIRELESS_SUPPORT
	{ "wl_wme", "off", 0 },			/* WME mode (off|on) */
	{ "wl_antdiv", "-1", 0 },		/* Antenna Diversity (-1|0|1|3) */
	{ "wl_infra", "1", 0 },			/* Network Type (BSS/IBSS) */

	{ "wl_passphrase", "", 0 },		/* Passphrase */	/* Add */
	{ "wl_wep_bit", "64", 0 },		/* WEP encryption [64 | 128] */ /* Add */
	{ "wl_wep_buf", "", 0 },		/* save all settings for web */ /* Add */
	{ "wl_wep_gen", "", 0 },		/* save all settings for generate button */	/* Add */
	{ "wl_wep_last", "", 0 },		/* Save last wl_wep mode */	/* Add */
	{ "wl_active_mac", "", 0 },		/* xx:xx:xx:xx:xx:xx ... */	/* Add */
	{ "wl_mac_list", "", 0 },		/* filter MAC */	/* Add */
	{ "wl_mac_deny", "", 0 },		/* filter MAC */	/* Add */

	/* WPA parameters */
	{ "security_mode2", "disabled", 0 },	/* WPA mode (disabled|radius|wpa_personal|wpa_enterprise|wep|wpa2_personal|wpa2_enterprise) for WEB */	/* Add */
	{ "security_mode", "disabled", 0 },	/* WPA mode (disabled|radius|wpa|psk|wep|psk psk2|wpa wpa2) for WEB */	/* Add */
	{ "security_mode_last", "", 0 },	/* Save last WPA mode */	/* Add */
	//{ "wl_auth_mode", "disabled", 0 },	/* WPA mode (disabled|radius|wpa|psk) */
	{ "wl_auth_mode", "none", 0 },		/* Network authentication mode (radius|none) */
	{ "wl_wpa_psk", "", 0 },		/* WPA pre-shared key */
	{ "wl_wpa_gtk_rekey", "3600", 0 },	/* WPA GTK rekey interval */	/* Modify */
	{ "wl_radius_ipaddr", "", 0 },		/* RADIUS server IP address */
	{ "wl_radius_key", "", 0 },		/* RADIUS shared secret */
	{ "wl_radius_port", "1812", 0 },	/* RADIUS server UDP port */
	{ "wl_crypto", "tkip", 0 },		/* WPA data encryption */
	{ "wl_net_reauth", "36000", 0 },	/* Network Re-auth/PMK caching duration */
	{ "wl_akm", "", 0 },			/* WPA akm list */

#ifdef __CONFIG_SES__
	/* SES parameters */
	{ "ses_enable", "1", 0 },		/* enable ses */
	{ "ses_event", "2", 0 },		/* initial ses event */
	{ "gpio2", "ses_led", 0 },		/* For SES II */
	{ "gpio3", "ses_led2", 0 },		/* For SES II */
	{ "gpio4", "ses_button",0 },		/* For SES II */
	{ "ses_led_assertlvl", "0",0 },		/* For SES II */
	{ "ses_client_join", "0",0 },		/* For SES II */
	{ "ses_sw_btn_status","DEFAULTS",0 }, /* Barry Adds 20050309 for SW SES BTN */
	{ "ses_count", "0",0 },
#endif /* __CONFIG_SES__ */

	/* WME parameters */
	/* EDCA parameters for STA */
	{ "wl_wme_sta_bk", "15 1023 7 0 0 off", 0 },	/* WME STA AC_BK paramters */
	{ "wl_wme_sta_be", "15 1023 3 0 0 off", 0 },	/* WME STA AC_BE paramters */
	{ "wl_wme_sta_vi", "7 15 2 6016 3008 off", 0 },	/* WME STA AC_VI paramters */
	{ "wl_wme_sta_vo", "3 7 2 3264 1504 off", 0 },	/* WME STA AC_VO paramters */

	/* EDCA parameters for AP */
	{ "wl_wme_ap_bk", "15 1023 7 0 0 off", 0 },	/* WME AP AC_BK paramters */
	{ "wl_wme_ap_be", "15 63 3 0 0 off", 0 },	/* WME AP AC_BE paramters */
	{ "wl_wme_ap_vi", "7 15 1 6016 3008 off", 0 },	/* WME AP AC_VI paramters */
	{ "wl_wme_ap_vo", "3 7 1 3264 1504 off", 0 },	/* WME AP AC_VO paramters */

	{ "wl_wme_no_ack", "off", 0},		/* WME No-Acknowledgmen mode */

	{ "wl_maxassoc", "128", 0},		/* Max associations driver could support */

	{ "wl_unit", "0", 0 },			/* Last configured interface */
#endif
	/* Restore defaults */
	{ "restore_defaults", "0", 0 },		/* Set to 0 to not restore defaults on boot */

	////////////////////////////////////////
	{ "router_name", MODEL_NAME, 0 },	/* Router name string */
	{ "ntp_mode", "auto", 0 },		/* NTP server [manual | auto] */
	{ "pptp_server_ip", "", 0 },		/* as same as WAN gateway */
	{ "pptp_get_ip", "", 0 },		/* IP Address assigned by PPTP server */

	/* for firewall */
	{ "filter", "on", 0 },			/* Firewall Protection [on|off] */
	{ "block_loopback", "0", 0 },		/* Filter Internet NAT Redirection [1|0] */
	{ "block_wan", "1", 0 },		/* Block WAN Request [1|0] */
	{ "ident_pass", "0", 0 },		/* IDENT passthrough [1|0] */
	{ "block_proxy", "0", 0 },		/* Block Proxy [1|0] */
	{ "block_java", "0", 0 },		/* Block Java [1|0] */
	{ "block_activex", "0", 0 },		/* Block ActiveX [1|0] */
	{ "block_cookie", "0", 0 },		/* Block Cookie [1|0] */
	{ "multicast_pass", "0", 0 },		/* Multicast Pass Through [1|0] */
	{ "ipsec_pass", "1", 0 },		/* IPSec Pass Through [1|0] */
	{ "pptp_pass", "1", 0 },		/* PPTP Pass Through [1|0] */
	{ "l2tp_pass", "1", 0 },		/* L2TP Pass Through [1|0] */
#ifdef PPPOE_RELAY_SUPPORT
	{ "pppoe_pass", "1", 0 },		/* PPPoE Pass Through [1|0] */
#endif
	{ "remote_management", "0", 0 },	/* Remote Management [1|0]*/
#ifdef HTTPS_SUPPORT
	{ "remote_mgt_https", "0", 0 },		/* Remote Management use https [1|0]*/	// add
#endif
	{ "mtu_enable", "0", 0 },		/* WAN MTU [1|0] */
	{ "wan_mtu", "1500", 0 },		/* Negotiate MTU to the smaller of this value or the peer MRU */

	/* for forward */
#ifdef UPNP_FORWARD_SUPPORT
						/* wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1[:,]proto[:,]enable[:,]desc */
	{ "forward_port0", "21-21>192.168.1.0:21-21,tcp,off,FTP", 0 },		// UPnP Forwarding default rule
	{ "forward_port1", "23-23>192.168.1.0:23-23,tcp,off,Telnet", 0 },	// UPnP Forwarding default rule
	{ "forward_port2", "25-25>192.168.1.0:25-25,tcp,off,SMTP", 0 },		// UPnP Forwarding default rule
	{ "forward_port3", "53-53>192.168.1.0:53-53,udp,off,DNS", 0 },		// UPnP Forwarding default rule
	{ "forward_port4", "69-69>192.168.1.0:59-59,udp,off,TFTP", 0 },		// UPnP Forwarding default rule
	{ "forward_port5", "79-79>192.168.1.0:79-79,tcp,off,finger", 0 },	// UPnP Forwarding default rule
	{ "forward_port6", "80-80>192.168.1.0:80-80,tcp,off,HTTP", 0 },		// UPnP Forwarding default rule
	{ "forward_port7", "110-110>192.168.1.0:110-110,tcp,off,POP3", 0 },	// UPnP Forwarding default rule
	{ "forward_port8", "119-119>192.168.1.0:119-119,tcp,off,NNTP", 0 },	// UPnP Forwarding default rule
	{ "forward_port9", "161-161>192.168.1.0:161-161,udp,off,SNMP", 0 },	// UPnP Forwarding default rule
#endif
#ifdef ALG_FORWARD_SUPPORT
						/* wan_port0-wan_port1>lan_ipaddr:lan_port0-lan_port1[:,]proto[:,]enable[:,]desc */
	{ "forward_alg0", "21-21>192.168.1.0:21-21,tcp,accept,off,FTP", 0 },		// ALG Forwarding default rule
	{ "forward_alg1", "23-23>192.168.1.0:23-23,tcp,accept,off,Telnet", 0 },	// ALG Forwarding default rule
	{ "forward_alg2", "25-25>192.168.1.0:25-25,tcp,accept,off,SMTP", 0 },		// ALG Forwarding default rule
	{ "forward_alg3", "53-53>192.168.1.0:53-53,udp,accept,off,DNS", 0 },		// ALG Forwarding default rule
	{ "forward_alg4", "59-59>192.168.1.0:59-59,udp,accept,off,TFTP", 0 },		// ALG Forwarding default rule
	{ "forward_alg5", "79-79>192.168.1.0:79-79,tcp,accept,off,finger", 0 },	// ALG Forwarding default rule
	{ "forward_alg6", "80-80>192.168.1.0:80-80,tcp,accept,off,HTTP", 0 },		// ALG Forwarding default rule
	{ "forward_alg7", "110-110>192.168.1.0:110-110,tcp,accept,off,POP3", 0 },	// ALG Forwarding default rule
	{ "forward_alg8", "119-119>192.168.1.0:119-119,tcp,accept,off,NNTP", 0 },	// ALG Forwarding default rule
	{ "forward_alg9", "161-161>192.168.1.0:161-161,udp,accept,off,SNMP", 0 },	// ALG Forwarding default rule
	{ "forward_alg10", "", 0 },	// ALG Forwarding default rule
	{ "forward_alg11", "", 0 },	// ALG Forwarding default rule
	{ "forward_alg12", "", 0 },	// ALG Forwarding default rule
	{ "forward_alg13", "", 0 },	// ALG Forwarding default rule
	{ "forward_alg14", "", 0 },	// ALG Forwarding default rule
#endif
	{ "forward_port", "", 0 },		/* name:[on|off]:[tcp|udp|both]:wan_port>lan_ipaddr:lan_port0 */
        { "forward_portsip", "", 0 },           /* src_ipaddr */
	{ "port_trigger", "", 0 },		/* name:[on|off]:[tcp|udp|both]:wan_port0-wan_port1>lan_port0-lan_port1 */

	/* for dynamic route */
	{ "wk_mode", "gateway", 0 },		/* Network mode [gateway|router]*/
	{ "dr_setting", "0", 0},		/* [ Disable | WAN | LAN | Both ] */
	{ "dr_lan_tx", "0", 0 },		/* Dynamic-Routing LAN out */
	{ "dr_lan_rx", "0", 0 },		/* Dynamic-Routing LAN in  */
	{ "dr_wan_tx", "0", 0 },		/* Dynamic-Routing WAN out */
	{ "dr_wan_rx", "0", 0 },		/* Dynamic-Routing WAN in  */

	/* for mac clone */
	{ "mac_clone_enable", "0", 0 },		/* User define WAN interface MAC address */
	{ "def_hwaddr", "00:00:00:00:00:00", 0 },		/* User define WAN interface MAC address */

	/* for DDNS */
		/* for dyndns */
	{ "ddns_enable", "0", 0 },		/* 0:Disable 1:dyndns 2:tzo*/
	{ "ddns_username", "", 0 },		/* DynDNS Username */
	{ "ddns_passwd", "", 0 },		/* DynDNS Password */
	{ "ddns_hostname", "", 0 },		/* DynDNS Hostname */
	{ "ddns_service", "dyndns", 0 },	/* [ dyndns | dyndns-static | dyndns-custom ] DynDNS service */
	{ "ddns_wildcard", "OFF", 0 },		/* Enable wildcard [ON | OFF]*/
	{ "ddns_mx", "", 0 },			/* Mail Exchange */
	{ "ddns_backmx", "NO", 0 },		/* Backup MX [NO | YES]*/
		/* for tzo */
	{ "ddns_username_2", "", 0 },		/* TZO Email Address */
	{ "ddns_passwd_2", "", 0 },		/* TZO Password Key */
	{ "ddns_hostname_2", "", 0 },		/* TZO Domain Name */
#ifdef DDNS3322_SUPPORT
		/* for 3322 */
	{ "ddns_username_3", "", 0 },           /* 3322 Email Address */
	{ "ddns_passwd_3", "", 0 },             /* 3322 Password Key */
	{ "ddns_hostname_3", "", 0 },           /* 3322 Domain Name */
#endif
#ifdef PEANUTHULL_SUPPORT
		/* for peanuthull */
	{ "ddns_username_4", "", 0 },           /* peanuthull Email Address */
	{ "ddns_passwd_4", "", 0 },             /* peanuthull Password Key */
	{ "ddns_hostname_4", "", 0 },           /* peanuthull Domain Name */
#endif
		/* for last value */
	{ "ddns_enable_buf", "", 0 },		/* 0:Disable 1:Eanble */
	{ "ddns_username_buf", "", 0 },		/* DDNS username */
	{ "ddns_passwd_buf", "", 0 },		/* DDNS password */
	{ "ddns_hostname_buf", "", 0 },		/* DDNS hostname */

	{ "ddns_status", "", 0 },		/* DDNS status */
	{ "ddns_interval", "60", 0 },		/* DDNS timer interval in second */
	{ "ddns_cache", "", 0 },		/* DDNS cache data */
	{ "public_ip", "", 0 },			/* public ip */

	/* for AOL */
	{ "aol_block_traffic", "0", 0 },	/* 0:Disable 1:Enable for global */
	{ "aol_block_traffic1", "0", 0 },	/* 0:Disable 1:Enable for "ppp_username" */
	{ "aol_block_traffic2", "0", 0 },	/* 0:Disable 1:Enable for "Parental control" */
	{ "skip_amd_check", "0", 0 },		/* 0:Disable 1:Enable */
	{ "skip_intel_check", "0", 0 },		/* 0:Disable 1:Enable */

#ifdef HSIAB_SUPPORT
	{ "hsiab_mode", "0", 0},		/* 0:Disable 1:Enable */
	{ "hsiab_provider", "0", 0},		/* HSIAB Provider */
	{ "hsiab_device_id", "", 0},		/* MAC/serial */
	{ "hsiab_device_password", "", 0},
	{ "hsiab_admin_url", "", 0},
	{ "hsiab_registered", "0", 0},		/* Hsiab device has been registered */
	{ "hsiab_configured", "0", 0},		/* Hsiab device has been configured */
	{ "hsiab_register_ops", "https://hsiab.boingo.com/ops", 0},	/* Register Ops */
	{ "hsiab_session_ops", "https://hsiab.boingo.com/ops", 0},	/* Session Ops */
	{ "hsiab_config_ops", "https://hsiab.boingo.com/ops", 0},	/* Config Ops */
	{ "hsiab_manual_reg_ops", "", 0},	/* Register Ops */
	{ "hsiab_proxy_host", "", 0},
	{ "hsiab_proxy_port", "", 0},
	{ "hsiab_conf_time", "0", 0},		/* N seconds to receive config from HSAIB server */
	{ "hsiab_stats_time", "0", 0},		/* N seconds to send statistics to HSIAB server */
	{ "hsiab_session_time", "60", 0},	/* N seconds to check session */
	{ "hsiab_sync", "1", 0},		/* 1: Sync Setting 0: nothing */
	{ "hsiab_config", "", 0},		/* HSIAB configuration */
#endif

#ifdef L2TP_SUPPORT
	{ "l2tp_server_ip", "", 0},		/* L2TP auth server (IP Address) */
	{ "l2tp_get_ip", "", 0 },		/* IP Address assigned by L2TP server */
	{ "wan_gateway_buf", "0.0.0.0", 0 },	/* save the default gateway for DHCP */
#endif

#ifdef HEARTBEAT_SUPPORT
	{ "hb_server_ip", "", 0},		/* heartbeat auth server (IP Address) */
	{ "hb_server_domain", "", 0},		/* heartbeat auth server (domain name) */
#endif

#ifdef PARENTAL_CONTROL_SUPPORT
	{ "artemis_enable", "0", 0},		/* 0:Disable 1:Enable */
	{ "artemis_provisioned", "0", 0},	/* 0:no register 1:registered */
	{ "artemis_SVCGLOB", "", 0},
	{ "artemis_HB_DB", "", 0},
	{ "artemis_GLOB", "", 0},
	{ "artemis_NOS_CTR", "", 0},
	//{ "artemis_cpeid", "", 0},
#endif

#ifdef WL_STA_SUPPORT
	{ "wl_ap_ssid", "", 0},			/* SSID of associating AP */
	{ "wl_ap_ip", "", 0},			/* Default IP of associating AP */
#endif

#ifdef DDM_SUPPORT
	{ "DDM_Error_No", "0", 0},		/* DDM Setting respone Error Number */
	{ "DDM_Error_Desc", "No Error", 0},	/* DDM Setting respone Error Descript */
	{ "DDM_pass_flag", "0", 0},	/* DDM Setting respone Error Descript */
#endif
#ifdef HW_QOS_SUPPORT
	{ "wan_speed", "4", 0},			/* 0:10 Mb Full, 1:10 Mb Half, 2:100 Mb Full, 3:100 Mb Half, 4:Auto */
	{ "QoS", "0", 0},
	{ "rate_mode", "1", 0},
	{ "manual_rate", "0", 0},
	/*{ "sel_qosftp", "0", 0},
	{ "sel_qoshttp", "0", 0},
	{ "sel_qostelnet", "0", 0},
	{ "sel_qossmtp", "0", 0},
	{ "sel_qospop3", "0", 0},*/
	{ "sel_qosport1", "0", 0},
	{ "sel_qosport2", "0", 0},
	{ "sel_qosport3", "0", 0},
	{ "sel_qosport4", "0", 0},
	{ "sel_qosport5", "0", 0},
	{ "sel_qosport6", "0", 0},
	{ "sel_qosport7", "0", 0},
	{ "sel_qosport8", "0", 0},
	{ "qos_appname1", "", 0},
	{ "qos_appname2", "", 0},
	{ "qos_appname3", "", 0},
	{ "qos_appname4", "", 0},
	{ "qos_appname5", "", 0},
	{ "qos_appname6", "", 0},
	{ "qos_appname7", "", 0},
	{ "qos_appname8", "", 0},
	{ "qos_appport1", "0", 0},
	{ "qos_appport2", "0", 0},
	{ "qos_appport3", "0", 0},
	{ "qos_appport4", "0", 0},
	{ "qos_appport5", "0", 0},
	{ "qos_appport6", "0", 0},
	{ "qos_appport7", "0", 0},
	{ "qos_appport8", "0", 0},
	{ "qos_devpri1", "0", 0},
	{ "qos_devpri2", "0", 0},
	{ "qos_devmac1", "00:00:00:00:00:00", 0 },
	{ "qos_devmac2", "00:00:00:00:00:00", 0 },
	{ "qos_devname1", "", 0 },
	{ "qos_devname2", "", 0 },
	{ "port_priority_1", "0" ,0},		/* port 1 priority; 1:high, 0:low */
	{ "port_flow_control_1", "1", 0},	/* port 1 flow control; 1:enable, 0:disable */
	{ "port_rate_limit_1", "0", 0},		/* port 1 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M */
	{ "port_priority_2", "0", 0},		/* port 2 priority; 1:high, 0:low */
	{ "port_flow_control_2", "1", 0},	/* port 2 flow control; 1:enable, 0:disable */
	{ "port_rate_limit_2", "0" , 0},	/* port 2 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M */
	{ "port_priority_3", "0", 0},		/* port 3 priority; 1:high, 0:low */
	{ "port_flow_control_3", "1", 0},	/* port 3 flow control; 1:enable, 0:disable */
	{ "port_rate_limit_3", "0", 0},		/* port 3 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M */
	{ "port_priority_4", "0", 0},		/* port 4 priority; 1:high, 0:low */
	{ "port_flow_control_4", "1", 0},	/* port 4 flow control; 1:enable, 0:disable */
	{ "port_rate_limit_4", "0", 0},		/* port 4 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M */
	{ "enable_game", "0", 0},		/* Gamming */
#endif
#ifdef DNS_SUPPORT
	{ "dns_entry0", "0 0.0.0.0 NULL", 0},
	{ "dns_entry1", "0 0.0.0.0 NULL", 0},
  	{ "dns_entry2", "0 0.0.0.0 NULL", 0},
	{ "dns_entry3", "0 0.0.0.0 NULL", 0},
	{ "dns_entry4", "0 0.0.0.0 NULL", 0},
	{ "dns_entry5", "0 0.0.0.0 NULL", 0},
	{ "dns_entry6", "0 0.0.0.0 NULL", 0},
	{ "dns_entry7", "0 0.0.0.0 NULL", 0},
	{ "dns_entry8", "0 0.0.0.0 NULL", 0},
 	{ "dns_entry9", "0 0.0.0.0 NULL", 0},
#endif
#ifdef SNMP_SUPPORT
 	{ "snmp_contact", "support@linksys.com", 0},
 	{ "snmp_device", "WRT54G", 0},
 	{ "snmp_location", "Linksys", 0},
 	{ "snmp_getcom", "public", 0},
 	{ "snmp_setcom", "private", 0},
 	{ "snmp_trust", "192.168.1.0", 0},
 	{ "trap_com", "public", 0},
 	{ "trap_dst", "1", 0},
 	{ "snmpv3_username", "admin", 0},
 	{ "snmpv3_passwd", "12345678", 0},
#endif	
#ifdef EOU_SUPPORT
	{ "eou_configured", "0", 0},
	//{ "eou_device_id", "", 0},
	//{ "eou_public_key", "", 0},
	//{ "eou_private_key","", 0},
	//{ "eou_public", "b49b5ec6866f5b166cc058110b20551d4fe7a5c96a9b5f01a3929f40015e4248359732b7467bae4948d6bb62f96996a7122c6834311c1ea276b35d12c37895501c0f5bd215499cf443d580b999830ac620ac2bf3b7f912741f54fea17627d13a92f44d014030d5c8d3249df385f500ffc90311563e89aa290e7c6f06ef9a6ec311", 0},
	//{ "eou_private","1fdf2ed7bd5ef1f4e603d34e4d41f0e70e19d1f65e1b6b1e6828eeed2d6afca354c0543e75d9973a1be9a898fed665e13f713f90bd5f50b3421fa7034fabde1ce63c44d01a5489765dc4dc3486521163bf6288db6c5e99c44bbb0ad7494fef20148ad862662dabcbff8dae7b466fad087d9f4754e9a6c84bc9adcbda7bc22e59", 0},
 	{ "eou_expired_hour", "72", 0},     /*The expired time is 72 hours, and this value = 72 * 10*/
#endif
#ifdef TINYLOGIN_SUPPORT
	{ "root_passwd", "admin", 0},
	{ "accounts_info", "michael:test:default barry:hahaha:admin", 0},
#endif
	{ "manual_boot_nv", "0", 0},
#ifdef GOOGLE_SUPPORT
	{ "google_enable", "0", 0},	
	{ "google_pass_mac", "", 0},	
	{ "google_dns", "", 0},	
	{ "google_pass_host", "*:udp:53 vpn.google.com:tcp:1723,443 wifi.google.com:tcp:443", 0},	
	{ "google_ori_ip", "", 0},
#endif	
	{ 0, 0, 0 }
};
