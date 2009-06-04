/* Quake3 extension for UDP NAT alteration.
 * (C) 2002 by Filip Sneppe <filip.sneppe@cronos.be>
 * based on ip_nat_ftp.c and ip_nat_tftp.c
 *
 * ip_nat_quake3.c v0.0.3 2002-08-31
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *      Module load syntax:
 *      insmod ip_nat_quake3.o ports=port1,port2,...port<MAX_PORTS>
 *
 *      please give the ports of all Quake3 master servers You wish to
 *      connect to. If you don't specify ports, the default will be UDP
 *      port 27950.
 *
 *      Thanks to the Ethereal folks for their analysis of the Quake3 protocol.
 *
 *      Notes: 
 *      - If you're one of those people who would try anything to lower
 *        latency while playing Quake (and who isn't :-) ), you may want to
 *        consider not loading ip_nat_quake3 at all and just MASQUERADE all
 *        outgoing UDP traffic.
 *        This will make ip_conntrack_quake3 add the necessary expectations,
 *        but there will be no overhead for client->server UDP streams. If
 *        ip_nat_quake3 is loaded, quake3_nat_expected will be called per NAT
 *        hook for every packet in the client->server UDP stream.
 *      - Only SNAT/MASQUEARDE targets are useful for ip_nat_quake3.
 *        The IP addresses in the master connection payload (=IP addresses
 *        of Quake servers) have no relation with the master server so
 *        DNAT'ing the master connection to a server should not change the
 *        expected connections.
 *      - Not tested due to lack of equipment:
 *        - multiple Quake3 clients behind one MASQUERADE gateway
 *        - what if Quake3 client is running on router too
 */

#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_quake3.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>

MODULE_AUTHOR("Filip Sneppe <filip.sneppe@cronos.be>");
MODULE_DESCRIPTION("Netfilter NAT helper for Quake III Arena");
MODULE_LICENSE("GPL");

#define MAX_PORTS 8

static int ports[MAX_PORTS];
static int ports_c = 0;
#ifdef MODULE_PARM
MODULE_PARM(ports,"1-" __MODULE_STRING(MAX_PORTS) "i");
MODULE_PARM_DESC(ports, "port numbers of Quake III master servers");
#endif

/* Quake3 master server reply will add > 100 expectations per reply packet; when
   doing lots of printk's, klogd may not be able to read /proc/kmsg fast enough */
#if 0 
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

static struct quake3_search quake3s_nat = { "****", "getserversResponse", sizeof("getserversResponse") - 1 };

static unsigned int 
quake3_nat_help(struct ip_conntrack *ct,
                struct ip_conntrack_expect *exp,
                struct ip_nat_info *info,
                enum ip_conntrack_info ctinfo,
                unsigned int hooknum,
                struct sk_buff **pskb)
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph = (void *)iph + iph->ihl * 4;
	struct ip_conntrack_tuple repl;
	int dir = CTINFO2DIR(ctinfo);
	int i;
	
	DEBUGP("ip_nat_quake3: quake3_nat_help, direction: %s hook: %s\n",
	       dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
	       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
	       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
	       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???"
	      );
	DUMP_TUPLE(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
	DUMP_TUPLE(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);
	
	/* Only mangle things once: original direction in POST_ROUTING
	   and reply direction on PRE_ROUTING. */
	if (!((hooknum == NF_IP_POST_ROUTING && dir == IP_CT_DIR_ORIGINAL)
	    || (hooknum == NF_IP_PRE_ROUTING && dir == IP_CT_DIR_REPLY))) {
		DEBUGP("ip_nat_quake3: Not touching dir %s at hook %s\n",
		       dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
		       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
		       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
		       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "????");
		return NF_ACCEPT;
	}

	if (!exp) {
		DEBUGP("no conntrack expectation to modify\n");
		return NF_ACCEPT;
	}

	if (strnicmp((const char *)udph + 12, quake3s_nat.pattern, quake3s_nat.plen) == 0) {
		for(i=31; /* 8 bytes UDP hdr, 4 bytes filler, 18 bytes "getserversResponse", 1 byte "\" */
		    i+6 < ntohs(udph->len);
		    i+=7) {
			DEBUGP("ip_nat_quake3: adding server at offset %u/%u %u.%u.%u.%u:%u\n", 
			       i, ntohs(udph->len),
			       NIPQUAD( (u_int32_t) *( (u_int32_t *)( (int)udph + i ) ) ),
			       ntohs((__u16) *( (__u16 *)( (int)udph + i + 4 ) ) ) );
			
			memset(&repl, 0, sizeof(repl));

			repl.dst.protonum = IPPROTO_UDP;
			repl.src.ip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
			repl.dst.ip = *( (u_int32_t *)( (int)udph + i ) );
			repl.dst.u.udp.port = (__u16) *( (__u16 *)( (int)udph + i + 4 )  );
			
			ip_conntrack_change_expect(exp, &repl);
		}
	}
	return NF_ACCEPT;
}

static unsigned int 
quake3_nat_expected(struct sk_buff **pskb,
                    unsigned int hooknum,
                    struct ip_conntrack *ct, 
                    struct ip_nat_info *info) 
{
	const struct ip_conntrack *master = ct->master->expectant;
	struct ip_nat_multi_range mr;
	u_int32_t newsrcip, newdstip, newip;
#if 0 
	const struct ip_conntrack_tuple *repl =
		&master->tuplehash[IP_CT_DIR_REPLY].tuple;
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph = (void *)iph + iph->ihl*4;
#endif

	DEBUGP("ip_nat_quake3: quake3_nat_expected: here we are\n");
	DUMP_TUPLE(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);

	IP_NF_ASSERT(info);
	IP_NF_ASSERT(master);
	IP_NF_ASSERT(!(info->initialized & (1 << HOOK2MANIP(hooknum))));
	
	newdstip = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip;
	newsrcip = master->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
	
	if (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) {
		newip = newsrcip;
		DEBUGP("hook: %s orig: %u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u "
		       "newsrc: %u.%u.%u.%u\n",
		       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
		       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
		       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "????",
		       NIPQUAD((*pskb)->nh.iph->saddr), ntohs(udph->source),
		       NIPQUAD((*pskb)->nh.iph->daddr), ntohs(udph->dest),
		       NIPQUAD(newip));
		
	} else {
		newip = newdstip;
		DEBUGP("hook: %s orig: %u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u "
		       "newdst: %u.%u.%u.%u\n",
		       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
		       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
		       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "????",
		       NIPQUAD((*pskb)->nh.iph->saddr), ntohs(udph->source),
		       NIPQUAD((*pskb)->nh.iph->daddr), ntohs(udph->dest),
		       NIPQUAD(newip));
	}
	
	mr.rangesize = 1;
	mr.range[0].flags = IP_NAT_RANGE_MAP_IPS;
	mr.range[0].min_ip = mr.range[0].max_ip = newip; 

	return ip_nat_setup_info(ct,&mr,hooknum);
}

static struct ip_nat_helper quake3[MAX_PORTS];
static char quake3_names[MAX_PORTS][13];  /* quake3-65535 */

static void fini(void)
{
	int i;
	
	for (i = 0 ; i < ports_c; i++) {
		DEBUGP("ip_nat_quake3: unregistering helper for port %d\n", ports[i]);
		       ip_nat_helper_unregister(&quake3[i]);
	}
}

static int __init init(void)
	{
		int i, ret = 0;
		char *tmpname;

		if (!ports[0])
			ports[0] = QUAKE3_MASTER_PORT;
		
		for (i = 0 ; (i < MAX_PORTS) && ports[i] ; i++) {
			memset(&quake3[i], 0, sizeof(struct ip_nat_helper));

			quake3[i].tuple.dst.protonum = IPPROTO_UDP;
			quake3[i].tuple.src.u.udp.port = htons(ports[i]);
			quake3[i].mask.dst.protonum = 0xFFFF;
			quake3[i].mask.src.u.udp.port = 0xFFFF;
			quake3[i].help = quake3_nat_help;
			quake3[i].flags = 0;
			quake3[i].me = THIS_MODULE;
			quake3[i].expect = quake3_nat_expected;
			
			tmpname = &quake3_names[i][0];
			if (ports[i] == QUAKE3_MASTER_PORT)
				sprintf(tmpname, "quake3");
			else
				sprintf(tmpname, "quake3-%d", i);
			quake3[i].name = tmpname;
			
			DEBUGP("ip_nat_quake3: registering helper for port %d: name %s\n",
			       ports[i], quake3[i].name);
			ret = ip_nat_helper_register(&quake3[i]);
			
			if (ret) {
				printk("ip_nat_quake3: unable to register helper for port %d\n",
				       ports[i]);
				fini();
				return ret;
			}
			ports_c++;
		}
		return ret;
	}
	
module_init(init);
module_exit(fini);
