/*
 * Linux network interface code
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: interface.c,v 1.13 2005/03/07 08:35:32 kanki Exp $
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/route.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <proto/ethernet.h>

#include <shutils.h>
#include <bcmnvram.h>
#include <bcmutils.h>
#include <bcmparams.h>
#include <bcmdevs.h>
#include <shared.h>

#include "rc.h"

int _ifconfig(const char *name, int flags, const char *addr, const char *netmask, const char *dstaddr)
{
	int s;
	struct ifreq ifr;
	struct in_addr in_addr, in_netmask, in_broadaddr;

	_dprintf("%s: name=%s flags=%s addr=%s netmask=%s\n", __FUNCTION__, name, flags == IFUP ? "IFUP" : "0", addr, netmask);
	
	/* Open a raw socket to the kernel */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) return errno;

	/* Set interface name */
	strlcpy(ifr.ifr_name, name, IFNAMSIZ);
	
	/* Set interface flags */
	ifr.ifr_flags = flags;
	if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
		goto ERROR;
	
	/* Set IP address */
	if (addr) {
		inet_aton(addr, &in_addr);
		sin_addr(&ifr.ifr_addr).s_addr = in_addr.s_addr;
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFADDR, &ifr) < 0)
			goto ERROR;
	}

	/* Set IP netmask and broadcast */
	if (addr && netmask) {
		inet_aton(netmask, &in_netmask);
		sin_addr(&ifr.ifr_netmask).s_addr = in_netmask.s_addr;
		ifr.ifr_netmask.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFNETMASK, &ifr) < 0)
			goto ERROR;

		in_broadaddr.s_addr = (in_addr.s_addr & in_netmask.s_addr) | ~in_netmask.s_addr;
		sin_addr(&ifr.ifr_broadaddr).s_addr = in_broadaddr.s_addr;
		ifr.ifr_broadaddr.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFBRDADDR, &ifr) < 0)
			goto ERROR;
	}

	/* Set dst or P-t-P IP address */
	if (dstaddr) {
		inet_aton(dstaddr, &in_addr);
		sin_addr(&ifr.ifr_dstaddr).s_addr = in_addr.s_addr;
		ifr.ifr_dstaddr.sa_family = AF_INET;
		if (ioctl(s, SIOCSIFDSTADDR, &ifr) < 0)
			goto ERROR;
	}

	close(s);
	return 0;

 ERROR:
	close(s);
	perror(name);
	return errno;
}

static int route_manip(int cmd, char *name, int metric, char *dst, char *gateway, char *genmask)
{
	int s;
	struct rtentry rt;
	
	_dprintf("%s: cmd=%s name=%s addr=%s netmask=%s gateway=%s metric=%d\n",
		__FUNCTION__, cmd == SIOCADDRT ? "ADD" : "DEL", name, dst, genmask, gateway, metric);

	/* Open a raw socket to the kernel */
	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) return errno;

	/* Fill in rtentry */
	memset(&rt, 0, sizeof(rt));
	if (dst)
		inet_aton(dst, &sin_addr(&rt.rt_dst));
	if (gateway)
		inet_aton(gateway, &sin_addr(&rt.rt_gateway));
	if (genmask)
		inet_aton(genmask, &sin_addr(&rt.rt_genmask));
	rt.rt_metric = metric;
	rt.rt_flags = RTF_UP;
	if (sin_addr(&rt.rt_gateway).s_addr)
		rt.rt_flags |= RTF_GATEWAY;
	if (sin_addr(&rt.rt_genmask).s_addr == INADDR_BROADCAST)
		rt.rt_flags |= RTF_HOST;
	rt.rt_dev = name;

	/* Force address family to AF_INET */
	rt.rt_dst.sa_family = AF_INET;
	rt.rt_gateway.sa_family = AF_INET;
	rt.rt_genmask.sa_family = AF_INET;
		
	if (ioctl(s, cmd, &rt) < 0) {
		perror(name);
		close(s);
		return errno;
	}

	close(s);
	return 0;

}

int route_add(char *name, int metric, char *dst, char *gateway, char *genmask)
{
	return route_manip(SIOCADDRT, name, metric + 1, dst, gateway, genmask);
}

void route_del(char *name, int metric, char *dst, char *gateway, char *genmask)
{
	while (route_manip(SIOCDELRT, name, metric + 1, dst, gateway, genmask) == 0) {
		//
	}
}

/* configure loopback interface */
void config_loopback(void)
{
	/* Bring up loopback interface */
	ifconfig("lo", IFUP, "127.0.0.1", "255.0.0.0");

	/* Add to routing table */
	route_add("lo", 0, "127.0.0.0", "0.0.0.0", "255.0.0.0");
}

#ifdef TCONFIG_IPV6
int ipv6_mapaddr4(struct in6_addr *addr6, int ip6len, struct in_addr *addr4, int ip4mask)
{
	int i = ip6len >> 5;
	int m = ip6len & 0x1f;
	int ret = ip6len + 32 - ip4mask;
	u_int32_t addr = 0;
	u_int32_t mask = 0xffffffffUL << ip4mask;

	if (ip6len > 128 || ip4mask > 32 || ret > 128)
		return 0;
	if (ip4mask == 32)
		return ret;

	if (addr4)
		addr = ntohl(addr4->s_addr) << ip4mask;

	addr6->s6_addr32[i] &= ~htonl(mask >> m);
	addr6->s6_addr32[i] |= htonl(addr >> m);
	if (m) {
		i++;
		addr6->s6_addr32[i] &= ~htonl(mask << (32 - m));
		addr6->s6_addr32[i] |= htonl(addr << (32 - m));
	}

	return ret;
}
#endif

/* configure/start vlan interface(s) based on nvram settings */
int start_vlan(void)
{
	int s;
	struct ifreq ifr;
	int i, j, vlan0tag;
	char ea[ETHER_ADDR_LEN];
	const char *et2mac;

	if ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) == 0) return 0;

	// FIXIT - shibby
	// When GMAC3 is build-in then LAN interface can be detect as fwd0 (et0), fwd1(et1) and then eth0 (et2).
	// Our LAN interface will be et2 but tomato require in many places et0. So we just copy et2macaddr to et0macaddr.
	if (get_model() == MODEL_R8000) {
		et2mac = nvram_safe_get( "et2macaddr" );
		if (!nvram_match("et0macaddr", (char *)et2mac)) {
			nvram_set("et0macaddr", et2mac);
		}
	}

	/* set vlan i/f name to style "vlan<ID>" */
	eval("vconfig", "set_name_type", "VLAN_PLUS_VID_NO_PAD");

	/* create vlan interfaces */
	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return errno;

	vlan0tag = nvram_get_int("vlan0tag");

	for (i = 0; i <= VLAN_MAXVID; i ++) {
		char nvvar_name[16];
		char vlan_id[16];
		char *hwname, *hwaddr;
		char prio[8];
		int vid_map;

		/* get the address of the EMAC on which the VLAN sits */
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dhwname", i);
		if (!(hwname = nvram_get(nvvar_name)))
			continue;
		snprintf(nvvar_name, sizeof(nvvar_name), "%smacaddr", hwname);
		if (!(hwaddr = nvram_get(nvvar_name)))
			continue;
		ether_atoe(hwaddr, ea);
		/* find the interface name to which the address is assigned */
		for (j = 1; j <= DEV_NUMIFS; j ++) {
			ifr.ifr_ifindex = j;
			if (ioctl(s, SIOCGIFNAME, &ifr))
				continue;
			if (ioctl(s, SIOCGIFHWADDR, &ifr))
				continue;
			if (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)
				continue;
			if (!bcmp(ifr.ifr_hwaddr.sa_data, ea, ETHER_ADDR_LEN))
				break;
		}
		if (j > DEV_NUMIFS)
			continue;
		if (ioctl(s, SIOCGIFFLAGS, &ifr))
			continue;
		if (!(ifr.ifr_flags & IFF_UP))
			ifconfig(ifr.ifr_name, IFUP, 0, 0);

		/* vlan ID mapping */
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dvid", i);
		vid_map = nvram_get_int(nvvar_name);
		if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | i;

		/* create the VLAN interface */
		snprintf(vlan_id, sizeof(vlan_id), "%d", vid_map);
		eval("vconfig", "add", ifr.ifr_name, vlan_id);
		/* setup ingress map (vlan->priority => skb->priority) */
		snprintf(vlan_id, sizeof(vlan_id), "vlan%d", vid_map);
		for (j = 0; j < VLAN_NUMPRIS; j ++) {
			snprintf(prio, sizeof(prio), "%d", j);
			eval("vconfig", "set_ingress_map", vlan_id, prio, prio);
		}
	}

	close(s);

	return 0;
}

/* stop/rem vlan interface(s) based on nvram settings */
int stop_vlan(void)
{
	int i;
	int vlan0tag, vid_map;
	char nvvar_name[16];
	char vlan_id[16];
	char *hwname;

	if ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) == 0) return 0;

	vlan0tag = nvram_get_int("vlan0tag");

	for (i = 0; i <= VLAN_MAXVID; i ++) {
		/* get the address of the EMAC on which the VLAN sits */
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dhwname", i);
		if (!(hwname = nvram_get(nvvar_name)))
			continue;

		/* vlan ID mapping */
		snprintf(nvvar_name, sizeof(nvvar_name), "vlan%dvid", i);
		vid_map = nvram_get_int(nvvar_name);
		if ((vid_map < 1) || (vid_map > 4094)) vid_map = vlan0tag | i;

		/* remove the VLAN interface */
		snprintf(vlan_id, sizeof(vlan_id), "vlan%d", vid_map);
		eval("vconfig", "rem", vlan_id);
	}

	return 0;
}

