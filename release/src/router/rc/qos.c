/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate



Notes:

Originally Tomato had 10 classes, but only used 6 priority levels for some unknown reason.
Therefore, the last 4 classes all had the same prio of 7. The ingress system had no priority system
and allowed only class limits. It looked as if this was a legacy of an earlier prototype using CBQ.

On 4th February 2012 a new IMQ based ingress system was added to Toastman builds which
allows use of priorities on all incoming classes. QOS now uses 10 prios in both egress and ingress.

An incoming bandwidth pie chart was added at the same time, making it easier to see the result of 
QOS rules on incoming data.

-Toastman 

*/

#include "rc.h"

#include <sys/stat.h>

static const char *qosfn = "/etc/qos";
static const char *qosImqDeviceNumberString = "0";
static const char *qosImqDeviceString = "imq0";

// in mangle table
void ipt_qos(void)
{
	char *buf;
	char *g;
	char *p;
	char *addr_type, *addr;
	char *proto;
	char *port_type, *port;
	char *class_prio;
	char *ipp2p, *layer7;
	char *bcount;
	char *dscp;
	char *desc;
	int class_num;
	int proto_num;
	int v4v6_ok;
	int i;
	char sport[192];
	char saddr[192];
	char end[256];
	char s[32];
	char app[128];
	int inuse;
	const char *chain;
	unsigned long min;
	unsigned long max;
	unsigned long prev_max;
	int gum;
	const char *qface;
	int sizegroup;
	int class_flag;
	int rule_num;

	if (!nvram_get_int("qos_enable")) return;

	inuse = 0;
	gum = 0x100;
	sizegroup = 0;
	prev_max = 0;
	rule_num = 0;

	ip46t_write(
		":QOSO - [0:0]\n"
		"-A QOSO -j CONNMARK --restore-mark --mask 0xff\n"
		"-A QOSO -m connmark ! --mark 0/0x0f00 -j RETURN\n");

	g = buf = strdup(nvram_safe_get("qos_orules"));
	while (g) {

		/*

		addr_type<addr<proto<port_type<port<ipp2p<L7<bcount<dscp<class_prio<desc

		addr_type:
			0 = any
			1 = dest ip
			2 = src ip
			3 = src mac
		addr:
			ip/mac if addr_type == 1-3
		proto:
			0-65535 = protocol
			-1 = tcp or udp
			-2 = any protocol
		port_type:
			if proto == -1,tcp,udp:
				d = dest
				s = src
				x = both
				a = any
		port:
			port # if proto == -1,tcp,udp
		bcount:
			min:max
			blank = none
		dscp:
			empty - any
			numeric (0:63) - dscp value
			afXX, csX, be, ef - dscp class
		class_prio:
			0-10				// was 0-8 - Changed from 8 in pkt_sched.h - Toastman
			-1 = disabled

		*/

		if ((p = strsep(&g, ">")) == NULL) break;
		i = vstrsep(p, "<", &addr_type, &addr, &proto, &port_type, &port, &ipp2p, &layer7, &bcount, &dscp, &class_prio, &desc);
		rule_num++;
		if (i == 10) {
			// fixup < v1.28.XX55
			desc = class_prio;
			class_prio = dscp;
			dscp = "";
		}
		else if (i == 9) {
			// fixup < v0.08		// !!! temp
			desc = class_prio;
			class_prio = bcount;
			bcount = "";
			dscp = "";
		}
		else if (i != 11) continue;

		class_num = atoi(class_prio);
		if ((class_num < 0) || (class_num > 9)) continue;

		i = 1 << class_num;
		++class_num;

		if ((inuse & i) == 0) {
			inuse |= i;
		}
		
		v4v6_ok = IPT_V4;
#ifdef TCONFIG_IPV6
		if (ipv6_enabled())
			v4v6_ok |= IPT_V6;
#endif
		class_flag = gum;

		//
		if (ipt_ipp2p(ipp2p, app)) v4v6_ok &= ~IPT_V6;
		else ipt_layer7(layer7, app);
		if (app[0]) {
			v4v6_ok &= ~IPT_V6; // temp: l7 not working either!
			class_flag = 0x100;
			// IPP2P and L7 rules may need more than one packet before matching
			// so port-based rules that come after them in the list can't be sticky
			// or else these rules might never match.
			gum = 0;
		}
		strcpy(end, app);

		// dscp
		if (ipt_dscp(dscp, s)) {
#ifndef LINUX26
			v4v6_ok &= ~IPT_V6; // dscp ipv6 match is not present in K2.4
#endif
			strcat(end, s);
		}

		// mac or ip address
		if ((*addr_type == '1') || (*addr_type == '2')) {	// match ip
			v4v6_ok &= ipt_addr(saddr, sizeof(saddr), addr, (*addr_type == '1') ? "dst" : "src", 
				v4v6_ok, (v4v6_ok==IPT_V4), "QoS", desc);
			if (!v4v6_ok) continue;
		}
		else if (*addr_type == '3') {						// match mac
			sprintf(saddr, "-m mac --mac-source %s", addr);	// (-m mac modified, returns !match in OUTPUT)
		}
		else {
			saddr[0] = 0;
		}


		// -m connbytes --connbytes x:y --connbytes-dir both --connbytes-mode bytes
		if (*bcount) {
			min = strtoul(bcount, &p, 10);
			if (*p != 0) {
				strcat(end, " -m connbytes --connbytes-mode bytes --connbytes-dir both --connbytes ");
				++p;
				if (*p == 0) {
					sprintf(end + strlen(end), "%lu:", min * 1024);
				}
				else {
					max = strtoul(p, NULL, 10);
					sprintf(end + strlen(end), "%lu:%lu", min * 1024, (max * 1024) - 1);
					if (gum) {
						if (!sizegroup) {
							// Create table of connbytes sizes, pass appropriate connections there
							// and only continue processing them if mark was wiped
							ip46t_write(
								":QOSSIZE - [0:0]\n"
								"-I QOSO 3 -m connmark ! --mark 0/0xff000 -j QOSSIZE\n"
								"-I QOSO 4 -m connmark ! --mark 0/0xff000 -j RETURN\n");
						}
						if (max != prev_max && sizegroup<255) {
							class_flag = ++sizegroup << 12;
							prev_max = max;
							ip46t_flagged_write(v4v6_ok,
								"-A QOSSIZE -m connmark --mark 0x%x/0xff000"
								" -m connbytes --connbytes-mode bytes --connbytes-dir both --connbytes %lu: -j CONNMARK --set-return 0x00000/0xFF\n",
									(sizegroup << 12), (max * 1024));
						}
						else {
							class_flag = sizegroup << 12;
						}
					}
				}

			}
			else {
				bcount = "";
			}
		}

		chain = "QOSO";
		class_num |= class_flag;
		class_num |= rule_num << 20;
		sprintf(end + strlen(end), " -j CONNMARK --set-return 0x%x/0xFF\n", class_num);

		// protocol & ports
		proto_num = atoi(proto);
		if (proto_num > -2) {
			if ((proto_num == 6) || (proto_num == 17) || (proto_num == -1)) {
				if (*port_type != 'a') {
					if ((*port_type == 'x') || (strchr(port, ','))) {
						// dst-or-src port matches, and anything with multiple lists "," use multiport
						sprintf(sport, "-m multiport --%sports %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
					}
					else {
						// single or simple x:y range, use built-in tcp/udp match
						sprintf(sport, "--%sport %s", (*port_type == 's') ? "s" : ((*port_type == 'd') ? "d" : ""), port);
					}
				}
				else {
					sport[0] = 0;
				}
				if (proto_num != 6) ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s %s", chain, "udp", sport, saddr, end);
				if (proto_num != 17) ip46t_flagged_write(v4v6_ok, "-A %s -p %s %s %s %s", chain, "tcp", sport, saddr, end);
			}
			else {
				ip46t_flagged_write(v4v6_ok, "-A %s -p %d %s %s", chain, proto_num, saddr, end);
			}
		}
		else {	// any protocol
			ip46t_flagged_write(v4v6_ok, "-A %s %s %s", chain, saddr, end);
		}

	}
	free(buf);

	qface = wanfaces.iface[0].name;

	i = nvram_get_int("qos_default");
	if ((i < 0) || (i > 9)) i = 3;	// "low"
	class_num = i + 1;
	class_num |= 0xFF00000; // use rule_num=255 for default
	ip46t_write("-A QOSO -j CONNMARK --set-return 0x%x\n", class_num);
	
	ipt_write(
		"-A FORWARD -o %s -j QOSO\n"
		"-A OUTPUT -o %s -j QOSO\n",
			qface, qface);

#ifdef TCONFIG_IPV6
	if (*wan6face) {
		ip6t_write(
			"-A FORWARD -o %s -j QOSO\n"
			"-A OUTPUT -o %s -j QOSO\n",
			wan6face, wan6face);
	}
#endif

	inuse |= (1 << i) | 1;	// default and highest are always built
	sprintf(s, "%d", inuse);
	nvram_set("qos_inuse", s);


	g = buf = strdup(nvram_safe_get("qos_irates"));
	
	for (i = 0; i < 10; ++i) 
	{
		if ((!g) || ((p = strsep(&g, ",")) == NULL)) continue;
		if ((inuse & (1 << i)) == 0) continue;
		
		unsigned int rate;
		unsigned int ceil;
		
		// check if we've got a percentage definition in the form of "rate-ceiling"
		// and that rate > 1
		if ((sscanf(p, "%u-%u", &rate, &ceil) == 2) && (rate >= 1))
		{		
			ipt_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", qface);


		if (nvram_get_int("qos_udp")) {
			ipt_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %s\n", qface, qosImqDeviceNumberString);	// pass only tcp
		}
		else {
			ipt_write("-A PREROUTING -i %s -j IMQ --todev %s\n", qface, qosImqDeviceNumberString);	// pass everything thru ingress
		}

#ifdef TCONFIG_IPV6
			if (*wan6face)
			{
				ip6t_write("-A PREROUTING -i %s -j CONNMARK --restore-mark --mask 0xff\n", wan6face);

				if (nvram_get_int("qos_udp")) {
						ip6t_write("-A PREROUTING -i %s -p tcp -j IMQ --todev %s\n", wan6face, qosImqDeviceNumberString);	// pass only tcp
				}
				else {
						ip6t_write("-A PREROUTING -i %s -j IMQ --todev %s\n", wan6face, qosImqDeviceNumberString);	// pass everything thru ingress
				}
			}
#endif
			break;
		}
	}
	free(buf);
}

static unsigned calc(unsigned bw, unsigned pct)
{
	unsigned n = ((unsigned long)bw * pct) / 100;
	return (n < 2) ? 2 : n;
}

void start_qos(void)
{
	int i;
	char *buf, *g, *p;
	unsigned int rate;
	unsigned int ceil;
	unsigned int bw;
	unsigned int incomingBandwidthInKilobitsPerSecond;
	unsigned int mtu;
	unsigned int r2q;
	unsigned int qosDefaultClassId;
	unsigned int overhead;
	FILE *f;
	int x;
	int inuse;
	char s[256];
	int first;
	char burst_root[32];
	char burst_leaf[32];

	qosDefaultClassId = (nvram_get_int("qos_default") + 1) * 10;
	incomingBandwidthInKilobitsPerSecond = strtoul(nvram_safe_get("qos_ibw"), NULL, 10);
	
	// move me?
	x = nvram_get_int("ne_vegas");
#ifdef LINUX26
	if (x) {
		char alpha[10], beta[10], gamma[10];
		sprintf(alpha, "alpha=%d", nvram_get_int("ne_valpha"));
		sprintf(beta, "beta=%d", nvram_get_int("ne_vbeta"));
		sprintf(gamma, "gamma=%d", nvram_get_int("ne_vgamma"));
		modprobe("tcp_vegas", alpha, beta, gamma);
		f_write_string("/proc/sys/net/ipv4/tcp_congestion_control", "vegas", 0, 0);
	}
	else {
		modprobe_r("tcp_vegas");
		f_write_string("/proc/sys/net/ipv4/tcp_congestion_control", "", FW_NEWLINE, 0);
	}
#else
	f_write_string("/proc/sys/net/ipv4/tcp_vegas_cong_avoid", x ? "1" : "0", 0, 0);
	if (x) {
		f_write_string("/proc/sys/net/ipv4/tcp_vegas_alpha", nvram_safe_get("ne_valpha"), 0, 0);
		f_write_string("/proc/sys/net/ipv4/tcp_vegas_beta", nvram_safe_get("ne_vbeta"), 0, 0);
		f_write_string("/proc/sys/net/ipv4/tcp_vegas_gamma", nvram_safe_get("ne_vgamma"), 0, 0);
	}
#endif

	if (!nvram_get_int("qos_enable")) return;

	if ((f = fopen(qosfn, "w")) == NULL) return;

	i = nvram_get_int("qos_burst0");
	if (i > 0) sprintf(burst_root, "burst %dk", i);
		else burst_root[0] = 0;
	i = nvram_get_int("qos_burst1");
	if (i > 0) sprintf(burst_leaf, "burst %dk", i);
		else burst_leaf[0] = 0;

	mtu = strtoul(nvram_safe_get("wan_mtu"), NULL, 10);
	bw = strtoul(nvram_safe_get("qos_obw"), NULL, 10);
	overhead = strtoul(nvram_safe_get("atm_overhead"), NULL, 10);
	r2q = 10;

	if ((bw * 1000) / (8 * r2q) < mtu) {
		r2q = (bw * 1000) / (8 * mtu);
		if (r2q < 1) r2q = 1;
	} else if ((bw * 1000) / (8 * r2q) > 60000) {
		r2q = (bw * 1000) / (8 * 60000) + 1;
	}

	if (overhead == 0) {
		fprintf(f,
			"#!/bin/sh\n"
			"WAN_DEV=%s\n"
			"IMQ_DEV=%s\n"
			"TQA=\"tc qdisc add dev $WAN_DEV\"\n"
			"TCA=\"tc class add dev $WAN_DEV\"\n"
			"TFA=\"tc filter add dev $WAN_DEV\"\n"
			"TQA_IMQ=\"tc qdisc add dev $IMQ_DEV\"\n"
			"TCA_IMQ=\"tc class add dev $IMQ_DEV\"\n"
			"TFA_IMQ=\"tc filter add dev $IMQ_DEV\"\n"
			"Q=\"%s\"\n"
			"\n"
			"case \"$1\" in\n"
			"start)\n"
			"\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n"
			"\t$TQA root handle 1: htb default %u r2q %u\n"
			"\t$TCA parent 1: classid 1:1 htb rate %ukbit ceil %ukbit %s\n",
				get_wanface(),
				qosImqDeviceString,
				nvram_get_int("qos_pfifo") ? "pfifo limit 256" : "sfq perturb 10",
				qosDefaultClassId, r2q,
				bw, bw, burst_root);
	} else {
		fprintf(f,
			"#!/bin/sh\n"
			"WAN_DEV=%s\n"
			"IMQ_DEV=%s\n"
			"TQA=\"tc qdisc add dev $WAN_DEV\"\n"
			"TCA=\"tc class add dev $WAN_DEV\"\n"
			"TFA=\"tc filter add dev $WAN_DEV\"\n"
			"TQA_IMQ=\"tc qdisc add dev $IMQ_DEV\"\n"
			"TCA_IMQ=\"tc class add dev $IMQ_DEV\"\n"
			"TFA_IMQ=\"tc filter add dev $IMQ_DEV\"\n"
			"Q=\"%s\"\n"
			"\n"
			"case \"$1\" in\n"
			"start)\n"
			"\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n"
			"\t$TQA root handle 1: htb default %u r2q %u\n"
			"\t$TCA parent 1: classid 1:1 htb rate %ukbit ceil %ukbit %s overhead %u atm\n",
				get_wanface(),
				qosImqDeviceString,
				nvram_get_int("qos_pfifo") ? "pfifo limit 256" : "sfq perturb 10",
				qosDefaultClassId, r2q,
				bw, bw, burst_root, overhead);
		}

	inuse = nvram_get_int("qos_inuse");

	g = buf = strdup(nvram_safe_get("qos_orates"));
	for (i = 0; i < 10; ++i) {
		if ((!g) || ((p = strsep(&g, ",")) == NULL)) break;

		if ((inuse & (1 << i)) == 0) continue;

		// check if we've got a percentage definition in the form of "rate-ceiling"
		if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1)) continue;	// 0=off

		if (ceil > 0) sprintf(s, "ceil %ukbit ", calc(bw, ceil));
			else s[0] = 0;
		x = (i + 1) * 10;

		if (overhead == 0) {
			fprintf(f,
				"# egress %d: %u-%u%%\n"
				"\t$TCA parent 1:1 classid 1:%d htb rate %ukbit %s %s prio %d quantum %u\n"
				"\t$TQA parent 1:%d handle %d: $Q\n"
				"\t$TFA parent 1: prio %d protocol ip handle %d fw flowid 1:%d\n",
					i, rate, ceil,
					x, calc(bw, rate), s, burst_leaf, i+1, mtu,
					x, x,
					x, i + 1, x);
		} else {
			fprintf(f,
				"# egress %d: %u-%u%%\n"
				"\t$TCA parent 1:1 classid 1:%d htb rate %ukbit %s %s prio %d quantum %u overhead %u atm\n"
				"\t$TQA parent 1:%d handle %d: $Q\n"
				"\t$TFA parent 1: prio %d protocol ip handle %d fw flowid 1:%d\n",
					i, rate, ceil,
					x, calc(bw, rate), s, burst_leaf, i+1, mtu, overhead,
					x, x,
					x, i + 1, x);
		}
	}
	free(buf);

//		"\t$TFA parent 1: prio 10 protocol ip u32 match ip tos 0x10 0xff flowid :10\n"	// TOS EF -> Highest


/*
		if (nvram_match("qos_ack", "1")) {
			fprintf(f,
				"\n"
				"\t$TFA parent 1: prio 15 protocol ip u32 "
				"match ip protocol 6 0xff "		// TCP
				"match u8 0x05 0x0f at 0 "		// IP header length
				"match u16 0x0000 0xffc0 at 2 "	// total length (0-63)
				"match u8 0x10 0xff at 33 "		// ACK only
				"flowid 1:10\n");
		}
		if (nvram_match("qos_icmp", "1")) {
			fputs("\n\t$TFA parent 1: prio 14 protocol ip u32 match ip protocol 1 0xff flowid 1:10\n", f);
		}
*/

/*
	if (nvram_get_int("qos_ack")) {
		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 14 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u16 0x0000 0xff80 at 2 "		// total length (0-127)
			"match u8 0x10 0xff at 33 "			// ACK only
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_syn")) {
		//	10000 = ACK
		//	00010 = SYN

		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 15 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u16 0x0000 0xff80 at 2 "		// total length (0-127)
			"match u8 0x02 0xff at 33 "			// SYN only
			"flowid 1:10\n"
			"\n"
			"\t$TFA parent 1: prio 16 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u16 0x0000 0xff80 at 2 "		// total length (0-127)
			"match u8 0x12 0xff at 33 "			// SYN,ACK
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_fin")) {
		//	10000 = ACK
		//	00001 = FIN

		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 17 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u8 0x11 0xff at 33 "			// ACK,FIN
			"flowid 1:10\n"
			"\n"
			"\t$TFA parent 1: prio 18 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u8 0x01 0xff at 33 "			// FIN
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_rst")) {
		//	10000 = ACK
		//	00100 = RST
		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 19 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u8 0x14 0xff at 33 "			// ACK,RST
			"flowid 1:10\n"
			"\n"
			"\t$TFA parent 1: prio 20 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u8 0x04 0xff at 33 "			// RST
			"flowid 1:10\n");
	}


	if (nvram_get_int("qos_icmp")) {
		fputs("\n\t$TFA parent 1: prio 13 protocol ip u32 match ip protocol 1 0xff flowid 1:10\n", f);
	}
*/

	/*
		10000 = ACK
		00100 = RST
		00010 = SYN
		00001 = FIN
	*/

	if (nvram_get_int("qos_ack")) {
		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 14 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
//			"match u16 0x0000 0xff80 at 2 "		// total length (0-127)
			"match u16 0x0000 0xffc0 at 2 "		// total length (0-63)
			"match u8 0x10 0xff at 33 "			// ACK only
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_syn")) {
		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 15 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u16 0x0000 0xffc0 at 2 "		// total length (0-63)
			"match u8 0x02 0x02 at 33 "			// SYN,*
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_fin")) {
		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 17 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u16 0x0000 0xffc0 at 2 "		// total length (0-63)
			"match u8 0x01 0x01 at 33 "			// FIN,*
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_rst")) {
		fprintf(f,
			"\n"
			"\t$TFA parent 1: prio 19 protocol ip u32 "
			"match ip protocol 6 0xff "			// TCP
			"match u8 0x05 0x0f at 0 "			// IP header length
			"match u16 0x0000 0xffc0 at 2 "		// total length (0-63)
			"match u8 0x04 0x04 at 33 "			// RST,*
			"flowid 1:10\n");
	}

	if (nvram_get_int("qos_icmp")) {
		fputs("\n\t$TFA parent 1: prio 13 protocol ip u32 match ip protocol 1 0xff flowid 1:10\n", f);
	}


	////
	//// INCOMING TRAFFIC SHAPING
	////
	
	first = 1;
	overhead = strtoul(nvram_safe_get("atm_overhead"), NULL, 10);

	g = buf = strdup(nvram_safe_get("qos_irates"));
	
	for (i = 0; i < 10; ++i)
	{	
		if ((!g) || ((p = strsep(&g, ",")) == NULL))
		{
			break;
		}
		
		if ((inuse & (1 << i)) == 0)
		{
			continue;
		}

		// check if we've got a percentage definition in the form of "rate-ceiling"
		if ((sscanf(p, "%u-%u", &rate, &ceil) != 2) || (rate < 1))
		{
			continue;	// 0=off
		}
		
		// class ID
		unsigned int classid = ((unsigned int)i + 1) * 10;
		
		// priority
		unsigned int priority = (unsigned int)i + 1;			//prios 1-10 - Toastman
		
		// rate in kb/s
		unsigned int rateInKilobitsPerSecond =
			calc(incomingBandwidthInKilobitsPerSecond, rate);
		
		// ceiling in kb/s
		unsigned int ceilingInKilobitsPerSecond =
			calc(incomingBandwidthInKilobitsPerSecond, ceil);

		// burst rate (2% of the classes' rate) - don't know if we should use this
//Commented out KDB 20130531 - produces compiler warning about being unused!
//		unsigned int burstRateInBitsPerSecond =
//			(rateInKilobitsPerSecond * 1000) / 50;

		r2q = 10;
		if ((incomingBandwidthInKilobitsPerSecond * 1000) / (8 * r2q) < mtu) 
		{
			r2q = (incomingBandwidthInKilobitsPerSecond * 1000) / (8 * mtu);
			if (r2q < 1) r2q = 1;
		} 
		else if ((incomingBandwidthInKilobitsPerSecond * 1000) / (8 * r2q) > 60000) 
		{
			r2q = (incomingBandwidthInKilobitsPerSecond * 1000) / (8 * 60000) + 1;
		}

		if (first)
		{
			first = 0;
			if (overhead == 0) {
				fprintf(f,
				"\n"
				"\tip link set $IMQ_DEV up\n"
				"\ttc qdisc del dev $IMQ_DEV 2>/dev/null\n"
				"\t$TQA_IMQ handle 1: root htb default %u r2q %u\n"
				"\t$TCA_IMQ parent 1: classid 1:1 htb rate %ukbit ceil %ukbit\n",
				qosDefaultClassId, r2q,
				incomingBandwidthInKilobitsPerSecond,
				incomingBandwidthInKilobitsPerSecond);
			} else {
				fprintf(f,
					"\n"
					"\tip link set $IMQ_DEV up\n"
					"\ttc qdisc del dev $IMQ_DEV 2>/dev/null\n"
					"\t$TQA_IMQ handle 1: root htb default %u r2q %u\n"
					"\t$TCA_IMQ parent 1: classid 1:1 htb rate %ukbit ceil %ukbit overhead %u atm\n",
					qosDefaultClassId, r2q,
					incomingBandwidthInKilobitsPerSecond,
					incomingBandwidthInKilobitsPerSecond, overhead);
				}
		}
		
		fprintf(
			f,
			"\n"
			"\t# class id %u: rate %ukbit ceil %ukbit\n",
			classid, rateInKilobitsPerSecond, ceilingInKilobitsPerSecond);

		if (overhead == 0) {
			fprintf(
				f,
				"\t$TCA_IMQ parent 1:1 classid 1:%u htb rate %ukbit ceil %ukbit prio %u quantum %u\n",
				classid, rateInKilobitsPerSecond, ceilingInKilobitsPerSecond, priority, mtu);
		} else {
			fprintf(
				f,
				"\t$TCA_IMQ parent 1:1 classid 1:%u htb rate %ukbit ceil %ukbit prio %u quantum %u overhead %u atm\n",
				classid, rateInKilobitsPerSecond, ceilingInKilobitsPerSecond, priority, mtu, overhead);
			}

		fprintf(
			f,
			"\t$TQA_IMQ parent 1:%u handle %u: $Q\n",
			classid, classid);

		fprintf(
			f,
			"\t$TFA_IMQ parent 1: prio %u protocol ip handle %u fw flowid 1:%u \n",           
			classid, priority, classid);
	}

	free(buf);

	//// write commands which adds rule to forward traffic to IMQ device
	fputs(
		"\n"
		"\t# set up the IMQ device (otherwise this won't work) to limit the incoming data\n"
		"\tip link set $IMQ_DEV up\n",
		f);

	fputs(
		"\t;;\n"
		"stop)\n"
		"\tip link set $IMQ_DEV down\n"
		"\ttc qdisc del dev $WAN_DEV root 2>/dev/null\n"
		"\ttc qdisc del dev $IMQ_DEV root 2>/dev/null\n"
		"\t;;\n"
		"*)\n"
		"\techo \"...\"\n"
		"\techo \"... OUTGOING QDISCS AND CLASSES FOR $WAN_DEV\"\n"
		"\techo \"...\"\n"
		"\ttc -s -d qdisc ls dev $WAN_DEV\n"
		"\techo\n"
		"\ttc -s -d class ls dev $WAN_DEV\n"
		"\techo\n"
		"\techo \"...\"\n"
		"\techo \"... INCOMING QDISCS AND CLASSES FOR $WAN_DEV (routed through $IMQ_DEV)\"\n"
		"\techo \"...\"\n"
		"\ttc -s -d qdisc ls dev $IMQ_DEV\n"
		"\techo\n"
		"\ttc -s -d class ls dev $IMQ_DEV\n"
		"\techo\n"
		"esac\n",
		f);

	fclose(f);
	chmod(qosfn, 0700);
	eval((char *)qosfn, "start");
}

void stop_qos(void)
{
	eval((char *)qosfn, "stop");
/*
	if (!nvram_match("debug_keepfiles", "1")) {
		unlink(qosfn);
	}
*/
}

/*

PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
		   QD         |                         ^
					  |                         |
					  v                         |
					INPUT (f)                 OUTPUT (mnf)


*/
