/*

	IPTraffic monitoring extensions for Tomato
	Copyright (C) 2011-2012 Augusto Bott

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate


	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

*/

#include <arpa/inet.h>
#include <tomato.h>
#include <shared.h>
#include "iptraffic.h"

void asp_iptraffic(int argc, char **argv) {
	char comma;
	char sa[256];
	FILE *a;
	char ip[INET_ADDRSTRLEN];

	char *exclude;

	unsigned long tx_bytes, rx_bytes;
	unsigned long tp_tcp, rp_tcp;
	unsigned long tp_udp, rp_udp;
	unsigned long tp_icmp, rp_icmp;
	unsigned int ct_tcp, ct_udp;

	exclude = nvram_safe_get("cstats_exclude");

	Node tmp;
	Node *ptr;

	iptraffic_conntrack_init();

	char br;
	char name[] = "/proc/net/ipt_account/lanX";

	web_puts("\n\niptraffic=[");
	comma = ' ';

	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(name, "/proc/net/ipt_account/lan%s", bridge);

		if ((a = fopen(name, "r")) == NULL) continue;

		fgets(sa, sizeof(sa), a); // network
		while (fgets(sa, sizeof(sa), a)) {
			if(sscanf(sa, 
				"ip = %s bytes_src = %lu %*u %*u %*u %*u packets_src = %*u %lu %lu %lu %*u bytes_dst = %lu %*u %*u %*u %*u packets_dst = %*u %lu %lu %lu %*u time = %*u",
					ip, &tx_bytes, &tp_tcp, &tp_udp, &tp_icmp, &rx_bytes, &rp_tcp, &rp_udp, &rp_icmp) != 9 ) continue;
			if (find_word(exclude, ip)) continue ;
			if ((tx_bytes > 0) || (rx_bytes > 0)){
				strncpy(tmp.ipaddr, ip, INET_ADDRSTRLEN);
				ptr = TREE_FIND(&tree, _Node, linkage, &tmp);
				if (!ptr) {
					ct_tcp = 0;
					ct_udp = 0;
				} else {
					ct_tcp = ptr->tcp_conn;
					ct_udp = ptr->udp_conn;
				}
				web_printf("%c['%s', %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu]", 
							comma, ip, rx_bytes, tx_bytes, rp_tcp, tp_tcp, rp_udp, tp_udp, rp_icmp, tp_icmp, ct_tcp, ct_udp);
				comma = ',';
			}
		}
		fclose(a);
	}
	web_puts("];\n");

	TREE_FORWARD_APPLY(&tree, _Node, linkage, Node_housekeeping, NULL);
	TREE_INIT(&tree, Node_compare);
}

void iptraffic_conntrack_init() {
	unsigned int a_time, a_proto;
	char a_src[INET_ADDRSTRLEN];
	char a_dst[INET_ADDRSTRLEN];
	char b_src[INET_ADDRSTRLEN];
	char b_dst[INET_ADDRSTRLEN];

	char sa[256];
	char sb[256];
	FILE *a;
	char *p;
	int x;

	Node tmp;
	Node *ptr;

	unsigned long rip[4];
	unsigned long lan[4];
	unsigned long mask[4];
	unsigned short int br;

	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");
		sprintf(sa, "lan%s_ifname", bridge);

		if (strcmp(nvram_safe_get(sa), "") != 0) {
			sprintf(sa, "lan%s_ipaddr", bridge);
			rip[br] = inet_addr(nvram_safe_get(sa));
			sprintf(sa, "lan%s_netmask", bridge);
			mask[br] = inet_addr(nvram_safe_get(sa));
			lan[br] = rip[br] & mask[br];
//			_dprintf("rip[%d]=%lu\n", br, rip[br]);
//			_dprintf("mask[%d]=%lu\n", br, mask[br]);
//			_dprintf("lan[%d]=%lu\n", br, lan[br]);
		} else {
			mask[br] = 0;
			rip[br] = 0;
			lan[br] = 0;
		}
	}

	const char conntrack[] = "/proc/net/ip_conntrack";

	if ((a = fopen(conntrack, "r")) == NULL) return;

	ctvbuf(a);	// if possible, read in one go

	while (fgets(sa, sizeof(sa), a)) {
		if (sscanf(sa, "%*s %u %u", &a_proto, &a_time) != 2) continue;

		if ((a_proto != 6) && (a_proto != 17)) continue;

		if ((p = strstr(sa, "src=")) == NULL) continue;
		if (sscanf(p, "src=%s dst=%s %n", a_src, a_dst, &x) != 2) continue;
		p += x;

		if ((p = strstr(p, "src=")) == NULL) continue;
		if (sscanf(p, "src=%s dst=%s", b_src, b_dst) != 2) continue;

		snprintf(sb, sizeof(sb), "%s %s %s %s", a_src, a_dst, b_src, b_dst);
		remove_dups(sb, sizeof(sb));

		char ipaddr[INET_ADDRSTRLEN], *next = NULL;
		char skip;

		foreach(ipaddr, sb, next) {
			skip = 1;
			for(br=0 ; br<=3 ; br++) {
				if ((mask[br] != 0) && ((inet_addr(ipaddr) & mask[br]) == lan[br])) {
						skip = 0;
						break;
				}
			}
			if (skip == 1) continue;

			strncpy(tmp.ipaddr, ipaddr, INET_ADDRSTRLEN);
			ptr = TREE_FIND(&tree, _Node, linkage, &tmp);

			if (!ptr) {
				_dprintf("%s: new ip: %s\n", __FUNCTION__, ipaddr);
				TREE_INSERT(&tree, _Node, linkage, Node_new(ipaddr));
				ptr = TREE_FIND(&tree, _Node, linkage, &tmp);
			}
			if (a_proto == 6) ++ptr->tcp_conn;
			if (a_proto == 17) ++ptr->udp_conn;
		}
	}
	fclose(a);
//	Tree_info();
}

