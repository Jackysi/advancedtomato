
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
 * udhcpc scripts
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: udhcpc.c,v 1.27 2005/03/29 02:00:06 honor Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <net/route.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/ioctl.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <rc.h>
#include <code_pattern.h>
#include <cy_conf.h>
#include <utils.h>

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)

static int
expires(unsigned int in)
{
	struct sysinfo info;
	FILE *fp;
	char *ifname = safe_getenv("interface");

	if(nvram_match("wan_ifname",ifname))
	{
        	sysinfo(&info);

		/* Save uptime ranther than system time, because the system time may change */
		if (!(fp = fopen("/tmp/udhcpc.expires", "w"))) {
			perror("/tmp/udhcpd.expires");
			return errno;
		}
		fprintf(fp, "%d", (unsigned int) info.uptime + in);
		fclose(fp);
	}
#ifdef LAN_AUTO_DHCP_SUPPORT
	else if(nvram_match("lan_ifname",ifname))
	{
        	sysinfo(&info);

		/* Save uptime ranther than system time, because the system time may change */
		if (!(fp = fopen("/tmp/lan_udhcpc.expires", "w"))) {
			perror("/tmp/udhcpd.expires");
			return errno;
		}
		fprintf(fp, "%d", (unsigned int) info.uptime + in);
		fclose(fp);
	}
#endif
	return 0;
}	
/* 
 * deconfig: This argument is used when udhcpc starts, and when a
 * leases is lost. The script should put the interface in an up, but
 * deconfigured state.
*/
static int
deconfig(void)
{
	char *ifname = safe_getenv("interface");

	ifconfig(ifname, IFUP, "0.0.0.0", NULL);
	expires(0);

	if(nvram_match("wan_ifname",ifname))
	{
		nvram_set("wan_ipaddr","0.0.0.0");
		nvram_set("wan_netmask","0.0.0.0");
		nvram_set("wan_gateway","0.0.0.0");
		nvram_set("wan_get_dns","");
		//nvram_set("wan_wins","0.0.0.0");      // Don't care for linksys spec
		nvram_set("wan_lease","0");
	}
#ifdef LAN_AUTO_DHCP_SUPPORT 
	else if(nvram_match("lan_ifname",ifname))
	{
		nvram_set("lan_ipaddr","0.0.0.0");
		nvram_set("lan_netmask","0.0.0.0");
		nvram_set("lan_gateway","0.0.0.0");
		//nvram_set("lan_get_dns","");
		//nvram_set("wan_wins","0.0.0.0");      // Don't care for linksys spec
		nvram_set("lan_lease","0");
		//cprintf("tallest:=====( reset lan ip!! )=====\n");
		unlink("/tmp/lan_ip_check");
	}
#endif
	
	unlink("/tmp/get_lease_time");
	unlink("/tmp/lease_time");

	dprintf("done\n");
	return 0;
}

//==================================================================
static int
update_value(void)
{
	
	char *value;
	char *ifname = safe_getenv("interface");
	int changed=0;

	if(nvram_match("wan_ifname",ifname))
	{
		if ((value = getenv("ip"))){
			chomp(value);
			if(nvram_invmatch("wan_ipaddr",value)){
				nvram_set("wan_ipaddr", value);
				changed++;
			}
		}
		if ((value = getenv("subnet"))){
			chomp(value);
			if(nvram_invmatch("wan_netmask",value)){
				nvram_set("wan_netmask", value);
				changed++;
			}
		}
	        if ((value = getenv("router"))){
			chomp(value);
			if(nvram_invmatch("wan_gateway",value)){
				nvram_set("wan_gateway", value);
				changed++;
			}
		}
		if ((value = getenv("dns"))){
			chomp(value);
			if(nvram_invmatch("wan_get_dns",value)){
				nvram_set("wan_get_dns", value);
				/*2007-07-09 add by zg for cdrouter3.6 item 341(dns_60) bug*/
				dns_to_resolv();
				changed++;
			}
		}
		/*
		if ((value = getenv("wins")))
			nvram_set("wan_wins", value);
		if ((value = getenv("hostname")))
			sethostname(value, strlen(value) + 1);
		*/
		if ((value = getenv("domain"))){
			chomp(value);
			if(nvram_invmatch("wan_get_domain",value)){
				nvram_set("wan_get_domain", value);
				changed++;
			}
		}
		if ((value = getenv("lease"))) {
			chomp(value);
			if(nvram_invmatch("wan_lease",value)){
				nvram_set("wan_lease", value);
				changed++;
			}
			expires(atoi(value));
		}
	
		if(changed){
			set_host_domain_name();
			stop_dhcpd();
	                start_dhcpd();
		}
	}
#ifdef LAN_AUTO_DHCP_SUPPORT
	else if(nvram_match("lan_ifname",ifname))
	{
		if ((value = getenv("ip"))){
			chomp(value);
			if(nvram_invmatch("lan_ipaddr",value)){
				nvram_set("lan_ipaddr", value);
				changed++;
			}
		}
		if ((value = getenv("subnet"))){
			chomp(value);
			if(nvram_invmatch("lan_netmask",value)){
				nvram_set("lan_netmask", value);
				changed++;
			}
		}
	        if ((value = getenv("router"))){
			chomp(value);
			if(nvram_invmatch("lan_gateway",value)){
				nvram_set("lan_gateway", value);
				changed++;
			}
		}
		/*
		if ((value = getenv("dns"))){
			chomp(value);
			if(nvram_invmatch("wan_get_dns",value)){
				nvram_set("wan_get_dns", value);
				changed++;
			}
		}
		if ((value = getenv("wins")))
			nvram_set("wan_wins", value);
		if ((value = getenv("hostname")))
			sethostname(value, strlen(value) + 1);
		if ((value = getenv("domain"))){
			chomp(value);
			if(nvram_invmatch("lan_get_domain",value)){
				nvram_set("lan_get_domain", value);
				changed++;
			}
		}
		*/
		if ((value = getenv("lease"))) {
			chomp(value);
			if(nvram_invmatch("lan_lease",value)){
				nvram_set("lan_lease", value);
				changed++;
			}
			expires(atoi(value));
		}
	
//		if(changed){
//			set_host_domain_name();
//			stop_dhcpd();
//	                start_dhcpd();
//		}

	}
#endif
	return 0;
}
//=================================================================

/*
 * bound: This argument is used when udhcpc moves from an unbound, to
 * a bound state. All of the paramaters are set in enviromental
 * variables, The script should configure the interface, and set any
 * other relavent parameters (default gateway, dns server, etc).
*/
static int
bound(void)
{
	char *ifname = safe_getenv("interface");
	char *value;

	if(nvram_match("wan_ifname",ifname))
	{
		if ((value = getenv("ip"))){
			chomp(value);
			nvram_set("wan_ipaddr", value);
		}
		if ((value = getenv("subnet"))){
			chomp(value);
			nvram_set("wan_netmask", value);
		}
		if ((value = getenv("router"))){
			chomp(value);
			nvram_set("wan_gateway", value);
		}
		if ((value = getenv("dns"))){
			chomp(value);
			nvram_set("wan_get_dns", value);
		}
		/*
		if ((value = getenv("wins")))
			nvram_set("wan_wins", value);
		if ((value = getenv("hostname")))
			sethostname(value, strlen(value) + 1);
		*/
		if ((value = getenv("domain"))){
			chomp(value);
			nvram_set("wan_get_domain", value);	// HeartBeat need to use
		}
		if ((value = getenv("lease"))) {
			chomp(value);
			nvram_set("wan_lease", value);
			expires(atoi(value));
		}
	
		ifconfig(ifname, IFUP,
			 nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));

#ifdef HEARTBEAT_SUPPORT 
		/* We only want to exec bellow functions after dhcp get ip if the wan_proto is heartbeat */
		if(nvram_match("wan_proto", "heartbeat")){
			int i=0;
			/* Delete all default routes */
		        while (route_del(ifname, 0, NULL, NULL, NULL) == 0 || i++ < 10);
	
			/* Set default route to gateway if specified */
		        route_add(ifname, 0, "0.0.0.0", nvram_safe_get("wan_gateway"), "0.0.0.0");

		        /* save dns to resolv.conf */
			dns_to_resolv();
			stop_dhcpd();
			start_dhcpd();
					
			start_firewall();
			stop_cron();
			start_cron();
			if(nvram_match("ppp_demand", "1")) {
				if(nvram_match("action_service","start_heartbeat")){
					start_heartbeat(BOOT);
					nvram_set("action_service","");
				}
				else
					eval("listen", nvram_safe_get("lan_ifname"));
			}
			else
				start_heartbeat(BOOT);
		} else
#endif
#ifdef L2TP_SUPPORT
		if(nvram_match("wan_proto", "l2tp")){
			int i=0;
			/* Delete all default routes */
			while (route_del(ifname, 0, NULL, NULL, NULL) == 0 || i++ < 10);

			/* Set default route to gateway if specified */
			route_add(ifname, 0, "0.0.0.0", nvram_safe_get("wan_gateway"), "0.0.0.0");

			/* Backup the default gateway. It should be used if L2TP connection is broken */
			nvram_set("wan_gateway_buf", nvram_get("wan_gateway"));

			/* clear dns from the resolv.conf */
			nvram_set("wan_get_dns","");
			dns_to_resolv();

			start_firewall();
			stop_cron();
			start_cron();
			start_l2tp(BOOT);
		} else
#endif
		{
			start_wan_done(ifname);
		}
	}
#ifdef LAN_AUTO_DHCP_SUPPORT
	else if(nvram_match("lan_ifname",ifname))
	{
		if ((value = getenv("ip"))){
			chomp(value);
			nvram_set("lan_ipaddr", value);
		}
		if ((value = getenv("subnet"))){
			chomp(value);
			nvram_set("lan_netmask", value);
		}
		if ((value = getenv("router"))){
			chomp(value);
			nvram_set("lan_gateway", value);
		}
		/*
		if ((value = getenv("dns"))){
			chomp(value);
			nvram_set("wan_get_dns", value);
		}
		if ((value = getenv("wins")))
			nvram_set("wan_wins", value);
		if ((value = getenv("hostname")))
			sethostname(value, strlen(value) + 1);
		if ((value = getenv("domain"))){
			chomp(value);
			nvram_set("wan_get_domain", value);	// HeartBeat need to use
		}
		*/
		if ((value = getenv("lease"))) {
			chomp(value);
			nvram_set("lan_lease", value);
			expires(atoi(value));
		}
	
		ifconfig(ifname, IFUP,
			 nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"));

		if(nvram_invmatch("lan_ipaddr","0.0.0.0"))
		{
			FILE *fp;
			if(fp=fopen("/tmp/lan_ip_check","w"))
			{
				cprintf("got lan ip by dhcp client!!\n");
				fprintf(fp,"%s","got_lan_ip");
				fclose(fp);
			}
		}
		
	}
#endif
	dprintf("done\n");
	return 0;
}

/*
 * renew: This argument is used when a DHCP lease is renewed. All of
 * the paramaters are set in enviromental variables. This argument is
 * used when the interface is already configured, so the IP address,
 * will not change, however, the other DHCP paramaters, such as the
 * default gateway, subnet mask, and dns server may change.
 */
static int
renew(void)
{
	char *ifname = safe_getenv("interface");

	if(nvram_match("wan_ifname",ifname))	
		SET_LED(RELEASE_IP);
	stop_firewall();
	bound();

	dprintf("done\n");
	return 0;
}

int
udhcpc_main(int argc, char **argv)
{
	if(check_action() != ACT_IDLE)
                return -1;

	if (!argv[1])
		return EINVAL;
	else if (strstr(argv[1], "deconfig"))
		return deconfig();
	else if (strstr(argv[1], "bound"))
		return bound();
	else if (strstr(argv[1], "renew"))
		return renew();
	else if (strstr(argv[1], "update"))
		return update_value();
	else
		return EINVAL;
}
