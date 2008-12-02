
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
 * Network services
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: network.c,v 1.89.10.2.2.1 2006/04/27 08:50:46 honor Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <sys/sysinfo.h>

typedef u_int64_t u64;
typedef u_int32_t u32;
typedef u_int16_t u16;
typedef u_int8_t u8;
#include <linux/sockios.h>
#include <linux/ethtool.h>

#include <bcmnvram.h>
#include <netconf.h>
#include <shutils.h>
#include <code_pattern.h>
#include <wlutils.h>
#include <utils.h>
#include <rc.h>
#include <cy_conf.h>
#include <cymac.h>
#include <bcmutils.h>
#include <nvparse.h>
#include <etsockio.h>
#include <bcmparams.h>


#ifdef THREE_ARP_GRATUATOUS_SUPPORT
struct	ether_header_tmp {
	u_int8_t  ether_dhost[6];
	u_int8_t  ether_shost[6];
	u_int16_t ether_type;
};
struct arpMsg {
	struct ether_header_tmp ethhdr;	 		/* Ethernet header */
	u_short htype;				/* hardware type (must be ARPHRD_ETHER) */
	u_short ptype;				/* protocol type (must be ETH_P_IP) */
	u_char  hlen;				/* hardware address length (must be 6) */
	u_char  plen;				/* protocol address length (must be 4) */
	u_short operation;			/* ARP opcode */
	u_char  sHaddr[6];			/* sender's hardware address */
	u_char  sInaddr[4];			/* sender's IP address */
	u_char  tHaddr[6];			/* target's hardware address */
	u_char  tInaddr[4];			/* target's IP address */
	u_char  pad[18];			/* pad for min. Ethernet payload (60 bytes) */
};
#define	ETH_P_IP		0x0800	/* IP protocol */
#define	ETH_P_ARP		0x0806	/* address resolution protocol */
/* miscellaneous defines */
#define MAC_BCAST_ADDR_SERVER		(unsigned char *) "\xff\xff\xff\xff\xff\xff"
int read_iface(char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp);
int arpping(u_int32_t yiaddr, u_int32_t ip, unsigned char * mac, char * interface);
#endif

#define IFUP (IFF_UP | IFF_RUNNING | IFF_BROADCAST | IFF_MULTICAST)
#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

#ifdef MPPPOE_SUPPORT
extern char pppoe_in_use; // tallest 1216
#endif

void
start_lan(void)
{
	char *lan_ifname = strdup (nvram_safe_get("lan_ifname"));
	char *lan_ifnames = strdup (nvram_safe_get("lan_ifnames")); 
	char name[80], *next;
	char tmp[100];
	int s;
	struct ifreq ifr;
	char wl_face[10];

	dprintf("%s\n", lan_ifname);

	/* Create links */
	symlink("/sbin/rc", "/tmp/ldhclnt");

#ifdef __CONFIG_SES__
	if (nvram_match("ses_bridge_disable", "1")) {
		/* dont bring up the bridge; only the individual interfaces */
		foreach(name, nvram_safe_get("lan_ifnames"), next) {
			/* Bring up interface */
			ifconfig(name, IFUP, NULL, NULL);
			/* if wl */
			eval("wlconf", name, "up");
		}
	} else
#endif
 	/* Bring up bridged interfaces */
	if (strncmp(lan_ifname, "br", 2) == 0) {
		eval("brctl", "addbr", lan_ifname);
		eval("brctl", "setfd", lan_ifname, "0");
		/* For 4712, we want to disable STP,
		 * For 4702, we want to enable STP to light wireless led */
		if(check_hw_type() != BCM4702_CHIP
#ifdef VERIZON_LAN_SUPPORT
		&& check_hw_type() != BCM4704_BCM5325F_CHIP
#endif
		) {
			//if (nvram_match("router_disable", "1")/* || nvram_match("lan_stp", "0")*/)
				eval("brctl", "stp", lan_ifname, "dis");
		}
		foreach(name, lan_ifnames, next) {
			printf("name=[%s] lan_ifname=[%s]\n", name, lan_ifname);

#ifdef WIRELESS_SUPPORT
			if((check_hw_type() == BCM4702_CHIP) || (check_hw_type() == BCM4704_BCM5325F_CHIP))
				strcpy(wl_face, "eth2");
			else
				strcpy(wl_face, "eth1");
				
			/* Write wireless mac , add by honor 2003-10-27 */
			if(!strcmp(name, wl_face)){
				unsigned char mac[20]; ;

				if(nvram_match("wl_gmode", "-1")){
					cprintf("Disable wireless interface\n");
					continue;
				}
				if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                                        continue;
				
				strcpy(mac, nvram_safe_get("et0macaddr"));
				MAC_ADD(mac);
				MAC_ADD(mac);	// The wireless mac equal lan mac add 2
				ether_atoe(mac, ifr.ifr_hwaddr.sa_data);
				ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
				strncpy(ifr.ifr_name, wl_face, IFNAMSIZ);
				if (ioctl(s, SIOCSIFHWADDR, &ifr) == -1)
					perror("Write wireless mac fail : ");
				else
					cprintf("Write wireless mac successfully\n");
				close(s);
			}
#endif
			/* Bring up interface */
			if (ifconfig(name, IFUP, NULL, NULL))
				continue;
			/* Set the logical bridge address to that of the first interface */
			if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
                                continue;
			strncpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
			if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0 &&
			    memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN) == 0) {
				strncpy(ifr.ifr_name, name, IFNAMSIZ);
				if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
					strncpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
					ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
					ioctl(s, SIOCSIFHWADDR, &ifr);
					cprintf("=====> set %s hwaddr to %s\n",lan_ifname,name);
				}
				else 
					perror(lan_ifname);
			}
			else
				perror(lan_ifname);
			close(s);
//#ifdef WIRELESS_SUPPORT
			/* If not a wl i/f then simply add it to the bridge */
			if (eval("wlconf", name, "up"))
				eval("brctl", "addif", lan_ifname, name);
			else {
				/* get the instance number of the wl i/f */
				char wl_name[] = "wlXXXXXXXXXX_mode";
				int unit;
				wl_ioctl(name, WLC_GET_INSTANCE, &unit, sizeof(unit));
				snprintf(wl_name, sizeof(wl_name), "wl%d_mode", unit);
				/* Receive all multicast frames in WET mode */
				if (nvram_match(wl_name, "wet"))
					ifconfig(name, IFUP | IFF_ALLMULTI, NULL, NULL);
				/* Do not attach the main wl i/f if in wds mode */
				if (nvram_invmatch(wl_name, "wds"))
					eval("brctl", "addif", lan_ifname, name);
			}
//#endif
		}
	}
#ifdef WIRELESS_SUPPORT
	/* specific non-bridged lan i/f */
	else if (strcmp(lan_ifname, "")) {	// FIXME
		/* Bring up interface */
		ifconfig(lan_ifname, IFUP, NULL, NULL);
		/* config wireless i/f */
		if (!eval("wlconf", lan_ifname, "up")) {
			char tmp[100], prefix[] = "wanXXXXXXXXXX_";
			int unit;
			/* get the instance number of the wl i/f */
			wl_ioctl(lan_ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));
			snprintf(prefix, sizeof(prefix), "wl%d_", unit);
			/* Receive all multicast frames in WET mode */
			if (nvram_match(strcat_r(prefix, "mode", tmp), "wet"))
				ifconfig(lan_ifname, IFUP | IFF_ALLMULTI, NULL, NULL);
		}
	}
#endif
	/* Get current LAN hardware address */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		char eabuf[32];
		strncpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
		if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0)
			nvram_set("lan_hwaddr", ether_etoa(ifr.ifr_hwaddr.sa_data, eabuf));
		close(s);
	}
#ifdef WIRELESS_SUPPORT
	/* Set QoS mode */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
		int i, qos;
		caddr_t ifrdata;
		struct ethtool_drvinfo info;

		qos = (strcmp(nvram_safe_get("wl_wme"), "on")) ? 0 : 1;
		for (i = 1; i <= DEV_NUMIFS; i ++) {
			ifr.ifr_ifindex = i;
			if (ioctl(s, SIOCGIFNAME, &ifr))
				continue;
			if (ioctl(s, SIOCGIFHWADDR, &ifr))
				continue;
			if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
				continue;
			/* get flags */
			if (ioctl(s, SIOCGIFFLAGS, &ifr))
				continue;
			/* if up(wan not up yet at this point) */
			if (ifr.ifr_flags & IFF_UP) {
				ifrdata = ifr.ifr_data;
				memset(&info, 0, sizeof(info));
				info.cmd = ETHTOOL_GDRVINFO;
				ifr.ifr_data = (caddr_t)&info;
				if (ioctl(s, SIOCETHTOOL, &ifr) >= 0) {
					/* currently only need to set QoS to et devices */
					if (!strncmp(info.driver, "et", 2)) {
						ifr.ifr_data = (caddr_t)&qos;
						ioctl(s, SIOCSETCQOS, &ifr);
					}
				}
				ifr.ifr_data = ifrdata;
			}
		}
	}
#endif
	/* Launch DHCP client - AP only */
	if (nvram_match("router_disable", "1") && nvram_match("lan_dhcp", "1")) {
		char *dhcp_argv[] = { "udhcpc",
					"-i", lan_ifname,
					"-p", (sprintf(tmp, "/var/run/udhcpc-%s.pid", lan_ifname), tmp),
					"-s", "/tmp/ldhclnt",
					NULL
		};
		int pid;
		
		/* Start dhcp daemon */
		_eval(dhcp_argv, ">/dev/console", 0, &pid);
	}
	/* Handle static IP address - AP or Router */
	else {
		/* Bring up and configure LAN interface */
		ifconfig(lan_ifname, IFUP,
			nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"));

		/* Set default route - AP only */
		if (nvram_match("router_disable", "1") && nvram_invmatch("lan_gateway", ""))
			route_add(lan_ifname, 0, "0.0.0.0", nvram_safe_get("lan_gateway"), "0.0.0.0");
			
		/* Install lan specific static routes */
		//add_lan_routes(lan_ifname);
	}

	/* start syslogd if either log_ipaddr or log_ram_enable is set */
	if (nvram_invmatch("log_ipaddr", "") || nvram_match("log_ram_enable", "1")) {
#if !defined(__CONFIG_BUSYBOX__) || defined(BB_SYSLOGD)
		char *argv[] = {
			"syslogd",
			NULL,		/* -C */
			NULL, NULL,	/* -R host */
			NULL
		};
		int pid;
		int argc = 1;

		if (nvram_match("log_ram_enable", "1"))
			argv[argc++] = "-C";

		if (nvram_invmatch("log_ipaddr", "")) {
			argv[argc++] = "-R";
			argv[argc++] = nvram_get("log_ipaddr");
		}


		_eval(argv, NULL, 0, &pid);
#else /* Busybox configured w/o syslogd */
		cprintf("Busybox configured w/o syslogd\n");
#endif
	}

	dprintf("%s %s\n",
		nvram_safe_get("lan_ipaddr"),
		nvram_safe_get("lan_netmask"));

	/* Bring up local host interface */
	config_loopback();

	/* Set additional lan static routes if need */
	set_routes();
	
	//eval("wl", "radio", nvram_invmatch("wl_gmode", "-1") ? "on" : "off");

#ifdef CYBERTAN_DEV
	/* The wireless led don't light after system ready, we want to light it. */
	else if(check_hw_type() == BCM4702_CHIP){
		if( first_time() && nvram_invmatch("wl_gmode", "-1")){
			int s;
		        struct ifreq ifr;
			if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0){
				strncpy(ifr.ifr_name, lan_ifname, IFNAMSIZ);
				if (!ioctl(s, SIOCGIFBRDADDR, &ifr)){
					char buf[80];
					snprintf(buf, sizeof(buf), "ping -c 1 %s > /dev/null &", inet_ntoa(sin_addr(&ifr.ifr_broadaddr)));
					system(buf);
				}
				close(s);
			}
		}
	}
#endif

	free(lan_ifnames);
	free(lan_ifname);
}

void
stop_lan(void)
{
	char *lan_ifname = nvram_safe_get("lan_ifname");
	char name[80], *next;

	dprintf("%s\n", lan_ifname);
#ifdef BRCM
	/* Stop the syslogd daemon */
        eval("killall", "syslogd");
	/* release the DHCP address and kill the client */
	snprintf(signal, sizeof(signal), "-%d", SIGUSR2);
	eval("killall", signal, "udhcpc");
	eval("killall", "udhcpc");

	/* Remove static routes */
	del_lan_routes(lan_ifname);
#endif
	/* Bring down LAN interface */
	ifconfig(lan_ifname, 0, NULL, NULL);

	/* Bring down bridged interfaces */
	if (strncmp(lan_ifname, "br", 2) == 0) {
		foreach(name, nvram_safe_get("lan_ifnames"), next) {
			eval("wlconf", name, "down");
			ifconfig(name, 0, NULL, NULL);
			eval("brctl", "delif", lan_ifname, name);
		}
		eval("brctl", "delbr", lan_ifname);
	}
	/* Bring down specific interface */
	else if (strcmp(lan_ifname, ""))
		eval("wlconf", lan_ifname, "down");
	
	unlink("/tmp/ldhclnt");
	
	dprintf("done\n");
}

#define sin_addr(s) (((struct sockaddr_in *)(s))->sin_addr)

void
start_wan(int status)
{
	char *wan_ifname = nvram_safe_get("wan_ifname");
	char *wan_proto = strdup(nvram_safe_get("wan_proto"));	// Fix for SES2, the variable value will be disappear.
	int s;
	struct ifreq ifr;

#ifdef MPPPOE_SUPPORT
	pppoe_in_use = NOT_USING; // tallest 1216
#endif

	dprintf("%s %s\n", wan_ifname, wan_proto);

	if (nvram_match("router_disable", "1") || strcmp(wan_proto, "disabled") == 0 || strcmp(nvram_safe_get("upnp_wan_proto"), "") != 0) {
		start_wan_done(wan_ifname);
		return;
	}

#ifdef __CONFIG_SES__
	/* do not bringup wan if in ses bridge disable mode */
	if (nvram_match("ses_bridge_disable", "1"))
		return;
#endif

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return;
	strncpy(ifr.ifr_name, wan_ifname, IFNAMSIZ);

	/* Set WAN hardware address before bringing interface up */
	memset(ifr.ifr_hwaddr.sa_data, 0, ETHER_ADDR_LEN);
	
        if(nvram_match("mac_clone_enable","1") &&
	   nvram_invmatch("def_hwaddr", "00:00:00:00:00:00") &&
	   nvram_invmatch("def_hwaddr", "")){
		ether_atoe(nvram_safe_get("def_hwaddr"), ifr.ifr_hwaddr.sa_data);
	}                                                              
	else{
		unsigned char mac[20];
		strcpy(mac, nvram_safe_get("et0macaddr"));
		MAC_ADD(mac);	// The wan mac equal lan mac add 1
		ether_atoe(mac, ifr.ifr_hwaddr.sa_data);
	}
	
	if (memcmp(ifr.ifr_hwaddr.sa_data, "\0\0\0\0\0\0", ETHER_ADDR_LEN)) {
		ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
		ioctl(s, SIOCSIFHWADDR, &ifr);
	}

	/* Set MTU */
        init_mtu(wan_proto);    // add by honor 2002/12/27
	
        strcpy(ifr.ifr_name, nvram_safe_get("wan_ifname"));
        ifr.ifr_mtu =  atoi(nvram_safe_get("wan_mtu"));
	/*zhijian 2006-12-25 for CD-Router v3.4 mtu bug of PPTP connection mode*/
	if(!strcmp(wan_proto, "pptp"))
	{
		ifr.ifr_mtu += 40;
	}
        ioctl(s, SIOCSIFMTU, &ifr);

	/* Bring up WAN interface */
	ifconfig(wan_ifname, IFUP, NULL, NULL);
#ifdef AOL_SUPPORT
	/* init aol */
        aol_init();
#endif
	nvram_set("wan_run_mtu", nvram_safe_get("wan_mtu"));
	
	set_host_domain_name();

	/* Configure WAN interface */
	if (strcmp(wan_proto, "pppoe") == 0
#ifdef UNNUMBERIP_SUPPORT
        || (strcmp(wan_proto, "unnumberip") == 0)
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT
	|| (strcmp(wan_proto, "auto_pppoe") == 0)
#endif
	) {
#ifdef MPPPOE_SUPPORT
		/* if multiple PPPoE is enable, we must be disable "Dial On Demand" function. */
		if(nvram_match("mpppoe_enable","1")){
			nvram_set("ppp_demand","0");
			nvram_set("ppp_demand_1","0");
		}
#endif

		start_pppoe(PPPOE0);

#ifdef MPPPOE_SUPPORT
		if(nvram_match("mpppoe_enable","1")){
			start_pppoe(PPPOE1);
		}
#endif
		if(nvram_invmatch("ppp_demand", "1")
#ifdef MPPPOE_SUPPORT
		|| nvram_invmatch("ppp_demand_1", "1")
#endif
		)
		{
			if(status != REDIAL)
				start_redial();
		}

	} else if (strcmp(wan_proto, "dhcp") == 0) {
		start_dhcpc("wan");
#ifdef WAN_AUTO_DETECT_SUPPORT		
	} else if (strcmp(wan_proto, "auto_dhcp") == 0) {
		start_dhcpc("wan");
#endif
	} else if (strcmp(wan_proto, "pptp") == 0) {
		start_pptp(status);
#ifdef L2TP_SUPPORT		
	} else if (strcmp(wan_proto, "l2tp") == 0) {
		stop_dhcpc();   //931230 Amin test l2tp (kill udhcpc process)
		start_dhcpc("wan");
#endif
#ifdef HEARTBEAT_SUPPORT		
	} else if (strcmp(wan_proto, "heartbeat") == 0) {
		start_dhcpc("wan");
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT		
	} else if (strcmp(wan_proto, "auto") == 0) {
		if(start_auto())
			stop_redial();
		else
		{
			if(status != REDIAL)
				start_redial();
		}
#endif
	} else {
		nvram_set("wan_iface", nvram_safe_get("wan_ifname"));

		ifconfig(wan_ifname, IFUP,
			 nvram_safe_get("wan_ipaddr"), nvram_safe_get("wan_netmask"));
		start_wan_done(wan_ifname);
	}

	/* Get current WAN hardware address */
	strncpy(ifr.ifr_name, wan_ifname, IFNAMSIZ);
	if (ioctl(s, SIOCGIFHWADDR, &ifr) == 0) {
		char eabuf[32];
		nvram_set("wan_hwaddr", ether_etoa(ifr.ifr_hwaddr.sa_data, eabuf));
	}

	close(s);

	set_ip_forward('1');

//===================================================================================
//Tallest move herei(from "start_lan" function ). Fixed when wireless disable, wireless LED dosen't off.
//===================================================================================
        /* Disable wireless will cause diag led blink, so we want to stop it. */
        if((check_hw_type() == BCM4712_CHIP) || (check_hw_type() == BCM5325E_CHIP)){
                //Barry will put disable WLAN here
                if(nvram_match("wl_gmode","-1"))
                {
                        diag_led(WL, STOP_LED);
			eval("killall","ses");
			diag_led(SES_LED1, STOP_LED);
			diag_led(SES_LED2, STOP_LED);
#if 0
                        eval("wlled","0 0 1");
                        eval("wlled","0 1 1");
#endif
                }
                diag_led(DIAG, STOP_LED);
        }

	/* Light or go out the DMZ led even if there is no wan ip. */
	if(nvram_match("dmz_enable", "1") &&
	   nvram_invmatch("dmz_ipaddr", "") && 
	   nvram_invmatch("dmz_ipaddr", "0"))
		diag_led(DMZ, START_LED);
	else
		diag_led(DMZ, STOP_LED);

	dprintf("%s %s\n",
		nvram_safe_get("wan_ipaddr"),
		nvram_safe_get("wan_netmask"));
	free(wan_proto);
}

void
start_wan_service(void)
{
	//stop_ntp();
	stop_ddns();
#if OEM == LINKSYS
	//start_ntp();
#elif OEM == ALLNET
	// nothing to do
#else
	//start_ntp();
#endif
	start_ddns();
}

void
start_wan_done(char *wan_ifname)
{
	int timeout = 5;
	dprintf("%s %s\n", wan_ifname, nvram_safe_get("wan_proto"));
#ifdef L2TP_SUPPORT //Add by crazy 20070803
	struct in_addr l2tp_server_ip, wan_ipaddr_old, wan_netmask;
#endif

#define CLEAR_IP_CONNTRACK
#ifdef CLEAR_IP_CONNTRACK
	static char pre_wan_ipaddr[20] = {0};
	if(strcmp(pre_wan_ipaddr, "") && strcmp(nvram_safe_get("wan_ipaddr"), pre_wan_ipaddr))
	{
		system("echo 1 > /proc/net/clear_ip_conntrack");
	}
	strcpy(pre_wan_ipaddr, nvram_safe_get("wan_ipaddr"));
#endif

#ifdef L2TP_SUPPORT
	if (nvram_match("wan_proto", "l2tp")) {
		/* Delete all default routes */
		while (route_del(nvram_safe_get("wan_ifname"), 0, NULL, NULL, NULL) == 0);
	}
#endif

#ifdef MPPPOE_SUPPORT
	if(((!strcmp(wan_ifname,nvram_safe_get("pppoe_ifname0"))) 
	&& (check_wan_link(0) || nvram_invmatch("ppp_demand", "1"))) 
	||(!strcmp(wan_ifname,nvram_safe_get("pppoe_ifname1")) 
	&& (check_wan_link(1) || nvram_invmatch("ppp_demand_1", "1")))
	||nvram_match("wan_proto", "static")
	||nvram_match("wan_proto", "dhcp")
	||nvram_match("wan_proto", "pptp")
	||nvram_match("wan_proto", "heartbeat")
	||nvram_match("wan_proto", "l2tp"))
#endif
	{

		/* Delete all default routes */
		while (route_del(wan_ifname, 0, NULL, NULL, NULL) == 0);

		if ((nvram_match("wan_proto", "pppoe")
#ifdef UNNUMBERIP_SUPPORT
                || nvram_match("wan_proto", "unnumberip")
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT
                || nvram_match("wan_proto", "auto_pppoe")
#endif
                ) && check_wan_link(1)){
                        while (route_del(nvram_safe_get("wan_ifname_1"), 0, NULL, NULL, NULL) == 0);
                }

		/* Set default route to gateway if specified */
		char *gateway = nvram_match("wan_proto", "pptp") ? nvram_safe_get("pptp_get_ip") : nvram_safe_get("wan_gateway") ;
		while (route_add(wan_ifname, 0, "0.0.0.0", gateway, "0.0.0.0") && timeout-- ){
			if ((nvram_match("wan_proto", "pppoe") 
#ifdef UNNUMBERIP_SUPPORT
                        || nvram_match("wan_proto", "unnumberip")
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT
	                || nvram_match("wan_proto", "auto_pppoe")
#endif
			)
			&& nvram_match("ppp_demand", "1")) {
				printf("Wait ppp interface to init (3) ...\n");
				sleep(1);
			}
		}
		
	}

#ifdef THREE_ARP_GRATUATOUS_SUPPORT
	/*Alpha add to send Gratuitous ARP when wan_proto is Static IP 2007-04-09*/
	if(nvram_match("wan_proto", "static")) 
	{
		int ifindex;
		u_int32_t wan_ip;
		unsigned char wan_mac[6];
		//unsigned long wan_ipaddr;
		//inet_aton(nvram_safe_get("wan_ipaddr"), &wan_ipaddr);
		/* Check network */
		if (read_iface(nvram_safe_get("wan_iface"), &ifindex, &wan_ip, wan_mac) >= 0)
			arpping(wan_ip, wan_ip, wan_mac, nvram_safe_get("wan_iface"));
	}
#endif
	/* For PPTP protocol, we must use pptp_get_ip as gateway, not pptp_server_ip */
	if(nvram_match("wan_proto", "pptp")) {
		route_del(nvram_safe_get("wan_iface"), 0, nvram_safe_get("wan_gateway"), NULL, "255.255.255.255");
		route_del(nvram_safe_get("wan_iface"), 0, nvram_safe_get("pptp_server_ip"), NULL, "255.255.255.255");
		route_add(nvram_safe_get("wan_iface"), 0, nvram_safe_get("pptp_get_ip"), NULL, "255.255.255.255");
	}
#ifdef L2TP_SUPPORT
	else if(nvram_match("wan_proto", "l2tp")) {
		route_del(nvram_safe_get("wan_iface"), 0, nvram_safe_get("wan_gateway"), NULL, "255.255.255.255");
		route_add(nvram_safe_get("wan_iface"), 0, nvram_safe_get("l2tp_get_ip"), NULL, "255.255.255.255");
#if 1 //Add by crazy 20070803
		/*
		   Fix these issues:
		   1. DUT can't response a L2TP ZLB Control message to L2TP server.
		   2. Configure DUT to be L2TP with Connect on demand in 5 minutes, 
		      but DUT will disconnect L2TP before 5 minutes.
		   3. It also causes DUT could often disconnect from L2TP server in 
		      L2TP Keep Alive mode.
		*/
		if(inet_aton(nvram_safe_get("l2tp_server_ip"), &l2tp_server_ip) 
				&& inet_aton(nvram_safe_get("wan_netmask"), &wan_netmask) 
				&& inet_aton(nvram_safe_get("wan_ipaddr"), &wan_ipaddr_old))
		{
			if((l2tp_server_ip.s_addr & wan_netmask.s_addr) != (wan_ipaddr_old.s_addr & wan_netmask.s_addr)) 
			{/* If DUT WAN IP and L2TP server IP are in different subnets, it could need this route. */
				//fixed routing problem in Israel by kanki
				route_add(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"), 
						nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); 
			}
		}
		else /* Fail to change IP from char to struct, still add this route. */
#endif
		route_add(nvram_safe_get("wan_ifname"), 0, nvram_safe_get("l2tp_server_ip"), nvram_safe_get("wan_gateway_buf"), "255.255.255.255"); //fixed routing problem in Israel by kanki
	}
#endif

	/* save dns to resolv.conf */
        dns_to_resolv();
	
	/* Restart DHCP server */
#ifdef LAN_AUTO_DHCP_SUPPORT
	if(!check_lan_ip())
#endif
	{
		stop_dhcpd();
        	start_dhcpd();
	}

	/* Restart DNS proxy */
	stop_dns();
	start_dns();

	/* Start firewall */
	stop_cron();
	start_firewall();
		start_cron();
#ifdef MULTICAST_SUPPORT
	stop_igmp_proxy();
	start_igmp_proxy();
#endif
	/* Set additional wan static routes if need */
	set_routes();
#ifdef BRCM
	/* Sync time */
	start_ntpc();
	
	/* Report stats */
	if (nvram_invmatch("stats_server", "")) {
		char *stats_argv[] = { "stats", nvram_get("stats_server"), NULL };
		_eval(stats_argv, NULL, 5, NULL);
	}
#endif
	if(nvram_match("wan_proto","pppoe") || nvram_match("wan_proto","pptp") || nvram_match("wan_proto","l2tp")
#ifdef UNNUMBERIP_SUPPORT
        || nvram_match("wan_proto", "unnumberip")
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT
	|| nvram_match("wan_proto", "auto_pppoe")
#endif
	){
                if(nvram_match("ppp_demand","1")){      // ntp and ddns will trigger DOD, so we must stop them when wan is unavaile.
			FILE *fp;
                        if ((fp = fopen("/tmp/ppp/link", "r"))) {
                                start_wan_service();
                                fclose(fp);
                        }
                }
		else{
			start_wan_service();
		}
        }
        else{
               start_wan_service();
        }
	stop_process_monitor();
	start_process_monitor();

	stop_zebra();
        start_zebra();
	
        //stop_upnp();
        //start_upnp();
	reinit_upnp();

	stop_cron();
	start_cron();
#ifdef HSIAB_SUPPORT
	start_hsiabd();
#endif

#ifdef PARENTAL_CONTROL_SUPPORT				
	stop_parental_control();
	start_parental_control();
#endif

#ifdef HW_QOS_SUPPORT				
	stop_voip_qos();
	start_voip_qos();
#endif

	/* We don't need STP after wireless led is lighted */
	if(check_hw_type() == BCM4702_CHIP) {
		eval("brctl", "stp", nvram_safe_get("lan_ifname"), "dis");
	}

	if(check_wan_link(0))
		SET_LED(GOT_IP)
	else if((!check_wan_link(0)) && nvram_match("wan_proto","auto")){
		SET_LED(GET_IP_ERROR);
	}

	dprintf("done\n");
}

void
stop_wan(void)
{
	char name[80], *next;

#ifdef HSIAB_SUPPORT
	stop_hsiabd();
#endif
	
#ifdef PARENTAL_CONTROL_SUPPORT				
	stop_parental_control();
#endif

#ifdef HW_QOS_SUPPORT				
	stop_voip_qos();
#endif
	/* Stop firewall */
	stop_firewall();
#ifdef MULTICAST_SUPPORT
	stop_igmp_proxy();
#endif

	/* Kill any WAN client daemons or callbacks */
	stop_pppoe();
#ifdef L2TP_SUPPORT
	stop_l2tp();
	sleep(1);
#endif
	stop_dhcpc();
#ifdef HEARTBEAT_SUPPORT
	stop_heartbeat();
#endif
	stop_pptp();
	stop_ntp();
	stop_redial();
	nvram_set("wan_get_dns", "");

	/* Bring down WAN interfaces */
	foreach(name, nvram_safe_get("wan_ifnames"), next)
		ifconfig(name, 0, "0.0.0.0", NULL);

	SET_LED(RELEASE_IP);	
	dprintf("done\n");
}

int
set_routes(void)
{
	char word[80], *tmp;
	char *ipaddr, *netmask, *gateway, *metric, *ifname;

	foreach(word, nvram_safe_get("static_route"), tmp) {
		netmask = word;
		ipaddr = strsep(&netmask, ":");
		if (!ipaddr || !netmask)
			continue;
		gateway = netmask;
		netmask = strsep(&gateway, ":");
		if (!netmask || !gateway)
			continue;
		metric = gateway;
		gateway = strsep(&metric, ":");
		if (!gateway || !metric)
			continue;
		ifname = metric;
		metric = strsep(&ifname, ":");
		if (!metric || !ifname)
			continue;

		route_add(ifname, atoi(metric) + 1, ipaddr, gateway, netmask);
	}

	return 0;
}

int
del_routes(char *route)
{
	char word[80], *tmp;
	char *ipaddr, *netmask, *gateway, *metric, *ifname;

	foreach(word, route, tmp) {
		netmask = word;
		ipaddr = strsep(&netmask, ":");
		if (!ipaddr || !netmask)
			continue;
		gateway = netmask;
		netmask = strsep(&gateway, ":");
		if (!netmask || !gateway)
			continue;
		metric = gateway;
		gateway = strsep(&metric, ":");
		if (!gateway || !metric)
			continue;
		ifname = metric;
		metric = strsep(&ifname, ":");
		if (!metric || !ifname)
			continue;

		route_del(ifname, atoi(metric) + 1, ipaddr, gateway, netmask);
	}

	return 0;
}
#ifdef WIRELESS_SUPPORT
static int
notify_nas(char *type, char *ifname, char *action)
{
	char *argv[] = {"nas4not", type, ifname, action, 
			NULL,	/* role */
			NULL,	/* crypto */
			NULL,	/* auth */
			NULL,	/* passphrase */
			NULL,	/* ssid */
			NULL};
	char *str = NULL;
	int retries = 10;
	char tmp[100], prefix[] = "wlXXXXXXXXXX_";
	int unit;
	char remote[ETHER_ADDR_LEN];
	char ssid[48], pass[80], auth[16], crypto[16], role[8];
	int i;

	/* the wireless interface must be configured to run NAS */
	wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit));
	snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	if (nvram_match(strcat_r(prefix, "akm", tmp), "") &&
	    nvram_match(strcat_r(prefix, "auth_mode", tmp), "none"))
		return 0;

	/* find WDS link configuration */
	wl_ioctl(ifname, WLC_WDS_GET_REMOTE_HWADDR, remote, ETHER_ADDR_LEN);
	for (i = 0; i < MAX_NVPARSE; i ++) {
		char mac[ETHER_ADDR_STR_LEN];
		uint8 ea[ETHER_ADDR_LEN];

		if (get_wds_wsec(unit, i, mac, role, crypto, auth, ssid, pass) &&
		    ether_atoe(mac, ea) && !bcmp(ea, remote, ETHER_ADDR_LEN)) {
			argv[4] = role;
			argv[5] = crypto;
			argv[6] = auth;
			argv[7] = pass;
			argv[8] = ssid;
			break;
		}
	}

	/* did not find WDS link configuration, use wireless' */
	if (i == MAX_NVPARSE) {
		/* role */
		argv[4] = "auto";
		/* crypto */
		argv[5] = nvram_safe_get(strcat_r(prefix, "crypto", tmp));
		/* auth mode */
		argv[6] = nvram_safe_get(strcat_r(prefix, "akm", tmp));
		/* passphrase */
		argv[7] = nvram_safe_get(strcat_r(prefix, "wpa_psk", tmp));
		/* ssid */
		argv[8] = nvram_safe_get(strcat_r(prefix, "ssid", tmp));
	}

	/* wait till nas is started */
	while (retries -- > 0 && !(str = file2str("/tmp/nas.lan.pid")))
		sleep(1);
	if (str) {
		int pid;
		free(str);
		return _eval(argv, ">/dev/console", 0, &pid);
	}
	return -1;
}

int
hotplug_net(void)
{
	char *lan_ifname = nvram_safe_get("lan_ifname");
	char *interface,*action,*next;
	char buf[254],ifname_tmp[18];

	if (!(interface = getenv("INTERFACE")) ||
	    !(action = getenv("ACTION")))
		return EINVAL;

	if (strncmp(interface, "wds", 3))
		return 0;

	if (!strcmp(action, "register")) {
		/* Bring up the interface and add to the bridge */
		ifconfig(interface, IFUP, NULL, NULL);
		
		cprintf("hotplug_net(): Bridge WDS interfaces %s\n", interface);

		//mark add for wds bug
		if(nvram_match("wds_ifname","")){
			nvram_set("wds_ifname",interface);
		}
		else{
			foreach(ifname_tmp,nvram_safe_get("wds_ifname") , next) 
			{
				if(strcmp(ifname_tmp,interface)!=0)
				{
				snprintf(buf, sizeof(buf), "%s %s",nvram_safe_get("wds_ifname"),interface);
				nvram_set("wds_ifname",buf);
				}
			}
			
		}
		cprintf("wds: wds_ifname=%s\n",nvram_safe_get("wds_ifname"));
		//end of wds bug

		/* Bridge WDS interfaces */
		if (!strncmp(lan_ifname, "br", 2) && 
		    eval("brctl", "addif", lan_ifname, interface))
		    return 0;

		/* Notify NAS of adding the interface */
		notify_nas("lan", interface, "up");

	}

	return 0;
}
#endif
int
init_mtu(char *wan_proto) 
{
	struct mtu_lists *mtu_list = NULL;

	mtu_list = get_mtu(wan_proto);

	if(nvram_match("mtu_enable","0"))
		nvram_set("wan_mtu", mtu_list->max);
	else {
		if(atoi(nvram_safe_get("wan_mtu")) > atoi(mtu_list->max))
			nvram_set("wan_mtu", mtu_list->max);

		if(atoi(nvram_safe_get("wan_mtu")) < atoi(mtu_list->min))
			nvram_set("wan_mtu", mtu_list->min);
		
	}
	return 0;
}

#ifdef AOL_SUPPORT
int
aol_init(void){
	int i;
	int a,b,c;
        unsigned char m[6];
	char *lan_mac = nvram_safe_get("lan_hwaddr");

	char *username = nvram_safe_get("ppp_username");
	char aol_username[] = "aolnet/abc.prod.dsl.linksys.XXXX.XXXXXXXXXXXX";
	char aol_passwd[] = "LinkSYS";

	if(nvram_match("wan_proto","pppoe") || nvram_match("wan_proto","pptp") || nvram_match("wan_proto","l2tp")
#ifdef UNNUMBERIP_SUPPORT
        || nvram_match("wan_proto","unnumberip")
#endif
#ifdef WAN_AUTO_DETECT_SUPPORT
	|| nvram_match("wan_proto", "auto_pppoe")
#endif
	){
		for(i=0 ; i<strlen(username)-1 ; i++)
			*(username+i)	=  toupper(*(username+i));
		//printf("ppp_username=[%s]\n",username);	

		if(!strcmp(username,"AMERICA ONLINE")){
			sscanf(CYBERTAN_VERSION,"v%d.%d.%d",&a,&b,&c);
			sscanf(lan_mac,"%02X:%02X:%02X:%02X:%02X:%02X",(uint *)&m[0],(uint *)&m[1],(uint *)&m[2],(uint *)&m[3],(uint *)&m[4],(uint *)&m[5]);	

			snprintf(aol_username,sizeof(aol_username),"aolnet/abc.prod.dsl.linksys.%d%02d.%02X%02X%02X%02X%02X%02X",
							    a,b,m[0],m[1],m[2],m[3],m[4],m[5]);
			printf("aol_username=[%s]\n",aol_username);	
			nvram_set("aol_username",aol_username);
			nvram_set("aol_passwd",aol_passwd);
			nvram_set("ppp_passwd",aol_passwd);
			nvram_set("ppp_demand","0");
			nvram_set("aol_block_traffic1","1");
			//nvram_commit();
		}
		else{
			nvram_set("aol_block_traffic1","0");
		}
	}
	else
		nvram_set("aol_block_traffic1","0");
	//printf("=[%s] 1=[%s] 2=[%s]\n",nvram_safe_get("aol_block_traffic2"),nvram_safe_get("aol_block_traffic1"),nvram_safe_get("aol_block_traffic2"));
	if(nvram_match("aol_block_traffic1","1") || nvram_match("aol_block_traffic2","1")){
		if(nvram_match("aol_block_traffic","0")){
			nvram_set("aol_block_traffic","1");
		}
	}
	else{
		nvram_set("aol_block_traffic","0");
	}

	return 0;
}
#endif

/* Trigger Connect On Demand */
int
//force_to_dial(void){
force_to_dial( char *whichone){
	int ret = 0;
	char dst[50];

#ifdef MPPPOE_SUPPORT
        if(!strcmp(whichone,"start_pppoe_1")){
                //sprintf(&dst,"www%s",nvram_safe_get("mpppoe_dname"));
                sprintf(dst,"%s",nvram_safe_get("wan_gateway_1"));
        }
        else
#endif
#ifdef PPTP_SUPPORT
	if (nvram_match("wan_proto", "pptp")) {
		sprintf(dst,"%s",PPP_PSEUDO_GW);
	}
	else
#endif
        {
                //sprintf(&dst,"1.1.1.1");
                sprintf(dst,"%s",nvram_safe_get("wan_gateway"));
        }
                                                                                                                             
        char *ping_argv[] = { "ping",
                             "-c", "1",
                             dst,
                             NULL
        };

	sleep(1);
#ifdef L2TP_SUPPORT
	if (nvram_match("wan_proto", "l2tp")) {
		char l2tpctrl[64];

		snprintf(l2tpctrl, sizeof(l2tpctrl), "/usr/sbin/l2tp-control \"start-session %s\"", nvram_safe_get("l2tp_server_ip"));
		system(l2tpctrl);
	}
	else 
#endif
#ifdef HEARTBEAT_SUPPORT
	if (nvram_match("wan_proto", "heartbeat")) {
		start_heartbeat(BOOT);
	}
	else
#endif
		_eval(ping_argv, NULL, 3, NULL);
	
	return ret;
}
#ifdef THREE_ARP_GRATUATOUS_SUPPORT
int arpping(u_int32_t yiaddr, u_int32_t ip, unsigned char *mac, char *interface)
{

	int	timeout = 1;		// we need to reduce time
	int 	optval = 1;
	int	s;			/* socket */
	int	rv = 1;			/* return value */
	struct sockaddr addr;		/* for interface name */
	struct arpMsg	arp;
	fd_set		fdset;
	struct timeval	tm;
	time_t		prevTime;
	struct in_addr ipaddr;	// add by honor
	int idx;

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_ARP))) == -1) {
		//LOG(LOG_ERR, "Could not open raw socket");
		return -1;
	}
	
	if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) == -1) {
		//LOG(LOG_ERR, "Could not setsocketopt on raw socket");
		close(s);
		return -1;
	}

	/* send arp request */
	memset(&arp, 0, sizeof(arp));
	memcpy(arp.ethhdr.ether_dhost, MAC_BCAST_ADDR_SERVER, 6);	/* MAC DA */
	memcpy(arp.ethhdr.ether_shost, mac, 6);		/* MAC SA */
	arp.ethhdr.ether_type = htons(ETH_P_ARP);		/* protocol type (Ethernet) */
	arp.htype = htons(ARPHRD_ETHER);		/* hardware type */
	arp.ptype = htons(ETH_P_IP);			/* protocol type (ARP message) */
	arp.hlen = 6;					/* hardware address length */
	arp.plen = 4;					/* protocol address length */
	arp.operation = htons(ARPOP_REQUEST);		/* ARP op code */
	*((u_int *) arp.sInaddr) = ip;			/* source IP address */
	memcpy(arp.sHaddr, mac, 6);			/* source hardware address */
	*((u_int *) arp.tInaddr) = yiaddr;		/* target IP address */
	
	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, interface);
	for(idx = 0; idx < 3; idx++)
	{
		if (sendto(s, &arp, sizeof(arp), 0, &addr, sizeof(addr)) < 0)
			rv = 0;
	}
	close(s);
	return rv;
}

int read_iface(char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
        int fd;
        struct ifreq ifr;
        struct sockaddr_in *sin;

        memset(&ifr, 0, sizeof(struct ifreq));
        if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
                ifr.ifr_addr.sa_family = AF_INET;
                strcpy(ifr.ifr_name, interface);

                if (addr) {
                        if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
                                sin = (struct sockaddr_in *) &ifr.ifr_addr;
                                *addr = sin->sin_addr.s_addr;
                                //DEBUG("%s (our ip) = %s \n", ifr.ifr_name, inet_ntoa(sin->sin_addr));
                        } else {
                                //DEBUG("SIOCGIFADDR failed!: \n");
                                return -1;
                        }
                }

                if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
                        //DEBUG("adapter index %d \n", ifr.ifr_ifindex);
                        *ifindex = ifr.ifr_ifindex;
                } else {
                      // DEBUG("SIOCGIFINDEX failed!: \n");
                        return -1;
                }
                if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
                        memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
                       // DEBUG("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x \n",
                               // arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
                } else {
                        //DEBUG("SIOCGIFHWADDR failed!: \n");
                        return -1;
                }
        } else {
               // DEBUG("socket failed!: \n");
                return -1;
        }
        close(fd);
        return 0;
}
#endif
