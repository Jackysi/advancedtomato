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


#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)


static void make_secrets(void)
{
	FILE *f;
	char *user;
	char *pass;
	
	user = nvram_safe_get("ppp_username");
	pass = nvram_safe_get("ppp_passwd");
	if ((f = fopen("/tmp/ppp/pap-secrets", "w")) != NULL) {
		fprintf(f, "\"%s\" * \"%s\" *\n", user, pass);
		fclose(f);
	}
	chmod("/tmp/ppp/pap-secrets", 0600);

	if ((f = fopen("/tmp/ppp/chap-secrets", "w")) != NULL) {
		fprintf(f, "\"%s\" * \"%s\" *\n", user, pass);
		fclose(f);
	}
	chmod("/tmp/ppp/chap-secrets", 0600);
}


// -----------------------------------------------------------------------------

int start_pptp(int mode)
{
	_dprintf("%s: begin\n", __FUNCTION__);

	FILE *fp;
	char username[80];
	char passwd[80];

	stop_dhcpc();
	stop_pppoe();

	strlcpy(username, nvram_safe_get("ppp_username"), sizeof(username));
	strlcpy(passwd, nvram_safe_get("ppp_passwd"), sizeof(passwd));

	if (mode != REDIAL) {
		mkdir("/tmp/ppp", 0777);
		symlink("/sbin/rc", "/tmp/ppp/ip-up");
		symlink("/sbin/rc", "/tmp/ppp/ip-down");
		symlink("/dev/null", "/tmp/ppp/connect-errors");

		// Generate options file
		if ((fp = fopen("/tmp/ppp/options", "w")) == NULL) {
			perror("/tmp/ppp/options");
			return -1;
		}
		
		fprintf(fp, "defaultroute\n");		// Add a default route to the system routing tables, using the peer as the gateway
		fprintf(fp, "usepeerdns\n");		// Ask the peer for up to 2 DNS server addresses
		fprintf(fp, "pty 'pptp %s --nolaunchpppd'\n", nvram_safe_get("pptp_server_ip"));
		fprintf(fp, "user '%s'\n", username);
		//fprintf(fp, "persist\n");			// Do not exit after a connection is terminated.

		fprintf(fp, "mtu %s\n",nvram_safe_get("wan_mtu"));

		if (nvram_match("ppp_demand", "1")) {
			//demand mode
			fprintf(fp, "idle %d\n", nvram_get_int("ppp_idletime") * 60);
			fprintf(fp, "demand\n");				// Dial on demand
			fprintf(fp, "persist\n");				// Do not exit after a connection is terminated.
			//43011: fprintf(fp, "%s:%s\n", PPP_PSEUDO_IP, PPP_PSEUDO_GW);	// <local IP>:<remote IP>
			fprintf(fp, "ipcp-accept-remote\n");
			fprintf(fp, "ipcp-accept-local\n");
			fprintf(fp, "connect true\n");
			fprintf(fp, "noipdefault\n");			// Disables  the  default  behaviour when no local IP address is specified
			fprintf(fp, "ktune\n");					// Set /proc/sys/net/ipv4/ip_dynaddr to 1 in demand mode if the local address changes
		}
		else {
			// keepalive mode
			start_redial();
		}

		fprintf(fp, "default-asyncmap\n");		// Disable  asyncmap  negotiation
		fprintf(fp, "nopcomp\n");				// Disable protocol field compression
		fprintf(fp, "noaccomp\n");				// Disable Address/Control compression
		fprintf(fp, "noccp\n");					// Disable CCP (Compression Control Protocol)
		fprintf(fp, "novj\n");					// Disable Van Jacobson style TCP/IP header compression
		fprintf(fp, "nobsdcomp\n");				// Disables BSD-Compress  compression
		fprintf(fp, "nodeflate\n");				// Disables Deflate compression
		fprintf(fp, "lcp-echo-interval 0\n");	// Don't send an LCP echo-request frame to the peer
		fprintf(fp, "lock\n");
		fprintf(fp, "noauth\n");
		
		if (nvram_match("debug_pppd", "1")) {
			fprintf(fp, "debug\n");
		}
		
		fclose(fp);

		make_secrets();
	}

	// Bring up  WAN interface
	ifconfig(nvram_safe_get("wan_ifname"), IFUP,
		nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));

	eval("pppd");

	if (nvram_match("ppp_demand", "1")) {
#if 1	// 43011: added by crazy 20070720
		/*
		   Fixed issue id 7887(or 7787):
		   When DUT is PPTP Connect on Demand mode, it couldn't be trigger from LAN.
		*/
		stop_dnsmasq();
		dns_to_resolv();
		start_dnsmasq();
#endif
	
		// Trigger Connect On Demand if user ping pptp server
		eval("listen", nvram_safe_get("lan_ifname"));
	}

	_dprintf("%s: end\n", __FUNCTION__);
	return 0;
}

int stop_pptp(void)
{
	_dprintf("%s: begin\n", __FUNCTION__);

	unlink("/tmp/ppp/link");

	while ((killall("pppd", SIGKILL) == 0) || (killall("pptp", SIGKILL) == 0) || (killall("listen", SIGKILL) == 0)) {
		sleep(1);
	}

	_dprintf("%s: end\n", __FUNCTION__);
	return 0;
}


// -----------------------------------------------------------------------------


// Get the IP, Subnetmask, Geteway from WAN interface and set nvram
static void start_tmp_ppp(int num)
{
	int timeout;
	char *ifname;
	struct ifreq ifr;
	int s;

	_dprintf("%s: num=%d\n", __FUNCTION__, num);

	if (num != 0) return;

	// Wait for ppp0 to be created
	timeout = 15;
	while ((ifconfig(ifname = nvram_safe_get("pppoe_ifname0"), IFUP, NULL, NULL) != 0) && (timeout-- > 0)) {
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
	nvram_set("wan_ipaddr", inet_ntoa(sin_addr(&(ifr.ifr_addr))));
	nvram_set("wan_netmask", "255.255.255.255");

	// Set temporary P-t-P address
	timeout = 3;
	while (ioctl(s, SIOCGIFDSTADDR, &ifr) && timeout--){
		_dprintf("[%d] waiting for %s...\n", __LINE__, ifname);
		sleep(1);
	}
	nvram_set("wan_gateway", inet_ntoa(sin_addr(&(ifr.ifr_dstaddr))));
	
	close(s);

	start_wan_done(ifname);
}


void start_pppoe(int num)
{
	pid_t pid;
	char idle[16];
	char retry[16];
	char lcp_echo_interval[16];
	char lcp_echo_fails[16];
	char *mtu;
	int dod;
	int n;
	
	_dprintf("%s pppoe_num=%d\n", __FUNCTION__, num);
	
	if (num != 0) return;

	stop_pppoe();

	nvram_set("pppoe_ifname0", "");
	
	dod = nvram_match("ppp_demand", "1");
	
	// -i
	sprintf(idle, "%d", dod ? (nvram_get_int("ppp_idletime") * 60) : 0);
	
	// [-N]
	sprintf(retry, "%d", (nvram_get_int("ppp_redialperiod") / 5) - 1);
	
	// [-r] [-t]
	mtu = nvram_safe_get("wan_mtu");

	// [-I n] Interval between LCP echo-requests
	sprintf(lcp_echo_interval, "%d", ((n = nvram_get_int("pppoe_lei")) > 0) ? n : 30);
	
	// [-T n] Tolerance to unanswered echo-requests
	sprintf(lcp_echo_fails, "%d", ((n = nvram_get_int("pppoe_lef")) > 0) ? n : 5);
	
	char *pppoe_argv[] = {
			"pppoecd",
			nvram_safe_get("wan_ifname"),
			"-u", nvram_safe_get("ppp_username"),
			"-p", nvram_safe_get("ppp_passwd"),
			"-r", mtu,
			"-t", mtu,
			"-i", idle,					// >0 == dial on demand
			"-I", lcp_echo_interval,	// Send an LCP echo-request frame to the server every X seconds
			"-N", retry,				// To avoid kill pppd when pppd has been connecting.
			"-T", lcp_echo_fails,		// pppd will presume the server to be dead if 3 LCP echo-requests are sent without receiving a valid LCP echo-reply
			"-P", "0",					// PPPOE session number.
			"-C", "pppoe_down",			// by tallest 0407
			"-R",			// set default route
			NULL,			// debug
			NULL, NULL,		// pppoe_service
			NULL, NULL,		// pppoe_ac
			NULL, NULL,		// static IP
			NULL,			// pppoe_keepalive
			NULL,			// -x extended logging
			NULL
	};
	char **arg;
	char *p;
	
	for (arg = pppoe_argv; *arg; arg++) {
		//
	}

	if (nvram_match("debug_pppoe", "1")) {
		*arg++ = "-d";		// debug mode; compile ppp w/ -DDEBUG	!
	}		

	if (((p = nvram_get("ppp_service")) != NULL) && (*p != 0)) {
		*arg++ = "-s";
		*arg++ = p;
	}

	// ??	zzz
	if (((p = nvram_get("ppp_ac")) != NULL) && (*p != 0)) {
		*arg++ = "-a";
		*arg++ = p;
	}
	
	if (nvram_match("ppp_static", "1")) {
		*arg++ = "-L";
		*arg++ = nvram_safe_get("ppp_static_ip");
	}
	
	// ??	zzz
	//if (nvram_match("pppoe_demand", "1") || nvram_match("pppoe_keepalive", "1"))
	*arg++ = "-k";
	
	if (nvram_contains_word("log_events", "pppoe")) *arg++ = "-x";

	mkdir("/tmp/ppp", 0777);
	
	// ??	zzz
	symlink("/sbin/rc", "/tmp/ppp/ip-up");
	symlink("/sbin/rc", "/tmp/ppp/ip-down");
	symlink("/sbin/rc", "/tmp/ppp/set-pppoepid"); // tallest 1219
	
	rename("/tmp/ppp/log", "/tmp/ppp/log.~");

	_eval(pppoe_argv, NULL, 0, &pid);

	if (dod) {
		start_tmp_ppp(num);
	}
}

void stop_pppoe(void)
{
	_dprintf("%s\n", __FUNCTION__);

	unlink("/tmp/ppp/link");
	nvram_unset("pppoe_ifname0");
	killall_tk("pppoecd");
	killall_tk("ip-up");
	killall_tk("ip-down");
}

void stop_singe_pppoe(int num)
{
	_dprintf("%s pppoe_num=%d\n", __FUNCTION__, num);

	int i;

	if (num != 0) return;
	
	i = nvram_get_int("pppoe_pid0");
	if ((i > 1) && (kill(i, SIGTERM) == 0)) {
		do {
			sleep(2);
		} while (kill(i, SIGKILL) == 0);
	}

	unlink("/tmp/ppp/link");
	nvram_unset("pppoe_ifname0");

	nvram_set("wan_get_dns", "");
	clear_resolv();
}

// -----------------------------------------------------------------------------


void start_l2tp(void)
{
	_dprintf("%s: begin\n", __FUNCTION__);

	int ret;
	FILE *fp;
	char *l2tp_argv[] = { "l2tpd", NULL };
	char l2tpctrl[64];
	char username[80];
	char passwd[80];

	stop_pppoe();
	stop_pptp();

	snprintf(username, sizeof(username), "%s", nvram_safe_get("ppp_username"));
	snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("ppp_passwd"));

	mkdir("/tmp/ppp", 0777);
	symlink("/sbin/rc", "/tmp/ppp/ip-up");
	symlink("/sbin/rc", "/tmp/ppp/ip-down");
	symlink("/dev/null", "/tmp/ppp/connect-errors");

	/* Generate L2TP configuration file */
	if ((fp = fopen("/tmp/l2tp.conf", "w")) == NULL) {
		return;
	}
	fprintf(fp, "global\n");				// Global section
	fprintf(fp, "load-handler \"sync-pppd.so\"\n");	// Load handlers
	fprintf(fp, "load-handler \"cmd.so\"\n");
	fprintf(fp, "listen-port 1701\n");		// Bind address
	fprintf(fp, "section sync-pppd\n");		// Configure the sync-pppd handler
	fprintf(fp, "section peer\n");			// Peer section
	fprintf(fp, "peer %s\n", nvram_safe_get("l2tp_server_ip"));
	fprintf(fp, "port 1701\n");
	fprintf(fp, "lac-handler sync-pppd\n");
	fprintf(fp, "section cmd\n");			// Configure the cmd handler
	fclose(fp);

	/* Generate options file */
	if ((fp = fopen("/tmp/ppp/options", "w")) == NULL) {
		return;
	}
	fprintf(fp, "defaultroute\n");			// Add a default route to the system routing tables, using the peer as the gateway
	fprintf(fp, "usepeerdns\n");			// Ask the peer for up to 2 DNS server addresses
	//fprintf(fp, "pty 'pptp %s --nolaunchpppd'\n",nvram_safe_get("pptp_server_ip"));
	fprintf(fp, "user '%s'\n", username);
	//fprintf(fp, "persist\n");				// Do not exit after a connection is terminated.

	fprintf(fp, "mtu %s\n",nvram_safe_get("wan_mtu"));

	if (nvram_match("ppp_demand", "1")){	// demand mode
		fprintf(fp, "idle %d\n", nvram_get_int("ppp_idletime") * 60);
		//fprintf(fp, "demand\n");			// Dial on demand
		//fprintf(fp, "persist\n");			// Do not exit after a connection is terminated.
		//fprintf(fp, "%s:%s\n",PPP_PSEUDO_IP,PPP_PSEUDO_GW);   // <local IP>:<remote IP>
		fprintf(fp, "ipcp-accept-remote\n");
		fprintf(fp, "ipcp-accept-local\n");
		fprintf(fp, "connect true\n");
		fprintf(fp, "noipdefault\n");		// Disables  the  default  behaviour when no local IP address is specified
		fprintf(fp, "ktune\n");				// Set /proc/sys/net/ipv4/ip_dynaddr to 1 in demand mode if the local address changes
	}
	else{	// keepalive mode
		if (nvram_match("l2tp_test", "1")) {
			fprintf(fp, "idle %d\n", 0);
			fprintf(fp, "ipcp-accept-remote\n");
			fprintf(fp, "ipcp-accept-local\n");
			fprintf(fp, "connect true\n");
			fprintf(fp, "noipdefault\n");		// Disables  the  default  behaviour when no local IP address is specified
		}
	}

	fprintf(fp, "default-asyncmap\n");		// Disable  asyncmap  negotiation
	fprintf(fp, "nopcomp\n");				// Disable protocol field compression
	fprintf(fp, "noaccomp\n");				// Disable Address/Control compression
	fprintf(fp, "noccp\n");					// Disable CCP (Compression Control Protocol)
	fprintf(fp, "novj\n");					// Disable Van Jacobson style TCP/IP header compression
	fprintf(fp, "nobsdcomp\n");				// Disable BSD-Compress  compression
	fprintf(fp, "nodeflate\n");				// Disable Deflate compression
	fprintf(fp, "lcp-echo-interval 0\n");	// Don't send an LCP echo-request frame to the peer
	fprintf(fp, "lock\n");
	fprintf(fp, "noauth");

	if (nvram_match("debug_pppd", "1")) {
		fprintf(fp, "debug\n");
	}
	fclose(fp);

	make_secrets();
	enable_ip_forward();

	/* Bring up  WAN interface */
	//ifconfig(nvram_safe_get("wan_ifname"), IFUP,
	//	 nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));

	ret = _eval(l2tp_argv, NULL, 0, NULL);
	sleep(1);

	if (nvram_match("ppp_demand", "1")) {
		eval("listen", nvram_safe_get("lan_ifname"));
	}
	else {
		snprintf(l2tpctrl, sizeof(l2tpctrl), "l2tp-control \"start-session %s\"", nvram_safe_get("l2tp_server_ip"));
		system(l2tpctrl);
		_dprintf("%s\n", l2tpctrl);

		start_redial();
	}

	_dprintf("%s: end\n", __FUNCTION__);
}

void stop_l2tp(void)
{
	_dprintf("%s: begin\n", __FUNCTION__);

	unlink("/tmp/ppp/link");
	while ((killall("pppd", SIGKILL) == 0) || (killall("l2tpd", SIGKILL) == 0) || (killall("listen", SIGKILL) == 0)) {
		sleep(1);
	}
	_dprintf("%s: end\n", __FUNCTION__);
}

// -----------------------------------------------------------------------------

// trigger connect on demand
void force_to_dial(void)
{
	char s[64];

	_dprintf("%s: begin\n", __FUNCTION__);

	sleep(1);
	switch (get_wan_proto()) {
	case WP_L2TP:
		snprintf(s, sizeof(s), "/usr/sbin/l2tp-control \"start-session %s\"", nvram_safe_get("l2tp_server_ip"));
		system(s);
		_dprintf("%s\n", s);
		break;
	case WP_PPTP:
		eval("ping", "-c", "2", "10.112.112.112");
		break;
	case WP_DISABLED:
	case WP_STATIC:
		break;
	default:
		eval("ping", "-c", "2", nvram_safe_get("wan_gateway"));
		break;
	}
	
	_dprintf("%s: end\n", __FUNCTION__);
}

// -----------------------------------------------------------------------------

const char wan_connecting[] = "/var/lib/misc/wan.connecting";

void start_wan(int mode)
{
	int wan_proto;
	char *wan_ifname;
	char *p;
	struct ifreq ifr;
	int sd;
	int max;
	int mtu;
	char buf[128];

	f_write(wan_connecting, NULL, 0, 0, 0);
	
	//

	if (nvram_match("wl_mode", "sta")) {
		p = nvram_safe_get("wl_ifname");
	}
	else {
		p = nvram_safe_get("wan_ifnameX");
		set_mac(p, "mac_wan", 1);
	}
	nvram_set("wan_ifname", p);
	nvram_set("wan_ifnames", p);

	//

	wan_ifname = nvram_safe_get("wan_ifname");
	wan_proto = get_wan_proto();

	if (wan_proto == WP_DISABLED) {
		start_wan_done(wan_ifname);
		return;
	}

	if ((sd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		perror("socket");
		return;
	}
	
	// MTU

	switch (wan_proto) {
	case WP_PPPOE:
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
	if (nvram_match("mtu_enable", "0")) {
		mtu = max;
	}
	else {
		mtu = nvram_get_int("wan_mtu");
		if (mtu > max) mtu = max;
			else if (mtu < 576) mtu = 576;
	}
	sprintf(buf, "%d", mtu);
	nvram_set("wan_mtu", buf);
	nvram_set("wan_run_mtu", buf);

	// 43011: zhijian 2006-12-25 for CD-Router v3.4 mtu bug of PPTP connection mode
/*	if (wan_proto == WP_PPTP) {
		mtu += 40;
	} */	// commented out; checkme -- zzz
	
	ifr.ifr_mtu =  mtu;
	strcpy(ifr.ifr_name, wan_ifname);
	ioctl(sd, SIOCSIFMTU, &ifr);

	//
	
	ifconfig(wan_ifname, IFUP, NULL, NULL);
	
	set_host_domain_name();

	switch (wan_proto) {
	case WP_PPPOE:
		start_pppoe(PPPOE0);
		if (nvram_invmatch("ppp_demand", "1")) {
			if (mode != REDIAL) start_redial();
		}
		break;
	case WP_DHCP:
	case WP_L2TP:
		stop_dhcpc();
		start_dhcpc();
		break;
	case WP_PPTP:
		start_pptp(mode);
		break;
	default:	// static
		nvram_set("wan_iface", wan_ifname);
		ifconfig(wan_ifname, IFUP, nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));
		start_wan_done(wan_ifname);
		break;
	}

	// Get current WAN hardware address
	strlcpy(ifr.ifr_name, wan_ifname, IFNAMSIZ);
	if (ioctl(sd, SIOCGIFHWADDR, &ifr) == 0) {
		nvram_set("wan_hwaddr", ether_etoa(ifr.ifr_hwaddr.sa_data, buf));
	}

	close(sd);

	enable_ip_forward();

	led(LED_DIAG, 0);	// for 4712, 5325E (?)
	led(LED_DMZ, nvram_match("dmz_enable", "1"));

	_dprintf("%s: end\n", __FUNCTION__);
}


//	ppp_demand: 0=keep alive, 1=connect on demand (run 'listen')
//	wan_ifname: vlan1
//	wan_iface:	ppp# (PPPOE, PPTP, L2TP), vlan1 (DHCP, HB, Static)


void start_wan_done(char *wan_ifname)
{
	int proto;
	int n;
	char *gw;
	int dod;
	struct sysinfo si;
	int wanup;
		
	TRACE_PT("begin wan_ifname=%s\n", wan_ifname);
	
	sysinfo(&si);
	f_write("/var/lib/misc/wantime", &si.uptime, sizeof(si.uptime), 0, 0);
	
	proto = get_wan_proto();
	dod = nvram_match("ppp_demand", "1");
	
	if (proto == WP_L2TP) {
		while (route_del(nvram_safe_get("wan_ifname"), 0, NULL, NULL, NULL) == 0) {
			//
		}
	}

	// delete all default routes
	while (route_del(wan_ifname, 0, NULL, NULL, NULL) == 0) {
		//
	}

	if (proto != WP_DISABLED) {
		// set default route to gateway if specified
		gw = (proto == WP_PPTP) ? nvram_safe_get("pptp_get_ip") : nvram_safe_get("wan_gateway");
		if ((*gw != 0) && (strcmp(gw, "0.0.0.0") != 0)) {
			n = 5;
			while ((route_add(wan_ifname, 0, "0.0.0.0", gw, "0.0.0.0") == 1) && (n--)) {
				sleep(1);
			}
			_dprintf("set default gateway=%s n=%d\n", gw, n);
		}
		
#ifdef THREE_ARP_GRATUATOUS_SUPPORT	// from 43011; checkme; commented-out	-- zzz
/*
		// 43011: Alpha add to send Gratuitous ARP when wan_proto is Static IP 2007-04-09
		if(nvram_match("wan_proto", "static")) 
		{
			int ifindex;
			u_int32_t wan_ip;
			unsigned char wan_mac[6];

			if (read_iface(nvram_safe_get("wan_iface"), &ifindex, &wan_ip, wan_mac) >= 0)
				arpping(wan_ip, wan_ip, wan_mac, nvram_safe_get("wan_iface"));
		}
*/
#endif
		

		if (proto == WP_PPTP) {
			// For PPTP protocol, we must use pptp_get_ip as gateway, not pptp_server_ip
			route_del(nvram_safe_get("wan_iface"), 0, nvram_safe_get("wan_gateway"), NULL, "255.255.255.255");
			route_del(nvram_safe_get("wan_iface"), 0, nvram_safe_get("pptp_server_ip"), NULL, "255.255.255.255");
			route_add(nvram_safe_get("wan_iface"), 0, nvram_safe_get("pptp_get_ip"), NULL, "255.255.255.255");
		}
		else if (proto == WP_L2TP) {
			route_del(nvram_safe_get("wan_iface"), 0, nvram_safe_get("wan_gateway"), NULL, "255.255.255.255");
			route_add(nvram_safe_get("wan_iface"), 0, nvram_safe_get("l2tp_get_ip"), NULL, "255.255.255.255");
			
#if 1		// 43011: add by crazy 20070803
			/*
			   Fix these issues:
			   1. DUT can't response a L2TP ZLB Control message to L2TP server.
			   2. Configure DUT to be L2TP with Connect on demand in 5 minutes, 
			      but DUT will disconnect L2TP before 5 minutes.
			   3. It also causes DUT could often disconnect from L2TP server in 
			      L2TP Keep Alive mode.
			*/
			struct in_addr l2tp_server_ip, wan_ipaddr_old, wan_netmask;

			if (inet_aton(nvram_safe_get("l2tp_server_ip"), &l2tp_server_ip) &&
				inet_aton(nvram_safe_get("wan_netmask"), &wan_netmask) &&
				inet_aton(nvram_safe_get("wan_ipaddr"), &wan_ipaddr_old)) {
				if ((l2tp_server_ip.s_addr & wan_netmask.s_addr) != (wan_ipaddr_old.s_addr & wan_netmask.s_addr)) {
					// If DUT WAN IP and L2TP server IP are in different subnets, it could need this route.
					route_add(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"), nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); // fixed routing problem in Israel by kanki
				}
			}
			else {
				// Fail to change IP from char to struct, still add this route.
				route_add(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"), nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); // fixed routing problem in Israel by kanki
			}
#else
			route_add(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"), nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); // fixed routing problem in Israel by kanki
#endif
		}
	}

	dns_to_resolv();
	start_dnsmasq();

	start_firewall();
	start_qos();

	stop_igmp_proxy();
	start_igmp_proxy();
	
	do_static_routes(1);

	stop_zebra();
	start_zebra();

	stop_upnp();
	start_upnp();

	wanup = check_wanup();
	
	if ((wanup) || (time(0) < Y2K)) {
		stop_ntpc();
		start_ntpc();
	}

	if ((wanup) || (proto == WP_DISABLED)) {
		stop_ddns();
		start_ddns();
	}

	if (wanup) {
		SET_LED(GOT_IP);
		notice_set("wan", "");
		
		run_nvscript("script_wanup", NULL, 0);
	}
	
	// We don't need STP after wireless led is lighted		//	no idea why... toggling it if necessary	-- zzz
	if (check_hw_type() == HW_BCM4702) {
		eval("brctl", "stp", nvram_safe_get("lan_ifname"), "0");
		if (nvram_match("lan_stp", "1")) eval("brctl", "stp", nvram_safe_get("lan_ifname"), "1");
	}

	unlink(wan_connecting);

	TRACE_PT("end\n");
}

void stop_wan(void)
{
	char name[80];
	char *next;
	
	_dprintf("%s: begin\n", __FUNCTION__);

	stop_qos();
	stop_firewall();
	stop_igmp_proxy();
	stop_ntpc();
	
	/* Kill any WAN client daemons or callbacks */
	stop_singe_pppoe(PPPOE0);
	stop_pppoe();
	stop_l2tp();
	stop_dhcpc();
	stop_pptp();
	stop_redial();
	nvram_set("wan_get_dns", "");

	/* Bring down WAN interfaces */
	foreach(name, nvram_safe_get("wan_ifnames"), next)
		ifconfig(name, 0, "0.0.0.0", NULL);

	SET_LED(RELEASE_IP);
	notice_set("wan", "");
	unlink(wan_connecting);

	_dprintf("%s: end\n", __FUNCTION__);
}
