/*

	Tomato Firmware
	for policy router
	Copyright (C) 2013-2014 arctic

*/

#include "rc.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

void ipt_routerpolicy(void){
	char *nv, *nvp, *b;
	char *active, *proto, *srt_type, *srt_addr, *srt_port, *dst_type, *dst_addr, *dst_port, *wanx, *desc;
	char msrt[192],mdst[192],jump[16];
	char *msport, *mdport;
	int proto_num;
	int wan_unit,mwan_num;
	char prefix[] = "wanx";

	/*
	*mangle
	:PREROUTING ACCEPT [78857:34564987]
	:OUTPUT ACCEPT [8853:1406808]
	:POSTROUTING ACCEPT [77610:34286843]
	:PPTP - [0:0]
	:WAN_1 - [0:0]
	:WAN_2 - [0:0]
	:WAN_PBR - [0:0]
	-A PREROUTING -i ppp0 -j WAN_1 
	-A PREROUTING -i ppp1 -j WAN_2 
	-A PREROUTING -i ppp4 -j PPTP 
	-A PREROUTING -i br+ -j WAN_PBR 
	-A PREROUTING -i br+ -m conntrack --ctstate RELATED,ESTABLISHED -j CONNMARK --restore-mark 
	-A OUTPUT -m conntrack --ctstate RELATED,ESTABLISHED -j CONNMARK --restore-mark 
	-A POSTROUTING -o ppp0 -j WAN_1 
	-A POSTROUTING -o ppp1 -j WAN_2 
	-A POSTROUTING -o ppp4 -j PPTP 
	-A PPTP -m conntrack --ctstate NEW -j CONNMARK --set-return 0x500/0xf00 
	-A WAN_1 -m conntrack --ctstate NEW -j CONNMARK --set-return 0x100/0xf00 
	-A WAN_2 -m conntrack --ctstate NEW -j CONNMARK --set-return 0x200/0xf00 
	-A WAN_PBR -m state --state RELATED,ESTABLISHED -j RETURN 
	-A WAN_PBR -d 107.182.170.243 -j WAN_2 
	-A WAN_PBR -d 38.83.103.226 -j WAN_1 
	-A WAN_PBR -d 117.79.238.0/255.255.255.0 -j PPTP 
	-A WAN_PBR -d 74.125.235.111 -j PPTP 
	-A WAN_PBR -d 74.125.235.127 -j PPTP 
	-A WAN_PBR -d 74.125.235.119 -j PPTP 
	-A WAN_PBR -d 74.125.235.120 -j PPTP 
	COMMIT
	*/
	
	mwan_num = nvram_get_int("mwan_num");	
	if((mwan_num == 1 || mwan_num > MWAN_MAX)) return;
	
	for(wan_unit = 1 ; wan_unit <= mwan_num; ++wan_unit){
		get_wan_prefix(wan_unit, prefix);
		if(check_wanup(prefix)){
			ipt_write(
				":WAN_%d - [0:0]\n"
				"-A WAN_%d -m conntrack --ctstate NEW -j CONNMARK --set-return 0x%d00/0xf00\n"
				"-A PREROUTING -i %s -j WAN_%d\n"
				"-A POSTROUTING -o %s -j WAN_%d\n",
				wan_unit,
				wan_unit,
				wan_unit,
				get_wanface(prefix),
				wan_unit,
				get_wanface(prefix),
				wan_unit
				);
		}
	}

	ipt_write(
		":WAN_PBR - [0:0]\n"
		"-A WAN_PBR -m state --state RELATED,ESTABLISHED -j RETURN\n"
		"-A PREROUTING -i br+ -j WAN_PBR\n"
		"-A PREROUTING -i br+ -m conntrack --ctstate ESTABLISHED,RELATED -j CONNMARK --restore-mark\n"
		"-A OUTPUT -m conntrack --ctstate ESTABLISHED,RELATED -j CONNMARK --restore-mark\n"
		);
		
	nv = nvp = strdup(nvram_safe_get("pbr_rules"));

	if(nv) {
		while ((b = strsep(&nvp, ">")) != NULL) {
			/*
			active<proto<srt_type<srt_addr<srt_port<dst_type<dst_addr<dst_port<wanx<desc

			1<6<1<192.168.1.100<80<1<220.249.92.18<8080<1<test
			
			active:
				1 = enable
				0 = disable
			proto:
				-2 = any protocol
				-1 = both tcp/udp
				6 = TCP
				17 = UDP
				2 = ICMP
			srt_type:
				0 = all
				1 = ip
				2 = mac
			dst_type:
				0 = all
				1 = ip
				3 = domain
			wanx:
				1 = wan1
				2 = wan2
				3 = wan3
				4 = wan3

			iptables -t mangle -A WAN_PBR -p tcp -s 192.168.1.100 --sport 80 -d 220.249.92.18 --dport 80 -j MARK --set-mark-return 0x200/0xf00
			iptables -t mangle -A WAN_PBR -p tcp -s 192.168.1.1 -m multiport --dports 80:90,40 -d 220.249.92.168 -m multiport --sports 10:90,30 -j MARK --set-mark-return 0x200/0xf00
			iptables -t mangle -A WAN_PBR -p tcp -m mac --mac-source 00:0A:C2:9F:47:3E -d 202.103.44.150  -j MARK --set-mark-return 0x200/0xf00

			1<-2<0<<<1<220.249.92.18<<1<1
			*/
			if ((vstrsep(b, "<", &active, &proto, &srt_type, &srt_addr, &srt_port, &dst_type, &dst_addr, &dst_port, &wanx, &desc)!=10)) continue;
			
			// active is not 1, drop the rule
			if (*active != '1') continue;

			memset(msrt, 0, sizeof(msrt));
			if(atoi(srt_type) == 1){
				sprintf(msrt, "-s %s", srt_addr);
			}
			else if(atoi(srt_type) == 2){
				sprintf(msrt, "-m mac --mac-source %s", srt_addr);
			}
			
			memset(jump, 0, sizeof(jump));
			if(atoi(wanx) >= 1 && atoi(wanx) <= 4){
				// wanup check fail, drop the rule
				get_wan_prefix(atoi(wanx), prefix);
				if(!check_wanup(prefix)) continue;
				sprintf(jump, "WAN_%s", wanx);
			}
			else{
				continue;
			}
			
			if(atoi(dst_type) == 3){
				int i;
				struct hostent *he;
				struct in_addr **addr_list;
				if ((he = gethostbyname(dst_addr)) == NULL) continue;

				addr_list = (struct in_addr **)he->h_addr_list;
				
				i = 0;
				while(addr_list[i] != NULL){
					memset(mdst, 0, sizeof(mdst));
					sprintf(mdst, "-d %s", inet_ntoa(*addr_list[i]));

					msport = (strchr(srt_port, ',') != NULL) ? " -m multiport --sports " : " --sport ";
					mdport = (strchr(dst_port, ',') != NULL) ? " -m multiport --dports " : " --dport ";

					// protocol & ports
					proto_num = atoi(proto);
					if (proto_num > -2) {
						if ((proto_num == 6) || (proto_num == 17) || (proto_num == -1)) {
							if (proto_num != 6){
								ipt_write("-A WAN_PBR -p %s %s%s%s %s%s%s -j %s\n",
									"udp",
									msrt,
									(*srt_port) ? msport : "",
									(*srt_port) ? srt_port : "",
									mdst,
									(*dst_port) ? mdport : "",
									(*dst_port) ? dst_port : "",
									jump);
							}
							if (proto_num != 17) {
								ipt_write("-A WAN_PBR -p %s %s%s%s %s%s%s -j %s\n",
									"tcp",
									msrt,
									(*srt_port) ? msport : "",
									(*srt_port) ? srt_port : "",
									mdst,
									(*dst_port) ? mdport : "",
									(*dst_port) ? dst_port : "",
									jump);
							}
						}
						else {
							ipt_write("-A WAN_PBR -p %s %s %s -j %s\n",
								proto_num,
								msrt,
								mdst,
								jump);
						}
					}
					else {	// any protocol
						ipt_write("-A WAN_PBR %s %s -j %s\n",
							msrt,
							mdst,
							jump);
					}
					i++;
				}
			}
			else
			{
				memset(mdst, 0, sizeof(mdst));
				if(atoi(dst_type) != 0){
					sprintf(mdst, "-d %s", dst_addr);
				}

				msport = (strchr(srt_port, ',') != NULL) ? " -m multiport --sports " : " --sport ";
				mdport = (strchr(dst_port, ',') != NULL) ? " -m multiport --dports " : " --dport ";


				// protocol & ports
				proto_num = atoi(proto);
				if (proto_num > -2) {
					if ((proto_num == 6) || (proto_num == 17) || (proto_num == -1)) {
						if (proto_num != 6){
							ipt_write("-A WAN_PBR -p %s %s%s%s %s%s%s -j %s\n",
								"udp",
								msrt,
								(*srt_port) ? msport : "",
								(*srt_port) ? srt_port : "",
								mdst,
								(*dst_port) ? mdport : "",
								(*dst_port) ? dst_port : "",
								jump);
						}
						if (proto_num != 17) {
							ipt_write("-A WAN_PBR -p %s %s%s%s %s%s%s -j %s\n",
								"tcp",
								msrt,
								(*srt_port) ? msport : "",
								(*srt_port) ? srt_port : "",
								mdst,
								(*dst_port) ? mdport : "",
								(*dst_port) ? dst_port : "",
								jump);
						}
					}
					else {
						ipt_write("-A WAN_PBR -p %s %s %s -j %s\n",
							proto_num,
							msrt,
							mdst,
							jump);
					}
				}
				else {	// any protocol
					ipt_write("-A WAN_PBR %s %s -j %s\n",
						msrt,
						mdst,
						jump);
				}
			}
		}
	}
	free(nv);
}
