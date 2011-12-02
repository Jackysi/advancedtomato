/*

	Copyright 2003, CyberTAN  Inc.
	All Rights Reserved.

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS",	AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS",	AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/

#include <string.h>
#include <bcmnvram.h>

#include <tomato_config.h>	//!!TB
#include "tomato_profile.h"
#include "defaults.h"

//! = see restore_main()

const defaults_t defaults[] = {
	{ "restore_defaults",	"0"					},	// Set to 0 to not restore defaults on boot

	// LAN H/W parameters
//!	{ "lan_ifname",			""				},	// LAN interface name
//!	{ "lan_ifnames",		""				},	// Enslaved LAN interfaces
	{ "lan_hwnames",		""				},	// LAN driver names (e.g. et0)
	{ "lan_hwaddr",			""				},	// LAN interface MAC address

	// LAN TCP/IP parameters
	{ "lan_dhcp",			"0"				},	// DHCP client [static|dhcp]
//	{ "lan_proto",			"dhcp"				},	// DHCP server [static|dhcp]  //start with no dhcp if nvram corrupted
	{ "lan_ipaddr",			"192.168.1.1"			},	// LAN IP address
	{ "lan_netmask",		"255.255.255.0"			},	// LAN netmask
	{ "lan_wins",			""				},	// x.x.x.x x.x.x.x ...
	{ "lan_domain",			""				},	// LAN domain name
	{ "lan_lease",			"86400"				},	// LAN lease time in seconds
	{ "lan_stp",			"0"				},	// LAN spanning tree protocol
	{ "lan_route",			""				},	// Static routes (ipaddr:netmask:gateway:metric:ifname ...)

	{ "lan_gateway",		"0.0.0.0"			},	// LAN Gateway
	{ "wl_wds_enable",		"0"				},	// WDS Enable (0|1)

#ifdef TCONFIG_VLAN
	{ "lan1_ipaddr",		""				},
	{ "lan1_netmask",		""				},
	{ "lan1_stp",			"0"				},
	{ "lan2_ipaddr",		""				},
	{ "lan2_netmask",		""				},
	{ "lan2_stp",			"0"				},
	{ "lan3_ipaddr",		""				},
	{ "lan3_netmask",		""				},
	{ "lan3_stp",			"0"				},
#endif

	// WAN H/W parameters
//!	{ "wan_ifname",			""				},	// WAN interface name
//!	{ "wan_ifnames",		""				},	// WAN interface names
	{ "wan_hwname",			""				},	// WAN driver name (e.g. et1)
	{ "wan_hwaddr",			""				},	// WAN interface MAC address
	{ "wan_ifnameX",		NULL				},	// real wan if; see wan.c:start_wan

	// WAN TCP/IP parameters
	{ "wan_proto",			"dhcp"				},	// [static|dhcp|pppoe|disabled]
	{ "wan_ipaddr",			"0.0.0.0"			},	// WAN IP address
	{ "wan_netmask",		"0.0.0.0"			},	// WAN netmask
	{ "wan_gateway",		"0.0.0.0"			},	// WAN gateway
	{ "wan_gateway_get",		"0.0.0.0"			},	// default gateway for PPP
	{ "wan_dns",			""				},	// x.x.x.x x.x.x.x ...
	{ "wan_wins",			""				},	// x.x.x.x x.x.x.x ...
	{ "wan_lease",			"86400"				},	// WAN lease time in seconds
	{ "wan_islan",			"0"				},
	{ "modem_ipaddr",		"0.0.0.0"		},	// modem IP address (i.e. PPPoE bridged modem)

	{ "wan_primary",		"1"				},	// Primary wan connection
	{ "wan_unit",			"0"				},	// Last configured connection
/* --- obsolete ---
	// Filters
	{ "filter_maclist",		""				},	// xx:xx:xx:xx:xx:xx ...
	{ "filter_macmode",		"deny"				},	// "allow" only, "deny" only, or "disabled" (allow all)
	{ "filter_client0",		""				},	// [lan_ipaddr0-lan_ipaddr1|*]:lan_port0-lan_port1,proto,enable,day_start-day_end,sec_start-sec_end,desc

	{ "filter",			"on"				},	// [on | off] Firewall Protection

	// Port forwards
	{ "autofw_port0",		""				},	// out_proto:out_port,in_proto:in_port0-in_port1>to_port0-to_port1,enable,desc
*/
	// DHCP server parameters
	{ "dhcp_start",			"100"				},	//
	{ "dhcp_num",			"50"				},	//
	{ "dhcpd_startip",		"" 				},	// if empty, tomato will use dhcp_start/dchp_num for better compatibility
	{ "dhcpd_endip",		"" 				},	// "
	{ "dhcp_lease",			"0"				},	// LAN lease time in minutes
	{ "dhcp_domain",		"wan"				},	// Use WAN domain name first if available (wan|lan)
	{ "wan_get_dns",		""				},	// DNS IP address which get by dhcpc // Add
	{ "wan_routes",			""				},
	{ "wan_msroutes",		""				},

#ifdef TCONFIG_VLAN
	{ "dhcp1_start",			""			},
	{ "dhcp1_num",			""			},
	{ "dhcpd1_startip",		"" 				},
	{ "dhcpd1_endip",		"" 				},
	{ "dhcp1_lease",			"0"				},
	{ "dhcp2_start",			""			},
	{ "dhcp2_num",			""			},
	{ "dhcpd2_startip",		"" 				},
	{ "dhcpd2_endip",		"" 				},
	{ "dhcp2_lease",			"0"				},
	{ "dhcp3_start",			""			},
	{ "dhcp3_num",			""			},
	{ "dhcpd3_startip",		"" 				},
	{ "dhcpd3_endip",		"" 				},
	{ "dhcp3_lease",			"0"				},
#endif

	// PPPoE parameters
	{ "pppoe_ifname",		""				},	// PPPoE enslaved interface
	{ "ppp_username",		""				},	// PPP username
	{ "ppp_passwd",			""				},	// PPP password
	{ "ppp_idletime",		"5"				},	// Dial on demand max idle time (mins)
	{ "ppp_keepalive",		"0"				},	// Restore link automatically
	{ "ppp_demand",			"0"				},	// Dial on demand
	{ "ppp_redialperiod",		"30"				},	// Redial Period  (seconds)*/
	{ "ppp_mru",			"1500"				},	// Negotiate MRU to this value
	{ "ppp_mtu",			"1500"				},	// Negotiate MTU to the smaller of this value or the peer MRU
	{ "ppp_service",		""				},	// PPPoE service name
	{ "ppp_ac",				""			},	// PPPoE access concentrator name
	{ "ppp_static",			"0"				},	// Enable / Disable Static IP
	{ "ppp_static_ip",		""				},	// PPPoE Static IP
	{ "ppp_get_ac",			""				},	// PPPoE Server ac name
	{ "ppp_get_srv",		""				},	// PPPoE Server service name
	{ "ppp_custom",			""				},	// PPPD additional options
	{ "ppp_mlppp",			"0"				},	// PPPoE single line MLPPP

	{ "pppoe_lei",			""				},
	{ "pppoe_lef",			""				},

#ifdef TCONFIG_IPV6
	// IPv6 parameters
	{ "ipv6_service",		""				},	// [''|native|native-pd|6to4|sit|other]
	{ "ipv6_prefix",		""				},	// The global-scope IPv6 prefix to route/advertise
	{ "ipv6_prefix_length",		"64"				},	// The bit length of the prefix. Used by dhcp6c. For radvd, /64 is always assumed.
	{ "ipv6_rtr_addr",		""				},	// defaults to $ipv6_prefix::1
	{ "ipv6_radvd",			"1"				},	// Enable Router Advertisement (radvd)
	{ "ipv6_accept_ra",		"0"				},	// Accept RA on WAN and/or LAN interfaces
	{ "ipv6_ifname",		"six0"				},	// The interface facing the rest of the IPv6 world
	{ "ipv6_tun_v4end",		"0.0.0.0"			},	// Foreign IPv4 endpoint of SIT tunnel
	{ "ipv6_relay",			"1"				},	// Foreign IPv4 endpoint host of SIT tunnel 192.88.99.?
	{ "ipv6_tun_addr",		""				},	// IPv6 address to assign to local tunnel endpoint
	{ "ipv6_tun_addrlen",		"64"				},	// CIDR prefix length for tunnel's IPv6 address	
	{ "ipv6_tun_mtu",		"0"				},	// Tunnel MTU, 0 for default
	{ "ipv6_tun_ttl",		"255"				},	// Tunnel TTL
	{ "ipv6_dns",			""				},	// DNS server(s) IPs
	{ "ipv6_get_dns",		""				},	// DNS IP address which get by dhcp6c
#endif

	// Wireless parameters
	{ "wl_ifname",			""				},	// Interface name
	{ "wl_hwaddr",			""				},	// MAC address
	{ "wl_phytype",			"b"				},	// Current wireless band ("a" (5 GHz), "b" (2.4 GHz), or "g" (2.4 GHz))	// Modify
	{ "wl_corerev",			""				},	// Current core revision
	{ "wl_phytypes",		""				},	// List of supported wireless bands (e.g. "ga")
	{ "wl_radioids",		""				},	// List of radio IDs
	{ "wl_ssid",			"wireless"			},	// Service set ID (network name)
	{ "wl1_ssid",			"wireless1"			},
	{ "wl_country_code",		"US"				},		// Country (default obtained from driver)
	{ "wl_radio",			"1"				},	// Enable (1) or disable (0) radio
	{ "wl1_radio",			"1"				},	// Enable (1) or disable (0) radio
	{ "wl_closed",			"0"				},	// Closed (hidden) network
	{ "wl_ap_isolate",		"0"				},	// AP isolate mode
	{ "wl_mode",			"ap"				},	// AP mode (ap|sta|wds)
	{ "wl_lazywds",			"1"				},	// Enable "lazy" WDS mode (0|1)
	{ "wl_wds",			""				},	// xx:xx:xx:xx:xx:xx ...
	{ "wl_wds_timeout",		"1"				},	// WDS link detection interval defualt 1 sec*/
	{ "wl_wep",				"disabled"		},	// WEP data encryption (enabled|disabled)
	{ "wl_auth",			"0"				},	// Shared key authentication optional (0) or required (1)
	{ "wl_key",			"1"				},	// Current WEP key
	{ "wl_key1",			""				},	// 5/13 char ASCII or 10/26 char hex
	{ "wl_key2",			""				},	// 5/13 char ASCII or 10/26 char hex
	{ "wl_key3",			""				},	// 5/13 char ASCII or 10/26 char hex
	{ "wl_key4",			""				},	// 5/13 char ASCII or 10/26 char hex
	{ "wl_channel",			"1"				},	// Channel number
	{ "wl1_channel",		"0"				},
	{ "wl_rate",			"0"				},	// Rate (bps, 0 for auto)
	{ "wl_mrate",			"0"				},	// Mcast Rate (bps, 0 for auto)
	{ "wl_rateset",			"default"			},	// "default" or "all" or "12"
	{ "wl_frag",			"2346"				},	// Fragmentation threshold
	{ "wl_rts",				"2347"			},	// RTS threshold
	{ "wl_dtim",			"1"				},	// DTIM period (3.11.5)*/	// It is best value for WiFi test
	{ "wl_bcn",				"100"			},	// Beacon interval
	{ "wl_plcphdr",			"long"				},	// 802.11b PLCP preamble type
	{ "wl_net_mode",		"mixed"				},	// Wireless mode (mixed|g-only|b-only|disable)
	{ "wl_gmode",			"1"				},	// 54g mode
	{ "wl_gmode_protection",	"off"				},	// 802.11g RTS/CTS protection (off|auto)
	{ "wl_afterburner",		"off"				},	// AfterBurner
	{ "wl_frameburst",		"off"				},	// BRCM Frambursting mode (off|on)
	{ "wl_wme",			"auto"				},	// WME mode (auto|off|on)
	{ "wl1_wme",			"auto"				},	// WME mode (auto|off|on)
	{ "wl_antdiv",			"-1"				},	// Antenna Diversity (-1|0|1|3)
	{ "wl_infra",			"1"				},	// Network Type (BSS/IBSS)
	{ "wl_btc_mode",		"0"				},	// !!TB - BT Coexistence Mode
	{ "wl_sta_retry_time",		"5"				},	// !!TB - Seconds between association attempts (0 to disable retries)
	{ "wl_mitigation",		"0"				},	// Interference Mitigation Mode (0|1|2|3|4)    //Toastman - 0=off
	{ "wl_passphrase",		""				},	// Passphrase	// Add
	{ "wl_wep_bit",			"128"				},	// WEP encryption [64 | 128] // Add
	{ "wl_wep_buf",			""				},	// save all settings for web // Add
	{ "wl_wep_gen",			""				},	// save all settings for generate button	// Add
	{ "wl_wep_last",		""				},	// Save last wl_wep mode	// Add

	// WPA parameters
	{ "wl_security_mode",		"disabled"		},	// WPA mode (disabled|radius|wpa_personal|wpa_enterprise|wep|wpa2_personal|wpa2_enterprise) for WEB	// Add
	{ "wl_auth_mode",		"none"			},	// Network authentication mode (radius|none)
	{ "wl_wpa_psk",			""				},	// WPA pre-shared key
	{ "wl_wpa_gtk_rekey",	"3600"				},	// WPA GTK rekey interval	// Modify
	{ "wl_radius_ipaddr",	""				},	// RADIUS server IP address
	{ "wl_radius_key",		""				},	// RADIUS shared secret
	{ "wl_radius_port",		"1812"			},	// RADIUS server UDP port
	{ "wl_crypto",			"aes"			},	// WPA data encryption
	{ "wl_net_reauth",		"36000"			},	// Network Re-auth/PMK caching duration
	{ "wl_akm",				""				},	// WPA akm list

	// WME parameters (cwmin cwmax aifsn txop_b txop_ag adm_control oldest_first)
	// EDCA parameters for STA
	{ "wl_wme_sta_bk",		"15 1023 7 0 0 off off"		},	// WME STA AC_BK paramters
	{ "wl_wme_sta_be",		"15 1023 3 0 0 off off"		},	// WME STA AC_BE paramters
	{ "wl_wme_sta_vi",		"7 15 2 6016 3008 off off"	},	// WME STA AC_VI paramters
	{ "wl_wme_sta_vo",		"3 7 2 3264 1504 off off"	},	// WME STA AC_VO paramters

	// EDCA parameters for AP
	{ "wl_wme_ap_bk",		"15 1023 7 0 0 off off"		},	// WME AP AC_BK paramters
	{ "wl_wme_ap_be",		"15 63 3 0 0 off off"		},	// WME AP AC_BE paramters
	{ "wl_wme_ap_vi",		"7 15 1 6016 3008 off off"	},	// WME AP AC_VI paramters
	{ "wl_wme_ap_vo",		"3 7 1 3264 1504 off off"	},	// WME AP AC_VO paramters

	{ "wl_wme_no_ack",		"off"			},	// WME No-Acknowledgmen mode
	{ "wl_wme_apsd",		"off"			},	// WME APSD mode
	{ "wl_wme_bss_disable",		"0"			},	// WME BSS disable advertising (off|on)

	/* Per AC Tx parameters */
	{ "wl_wme_txp_be",		"7 3 4 2 0"		},	/* WME AC_BE Tx parameters */
	{ "wl_wme_txp_bk",		"7 3 4 2 0"		},	/* WME AC_BK Tx parameters */
	{ "wl_wme_txp_vi",		"7 3 4 2 0"		},	/* WME AC_VI Tx parameters */
	{ "wl_wme_txp_vo",		"7 3 4 2 0"		},	/* WME AC_VO Tx parameters */

	{ "wl_unit",			"0"				},	// Last configured interface
	{ "wl_mac_deny",		""				},	// filter MAC	// Add

	{ "wl_leddc",			"0x640000"		},	// !!TB - 100% duty cycle for LED on router (WLAN LED fix for some routers)
	{ "wl_bss_enabled",		"1"				},	// !!TB - If not present the new versions of wlconf may not bring up wlan
	{ "wl_reg_mode",		"off"			},	// !!TB - Regulatory: 802.11H(h)/802.11D(d)/off(off)

// !!TB: n-mode
	{ "wl_nmode",			"-1"			},	// N-mode
	{ "wl_nband",			"2"			},	// 2 - 2.4GHz, 1 - 5GHz, 0 - Auto
	{ "wl1_nband",			"1"			},
	{ "wl_nmcsidx",			"-1"			},	// MCS Index for N - rate
	{ "wl_nreqd",			"0"			},	// Require 802.11n support
	{ "wl_nbw",			"40"			},	// BW: 20 / 40 MHz
	{ "wl_nbw_cap",			"1"			},	// BW: def 20inB and 40inA
	{ "wl_mimo_preamble",		"mm"			},	// 802.11n Preamble: mm/gf/auto/gfbcm
	{ "wl_nctrlsb",			"upper"			},	// N-CTRL SB (none/lower/upper)
	{ "wl_nmode_protection",	"off"			},	// 802.11n RTS/CTS protection (off|auto)
	{ "wl_rxstreams",		"0"			},	// 802.11n Rx Streams, 0 is invalid, WLCONF will change it to a radio appropriate default
	{ "wl_txstreams",		"0"			},	// 802.11n Tx Streams 0, 0 is invalid, WLCONF will change it to a radio appropriate default
	{ "wl_dfs_preism",		"60"			},	// 802.11H pre network CAC time
	{ "wl_dfs_postism",		"60"			},	// 802.11H In Service Monitoring CAC time
	{ "wl_radarthrs",		"1 0x6c0 0x6e0 0x6bc 0x6e0 0x6ac 0x6cc 0x6bc 0x6e0" },	// Radar thrs params format: version thresh0_20 thresh1_20 thresh0_40 thresh1_40
	{ "wl_bcn_rotate",		"1"			},	// Beacon rotation
	{ "wl_vlan_prio_mode",		"off"			},	// VLAN Priority support
	{ "wl_obss_coex",		"0"			},	// OBSS Coexistence (0|1): when enabled, channel width is forced to 20MHz

#ifdef TCONFIG_EMF
	{ "emf_entry",			""			},	// Static MFDB entry (mgrp:if)
	{ "emf_uffp_entry",		""			},	// Unreg frames forwarding ports
	{ "emf_rtport_entry",		""			},	// IGMP frames forwarding ports
	{ "emf_enable",			"0"			},	// Disable EMF by default
#endif
#ifdef CONFIG_BCMWL5
	// AMPDU
	{ "wl_ampdu",			"auto"			},	// Default AMPDU setting
	{ "wl_ampdu_rtylimit_tid",	"5 5 5 5 5 5 5 5"	},	// Default AMPDU retry limit per-tid setting
	{ "wl_ampdu_rr_rtylimit_tid",	"2 2 2 2 2 2 2 2"	},	// Default AMPDU regular rate retry limit per-tid setting
	{ "wl_amsdu",			"auto"			},	// Default AMSDU setting
	// power save
	{ "wl_rxchain_pwrsave_enable",	"1"			},	// Rxchain powersave enable
	{ "wl_rxchain_pwrsave_quiet_time","1800"		},	// Quiet time for power save
	{ "wl_rxchain_pwrsave_pps",	"10"			},	// Packets per second threshold for power save
	{ "wl_radio_pwrsave_enable",	"0"			},	// Radio powersave enable
	{ "wl_radio_pwrsave_quiet_time","1800"			},	// Quiet time for power save
	{ "wl_radio_pwrsave_pps",	"10"			},	// Packets per second threshold for power save
	{ "wl_radio_pwrsave_on_time",	"50"			},	// Radio on time for power save
	// misc
	{ "wl_wmf_bss_enable",		"0"			},	// Wireless Multicast Forwarding Enable/Disable
	{ "wl_rifs_advert",		"auto"			},	// RIFS mode advertisement
	{ "wl_stbc_tx",			"auto"			},	// Default STBC TX setting
	{ "wl_mcast_regen_bss_enable",	"1"			},	// MCAST REGEN Enable/Disable
#endif

	{ "pptp_server_ip",		""				},	// as same as WAN gateway
	{ "ppp_get_ip",			""				},	// IP Address assigned by PPTP/L2TP server
	{ "pptp_dhcp",			"1"				},

	// for firewall
	{ "mtu_enable",			"0"				},	// WAN MTU [1|0]
	{ "wan_mtu",			"1500"			},	// Negotiate MTU to the smaller of this value or the peer MRU

	{ "l2tp_server_ip",		""				},	// L2TP auth server (IP Address)
//	hbobs	{ "hb_server_ip",		""				},	// heartbeat auth server (IP Address)
//	hbobs	{ "hb_server_domain",	""				},	// heartbeat auth server (domain name)

// misc
	{ "wl_tnoise",			"-99"			},
	{ "led_override",		""				},
	{ "btn_override",		""				},
	{ "btn_reset",			""				},
	{ "env_path",			""				},
	{ "manual_boot_nv",		"0"				},
//	{ "wlx_hpamp",			""				},
//	{ "wlx_hperx",			""				},	//	see init.c
	{ "t_fix1",				""				},

// basic-ddns
	{ "ddnsx0",				""				},
	{ "ddnsx1",				""				},
	{ "ddnsx0_cache",		""				},
	{ "ddnsx1_cache",		""				},
	{ "ddnsx_save",			"1"				},
	{ "ddnsx_refresh",		"28"			},

// basic-ident
	{ "router_name",		"toast"		},
	{ "wan_hostname",		"unknown"		},
	{ "wan_domain",			""				},

// basic-time
	{ "tm_sel",				"CET-1CEST,M3.5.0/2,M10.5.0/3"	},
	{ "tm_tz",				"CET-1CEST,M3.5.0/2,M10.5.0/3"	},
	{ "tm_dst",				"1",							},
	{ "ntp_updates",		"4"								},
	{ "ntp_tdod",			"0"								},
	{ "ntp_server",			"0.europe.pool.ntp.org 1.europe.pool.ntp.org 2.europe.pool.ntp.org" },
	{ "ntp_kiss",			""								},
	{ "ntp_kiss_ignore",	""								},

// basic-static
	{ "dhcpd_static",		""				},

// basic-wfilter
	{ "wl_maclist",			""			},	// xx:xx:xx:xx:xx:xx ... = 17
	{ "wl_macmode",			"disabled"		},
	{ "macnames",			""			},

// advanced-ctnf
	{ "ct_tcp_timeout",		"0 1800 30 20 20 20 10 20 20 0"				},
	{ "ct_udp_timeout",		"30 180"				},
	{ "ct_timeout",			"10 10"				},
	{ "ct_max",			"8192"				},
	{ "nf_ttl",			"0"				},
	{ "nf_l7in",			"1"				},
#ifdef LINUX26
	{ "nf_sip",			"1"				},
	{ "ct_hashsize",		"2048"				},
#endif
#ifdef LINUX26
	{ "nf_rtsp",			"0"				},
#else
	{ "nf_rtsp",			"1"				},
#endif
	{ "nf_pptp",			"1"				},
	{ "nf_h323",			"1"				},
	{ "nf_ftp",			"1"				},

// advanced-mac
	{ "mac_wan",			""				},
	{ "wl_macaddr",			""				},

// advanced-misc
	{ "boot_wait",			"on"				},
	{ "wait_time",			"5"				},
	{ "clkfreq",			""				},
	{ "wan_speed",			"4"				},	// 0=10 Mb Full, 1=10 Mb Half, 2=100 Mb Full, 3=100 Mb Half, 4=Auto
	{ "jumbo_frame_enable",		"0"				},	// Jumbo Frames support (for RT-N16/WNR3500L)
	{ "jumbo_frame_size",		"2000"				},
#ifdef CONFIG_BCMWL5
	{ "ctf_disable",		"1"				},
#endif

// advanced-dhcpdns
	{ "dhcpd_dmdns",		"1"				},
	{ "dhcpd_slt",			"0"				},
	{ "dhcpd_gwmode",		""				},
	{ "dhcpd_lmax",			""				},
	{ "dns_addget",			"0"				},
	{ "dns_intcpt",			"0"				},
	{ "dhcpc_minpkt",		"1"				},
	{ "dhcpc_custom",		""				},
	{ "dns_norebind",		"1"				},
	{ "dnsmasq_custom",		""				},
	{ "dhcpd_static_only",	"0"				},
//	{ "dnsmasq_norw",		"0"				},

// advanced-firewall
//	{ "block_loopback",		"0"				},	// nat loopback
	{ "nf_loopback",		"0"				},
	{ "block_wan",			"1"				},	// block inbound icmp
	{ "multicast_pass",		"0"				},	// enable multicast proxy
#ifdef TCONFIG_VLAN
	{ "multicast_lan",		"0"				},	// on LAN (br0)
	{ "multicast_lan1",		"0"				},	// on LAN1 (br1)
	{ "multicast_lan2",		"0"				},	// on LAN2 (br2)
	{ "multicast_lan3",		"0"				},	// on LAN3 (br3)
#endif
	{ "ne_syncookies",		"0"				},	// tcp_syncookies
	{ "dhcp_pass",			"1"				},	// allow DHCP responses
	{ "ne_shlimit",			"0,3,60"			},

// advanced-routing
	{ "routes_static",		""				},
	{ "dhcp_routes",		"1"				},
	{ "wk_mode",			"gateway"			},	// Network mode [gateway|router]
#ifdef TCONFIG_ZEBRA
	{ "dr_setting",			"0"				},	// [ Disable | WAN | LAN | Both ]
	{ "dr_lan_tx",			"0"				},	// Dynamic-Routing LAN out
	{ "dr_lan_rx",			"0"				},	// Dynamic-Routing LAN in
#ifdef TCONFIG_VLAN
	{ "dr_lan1_tx",			"0"				},	// Dynamic-Routing LAN out
	{ "dr_lan1_rx",			"0"				},	// Dynamic-Routing LAN in
	{ "dr_lan2_tx",			"0"				},	// Dynamic-Routing LAN out
	{ "dr_lan2_rx",			"0"				},	// Dynamic-Routing LAN in
	{ "dr_lan3_tx",			"0"				},	// Dynamic-Routing LAN out
	{ "dr_lan3_rx",			"0"				},	// Dynamic-Routing LAN in
#endif
	{ "dr_wan_tx",			"0"				},	// Dynamic-Routing WAN out
	{ "dr_wan_rx",			"0"				},	// Dynamic-Routing WAN in
#endif

// advanced-vlan
	{ "trunk_vlan_so",		"0"				},	// VLAN trunk support override

// advanced-wireless
	{ "wl_txant",			"3"				},
	{ "wl_txpwr",			"42"				},
	{ "wl_maxassoc",		"128"				},	// Max associations driver could support
	{ "wl_bss_maxassoc",		"128"				},
	{ "wl_distance",		""				},

// forward-*
	{ "portforward",		"0<3<1.1.1.0/24<1000:2000<<192.168.1.2<ex: 1000 to 2000, restricted>0<2<<1000,2000<<192.168.1.2<ex: 1000 and 2000>0<1<<1000<2000<192.168.1.2<ex: different internal port>0<3<<1000:2000,3000<<192.168.1.2<ex: 1000 to 2000, and 3000>" },
#ifdef TCONFIG_IPV6
	{ "ipv6_portforward",	""},
#endif
	{ "trigforward",		"0<1<3000:4000<5000:6000<ex: open 5000-6000 if 3000-4000>"	},
	{ "dmz_enable",			"0"				},
	{ "dmz_ipaddr",			"0"				},
	{ "dmz_sip",			""				},

// forward-upnp
	{ "upnp_enable",		"3"				},
	{ "upnp_secure",		"1"				},
	{ "upnp_port",			"0"				},
	{ "upnp_ssdp_interval",		"60"				},	// SSDP interval
	{ "upnp_max_age",		"180"				},	// Max age
	{ "upnp_mnp",			"0"				},
	{ "upnp_clean",			"1"				},	/* 0:Disable 1:Enable */
	{ "upnp_clean_interval",	"600"				},	/* Cleaning interval in seconds */
	{ "upnp_clean_threshold",	"20"				},	/* Threshold for cleaning unused rules */
#if 0	// disabled for miniupnpd
	{ "upnp_max_age",		"180"				},	// Max age
	{ "upnp_config",		"0"				},
#endif

// qos
	{ "qos_enable",			"0"				},
	{ "qos_ack",			"0"				},
	{ "qos_syn",			"1"				},
	{ "qos_fin",			"1"				},
	{ "qos_rst",			"1"				},
	{ "qos_icmp",			"1"				},
	{ "qos_reset",			"1"				},
	{ "qos_obw",			"700"				},
	{ "qos_ibw",			"16000"				},
	{ "qos_orules",			"0<<-1<d<53<0<<0:10<<0<DNS>0<<-1<d<37<0<<0:10<<0<Time>0<<-1<d<123<0<<0:10<<0<Network Time (NTP)>0<<-1<d<3455<0<<0:10<<0<RSVP>0<<-1<x<9<0<<<<0<SCTP, Discard>0<<-1<x<135,2101,2103,2105<0<<<<0<RPC (Microsoft)>0<<6<x<23,992<0<<<<0<Telnet>0<<-1<d<22<0<<<<3<SSH>0<<17<x<3544<0<<<<3<Teredo port>0<<6<s<80,8080<0<<<<3<Remote Router Access>0<<6<x<3389<0<<<<3<Remote Assistance>0<<-1<a<<0<flash<<<2<Flash Video, (Youtube)>0<<-1<a<<0<httpvideo<<<2<HTTP Video, (Youtube)>0<<-1<a<<0<shoutcast<<<2<Shoutcast>0<<-1<s<6970:7170,8554<0<<<<2<Quicktime/RealAudio>0<<-1<d<1220,7070<0<<<<2<Quicktime/RealAudio>0<<6<x<6005<0<<<<2<Camfrog>0<<-1<d<1220,1234,5100,6005,6970<0<<<<-1<VLC>0<<-1<x<554,5004,5005<0<<<<2<RTP/RTSP>0<<-1<x<1755<0<<<<2<MMS (Microsoft)>0<<-1<x<1935<0<<<<2<RTMP>0<<-1<d<3478,3479,5060:5063<0<<<<1<SIP, Sipgate Stun Services>0<<-1<d<1718:1720<0<<<<1<H323>0<<-1<a<<0<skypetoskype<<<1<Skype>0<<-1<a<<0<skypeout<<<1<Skypeout>0<<-1<d<80<0<<0:512<<4<HTTP>0<<-1<d<443<0<<0:512<<4<HTTPS>0<<6<d<8080<0<<0:512<<4<HTTP Proxy / Alternate>0<<-1<d<25,587,465<0<<<<5<SMTP, Submission>0<<-1<d<110,995<0<<<<5<POP3 Mail>0<<-1<d<119,563<0<<<<5<NNTP>0<<-1<d<143,220,585,993<0<<<<5<IMAP Mail>0<<-1<a<<0<irc<<<6<IRC>0<<-1<d<1493,1502,1503,1542,1863,1963,3389,5061,5190:5193,7001<0<<<<6<Windows Live>0<<-1<d<1071:1074,1455,1638,1644,5000:5010,5050,5100,5101,5150,8000:8002<0<<<<6<Yahoo Messenger>0<<-1<d<194,1720,1730:1732,5220:5223,5298,6660:6669,22555<0<<<<6<Other Chat Services>0<<6<d<20,21,989,990<0<<<<7<FTP>0<<-1<x<6571,6891:6901<0<<<<7<WLM File/Webcam>0<<6<d<80,443,8080<0<<512:<<7<HTTP,SSL File Transfers>0<<17<x<1:65535<0<<<<-1<P2P (uTP, UDP)" },
	{ "qos_burst0",			""				},
	{ "qos_burst1",			""				},
	{ "qos_default",		"8"				},
	{ "qos_orates",			"5-20,5-20,5-25,5-70,20-100,5-80,5-80,5-80,5-50,0-0"	},
	{ "qos_irates",			"10,60,60,70,0,60,60,80,30,1"	},
	{ "qos_classnames",		"Service VOIP/Game Media Remote WWW Mail Messenger Download P2P/Bulk Crawl"	},

	{ "ne_vegas",			"0"				},	// TCP Vegas
	{ "ne_valpha",			"3"				},	// "
	{ "ne_vbeta",			"3"				},	// "
	{ "ne_vgamma",			"2"				},	// "

// qos-bw-limiter
	{ "qosl_enable",		"0"			},
//	{ "qosl_obw",			""			},	unused - used qos_obw
//	{ "qosl_ibw",			""			},	unused - used qos_obw
	{ "qosl_rules",			"" 			},
	{ "qosl_denable",		"0" 			},
	{ "qosl_dtcp",			"0" 			},//unlimited
	{ "qosl_dudp",			"0" 			},//unlimited
	{ "qosl_ddlc",			"" 			},
	{ "qosl_dulc",			"" 			},
	{ "qosl_ddlr",			"" 			},
	{ "qosl_dulr",			"" 			},

// access restrictions
	{ "rruleN",				"0"				},
	{ "rrule0",				"0|1320|300|31|||word text\n^begins-with.domain.\n.ends-with.net$\n^www.exact-domain.net$|0|example" },
//*	{ "rrule##",			""				},
	{ "rrulewp",			"80,8080"			},

#if TOMATO_SL
// samba
	{ "smbd_on",			"0"				},
	{ "nmbd_on",			"0"				},
	{ "smbd_wgroup",		"WORKGROUP"		},
	{ "smbd_nbname",		"TOMATO"		},
	{ "smbd_adminpass",		"admin"			},
#endif

// admin-access
	{ "http_username",		""			},	// Username
	{ "http_passwd",		"admin"			},	// Password
	{ "remote_management",	"0"				},	// Remote Management [1|0]
	{ "remote_mgt_https",	"0"				},	// Remote Management use https [1|0]
	{ "http_wanport",		"8080"			},	// WAN port to listen on
	{ "http_lanport",		"80"			},	// LAN port to listen on
	{ "https_lanport",		"443"			},	// LAN port to listen on
	{ "http_enable",		"1"				},	// HTTP server enable/disable
	{ "https_enable",		"0"				},	// HTTPS server enable/disable
	{ "https_crt_save",		"0"				},
	{ "https_crt_cn",		""				},
	{ "https_crt_file",		""				},
	{ "https_crt",			""				},
	{ "web_wl_filter",		"0"				},	// Allow/Deny Wireless Access Web
	{ "web_favicon",		"0"				},
	{ "web_css",			"brownlight"			},
	{ "web_svg",			"1"				},
	{ "telnetd_eas",		"1"				},
	{ "telnetd_port",		"23"				},
	{ "sshd_eas",			"0"				},
	{ "sshd_pass",			"1"				},
	{ "sshd_port",			"22"				},
	{ "sshd_remote",		"0"				},
	{ "sshd_rport",			"2222"				},
	{ "sshd_authkeys",		""				},
	{ "sshd_hostkey",		""				},
	{ "sshd_dsskey",		""				},
	{ "sshd_forwarding",		"1"				},
	{ "rmgt_sip",			""				},	// remote management: source ip address

	{ "http_id",			""				},
	{ "web_mx",			"status,bwm"			},
	{ "web_pb",			""				},

// admin-bwm
	{ "rstats_enable",		"1"				},
	{ "rstats_path",		""				},
	{ "rstats_stime",		"48"				},
	{ "rstats_offset",		"1"				},
	{ "rstats_data",		""				},
	{ "rstats_colors",		""				},
	{ "rstats_exclude",		""				},
	{ "rstats_sshut",		"1"				},
	{ "rstats_bak",			"0"				},

// admin-ipt
	{ "cstats_enable",		"1"				},
	{ "cstats_path",		""				},
	{ "cstats_stime",		"48"			},
	{ "cstats_offset",		"1"				},
	{ "cstats_labels",		"0"				},
	{ "cstats_exclude",		""				},
	{ "cstats_include",		""				},
	{ "cstats_all",			"1"				},
	{ "cstats_sshut",		"1"				},
	{ "cstats_bak",			"0"				},

// advanced-buttons
	{ "sesx_led",			"0"				},
	{ "sesx_b0",			"1"				},
	{ "sesx_b1",			"4"				},
	{ "sesx_b2",			"4"				},
	{ "sesx_b3",			"4"				},
	{ "sesx_script",
		"[ $1 -ge 20 ] && telnetd -p 233 -l /bin/sh\n"
	},
	{ "script_brau",
		"if [ ! -e /tmp/switch-start ]; then\n"
		"  # do something at startup\n"
		"  echo position at startup was $1 >/tmp/switch-start\n"
		"  exit\n"
		"fi\n"
		"if [ $1 = \"bridge\" ]; then\n"
		"  # do something\n"
		"  led bridge on\n"
		"elif [ $1 = \"auto\" ]; then\n"
		"  # do something\n"
		"  led bridge off\n"
		"fi\n"
	},

// admin-log
	{ "log_remote",			"0"				},
	{ "log_remoteip",		""				},
	{ "log_remoteport",		"514"				},
	{ "log_file",			"1"				},
	{ "log_file_custom",		"0"				},
	{ "log_file_path",		"/var/log/messages"		},
	{ "log_limit",			"60"				},
	{ "log_in",			"0"				},
	{ "log_out",			"0"				},
	{ "log_mark",			"60"				},
	{ "log_events",			""				},

// admin-log-webmonitor
	{ "log_wm",			"0"				},
	{ "log_wmtype",			"0"				},
	{ "log_wmip",			""				},
	{ "log_wmdmax",			"300"				},
	{ "log_wmsmax",			"300"				},

// admin-debugging
	{ "debug_nocommit",		"0"				},
	{ "debug_cprintf",		"0"				},
	{ "debug_cprintf_file",		"0"				},
//	{ "debug_keepfiles",		"0"				},
	{ "console_loglevel",		"1"				},
	{ "t_cafree",			"1"				},
	{ "t_hidelr",			"0"				},
	{ "debug_clkfix",		"1"				},
	{ "debug_ddns",			"0"				},

// admin-cifs
	{ "cifs1",			""				},
	{ "cifs2",			""				},

// admin-jffs2
	{ "jffs2_on",			"0"				},
	{ "jffs2_exec",			""				},

#ifdef TCONFIG_USB
// nas-usb - !!TB
	{ "usb_enable",			"0"				},
	{ "usb_uhci",			"0"				},
	{ "usb_ohci",			"0"				},
	{ "usb_usb2",			"1"				},
#if defined(LINUX26) && defined(TCONFIG_USB_EXTRAS)
	{ "usb_mmc",			"-1"				},
#endif
	{ "usb_irq_thresh",		"0"				},
	{ "usb_storage",		"1"				},
	{ "usb_printer",		"1"				},
	{ "usb_printer_bidirect",	"0"				},
	{ "usb_ext_opt",		""				},
	{ "usb_fat_opt",		""				},
	{ "usb_ntfs_opt",		""				},
	{ "usb_fs_ext3",		"1"				},
	{ "usb_fs_fat",			"1"				},
#ifdef TCONFIG_NTFS
	{ "usb_fs_ntfs",		"1"				},
#endif
	{ "usb_fs_hfs",			"0"				}, //!Victek
	{ "usb_fs_hfsplus",		"0"				}, //!Victek
	{ "usb_automount",		"1"				},
#if 0
	{ "usb_bdflush",		"30 500 0 0 100 100 60 0 0"	},
#endif
	{ "script_usbhotplug",		""				},
	{ "script_usbmount",		""				},
	{ "script_usbumount",		""				},
	{ "idle_enable",		"0"				},
#endif

#ifdef TCONFIG_FTP
// nas-ftp - !!TB
	{ "ftp_enable",			"0"				},
	{ "ftp_super",			"0"				},
	{ "ftp_anonymous",		"0"				},
	{ "ftp_dirlist",		"0"				},
	{ "ftp_port",			"21"				},
	{ "ftp_max",			"0"				},
	{ "ftp_ipmax",			"0"				},
	{ "ftp_staytimeout",		"300"				},
	{ "ftp_rate",			"0"				},
	{ "ftp_anonrate",		"0"				},
	{ "ftp_anonroot",		""				},
	{ "ftp_pubroot",		""				},
	{ "ftp_pvtroot",		""				},
	{ "ftp_users",			""				},
	{ "ftp_custom",			""				},
	{ "ftp_sip",			""				},	// wan ftp access: source ip address(es)
	{ "ftp_limit",			"0,3,60"			},
	{ "log_ftp",			"0"				},
#endif

#ifdef TCONFIG_SNMP
	{ "snmp_enable",		"0"				},
	{ "snmp_location",		"router"			},
	{ "snmp_contact",		"admin@tomato"			},
	{ "snmp_ro",			"rocommunity"			},
#endif

#ifdef TCONFIG_SAMBASRV
// nas-samba - !!TB
	{ "smbd_enable",		"0"				},
	{ "smbd_wgroup",		"WORKGROUP"			},
	{ "smbd_master",		"1"				},
	{ "smbd_wins",			"1"				},
	{ "smbd_cpage",			""				},
	{ "smbd_cset",			"utf8"				},
	{ "smbd_custom",		""				},
	{ "smbd_autoshare",		"2"				},
	{ "smbd_shares",
		"jffs</jffs<JFFS<1<0>root$</<Hidden Root<0<1"
	},
	{ "smbd_user",			"nas"				},
	{ "smbd_passwd",		""				},
#endif

#ifdef TCONFIG_MEDIA_SERVER
// nas-media
	{ "ms_enable",			"0"				},	/* 0:Disable 1:Enable 2:Enable&Rescan */
	{ "ms_dirs",			"/mnt<"				},
	{ "ms_port",			"0"				},
	{ "ms_dbdir",			""				},
	{ "ms_tivo",			"0"				},
	{ "ms_stdlna",			"0"				},
	{ "ms_sas",			"0"				},
#endif

// admin-sch
	{ "sch_rboot",			""				},
	{ "sch_rcon",			""				},
	{ "sch_c1",				""	},
	{ "sch_c2",				""				},
	{ "sch_c3",				""				},
	{ "sch_c1_cmd",			"" },
	{ "sch_c2_cmd",			""				},
	{ "sch_c3_cmd",			""				},

// admin-script
	{ "script_init",		""				},
	{ "script_shut",		""				},
	{ "script_fire",		"#Restrict number of TCP connections per user #iptables -t nat -I PREROUTING -p tcp --syn -m iprange --src-range 192.168.1.50-192.168.1.250 -m connlimit --connlimit-above 100 -j DROP  #Restrict number of non-TCP connections per user #iptables -t nat -I PREROUTING -p ! tcp -m iprange --src-range 192.168.1.50-192.168.1.250 -m connlimit --connlimit-above 50 -j DROP  #Restrict number of simltaneous SMTP connections (from mailer viruses) #iptables -t nat -I PREROUTING -p tcp --dport 25 -m connlimit --connlimit-above 5 -j DROP"				},
	{ "script_wanup",		""				},


#ifdef TCONFIG_OPENVPN
// vpn
	{ "vpn_debug",            "0"             },
	{ "vpn_server_eas",       ""              },
	{ "vpn_server_dns",       ""              },
	{ "vpn_server1_poll",     "0"             },
	{ "vpn_server1_if",       "tun"           },
	{ "vpn_server1_proto",    "udp"           },
	{ "vpn_server1_port",     "1194"          },
	{ "vpn_server1_firewall", "auto"          },
	{ "vpn_server1_crypt",    "tls"           },
	{ "vpn_server1_comp",     "adaptive"      },
	{ "vpn_server1_cipher",   "default"       },
	{ "vpn_server1_dhcp",     "1"             },
	{ "vpn_server1_r1",       "192.168.1.50"  },
	{ "vpn_server1_r2",       "192.168.1.55"  },
	{ "vpn_server1_sn",       "10.8.0.0"      },
	{ "vpn_server1_nm",       "255.255.255.0" },
	{ "vpn_server1_local",    "10.8.0.1"      },
	{ "vpn_server1_remote",   "10.8.0.2"      },
	{ "vpn_server1_reneg",    "-1"            },
	{ "vpn_server1_hmac",     "-1"            },
	{ "vpn_server1_plan",     "1"             },
	{ "vpn_server1_ccd",      "0"             },
	{ "vpn_server1_c2c",      "0"             },
	{ "vpn_server1_ccd_excl", "0"             },
	{ "vpn_server1_ccd_val",  ""              },
	{ "vpn_server1_pdns",     "0"             },
	{ "vpn_server1_rgw",      "0"             },
	{ "vpn_server1_custom",   ""              },
	{ "vpn_server1_static",   ""              },
	{ "vpn_server1_ca",       ""              },
	{ "vpn_server1_crt",      ""              },
	{ "vpn_server1_key",      ""              },
	{ "vpn_server1_dh",       ""              },
	{ "vpn_server2_poll",     "0"             },
	{ "vpn_server2_if",       "tun"           },
	{ "vpn_server2_proto",    "udp"           },
	{ "vpn_server2_port",     "1194"          },
	{ "vpn_server2_firewall", "auto"          },
	{ "vpn_server2_crypt",    "tls"           },
	{ "vpn_server2_comp",     "adaptive"      },
	{ "vpn_server2_cipher",   "default"       },
	{ "vpn_server2_dhcp",     "1"             },
	{ "vpn_server2_r1",       "192.168.1.50"  },
	{ "vpn_server2_r2",       "192.168.1.55"  },
	{ "vpn_server2_sn",       "10.8.0.0"      },
	{ "vpn_server2_nm",       "255.255.255.0" },
	{ "vpn_server2_local",    "10.8.0.1"      },
	{ "vpn_server2_remote",   "10.8.0.2"      },
	{ "vpn_server2_reneg",    "-1"            },
	{ "vpn_server2_hmac",     "-1"            },
	{ "vpn_server2_plan",     "1"             },
	{ "vpn_server2_ccd",      "0"             },
	{ "vpn_server2_c2c",      "0"             },
	{ "vpn_server2_ccd_excl", "0"             },
	{ "vpn_server2_ccd_val",  ""              },
	{ "vpn_server2_pdns",     "0"             },
	{ "vpn_server2_rgw",      "0"             },
	{ "vpn_server2_custom",   ""              },
	{ "vpn_server2_static",   ""              },
	{ "vpn_server2_ca",       ""              },
	{ "vpn_server2_crt",      ""              },
	{ "vpn_server2_key",      ""              },
	{ "vpn_server2_dh",       ""              },
	{ "vpn_client_eas",       ""              },
	{ "vpn_client1_poll",     "0"             },
	{ "vpn_client1_if",       "tun"           },
	{ "vpn_client1_bridge",   "1"             },
	{ "vpn_client1_nat",      "1"             },
	{ "vpn_client1_proto",    "udp"           },
	{ "vpn_client1_addr",     ""              },
	{ "vpn_client1_port",     "1194"          },
	{ "vpn_client1_retry",    "30"            },
	{ "vpn_client1_rg",       "0"             },
	{ "vpn_client1_firewall", "auto"          },
	{ "vpn_client1_crypt",    "tls"           },
	{ "vpn_client1_comp",     "adaptive"      },
	{ "vpn_client1_cipher",   "default"       },
	{ "vpn_client1_local",    "10.8.0.2"      },
	{ "vpn_client1_remote",   "10.8.0.1"      },
	{ "vpn_client1_nm",       "255.255.255.0" },
	{ "vpn_client1_reneg",    "-1"            },
	{ "vpn_client1_hmac",     "-1"            },
	{ "vpn_client1_adns",     "0"             },
	{ "vpn_client1_rgw",      "0"             },
	{ "vpn_client1_gw",       ""              },
	{ "vpn_client1_custom",   ""              },
	{ "vpn_client1_static",   ""              },
	{ "vpn_client1_ca",       ""              },
	{ "vpn_client1_crt",      ""              },
	{ "vpn_client1_key",      ""              },
	{ "vpn_client2_poll",     "0"             },
	{ "vpn_client2_if",       "tun"           },
	{ "vpn_client2_bridge",   "1"             },
	{ "vpn_client2_nat",      "1"             },
	{ "vpn_client2_proto",    "udp"           },
	{ "vpn_client2_addr",     ""              },
	{ "vpn_client2_port",     "1194"          },
	{ "vpn_client2_retry",    "30"            },
	{ "vpn_client2_rg",       "0"             },
	{ "vpn_client2_firewall", "auto"          },
	{ "vpn_client2_crypt",    "tls"           },
	{ "vpn_client2_comp",     "adaptive"      },
	{ "vpn_client2_cipher",   "default"       },
	{ "vpn_client2_local",    "10.8.0.2"      },
	{ "vpn_client2_remote",   "10.8.0.1"      },
	{ "vpn_client2_nm",       "255.255.255.0" },
	{ "vpn_client2_reneg",    "-1"            },
	{ "vpn_client2_hmac",     "-1"            },
	{ "vpn_client2_adns",     "0"             },
	{ "vpn_client2_rgw",      "0"             },
	{ "vpn_client2_gw",       ""              },
	{ "vpn_client2_custom",   ""              },
	{ "vpn_client2_static",   ""              },
	{ "vpn_client2_ca",       ""              },
	{ "vpn_client2_crt",      ""              },
	{ "vpn_client2_key",      ""              },
#endif	// vpn
#ifdef TCONFIG_PPTP
	{ "pptp_client_enable",   "0"             },
	{ "pptp_client_peerdns",  "0"             },
	{ "pptp_client_mtuenable","0"             },
	{ "pptp_client_mtu",      "1450"          },
	{ "pptp_client_mruenable","0"             },
	{ "pptp_client_mru",      "1450"          },
	{ "pptp_client_nat",      "0"             },
	{ "pptp_client_srvip",    ""              },
	{ "pptp_client_srvsub",   "10.0.0.0"      },
	{ "pptp_client_srvsubmsk","255.0.0.0"     },
	{ "pptp_client_username", ""              },
	{ "pptp_client_passwd",   ""              },
	{ "pptp_client_crypt",    "0"             },
	{ "pptp_client_custom",   ""              },
	{ "pptp_client_dfltroute","0"             },
#endif

#if 0
// safe to remove?
	{ "QoS",					"0"			},

	{ "ses_enable",			"0"				},	// enable ses
	{ "ses_event",			"2"				},	// initial ses event
	{ "ses_led_assertlvl",	"0"				},	// For SES II
	{ "ses_client_join",	"0"				},	// For SES II
	{ "ses_sw_btn_status",	"DEFAULTS"		},	// Barry Adds 20050309 for SW SES BTN
	{ "ses_count",			"0"				},
	{ "eou_configured",		"0"				},

	{ "port_priority_1",		"0"			},	// port 1 priority; 1:high, 0:low
	{ "port_flow_control_1",	"1"			},	// port 1 flow control; 1:enable, 0:disable
	{ "port_rate_limit_1",		"0"			},	// port 1 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M
	{ "port_priority_2",		"0"			},	// port 2 priority; 1:high, 0:low
	{ "port_flow_control_2",	"1"			},	// port 2 flow control; 1:enable, 0:disable
	{ "port_rate_limit_2",		"0" 		},	// port 2 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M
	{ "port_priority_3",		"0"			},	// port 3 priority; 1:high, 0:low
	{ "port_flow_control_3",	"1"			},	// port 3 flow control; 1:enable, 0:disable
	{ "port_rate_limit_3",		"0"			},	// port 3 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M
	{ "port_priority_4",		"0"			},	// port 4 priority; 1:high, 0:low
	{ "port_flow_control_4",	"1"			},	// port 4 flow control; 1:enable, 0:disable
	{ "port_rate_limit_4",		"0"			},	// port 4 rate limit: 256k,512k,1M,2M,5M,10M,20M,50M

//obs	zzz	{ "http_method",		"post"	},	// HTTP method

//	{ "wl_macmode1",		"disabled"		},

/* obsolete
	{ "filter",				"on"	},	// Firewall Protection [on|off]
	{ "ipsec_pass",			"1"	},	// IPSec Pass Through [1|0]
	{ "pptp_pass",			"1"	},	// PPTP Pass Through [1|0]
	{ "l2tp_pass",			"1"	},	// L2TP Pass Through [1|0]
	{ "block_cookie",		"0"	},	// Block Cookie [1|0]
	{ "ident_pass",			"0"	},	// IDENT passthrough [1|0]
	{ "block_proxy",		"0"	},	// Block Proxy [1|0]
*/

/* --- obsolete ---
	{ "forward_port",		""	},	// name:[on|off]:[tcp|udp|both]:wan_port>lan_ipaddr:lan_port0
	{ "port_trigger",		""	},	// name:[on|off]:[tcp|udp|both]:wan_port0-wan_port1>lan_port0-lan_port1

	// for mac clone
	{ "mac_clone_enable",	"0"	},	// User define WAN interface MAC address
	{ "def_hwaddr",	"00:00:00:00:00:00"	},	// User define WAN interface MAC address

	{ "public_ip",			""	},	// public ip
*/

//forced in rc.c	{ "os_name",			""	},	// OS name string
//forced in rc.c	{ "os_version",			EPI_VERSION_STR	},	// OS revision
//forced in rc.c	{ "os_date",			__DATE__	},	// OS date
//not used	{ "ct_modules",			""	},	// CyberTAN kernel modules
//obs	{ "timer_interval",		"3600"	},	// Timer interval in seconds
//obs	{ "ezc_enable",			"1"	},	// Enable EZConfig updates
//obs	{ "ezc_version",		EZC_VERSION_STR	},	// EZConfig version
//obs	{ "is_default",			"1"	},	// is it default setting: 1:yes 0:no*/
//obs	{ "os_server",			""	},	// URL for getting upgrades
//obs	{ "stats_server",		""	},	// URL for posting stats	-- used by httpd/stats.c
//obs	{ "router_disable",		"0"	},	// lan_proto=static lan_stp=0 wan_proto=disabled
//obs	{ "fw_disable",			"0"	},	// Disable firewall (allow new connections from the WAN)
//obs	{ "static_route",		""	},	// Static routes (ipaddr:netmask:gateway:metric:ifname ...)
//obs	{ "static_route_name",	""	},	// Static routes name ($NAME:name)
//	{ "filter_port",		""				},	// [lan_ipaddr|*]:lan_port0-lan_port1
	//{ "dhcp_end",			"150"			},	// Last assignable DHCP address	// Remove
//zzz	not used	{ "dhcp_wins",			"wan"	},	// Use WAN WINS first if available (wan|lan)
	//{ "eou_device_id",	""				},
	//{ "eou_public_key",	""	},
	//{ "eou_private_key",	""	},
	//{ "eou_public",		"b49b5ec6866f5b166cc058110b20551d4fe7a5c96a9b5f01a3929f40015e4248359732b7467bae4948d6bb62f96996a7122c6834311c1ea276b35d12c37895501c0f5bd215499cf443d580b999830ac620ac2bf3b7f912741f54fea17627d13a92f44d014030d5c8d3249df385f500ffc90311563e89aa290e7c6f06ef9a6ec311"	},
	//{ "eou_private",		"1fdf2ed7bd5ef1f4e603d34e4d41f0e70e19d1f65e1b6b1e6828eeed2d6afca354c0543e75d9973a1be9a898fed665e13f713f90bd5f50b3421fa7034fabde1ce63c44d01a5489765dc4dc3486521163bf6288db6c5e99c44bbb0ad7494fef20148ad862662dabcbff8dae7b466fad087d9f4754e9a6c84bc9adcbda7bc22e59"	},
 	{ "eou_expired_hour",	"72"	},     //The expired time is 72 hours, and this value = 72 * 10*/
//	{ "ntp_enable",			"1"	},	// replaced with ntp_updates
//	{ "ntp_mode",			"auto"	},	// auto, manual


	// for AOL
	{ "aol_block_traffic",	"0"				},	// 0:Disable 1:Enable for global
	{ "aol_block_traffic1",	"0"				},	// 0:Disable 1:Enable for "ppp_username"
	{ "aol_block_traffic2",	"0"				},	// 0:Disable 1:Enable for "Parental control"
	{ "skip_amd_check",		"0"				},	// 0:Disable 1:Enable
	{ "skip_intel_check",	"0"				},	// 0:Disable 1:Enable

// advanced-watchdog
	{ "wd_en",				""				},
	{ "wd_atp0",			""				},
	{ "wd_atp1",			""				},
	{ "wd_atp2",			""				},
	{ "wd_atp3",			""				},
	{ "wd_atp4",			""				},
	{ "wd_mxr",				"3"				},
	{ "wd_rdy",				"15"			},
	{ "wd_cki",				"300"			},
	{ "wd_fdm",				""				},
	{ "wd_aof",				""				},

#endif	// 0

// arpbind
	{ "arpbind_enable",			"0"			},
	{ "arpbind_only",			"0"			},
	{ "arpbind_list",			"" 			},

// NoCatSplash. !!Victek
#ifdef TCONFIG_NOCAT
	{ "NC_enable",		"0" 					}, 	// enable NoCatSplash
	{ "NC_Verbosity",	"0"					},	// logging too verbose on startup!
	{ "NC_GatewayName",	"WWW Portal" 				},
        { "NC_GatewayPort",	"5280" 					},
        { "NC_GatewayMode",	"Open" 					},
        { "NC_DocumentRoot",	"/tmp/splashd" 				},
        { "NC_ExcludePorts",	"1863" 					},
        { "NC_HomePage",	"" 					},
        { "NC_ForcedRedirect",	"0" 					},
        { "NC_IdleTimeout",	"0" 					},
        { "NC_MaxMissedARP",	"5" 					},
	{ "NC_PeerChecktimeout", "0"					},
        { "NC_LoginTimeout",	"86400"					},
        { "NC_RenewTimeout",	"0" 					},
        { "NC_AllowedWebHosts",	""					},
#endif
	{ NULL, NULL}

};

const defaults_t if_generic[] = {
	{ "lan_ifname",		"br0"					},
	{ "lan_ifnames",	"eth0 eth2 eth3 eth4"			},
	{ "wan_ifname",		"eth1"					},
	{ "wan_ifnames",	"eth1"					},
	{ NULL, NULL }
};

const defaults_t if_vlan[] = {
	{ "lan_ifname",		"br0"					},
	{ "lan_ifnames",	"vlan0 eth1 eth2 eth3"	},
#ifdef TCONFIG_VLAN
	{ "lan1_ifname",	""					},
	{ "lan1_ifnames",	""					},
	{ "lan2_ifname",	""					},
	{ "lan2_ifnames",	""					},
	{ "lan3_ifname",	""					},
	{ "lan3_ifnames",	""					},
#endif
	{ "wan_ifname",		"vlan1"					},
	{ "wan_ifnames",	"vlan1"					},
	{ NULL, NULL }
};
