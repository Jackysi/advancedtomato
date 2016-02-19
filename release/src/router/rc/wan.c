/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

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

	Copyright 2005, Broadcom Corporation
	All Rights Reserved.

	THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.

*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "rc.h"

#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>
#include <time.h>
#include <bcmdevs.h>

#define mwanlog(level,x...) if(nvram_get_int("mwan_debug")>=level) syslog(level, x)

static void make_secrets(char *prefix) //static void make_secrets(void)
{
	FILE *f;
	char *user;
	char *pass;
	
	char secrets_file[256];
	char tmp[100];

	user = nvram_safe_get(strcat_r(prefix, "_ppp_username", tmp)); //"ppp_username" -> strcat_r(prefix, "_ppp_username", tmp)
	pass = nvram_safe_get(strcat_r(prefix, "_ppp_passwd", tmp));   //"ppp_passwd" -> strcat_r(prefix, "_ppp_passwd", tmp)
	
	memset(secrets_file, 0, 256);
	sprintf(secrets_file, "/tmp/ppp/%s_pap-secrets", prefix);
	if ((f = fopen(secrets_file, "w")) != NULL) {
		fprintf(f, "\"%s\" * \"%s\" *\n", user, pass);
		fclose(f);
	}
	chmod(secrets_file, 0600);

	memset(secrets_file, 0, 256);
	sprintf(secrets_file, "/tmp/ppp/%s_chap-secrets", prefix);
	if ((f = fopen(secrets_file, "w")) != NULL) {
		fprintf(f, "\"%s\" * \"%s\" *\n", user, pass);
		fclose(f);
	}
	chmod(secrets_file, 0600);
}

// -----------------------------------------------------------------------------

static int config_pppd(int wan_proto, int num, char *prefix) //static int config_pppd(int wan_proto, int num)
{
	TRACE_PT("begin\n");

	FILE *fp;
	FILE *cfp;
	char *p;
	int demand;

	char ppp_optfile[256];
	char tmp[100];

	mkdir("/tmp/ppp", 0777);
	symlink("/sbin/rc", "/tmp/ppp/ip-up");
	symlink("/sbin/rc", "/tmp/ppp/ip-down");
#ifdef TCONFIG_IPV6
	symlink("/sbin/rc", "/tmp/ppp/ipv6-up");
	symlink("/sbin/rc", "/tmp/ppp/ipv6-down");
#endif
	symlink("/dev/null", "/tmp/ppp/connect-errors");

	demand = nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp)); //"ppp_demand" -> strcat_r(prefix, "_ppp_demand", tmp)

	// Generate options file
	memset(ppp_optfile, 0, 256);
	sprintf(ppp_optfile, "/tmp/ppp/%s_options", prefix);
	if ((fp = fopen(ppp_optfile, "w")) == NULL) {
		perror(ppp_optfile);
		return -1;
	}

#ifdef LINUX26
#ifdef TCONFIG_USB
	char ppp3g_chatfile[256];
	memset(ppp3g_chatfile, 0, 256);
	sprintf(ppp3g_chatfile, "/tmp/ppp/%s_connect.chat", prefix);
	if (nvram_match(strcat_r(prefix, "_proto", tmp), "ppp3g") ) { //"_proto" -> strcat_r(prefix, "_proto", tmp)
		fprintf(fp,
			"/dev/%s\n"
			"460800\n"
			"connect \"/usr/sbin/chat -V -t 60 -f %s\"\n"
			"noipdefault\n"
			"lock\n"
			"crtscts\n"
			"modem\n"
			"ipcp-accept-local\n",
			nvram_safe_get(strcat_r(prefix, "_modem_dev", tmp)), //"modem_dev" -> strcat_r(prefix, "_modem_dev", tmp)
			ppp3g_chatfile);

		if (strlen(nvram_get(strcat_r(prefix, "_ppp_username", tmp))) >0 ) //if (strlen(nvram_get("ppp_username")) >0 )
			fprintf(fp, "user '%s'\n", nvram_get(strcat_r(prefix, "_ppp_username", tmp)));// "ppp_username" -> strcat_r(prefix, "_ppp_username", tmp)
	} else {
#endif
#endif
		fprintf(fp,
			"unit %d\n"
			"user '%s'\n"
			"lcp-echo-adaptive\n",	// Suppress LCP echo-requests if traffic was received
			num,
			nvram_safe_get(strcat_r(prefix, "_ppp_username", tmp))); //"ppp_usrename" -> strcat_r(prefix, "_ppp_username", tmp
#ifdef LINUX26
#ifdef TCONFIG_USB
	}
#endif
#endif

	fprintf(fp,
		"defaultroute\n"	// Add a default route to the system routing tables, using the peer as the gateway
		"usepeerdns\n"		// Ask the peer for up to 2 DNS server addresses
		"default-asyncmap\n"	// Disable  asyncmap  negotiation
		"novj\n"		// Disable Van Jacobson style TCP/IP header compression
		"nobsdcomp\n"		// Disable BSD-Compress  compression
		"nodeflate\n"		// Disable Deflate compression
		"noauth\n"		// Do not authenticate peer
		"refuse-eap\n"		// Do not use eap
		"maxfail 0\n"		// Never give up
		"lcp-echo-interval %d\n"// Interval between LCP echo-requests
		"lcp-echo-failure %d\n"	// Tolerance to unanswered echo-requests
		"%s",			// Debug
		nvram_get_int(strcat_r(prefix, "_pppoe_lei", tmp)) ? : 10, //"pppoe_lei" -> strcat_r(prefix, "_pppoe_lei", tmp)
		nvram_get_int(strcat_r(prefix, "_pppoe_lef", tmp)) ? : 5,  //"pppoe_lef" -> 
		nvram_get_int("debug_ppp") ? "debug\n" : ""); //"debug_ppp" -> 

#ifdef LINUX26
#ifdef TCONFIG_USB
	if (nvram_match(strcat_r(prefix, "_wan_proto", tmp), "ppp3g") && nvram_match(strcat_r(prefix, "_modem_dev", tmp), "ttyACM0") ) {  //"wan_proto", "modem_dev"
		//don`t write nopcomp and noaccomp options
	} else {
#endif
#endif
		fprintf(fp,
			"nopcomp\n"		// Disable protocol field compression
			"noaccomp\n"		// Disable Address/Control compression
			);
#ifdef LINUX26
#ifdef TCONFIG_USB
	}
#endif
#endif


	if (wan_proto != WP_L2TP) {
		fprintf(fp,
			"persist\n"
			"holdoff %d\n",
			demand ? 30 : (nvram_get_int(strcat_r(prefix, "ppp_redialperiod", tmp)) ? : 30)); //"ppp_redialperiod"
	}

	switch (wan_proto) {
	case WP_PPTP:
		fprintf(fp,
			"plugin pptp.so\n"
			"pptp_server %s\n"
			"nomppe-stateful\n"
			"mtu %d\n",
			nvram_safe_get(strcat_r(prefix, "_pptp_server_ip", tmp)), //"pptp_server_ip"
			nvram_get_int(strcat_r(prefix, "_mtu_enable", tmp)) ? nvram_get_int(strcat_r(prefix, "_wan_mtu", tmp)) : 1400); 
			//"mtu_enable", "wan_mtu"
		break;
	case WP_PPPOE:
		fprintf(fp,
			"password '%s'\n"
			"plugin rp-pppoe.so\n"
			"nomppe nomppc\n"
			"nic-%s\n"
			"mru %d mtu %d\n",
			nvram_safe_get(strcat_r(prefix, "_ppp_passwd", tmp)), //"ppp_passwd"
			nvram_safe_get(strcat_r(prefix, "_ifname", tmp)), //"wan_ifname"
			nvram_get_int(strcat_r(prefix, "_mtu", tmp)),
			nvram_get_int(strcat_r(prefix, "_mtu", tmp)));  //"wan_mtu
		if (((p = nvram_get(strcat_r(prefix, "_ppp_service", tmp))) != NULL) && (*p)) {  //"ppp_service"
			fprintf(fp, "rp_pppoe_service '%s'\n", p);
		}
		if (((p = nvram_get(strcat_r(prefix, "_ppp_ac", tmp))) != NULL) && (*p)) { //"ppp_ac"
			fprintf(fp, "rp_pppoe_ac '%s'\n", p);
		}
		if (nvram_match(strcat_r(prefix, "_ppp_mlppp", tmp), "1")) {  //"ppp_mlppp"
			fprintf(fp, "mp\n");
		}
		break;
#ifdef LINUX26
#ifdef TCONFIG_USB
	case WP_PPP3G:
		memset(ppp3g_chatfile, 0, 256);
		sprintf(ppp3g_chatfile, "/tmp/ppp/%s_connect.chat", prefix);
		if ((cfp = fopen(ppp3g_chatfile, "w")) == NULL) {
			perror(ppp3g_chatfile);
			return -1;
		}
		fprintf(cfp,
			"ABORT \"NO CARRIER\"\n"
			"ABORT \"NO DIALTONE\"\n"
			"ABORT \"NO ERROR\"\n"
			"ABORT \"NO ANSWER\"\n"
			"ABORT \"BUSY\"\n"
			"REPORT CONNECT\n"
			"\"\" \"AT\"\n");
/* moved to switch3g script
		if (strlen(nvram_get("modem_pin")) >0 ) {
			fprintf(cfp, 
				"TIMEOUT 60\n"
				"OK \"AT+CPIN=%s\"\n"
				"TIMEOUT 10\n",
				nvram_get("modem_pin"));
		}
*/
		fprintf(cfp,
			"OK \"AT&FE0V1X1&D2&C1S0=0\"\n"
			"OK \"AT\"\n"
			"OK \"ATS0=0\"\n"
			"OK \"AT\"\n"
			"OK \"AT&FE0V1X1&D2&C1S0=0\"\n"
			"OK \"AT\"\n"
			"OK 'AT+CGDCONT=1,\"IP\",\"%s\"'\n"
			"OK \"ATDT%s\"\n"
			"CONNECT \\c\n",
			nvram_safe_get(strcat_r(prefix, "_modem_apn", tmp)), //"modem_apn"
			nvram_safe_get(strcat_r(prefix, "_modem_init", tmp)) //"modem_init"
			);
		fclose(cfp);


		if (nvram_match("usb_3g", "1") && nvram_match(strcat_r(prefix, "_proto", tmp), "ppp3g")) {
			// clear old gateway
			if (strlen(nvram_get(strcat_r(prefix, "_gateway", tmp))) >0 ) {  //"wan_gateway"
				nvram_set(strcat_r(prefix, "_gateway", tmp), "");
			}

			// detect 3G Modem
			xstart("switch3g", prefix);
		}
		break;
#endif
#endif
	case WP_L2TP:
		fprintf(fp, "nomppe nomppc\n");
		if (nvram_get_int(strcat_r(prefix, "_mtu_enable", tmp))) //"mtu_enable"
			fprintf(fp, "mtu %d\n", nvram_get_int(strcat_r(prefix, "_mtu", tmp))); //"wan_mtu"
		break;
	}

	if (demand) {
		// demand mode
		fprintf(fp,
			"demand\n"		// Dial on demand
			"idle %d\n"
			"ipcp-accept-remote\n"
			"ipcp-accept-local\n"
			"noipdefault\n"		// Disables  the  default  behaviour when no local IP address is specified
			"ktune\n",		// Set /proc/sys/net/ipv4/ip_dynaddr to 1 in demand mode if the local address changes
			nvram_get_int(strcat_r(prefix, "_ppp_idletime", tmp)) * 60);  //"ppp_idletime"
	}

#ifdef TCONFIG_IPV6
	switch (get_ipv6_service()) {
	case IPV6_NATIVE:
	case IPV6_NATIVE_DHCP:
		fprintf(fp, "+ipv6\n");
		break;
	}
#endif
	// User specific options
	fprintf(fp, "%s\n", nvram_safe_get(strcat_r(prefix, "_ppp_custom", tmp))); //"ppp_custom"

	fclose(fp);
	make_secrets(prefix);

	TRACE_PT("end\n");
	return 0;
}

static void stop_ppp(char *prefix)
{
	TRACE_PT("begin\n");
	char ppp_linkfile[256];
	char pppd_name[256];
	char tmp[100];
	
	memset(ppp_linkfile, 0, 256);
	sprintf(ppp_linkfile, "/tmp/ppp/%s_link", prefix);
	memset(pppd_name, 0, 256);
	sprintf(pppd_name, "pppd%s", prefix);

	unlink(ppp_linkfile);

	killall_tk("ip-up");
	killall_tk("ip-down");
#ifdef TCONFIG_IPV6
	killall_tk("ipv6-up");
	killall_tk("ipv6-down");
#endif
	killall_tk("xl2tpd");
	//kill(nvram_get_int(strcat_r(prefix, "_pppd_pid", tmp)),1); 
	killall_tk((char *)pppd_name);
	killall_tk("listen");

	TRACE_PT("end\n");
}

static void run_pppd(char *prefix)
{
	char tmp[100];

	char pppd_path[256];
	memset(pppd_path, 0, 256);
	sprintf(pppd_path, "/tmp/ppp/pppd%s", prefix);
	char ppp_optfile[256];
	memset(ppp_optfile, 0, 256);
	sprintf(ppp_optfile, "/tmp/ppp/%s_options", prefix);
	symlink("/usr/sbin/pppd", pppd_path);
	eval(pppd_path, "file", ppp_optfile);

	if (nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp))) {  //"ppp_demand"
		// demand mode
		/*
		   Fixed issue id 7887(or 7787):
		   When DUT is PPTP Connect on Demand mode, it couldn't be trigger from LAN.
		*/
		stop_dnsmasq();
		dns_to_resolv();
		start_dnsmasq();

		// Trigger Connect On Demand if user ping pptp server
		eval("listen", nvram_safe_get("lan_ifname"), prefix);
	}
	else {
		// keepalive mode
		start_redial(prefix);
	}
}

// -----------------------------------------------------------------------------

inline void stop_pptp(char *prefix)
{
	stop_ppp(prefix);
}

void start_pptp(int mode, char *prefix)
{

	TRACE_PT("begin\n");

	if (!using_dhcpc(prefix)) stop_dhcpc(prefix);
	stop_pptp(prefix);

	if (config_pppd(WP_PPTP, 0, prefix) != 0)
		return;

	run_pppd(prefix);

	TRACE_PT("end\n");
}

// -----------------------------------------------------------------------------

void preset_wan(char *ifname, char *gw, char *netmask, char *prefix)
{
	int i;
	char tmp[100];

	/* Delete all default routes */
	route_del(ifname, 0, NULL, NULL, NULL);

	/* try adding a route to gateway first */
	route_add(ifname, 0, gw, NULL, "255.255.255.255");

	/* Set default route to gateway if specified */
	i = 5;
	while ((route_add(ifname, 1, "0.0.0.0", gw, "0.0.0.0") == 1) && (i--)) {
		sleep(1);
	}
	_dprintf("set default gateway=%s n=%d\n", gw, i);

	/* Add routes to dns servers as well for demand ppp to work */
	char word[100], *next;
	in_addr_t mask = inet_addr(netmask);
	foreach(word, nvram_safe_get(strcat_r(prefix, "_get_dns", tmp)), next) {  //"wan_get_dns"
		if ((inet_addr(word) & mask) != (inet_addr(nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp))) & mask))  //"wan_ipaddr"
			route_add(ifname, 0, word, gw, "255.255.255.255");
	}
	if(!strcmp(prefix,"wan")){
		dns_to_resolv();
		start_dnsmasq();
		sleep(1);
		start_firewall();
	}
}

// -----------------------------------------------------------------------------


// Get the IP, Subnetmask, Geteway from WAN interface and set nvram
static void start_tmp_ppp(int num, char *ifname, char *prefix)
{
	int timeout;
	struct ifreq ifr;
	int s;
	
	char tmp[100];

	TRACE_PT("begin: num=%d\n", num);

	if (num != 0) return;

	// Wait for ppp0 to be created
	timeout = 15;
	while ((ifconfig(ifname, IFUP, NULL, NULL) != 0) && (timeout-- > 0)) {
		sleep(1);
		_dprintf("[%d] waiting for %s %d...\n", __LINE__, ifname, timeout);
	}

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) return;
	strlcpy(ifr.ifr_name, ifname, IFNAMSIZ);

	// Set temporary IP address
	timeout = 3;
	while (ioctl(s, SIOCGIFADDR, &ifr) && timeout--){
		_dprintf("[%d] waiting for %s...\n", __LINE__, ifname);
		sleep(1);
	};
	nvram_set(strcat_r(prefix, "_ipaddr", tmp), inet_ntoa(sin_addr(&(ifr.ifr_addr)))); //"wan_ipaddr"
	nvram_set(strcat_r(prefix, "_netmask", tmp), "255.255.255.255"); //"wan_netmask"

	// Set temporary P-t-P address
	timeout = 3;
	while (ioctl(s, SIOCGIFDSTADDR, &ifr) && timeout--){
		_dprintf("[%d] waiting for %s...\n", __LINE__, ifname);
		sleep(1);
	}
	nvram_set(strcat_r(prefix, "_gateway", tmp), inet_ntoa(sin_addr(&(ifr.ifr_dstaddr)))); //"wan_getaway"
	
	close(s);

	start_wan_done(ifname,prefix);
	TRACE_PT("end\n");
}

void start_pppoe(int num, char *prefix)
{
	char ifname[8];
	char tmp[100];

	TRACE_PT("begin pppoe_num=%d\n", num);
	
	if (num < 0 || num >3) return;

	stop_pppoe(prefix);

	snprintf(ifname, sizeof(ifname), "ppp%d", num);

#ifdef LINUX26
#ifdef TCONFIG_USB
	if (nvram_match( strcat_r(prefix, "_proto", tmp), "ppp3g") ) { //wan_proto
		if (config_pppd(WP_PPP3G, num, prefix) != 0)
		return;
	} else {
#endif
#endif
		if (config_pppd(WP_PPPOE, num, prefix) != 0)
		return;
#ifdef LINUX26
#ifdef TCONFIG_USB
	}
#endif
#endif
	run_pppd(prefix);

	if (nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp))){ //"ppp_demand"
		start_tmp_ppp(num, ifname, prefix);
	}
	else {
		ifconfig(ifname, IFUP, NULL, NULL);
	}

	TRACE_PT("end\n");
}

void stop_pppoe(char *prefix)
{
	stop_ppp(prefix);
}

#if 0
void stop_singe_pppoe(int num, char *prefix)
{
	char tmp[100];
	_dprintf("%s pppoe_num=%d\n", __FUNCTION__, num);

	int i;

	if (num != 0) return;
	
	i = nvram_get_int(strcat_r(prefix, "_pppoe_pid0", tmp);  //"pppoe_pid0"
	if ((i > 1) && (kill(i, SIGTERM) == 0)) {
		do {
			sleep(2);
		} while (kill(i, SIGKILL) == 0);
	}

	char ppp_linkfile[256] ;
	memset(ppp_optfile, 0, 256);
	sprintf(ppp_linkfile, "/tmp/ppp/%s_link", prefix);

	unlink(ppp_linkfile);
	nvram_unset(strcat_r(prefix, "_pppoe_ifname0", tmp);  //"pppoe_pid0"

	nvram_set(strcat_r(prefix, "_get_dns", ""); //"wan_get_dns"
	clear_resolv();
}
#endif

// -----------------------------------------------------------------------------

inline void stop_l2tp(char *prefix)
{
	stop_ppp(prefix);
}

void start_l2tp(char *prefix)
{
	char tmp[100];

	TRACE_PT("begin\n");

	FILE *fp;
	int demand;
	
	char ppp_optfile[256];
	memset(ppp_optfile, 0, 256);
	sprintf(ppp_optfile, "/tmp/ppp/%s_options", prefix);
	
	char xl2tp_file[256];

	stop_l2tp(prefix);

	if (config_pppd(WP_L2TP, 0, prefix) != 0)
		return;

	demand = nvram_get_int(strcat_r(prefix, "_ppp_demand", tmp)); //"ppp_demand"

	/* Generate XL2TPD configuration file */
	memset(xl2tp_file, 0, 256);
	sprintf(xl2tp_file, "/etc/%s_xl2tpd.conf", prefix);
	if ((fp = fopen(xl2tp_file, "w")) == NULL)
		return;
	fprintf(fp,
		"[global]\n"
		"access control = no\n"
		"port = 1701\n"
		"[lac l2tp]\n"
		"lns = %s\n"
		"tx bps = 100000000\n"
		"pppoptfile = %s\n"
		"redial = yes\n"
		"max redials = 32767\n"
		"redial timeout = %d\n"
<<<<<<< HEAD
		"tunnel rws = 8\n"
		"ppp debug = %s\n"
		"%s\n",
		nvram_safe_get("l2tp_server_ip"),
		ppp_optfile,
		demand ? 30 : (nvram_get_int("ppp_redialperiod") ? : 30),
		nvram_get_int("debug_ppp") ? "yes" : "no",
		nvram_safe_get("xl2tpd_custom"));
	fappend(fp, "/etc/xl2tpd.custom");
=======
		"ppp debug = %s\n",
		"%s\n",
		nvram_safe_get(strcat_r(prefix, "_l2tp_server_ip", tmp)),  //"l2tp_server_ip"
		ppp_optfile,
		demand ? 30 : (nvram_get_int(strcat_r(prefix, "_ppp_redialperiod", tmp)) ? : 30),  //"ppp_redialperiod"
		nvram_get_int(strcat_r(prefix, "_debug_ppp", tmp)) ? "yes" : "no",  //"debug_ppp"
	nvram_safe_get(strcat_r(prefix, "_xl2tpd_custom", tmp))); //"xl2tpd_custom"
	
	memset(xl2tp_file, 0, 256);
	sprintf(xl2tp_file, "/etc/%s_xl2tpd.custom", prefix);
	fappend(fp, xl2tp_file);
>>>>>>> tomato-shibby
	fclose(fp);

	enable_ip_forward();

	eval("xl2tpd");

	if (demand) {
		eval("listen", nvram_safe_get("lan_ifname"), prefix);
	}
	else {
		force_to_dial(prefix);
		start_redial(prefix);
	}

	TRACE_PT("end\n");
}

// -----------------------------------------------------------------------------

char *wan_gateway(char *prefix)
{
	char tmp[100];
	char *gw = nvram_safe_get(strcat_r(prefix, "_gateway_get", tmp)); //"wan_getway_get"
	if ((*gw == 0) || (strcmp(gw, "0.0.0.0") == 0))
		gw = nvram_safe_get(strcat_r(prefix, "_gateway", tmp)); //"wan_getway"
	return gw;
}

// -----------------------------------------------------------------------------

// trigger connect on demand
void force_to_dial(char *prefix)
{
	char l2tp_file[256];

	TRACE_PT("begin\n");

	sleep(1);
	switch (get_wanx_proto(prefix)) {
	case WP_L2TP:
		memset(l2tp_file, 0, 256);
		sprintf(l2tp_file, "/var/run/%s_l2tp-control", prefix);
		f_write_string(l2tp_file, "c l2tp", 0, 0);
		break;
	case WP_PPTP:
		eval("ping", "-c", "2", "10.112.112.112");
		break;
	case WP_DISABLED:
	case WP_STATIC:
		break;
	default:
		eval("ping", "-c", "2", wan_gateway(prefix));
		break;
	}
	
	TRACE_PT("end\n");
}

// -----------------------------------------------------------------------------

static void _do_wan_routes(char *ifname, char *nvname, int metric, int add)
{
	char *routes, *tmp;
	int bits;
	struct in_addr mask;
	char netmask[16];

	// IP[/MASK] ROUTER IP2[/MASK2] ROUTER2 ...
	tmp = routes = strdup(nvram_safe_get(nvname));
	while (tmp && *tmp) {
		char *ipaddr, *gateway, *nmask;

		ipaddr = nmask = strsep(&tmp, " ");
		strcpy(netmask, "255.255.255.255");

		if (nmask) {
			ipaddr = strsep(&nmask, "/");
			if (nmask && *nmask) {
				bits = strtol(nmask, &nmask, 10);
				if (bits >= 1 && bits <= 32) {
					mask.s_addr = htonl(0xffffffff << (32 - bits));
					strcpy(netmask, inet_ntoa(mask));
				}
			}
		}
		gateway = strsep(&tmp, " ");

		if (gateway && *gateway) {
			if (add)
				route_add(ifname, metric, ipaddr, gateway, netmask);
			else
				route_del(ifname, metric, ipaddr, gateway, netmask);
		}
	}
	free(routes);
}

void do_wan_routes(char *ifname, int metric, int add, char *prefix )
{
	if (nvram_get_int("dhcp_routes")) {
		char tmp[100];
		// Static Routes:		IP ROUTER IP2 ROUTER2 ...
		// Classless Static Routes:	IP/MASK ROUTER IP2/MASK2 ROUTER2 ...
		_do_wan_routes(ifname, strcat_r(prefix, "_routes1", tmp), metric, add);  //"wan_routes1"
		_do_wan_routes(ifname, strcat_r(prefix, "_routes2", tmp), metric, add);  //"wan_routes2"
	}
}

// -----------------------------------------------------------------------------

//const char wan_connecting[] = "/var/lib/misc/wan.connecting";

static int is_sta(int idx, int unit, int subunit, void *param)
{
	char **p = param;

	if (nvram_match(wl_nvname("mode", unit, subunit), "sta")) {
		*p = nvram_safe_get(wl_nvname("ifname", unit, subunit));
		return 1;
	}
	return 0;	
}

void start_wan_if(int mode, char *prefix)
{
	int wan_proto;
	char *wan_ifname;
	char *p = NULL;
	char *w = NULL;
	struct ifreq ifr;
	int sd;
	int max;
	int mtu;
	char buf[128];
	int vid;
	int vid_map;
	int vlan0tag;
	int wan_unit;
	
	char wanconn_file[256];
	char tmp[100];

	TRACE_PT("begin\n");
	
	wan_unit = get_wan_unit(prefix);
	
	mwanlog(LOG_DEBUG, "start %s.", prefix);

	memset(wanconn_file, 0, 256);
	sprintf(wanconn_file, "/var/lib/misc/%s.connecting", prefix);
	f_write(wanconn_file, NULL, 0, 0, 0);

	/*
	if(!strcmp(prefix,"wan")){
		if (!foreach_wif(1, &p, is_sta)) {
			p = nvram_safe_get("wan_ifnameX"); //"wan_ifnameX"
			if (sscanf(p, "vlan%d", &vid) == 1) {
				vlan0tag = nvram_get_int("vlan0tag");
				snprintf(buf, sizeof(buf), "vlan%dvid", vid);
				vid_map = nvram_get_int(buf);
				if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | vid;
				snprintf(buf, sizeof(buf), "vlan%d", vid_map);
				p = buf;
			}
			//set_mac(p, "mac_wan", 1);
		}
	}
	*/
	
	//
	// shibby fix wireless client
	if (nvram_invmatch(strcat_r(prefix, "_sta", tmp), "")) { //wireless client as wan
		w = nvram_safe_get(strcat_r(prefix, "_sta", tmp));
		p = nvram_safe_get(strcat_r(w, "_ifname", tmp));
	} else {
		p = nvram_safe_get(strcat_r(prefix, "_ifnameX", tmp));
	}
	nvram_set(strcat_r(prefix, "_ifname", tmp), p);  //"wan_ifname"
	nvram_set(strcat_r(prefix, "_ifnames", tmp), p); //"wan_ifnames
	set_mac(p, strcat_r(prefix, "_mac", tmp), wan_unit + 15); //set_mac(p, "mac_wan", 1);

	wan_ifname = nvram_safe_get(strcat_r(prefix, "_ifname", tmp)); //"wan_ifname"
	if (wan_ifname[0] == 0) {
		wan_ifname = "none";
		nvram_set(strcat_r(prefix, "_ifname", tmp), wan_ifname); //"wan_ifname"
	}

	if (strcmp(wan_ifname, "none") == 0) {
		nvram_set(strcat_r(prefix, "_proto", tmp), "disabled");  //"wan_proto"
		syslog(LOG_WARNING, "%s ifname is NONE, please check you vlan settings!", prefix);
	}
	
	//	
	wan_proto = get_wanx_proto(prefix);

	// set the default gateway for WAN interface
	nvram_set(strcat_r(prefix, "_gateway_get", tmp), nvram_safe_get(strcat_r(prefix, "_gateway", tmp))); //"wan_getway_get","wan_gateway"

	if (wan_proto == WP_DISABLED) {
		start_wan_done(wan_ifname,prefix);
		return;
	}

	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		return;
	}
	
	// MTU

	switch (wan_proto) {
	case WP_PPPOE:
	case WP_PPP3G:
		max = 1492;
		break;
	case WP_PPTP:
	case WP_L2TP:
		max = 1460;
		break;
	default:
		max = 1500;
		break;
	}
	if (nvram_match(strcat_r(prefix, "_mtu_enable", tmp), "0")) {  //"mtu_enable"
		mtu = max;
	}
	else {
		// KDB If we've big fat frames enabled then we *CAN* break the
		// max MTU on PPP link
		mtu = nvram_get_int(strcat_r(prefix, "_mtu", tmp));  //"wan_mtu"
		if (!(nvram_get_int("jumbo_frame_enable")) && (mtu > max)) mtu = max;
			else if (mtu < 576) mtu = 576;
	}
	sprintf(buf, "%d", mtu);
	nvram_set(strcat_r(prefix, "_mtu", tmp), buf);  //"wan_mtu"
	nvram_set(strcat_r(prefix, "_run_mtu", tmp), buf);  //"wan_run_mtu"

	// 43011: zhijian 2006-12-25 for CD-Router v3.4 mtu bug of PPTP connection mode
/*	if (wan_proto == WP_PPTP) {
		mtu += 40;
	} */	// commented out; checkme -- zzz
	
	if (wan_proto != WP_PPTP && wan_proto != WP_L2TP && wan_proto != WP_PPPOE && wan_proto != WP_PPP3G) {
		// Don't set the MTU on the port for PPP connections, it will be set on the link instead
		ifr.ifr_mtu =  mtu;
		strcpy(ifr.ifr_name, wan_ifname);
		ioctl(sd, SIOCSIFMTU, &ifr);
	}

	//
	
	ifconfig(wan_ifname, IFUP, NULL, NULL);

	switch (wan_proto) {
	case WP_PPPOE:
	case WP_PPP3G:
		if(!strcmp(prefix,"wan")) start_pppoe(PPPOEWAN, prefix);
		if(!strcmp(prefix,"wan2")) start_pppoe(PPPOEWAN2, prefix);
#ifdef TCONFIG_MULTIWAN
		if(!strcmp(prefix,"wan3")) start_pppoe(PPPOEWAN3, prefix);
		if(!strcmp(prefix,"wan4")) start_pppoe(PPPOEWAN4, prefix);
#endif

		break;
	case WP_DHCP:
	case WP_LTE:
	case WP_L2TP:
	case WP_PPTP:
		if (wan_proto == WP_LTE) {
			// prepare LTE modem
			xstart("switch4g", prefix);
		}
		else if (using_dhcpc(prefix)) {
			stop_dhcpc(prefix);
			start_dhcpc(prefix);
		}
		else if (wan_proto != WP_DHCP && wan_proto != WP_LTE) {
			ifconfig(wan_ifname, IFUP, "0.0.0.0", NULL);
			ifconfig(wan_ifname, IFUP, nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)), nvram_safe_get(strcat_r(prefix, "_netmask", tmp)));  //"wan_ipaddr","wan_netmask"

			p = nvram_safe_get(strcat_r(prefix, "_gateway", tmp));  //"wan_getaway"
			if ((*p != 0) && (strcmp(p, "0.0.0.0") != 0))
				preset_wan(wan_ifname, p, nvram_safe_get(strcat_r(prefix, "_netmask", tmp)), prefix);  //"wan_netmask"

			switch (wan_proto) {
			case WP_PPTP:
				start_pptp(mode,prefix);
				break;
			case WP_L2TP:
				start_l2tp(prefix);
				break;
			}
		}
		break;
	default:	// static
		nvram_set(strcat_r(prefix, "_iface", tmp), wan_ifname);  //"wan_iface"
		ifconfig(wan_ifname, IFUP, nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)), nvram_safe_get(strcat_r(prefix, "_netmask", tmp))); //"wan_ipaddr","wan_netmask"
		
		int r = 10;
		while ((!check_wanup(prefix)) && (r-- > 0)) {
			sleep(1);
		}
		
		start_wan_done(wan_ifname,prefix);
		break;
	}

	// Get current WAN hardware address
	strlcpy(ifr.ifr_name, wan_ifname, IFNAMSIZ);
	if (ioctl(sd, SIOCGIFHWADDR, &ifr) == 0) {
		nvram_set(strcat_r(prefix, "_hwaddr", tmp), ether_etoa(ifr.ifr_hwaddr.sa_data, buf));  //"wan_hwaddr"
	}

	/* Set initial QoS mode again now that WAN port is ready. */
	set_et_qos_mode(sd);

	close(sd);

	if(nvram_get_int("mwan_cktime") > 0)
		xstart("watchdog", prefix, "add");

	TRACE_PT("end\n");
}

void start_wan(int mode)
{
	int mwan_num;
	int wan_unit;
	char prefix[] = "wanXX";

	mwan_num = atoi(nvram_safe_get("mwan_num"));
	if(mwan_num < 1 || mwan_num > MWAN_MAX){
		mwan_num = 1;
	}

	syslog(LOG_INFO, "MultiWAN: MWAN is %d.", mwan_num);
	for(wan_unit = 1; wan_unit <= mwan_num; ++wan_unit)
	{
		get_wan_prefix(wan_unit, prefix);
		start_wan_if(mode, prefix);
	}

	stop_wireless();
	start_wireless();
	start_firewall();
	set_host_domain_name();

	enable_ip_forward();

	killall_tk("mwanroute");
	xstart("mwanroute");

	led(LED_DIAG, 0);	// for 4712, 5325E (?)
	led(LED_DMZ, nvram_match("dmz_enable", "1"));
}

#ifdef TCONFIG_IPV6
void start_wan6_done(const char *wan_ifname)
{
	struct in_addr addr4;
	struct in6_addr addr;
	static char addr6[INET6_ADDRSTRLEN];

	int service = get_ipv6_service();

	if (service != IPV6_DISABLED) {
		if ((nvram_get_int("ipv6_accept_ra") & 1) != 0)
			accept_ra(wan_ifname);
	}

	switch (service) {
	case IPV6_NATIVE:
		eval("ip", "route", "add", "::/0", "dev", (char *)wan_ifname, "metric", "2048");
		break;
	case IPV6_NATIVE_DHCP:
		if (nvram_get_int("ipv6_pdonly") == 1) {
			eval("ip", "route", "add", "::/0", "dev", (char *)wan_ifname);
		}
		stop_dhcp6c();
		start_dhcp6c();
		break;
	case IPV6_ANYCAST_6TO4:
	case IPV6_6IN4:
		stop_ipv6_tunnel();
		if (service == IPV6_ANYCAST_6TO4) {
			addr4.s_addr = 0;
			memset(&addr, 0, sizeof(addr));
			inet_aton(get_wanip("wan"), &addr4);
			addr.s6_addr16[0] = htons(0x2002);
			ipv6_mapaddr4(&addr, 16, &addr4, 0);
			addr.s6_addr16[3] = htons(0x0001);
			inet_ntop(AF_INET6, &addr, addr6, sizeof(addr6));
			nvram_set("ipv6_prefix", addr6);
		}
		start_ipv6_tunnel();
		// FIXME: give it a few seconds for DAD completion
		sleep(2);
		break;
	case IPV6_6RD:
	case IPV6_6RD_DHCP:
		stop_6rd_tunnel();
		start_6rd_tunnel();
		// FIXME2?: give it a few seconds for DAD completion
		sleep(2);
		break;
	}
}
#endif

//	ppp_demand: 0=keep alive, 1=connect on demand (run 'listen')
//	wan_ifname: vlan1
//	wan_iface:	ppp# (PPPOE, PPP3G, PPTP, L2TP), vlan1 (DHCP, HB, Static, LTE)

void start_wan_done(char *wan_ifname, char *prefix)
{
	int proto;
	int n;
	char *gw;
	struct sysinfo si;
	int wanup;
	
	char wantime_file[256];
	char wanconn_file[256];
	char tmp[100];

	TRACE_PT("begin %s_ifname=%s\n", prefix, wan_ifname);

	sysinfo(&si);
	
	memset(wantime_file, 0, 256);
	sprintf(wantime_file, "/var/lib/misc/%s_time", prefix);	
	f_write(wantime_file, &si.uptime, sizeof(si.uptime), 0, 0);

	proto = get_wanx_proto(prefix); //proto = get_wanx_proto(prefix);

	mwanlog(LOG_DEBUG, "start_wan_done, interface=%s, wan_prefix=%s, proto=%d", wan_ifname, prefix, proto);

	// delete all default routes
	route_del(wan_ifname, 0, NULL, NULL, NULL);

	if (proto != WP_DISABLED) {

		// set default route to gateway if specified
		gw = wan_gateway(prefix);
#if 0
		if (proto == WP_PPTP && !using_dhcpc(prefix)) {
			// For PPTP protocol, we must use ppp_get_ip as gateway, not pptp_server_ip (why ??)
			if (*gw == 0 || strcmp(gw, "0.0.0.0") == 0) gw = nvram_safe_get(strcat_r(prefix, "_ppp_get_ip", tmp));  //"ppp_get_ip"
		}
#endif
		if ((*gw != 0) && (strcmp(gw, "0.0.0.0") != 0)) {
			if (proto == WP_DHCP || proto == WP_STATIC || proto == WP_LTE) {
				// possibly gateway is over the bridge, try adding a route to gateway first
				route_add(wan_ifname, 0, gw, NULL, "255.255.255.255");
			}

			n = 5;
			while ((route_add(wan_ifname, 0, "0.0.0.0", gw, "0.0.0.0") == 1) && (n--)) {
				sleep(1);
			}
			_dprintf("set default gateway=%s n=%d\n", gw, n);

			// hack: avoid routing cycles, when both peer and server have the same IP
			if (proto == WP_PPTP || proto == WP_L2TP) {
				// delete gateway route as it's no longer needed
				route_del(wan_ifname, 0, gw, "0.0.0.0", "255.255.255.255");
			}
		}

#ifdef THREE_ARP_GRATUATOUS_SUPPORT	// from 43011; checkme; commented-out	-- zzz
/*
		// 43011: Alpha add to send Gratuitous ARP when wan_proto is Static IP 2007-04-09
		if (proto == WP_STATIC)
		{
			int ifindex;
			u_int32_t wan_ip;
			unsigned char wan_mac[6];

			if (read_iface(nvram_safe_get("wan_iface"), &ifindex, &wan_ip, wan_mac) >= 0)
				arpping(wan_ip, wan_ip, wan_mac, nvram_safe_get("wan_iface"));
		}
*/
#endif
		if (proto == WP_PPTP || proto == WP_L2TP) {
			route_del(nvram_safe_get(strcat_r(prefix, "_iface", tmp)), 0, nvram_safe_get(strcat_r(prefix, "_gateway_get", tmp)), NULL, "255.255.255.255"); //"wan_iface","wan_gateway_get"
			route_add(nvram_safe_get(strcat_r(prefix, "_iface", tmp)), 0, nvram_safe_get(strcat_r(prefix, "_ppp_get_ip", tmp)), NULL, "255.255.255.255"); //"wan_iface","ppp_get_ip"
		}
		if (proto == WP_L2TP) {
			route_add(nvram_safe_get(strcat_r(prefix, "_ifname", tmp)), 0, nvram_safe_get(strcat_r(prefix, "_l2tp_server_ip", tmp)), nvram_safe_get(strcat_r(prefix, "_gateway", tmp)), "255.255.255.255"); // fixed routing problem in Israel by kanki
			// "wan_ifname","l2tp_server_ip","wan_getaway"
		}
	}

	dns_to_resolv();
	start_dnsmasq();

	start_firewall();
	start_qos(prefix);

	do_static_routes(1);
	// and routes supplied via DHCP
	do_wan_routes(using_dhcpc(prefix) ? nvram_safe_get(strcat_r(prefix, "_ifname",tmp)) : wan_ifname, 0, 1, prefix); //"wan_ifname"
	if(!strcmp(prefix,"wan")){
		stop_zebra();
		start_zebra();

		wanup = check_wanup(prefix);
	
		if ((wanup) || (time(0) < Y2K)) {
			stop_ntpc();
			start_ntpc();
		}

		if ((wanup) || (proto == WP_DISABLED)) {
			stop_ddns();
			start_ddns();
			stop_igmp_proxy();
			stop_udpxy();
			start_igmp_proxy();
			start_udpxy();
		}

#ifdef TCONFIG_IPV6
		start_wan6_done(get_wan6face());
#endif

#ifdef TCONFIG_DNSSEC
		if (nvram_match("dnssec_enable", "1")) {
			killall("dnsmasq", SIGHUP);
		}
#endif

		stop_upnp();
		start_upnp();

		// restart httpd
		start_httpd();

		if (wanup) {
			SET_LED(GOT_IP);
			notice_set("wan", "");

			run_nvscript("script_wanup", NULL, 0);
		}

		// We don't need STP after wireless led is lighted		//	no idea why... toggling it if necessary	-- zzz
		if (check_hw_type() == HW_BCM4702) {
			eval("brctl", "stp", nvram_safe_get("lan_ifname"), "0");
			if (nvram_match("lan_stp", "1")) 
				eval("brctl", "stp", nvram_safe_get("lan_ifname"), "1");
			if(strcmp(nvram_safe_get("lan1_ifname"),"")!=0) {
				eval("brctl", "stp", nvram_safe_get("lan1_ifname"), "0");
				if (nvram_match("lan1_stp", "1")) 
					eval("brctl", "stp", nvram_safe_get("lan1_ifname"), "1");
			}
			if(strcmp(nvram_safe_get("lan2_ifname"),"")!=0) {
				eval("brctl", "stp", nvram_safe_get("lan2_ifname"), "0");
				if (nvram_match("lan2_stp", "1")) 
					eval("brctl", "stp", nvram_safe_get("lan2_ifname"), "1");
			}
			if(strcmp(nvram_safe_get("lan3_ifname"),"")!=0) {
				eval("brctl", "stp", nvram_safe_get("lan3_ifname"), "0");
				if (nvram_match("lan3_stp", "1")) 
					eval("brctl", "stp", nvram_safe_get("lan3_ifname"), "1");
			}
		}

		if (wanup)
			start_vpn_eas();

#ifdef TCONFIG_TINC
		if(wanup)
			start_tinc_wanup();
#endif

#ifdef TCONFIG_PPTPD
		if (wanup && nvram_get_int("pptp_client_enable"))
			start_pptp_client();
#endif

		new_qoslimit_start(); //!! RAF
	}

	mwan_table_add(prefix);
	mwan_load_balance();

	memset(wanconn_file, 0, 256);
	sprintf(wanconn_file, "/var/lib/misc/%s.connecting", prefix);
	unlink(wanconn_file);

	TRACE_PT("end\n");
}

void stop_wan_if(char *prefix)
{
	char name[80];
	char *next;
	int wan_proto;

	char tmp[100];
	char wanconn_file[256];
	char wannotice_file[256];

	mwan_table_del(prefix);

	TRACE_PT("begin\n");

	stop_qos(prefix);
	/* Kill any WAN client daemons or callbacks */
	stop_redial(prefix);
	stop_pppoe(prefix);
	// stop_ppp(prefix);
	stop_dhcpc(prefix);
	nvram_set(strcat_r(prefix, "_get_dns", tmp), ""); //"wan_get_dns"

	wan_proto = get_wanx_proto(prefix);

	if (wan_proto == WP_LTE) {
		xstart("switch4g", prefix, "disconnect");
	}

	/* Bring down WAN interfaces */
	foreach(name, nvram_safe_get(strcat_r(prefix, "_ifnames", tmp)), next)  //"wan_ifnames"
		ifconfig(name, 0, "0.0.0.0", NULL);

	//notice_set(prefix, "");
	memset(wannotice_file, 0, 256);
	sprintf(wannotice_file, "/var/notice/%s", prefix);
	memset(wanconn_file, 0, 256);
	sprintf(wanconn_file, "/var/lib/misc/%s.connecting", prefix);
	unlink(wannotice_file);
	unlink(wanconn_file);

	mwan_load_balance();

	xstart("watchdog", prefix, "del");

	TRACE_PT("end\n");
}

void stop_wan(void)
{

#ifdef TCONFIG_TINC
	stop_tinc();
#endif

#ifdef TCONFIG_PPTPD
	stop_pptp_client();
	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();
#endif

	new_qoslimit_stop(); //!! RAF

	//stop_qos();
	stop_upnp();	//!!TB - moved from stop_services()
	stop_firewall();
	stop_igmp_proxy();
	stop_udpxy();
	stop_ntpc();

#ifdef TCONFIG_IPV6
	stop_ipv6_tunnel();
	stop_dhcp6c();
	nvram_set("ipv6_get_dns", "");
#endif

	stop_vpn_eas();
	clear_resolv();
	stop_wan_if("wan");
	stop_wan_if("wan2");
#ifdef TCONFIG_MULTIWAN
	stop_wan_if("wan3");
	stop_wan_if("wan4");
#endif

	SET_LED(RELEASE_IP);
}
