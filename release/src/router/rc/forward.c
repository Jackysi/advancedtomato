/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate

*/

#include "rc.h"

#include <arpa/inet.h>


static const char tcpudp[2][4] = {"tcp", "udp"};

void ipt_forward(ipt_table_t table)
{
	char *nv, *nvp, *b;
	const char *proto, *saddr, *xports, *iport, *iaddr;
	const char *c;
	const char *mdport;
	int i;
	char ip[64];
	char src[64];

	nvp = nv = strdup(nvram_safe_get("portforward"));
	if (!nv) return;

	while ((b = strsep(&nvp, ">")) != NULL) {
		/*
			[<1.01] 1<3<30,40-45<60<5<desc
			[<1.07] 1<3<30,40-45<60<192.168.1.5<desc

			1<3<71.72.73.74<30,40-45<60<192.168.1.5<desc

			1 = enabled
			3 = tcp & udp
			71.72.73.74 = src addr
			30,40-45 = ext port
			60 = int port
			192.168.1.5 = dst addr
			desc = desc

		*/
		if ((vstrsep(b, "<", &c, &proto, &saddr, &xports, &iport, &iaddr) != 6) || (*c != '1')) continue;
		
		src[0] = 0;
		if (*saddr != 0) {
			if (strchr(saddr, '.') == NULL) {
				// <1.07
				iaddr = iport;
				iport = xports;
				xports = saddr;
				saddr = "";
			}
			else if (strlen(saddr) < 32) {
				sprintf(src, "-%s %s", strchr(saddr, '-') ? "m iprange --src-range" : "s", saddr);
			}
		}

		mdport = (strchr(xports, ',') != NULL) ? "-m mport --dports" : "--dport";
		for (i = 0; i < 2; ++i) {
			if ((1 << i) & (*proto - '0')) {
				c = tcpudp[i];
				if (strchr(iaddr, '.')) {
					strlcpy(ip, iaddr, sizeof(ip));
				}
				else {
					// < 1.01: 5 -> 192.168.1.5
					strcpy(ip, lan_cclass);
					strlcat(ip, iaddr, sizeof(ip));
				}
				if (table == IPT_TABLE_NAT) {
					ipt_write("-A PREROUTING -p %s %s -d %s %s %s -j DNAT --to-destination %s%s%s\n",
						c,
						src,
						wanaddr,
						mdport, xports,
						ip,  *iport ? ":" : "", iport);

					if (nvram_match("nf_loopback", "1")) {
#if 1
						ipt_write("-A POSTROUTING -p %s %s %s -s %s/%s -d %s -j SNAT --to-source %s\n",
							c,
							mdport, *iport ? iport : xports,
							nvram_safe_get("lan_ipaddr"),	// corrected by ipt
							nvram_safe_get("lan_netmask"),
							ip,
							nvram_safe_get("lan_ipaddr"));
#else
						ipt_write("-A POSTROUTING -p %s %s %s -s %s/%s -d %s -j SNAT --to-source %s\n",
							c,
							mdport, xports,
							nvram_safe_get("lan_ipaddr"),	// corrected by ipt
							nvram_safe_get("lan_netmask"),
							ip,
							nvram_safe_get("lan_ipaddr"));
#endif
					}
				}
				else {	// filter
					ipt_write("-A wanin %s -p %s -m %s -d %s %s %s -j %s\n",
						src,
						c,
						c,
						ip,
						mdport, *iport ? iport : xports,
						chain_in_accept);
				}
			}
		}
	}
	free(nv);
}


void ipt_triggered(ipt_table_t table)
{
	char *nv, *nvp, *b;
	const char *proto, *mports, *fports;
	const char *c;
	char *p;
	int i;
	int first;
	char s[256];

	nvp = nv = strdup(nvram_safe_get("trigforward"));
	if (!nv) return;

	first = 1;
	while ((b = strsep(&nvp, ">")) != NULL) {
		if ((vstrsep(b, "<", &c, &proto, &mports, &fports) != 4) || (*c != '1')) continue;
		for (i = 0; i < 2; ++i) {
			if ((1 << i) & (*proto - '0')) {
				if (first) {
					// should only be created if there is at least one enabled

					if (table == IPT_TABLE_NAT) {
						ipt_write("-A PREROUTING -d %s -j TRIGGER --trigger-type dnat\n", wanaddr);
						goto QUIT;
					}

					ipt_write(":triggers - [0:0]\n"
							  "-A wanout -j triggers\n"
							  "-A wanin -j TRIGGER --trigger-type in\n");

					first = 0;
				}
				strlcpy(s, mports, sizeof(s));
				if ((p = strchr(s, ':')) != NULL) *p = '-';
				if ((p = strchr(fports, ':')) != NULL) *p = '-';
				c = tcpudp[i];
				ipt_write("-A triggers -p %s -m %s --dport %s "
						  "-j TRIGGER --trigger-type out --trigger-proto %s --trigger-match %s --trigger-relate %s\n",
							c, c, mports,
							c, s, fports);
				// can't use mport... trigger-match must be set to the same
				// ports as dport since it's used to refresh timer during inbound	-- zzz
			}
		}
	}

QUIT:
	free(nv);
}
