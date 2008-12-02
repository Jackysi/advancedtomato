
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
 * Miscellaneous services
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: services.c,v 1.113.10.5.2.5 2006/04/27 08:50:46 honor Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <wait.h>
#include <net/route.h>
#include <sys/types.h>
#include <signal.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <utils.h>
#include <code_pattern.h>
#include <rc.h>
#include <build_date.h>
#include <cy_conf.h>
#include <nvparse.h>

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)
                                                                                                                             
#ifdef MPPPOE_SUPPORT
extern char pppoe_in_use;       //tallest 1216
#endif

int
adjust_dhcp_range(void)
{
	struct in_addr ipaddr, netaddr, netmask;

	char *lan_ipaddr = nvram_safe_get("lan_ipaddr");
	char *lan_netmask = nvram_safe_get("lan_netmask");
	char *dhcp_num = nvram_safe_get("dhcp_num");
	char *dhcp_start = nvram_safe_get("dhcp_start");

	int legal_start_ip, legal_end_ip, legal_total_ip, dhcp_start_ip;
	int set_dhcp_start_ip=0, set_dhcp_num=0;
	int adjust_ip = 0, adjust_num = 0;

	inet_aton(lan_ipaddr, &netaddr);
        inet_aton(lan_netmask, &netmask);
        inet_aton(dhcp_start, &ipaddr);
	
	legal_total_ip = 254 - get_single_ip(lan_netmask,3);
	legal_start_ip = (get_single_ip(lan_ipaddr,3) & get_single_ip(lan_netmask,3)) + 1;
	legal_end_ip = legal_start_ip + legal_total_ip - 1;
	dhcp_start_ip = atoi(dhcp_start);
#if 1 //fixed by crazy 20070709 - issue id 7597
	if(legal_start_ip == get_single_ip(lan_ipaddr, 3))
	{
		legal_start_ip++; //LAN IP can't be the start IP.
	}
	legal_total_ip = legal_total_ip - 1; //LAN IP can't be a requested IP.
#endif

	dprintf("legal_total_ip=[%d] legal_start_ip=[%d] legal_end_ip=[%d] dhcp_start_ip=[%d]\n", 
		legal_total_ip, legal_start_ip, legal_end_ip, dhcp_start_ip);

        if ((dhcp_start_ip > legal_end_ip) || (dhcp_start_ip < legal_start_ip)){
		dprintf("Illegal DHCP Start IP: We need to adjust DHCP Start ip.\n");
		set_dhcp_start_ip = legal_start_ip;
		adjust_ip = 1;
		if(atoi(dhcp_num) > legal_total_ip){
			dprintf("DHCP num is exceed, we need to adjust.");
			set_dhcp_num = legal_total_ip;
			adjust_num = 1;
		}
	}
	else{
		dprintf("Legal DHCP Start IP: We need to check DHCP num.\n");
		if((atoi(dhcp_num) + dhcp_start_ip) > legal_end_ip){
			dprintf("DHCP num is exceed, we need to adjust.\n");
			set_dhcp_num = legal_end_ip - dhcp_start_ip + 1;
			adjust_num = 1;
		}
	}

	if(adjust_ip){
		char ip[20];
		dprintf("set_dhcp_start_ip=[%d]\n", set_dhcp_start_ip);	
		snprintf(ip, sizeof(ip), "%d", set_dhcp_start_ip);
		nvram_set("dhcp_start", ip);
	}
	if(adjust_num){
		char num[5];
		dprintf("set_dhcp_num=[%d]\n", set_dhcp_num);	
		snprintf(num, sizeof(num), "%d", set_dhcp_num);
		nvram_set("dhcp_num", num);
	}
	
	return 1;
}


#ifdef VERIZON_LAN_SUPPORT
int
start_dhcp_relay(void)
{
	int ret = eval("dhcrelay", nvram_safe_get("dhcrelay_ipaddr"));

	dprintf("done\n");
	return ret;
}

int
stop_dhcp_relay(void)
{
	int ret = eval("killall", "dhcrelay");

	dprintf("done\n");
	return ret ;
}
#endif

int
start_dhcpd(void)
{
#ifdef BRCM
	char name[100];
#endif
	struct dns_lists *dns_list = NULL;
        FILE *fp;
#ifdef UDHCPD_STATIC_SUPPORT
	char dhcp_statics[] = "dhcp_staticsXX";
	char mac[20], ip[10], enable[10];
	char *wordlist;
	int  i;
#endif

	if (
#ifdef VERIZON_LAN_SUPPORT
	nvram_match("router_disable", "1") || nvram_match("lan_proto", "static")
#else
	nvram_match("router_disable", "1") || nvram_invmatch("lan_proto", "dhcp")
#endif
#ifdef UNNUMBERIP_SUPPORT
        || nvram_match("wan_proto", "unnumberip")
#endif
	)
	{
                if(nvram_match("wan_proto", "unnumberip"))
                        cprintf("tallest: ================< we don't need DHCP in LAN side !! >================\n");
		return 0;
	}

#ifdef VERIZON_LAN_SUPPORT
	if(nvram_match("lan_proto", "dhcprelay"))
	{
		start_dhcp_relay();
		return 0;
	}
#endif

	/* Automatically adjust DHCP Start IP and num when LAN IP or netmask is changed */
	adjust_dhcp_range();

	cprintf("%s %d.%d.%d.%s %s %s\n",
		nvram_safe_get("lan_ifname"),
		get_single_ip(nvram_safe_get("lan_ipaddr"),0),
		get_single_ip(nvram_safe_get("lan_ipaddr"),1),
		get_single_ip(nvram_safe_get("lan_ipaddr"),2),
		nvram_safe_get("dhcp_start"),
		nvram_safe_get("dhcp_end"),
		nvram_safe_get("lan_lease"));

	/* Touch leases file */
	if (!(fp = fopen("/tmp/udhcpd.leases", "a"))) {
		perror("/tmp/udhcpd.leases");
		return errno;
	}
	fclose(fp);

	/* Write configuration file based on current information */
	if (!(fp = fopen("/tmp/udhcpd.conf", "w"))) {
		perror("/tmp/udhcpd.conf");
		return errno;
	}
	fprintf(fp, "pidfile /var/run/udhcpd.pid\n");
	fprintf(fp, "start %d.%d.%d.%s\n", get_single_ip(nvram_safe_get("lan_ipaddr"),0),
					   get_single_ip(nvram_safe_get("lan_ipaddr"),1),
					   get_single_ip(nvram_safe_get("lan_ipaddr"),2),
					   nvram_safe_get("dhcp_start"));
#if 1 //fixed by crazy 20070709 - issue id 7597
	{//These codes are ported from WRH54G.
		int i_1 = 0;
		int i_2 = 0;
		unsigned char broadcast = ~(get_single_ip(nvram_safe_get("lan_netmask"), 3));
		unsigned char end_tmp = 0;
		char *C_1 = nvram_safe_get("dhcp_start");
		
		i_2 = atoi(C_1);
		
		if((i_2 <= get_single_ip(nvram_safe_get("lan_ipaddr"), 3)) 
				&& ((i_2 + atoi(nvram_safe_get("dhcp_num")) - 1) >= get_single_ip(nvram_safe_get("lan_ipaddr"), 3)))
		{
			i_1 = 1;
		}
		else
		{
			i_1 = 0;
		}
		
		end_tmp = i_1 + atoi(nvram_safe_get("dhcp_start")) + atoi(nvram_safe_get("dhcp_num")) - 1;
		
		if(broadcast == (end_tmp & broadcast))
		{
			int dhcp_num;
			unsigned char ip[4];
			
			memset(ip, 0, sizeof(ip));
			dhcp_num = atoi(nvram_safe_get("dhcp_num"));
			dhcp_num--;
			snprintf(ip, sizeof(ip), "%d", dhcp_num);
			nvram_set("dhcp_num", ip);
		}
		fprintf(fp, "end %d.%d.%d.%d\n", get_single_ip(nvram_safe_get("lan_ipaddr"), 0), 
				get_single_ip(nvram_safe_get("lan_ipaddr"), 1), 
				get_single_ip(nvram_safe_get("lan_ipaddr"), 2), 
				i_1 + atoi(nvram_safe_get("dhcp_start")) + atoi(nvram_safe_get("dhcp_num")) - 1);
	}
#else
	fprintf(fp, "end %d.%d.%d.%d\n", get_single_ip(nvram_safe_get("lan_ipaddr"),0),
			      		 get_single_ip(nvram_safe_get("lan_ipaddr"),1),
			 	         get_single_ip(nvram_safe_get("lan_ipaddr"),2),
					 atoi(nvram_safe_get("dhcp_start")) + atoi(nvram_safe_get("dhcp_num")) - 1);
#endif
	fprintf(fp, "max_leases 254\n");
	fprintf(fp, "interface %s\n", nvram_safe_get("lan_ifname"));
	fprintf(fp, "wan_interface %s\n", nvram_safe_get("wan_ifname"));
	fprintf(fp, "remaining yes\n");
	fprintf(fp, "auto_time 30\n");		// N seconds to update lease table
	fprintf(fp, "lease_file /tmp/udhcpd.leases\n");
#ifdef UDHCPD_STATIC_SUPPORT
	for (i=0; i<10; i++) {
		snprintf(dhcp_statics, sizeof(dhcp_statics), "dhcp_statics%d", i);
		wordlist = nvram_safe_get(dhcp_statics);
		sscanf(wordlist, "%s %s %s", mac, ip, enable);
		if (!strcmp(enable, "on")) {
			fprintf(fp, "static_lease %s %d.%d.%d.%s\n",
				mac,
				get_single_ip(nvram_safe_get("lan_ipaddr"),0),
				get_single_ip(nvram_safe_get("lan_ipaddr"),1),
				get_single_ip(nvram_safe_get("lan_ipaddr"),2),
				ip);
		}
	}
#endif
	fprintf(fp, "siaddr %s\n", nvram_safe_get("lan_ipaddr"));	// 20040511
	fprintf(fp, "option subnet %s\n", nvram_safe_get("lan_netmask"));
	fprintf(fp, "option router %s\n", nvram_safe_get("lan_ipaddr"));
#ifdef BRCM
	fprintf(fp, "option dns %s\n", nvram_safe_get("lan_ipaddr"));
	fprintf(fp, "option lease %s\n", nvram_safe_get("lan_lease"));
	snprintf(name, sizeof(name), "%s_wins", nvram_safe_get("dhcp_wins"));
	if (nvram_invmatch(name, ""))
		fprintf(fp, "option wins %s\n", nvram_get(name));
	snprintf(name, sizeof(name), "%s_domain", nvram_safe_get("dhcp_domain"));
	if (nvram_invmatch(name, ""))
		fprintf(fp, "option domain %s\n", nvram_get(name));
	fclose(fp);
#endif
	if (nvram_invmatch("wan_wins", "") && nvram_invmatch("wan_wins", "0.0.0.0"))
		fprintf(fp, "option wins %s\n", nvram_safe_get("wan_wins"));

	//dns_list = get_dns_list();
        dns_list = get_dns_list(0);
	if(!dns_list || dns_list->num_servers == 0){
#ifdef XBOX_SUPPORT
		fprintf(fp, "option lease %s\n", "172800");	// no dns, lease time default to 2 days for XBOX 
#else
		fprintf(fp, "option lease %s\n", "300");	// no dns, lease time default to 300 seconds
#endif
#ifdef DNSMASQ_SUPPORT
                fprintf(fp, "option dns %s\n", nvram_safe_get("lan_ipaddr"));
#endif
	}
	else{
#ifdef VERIZON_LAN_SUPPORT
		fprintf(fp, "option lease %d\n", atoi(nvram_safe_get("dhcp_lease")) ? atoi(nvram_safe_get("dhcp_lease"))*60 : 86400*7);
#else
		fprintf(fp, "option lease %d\n", atoi(nvram_safe_get("dhcp_lease")) ? atoi(nvram_safe_get("dhcp_lease"))*60 : 86400);
#endif
#ifdef MPPPOE_SUPPORT
		if (nvram_match("wan_proto", "pppoe"))
                	fprintf(fp, "option dns %s\n", nvram_safe_get("lan_ipaddr"));
		else
		fprintf(fp, "option dns %s %s %s\n", dns_list->dns_server[0], dns_list->dns_server[1], dns_list->dns_server[2]);
#else
		fprintf(fp, "option dns %s %s %s\n", dns_list->dns_server[0], dns_list->dns_server[1], dns_list->dns_server[2]);
#endif
	}
	if (nvram_invmatch("wan_domain", ""))
		fprintf(fp, "option domain %s\n", nvram_safe_get("wan_domain"));
	else if (nvram_invmatch("wan_get_domain", ""))
		fprintf(fp, "option domain %s\n", nvram_safe_get("wan_get_domain"));

	fclose(fp);
	eval("udhcpd", "/tmp/udhcpd.conf");

	dprintf("done\n");
	return 0;
}

int
stop_dhcpd(void)
{
	char sigusr1[] = "-XX";
	int ret;

/*
* Process udhcpd handles two signals - SIGTERM and SIGUSR1
*
*  - SIGUSR1 saves all leases in /tmp/udhcpd.leases
*  - SIGTERM causes the process to be killed
*
* The SIGUSR1+SIGTERM behavior is what we like so that all current client
* leases will be honorred when the dhcpd restarts and all clients can extend
* their leases and continue their current IP addresses. Otherwise clients
* would get NAK'd when they try to extend/rebind their leases and they 
* would have to release current IP and to request a new one which causes 
* a no-IP gap in between.
*/
	sprintf(sigusr1, "-%d", SIGUSR1);
	eval("killall", sigusr1, "udhcpd");
	ret = eval("killall", "udhcpd");
#ifdef VERIZON_LAN_SUPPORT
	stop_dhcp_relay();
#endif

	dprintf("done\n");
	return ret;
}

int
start_dns(void)
{
#ifdef BRCM
	FILE *fp;
	char word[100], *tmp;
#endif
#ifdef DNS_SUPPORT
	int i;
	FILE *fp;
	char nv_name[] = "dns_entryXX";
	char dns_entry[256];
	char ip_addr[20], domain[256], enable[2];
#endif
	int ret;
#ifdef MPPPOE_SUPPORT
        char word[100], *next;
        char got_dns[3][20];
#endif
	

#ifdef DNS_SUPPORT
	/* Write /tmp/hosts with DNS entries */
	if (!(fp = fopen("/tmp/hosts", "w"))) {
		perror("/tmp/hosts");
		return errno;
	}
	for (i = 0; i < 10; i++) {
		sprintf(nv_name, "dns_entry%d", i);
		strcpy(dns_entry, nvram_safe_get(nv_name));
		sscanf(dns_entry, "%s %s %s", enable, ip_addr, domain);
		if (!strcmp(enable, "1"))
			fprintf(fp, "%s\t%s\n", ip_addr, domain);
	}
	fclose(fp);
#endif

#ifdef BRCM
	if (nvram_match("router_disable", "1"))
		return 0;

	/* Write resolv.conf with upstream nameservers */
	if (!(fp = fopen("/tmp/resolv.conf", "w"))) {
		perror("/tmp/resolv.conf");
		return errno;
	}
	foreach(word, nvram_safe_get("wan_dns"), tmp)
		fprintf(fp, "nameserver %s\n", word);
	fclose(fp);
#endif
	if(!is_exist("/tmp/resolv.conf"))
		return -1;

#ifdef MPPPOE_SUPPORT
        if ((nvram_match("wan_proto", "pppoe")
#ifdef UNNUMBERIP_SUPPORT
        || nvram_match("wan_proto","unnumberip")
#endif
        ) && (check_wan_link(1) || nvram_match("ppp_demand_1", "1"))){
                                                                                                                             
                struct in_addr wanip1, dnsip1;
                struct rtentry routeEntry;
                int dnsno = 0;
                                                                                                                             
                foreach(got_dns[dnsno], nvram_safe_get("wan_get_dns_1"), next){
                        dnsno ++;
                }
                sprintf(&word,"/%s/%s",nvram_safe_get("mpppoe_dname"),got_dns[0]);
                                                                                                                             
                inet_aton(nvram_safe_get("wan_ipaddr_1"), &wanip1);
                //inet_aton(nvram_safe_get("wan_get_dns_1"), &dnsip1);
                inet_aton(got_dns[0], &dnsip1);
                                                                                                                             
                wanip1.s_addr = wanip1.s_addr & 0x00ffffff;
                dnsip1.s_addr = dnsip1.s_addr & 0x00ffffff;
                                                                                                                             
                ((struct sockaddr_in *)&(routeEntry.rt_dst))->sin_addr.s_addr = dnsip1.s_addr;
                                                                                                                             
                if(find_route_entry(&routeEntry)){
                        eval ("route","del"
                                ,"-net",inet_ntoa(((struct sockaddr_in *)&(routeEntry.rt_dst))->sin_addr)
                                ,"netmask",inet_ntoa(((struct sockaddr_in *)&(routeEntry.rt_genmask))->sin_addr)
                                ,"gw",inet_ntoa(((struct sockaddr_in *)&(routeEntry.rt_gateway))->sin_addr)
                                );
                }
                                                                                                                             
                eval ("route","add","-net",inet_ntoa(dnsip1),
                        "netmask","255.255.255.0",
                        "gw",nvram_safe_get("wan_gateway_1")
                        );
                                                                                                                             
                ret = eval("dnsmasq",
                           "-T","30",
                           "-i", nvram_safe_get("lan_ifname"),
                           "-r", "/tmp/resolv.conf",
                           "-S",&word);
        }
        else
#endif
        {
#ifdef DNS_SUPPORT
	ret = eval("dnsmasq",
			"-i", nvram_safe_get("lan_ifname"),
			"-r", "/tmp/resolv.conf");
#else
	ret = eval("dnsmasq",
			"-h",
			"-i", nvram_safe_get("lan_ifname"),
			"-r", "/tmp/resolv.conf");
#endif
       }

	dprintf("done\n");
	return ret;
}	

int
stop_dns(void)
{
	int ret = eval("killall", "dnsmasq");

	dprintf("done\n");
	return ret;
}	

int
stop_dns_clear_resolv(void)
{
        int ret = eval("killall", "dnsmasq");
	char buf[80];
                                                                                                                             
	snprintf(buf, sizeof(buf), "echo "" > %s", RESOLV_FILE);
	system(buf);
                                                                                                                             
        dprintf("done\n");
        return ret;
}
 
int
start_httpd(void)
{
	int ret = 0;
	if(nvram_invmatch("http_enable", "0") && !is_exist("/var/run/httpd.pid")) {
		chdir("/www");
#ifdef MULTILANG_SUPPORT
		if(chdir("/tmp/www") == 0)
			cprintf("[HTTPD Starting on /tmp/www]\n");
		else
			cprintf("[HTTPD Starting on /www]\n");
#endif
		ret = eval("httpd");
		chdir("/");
	}

#ifdef HTTPS_SUPPORT
	if(nvram_invmatch("https_enable", "0") && !is_exist("/var/run/httpsd.pid")) {

		// Generate a new certificate
		if(!is_exist("/tmp/cert.pem") || !is_exist("/tmp/key.pem"))
			eval("gencert.sh", BUILD_SECS);		

		chdir("/www");
		ret = eval("httpd", "-S");
		chdir("/");
	}
#endif

	dprintf("done\n");
	return ret;
}

int
stop_httpd(void)
{
	int ret = eval("killall", "httpd");

	unlink("/var/run/httpd.pid");
	unlink("/var/run/httpsd.pid");

	dprintf("done\n");
	return ret;
}

int
start_upnp(void)
{
	int ret;
	char sleep_time[20];

	if (!nvram_invmatch("upnp_enable", "0"))
		return 0;

#ifdef __CONFIG_SES__
	/* UPNP conflict with SES2 , so we want to delay UPNP daemon after SES2 is configured first. */
	if(nvram_match("ses_fsm_current_states", "06:03") || nvram_match("ses_fsm_current_states", "06:01"))
		snprintf(sleep_time, sizeof(sleep_time), "%d", 6);
	else
#endif
		snprintf(sleep_time, sizeof(sleep_time), "%d", 0);

        ret = eval("upnp", "-D",
	       "-L", nvram_safe_get("lan_ifname"),
	       "-W", nvram_safe_get("wan_iface"),
	       "-S", sleep_time,
	       "-I", nvram_safe_get("upnp_ssdp_interval"),
	       "-A", nvram_safe_get("upnp_max_age"));


	dprintf("done\n");
	return ret;
}

int
stop_upnp(void)
{
	int ret;

	ret = eval("killall", "upnp");
	
	dprintf("done\n");
	return ret;
}

int
reinit_upnp(void)
{
	int ret = eval("killall", "-USR1", "upnp");

	return ret;
}

#ifdef __CONFIG_SES__
int
start_ses(void)
{
	if(nvram_match("wl_gmode", "-1"))
		return 0;

	if (nvram_match("ses_enable", "1")) {
		eval("ses", "-f");
	}

	return 0;
}

int
stop_ses(void)
{
	int ret = 0;

	if(nvram_match("wl_gmode", "-1") || nvram_match("ses_enable", "0")) {
		diag_led(SES_LED1,STOP_LED);
		diag_led(SES_LED2,STOP_LED);
	}

	ret = eval("killall", "ses");

	return ret;
}
#endif	/* __CONFIG_SES__ */

static void
convert_wds(void)
{
	char wds_mac[254];
	char buf[254];

	if(nvram_match("wl_wds", ""))		// For Router, accept all WDS link
		strcpy(wds_mac, "*");
	else					// For AP, assign remote WDS MAC
		strcpy(wds_mac, nvram_safe_get("wl_wds"));
	
	/* For WPA-PSK mode, we want to convert wl_wds_mac to wl0_wds0 ... wl0_wds255 */
	if(nvram_match("security_mode2", "wpa_personal")) {
		char wl_wds[]="wl0_wdsXXX";
		int i = 0;
		int j;
		char mac[254];
		char *next;

		foreach(mac, wds_mac, next) {
			snprintf(wl_wds, sizeof(wl_wds), "wl0_wds%d", i);
			snprintf(buf, sizeof(buf), "%s,auto,%s,psk,%s,%s", 
				mac, 
				nvram_safe_get("wl_crypto"), 
				nvram_safe_get("wl_ssid"), 
				nvram_safe_get("wl_wpa_psk"));

			nvram_set(wl_wds, buf);
			i++;
		}
		
		/* Del unused entry */
		for(j = i; j < MAX_NVPARSE; j++)
			del_wds_wsec(0,j);
	}
}

int
start_nas(char *type)
{
	char cfgfile[64];
	char pidfile[64];
	
	char *security_mode = nvram_safe_get("security_mode");

	/* The WPA and PSK mode don't support Shared Key */
	if(strstr(security_mode, "psk") || strstr(security_mode, "wpa"))
		nvram_set("wl_auth", "0");

	/* The WPA mode need some extra parameters for WDS mode. */
	convert_wds();

	if (!type || !*type)
		type = "lan";
	snprintf(cfgfile, sizeof(cfgfile), "/tmp/nas.%s.conf", type);
	snprintf(pidfile, sizeof(pidfile), "/tmp/nas.%s.pid", type);
	{
		char *argv[] = {"nas", cfgfile, pidfile, type, NULL};
		pid_t pid;

		_eval(argv, NULL, 0, &pid);
		dprintf("done\n");
	}
	return 0;
}

int
stop_nas(void)
{
	int ret = eval("killall", "nas");

	dprintf("done\n");
	return ret;
}

int
start_ntpc(void)
{
	char *servers = nvram_safe_get("ntp_server");

#ifdef THROUGHPUT_TEST_SUPPORT
        if(nvram_match("throughput_test","1"))
                return 0;
#endif

	if (strlen(servers)) {
		char *nas_argv[] = {"ntpclient", "-h", servers, "-i", "3", "-l", "-s", NULL};
		pid_t pid;

		_eval(nas_argv, NULL, 0, &pid);
	}
	
	dprintf("done\n");
	return 0;
}

int
stop_ntpc(void)
{
	int ret = eval("killall", "ntpclient");

	dprintf("done\n");
	return ret;
}

#ifdef LAN_AUTO_DHCP_SUPPORT
int check_lan_ip(void)
{
	FILE *fp;

	if(!(fp=fopen("/tmp/lan_ip_check","r")))
		return 0;
	else
	{
		fclose(fp);
		return 1;
	}
}
int
start_auto_dhcp_detect(void)
{
	FILE *fp;
	char tmp_lan_ip[20];
	char tmp_netmask[20];
	int i;

	memcpy(&tmp_lan_ip, nvram_safe_get("lan_ipaddr"), 20);
	memcpy(&tmp_netmask, nvram_safe_get("lan_netmask"), 20);

	start_dhcpc("lan");
	for(i=0; i<3; i++)
		sleep(1);

	//cprintf("tallest:=====(lan_ip = %s, %s, %s)=====\n",nvram_safe_get("lan_ipaddr"), &tmp_lan_ip, &tmp_netmask);
	if(!check_lan_ip())
	{
		cprintf("no dhcp server in lan !! starting dhcp server...\n");
		stop_dhcpc();
		sleep(3);

		eval("ifconfig", nvram_safe_get("lan_ifname"), &tmp_lan_ip);
		start_dhcpd();
	}
	
	return 0;
}
#endif

int
start_services(void)
{
	start_syslog();
	start_tftpd();
	start_cron();
	start_httpd();
	start_dns();
#ifdef LAN_AUTO_DHCP_SUPPORT
	start_auto_dhcp_detect();
#else
	start_dhcpd();
#endif
	start_nas("lan");
	start_zebra();
#ifdef SNMP_SUPPORT
 	start_snmp();
#endif	
#ifdef EOU_SUPPORT
	start_eou();
#endif
#ifdef PPPOE_SERVER_SUPPORT
	start_pppoe_server();
#endif
#ifdef PPPOE_RELAY_SUPPORT
	start_pppoe_relay();
#endif
#ifdef __CONFIG_SES__
	if(nvram_match("ses_fsm_current_states", "06:03") || nvram_match("ses_fsm_current_states", "06:01")) {	// SES2 completed
		cprintf("%d: Waiting for SES2 to finish all session .....\n", time(NULL));
		start_ses();
		int i;
		for(i=0;i<8;i++) sleep(1);
		cprintf("%d: Continue to execute other services .....\n", time(NULL));
	}
	else
		start_ses();
#endif /* __CONFIG_SES__ */

	start_upnp();

#ifdef UTELNETD_SUPPORT
	start_utelnetd();
#endif
#ifdef EBTABLES_SUPPORT
	start_br_firewall(); //mark add for wds bug
#endif

	dprintf("done\n");
	return 0;
}

int
stop_services(void)
{
#ifdef UTELNETD_SUPPORT
	stop_utelnetd();
#endif
#ifdef SNMP_SUPPORT
 	stop_snmp();
#endif	
#ifdef __CONFIG_SES__
	stop_ses();
#endif /* __CONFIG_SES__ */
	stop_nas();
	stop_upnp();
	stop_dhcpd();
	//stop_dns();
        stop_dns_clear_resolv();	
	//stop_httpd();
	stop_cron();
	stop_tftpd();
	stop_syslog();
	stop_zebra();
#ifdef EOU_SUPPORT
	stop_eou();
#endif
#ifdef PPPOE_SERVER_SUPPORT
	stop_pppoe_server();
#endif
#ifdef PPPOE_RELAY_SUPPORT
	stop_pppoe_relay();
#endif
#ifdef EBTABLES_SUPPORT
	stop_br_firewall(); //mark add for wds bug
#endif
	dprintf("done\n");
	return 0;
}

/////////////////////////////////////////////////////
int
start_resetbutton(void)
{
        int ret = 0;

        ret = eval("resetbutton");

        dprintf("done\n");
        return ret;
}

int 
stop_resetbutton(void)
{
        int ret = 0;

        ret = eval("killall","-9","resetbutton");

        dprintf("done\n");
        return ret ;
}

int
start_iptqueue(void)
{
        int ret = 0;

        ret = eval("iptqueue");

        dprintf("done\n");
        return ret;
}

int 
stop_iptqueue(void)
{
        int ret = 0;

        ret = eval("killall","-9","iptqueue");

        dprintf("done\n");
        return ret ;
}

int
start_tftpd(void)
{
#ifdef VERIZON_WAN_SUPPORT
	if(nvram_invmatch("enable_tftpd","1"))
		return 0;
#endif
	int ret = 0;
	pid_t pid;
	char *tftpd_argv[] = { "tftpd",
				"-s","/tmp",	// chroot to /tmp
				"-c",		// allow new files to be created 
				"-l",		// standalone
			      NULL
	};

	ret = _eval(tftpd_argv, NULL, 0, &pid); 

	dprintf("done\n");
	return ret;
}

int 
stop_tftpd(void)
{
	int ret;

        ret = eval("killall","-9","tftpd");

	dprintf("done\n");
	return ret ;
}

#ifdef UTELNETD_SUPPORT
int
start_utelnetd(void)
{
	int ret = 0;
	pid_t pid;
	char enable[] = "X";
	char port[] = "65535";
        char *utelnetd_argv[] = { "utelnetd", "-l", "/bin/sh", "-p", port, NULL
        };

	/* check if it allow telnet server */
	strcpy(enable, nvram_safe_get("telnet_enable"));
	if (strcmp(enable, "0") == 0)
		return ret;

	strcpy(port, nvram_safe_get("telnet_port"));
	if (strcmp(port, "") == 0)
		sprintf(port,"23") ;
	ret = _eval(utelnetd_argv, NULL, 0, &pid);
	dprintf("done\n");
	return ret;
}

int
stop_utelnetd(void)
{
	int ret;

	ret = eval("killall","-9","utelnetd");

	dprintf("done\n");
	return ret;
}
#endif

int
start_cron(void)
{
	int ret = 0;
	struct stat buf;

#ifdef THROUGHPUT_TEST_SUPPORT
	if(nvram_match("throughput_test","1"))
		return 0;
#endif
	/* Create cron's database directory */
	if( stat("/var/spool", &buf) != 0 ){
		mkdir("/var/spool", 0700);
		mkdir("/var/spool/cron", 0700);
	}
	mkdir("/tmp/cron.d", 0700);
	
	buf_to_file("/tmp/cron.d/check_ps", "*/2 * * * * root /sbin/check_ps\n");
#ifdef SYSLOG_SUPPORT
	buf_to_file("/tmp/cron.d/rotatelog", "*/2 * * * * root /usr/sbin/rotatelog.sh 1000\n");
#endif
	ret = eval("cron"); 
	
		
	dprintf("done\n");
	return ret;
}

int 
stop_cron(void)
{
	int ret = 0;

        ret = eval("killall","-9","cron");

	dprintf("done\n");
	return ret ;
}

/* Written by Sparq in 2002/07/16 */
int
start_zebra(void)
{
	FILE *fp;
	int  ret1, ret2;

	char *lt = nvram_safe_get("dr_lan_tx");
	char *lr = nvram_safe_get("dr_lan_rx");
	char *wt = nvram_safe_get("dr_wan_tx");
	char *wr = nvram_safe_get("dr_wan_rx");
	char *lf = nvram_safe_get("lan_ifname");
	char *wf = nvram_safe_get("wan_ifname");

//	printf("Start zebra\n");
	if ( !strcmp(lt, "0") && !strcmp(lr, "0") &&
	     !strcmp(wt, "0") && !strcmp(wr, "0") ){
		printf("zebra disabled.\n");
		return 0;
	}

#if OEM == LINKSYS
	if ( nvram_match("wk_mode", "gateway")
#ifdef UNNUMBERIP_SUPPORT
        && nvram_invmatch("wan_proto", "unnumberip")
#endif
	){
		printf("zebra disabled.\n");
		return 0;
	}
#endif

	/* Write configuration file based on current information */
	if (!(fp = fopen("/tmp/zebra.conf", "w"))) {
		perror("/tmp/zebra.conf");
		return errno;
	}
	fclose(fp);

	if (!(fp = fopen("/tmp/ripd.conf", "w"))) {
		perror("/tmp/ripd.conf");
		return errno;
	}
	fprintf(fp, "router rip\n");
	fprintf(fp, "  network %s\n", lf);
	fprintf(fp, "  network %s\n", wf);
	fprintf(fp, "redistribute connected\n");
        /******** modify by zg 2006.10.18 for cdrouter3.3 item 173(cdrouter_rip_30) bug **********/
	//fprintf(fp, "redistribute kernel\n");
        fprintf(fp, "redistribute kernel\n");
	//fprintf(fp, "redistribute static\n");
	
	fprintf(fp, "interface %s\n", lf);
	if( strcmp(lt, "0") != 0 )
		fprintf(fp, "  ip rip send version %s\n", lt);
	if( strcmp(lr, "0") != 0 )
		fprintf(fp, "  ip rip receive version %s\n", lr);
	
	fprintf(fp, "interface %s\n", wf);
	if( strcmp(wt, "0") != 0 )
		fprintf(fp, "  ip rip send version %s\n", wt);
	if( strcmp(wr, "0") != 0 )
		fprintf(fp, "  ip rip receive version %s\n", wr);

	fprintf(fp, "router rip\n");
	if( strcmp(lt, "0") == 0 )
		fprintf(fp, "  distribute-list private out %s\n",lf);
	if( strcmp(lr, "0") == 0 )
		fprintf(fp, "  distribute-list private in  %s\n",lf);
	if( strcmp(wt, "0") == 0 )
		fprintf(fp, "  distribute-list private out %s\n",wf);
	if( strcmp(wr, "0") == 0 )
		fprintf(fp, "  distribute-list private in  %s\n",wf);
	fprintf(fp, "access-list private deny any\n");

	//fprintf(fp, "debug rip events\n");
	//fprintf(fp, "log file /tmp/ripd.log\n");
	fflush(fp);
	fclose(fp);

	ret1 = eval("zebra", "-d", "-f", "/tmp/zebra.conf");
	ret2 = eval("ripd",  "-d", "-f", "/tmp/ripd.conf");

//	printf("Start RET=%d, %d\n",ret1,ret2);
//	printf("Start zebra done\n");

	dprintf("done\n");
	return ret1 | ret2 ;
//	return 0 ;
}

/* Written by Sparq in 2002/07/16 */
int
stop_zebra(void)
{
	int  ret1, ret2;

//	printf("Stop zebra !\n");

	ret1 = eval("killall", "zebra");
	ret2 = eval("killall", "ripd");

//	printf("Stop RET=%d, %d\n",ret1,ret2);
//	printf("Stop zebra done!\n");
	eval("rm", "-f", "/tmp/routed_gb");
	dprintf("done\n");
	return ret1 | ret2 ;
}

int
start_syslog(void)
{
        int ret = 0;
#ifdef SYSLOG_SUPPORT
	char ipaddr[20];
	
	sprintf(ipaddr,"%d.%d.%d.%s",get_single_ip(nvram_safe_get("lan_ipaddr"),0),
				     get_single_ip(nvram_safe_get("lan_ipaddr"),1),
				     get_single_ip(nvram_safe_get("lan_ipaddr"),2),
				     nvram_safe_get("lan_ipaddr"));


#ifdef KLOGD_SUPPORT
	ret = eval("klogd");
#endif
        if(nvram_invmatch("log_ipaddr", "0"))
                ret += eval("syslogd","-R", ipaddr);
	else
		ret += eval("syslogd","-m","0","-O", LOG_FILE);

	dprintf("done\n");
#endif
	return ret;
}

int
stop_syslog(void)
{
	int ret;

        ret = eval("killall","klogd");
        ret += eval("killall","syslogd");
        ret += eval("killall","-9","rotatelog.sh");
	ret += eval("killall","-9","sleep");

	dprintf("done\n");
	return ret ;
}


int
start_redial(void)
{
	int ret;
	pid_t pid;
	char *redial_argv[] = { "/tmp/ppp/redial",
				nvram_safe_get("ppp_redialperiod"),
			      NULL
	};

	symlink("/sbin/rc", "/tmp/ppp/redial");

	ret = _eval(redial_argv, NULL, 0, &pid); 

	dprintf("done\n");
	return ret;
}

int 
stop_redial(void)
{
	int ret;

        ret = eval("killall","-9","redial");

	dprintf("done\n");
	return ret ;
}


int
stop_pppoe(void)
{
        int ret;

	unlink("/tmp/ppp/link");
        nvram_safe_unset("pppoe_ifname0");
#ifdef MPPPOE_SUPPORT
        unlink("/tmp/ppp/link_1");
        nvram_safe_unset("pppoe_ifname1");
#endif
        ret = eval("killall","pppoecd");
	sleep(1);
        ret += eval("killall", "ip-up");
        ret += eval("killall", "ip-down");

        dprintf("done\n");
        return ret ;
}

int
stop_singe_pppoe(int pppoe_num)
{
        int ret;
        char pppoe_pid[15], pppoe_ifname[15];
        char ppp_unlink[2][20]={"/tmp/ppp/link","/tmp/ppp/link_1"};
        char ppp_wan_dns[2][20]={"wan_get_dns","wan_get_dns_1"};
                                                                                                                             
        sprintf(pppoe_pid,"pppoe_pid%d",pppoe_num);
        sprintf(pppoe_ifname,"pppoe_ifname%d",pppoe_num);
        dprintf("start! stop pppoe %d, pid %s \n",pppoe_num,nvram_safe_get(pppoe_pid));
                                                                                                                             
        ret = eval("kill",nvram_safe_get(pppoe_pid));
        unlink(ppp_unlink[pppoe_num]);
        nvram_unset(pppoe_ifname);
                                                                                                                             
        nvram_set(ppp_wan_dns[pppoe_num],"");
        stop_dns_clear_resolv();
                                                                                                                             
        dprintf("done\n");
        return ret ;
}

#if 1
int start_dhcpc(char *wan_or_lan)
{
	pid_t pid;
	char *hostname;
	char *wan_ifname = "";
        char *lan_ifname = "";
	char dhcp_pid[30];

	cprintf("tallest:=====( wan_or_lan=%s )=====\n",wan_or_lan);

	if(!strncmp(wan_or_lan,"lan", 3))
	{
	cprintf("tallest:=====( wan_or_lan=%s is Lan !!)=====\n",wan_or_lan);
		hostname = nvram_safe_get("lan_hostname");
		lan_ifname = nvram_safe_get("lan_ifname");
		strcpy(dhcp_pid,"/var/run/lan_udhcpc.pid");
	}
	else
	{
	cprintf("tallest:=====( wan_or_lan=%s is wan !!)=====\n",wan_or_lan);
		hostname = nvram_safe_get("wan_hostname");
		wan_ifname = nvram_safe_get("wan_ifname");
		lan_ifname = nvram_safe_get("lan_ifname");
		strcpy(dhcp_pid,"/var/run/wan_udhcpc.pid");
	}
	
	char *dhcp_argv[] = { "udhcpc",
		"-i", wan_ifname,
		"-l", lan_ifname,
		"-p", dhcp_pid,
		"-s", "/tmp/udhcpc",
		hostname && *hostname ? "-H" : NULL,
		hostname && *hostname ? hostname : NULL,
		NULL
	};


	symlink("/sbin/rc", "/tmp/udhcpc");

	if(!strncmp(wan_or_lan,"wan", 3))
	{
#ifdef L2TP_SUPPORT
		if(strcmp(nvram_safe_get("wan_proto"), "l2tp") != 0)
#endif
		nvram_set("wan_iface", nvram_safe_get("wan_ifname"));

		nvram_set("wan_get_dns","");
	}

	_eval(dhcp_argv, NULL, 0, &pid);

	return 0;
}
#else
int start_dhcpc(void)
{
	pid_t pid;
	char *wan_hostname = nvram_safe_get("wan_hostname");
	char *wan_ifname = nvram_safe_get("wan_ifname");
	char *lan_ifname = nvram_safe_get("lan_ifname");
	char *wan_proto = nvram_safe_get("wan_proto");
	char *dhcp_argv[] = { "udhcpc",
		"-i", wan_ifname,
		"-l", lan_ifname,
		"-p", "/var/run/udhcpc.pid",
		"-s", "/tmp/udhcpc",
		wan_hostname && *wan_hostname ? "-H" : NULL,
		wan_hostname && *wan_hostname ? wan_hostname : NULL,
		NULL
	};


	symlink("/sbin/rc", "/tmp/udhcpc");

#ifdef L2TP_SUPPORT
	if(strcmp(wan_proto, "l2tp") != 0)
#endif
	nvram_set("wan_iface", nvram_safe_get("wan_ifname"));

	nvram_set("wan_get_dns","");

	_eval(dhcp_argv, NULL, 0, &pid);

	return 0;
}
#endif

int
stop_dhcpc(void)
{
	int ret = 0;
	
#ifdef L2TP_SUPPORT
	if (nvram_match("wan_proto", "l2tp")) {
		char sigusr[] = "-XX";
		sprintf(sigusr, "-%d", SIGUSR2);
		sleep(1);
		eval("killall", sigusr, "udhcpc");
	}
//	else //if without mark this line, can't kill udhcpc,only doing release action
#endif
		ret += eval("killall","udhcpc");

	dprintf("done\n");
	return ret ;
}

#ifdef WAN_AUTO_DETECT_SUPPORT
int start_auto(void)
{
	int i;

	nvram_set("wan_proto","auto_pppoe");

	nvram_set("wan_mtu",(get_mtu("auto_pppoe"))->max);
	start_pppoe(PPPOE0);
	for(i=0; i<5; i++)
	{
		sleep(1);
	}

	if(check_wan_link(0))
	{
		cprintf("tallest:=====( pppoe connected!! )=====\n");
		return 1;
	}
	else
	{
		cprintf("tallest:=====( pppoe connecting faile!! )=====\n");

		unlink("/tmp/ppp/link");
		nvram_safe_unset("pppoe_ifname0");
	        eval("killall","-9","pppoecd");
	        eval("killall", "ip-up");
	        eval("killall", "ip-down");

		nvram_set("wan_proto","");
	}

	nvram_set("wan_proto","auto_dhcp");
	start_dhcpc("wan");
	for(i=0; i<5; i++)
	{
		sleep(1);
	}

	if(check_wan_link(0))
	{
		cprintf("tallest:=====( dhcp connected!! )=====\n");
		return 1;
	}
	else
	{
		cprintf("tallest:=====( dhcp connecting faile!! )=====\n");
		stop_dhcpc();
		nvram_set("wan_proto","");
	}
	
	nvram_set("wan_proto","auto");
	start_wan_done(nvram_safe_get("wan_ifname"));
	return 0;
}
#endif

int
start_single_service(void)
{
	char *service;

	service = nvram_get("action_service");

	if(!service)
		kill(1, SIGHUP);

	printf("Restart service=[%s]\n",service);
#ifdef MPPPOE_SUPPORT
        pppoe_in_use = NOT_USING; // tallest 1216
#endif

	if(!strcmp(service,"dhcp")){
		stop_dhcpd();
		start_dhcpd();
	}
	else if(!strcmp(service,"start_pppoe")){
                stop_singe_pppoe(PPPOE0);
                start_pppoe(PPPOE0);
        }
        else if(!strcmp(service,"stop_pppoe")){
                stop_singe_pppoe(PPPOE0);
                if(nvram_match("ppp_demand","1")){      // Connect On Demand
                        start_pppoe(PPPOE0);
                        start_dns();
                }else
                        stop_redial();
		//SET_LED(RELEASE_IP);
        }
#ifdef MPPPOE_SUPPORT // tallest 1222
        else if(!strcmp(service,"start_pppoe_1")){
                stop_singe_pppoe(PPPOE1);
                start_pppoe(PPPOE1);
        }
        else if(!strcmp(service,"stop_pppoe_1")){
                stop_singe_pppoe(PPPOE1);
                if(nvram_match("ppp_demand_1","1")){      // Connect On Demand
                        start_pppoe(PPPOE1);
                }else
                        stop_redial();
		//SET_LED(RELEASE_IP);
        }
#endif
	else if(!strcmp(service,"start_pptp") || !strcmp(service,"start_l2tp") || !strcmp(service,"start_heartbeat")){
		unlink("/tmp/ppp/log");
		stop_lan();
		stop_wan();
		//stop_pppoe();
		start_lan();
		start_wan(BOOT);
		//start_pppoe(BOOT);
	}
	else if(!strcmp(service,"stop_pptp") || !strcmp(service,"stop_l2tp") || !strcmp(service,"stop_heartbeat")){
#if OEM == LINKSYS
		stop_wan();
#elif OEM == PCI || OEM == ELSA
		stop_wan();
#else	// nonbrand	(2003-04-11 by honor)
		if(nvram_match("ppp_demand","1")){	// Connect On Demand
			stop_wan();
			stop_lan();
			start_lan();
			start_wan(BOOT);
		}
		else{	// Keep Alive
			stop_wan();
		}
		//SET_LED(RELEASE_IP);
#endif
	}
	else if(!strcmp(service,"filters")){
		stop_cron();
		start_cron();
		//stop_iptqueue();
		//start_iptqueue();
		stop_firewall();
		start_firewall();
		stop_cron();
		start_cron();
#ifdef MULTICAST_SUPPORT
		stop_igmp_proxy();
		start_igmp_proxy();
#endif
#ifdef PARENTAL_CONTROL_SUPPORT
		stop_parental_control();
		start_parental_control();
#endif
	}
	else if(!strcmp(service,"forward")){
		stop_firewall();
		start_firewall();
		stop_cron();
		start_cron();
#ifdef PARENTAL_CONTROL_SUPPORT
		stop_parental_control();
		start_parental_control();
#endif
	}
	else if(!strcmp(service,"forward_upnp")){
		stop_upnp();
		stop_firewall();
		start_upnp();
		start_firewall();
		stop_cron();
		start_cron();
	}
	else if(!strcmp(service,"static_route_del")){
		if(nvram_safe_get("action_service_arg1")){
			del_routes(nvram_safe_get("action_service_arg1"));
		}
	}
#ifdef SNMP_SUPPORT
 	else if(!strcmp(service,"snmp")){
 		stop_snmp();
 		start_snmp();
 	}
#endif
#ifdef DNS_SUPPORT
	else if (!strcmp(service, "dns")) {
		stop_dns();
		start_dns();
	}
#endif	
	else if(!strcmp(service,"ddns")){
		stop_ddns();
		start_ddns();
		nvram_set("ddns_change","update");
		stop_process_monitor();
		start_process_monitor();	
	}
	else if(!strcmp(service,"start_ping")){
		char *ip = nvram_safe_get("ping_ip");
#if 0 //Sync from WRH54G by crazy 20070719
		/*
		   Fixed issue id 7630: 
		   When DUT without a WAN IP, open ping test window and enter target IP address as 
		   192.168.1.1. Ping test doesn't get any reply. It shows "Network is unreachable".
		*/
		if(!check_wan_link(0))
			buf_to_file(PING_TMP, "Network is unreachable\n");
#else
		/*Daniel add to avoid can't ping LAN IP when WAN is down*/
		int lan_flag;
		unsigned int ip_read[4] = {0,0,0,0};
		unsigned int lan_ip[4] = {0,0,0,0};
		unsigned int lan_mask = get_single_ip(nvram_get("lan_netmask"), 3);

		lan_flag = 0;
		//cprintf("lan_mask == %d\n", lan_mask);
		if((sscanf(ip, "%d.%d.%d.%d", &ip_read[0], &ip_read[1], &ip_read[2], &ip_read[3]) == 4)
		&& (sscanf(nvram_get("lan_ipaddr"), "%d.%d.%d.%d", &lan_ip[0], &lan_ip[1], &lan_ip[2], &lan_ip[3]) == 4))
		{
			if(ip_read[0] == lan_ip[0] && ip_read[1] == lan_ip[1] && ip_read[2] == lan_ip[2] && (ip_read[3] & lan_mask) == (lan_ip[3] & lan_mask))
			{
				//cprintf("LAN IP Address!\n");
				lan_flag = 1;
			}
			else
			{
				//cprintf("Diff LAN IP Address!\n");
			}
		}

		if(!check_wan_link(0) && !lan_flag)
			buf_to_file(PING_TMP, "Network is unreachable\n");
#endif
		
		else if(strchr(ip, ' ') || strchr(ip, '`') || strchr(ip, '|') || strchr(ip, '/') || strchr(ip, '>'))		// Fix Ping.asp bug, user can execute command in Ping.asp
			buf_to_file(PING_TMP, "Invalid IP Address or Domain Name\n");
			
		else if(nvram_invmatch("ping_times","") && nvram_invmatch("ping_ip","")){
			char cmd[80];
			snprintf(cmd, sizeof(cmd), "ping -c %s -f %s %s &", nvram_safe_get("ping_times"), PING_TMP, ip);
	   	     	printf("cmd=[%s]\n",cmd);
			eval("killall", "ping");
			unlink(PING_TMP);
	        	system(cmd);    
		}
	}
	else if(!strcmp(service,"start_traceroute")){
		char *ip = nvram_safe_get("traceroute_ip");
		if(!check_wan_link(0))
			if(nvram_match("language", "DE"))
				buf_to_file(TRACEROUTE_TMP, "Netzwerk ist unerreichbar\n");
			else
				buf_to_file(TRACEROUTE_TMP, "Network is unreachable\n");
		
		else if(strchr(ip, ' ') || strchr(ip, '`') || strchr(ip, '|') || strchr(ip, '/') || strchr(ip, '>'))	// Fix Traceroute.asp bug, users can execute command in Traceroute.asp 
			buf_to_file(TRACEROUTE_TMP, "Invalid IP Address or Domain Name\n");
		
		else if(nvram_invmatch("traceroute_ip","")){
			/* Some site block UDP packets, so we want to use ICMP packets */
			char cmd[80];
			snprintf(cmd, sizeof(cmd), "/usr/sbin/traceroute -I -O %s %s &", TRACEROUTE_TMP, ip);
	        	printf("cmd=[%s]\n",cmd);
			eval("killall", "traceroute");
			unlink(TRACEROUTE_TMP);
	        	system(cmd);    
		}
	}
#ifdef HSIAB_SUPPORT
	else if(!strcmp(service,"hsiab_register")){
		//stop_hsiabd();
		stop_firewall();
		start_firewall();
		stop_cron();
		start_cron();
	}
#endif
	else if(!strcmp(service,"tftp_upgrade")){
		stop_wan();
		stop_httpd();
		stop_zebra();
		stop_upnp();
		stop_cron();
#ifdef EOU_SUPPORT
		stop_eou();
#endif
#ifdef SES_SUPPORT
		stop_ses();
#endif
	}
	else if(!strcmp(service,"http_upgrade")){
		stop_wan();
		stop_zebra();
		stop_upnp();
		stop_cron();
#ifdef EOU_SUPPORT
		stop_eou();
#endif
#ifdef SES_SUPPORT
		stop_ses();
#endif
	}
	else if(!strcmp(service,"wireless")){
		stop_services();
		stop_lan();
		start_lan();
		start_services();
		dns_to_resolv();
		stop_firewall();
		start_firewall();
#ifdef MULTICAST_SUPPORT
		stop_igmp_proxy();
		start_igmp_proxy();
#endif
stop_cron();
		start_firewall();
start_cron();
	}
	else if(!strcmp(service,"dhcp_release")){
		char sigusr[] = "-XX";
		sprintf(sigusr, "-%d", SIGUSR2);
		sleep(1);
		eval("killall", sigusr, "udhcpc");
		SET_LED(RELEASE_IP);
	}
	else if(!strcmp(service,"management")){
		if(nvram_match("upnp_enable","0"))
			stop_upnp();
		start_upnp();
		stop_httpd();
		start_httpd();
		start_firewall();
		stop_cron();
		start_cron();
#ifdef SNMP_SUPPORT
		stop_snmp();
		start_snmp();
#endif
#ifdef UTELNETD_SUPPORT
		stop_utelnetd();
		start_utelnetd();
#endif
	}
#ifdef HSIAB_SUPPORT
	else if(!strcmp(service,"start_hsiabsys")){
		stop_dns();
		start_dns();	
	
		stop_dhcpd();
		start_dhcpd();	
	
		stop_process_monitor();
		start_process_monitor();	

		eval("wlconf", nvram_safe_get("wl0_ifname"), "down");
		eval("wlconf", nvram_safe_get("wl0_ifname"), "up");
	
		eval("hsiab_fw");
	}
#endif
#ifdef SES_SUPPORT
	else if(!strcmp(service,"ses_led")){
		char *led_argv[] = { "ses_led",
				     "9999",
				     "25",
				     NULL
		};
		pid_t pid;
	
		eval("killall", "ses_led");
		_eval(led_argv, NULL, 0, &pid);
	}
#endif
	/***********Start********************************/
	else if(!strcmp(service,"upnp_dhcp_renew"))
	{
		char sigusr[] = "-XX";

		sprintf(sigusr, "-%d", SIGUSR1);
		eval("killall", sigusr, "udhcpc");
		//kill_ps("udhcpc", SIGUSR1, 0);
	}
	else if(!strcmp(service,"upnp_teminate"))
	{
		char *wan_proto_name = nvram_safe_get("wan_proto");
		if(!strcmp(wan_proto_name, "dhcp"))
		{
			nvram_set("wan_ipaddr","0.0.0.0");
			nvram_set("wan_netmask","0.0.0.0");
			nvram_set("wan_gateway","0.0.0.0");
			nvram_set("wan_get_dns","");
			nvram_set("wan_lease","0");
			
			unlink("/tmp/get_lease_time");
			unlink("/tmp/lease_time");

			eval("killall", "-17", "udhcpc");
			SET_LED(RELEASE_IP);
		
		}
		else if(!strcmp(wan_proto_name, "pppoe"))
		{
			unlink("/tmp/ppp/log");
			unlink("/tmp/ppp/link");

			stop_singe_pppoe(PPPOE0);
			if(nvram_match("ppp_demand","1"))
			{      // Connect On Demand
				start_pppoe(PPPOE0);
#ifdef DNS_COMPACT
		    		start_dns(1);
#else
		    		start_dns();
#endif
			}

		}
		else if(!strcmp(wan_proto_name, "pptp") || !strcmp(wan_proto_name, "l2tp") || !strcmp(wan_proto_name, "heartbeat"))
		{
#if OEM == LINKSYS
		#if LINKSYS_MODEL == WRV600
			stop_wan();
			stop_lan();
			start_lan();
			start_wan(BOOT);
		#else
			stop_wan();
		#endif
	/************ End of tanghui ********************/
#elif OEM == PCI || OEM == ELSA
			stop_wan();
#else	// nonbrand	(2003-04-11 by honor)
			if(nvram_match("ppp_demand","1")){	// Connect On Demand
				stop_wan();
				clean_arp_binding();
				stop_lan();
				start_lan();
				do_arp_binding();
				start_arp_broadcast();
				start_wan(BOOT);
			}
			else{	// Keep Alive
				stop_wan();
			}
#endif
		}
		stop_upnp();
		start_upnp();

	}
	/***********End*********************************/
	else{
		nvram_unset("action_service");
		nvram_unset("action_service_arg1");
		kill(1, SIGHUP);
	}

	nvram_set("action_service","");
	nvram_set("action_service_arg1","");

	return 0;
}

int
start_pptp(int status)
{
	int ret;
	FILE *fp;
	char *pptp_argv[] = { "pppd",
			      NULL
	};
	char username[80],passwd[80];

	stop_dhcpc();
	stop_pppoe();

	if(nvram_match("aol_block_traffic","0")){
		snprintf(username, sizeof(username), "%s", nvram_safe_get("ppp_username"));
		snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("ppp_passwd"));
	}
	else{
		if(!strcmp(nvram_safe_get("aol_username"),"")){
			snprintf(username, sizeof(username), "%s", nvram_safe_get("ppp_username"));
			snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("ppp_passwd"));
		}
		else{
			snprintf(username, sizeof(username), "%s", nvram_safe_get("aol_username"));
			snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("aol_passwd"));
		}		
	}

	if(status != REDIAL){
		mkdir("/tmp/ppp", 0777);
		symlink("/sbin/rc", "/tmp/ppp/ip-up");
		symlink("/sbin/rc", "/tmp/ppp/ip-down");
		symlink("/dev/null", "/tmp/ppp/connect-errors");

		/* Generate options file */
	       	if (!(fp = fopen("/tmp/ppp/options", "w"))) {
       	        	perror("/tmp/ppp/options");
       	        	return -1;
	       	}
	       	fprintf(fp, "defaultroute\n");  //Add a default route to the system routing tables, using the peer as the gateway
      	 	fprintf(fp, "usepeerdns\n");    //Ask the peer for up to 2 DNS server addresses
       		fprintf(fp, "pty 'pptp %s --nolaunchpppd'\n",nvram_safe_get("pptp_server_ip")); 
       		fprintf(fp, "user '%s'\n",username);
       		//fprintf(fp, "persist\n");        // Do not exit after a connection is terminated.

       		fprintf(fp, "mtu %s\n",nvram_safe_get("wan_mtu"));
	
		if(nvram_match("ppp_demand", "1")){ //demand mode
       			fprintf(fp, "idle %d\n",nvram_match("ppp_demand", "1") ? atoi(nvram_safe_get("ppp_idletime"))*60 : 0);
       			fprintf(fp, "demand\n");         // Dial on demand
       			fprintf(fp, "persist\n");        // Do not exit after a connection is terminated.
	       		//fprintf(fp, "%s:%s\n",PPP_PSEUDO_IP,PPP_PSEUDO_GW);   // <local IP>:<remote IP>
       			fprintf(fp, "ipcp-accept-remote\n");        
       			fprintf(fp, "ipcp-accept-local\n");        
       			fprintf(fp, "connect true\n"); 
       			fprintf(fp, "noipdefault\n");          // Disables  the  default  behaviour when no local IP address is specified
       			fprintf(fp, "ktune\n");         // Set /proc/sys/net/ipv4/ip_dynaddr to 1 in demand mode if the local address changes
		}
		else{	// keepalive mode
			start_redial();
		}

    	  	fprintf(fp, "default-asyncmap\n"); // Disable  asyncmap  negotiation
		fprintf(fp, "nopcomp\n");	// Disable protocol field compression
		fprintf(fp, "noaccomp\n");	// Disable Address/Control compression 
       		fprintf(fp, "noccp\n");         // Disable CCP (Compression Control Protocol)
       		fprintf(fp, "novj\n");          // Disable Van Jacobson style TCP/IP header compression
       		fprintf(fp, "nobsdcomp\n");     // Disables BSD-Compress  compression
       		fprintf(fp, "nodeflate\n");     // Disables Deflate compression
       		fprintf(fp, "lcp-echo-interval 0\n");     // Don't send an LCP echo-request frame to the peer
       		fprintf(fp, "lock\n");
       		fprintf(fp, "noauth");
	
       		fclose(fp);

       		/* Generate pap-secrets file */
       		if (!(fp = fopen("/tmp/ppp/pap-secrets", "w"))) {
       	        	perror("/tmp/ppp/pap-secrets");
        	       	return -1;
       		}
       		fprintf(fp, "\"%s\" * \"%s\" *\n",
			username,
			passwd);
       		fclose(fp);
		chmod("/tmp/ppp/pap-secrets", 0600);

       		/* Generate chap-secrets file */
       		if (!(fp = fopen("/tmp/ppp/chap-secrets", "w"))) {
        	       	perror("/tmp/ppp/chap-secrets");
               		return -1;
       		}
       		fprintf(fp, "\"%s\" * \"%s\" *\n",
			username,
			passwd);
       		fclose(fp);
		chmod("/tmp/ppp/chap-secrets", 0600);

		/* Enable Forwarding */
		if ((fp = fopen("/proc/sys/net/ipv4/ip_forward", "r+"))) {
			fputc('1', fp);
			fclose(fp);
		} else
			perror("/proc/sys/net/ipv4/ip_forward");
	}
	
	/* Bring up  WAN interface */
	ifconfig(nvram_safe_get("wan_ifname"), IFUP,
		 nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));

	ret = _eval(pptp_argv, NULL, 0, NULL); 
	
	if (nvram_match("ppp_demand", "1")){
		/* Trigger Connect On Demand if user press Connect button in Status page */
		if(nvram_match("action_service","start_pptp")){
			//force_to_dial();
                        force_to_dial(nvram_safe_get("action_service"));
			nvram_set("action_service","");
		}
		/* Trigger Connect On Demand if user ping pptp server */
		else
		{
#if 1 //added by crazy 20070720
			/*
			   Fixed issue id 7887(or 7787):
			   When DUT is PPTP Connect on Demand mode, it couldn't be trigger from LAN.
			*/
			dns_to_resolv();
			stop_dns();
			start_dns();
#endif
			eval("listen", nvram_safe_get("lan_ifname"));
		}
	}

	dprintf("done\n");
	return ret;
}

int
stop_pptp(void)
{
	int ret;

	unlink("/tmp/ppp/link");
        ret = eval("killall","-9","pppd");
	sleep(1);
        ret += eval("killall","-9","pptp");
	sleep(1);
        ret += eval("killall","-9","listen");

	dprintf("done\n");
	return ret ;
}
//=========================================tallest============================================
/*
 * This functin build the pppoe instuction & execute it.
 */
int
start_pppoe(int pppoe_num)
{
        char idletime[20], retry_num[20],param[4];
        char username[80], passwd[80];
                                                                                                                             
        char ppp_username[2][20]={"ppp_username","ppp_username_1"};
        char ppp_passwd[2][20]={"ppp_passwd","ppp_passwd_1"};
        char ppp_demand[2][20]={"ppp_demand","ppp_demand_1"};
        char ppp_service[2][20]={"ppp_service","ppp_service_1"};
        char ppp_ac[2][10]={"ppp_ac","ppp_ac_1"};
        char wanip[2][15]={"wan_ipaddr","wan_ipaddr_1"};
        char wanmask[2][15]={"wan_netmask","wan_netmask_1"};
        char wangw[2][15]={"wan_gateway","wan_gateway_1"};
        char pppoeifname[15];
                                                                                                                             
        pid_t pid;
                                                                                                                             
        sprintf(pppoeifname,"pppoe_ifname%d",pppoe_num);
        nvram_set(pppoeifname,"");
                                                                                                                             
        dprintf("start session %d\n",pppoe_num);
#ifdef MPPPOE_SUPPORT
        /* 1. To avoid multi-session pppoe dialing in same time.
         * 2. "pppoe_in_use" this flag init value is NOT_USING.
         * 3. "pppoe_in_use" will be set to USING after the exec. "_eval()" function.
         * 4. "pppoe_in_use" will be set to NOT_USING in ppp.c "ipup_main()" function.
         */
        if((pppoe_in_use == USING) && nvram_invmatch(ppp_demand[pppoe_num],"1")){       //tallest 1216
                wait_pppoe(8);
        }
                                                                                                                             
        if(nvram_invmatch("mpppoe_enable","1") && pppoe_num == PPPOE1)  return 1;
#endif
        sprintf(idletime,"%d",atoi(nvram_safe_get("ppp_idletime"))*60);
        snprintf(retry_num, sizeof(retry_num), "%d", (atoi(nvram_safe_get("ppp_redialperiod"))/5)-1);
                                                                                                                             
        if(nvram_match("aol_block_traffic","1") && pppoe_num == PPPOE0){
                if(!strcmp(nvram_safe_get("aol_username"),"")){
                        snprintf(username, sizeof(username), "%s", nvram_safe_get("ppp_username"));
                        snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("ppp_passwd"));
                }
                else{
                        snprintf(username, sizeof(username), "%s", nvram_safe_get("aol_username"));
                        snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("aol_passwd"));
                }
                                                                                                                             
        }
        else{
                snprintf(username, sizeof(username), "%s", nvram_safe_get(ppp_username[pppoe_num]));
                snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get(ppp_passwd[pppoe_num]));
        }
        sprintf(param,"%d",pppoe_num);
                                                                                                                             
        char *pppoe_argv[] = { "pppoecd",
                nvram_safe_get("wan_ifname"),
                "-u", username,
                "-p", passwd,
                "-r", nvram_safe_get("wan_mtu"),//del by honor, add by tallest.
                "-t", nvram_safe_get("wan_mtu"),
                "-i", nvram_match(ppp_demand[pppoe_num], "1") ? idletime : "0",
#ifdef VERIZON_WAN_SUPPORT
                "-v", "10", 	// retrasmits time (sec).
		"-I", "60",	// Send an LCP echo-request frame to the server every 60 seconds
                "-N", "6", // To avoid kill pppd when pppd has been connecting.
                "-T", "6",      // pppd will presume the server to be dead if 6 LCP echo-requests are sent without receiving a valid LCP echo-reply
#else
                "-I", "30",     // Send an LCP echo-request frame to the server every 30 seconds
                "-N", retry_num, // To avoid kill pppd when pppd has been connecting.
                "-T", "3",      // pppd will presume the server to be dead if 3 LCP echo-requests are sent without receiving a valid LCP echo-reply
#endif
                "-P", param,    // PPPOE session number.
#if LOG_PPPOE == 2
                "-d",
#endif
                "-C", "disconnected_pppoe", //by tallest 0407
                NULL,           /* set default route */
#ifdef UNNUMBERIP_SUPPORT
                NULL,           /* using unnumber ip */
#endif
                NULL, NULL,     /* pppoe_service */
                NULL, NULL,     /* pppoe_ac */
                NULL,           /* pppoe_keepalive */
                NULL
        }, **arg;
        /* Add optional arguments */
        for (arg = pppoe_argv; *arg; arg++);
                                                                                                                             
        if (pppoe_num == PPPOE0) { // PPPOE0 must set default route.
                *arg++ = "-R";
        }
#ifdef UNNUMBERIP_SUPPORT
        if (nvram_match("wan_proto","unnumberip")) { /* using unnumber ip */ // tallest must be change...
                *arg++ = "-n";
        }
#endif
        if (nvram_invmatch(ppp_service[pppoe_num], "")) {
                *arg++ = "-s";
                *arg++ = nvram_safe_get(ppp_service[pppoe_num]);
        }
        if (nvram_invmatch(ppp_ac[pppoe_num], "")) {
                *arg++ = "-a";
                *arg++ = nvram_safe_get(ppp_ac[pppoe_num]);
        }
        if (nvram_match("ppp_static", "1")) {
                        *arg++ = "-L";
                        *arg++ = nvram_safe_get("ppp_static_ip");
        }
        //if (nvram_match("pppoe_demand", "1") || nvram_match("pppoe_keepalive", "1"))
                *arg++ = "-k";
                                                                                                                             
        mkdir("/tmp/ppp", 0777);
        symlink("/sbin/rc", "/tmp/ppp/ip-up");
        symlink("/sbin/rc", "/tmp/ppp/ip-down");
        symlink("/sbin/rc", "/tmp/ppp/set-pppoepid"); // tallest 1219
        unlink("/tmp/ppp/log");

	_eval(pppoe_argv, NULL, 0, &pid);
#ifdef MPPPOE_SUPPORT
        pppoe_in_use = USING ;  //tallest 1216
#endif
                                                                                                                             
        if (nvram_match(ppp_demand[pppoe_num], "1")) {
                start_tmp_ppp(pppoe_num);
                cprintf("------------------------------------------------------------------------------\n");
                char tmpifname[15];
                sprintf(tmpifname,"pppoe_ifname%d",pppoe_num);
                cprintf("pppoe%d ifname=%s ip=%s , netmask=%s, gw=%s\n",
                        pppoe_num,
                        nvram_safe_get(tmpifname),
                        nvram_safe_get(wanip[pppoe_num]),
                        nvram_safe_get(wanmask[pppoe_num]),
                        nvram_safe_get(wangw[pppoe_num]));
                cprintf("------------------------------------------------------------------------------\n");
        }
        dprintf("done. session %d\n",pppoe_num);
        return 0;
}
                                                                                                                             
/*
 * Get the IP, Subnetmask, Geteway from WAN interface
 * and set to NV ram.
 */
void
start_tmp_ppp(int num){
                                                                                                                             
        int timeout = 5;
        char pppoeifname[15];
        char wanip[2][15]={"wan_ipaddr","wan_ipaddr_1"};
        char wanmask[2][15]={"wan_netmask","wan_netmask_1"};
        char wangw[2][15]={"wan_gateway","wan_gateway_1"};
        //char wanif[2][15]={"wan_ifname","wan_ifname_1"};
        //char *wan_ifname = nvram_safe_get("wan_ifname");
        struct ifreq ifr;
        int s;
                                                                                                                             
        dprintf("start session %d\n",num);
                                                                                                                             
        sprintf(pppoeifname,"pppoe_ifname%d",num);
                                                                                                                             
        if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                return;
                                                                                                                             
        /* Wait for ppp0 to be created */
        while (ifconfig(nvram_safe_get(pppoeifname), IFUP, NULL, NULL) && timeout--)
                sleep(1);
                                                                                                                             
        strncpy(ifr.ifr_name, nvram_safe_get(pppoeifname), IFNAMSIZ);
                                                                                                                             
        /* Set temporary IP address */
        timeout = 3;
        while (ioctl(s, SIOCGIFADDR, &ifr) && timeout--){
                perror(nvram_safe_get(pppoeifname));
                printf("Wait %s inteface to init (1) ...\n",nvram_safe_get(pppoeifname));
                sleep(1);
        };
        nvram_set(wanip[num], inet_ntoa(sin_addr(&(ifr.ifr_addr))));
        nvram_set(wanmask[num], "255.255.255.255");
                                                                                                                             
        /* Set temporary P-t-P address */
        timeout = 3;
        while (ioctl(s, SIOCGIFDSTADDR, &ifr) && timeout--){
                perror(nvram_safe_get(pppoeifname));
                printf("Wait %s inteface to init (2) ...\n",nvram_safe_get(pppoeifname));
                sleep(1);
        }
        nvram_set(wangw[num], inet_ntoa(sin_addr(&(ifr.ifr_dstaddr))));
                                                                                                                             
#ifdef MPPPOE_SUPPORT // tallest 1208
        if((num == 1 && nvram_match("mpppoe_enable","1"))
        || (num == 0 && nvram_invmatch("mpppoe_enable","1")))
        {
                start_wan_done(nvram_safe_get(pppoeifname));
        }
#else
        start_wan_done(nvram_safe_get(pppoeifname));
#endif
                                                                                                                             
        // if user press Connect" button from web, we must force to dial
        if(nvram_match("action_service","start_pppoe") || nvram_match("action_service","start_pppoe_1")){
                sleep(3);
                force_to_dial(nvram_safe_get("action_service"));
                nvram_set("action_service","");
        }
                                                                                                                             
        close(s);
        dprintf("done session %d\n",num);
        return;
}
                                                                                                                             
//=====================================================================================================

#ifdef L2TP_SUPPORT
int
start_l2tp(int status)
{
	int ret;
	FILE *fp;
	char *l2tp_argv[] = { "l2tpd",
			      NULL
	};
	char l2tpctrl[64];
	char username[80],passwd[80];

	//stop_dhcpc();
	stop_pppoe();
	stop_pptp();

	if(nvram_match("aol_block_traffic","0")){
		snprintf(username, sizeof(username), "%s", nvram_safe_get("ppp_username"));
		snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("ppp_passwd"));
	}
	else{
		if(!strcmp(nvram_safe_get("aol_username"),"")){
			snprintf(username, sizeof(username), "%s", nvram_safe_get("ppp_username"));
			snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("ppp_passwd"));
		}
		else{
			snprintf(username, sizeof(username), "%s", nvram_safe_get("aol_username"));
			snprintf(passwd, sizeof(passwd), "%s", nvram_safe_get("aol_passwd"));
		}		
	}

	if(status != REDIAL){
		mkdir("/tmp/ppp", 0777);
		symlink("/sbin/rc", "/tmp/ppp/ip-up");
		symlink("/sbin/rc", "/tmp/ppp/ip-down");
		symlink("/dev/null", "/tmp/ppp/connect-errors");

                /* Generate L2TP configuration file */
                if (!(fp = fopen("/tmp/l2tp.conf", "w"))) {
                        perror("/tmp/l2tp.conf");
                        return -1;
                }
                fprintf(fp, "global\n");			// Global section
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
	       	if (!(fp = fopen("/tmp/ppp/options", "w"))) {
       	        	perror("/tmp/ppp/options");
       	        	return -1;
	       	}
	       	fprintf(fp, "defaultroute\n");  //Add a default route to the system routing tables, using the peer as the gateway
      	 	fprintf(fp, "usepeerdns\n");    //Ask the peer for up to 2 DNS server addresses
       		//fprintf(fp, "pty 'pptp %s --nolaunchpppd'\n",nvram_safe_get("pptp_server_ip")); 
       		fprintf(fp, "user '%s'\n",username);
       		//fprintf(fp, "persist\n");        // Do not exit after a connection is terminated.

       		fprintf(fp, "mtu %s\n",nvram_safe_get("wan_mtu"));
	
		if(nvram_match("ppp_demand", "1")){ //demand mode
       			fprintf(fp, "idle %d\n",nvram_match("ppp_demand", "1") ? atoi(nvram_safe_get("ppp_idletime"))*60 : 0);
       			//fprintf(fp, "demand\n");         // Dial on demand
       			//fprintf(fp, "persist\n");        // Do not exit after a connection is terminated.
	       		//fprintf(fp, "%s:%s\n",PPP_PSEUDO_IP,PPP_PSEUDO_GW);   // <local IP>:<remote IP>
       			fprintf(fp, "ipcp-accept-remote\n");        
       			fprintf(fp, "ipcp-accept-local\n");        
       			fprintf(fp, "connect true\n"); 
       			fprintf(fp, "noipdefault\n");          // Disables  the  default  behaviour when no local IP address is specified
       			fprintf(fp, "ktune\n");         // Set /proc/sys/net/ipv4/ip_dynaddr to 1 in demand mode if the local address changes
		}
		else{	// keepalive mode
			start_redial();
		}

    	  	fprintf(fp, "default-asyncmap\n"); // Disable  asyncmap  negotiation
		fprintf(fp, "nopcomp\n");	// Disable protocol field compression
		fprintf(fp, "noaccomp\n");	// Disable Address/Control compression 
       		fprintf(fp, "noccp\n");         // Disable CCP (Compression Control Protocol)
       		fprintf(fp, "novj\n");          // Disable Van Jacobson style TCP/IP header compression
       		fprintf(fp, "nobsdcomp\n");     // Disables BSD-Compress  compression
       		fprintf(fp, "nodeflate\n");     // Disables Deflate compression
       		fprintf(fp, "lcp-echo-interval 0\n");     // Don't send an LCP echo-request frame to the peer
       		fprintf(fp, "lock\n");
       		fprintf(fp, "noauth");
	
       		fclose(fp);

       		/* Generate pap-secrets file */
       		if (!(fp = fopen("/tmp/ppp/pap-secrets", "w"))) {
       	        	perror("/tmp/ppp/pap-secrets");
        	       	return -1;
       		}
       		fprintf(fp, "\"%s\" * \"%s\" *\n",
			username,
			passwd);
       		fclose(fp);
		chmod("/tmp/ppp/pap-secrets", 0600);

       		/* Generate chap-secrets file */
       		if (!(fp = fopen("/tmp/ppp/chap-secrets", "w"))) {
        	       	perror("/tmp/ppp/chap-secrets");
               		return -1;
       		}
       		fprintf(fp, "\"%s\" * \"%s\" *\n",
			username,
			passwd);
       		fclose(fp);
		chmod("/tmp/ppp/chap-secrets", 0600);

		/* Enable Forwarding */
		if ((fp = fopen("/proc/sys/net/ipv4/ip_forward", "r+"))) {
			fputc('1', fp);
			fclose(fp);
		} else
			perror("/proc/sys/net/ipv4/ip_forward");
	}
	
	/* Bring up  WAN interface */
	//ifconfig(nvram_safe_get("wan_ifname"), IFUP,
	//	 nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));

	ret = _eval(l2tp_argv, NULL, 0, NULL);
	sleep(1);
	snprintf(l2tpctrl, sizeof(l2tpctrl), "/usr/sbin/l2tp-control \"start-session %s\"", nvram_safe_get("l2tp_server_ip"));
	//system(l2tpctrl);
	
	if (nvram_match("ppp_demand", "1")){
		/* Trigger Connect On Demand if user press Connect button in Status page */
		if(nvram_match("action_service","start_l2tp")){
			//force_to_dial();
			force_to_dial(nvram_safe_get("action_service"));
			nvram_set("action_service","");
		}
		/* Trigger Connect On Demand if user ping pptp server */
		else
			eval("listen", nvram_safe_get("lan_ifname"));
	}
	else
		system(l2tpctrl);

	dprintf("done\n");
	return ret;
}

int
stop_l2tp(void)
{
	int ret;

	unlink("/tmp/ppp/link");
        ret = eval("killall","-9","pppd");
	sleep(1);
        ret += eval("killall","-9","l2tpd");
	sleep(1);
        ret += eval("killall","-9","listen");

	dprintf("done\n");
	return ret ;
}
#endif

#ifdef MULTICAST_SUPPORT
int
start_igmp_proxy(void)
{
	int ret = 0;
	char *igmp_proxy_argv[] = { "igmprt",
				    "-f",
				    "-i", nvram_safe_get("wan_iface"),
				    NULL
	};

	if (nvram_match("multicast_pass", "1"))
		ret = _eval(igmp_proxy_argv, NULL, 0, NULL); 

	dprintf("done\n");
	return ret;
}

int
stop_igmp_proxy(void)
{
	int ret = eval("killall","-9","igmprt");

	dprintf("done\n");
	return ret ;
}
#endif

#ifdef PARENTAL_CONTROL_SUPPORT
int
start_parental_control(void)
{
	int ret = 0;
	struct stat buf;

	if(nvram_match("artemis_enable", "1")){
		char *argv[] = { "artemis", 
#if LOCALE != EUROPE
				 "-i", nvram_safe_get("wan_ifname"), 	// For get MAC
#endif
				 "-L", nvram_safe_get("lan_ifname"),	// For get MAC & IP Address
				 "-W", nvram_safe_get("wan_iface"), 	// For get IP address
				 NULL };
		pid_t pid;
		if ( nvram_match("artemis_provisioned","1")  ) {
			/* Create directory */
        		if( stat("/var/artemis", &buf) != 0 )
                		mkdir("/var/artemis", 0700);
			nvram2file("artemis_SVCGLOB","/var/artemis/SVCGLOB");
			nvram2file("artemis_HB_DB","/var/artemis/HB_DB");
			nvram2file("artemis_GLOB","/var/artemis/GLOB");
                        nvram2file("artemis_NOS_CTR","/var/artemis/NOS_CTR");
		}
		ret = _eval(argv, "/dev/console", 0, &pid);	// To avoid signal fail
		//ret = eval("artemis");
	}
	
	dprintf("done\n");
	return ret ;
}

int
stop_parental_control(void)
{
	int ret;

	ret = eval("killall","artemis");
	ret = eval("killall","artmain");

	dprintf("done");
	return ret;
}
#endif
#ifdef SNMP_SUPPORT
//----------------- for snmp ----------------------------	
int
start_snmp(void)
{
	int ret = 0;
	pid_t pid;
	char *snmpd_argv[] = { "snmpd",
				"-c","/tmp/snmpd.conf",	// chroot to /tmp
			      NULL
	};
        eval("gen_snmpd_conf");
	ret = _eval(snmpd_argv, NULL, 0, &pid); 

	dprintf("done\n");
	return ret;

}

int 
stop_snmp(void)
{
	int ret;

        ret = eval("killall","-9","snmpd");

	dprintf("done\n");
	return ret ;
}
// ---------------- for snmp <----------------------
#endif

#ifdef PPPOE_SERVER_SUPPORT
int
start_pppoe_server(void)
{
	FILE *fp;
	pid_t pid;

	mkdir("/tmp/ppp/", 0777);

       	if (!(fp = fopen("/tmp/ppp/pppoe-server-options", "w"))) {
		perror("/tmp/ppp/pppoe-server-options");
		return -1;
	}
	fprintf(fp, "require-pap\n");
	fprintf(fp, "mtu %s\n", nvram_safe_get("wan_mtu"));

	fclose(fp);

	/* Generate pap-secrets file */
	if (!(fp = fopen("/tmp/ppp/pap-secrets", "w"))) {
		perror("/tmp/ppp/pap-secrets");
		return -1;
	}
	fprintf(fp, "\"%s\" * \"%s\" *\n", "honor", "123456");
	fclose(fp);

	/* Generate chap-secrets file */
	if (!(fp = fopen("/tmp/ppp/chap-secrets", "w"))) {
		perror("/tmp/ppp/chap-secrets");
		return -1;
	}
	fprintf(fp, "\"%s\" * \"%s\" *\n", "honor", "123456");
	fclose(fp);

	char *pppoe_argv[] = { "pppoe-server",
			       "-I", nvram_safe_get("lan_ifname"),
			       "-C", MODEL_NAME,
			       NULL
	};

	if(nvram_match("pppoe_server", "enable"))
		_eval(pppoe_argv, NULL, 0, &pid);
}

int
stop_pppoe_server(void)
{
	int ret;

	ret = eval("killall", "-9", "pppoe-server");

	dprintf("done\n");
	return ret;
}
#endif

#ifdef PPPOE_RELAY_SUPPORT
int
start_pppoe_relay(void)
{
	pid_t pid;
	char *pppoe_argv[] = { "pppoe-relay",
			       "-S", nvram_safe_get("wan_ifname"),
			       "-C", nvram_safe_get("lan_ifname"),
			       NULL
	};

	if(nvram_match("pppoe_pass", "1")){
		_eval(pppoe_argv, NULL, 0, &pid);
	}
	
}

int
stop_pppoe_relay(void)
{
	int ret;

	ret = eval("killall", "-9", "pppoe-relay");

cprintf("tallest:=====( stop pppoe relay !! )=====\n");
	dprintf("done\n");
	return ret;
}
#endif

#ifdef HSIAB_SUPPORT
int
start_hsiabd(void)
{
	int ret=0;
	
	mkdir("/tmp/hsiab", 0777);
	init_hsiabd();	
	
	if(nvram_match("hsiab_mode","1")){
		char *argv[] = { "hsiabd", NULL };
                pid_t pid;
		nvram_set("hsiab_admin_key","");
		ret = _eval(argv, "/dev/console", 0, &pid);
	}
	
	dprintf("done\n");
	return ret;
}

int
stop_hsiabd(void)
{
	int ret = 0;
	FILE *fp;
	
	//stop_configmon();
	//stop_statsmon();
	//stop_hsiabsys();

	if((fp = fopen("/var/run/hsiabd.pid", "r"))){
       		fclose(fp);
		eval("killall", "hsiabd");
		sleep(5);	// We must wait
	}

	if(nvram_match("hsiab_mode", "0")){
		eval("killall", "-9", "hsiabd");
	}
	
	dprintf("done\n");
	return ret;
}

int
start_configmon()
{
	pid_t pid;
	char *argv[] = {"configmon", NULL};
	int ret = _eval(argv, NULL, 10, NULL);

	dprintf("done\n");
	return ret;
}

int
start_statsmon()
{
	pid_t pid;
	char *argv[] = {"statsmon", NULL};
	int ret = _eval(argv, NULL, 10, NULL);

	dprintf("done\n");
	return ret;
}

int
start_monitor_ip()
{
	pid_t pid;
	char *argv[] = {"monitor_ip", NULL};
	int ret = _eval(argv, NULL, 10, NULL);

	dprintf("done\n");
	return ret;
}
#endif

/*
 * Call when keepalive mode
 */
int
redial_main(int argc, char **argv)
{
	int need_redial = 0;
	int status;
	pid_t pid;
	int count = 1;
#ifdef MPPPOE_SUPPORT
	char buf[80];
	int pid1=0, pid2=0;
#endif
	int num;

	while(1)
	{

#ifdef MPPPOE_SUPPORT	
		sleep(20);	
		if(nvram_invmatch("mpppoe_enable","1"))	// Multi PPPoE is disabled.
			num = 0;			// Always check PPPoE 0
		else
			num = count%2;			// Check PPPoE 0 and PPPoE 1 take turn.
#else
		sleep(atoi(argv[1]));	
		num = 0;
#endif
		count ++;

		//fprintf(stderr, "check PPPoE %d\n", num);
		if(!check_wan_link(num)){
			//fprintf(stderr, "PPPoE %d need to redial\n", num);
			need_redial = 1;
		}
		else{
			//fprintf(stderr, "PPPoE %d not need to redial\n", num);
			continue;
		}

#ifdef MPPPOE_SUPPORT	
		pid1 = get_ppp_pid("/tmp/ppp/link");
		pid2 = get_ppp_pid("/tmp/ppp/link_1");
		//printf("%s(): pid1=[%d] pid2=[%d]\n", __FUNCTION__, pid1, pid2);
#endif
			
		if(need_redial){
			pid = fork();
			SET_LED(GET_IP_ERROR);
			switch(pid)
			{
				case -1:
					perror("fork failed");
					exit(1);
				case 0:
					if(nvram_match("wan_proto","pppoe")
#ifdef UNNUMBERIP_SUPPORT
                                        ||nvram_match("wan_proto","unnumberip")
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT
                                        ||nvram_match("wan_proto","auto_pppoe")
#endif
					){
#ifdef MPPPOE_SUPPORT
						// We only want to kill un-connected pppoecd
						// So we want to find which pppoecd is connected.
						int *pidList = find_all_pid_by_ps("pppoecd");		
						for(; pidList && *pidList!=0; pidList++) {
							//printf("%s(): find pid=[%d]\n", __FUNCTION__, *pidList);
							if(*pidList != pid1 && *pidList != pid2){
								char p[10];
								snprintf(p, sizeof(p), "%d", *pidList);
								//printf("%s(): We want to kill %s\n", __FUNCTION__, p);
								eval("kill", "-9", p);
							}
						}
						if(pidList)	free(pidList);
						
						if(num == 0)
							 start_pppoe(PPPOE0);
                                                        //start_wan(REDIAL);
						else if(num == 1)
							start_pppoe(PPPOE1);
                                                        //start_pppoe2();
#else
						stop_pppoe();
						eval("killall", "-9", "pppoecd");
						sleep(1);
						start_wan(REDIAL);
#endif
					}
					else if(nvram_match("wan_proto","pptp")){
						stop_pptp();
						sleep(1);
						start_wan(REDIAL);
                                        }
#ifdef L2TP_SUPPORT
                                        else if(nvram_match("wan_proto","l2tp")){
                                                stop_l2tp();
                                                sleep(1);
                                                start_l2tp(REDIAL);
                                        }
#endif
#ifdef HEARTBEAT_SUPPORT					
					else if(nvram_match("wan_proto","heartbeat")){
						stop_heartbeat();
						sleep(1);
						start_heartbeat(REDIAL);
                                        }
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT	
					else if(nvram_match("wan_proto","auto")){
						sleep(1);
						start_wan(REDIAL);
						eval("killall", "redial");
					}
#endif
					exit(0);
					break;
				default:
					waitpid(pid, &status, 0);
					//dprintf("parent\n");
					break;
			} // end switch
		} // end if
	} // end while
} // end main

int
ses_led_main(int argc, char **argv)
{
	int i;
	int times;
	int interval;
	int count;
	char buf[20];

	switch (fork()) {
	case -1:
		exit(0);
		break;
	case 0:		
		/* child process */
		(void) setsid();
		break;
	default:	
		/* parent process should just die */
		_exit(0);
	}
	
	times = atoi(argv[1]);
	interval = atoi(argv[2]);

	count = atoi(nvram_safe_get("ses_count"));
	snprintf(buf, sizeof(buf), "%d", count+1);
	nvram_set("ses_count", buf);

	if((check_hw_type() == BCM4712_CHIP) || (check_hw_type() ==  BCM4702_CHIP))
		return 0;

	diag_led(SES_LED2,STOP_LED);

	for(i=0 ; i<times ; i++) {
		/* White led */
		diag_led(SES_LED1,START_LED);			
		usleep(1000 * interval);
		diag_led(SES_LED1,STOP_LED);
		usleep(1000 * interval);
	}
	return 0;
}
