/*
	account.c

	Bandwidth monitoring enhancements for Tomato firmware
	Copyright (C) 2011 Augusto Bott
	http://code.google.com/p/tomato-sdhc-vlan/

*/

#include "rc.h"
#include <arpa/inet.h>

void accountoneachbridge(int enable);

void start_account(void) {
	accountoneachbridge(0);
	accountoneachbridge(1);
}

void stop_account(void) {
	accountoneachbridge(0);
}

void accountoneachbridge(int enable) {

	struct in_addr ipaddr;
	struct in_addr netmask;
	struct in_addr network;

	char lanN_ifname[] = "lanXX_ifname";
	char lanN_ipaddr[] = "lanXX_ipaddr";
	char lanN_netmask[] = "lanXX_netmask";
	char lanN[] = "lanXX";
	char netaddrnetmask[] = "255.255.255.255/255.255.255.255 ";
	char br;

	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(lanN_ifname, "lan%s_ifname", bridge);

		if (strcmp(nvram_safe_get(lanN_ifname), "")!=0) {

			sprintf(lanN_ipaddr, "lan%s_ipaddr", bridge);
			sprintf(lanN_netmask, "lan%s_netmask", bridge);
			sprintf(lanN, "lan%s", bridge);

			inet_aton(nvram_safe_get(lanN_ipaddr), &ipaddr);
			inet_aton(nvram_safe_get(lanN_netmask), &netmask);

			// bitwise AND of ip and netmask gives the network
			network.s_addr = ipaddr.s_addr & netmask.s_addr;

			sprintf(netaddrnetmask, "%s/%s", inet_ntoa(network), nvram_safe_get(lanN_netmask));

			if (enable == 1) {
				// iptables -I FORWARD 1 -m account --aaddr 192.168.0.0/255.255.255.0 --aname mynetwork
//				cprintf("ipt_account: iptables -I FORWARD 1 -m account --aaddr %s --aname %s\n", netaddrnetmask, lanN);
				eval("iptables", "-I", "FORWARD", "1", "-m", "account", "--aaddr", netaddrnetmask, "--aname", lanN);
			} else {
				// iptables -D FORWARD -m account --aaddr 192.168.0.0/255.255.255.0 --aname mynetwork
//				cprintf("ipt_account: iptables -D FORWARD -m account --aaddr %s --aname %s\n", netaddrnetmask, lanN);
				eval("iptables", "-D", "FORWARD", "-m", "account", "--aaddr", netaddrnetmask, "--aname", lanN);
			}
		}
	}
}

