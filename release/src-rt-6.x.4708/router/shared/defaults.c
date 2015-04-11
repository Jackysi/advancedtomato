#include <epivers.h>
#include <typedefs.h>
#include <string.h>
#include <ctype.h>
#include <bcmnvram.h>
#include <wlioctl.h>
#include <stdio.h>
#include <shared.h>
//#include <version.h>
#include <shutils.h>

#define XSTR(s) STR(s)
#define STR(s) #s

#include <tomato_config.h>
#include "tomato_profile.h"

//! = see restore_main()

struct nvram_tuple router_defaults[] = {
	{ "restore_defaults",		"0"				, 0 },	// Set to 0 to not restore defaults on boot

	// LAN H/W parameters
	{ "lan_hwnames",		""				, 0 },	// LAN driver names (e.g. et0)
	{ "lan_hwaddr",			""				, 0 },	// LAN interface MAC address

	// LAN TCP/IP parameters
	{ "lan_dhcp",			"0"				, 0 },	// DHCP client [static|dhcp]
	{ "lan_proto",			"dhcp"				, 0 },	// DHCP server [static|dhcp]  //Barry add 2004 09 16
	{ "lan_ipaddr",			"192.168.1.1"			, 0 },	// LAN IP address
	{ "lan_netmask",		"255.255.255.0"			, 0 },	// LAN netmask
	{ "lan_wins",			""				, 0 },	// x.x.x.x x.x.x.x ...
	{ "lan_domain",			""				, 0 },	// LAN domain name
	{ "lan_lease",			"86400"				, 0 },	// LAN lease time in seconds
	{ "lan_stp",			"0"				, 0 },	// LAN spanning tree protocol
	{ "lan_route",			""				, 0 },	// Static routes (ipaddr:netmask:gateway:metric:ifname ...)

	{ "lan_gateway",		"0.0.0.0"			, 0 },	// LAN Gateway
	{ "wl_wds_enable",		"0"				, 0 },	// WDS Enable (0|1)

	{ "lan_state",			"1"				, 0 },	// Show Ethernet LAN ports state (0|1)
	{ "lan_desc",			"1"				, 0 },	// Show Ethernet LAN ports state (0|1)
	{ "lan_invert",			"0"				, 0 },	// Invert Ethernet LAN ports state (0|1)

	{ "lan1_ipaddr",		""				, 0 },
	{ "lan1_netmask",		""				, 0 },
	{ "lan1_stp",			"0"				, 0 },
	{ "lan2_ipaddr",		""				, 0 },
	{ "lan2_netmask",		""				, 0 },
	{ "lan2_stp",			"0"				, 0 },
	{ "lan3_ipaddr",		""				, 0 },
	{ "lan3_netmask",		""				, 0 },
	{ "lan3_stp",			"0"				, 0 },

	// WAN H/W parameters
	{ "wan_hwname",			""				, 0 },	// WAN driver name (e.g. et1)
	{ "wan_hwaddr",			""				, 0 },	// WAN interface MAC address
	{ "wan_ifnameX",		NULL				, 0 },	// real wan if; see wan.c:start_wan

	// WAN TCP/IP parameters
	{ "wan_proto",			"dhcp"				, 0 },	// [static|dhcp|pppoe|disabled]
	{ "wan_ipaddr",			"0.0.0.0"			, 0 },	// WAN IP address
	{ "wan_netmask",		"0.0.0.0"			, 0 },	// WAN netmask
	{ "wan_gateway",		"0.0.0.0"			, 0 },	// WAN gateway
	{ "wan_gateway_get",		"0.0.0.0"			, 0 },	// default gateway for PPP
	{ "wan_dns",			""				, 0 },	// x.x.x.x x.x.x.x ...
#ifdef TCONFIG_DNSSEC
	{ "dnssec_enable",		"0"				, 0 },
#endif
#ifdef TCONFIG_DNSCRYPT
	{ "dnscrypt_proxy",		"0"				, 0 },
	{ "dnscrypt_priority",		"1"				, 0 }, // 0=none, 1=strict-order, 2=no-resolv
	{ "dnscrypt_port",		"40"				, 0 }, // local port
	{ "dnscrypt_resolver",		"opendns"			, 0 }, // default resolver
	{ "dnscrypt_log",		"99"				, 0 }, // log level
	{ "dnscrypt_manual",		"0"				, 0 }, // Set manual resolver
	{ "dnscrypt_provider_name",	""				, 0 }, // Set manual provider name
	{ "dnscrypt_provider_key",	""				, 0 }, // Set manual provider key
	{ "dnscrypt_resolver_address",	""				, 0 }, // Set manual resolver address
#endif
	{ "wan_wins",			""				, 0 },	// x.x.x.x x.x.x.x ...
	{ "wan_lease",			"86400"				, 0 },	// WAN lease time in seconds
	{ "wan_islan",			"0"				, 0 },
	{ "modem_ipaddr",		"0.0.0.0"			, 0 }, // modem IP address (i.e. PPPoE bridged modem)

	{ "wan_primary",		"1"				, 0 },	// Primary wan connection
	{ "wan_unit",			"0"				, 0 },	// Last configured connection

	// DHCP server parameters
	{ "dhcp_start",			"2"				, 0 },	//
	{ "dhcp_num",			"50"				, 0 },	//
	{ "dhcpd_startip",		"" 				, 0 },	// if empty, tomato will use dhcp_start/dchp_num for better compatibility
	{ "dhcpd_endip",		"" 				, 0 },	// "
	{ "dhcp_lease",			"1440"				, 0 },	// LAN lease time in minutes
	{ "dhcp_domain",		"wan"				, 0 },	// Use WAN domain name first if available (wan|lan)
	{ "wan_get_dns",		""				, 0 },	// DNS IP address which get by dhcpc // Add
	{ "wan_routes",			""				, 0 },
	{ "wan_msroutes",		""				, 0 },

	{ "dhcp1_start",		""				, 0 },
	{ "dhcp1_num",			""				, 0 },
	{ "dhcpd1_startip",		"" 				, 0 },
	{ "dhcpd1_endip",		"" 				, 0 },
	{ "dhcp1_lease",		"1440"				, 0 },
	{ "dhcp2_start",		""				, 0 },
	{ "dhcp2_num",			""				, 0 },
	{ "dhcpd2_startip",		"" 				, 0 },
	{ "dhcpd2_endip",		"" 				, 0 },
	{ "dhcp2_lease",		"1440"				, 0 },
	{ "dhcp3_start",		""				, 0 },
	{ "dhcp3_num",			""				, 0 },
	{ "dhcpd3_startip",		"" 				, 0 },
	{ "dhcpd3_endip",		"" 				, 0 },
	{ "dhcp3_lease",		"1440"				, 0 },

	// 3G Modem
	{ "modem_pin",			""				, 0 },
	{ "modem_dev",			"ttyUSB0"			, 0 },
	{ "modem_init",			"*99#"				, 0 },
	{ "modem_apn",			"internet"			, 0 },

	// PPPoE parameters
	{ "pppoe_ifname",		""				, 0 },	// PPPoE enslaved interface
	{ "ppp_username",		""				, 0 },	// PPP username
	{ "ppp_passwd",			""				, 0 },	// PPP password
	{ "ppp_idletime",		"5"				, 0 },	// Dial on demand max idle time (mins)
	{ "ppp_keepalive",		"0"				, 0 },	// Restore link automatically
	{ "ppp_demand",			"0"				, 0 },	// Dial on demand
	{ "ppp_redialperiod",		"10"				, 0 },	// Redial Period  (seconds)*/
	{ "ppp_mru",			"1500"				, 0 },	// Negotiate MRU to this value
	{ "ppp_mtu",			"1500"				, 0 },	// Negotiate MTU to the smaller of this value or the peer MRU
	{ "ppp_service",		""				, 0 },	// PPPoE service name
	{ "ppp_ac",			""				, 0 },	// PPPoE access concentrator name
	{ "ppp_static",			"0"				, 0 },	// Enable / Disable Static IP
	{ "ppp_static_ip",		""				, 0 },	// PPPoE Static IP
	{ "ppp_get_ac",			""				, 0 },	// PPPoE Server ac name
	{ "ppp_get_srv",		""				, 0 },	// PPPoE Server service name
	{ "ppp_custom",			""				, 0 },	// PPPD additional options
	{ "ppp_mlppp",			"0"				, 0 },	// PPPoE single line MLPPP

	{ "pppoe_lei",			"10"				, 0 },
	{ "pppoe_lef",			"5"				, 0 },

#ifdef TCONFIG_IPV6
	// IPv6 parameters
	{ "ipv6_service",		""				, 0 },	// [''|native|native-pd|6to4|sit|other]
	{ "ipv6_prefix",		""				, 0 },	// The global-scope IPv6 prefix to route/advertise
	{ "ipv6_prefix_length",		"64"				, 0 },	// The bit length of the prefix. Used by dhcp6c. For radvd, /64 is always assumed.
	{ "ipv6_rtr_addr",		""				, 0 },	// defaults to $ipv6_prefix::1
	{ "ipv6_radvd",			"1"				, 0 },	// Enable Router Advertisement (radvd)
	{ "ipv6_accept_ra",		"1"				, 0 },	// Accept RA on bit 0WAN and/or bit1LAN interfaces
	{ "ipv6_ifname",		"six0"				, 0 },	// The interface facing the rest of the IPv6 world
	{ "ipv6_tun_v4end",		"0.0.0.0"			, 0 },	// Foreign IPv4 endpoint of SIT tunnel
	{ "ipv6_relay",			"1"				, 0 },	// Foreign IPv4 endpoint host of SIT tunnel 192.88.99.?
	{ "ipv6_tun_addr",		""				, 0 },	// IPv6 address to assign to local tunnel endpoint
	{ "ipv6_tun_addrlen",		"64"				, 0 },	// CIDR prefix length for tunnel's IPv6 address	
	{ "ipv6_tun_mtu",		"0"				, 0 },	// Tunnel MTU, 0 for default
	{ "ipv6_tun_ttl",		"255"				, 0 },	// Tunnel TTL
	{ "ipv6_dns",			""				, 0 },	// DNS server(s) IPs
	{ "ipv6_get_dns",		""				, 0 },	// DNS IP address which get by dhcp6c
	{ "ipv6_6rd_prefix",		"2602:100::"			, 0 },	// 6RD prefix (Charter)
	{ "ipv6_6rd_prefix_length",	"32"				, 0 },	// 6RD prefix length (32-62) checkme
	{ "ipv6_6rd_borderrelay",	"68.113.165.1"			, 0 },	// 6RD border relay address
	{ "ipv6_6rd_ipv4masklen",	"0"				, 0 },	// 6RD IPv4 mask length (0-30) checkme
	{ "ipv6_vlan",			"0"				, 0 },	// Enable IPv6 on 1=LAN1 2=LAN2 4=LAN3
	{ "ipv6_isp_opt",		"0"				, 0 },	// wan.c add eval option for dhcpd
#endif

	// Wireless parameters
	{ "wl_ifname",			""				, 0 },	// Interface name
	{ "wl_hwaddr",			""				, 0 },	// MAC address
	{ "wl_phytype",			"b"				, 0 },	// Current wireless band ("a" (5 GHz), "b" (2.4 GHz), or "g" (2.4 GHz))	// Modify
	{ "wl_corerev",			""				, 0 },	// Current core revision
	{ "wl_phytypes",		""				, 0 },	// List of supported wireless bands (e.g. "ga")
	{ "wl_radioids",		""				, 0 },	// List of radio IDs
	{ "wl_ssid",			"Tomato24"			, 0 },	// Service set ID (network name)
	{ "wl1_ssid",			"Tomato50"			, 0 },
	{ "wl_country_code",		""				, 0 },		// Country (default obtained from driver)
	{ "wl_radio",			"1"				, 0 },	// Enable (1) or disable (0) radio
	{ "wl1_radio",			"1"				, 0 },	// Enable (1) or disable (0) radio
	{ "wl_closed",			"0"				, 0 },	// Closed (hidden) network
	{ "wl_ap_isolate",		"0"				, 0 },	// AP isolate mode
	{ "wl_mode",			"ap"				, 0 },	// AP mode (ap|sta|wds)
	{ "wl_lazywds",			"1"				, 0 },	// Enable "lazy" WDS mode (0|1)
	{ "wl_wds",			""				, 0 },	// xx:xx:xx:xx:xx:xx ...
	{ "wl_wds_timeout",		"1"				, 0 },	// WDS link detection interval defualt 1 sec*/
	{ "wl_wep",			"disabled"			, 0 },	// WEP data encryption (enabled|disabled)
	{ "wl_auth",			"0"				, 0 },	// Shared key authentication optional (0) or required (1)
	{ "wl_key",			"1"				, 0 },	// Current WEP key
	{ "wl_key1",			""				, 0 },	// 5/13 char ASCII or 10/26 char hex
	{ "wl_key2",			""				, 0 },	// 5/13 char ASCII or 10/26 char hex
	{ "wl_key3",			""				, 0 },	// 5/13 char ASCII or 10/26 char hex
	{ "wl_key4",			""				, 0 },	// 5/13 char ASCII or 10/26 char hex
	{ "wl_channel",			"6"				, 0 },	// Channel number
	{ "wl1_channel",		"0"				, 0 },
	{ "wl_rate",			"0"				, 0 },	// Rate (bps, 0 for auto)
	{ "wl_mrate",			"0"				, 0 },	// Mcast Rate (bps, 0 for auto)
	{ "wl_rateset",			"default"			, 0 },	// "default" or "all" or "12"
	{ "wl_frag",			"2346"				, 0 },	// Fragmentation threshold
	{ "wl_rts",			"2347"				, 0 },	// RTS threshold
	{ "wl_dtim",			"1"				, 0 },	// DTIM period (3.11.5)*/	// It is best value for WiFi test
	{ "wl_bcn",			"100"				, 0 },	// Beacon interval
	{ "wl_plcphdr",			"long"				, 0 },	// 802.11b PLCP preamble type
	{ "wl_net_mode",		"mixed"				, 0 },	// Wireless mode (mixed|g-only|b-only|disable)
	{ "wl_gmode",			"1"				, 0 },	// 54g mode
	{ "wl_gmode_protection",	"off"				, 0 },	// 802.11g RTS/CTS protection (off|auto)
	{ "wl_afterburner",		"off"				, 0 },	// AfterBurner
	{ "wl_frameburst",		"off"				, 0 },	// BRCM Frambursting mode (off|on)
	{ "wl_wme",			"auto"				, 0 },	// WME mode (auto|off|on)
	{ "wl1_wme",			"auto"				, 0 },	// WME mode (auto|off|on)
	{ "wl_antdiv",			"-1"				, 0 },	// Antenna Diversity (-1|0|1|3)
	{ "wl_infra",			"1"				, 0 },	// Network Type (BSS/IBSS)
	{ "wl_btc_mode",		"0"				, 0 },	// !!TB - BT Coexistence Mode
	{ "wl_sta_retry_time",		"5"				, 0 },	// !!TB - Seconds between association attempts (0 to disable retries)
	{ "wl_mitigation",		"0"				, 0 },	// Interference Mitigation Mode (0|1|2|3|4)

	{ "wl_passphrase",		""				, 0 },	// Passphrase	// Add
	{ "wl_wep_bit",			"128"				, 0 },	// WEP encryption [64 | 128] // Add
	{ "wl_wep_buf",			""				, 0 },	// save all settings for web // Add
	{ "wl_wep_gen",			""				, 0 },	// save all settings for generate button	// Add
	{ "wl_wep_last",		""				, 0 },	// Save last wl_wep mode	// Add

	{ "wl_vifs",			""				, 0 },	// multiple/virtual BSSIDs

	// WPA parameters
	{ "wl_security_mode",		"disabled"			, 0 },	// WPA mode (disabled|radius|wpa_personal|wpa_enterprise|wep|wpa2_personal|wpa2_enterprise) for WEB	// Add
	{ "wl_auth_mode",		"none"				, 0 },	// Network authentication mode (radius|none)
	{ "wl_wpa_psk",			""				, 0 },	// WPA pre-shared key
	{ "wl_wpa_gtk_rekey",		"3600"				, 0 },	// WPA GTK rekey interval	// Modify
	{ "wl_radius_ipaddr",		""				, 0 },	// RADIUS server IP address
	{ "wl_radius_key",		""				, 0 },	// RADIUS shared secret
	{ "wl_radius_port",		"1812"				, 0 },	// RADIUS server UDP port
	{ "wl_crypto",			"aes"				, 0 },	// WPA data encryption
	{ "wl_net_reauth",		"36000"				, 0 },	// Network Re-auth/PMK caching duration
	{ "wl_akm",			""				, 0 },	// WPA akm list

	// WME parameters (cwmin cwmax aifsn txop_b txop_ag adm_control oldest_first)
	// EDCA parameters for STA
	{ "wl_wme_sta_bk",		"15 1023 7 0 0 off off"		, 0 },	// WME STA AC_BK paramters
	{ "wl_wme_sta_be",		"15 1023 3 0 0 off off"		, 0 },	// WME STA AC_BE paramters
	{ "wl_wme_sta_vi",		"7 15 2 6016 3008 off off"	, 0 },	// WME STA AC_VI paramters
	{ "wl_wme_sta_vo",		"3 7 2 3264 1504 off off"	, 0 },	// WME STA AC_VO paramters

	// EDCA parameters for AP
	{ "wl_wme_ap_bk",		"15 1023 7 0 0 off off"		, 0 },	// WME AP AC_BK paramters
	{ "wl_wme_ap_be",		"15 63 3 0 0 off off"		, 0 },	// WME AP AC_BE paramters
	{ "wl_wme_ap_vi",		"7 15 1 6016 3008 off off"	, 0 },	// WME AP AC_VI paramters
	{ "wl_wme_ap_vo",		"3 7 1 3264 1504 off off"	, 0 },	// WME AP AC_VO paramters

	{ "wl_wme_no_ack",		"off"				, 0 },	// WME No-Acknowledgmen mode
	{ "wl_wme_apsd",		"on"				, 0 },	// WME APSD mode
	{ "wl_wme_bss_disable",		"0"				, 0 },	// WME BSS disable advertising (off|on)

	/* Per AC Tx parameters */
	{ "wl_wme_txp_be",		"7 3 4 2 0"			, 0 },	/* WME AC_BE Tx parameters */
	{ "wl_wme_txp_bk",		"7 3 4 2 0"			, 0 },	/* WME AC_BK Tx parameters */
	{ "wl_wme_txp_vi",		"7 3 4 2 0"			, 0 },	/* WME AC_VI Tx parameters */
	{ "wl_wme_txp_vo",		"7 3 4 2 0"			, 0 },	/* WME AC_VO Tx parameters */

	{ "wl_unit",			"0"				, 0 },	// Last configured interface
	{ "wl_mac_deny",		""				, 0 },	// filter MAC	// Add

	{ "wl_leddc",			"0x640000"			, 0 },	// !!TB - 100% duty cycle for LED on router (WLAN LED fix for some routers)
	{ "wl_bss_enabled",		"1"				, 0 },	// !!TB - If not present the new versions of wlconf may not bring up wlan
	{ "wl_reg_mode",		"off"				, 0 },	// !!TB - Regulatory: 802.11H(h)/802.11D(d)/off(off)

// !!TB: n-mode
	{ "wl_nmode",			"-1"				, 0 },	// N-mode
	{ "wl_nband",			"2"				, 0 },	// 2 - 2.4GHz, 1 - 5GHz, 0 - Auto
	{ "wl1_nband",			"1"				, 0 },
	{ "wl_nmcsidx",			"-1"				, 0 },	// MCS Index for N - rate
	{ "wl_nreqd",			"0"				, 0 },	// Require 802.11n support
	{ "wl_nbw",			"40"				, 0 },	// BW: 20 / 40 MHz
	{ "wl_nbw_cap",			"1"				, 0 },	// BW: def 20inB and 40inA
	{ "wl_mimo_preamble",		"mm"				, 0 },	// 802.11n Preamble: mm/gf/auto/gfbcm
	{ "wl_nctrlsb",			"upper"				, 0 },	// N-CTRL SB (none/lower/upper)
	{ "wl_nmode_protection",	"off"				, 0 },	// 802.11n RTS/CTS protection (off|auto)
	{ "wl_rxstreams",		"0"				, 0 },	// 802.11n Rx Streams, 0 is invalid, WLCONF will change it to a radio appropriate default
	{ "wl_txstreams",		"0"				, 0 },	// 802.11n Tx Streams 0, 0 is invalid, WLCONF will change it to a radio appropriate default
	{ "wl_dfs_preism",		"60"				, 0 },	// 802.11H pre network CAC time
	{ "wl_dfs_postism",		"60"				, 0 },	// 802.11H In Service Monitoring CAC time
	{ "wl_radarthrs",		"1 0x6c0 0x6e0 0x6bc 0x6e0 0x6ac 0x6cc 0x6bc 0x6e0" , 0 },	// Radar thrs params format: version thresh0_20 thresh1_20 thresh0_40 thresh1_40
	{ "wl_bcn_rotate",		"1"				, 0 },	// Beacon rotation
	{ "wl_vlan_prio_mode",		"off"				, 0 },	// VLAN Priority support
	{ "wl_obss_coex",		"0"				, 0 },	// OBSS Coexistence (0|1): when enabled, channel width is forced to 20MHz

#ifdef TCONFIG_EMF
	{ "emf_entry",			""				, 0 },	// Static MFDB entry (mgrp:if)
	{ "emf_uffp_entry",		""				, 0 },	// Unreg frames forwarding ports
	{ "emf_rtport_entry",		""				, 0 },	// IGMP frames forwarding ports
	{ "emf_enable",			"0"				, 0 },	// Disable EMF by default
#endif
#ifdef CONFIG_BCMWL5
	// AMPDU
	{ "wl_ampdu",			"auto"				, 0 },	// Default AMPDU setting
	{ "wl_ampdu_rtylimit_tid",	"5 5 5 5 5 5 5 5"		, 0 },	// Default AMPDU retry limit per-tid setting
	{ "wl_ampdu_rr_rtylimit_tid",	"2 2 2 2 2 2 2 2"		, 0 },	// Default AMPDU regular rate retry limit per-tid setting
	{ "wl_amsdu",			"auto"				, 0 },	// Default AMSDU setting
	// power save
#ifdef TCONFIG_BCMWL6
	{ "wl_bss_opmode_cap_reqd",	"0"				, 0 },
#endif
#ifndef TCONFIG_BCMARM
	{ "wl_rxchain_pwrsave_enable",	"1"				, 0 },	// Rxchain powersave enable
	{ "wl_rxchain_pwrsave_quiet_time","1800"			, 0 },	// Quiet time for power save
	{ "wl_rxchain_pwrsave_pps",	"10"				, 0 },	// Packets per second threshold for power save
	{ "wl_rxchain_pwrsave_stas_assoc_check", "0"			, 0 },	/* STAs associated before powersave */
#endif
	{ "wl_radio_pwrsave_enable",	"0"				, 0 },	// Radio powersave enable
	{ "wl_radio_pwrsave_quiet_time","1800"				, 0 },	// Quiet time for power save
	{ "wl_radio_pwrsave_pps",	"10"				, 0 },	// Packets per second threshold for power save
	{ "wl_radio_pwrsave_on_time",	"50"				, 0 },	// Radio on time for power save
	{ "acs_mode", 			"legacy"			, 0 },	/* Legacy mode if ACS is enabled */
	// misc
	{ "wl_wmf_bss_enable",		"0"				, 0 },	// Wireless Multicast Forwarding Enable/Disable
	{ "wl_rifs_advert",		"auto"				, 0 },	// RIFS mode advertisement
	{ "wl_stbc_tx",			"auto"				, 0 },	// Default STBC TX setting
	{ "wl_mcast_regen_bss_enable",	"1"				, 0 },	// MCAST REGEN Enable/Disable
#endif
#ifdef TCONFIG_BCMWL6
	{ "wl_ack_ratio",		"0"},
	{ "wl_ampdu_mpdu",		"0"},
	{ "wl_ampdu_rts",		"1"},
#ifdef TCONFIG_BCMARM
	{ "wl_turbo_qam",		"1"},
	{ "wl_itxbf",			"1"},
	{ "wl0_itxbf",			"0"},
#endif
#endif

	{ "pptp_server_ip",		""				, 0 },	// as same as WAN gateway
	{ "ppp_get_ip",			""				, 0 },	// IP Address assigned by PPTP/L2TP server
	{ "pptp_dhcp",			"1"				, 0 },

	// for firewall
	{ "mtu_enable",			"0"				, 0 },	// WAN MTU [1|0]
	{ "wan_mtu",			"1500"				, 0 },	// Negotiate MTU to the smaller of this value or the peer MRU

	{ "l2tp_server_ip",		""				, 0 },	// L2TP auth server (IP Address)

// misc
	{ "wl_tnoise",			"-99"				, 0 },
	{ "led_override",		""				, 0 },
	{ "btn_override",		""				, 0 },
	{ "btn_reset",			""				, 0 },
	{ "env_path",			""				, 0 },
	{ "manual_boot_nv",		"0"				, 0 },
	{ "t_fix1",			""				, 0 },

// basic-ddns
	{ "ddnsx0",			""				, 0 },
	{ "ddnsx1",			""				, 0 },
	{ "ddnsx0_cache",		""				, 0 },
	{ "ddnsx1_cache",		""				, 0 },
	{ "ddnsx_save",			"1"				, 0 },
	{ "ddnsx_refresh",		"28"				, 0 },

// basic-ident
	{ "router_name",		"TomatoUSB"			, 0 },
	{ "wan_hostname",		"unknown"			, 0 },
	{ "wan_domain",			""				, 0 },

// basic-time
	{ "tm_sel",			"CET-1CEST,M3.5.0/2,M10.5.0/3"	, 0 },
	{ "tm_tz",			"CET-1CEST,M3.5.0/2,M10.5.0/3"	, 0 },
	{ "tm_dst",			"1"				, 0 },
	{ "ntp_updates",		"4"				, 0 },
	{ "ntp_tdod",			"0"				, 0 },
	{ "ntp_server",			"0.europe.pool.ntp.org 1.europe.pool.ntp.org 2.europe.pool.ntp.org" , 0 },
	{ "ntp_kiss",			""				, 0 },
	{ "ntp_kiss_ignore",		""				, 0 },

// basic-static
	{ "dhcpd_static",		""				, 0 },
	{ "dhcpd_static_only",		"0"				, 0 },
// basic-wfilter
	{ "wl_maclist",			""				, 0 },	// xx:xx:xx:xx:xx:xx ...
	{ "wl_macmode",			"disabled"			, 0 },
	{ "macnames",			""				, 0 },

// advanced-ctnf
	{ "ct_tcp_timeout",		""				, 0 },
	{ "ct_udp_timeout",		""				, 0 },
	{ "ct_timeout",			""				, 0 },
	{ "ct_max",			""				, 0 },
	{ "nf_ttl",			"0"				, 0 },
	{ "nf_l7in",			"1"				, 0 },
#ifdef LINUX26
	{ "nf_sip",			"1"				, 0 },
	{ "ct_hashsize",		"2048"				, 0 },
#endif
#ifdef LINUX26
	{ "nf_rtsp",			"0"				, 0 },
#else
	{ "nf_rtsp",			"1"				, 0 },
#endif
	{ "nf_pptp",			"1"				, 0 },
	{ "nf_h323",			"1"				, 0 },
	{ "nf_ftp",			"1"				, 0 },

// advanced-mac
	{ "mac_wan",			""				, 0 },
	{ "wl_macaddr",			""				, 0 },

// advanced-misc
	{ "boot_wait",			"on"				, 0 },
	{ "wait_time",			"5"				, 0 },
	{ "wan_speed",			"4"				, 0 },	// 0=10 Mb Full, 1=10 Mb Half, 2=100 Mb Full, 3=100 Mb Half, 4=Auto
	{ "jumbo_frame_enable",		"0"				, 0 },	// Jumbo Frames support (for RT-N16/WNR3500L)
	{ "jumbo_frame_size",		"2000"				, 0 },
#ifdef CONFIG_BCMWL5
	{ "ctf_disable",		"1"				, 0 },
#endif
#ifdef TCONFIG_BCMFA
	{ "ctf_fa_mode",		"0"				, 0 },
#endif

// advanced-dhcpdns
	{ "dhcpd_dmdns",		"1"				, 0 },
	{ "dhcpd_slt",			"0"				, 0 },
	{ "dhcpd_gwmode",		""				, 0 },
	{ "dhcpd_lmax",			""				, 0 },
	{ "dns_addget",			"0"				, 0 },
	{ "dns_intcpt",			"0"				, 0 },
	{ "dhcpc_minpkt",		"1"				, 0 },
	{ "dhcpc_custom",		""				, 0 },
	{ "dns_norebind",		"1"				, 0 },
	{ "dnsmasq_custom",		""				, 0 },
	{ "dnsmasq_static_only",	"0"				, 0 },
	{ "dnsmasq_q",			"0"				, 0 }, 	//Bit0=quiet-dhcp, 1=dhcp6, 2=ra

// advanced-firewall
	{ "nf_loopback",		"0"				, 0 },
	{ "block_wan",			"1"				, 0 },	// block inbound icmp
	{ "block_wan_limit",		"1"				, 0 },
	{ "block_wan_limit_icmp",	"1"				, 0 },
	{ "block_wan_limit_tr",		"5"				, 0 },
	{ "multicast_pass",		"0"				, 0 },	// enable multicast proxy
	{ "multicast_lan",		"0"				, 0 },	// on LAN (br0)
	{ "multicast_lan1",		"0"				, 0 },	// on LAN1 (br1)
	{ "multicast_lan2",		"0"				, 0 },	// on LAN2 (br2)
	{ "multicast_lan3",		"0"				, 0 },	// on LAN3 (br3)
	{ "udpxy_enable",		"0"				, 0 },
	{ "udpxy_stats",		"0"				, 0 },
	{ "udpxy_clients",		"3"				, 0 },
	{ "udpxy_port",			"4022"				, 0 },
	{ "ne_syncookies",		"0"				, 0 },	// tcp_syncookies
	{ "DSCP_fix_enable",		"1"				, 0 },	// Comacst DSCP fix
	{ "ne_snat",			"0"				, 0 },	// use SNAT instead of MASQUERADE
	{ "dhcp_pass",			"1"				, 0 },	// allow DHCP responses
	{ "ne_shlimit",			"1,3,60"			, 0 },	//shibby - enable limit connection attempts for sshd

// advanced-routing
	{ "routes_static",		""				, 0 },
	{ "dhcp_routes",		"1"				, 0 },
	{ "wk_mode",			"gateway"			, 0 },	// Network mode [gateway|router]
#ifdef TCONFIG_ZEBRA
	{ "dr_setting",			"0"				, 0 },	// [ Disable | WAN | LAN | Both ]
	{ "dr_lan_tx",			"0"				, 0 },	// Dynamic-Routing LAN out
	{ "dr_lan_rx",			"0"				, 0 },	// Dynamic-Routing LAN in
	{ "dr_lan1_tx",			"0"				, 0 },	// Dynamic-Routing LAN out
	{ "dr_lan1_rx",			"0"				, 0 },	// Dynamic-Routing LAN in
	{ "dr_lan2_tx",			"0"				, 0 },	// Dynamic-Routing LAN out
	{ "dr_lan2_rx",			"0"				, 0 },	// Dynamic-Routing LAN in
	{ "dr_lan3_tx",			"0"				, 0 },	// Dynamic-Routing LAN out
	{ "dr_lan3_rx",			"0"				, 0 },	// Dynamic-Routing LAN in
	{ "dr_wan_tx",			"0"				, 0 },	// Dynamic-Routing WAN out
	{ "dr_wan_rx",			"0"				, 0 },	// Dynamic-Routing WAN in
#endif

// advanced-vlan
	{ "trunk_vlan_so",		"0"				, 0 },	// VLAN trunk support override

// advanced-wireless
	{ "wl_txant",			"3"				, 0 },
	{ "wl_txpwr",			"42"				, 0 },
	{ "wl_maxassoc",		"128"				, 0 },	// Max associations driver could support
	{ "wl_bss_maxassoc",		"128"				, 0 },
	{ "wl_distance",		""				, 0 },

// forward-*
	{ "portforward",		"0<3<1.1.1.0/24<1000:2000<<192.168.1.2<ex: 1000 to 2000, restricted>0<2<<1000,2000<<192.168.1.2<ex: 1000 and 2000>0<1<<1000<2000<192.168.1.2<ex: different internal port>0<3<<1000:2000,3000<<192.168.1.2<ex: 1000 to 2000, and 3000>" , 0 },
#ifdef TCONFIG_IPV6
	{ "ipv6_portforward",	"", 0 },
#endif
	{ "trigforward",		"0<1<3000:4000<5000:6000<ex: open 5000-6000 if 3000-4000>"	, 0 },
	{ "dmz_enable",			"0"				, 0 },
	{ "dmz_ipaddr",			"0"				, 0 },
	{ "dmz_sip",			""				, 0 },
	{ "dmz_ifname",			"br0"				, 0 },
	{ "dmz_ra",			"1"				, 0 },

// forward-upnp
	{ "upnp_enable",		"0"				, 0 },
	{ "upnp_secure",		"1"				, 0 },
	{ "upnp_port",			"0"				, 0 },
	{ "upnp_ssdp_interval",		"60"				, 0 },	// SSDP interval
	{ "upnp_mnp",			"0"				, 0 },
	{ "upnp_custom",		""				, 0 },

	{ "upnp_clean",			"1"				, 0 },	/* 0:Disable 1:Enable */
	{ "upnp_clean_interval",	"600"				, 0 },	/* Cleaning interval in seconds */
	{ "upnp_clean_threshold",	"20"				, 0 },	/* Threshold for cleaning unused rules */
#if 0	// disabled for miniupnpd
	{ "upnp_max_age",		"180"				, 0 },	// Max age
	{ "upnp_config",		"0"				, 0 },
#endif

// qos
	{ "atm_overhead",		"0"				, 0 },
	{ "qos_enable",			"0"				, 0 },
	{ "qos_ack",			"0"				, 0 },
	{ "qos_syn",			"1"				, 0 },
	{ "qos_fin",			"1"				, 0 },
	{ "qos_rst",			"1"				, 0 },
	{ "qos_udp",			"0"				, 0 },
	{ "qos_icmp",			"1"				, 0 },
	{ "qos_reset",			"1"				, 0 },
	{ "qos_obw",			"700"				, 0 },
	{ "qos_ibw",			"16000"				, 0 },
	
{ "qos_orules",			"0<<-1<d<53<0<<0:10<<0<DNS>0<<-1<d<37<0<<0:10<<0<Time>0<<17<d<123<0<<0:10<<0<NTP>0<<-1<d<3455<0<<0:10<<0<RSVP>0<<-1<d<9<0<<0:50<<4<SCTP, Discard>0<<-1<x<135,2101,2103,2105<0<<<<4<RPC (Microsoft)>0<<17<d<3544<0<<<<-1<Teredo Tunnel>0<<6<x<22,2222<0<<<<3<SSH>0<<6<d<23,992<0<<<<3<Telnet>0<<6<s<80,5938,8080,2222<0<<<<3<Remote Access>0<<-1<x<3389<0<<<<3<Remote Assistance>0<<-1<x<6970:7170,8554<0<<<<2<Quicktime/RealAudio>0<<-1<d<1220,7070<0<<<<2<Quicktime/RealAudio>0<<-1<x<554,5004,5005<0<<<<2<RTP, RTSP>0<<-1<x<1755<0<<<<2<MMS (Microsoft)>0<<-1<d<3478,3479,5060:5063<0<<<<1<SIP, Sipgate Stun Services>0<<-1<s<53,88,3074<0<<<<1<Xbox Live>0<<6<d<1718:1720<0<<<<1<H323>0<<-1<d<11031,11235:11335,11999,2300:2400,6073,28800:29100,47624<0<<<<1<Various Games>0<<-1<d<1493,1502,1503,1542,1863,1963,3389,5061,5190:5193,7001<0<<<<6<MSGR1 - Windows Live>0<<-1<d<1071:1074,1455,1638,1644,5000:5010,5050,5100,5101,5150,8000:8002<0<<<<6<MSGR2 - Yahoo>0<<-1<d<194,1720,1730:1732,5220:5223,5298,6660:6669,22555<0<<<<6<MSGR3 - Additional>0<<-1<d<19294:19310<0<<<<6<Google+ & Voice>0<<6<d<6005,6006<0<<<<6<Camfrog>0<<-1<x<6571,6891:6901<0<<<<6<WLM File/Webcam>0<<-1<a<<0<skypetoskype<<<1<Skype to Skype>0<<-1<a<<0<skypeout<<<-1<Skype Phone (deprecated)>0<<-1<a<<0<youtube-2012<<<2<YouTube 2012 (Youtube)>0<<-1<a<<0<flash<<<2<Flash Video (Youtube)>0<<-1<a<<0<httpvideo<<<2<HTTP Video (Youtube)>0<<-1<a<<0<rtp<<<2<RTP>0<<-1<a<<0<rtmp<<<2<RTMP>0<<-1<a<<0<rtmpt<<<2<RTMPT (RTMP over HTTP)>0<<-1<a<<0<shoutcast<<<2<Shoutcast>0<<-1<a<<0<irc<<<6<IRC>0<<6<d<80,443,8080<0<<0:512<<4<HTTP, HTTPS, HTTP Proxy>0<<6<d<80,443,8080<0<<512:<<7<HTTP, SSL File Transfers>0<<6<d<20,21,989,990<0<<<<7<FTP>0<<6<d<119,563<0<<<<7<NNTP News & Downloads>0<<6<d<25,587,465,2525<0<<<<5<SMTP, Submission Mail>0<<6<d<110,995<0<<<<5<POP3 Mail>0<<6<d<143,220,585,993<0<<<<5<IMAP Mail>0<<17<d<1:65535<0<<<<9<P2P (uTP, UDP)" , 0 },

	{ "qos_burst0",			""				, 0 },
	{ "qos_burst1",			""				, 0 },
	{ "qos_default",		"8"				, 0 },
	{ "qos_orates",			"5-100,5-30,5-30,5-100,20-100,5-70,5-70,5-70,5-30,1-5"				, 0 },
	{ "qos_irates",			"5-100,5-30,5-40,5-100,20-90,5-60,5-60,5-60,5-60,1-5"   			, 0 },
	{ "qos_classnames",		"Service VOIP/Game Media Remote WWW Mail Messenger FileXfer P2P/Bulk Crawl"	, 0 },

	{ "ne_vegas",			"0"				, 0 },	// TCP Vegas
	{ "ne_valpha",			"2"				, 0 },	// "
	{ "ne_vbeta",			"6"				, 0 },	// "
	{ "ne_vgamma",			"2"				, 0 },	// "

// access restrictions
	{ "rruleN",			"0"			, 0 },
	{ "rrule0",			"0|1320|300|31|||word text\n^begins-with.domain.\n.ends-with.net$\n^www.exact-domain.net$|0|example" , 0 },
	{ "rrulewp",			"80,8080"		, 0 },

#if TOMATO_SL
// samba
	{ "smbd_on",			"0"				, 0 },
	{ "nmbd_on",			"0"				, 0 },
	{ "smbd_wgroup",		"WORKGROUP"			, 0 },
	{ "smbd_nbname",		"TOMATO"			, 0 },
	{ "smbd_adminpass",		"admin"				, 0 },
#endif

// admin-access
	{ "http_username",		""				, 0 },	// Username
	{ "http_passwd",		"admin"				, 0 },	// Password
	{ "remote_management",	"0"					, 0 },	// Remote Management [1|0]
	{ "remote_mgt_https",	"0"					, 0 },	// Remote Management use https [1|0]
	{ "http_wanport",		"8080"				, 0 },	// WAN port to listen on
	{ "http_lanport",		"80"				, 0 },	// LAN port to listen on
	{ "https_lanport",		"443"				, 0 },	// LAN port to listen on
	{ "http_enable",		"1"				, 0 },	// HTTP server enable/disable
	{ "https_enable",		"0"				, 0 },	// HTTPS server enable/disable
	{ "https_crt_save",		"0"				, 0 },
	{ "https_crt_cn",		""				, 0 },
	{ "https_crt_file",		""				, 0 },
	{ "https_crt",			""				, 0 },
	{ "http_root",			"1"				, 0 },	// 0 - deny, 1 - Allow
	{ "web_wl_filter",		"0"				, 0 },	// Allow/Deny Wireless Access Web
	{ "web_css",			"openlinksys"			, 0 },
	{ "web_dir",			"default"			, 0 },  // jffs, opt, tmp or default (/www)
	{ "ttb_css",			"example"			, 0 },	//Tomato Themes Base
	{ "web_svg",			"1"				, 0 },
	{ "telnetd_eas",		"1"				, 0 },
	{ "telnetd_port",		"23"				, 0 },
	{ "sshd_eas",			"1"				, 0 }, //shibby - enable sshd by default
	{ "sshd_pass",			"1"				, 0 },
	{ "sshd_port",			"22"				, 0 },
	{ "sshd_remote",		"0"				, 0 },
	{ "sshd_motd",			"1"				, 0 },
	{ "sshd_rport",			"22"				, 0 },
	{ "sshd_authkeys",		""				, 0 },
	{ "sshd_hostkey",		""				, 0 },
	{ "sshd_dsskey",		""				, 0 },
	{ "sshd_forwarding",		"1"				, 0 },
	{ "rmgt_sip",			""				, 0 },	// remote management: source ip address

	{ "http_id",			""				, 0 },
	{ "web_mx",			"status,bwm"			, 0 },
	{ "web_pb",			""				, 0 },

// admin-bwm
	{ "rstats_enable",		"1"				, 0 },
	{ "rstats_path",		""				, 0 },
	{ "rstats_stime",		"48"				, 0 },
	{ "rstats_offset",		"1"				, 0 },
	{ "rstats_data",		""				, 0 },
	{ "rstats_colors",		""				, 0 },
	{ "rstats_exclude",		""				, 0 },
	{ "rstats_sshut",		"1"				, 0 },
	{ "rstats_bak",			"0"				, 0 },

// admin-ipt
	{ "cstats_enable",		"1"				, 0 },
	{ "cstats_path",		""				, 0 },
	{ "cstats_stime",		"48"				, 0 },
	{ "cstats_offset",		"1"				, 0 },
	{ "cstats_labels",		"0"				, 0 },
	{ "cstats_exclude",		""				, 0 },
	{ "cstats_include",		""				, 0 },
	{ "cstats_all",			"1"				, 0 },
	{ "cstats_sshut",		"1"				, 0 },
	{ "cstats_bak",			"0"				, 0 },

// advanced-buttons
	{ "sesx_led",			"0"				, 0 },
	{ "sesx_b0",			"1"				, 0 },
	{ "sesx_b1",			"4"				, 0 },
	{ "sesx_b2",			"4"				, 0 },
	{ "sesx_b3",			"4"				, 0 },
	{ "sesx_script",
		"[ $1 -ge 20 ] && telnetd -p 233 -l /bin/sh\n"
	, 0 },
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
	, 0 },

// admin-log
	{ "log_remote",			"0"				, 0 },
	{ "log_remoteip",		""				, 0 },
	{ "log_remoteport",		"514"				, 0 },
	{ "log_file",			"1"				, 0 },
	{ "log_file_custom",		"0"				, 0 },
	{ "log_file_path",		"/var/log/messages"		, 0 },
	{ "log_file_size",		"50"				, 0 },
	{ "log_file_keep",		"1"				, 0 },
	{ "log_limit",			"60"				, 0 },
	{ "log_in",			"0"				, 0 },
	{ "log_out",			"0"				, 0 },
	{ "log_mark",			"60"				, 0 },
	{ "log_events",			""				, 0 },

// admin-log-webmonitor
	{ "log_wm",			"0"				, 0 },
	{ "log_wmtype",			"0"				, 0 },
	{ "log_wmip",			""				, 0 },
	{ "log_wmdmax",			"2000"				, 0 },
	{ "log_wmsmax",			"2000"				, 0 },
	{ "webmon_bkp",			"0"				, 0 },
	{ "webmon_dir",			"/tmp"				, 0 },
	{ "webmon_shrink",		"0"				, 0 },

// admin-debugging
	{ "debug_nocommit",		"0"				, 0 },
	{ "debug_cprintf",		"0"				, 0 },
	{ "debug_cprintf_file",		"0"				, 0 },
	{ "console_loglevel",		"1"				, 0 },
	{ "t_cafree",			"1"				, 0 },
	{ "t_hidelr",			"0"				, 0 },
	{ "debug_clkfix",		"1"				, 0 },
	{ "debug_ddns",			"0"				, 0 },

// admin-cifs
	{ "cifs1",			""				, 0 },
	{ "cifs2",			""				, 0 },

// admin-jffs2
	{ "jffs2_on",			"0"				, 0 },
	{ "jffs2_exec",			""				, 0 },

// admin-tomatoanon
	{ "tomatoanon_enable",		"-1"				, 0 },
	{ "tomatoanon_answer",		"0"				, 0 },
	{ "tomatoanon_cru",		"6"				, 0 },
	{ "tomatoanon_id",		""				, 0 },
	{ "tomatoanon_notify",		"1"				, 0 },

#ifdef TCONFIG_USB
// nas-usb - !!TB
	{ "usb_enable",			"1"				, 0 },
	{ "usb_uhci",			"0"				, 0 },
	{ "usb_ohci",			"0"				, 0 },
	{ "usb_usb2",			"1"				, 0 },
	{ "usb_usb3",			"1"				, 0 },
#if defined(LINUX26) && defined(TCONFIG_MICROSD)
	{ "usb_mmc",			"-1"				, 0 },
#endif
	{ "usb_irq_thresh",		"0"				, 0 },
	{ "usb_storage",		"1"				, 0 },
	{ "usb_printer",		"0"				, 0 },
	{ "usb_printer_bidirect",	"0"				, 0 },
	{ "usb_ext_opt",		""				, 0 },
	{ "usb_fat_opt",		""				, 0 },
	{ "usb_ntfs_opt",		""				, 0 },
	{ "usb_fs_ext3",		"1"				, 0 },
	{ "usb_fs_ext4",		"1"				, 0 },
	{ "usb_fs_fat",			"1"				, 0 },
#ifdef TCONFIG_NTFS
	{ "usb_fs_ntfs",		"1"				, 0 },
#ifdef TCONFIG_TUXERA
	{ "usb_ntfs_driver",		"tuxera"			, 0 },
#elif TCONFIG_UFSD
	{ "usb_ntfs_driver",		"paragon"			, 0 },
#else
	{ "usb_ntfs_driver",		"ntfs3g"			, 0 },
#endif
#endif //TCONFIG_NTFS
#ifdef TCONFIG_HFS
	{ "usb_fs_hfs",			"0"				, 0 }, //!Victek
#endif
	{ "usb_automount",		"1"				, 0 },
#if 0
	{ "usb_bdflush",		"30 500 0 0 100 100 60 0 0"	, 0 },
#endif
	{ "script_usbhotplug",		""				, 0 },
	{ "script_usbmount",		""				, 0 },
	{ "script_usbumount",		""				, 0 },
	{ "idle_enable",		"0"				, 0 },
	{ "usb_3g",			"1"				, 0 },
#endif

#ifdef TCONFIG_FTP
// nas-ftp - !!TB
	{ "ftp_enable",			"0"				, 0 },
	{ "ftp_super",			"0"				, 0 },
	{ "ftp_anonymous",		"0"				, 0 },
	{ "ftp_dirlist",		"0"				, 0 },
	{ "ftp_port",			"21"				, 0 },
	{ "ftp_max",			"0"				, 0 },
	{ "ftp_ipmax",			"0"				, 0 },
	{ "ftp_staytimeout",		"300"				, 0 },
	{ "ftp_rate",			"0"				, 0 },
	{ "ftp_anonrate",		"0"				, 0 },
	{ "ftp_anonroot",		""				, 0 },
	{ "ftp_pubroot",		""				, 0 },
	{ "ftp_pvtroot",		""				, 0 },
	{ "ftp_users",			""				, 0 },
	{ "ftp_custom",			""				, 0 },
	{ "ftp_sip",			""				, 0 },	// wan ftp access: source ip address(es)
	{ "ftp_limit",			"0,3,60"			, 0 },
	{ "log_ftp",			"0"				, 0 },
#endif

#ifdef TCONFIG_SNMP
	{ "snmp_enable",		"0"				, 0 },
	{ "snmp_port",			"161"				, 0 },
	{ "snmp_remote",		"0"				, 0 },
	{ "snmp_remote_sip",		""				, 0 },
	{ "snmp_location",		"router"			, 0 },
	{ "snmp_contact",		"admin@tomato"			, 0 },
	{ "snmp_ro",			"rocommunity"			, 0 },
#endif

#ifdef TCONFIG_SAMBASRV
// nas-samba - !!TB
	{ "smbd_enable",		"0"				, 0 },
	{ "smbd_wgroup",		"WORKGROUP"			, 0 },
	{ "smbd_master",		"1"				, 0 },
	{ "smbd_wins",			"1"				, 0 },
	{ "smbd_cpage",			""				, 0 },
	{ "smbd_cset",			"utf8"				, 0 },
	{ "smbd_custom",		""				, 0 },
	{ "smbd_autoshare",		"2"				, 0 },
	{ "smbd_shares",
		"jffs</jffs<JFFS<1<0>root$</<Hidden Root<0<1"
	, 0 },
	{ "smbd_user",			"nas"				, 0 },
	{ "smbd_passwd",		""				, 0 },
#endif

#ifdef TCONFIG_MEDIA_SERVER
// nas-media
	{ "ms_enable",			"0"				, 0 },	/* 0:Disable 1:Enable 2:Enable&Rescan */
	{ "ms_dirs",			"/mnt<"				, 0 },
	{ "ms_port",			"0"				, 0 },
	{ "ms_dbdir",			""				, 0 },
	{ "ms_tivo",			"0"				, 0 },
	{ "ms_stdlna",			"0"				, 0 },
	{ "ms_sas",			"0"				, 0 },
#endif

#ifdef TCONFIG_SDHC
// admin-sdhc
	{ "mmc_on",			"0"				, 0 },
	{ "mmc_cs",			"7"				, 0 },
	{ "mmc_clk",			"3"				, 0 },
	{ "mmc_din",			"2"				, 0 },
	{ "mmc_dout",			"4"				, 0 },
	{ "mmc_fs_partition",		"1"				, 0 },
	{ "mmc_fs_type",		"ext2"				, 0 },
	{ "mmc_exec_mount",		""				, 0 },
	{ "mmc_exec_umount",		""				, 0 },
#endif

// admin-sch
	{ "sch_rboot",			""				, 0 },
	{ "sch_rcon",			""				, 0 },
	{ "sch_c1",			""				, 0 },
	{ "sch_c2",			""				, 0 },
	{ "sch_c3",			""				, 0 },
	{ "sch_c4",			""				, 0 },
	{ "sch_c5",			""				, 0 },
	{ "sch_c1_cmd",			""				, 0 },
	{ "sch_c2_cmd",			""				, 0 },
	{ "sch_c3_cmd",			""				, 0 },
	{ "sch_c4_cmd",			""				, 0 },
	{ "sch_c5_cmd",			""				, 0 },

// admin-script
	{ "script_init",		""				, 0 },
	{ "script_shut",		""				, 0 },
	{ "script_fire",		""				, 0 },
	{ "script_wanup",		""				, 0 },

#ifdef TCONFIG_NFS
	{ "nfs_enable",			"0"				, 0 },
	{ "nfs_exports",		""				, 0 },
#endif

#ifdef TCONFIG_OPENVPN
// vpn
	{ "vpn_debug",			"0"             , 0 },
	{ "vpn_server_eas",		""              , 0 },
	{ "vpn_server_dns",		""              , 0 },
	{ "vpn_server1_poll",		"0"             , 0 },
	{ "vpn_server1_if",		"tun"           , 0 },
	{ "vpn_server1_proto",		"udp"           , 0 },
	{ "vpn_server1_port",     "1194"          , 0 },
	{ "vpn_server1_firewall", "auto"          , 0 },
	{ "vpn_server1_crypt",    "tls"           , 0 },
	{ "vpn_server1_comp",     "adaptive"      , 0 },
	{ "vpn_server1_cipher",   "default"       , 0 },
	{ "vpn_server1_dhcp",     "1"             , 0 },
	{ "vpn_server1_r1",       "192.168.1.50"  , 0 },
	{ "vpn_server1_r2",       "192.168.1.55"  , 0 },
	{ "vpn_server1_sn",       "10.8.0.0"      , 0 },
	{ "vpn_server1_nm",       "255.255.255.0" , 0 },
	{ "vpn_server1_local",    "10.8.0.1"      , 0 },
	{ "vpn_server1_remote",   "10.8.0.2"      , 0 },
	{ "vpn_server1_reneg",    "-1"            , 0 },
	{ "vpn_server1_hmac",     "-1"            , 0 },
	{ "vpn_server1_plan",     "1"             , 0 },
	{ "vpn_server1_ccd",      "0"             , 0 },
	{ "vpn_server1_c2c",      "0"             , 0 },
	{ "vpn_server1_ccd_excl", "0"             , 0 },
	{ "vpn_server1_ccd_val",  ""              , 0 },
	{ "vpn_server1_pdns",     "0"             , 0 },
	{ "vpn_server1_rgw",      "0"             , 0 },
	{ "vpn_server1_custom",   ""              , 0 },
	{ "vpn_server1_static",   ""              , 0 },
	{ "vpn_server1_ca",       ""              , 0 },
	{ "vpn_server1_crt",      ""              , 0 },
	{ "vpn_server1_key",      ""              , 0 },
	{ "vpn_server1_dh",       ""              , 0 },
	{ "vpn_server1_br",       "br0"           , 0 },
	{ "vpn_server2_poll",     "0"             , 0 },
	{ "vpn_server2_if",       "tun"           , 0 },
	{ "vpn_server2_proto",    "udp"           , 0 },
	{ "vpn_server2_port",     "1194"          , 0 },
	{ "vpn_server2_firewall", "auto"          , 0 },
	{ "vpn_server2_crypt",    "tls"           , 0 },
	{ "vpn_server2_comp",     "adaptive"      , 0 },
	{ "vpn_server2_cipher",   "default"       , 0 },
	{ "vpn_server2_dhcp",     "1"             , 0 },
	{ "vpn_server2_r1",       "192.168.1.50"  , 0 },
	{ "vpn_server2_r2",       "192.168.1.55"  , 0 },
	{ "vpn_server2_sn",       "10.8.0.0"      , 0 },
	{ "vpn_server2_nm",       "255.255.255.0" , 0 },
	{ "vpn_server2_local",    "10.8.0.1"      , 0 },
	{ "vpn_server2_remote",   "10.8.0.2"      , 0 },
	{ "vpn_server2_reneg",    "-1"            , 0 },
	{ "vpn_server2_hmac",     "-1"            , 0 },
	{ "vpn_server2_plan",     "1"             , 0 },
	{ "vpn_server2_ccd",      "0"             , 0 },
	{ "vpn_server2_c2c",      "0"             , 0 },
	{ "vpn_server2_ccd_excl", "0"             , 0 },
	{ "vpn_server2_ccd_val",  ""              , 0 },
	{ "vpn_server2_pdns",     "0"             , 0 },
	{ "vpn_server2_rgw",      "0"             , 0 },
	{ "vpn_server2_custom",   ""              , 0 },
	{ "vpn_server2_static",   ""              , 0 },
	{ "vpn_server2_ca",       ""              , 0 },
	{ "vpn_server2_crt",      ""              , 0 },
	{ "vpn_server2_key",      ""              , 0 },
	{ "vpn_server2_dh",       ""              , 0 },
	{ "vpn_server2_br",       "br0"           , 0 },
	{ "vpn_client_eas",       ""              , 0 },
	{ "vpn_client1_poll",     "0"             , 0 },
	{ "vpn_client1_if",       "tun"           , 0 },
	{ "vpn_client1_bridge",   "1"             , 0 },
	{ "vpn_client1_nat",      "1"             , 0 },
	{ "vpn_client1_proto",    "udp"           , 0 },
	{ "vpn_client1_addr",     ""              , 0 },
	{ "vpn_client1_port",     "1194"          , 0 },
	{ "vpn_client1_retry",    "30"            , 0 },
	{ "vpn_client1_rg",       "0"             , 0 },
	{ "vpn_client1_firewall", "auto"          , 0 },
	{ "vpn_client1_crypt",    "tls"           , 0 },
	{ "vpn_client1_comp",     "adaptive"      , 0 },
	{ "vpn_client1_cipher",   "default"       , 0 },
	{ "vpn_client1_local",    "10.8.0.2"      , 0 },
	{ "vpn_client1_remote",   "10.8.0.1"      , 0 },
	{ "vpn_client1_nm",       "255.255.255.0" , 0 },
	{ "vpn_client1_reneg",    "-1"            , 0 },
	{ "vpn_client1_hmac",     "-1"            , 0 },
	{ "vpn_client1_adns",     "0"             , 0 },
	{ "vpn_client1_rgw",      "0"             , 0 },
	{ "vpn_client1_gw",       ""              , 0 },
	{ "vpn_client1_custom",   ""              , 0 },
	{ "vpn_client1_static",   ""              , 0 },
	{ "vpn_client1_ca",       ""              , 0 },
	{ "vpn_client1_crt",      ""              , 0 },
	{ "vpn_client1_key",      ""              , 0 },
	{ "vpn_client1_br",       "br0"           , 0 },
	{ "vpn_client2_poll",     "0"             , 0 },
	{ "vpn_client2_if",       "tun"           , 0 },
	{ "vpn_client2_bridge",   "1"             , 0 },
	{ "vpn_client2_nat",      "1"             , 0 },
	{ "vpn_client2_proto",    "udp"           , 0 },
	{ "vpn_client2_addr",     ""              , 0 },
	{ "vpn_client2_port",     "1194"          , 0 },
	{ "vpn_client2_retry",    "30"            , 0 },
	{ "vpn_client2_rg",       "0"             , 0 },
	{ "vpn_client2_firewall", "auto"          , 0 },
	{ "vpn_client2_crypt",    "tls"           , 0 },
	{ "vpn_client2_comp",     "adaptive"      , 0 },
	{ "vpn_client2_cipher",   "default"       , 0 },
	{ "vpn_client2_local",    "10.8.0.2"      , 0 },
	{ "vpn_client2_remote",   "10.8.0.1"      , 0 },
	{ "vpn_client2_nm",       "255.255.255.0" , 0 },
	{ "vpn_client2_reneg",    "-1"            , 0 },
	{ "vpn_client2_hmac",     "-1"            , 0 },
	{ "vpn_client2_adns",     "0"             , 0 },
	{ "vpn_client2_rgw",      "0"             , 0 },
	{ "vpn_client2_gw",       ""              , 0 },
	{ "vpn_client2_custom",   ""              , 0 },
	{ "vpn_client2_static",   ""              , 0 },
	{ "vpn_client2_ca",       ""              , 0 },
	{ "vpn_client2_crt",      ""              , 0 },
	{ "vpn_client2_key",      ""              , 0 },
	{ "vpn_client2_br",       "br0"           , 0 },
#endif	// vpn
#ifdef TCONFIG_PPTPD
	{ "pptp_client_enable",   "0"             , 0 },
	{ "pptp_client_peerdns",  "0"             , 0 },
	{ "pptp_client_mtuenable","0"             , 0 },
	{ "pptp_client_mtu",      "1450"          , 0 },
	{ "pptp_client_mruenable","0"             , 0 },
	{ "pptp_client_mru",      "1450"          , 0 },
	{ "pptp_client_nat",      "0"             , 0 },
	{ "pptp_client_srvip",    ""              , 0 },
	{ "pptp_client_srvsub",   "10.0.0.0"      , 0 },
	{ "pptp_client_srvsubmsk","255.0.0.0"     , 0 },
	{ "pptp_client_username", ""              , 0 },
	{ "pptp_client_passwd",   ""              , 0 },
	{ "pptp_client_crypt",    "0"             , 0 },
	{ "pptp_client_custom",   ""              , 0 },
	{ "pptp_client_dfltroute","0"             , 0 },
#endif

#ifdef TCONFIG_TINC
	{"tinc_wanup",			"0"		, 0 },
	{"tinc_name",			""		, 0 },
	{"tinc_devicetype",		"tun"		, 0 }, // tun, tap
	{"tinc_mode",			"switch"	, 0 }, // switch, hub
	{"tinc_vpn_netmask",		"255.255.0.0"	, 0 },
	{"tinc_private_rsa",		""		, 0 },
	{"tinc_private_ed25519",	""		, 0 },
	{"tinc_custom",			""		, 0 },
	{"tinc_hosts",			""		, 0 },
	{"tinc_manual_firewall",	""		, 0 },
	{"tinc_manual_tinc_up",		"0"		, 0 },
	// scripts
	{"tinc_tinc_up",		""		, 0 },
	{"tinc_tinc_down",		""		, 0 },
	{"tinc_host_up",		""		, 0 },
	{"tinc_host_down",		""		, 0 },
	{"tinc_subnet_up",		""		, 0 },
	{"tinc_subnet_down",		""		, 0 },
	{"tinc_firewall",		""		, 0 },
#endif

#ifdef TCONFIG_BT
// nas-transmission
	{ "bt_enable",			"0"				, 0 },
#ifdef TCONFIG_BBT
	{ "bt_binary",			"internal"			, 0 },
#else
	{ "bt_binary",			"optware"			, 0 },
#endif
	{ "bt_binary_custom",		"/path/to/binaries/directory"	, 0 },
	{ "bt_custom",			""				, 0 },
	{ "bt_port",			"51515"				, 0 },
	{ "bt_dir",			"/mnt"				, 0 },
	{ "bt_incomplete",		"1"				, 0 },
	{ "bt_settings",		"down_dir"			, 0 },
	{ "bt_settings_custom",		"/tmp/btclient"			, 0 },
	{ "bt_rpc_enable",		"1"				, 0 },
	{ "bt_rpc_wan",			"0"				, 0 },
	{ "bt_auth",			"1"				, 0 },
	{ "bt_login",			"admin"				, 0 },
	{ "bt_password",		"admin11"			, 0 },
	{ "bt_port_gui",		"9091"				, 0 },
	{ "bt_dl_enable",		"0"				, 0 },
	{ "bt_ul_enable",		"0"				, 0 },
	{ "bt_dl",			"248"				, 0 },
	{ "bt_ul",			"64"				, 0 },
	{ "bt_peer_limit_global",	"150"				, 0 },
	{ "bt_peer_limit_per_torrent",	"30"				, 0 },
	{ "bt_ul_slot_per_torrent",	"10"				, 0 },
	{ "bt_ratio_enable",		"0"				, 0 },
	{ "bt_ratio",			"1.0000"			, 0 },
	{ "bt_ratio_idle_enable",	"0"				, 0 },
	{ "bt_ratio_idle",		"30"				, 0 },
	{ "bt_dht",			"0"				, 0 },
	{ "bt_pex",			"0"				, 0 },
	{ "bt_lpd",			"0"				, 0 },
	{ "bt_utp",			"1"				, 0 },
	{ "bt_blocklist",		"0"				, 0 },
	{ "bt_blocklist_url",		"http://list.iblocklist.com/?list=bt_level1"	, 0 },
	{ "bt_sleep",			"10"				, 0 },
	{ "bt_check",			"1"				, 0 },
	{ "bt_check_time",		"15"				, 0 },
	{ "bt_dl_queue_enable",		"0"				, 0 },
	{ "bt_dl_queue_size",		"5"				, 0 },
	{ "bt_ul_queue_enable",		"0"				, 0 },
	{ "bt_ul_queue_size",		"5"				, 0 },
	{ "bt_message",			"2"				, 0 },
	{ "bt_log",			"0"				, 0 },
	{ "bt_log_path",		"/var/log"			, 0 },
#endif

// new_qoslimit
	{ "new_qoslimit_enable",	"0"				, 0 },
	{ "new_qoslimit_obw",		""				, 0 },
	{ "new_qoslimit_ibw",		""				, 0 },
	{ "new_qoslimit_rules",		""				, 0 },
	{ "qosl_enable",		"0"				, 0 },
	{ "qosl_tcp",			"0"				, 0 },//unlimited
	{ "qosl_udp",			"0"				, 0 },//unlimited
	{ "qosl_dlc",			""				, 0 },
	{ "qosl_ulc",			""				, 0 },
	{ "qosl_dlr",			""				, 0 },
	{ "qosl_ulr",			""				, 0 },
	{ "limit_br0_prio",		"3"				, 0 },
	{ "limit_br1_enable",		"0"				, 0 },
	{ "limit_br1_dlc",		""				, 0 },
	{ "limit_br1_ulc",		""				, 0 },
	{ "limit_br1_dlr",		""				, 0 },
	{ "limit_br1_ulr",		""				, 0 },
	{ "limit_br1_prio",		"2"				, 0 },
	{ "limit_br2_enable",		"0"				, 0 },
	{ "limit_br2_dlc",		""				, 0 },
	{ "limit_br2_ulc",		""				, 0 },
	{ "limit_br2_dlr",		""				, 0 },
	{ "limit_br2_ulr",		""				, 0 },
	{ "limit_br2_prio",		"2"				, 0 },
	{ "limit_br3_enable",		"0"				, 0 },
	{ "limit_br3_dlc",		""				, 0 },
	{ "limit_br3_ulc",		""				, 0 },
	{ "limit_br3_dlr",		""				, 0 },
	{ "limit_br3_ulr",		""				, 0 },
	{ "limit_br3_prio",		"2"				, 0 },


// NoCatSplash. !!Victek
#ifdef TCONFIG_NOCAT
	{ "NC_enable",			"0"				, 0 }, // enable NoCatSplash
	{ "NC_Verbosity",		"2"				, 0 },
	{ "NC_GatewayName",		"Tomato Captive Portal"		, 0 },
	{ "NC_GatewayPort",		"5280"				, 0 },
	{ "NC_GatewayMode",		"Open"				, 0 },
	{ "NC_DocumentRoot",		"/tmp/splashd"			, 0 },
	{ "NC_ExcludePorts",		"1863"				, 0 },
	{ "NC_HomePage",		"http://google.com"		, 0 },
	{ "NC_ForcedRedirect",		"0"				, 0 },
	{ "NC_IdleTimeout",		"0"				, 0 },
	{ "NC_MaxMissedARP",		"5"				, 0 },
	{ "NC_PeerChecktimeout",	"0"				, 0 },
	{ "NC_LoginTimeout",		"3600"				, 0 },
	{ "NC_RenewTimeout",		"0"				, 0 },
	{ "NC_AllowedWebHosts",		""				, 0 },
	{ "NC_BridgeLAN",		"br0"				, 0 },
#endif

//Tomato RAF - NGINX
#ifdef TCONFIG_NGINX
	{"nginx_enable",		"0"				, 0 }, // NGinX enabled
	{"nginx_php",			"0"				, 0 }, // PHP enabled
	{"nginx_keepconf",		"0"				, 0 }, // Enable/disable keep configuration files unmodified in /etc/nginx
	{"nginx_docroot",		"/www"				, 0 }, // path for server files
	{"nginx_port",			"85"				, 0 }, // port to listen
	{"nginx_remote",		"0"				, 0 }, // open port from WAN site
	{"nginx_fqdn",			"Tomato"			, 0 }, // server name
	{"nginx_upload",		"100"				, 0 }, // upload file size limit
	{"nginx_priority",		"10"				, 0 }, // server priority = worker_priority
	{"nginx_custom",		""				, 0 }, // additional lines for nginx.conf
	{"nginx_httpcustom",		""				, 0 }, // additional lines for nginx.conf
	{"nginx_servercustom",		""				, 0 }, // additional lines for nginx.conf
	{"nginx_phpconf",		""				, 0 }, // additional lines for php.ini
	{"nginx_user",			"root"				, 0 }, // user/group
	{"nginx_override",		"0"				, 0 }, // additional lines for php.ini
	{"nginx_overridefile",		"/path/to/nginx.conf"		, 0 }, // user/group

// bwq518 - MySQL
        { "mysql_enable",                               "0"                     , 0 },
        { "mysql_sleep",                                "2"                     , 0 },
        { "mysql_check",                                "1"                     , 0 },
        { "mysql_check_time",                           "1"                     , 0 },
        { "mysql_binary",                               "internal"              , 0 },
        { "mysql_binary_custom",                        "/mnt/sda1/mysql/bin"   , 0 },
        { "mysql_usb_enable",                           "1"                     , 0 },
        { "mysql_dlroot",                               ""                      , 0 },
        { "mysql_datadir",                              "data"                  , 0 },
        { "mysql_tmpdir",                               "tmp"                   , 0 },
        { "mysql_server_custom",                        ""                      , 0 },
        { "mysql_port",                                 "3306"                  , 0 },
        { "mysql_allow_anyhost",                        "0"                     , 0 },
        { "mysql_init_rootpass",                        "0"                     , 0 },
        { "mysql_username",                             "root"                  , 0 },      // mysqladmin username
        { "mysql_passwd",                               "admin"                 , 0 },      // mysqladmin password
        { "mysql_key_buffer",                           "16"                    , 0 }, //KB
        { "mysql_max_allowed_packet",                   "4"                     , 0 }, //MB
        { "mysql_thread_stack",                         "128"                   , 0 }, //KB
        { "mysql_thread_cache_size",                    "8"                     , 0 },
        { "mysql_init_priv",                            "0"                     , 0 },
        { "mysql_table_open_cache",                     "4"                     , 0 },
        { "mysql_sort_buffer_size",                     "128"                   , 0 }, //KB
        { "mysql_read_buffer_size",                     "128"                   , 0 }, //KB
        { "mysql_query_cache_size",                     "16"                    , 0 }, //MB
        { "mysql_read_rnd_buffer_size",                 "256"                   , 0 }, //KB
        { "mysql_net_buffer_length",                    "2"                     , 0 }, //K
        { "mysql_max_connections",                      "1000"                  , 0 },
#endif

#ifdef TCONFIG_TOR
	{ "tor_enable",			"0"				, 0 },
	{ "tor_socksport",		"9050"				, 0 },
	{ "tor_transport",		"9040"				, 0 },
	{ "tor_dnsport",		"9053"				, 0 },
	{ "tor_datadir",		"/tmp/tor"			, 0 },
	{ "tor_iface",			"br0"				, 0 },
	{ "tor_users",			"192.168.1.0/24"		, 0 },
	{ "tor_custom",			""				, 0 },
#endif

	{ 0, 0, 0}
};

#ifdef TCONFIG_BCMWL6
#ifndef TCONFIG_BCMARM
struct nvram_tuple bcm4360ac_defaults[] = {
	{ "pci/2/1/aa2g", "0", 0 },
	{ "pci/2/1/aa5g", "7", 0 },
	{ "pci/2/1/aga0", "0", 0 },
	{ "pci/2/1/aga1", "0", 0 },
	{ "pci/2/1/aga2", "0", 0 },
	{ "pci/2/1/agbg0", "133", 0 },
	{ "pci/2/1/agbg1", "133", 0 },
	{ "pci/2/1/agbg2", "133", 0 },
	{ "pci/2/1/antswitch", "0", 0 },
	{ "pci/2/1/cckbw202gpo", "0", 0 },
	{ "pci/2/1/cckbw20ul2gpo", "0", 0 },
	{ "pci/2/1/dot11agofdmhrbw202gpo", "0", 0 },
	{ "pci/2/1/femctrl", "3", 0 },
	{ "pci/2/1/papdcap2g", "0", 0 },
	{ "pci/2/1/tworangetssi2g", "0", 0 },
	{ "pci/2/1/pdgain2g", "4", 0 },
	{ "pci/2/1/epagain2g", "0", 0 },
	{ "pci/2/1/tssiposslope2g", "1", 0 },
	{ "pci/2/1/gainctrlsph", "0", 0 },
	{ "pci/2/1/papdcap5g", "0", 0 },
	{ "pci/2/1/tworangetssi5g", "0", 0 },
	{ "pci/2/1/pdgain5g", "4", 0 },
	{ "pci/2/1/epagain5g", "0", 0 },
	{ "pci/2/1/tssiposslope5g", "1", 0 },
	{ "pci/2/1/maxp2ga0", "76", 0 },
	{ "pci/2/1/maxp2ga1", "76", 0 },
	{ "pci/2/1/maxp2ga2", "76", 0 },
	{ "pci/2/1/mcsbw202gpo", "0", 0 },
	{ "pci/2/1/mcsbw402gpo", "0", 0 },
	{ "pci/2/1/measpower", "0x7f", 0 },
	{ "pci/2/1/measpower1", "0x7f", 0 },
	{ "pci/2/1/measpower2", "0x7f", 0 },
	{ "pci/2/1/noiselvl2ga0", "31", 0 },
	{ "pci/2/1/noiselvl2ga1", "31", 0 },
	{ "pci/2/1/noiselvl2ga2", "31", 0 },
	{ "pci/2/1/noiselvl5gha0", "31", 0 },
	{ "pci/2/1/noiselvl5gha1", "31", 0 },
	{ "pci/2/1/noiselvl5gha2", "31", 0 },
	{ "pci/2/1/noiselvl5gla0", "31", 0 },
	{ "pci/2/1/noiselvl5gla1", "31", 0 },
	{ "pci/2/1/noiselvl5gla2", "31", 0 },
	{ "pci/2/1/noiselvl5gma0", "31", 0 },
	{ "pci/2/1/noiselvl5gma1", "31", 0 },
	{ "pci/2/1/noiselvl5gma2", "31", 0 },
	{ "pci/2/1/noiselvl5gua0", "31", 0 },
	{ "pci/2/1/noiselvl5gua1", "31", 0 },
	{ "pci/2/1/noiselvl5gua2", "31", 0 },
	{ "pci/2/1/ofdmlrbw202gpo", "0", 0 },
	{ "pci/2/1/pa2ga0", "0xfe72,0x14c0,0xfac7", 0 },
	{ "pci/2/1/pa2ga1", "0xfe80,0x1472,0xfabc", 0 },
	{ "pci/2/1/pa2ga2", "0xfe82,0x14bf,0xfad9", 0 },
	{ "pci/2/1/pcieingress_war", "15", 0 },
	{ "pci/2/1/phycal_tempdelta", "255", 0 },
	{ "pci/2/1/rawtempsense", "0x1ff", 0 },
	{ "pci/2/1/rxchain", "7", 0 },
	{ "pci/2/1/rxgainerr2g", "0xffff", 0 },
	{ "pci/2/1/rxgainerr5g", "0xffff,0xffff,0xffff,0xffff", 0 },
	{ "pci/2/1/rxgains2gelnagaina0", "0", 0 },
	{ "pci/2/1/rxgains2gelnagaina1", "0", 0 },
	{ "pci/2/1/rxgains2gelnagaina2", "0", 0 },
	{ "pci/2/1/rxgains2gtrelnabypa0", "0", 0 },
	{ "pci/2/1/rxgains2gtrelnabypa1", "0", 0 },
	{ "pci/2/1/rxgains2gtrelnabypa2", "0", 0 },
	{ "pci/2/1/rxgains2gtrisoa0", "0", 0 },
	{ "pci/2/1/rxgains2gtrisoa1", "0", 0 },
	{ "pci/2/1/rxgains2gtrisoa2", "0", 0 },
	{ "pci/2/1/sar2g", "18", 0 },
	{ "pci/2/1/sar5g", "15", 0 },
	{ "pci/2/1/sromrev", "11", 0 },
	{ "pci/2/1/subband5gver", "0x4", 0 },
	{ "pci/2/1/tempcorrx", "0x3f", 0 },
	{ "pci/2/1/tempoffset", "255", 0 },
	{ "pci/2/1/temps_hysteresis", "15", 0 },
	{ "pci/2/1/temps_period", "15", 0 },
	{ "pci/2/1/tempsense_option", "0x3", 0 },
	{ "pci/2/1/tempsense_slope", "0xff", 0 },
	{ "pci/2/1/tempthresh", "255", 0 },
	{ "pci/2/1/txchain", "7", 0 },
	{ "pci/2/1/ledbh0", "2", 0 },
	{ "pci/2/1/ledbh1", "5", 0 },
	{ "pci/2/1/ledbh2", "4", 0 },
	{ "pci/2/1/ledbh3", "11", 0 },
	{ "pci/2/1/ledbh10", "7", 0 },

	{ 0, 0, 0 }
};
#else
struct nvram_tuple bcm4360ac_defaults[] = {
	{ "0:ledbh10", "7", 0 },
	{ "1:ledbh10", "7", 0 },
	{ 0, 0, 0 }
};
#endif

/* nvram override default setting for Media Router */
struct nvram_tuple router_defaults_override_type1[] = {
	{ "router_disable", "1", 0 },		/* lan_proto=static lan_stp=0 wan_proto=disabled */
	{ "lan_stp", "0", 0 },			/* LAN spanning tree protocol */
	{ "wl_wmf_bss_enable", "1", 0 },	/* WMF Enable for IPTV Media or WiFi+PLC */
	{ "wl_reg_mode", "h", 0 },		/* Regulatory: 802.11H(h) */
	{ "wl_wet_tunnel", "1", 0  },		/* Enable wet tunnel */
	{ "wl_taf_enable", "1", 0  },		/* Enable TAF */

	/* EBOS feature Media router default */
	{ "wl_ebos_enable", "0", 0 },		/* EBOS feature on */
	{ "wl_ebos_flags", "104", 0 },		/* 104(0x68) pseudo-round robin */
	{ "wl_ebos_prr_threshold", "0x0f000000", 0 },	/* pseudo-round robin threshold */

#ifdef __CONFIG_EMF__
	{ "emf_enable", "1", 0 },		/* Enable EMF by default */
	{ "wl_wmf_ucigmp_query", "1", 0 },	/* Enable Converting IGMP Query to ucast */
	{ "wl_wmf_ucast_upnp", "1", 0 },	/* Enable upnp to ucast conversion */
	{ "wl_wmf_igmpq_filter", "1", 0 },	/* Enable igmp query filter */
#endif
	{ "wl_acs_fcs_mode", "1", 0 },		/* Enable acsd fcs mode */
	{ "wl_acs_dfs", "1", 0 },		/* Enable first DFS chan Selection */
	{ "wl_dcs_csa_unicast", "1", 0 },	/* Enable unicast CSA */
	/* Exclude ACSD to select 140l, 144u, 140/80, 144/80 to compatible with Ducati 11N */
	{ "wl_acs_excl_chans", "0xd98e,0xd88e,0xe28a,0xe38a", 0 },
	{ "wl_pspretend_retry_limit", "5", 0 }, /* Enable PsPretend */
	{ "wl_pspretend_threshold", "0", 0 },	/* Disable PsPretend Threshold */
	{ "wl_acs_chan_dwell_time", "70", 0 },	/* WAR for AP to stay on DFS chan */
	{ "wl_frameburst", "on", 0 },		/* BRCM Frambursting mode (off|on) */
	{ "wl_amsdu", "off", 0 },		/* Default IPTV AMSDU setting */
	{ "wl_rx_amsdu_in_ampdu", "off", 0 },	/* Media RX AMSDU In AMPDU setting */
	{ 0, 0, 0 }
};

/* Translates from, for example, wl0_ (or wl0.1_) to wl_. */
/* Only single digits are currently supported */

static void
fix_name(const char *name, char *fixed_name)
{
	char *pSuffix = NULL;

	/* Translate prefix wlx_ and wlx.y_ to wl_ */
	/* Expected inputs are: wld_root, wld.d_root, wld.dd_root
	 * We accept: wld + '_' anywhere
	 */
	pSuffix = strchr(name, '_');

	if ((strncmp(name, "wl", 2) == 0) && isdigit(name[2]) && (pSuffix != NULL)) {
		strcpy(fixed_name, "wl");
		strcpy(&fixed_name[2], pSuffix);
		return;
	}

	/* No match with above rules: default to input name */
	strcpy(fixed_name, name);
}


/*
 * Find nvram param name; return pointer which should be treated as const
 * return NULL if not found.
 *
 * NOTE:  This routine special-cases the variable wl_bss_enabled.  It will
 * return the normal default value if asked for wl_ or wl0_.  But it will
 * return 0 if asked for a virtual BSS reference like wl0.1_.
 */
char *
nvram_default_get(const char *name)
{
	int idx;
	char fixed_name[NVRAM_MAX_VALUE_LEN];

	fix_name(name, fixed_name);
	if (strcmp(fixed_name, "wl_bss_enabled") == 0) {
		if (name[3] == '.' || name[4] == '.') { /* Virtual interface */
			return "0";
		}
	}

#ifdef __CONFIG_HSPOT__
	if (strcmp(fixed_name, "wl_bss_hs2_enabled") == 0) {
		if (name[3] == '.' || name[4] == '.') { /* Virtual interface */
			return "0";
		}
	}
#endif  /* __CONFIG_HSPOT__ */

	if (!strcmp(nvram_safe_get("devicemode"), "1")) {
		for (idx = 0; router_defaults_override_type1[idx].name != NULL; idx++) {
			if (strcmp(router_defaults_override_type1[idx].name, fixed_name) == 0) {
				return router_defaults_override_type1[idx].value;
			}
		}
	}

	for (idx = 0; router_defaults[idx].name != NULL; idx++) {
		if (strcmp(router_defaults[idx].name, fixed_name) == 0) {
			return router_defaults[idx].value;
		}
	}

	return NULL;
}
/* validate/restore all per-interface related variables */
void
nvram_validate_all(char *prefix, bool restore)
{
	struct nvram_tuple *t;
	char tmp[100];
	char *v;

	for (t = router_defaults; t->name; t++) {
		if (!strncmp(t->name, "wl_", 3)) {
			strcat_r(prefix, &t->name[3], tmp);
			if (!restore && nvram_get(tmp))
				continue;
			v = nvram_get(t->name);
			nvram_set(tmp, v ? v : t->value);
		}
	}

	/* override router type1 nvram setting */
	if (!strcmp(nvram_safe_get("devicemode"), "1")) {
		for (t = router_defaults_override_type1; t->name; t++) {
			if (!strncmp(t->name, "wl_", 3)) {
				strcat_r(prefix, &t->name[3], tmp);
				if (!restore && nvram_get(tmp))
					continue;
				v = nvram_get(t->name);
				nvram_set(tmp, v ? v : t->value);
			}
		}
	}
}

/* restore specific per-interface variable */
void
nvram_restore_var(char *prefix, char *name)
{
	struct nvram_tuple *t;
	char tmp[100];

	for (t = router_defaults; t->name; t++) {
		if (!strncmp(t->name, "wl_", 3) && !strcmp(&t->name[3], name)) {
			nvram_set(strcat_r(prefix, name, tmp), t->value);
			break;
		}
	}

	/* override router type1 setting */
	if (!strcmp(nvram_safe_get("devicemode"), "1")) {
		for (t = router_defaults_override_type1; t->name; t++) {
			if (!strncmp(t->name, "wl_", 3) && !strcmp(&t->name[3], name)) {
				nvram_set(strcat_r(prefix, name, tmp), t->value);
				break;
			}
		}
	}
}
#endif
