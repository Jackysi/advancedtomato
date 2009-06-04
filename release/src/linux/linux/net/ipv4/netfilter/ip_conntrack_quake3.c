/* Quake3 extension for IP connection tracking
 * (C) 2002 by Filip Sneppe <filip.sneppe@cronos.be>
 * based on ip_conntrack_ftp.c and ip_conntrack_tftp.c
 *
 * ip_conntrack_quake3.c v0.04 2002-08-31
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *      Module load syntax:
 *      insmod ip_conntrack_quake3.o ports=port1,port2,...port<MAX_PORTS>
 *
 *      please give the ports of all Quake3 master servers You wish to 
 *      connect to. If you don't specify ports, the default will be UDP 
 *      port 27950.
 *
 *      Thanks to the Ethereal folks for their analysis of the Quake3 protocol.
 */

#include <linux/module.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_quake3.h>

struct module *ip_conntrack_quake3 = THIS_MODULE;

MODULE_AUTHOR("Filip Sneppe <filip.sneppe@cronos.be>");
MODULE_DESCRIPTION("Netfilter connection tracking module for Quake III Arena");
MODULE_LICENSE("GPL");

#define MAX_PORTS 8
static int ports[MAX_PORTS];
static int ports_c = 0;
#ifdef MODULE_PARM
MODULE_PARM(ports, "1-" __MODULE_STRING(MAX_PORTS) "i");
MODULE_PARM_DESC(ports, "port numbers of Quake III master servers");
#endif

/* Quake3 master server reply will add > 100 expectations per reply packet; when
   doing lots of printk's, klogd may not be able to read /proc/kmsg fast enough */
#if 0 
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

struct quake3_search quake3s_conntrack = { "****", "getserversResponse", sizeof("getserversResponse") - 1 };

static int quake3_help(const struct iphdr *iph, size_t len,
	struct ip_conntrack *ct,
	enum ip_conntrack_info ctinfo)
{
	struct udphdr *udph = (void *)iph + iph->ihl * 4;
	int dir = CTINFO2DIR(ctinfo);
	struct ip_conntrack_expect exp;
	int i;
	
        /* Until there's been traffic both ways, don't look in packets. note: it's UDP ! */
	if (ctinfo != IP_CT_ESTABLISHED
	    && ctinfo != IP_CT_IS_REPLY) {
	        DEBUGP("ip_conntrack_quake3: not ok ! Conntrackinfo = %u\n", ctinfo);
	        return NF_ACCEPT;
	} else { DEBUGP("ip_conntrack_quake3: it's ok ! Conntrackinfo = %u\n", ctinfo); }
	
	if (strnicmp((const char *)udph + 12, quake3s_conntrack.pattern, quake3s_conntrack.plen) == 0) {
		for(i=31;    /* 8 bytes UDP hdr, 4 bytes filler, 18 bytes "getserversResponse", 1 byte "\" */
		    i+6 < ntohs(udph->len);
		    i+=7) {
			DEBUGP("ip_conntrack_quake3: adding server at offset %u/%u %u.%u.%u.%u:%u\n",
			       i, ntohs(udph->len),
			       NIPQUAD( (u_int32_t) *( (u_int32_t *)( (int)udph + i ) ) ), 
			       ntohs((__u16) *( (__u16 *)( (int)udph + i + 4 ) ) ) );

			memset(&exp, 0, sizeof(exp));

			exp.tuple = ((struct ip_conntrack_tuple)
			             { { ct->tuplehash[!dir].tuple.src.ip, { 0 } },
			               { (u_int32_t) *((u_int32_t *)((int)udph + i)), 
			               { .udp = { (__u16) *((__u16 *)((int)udph+i+4)) } }, 
			                 IPPROTO_UDP } }
			            );
			exp.mask  = ((struct ip_conntrack_tuple)
			             { { 0xFFFFFFFF, { 0 } },
		                       { 0xFFFFFFFF, { .udp = { 0xFFFF } }, 0xFFFF }});
			exp.expectfn = NULL;

			ip_conntrack_expect_related(ct, &exp);
		}

	}
	
	return(NF_ACCEPT);
}

static struct ip_conntrack_helper quake3[MAX_PORTS];
static char quake3_names[MAX_PORTS][13];  /* quake3-65535 */

static void fini(void)
{
	int i;

	for(i = 0 ; (i < ports_c); i++) {
		DEBUGP("ip_conntrack_quake3: unregistering helper for port %d\n",
					ports[i]);
		ip_conntrack_helper_unregister(&quake3[i]);
	} 
}

static int __init init(void)
{
	int i, ret;
	char *tmpname;

	if(!ports[0])
		ports[0]=QUAKE3_MASTER_PORT;

	for(i = 0 ; (i < MAX_PORTS) && ports[i] ; i++) {
		/* Create helper structure */
		memset(&quake3[i], 0, sizeof(struct ip_conntrack_helper));

		quake3[i].tuple.dst.protonum = IPPROTO_UDP;
		quake3[i].tuple.src.u.udp.port = htons(ports[i]);
		quake3[i].mask.dst.protonum = 0xFFFF;
		quake3[i].mask.src.u.udp.port = 0xFFFF;
		quake3[i].help = quake3_help;
		quake3[i].me = THIS_MODULE;

		tmpname = &quake3_names[i][0];
		if (ports[i] == QUAKE3_MASTER_PORT)
			sprintf(tmpname, "quake3");
		else
			sprintf(tmpname, "quake3-%d", i);
		quake3[i].name = tmpname;
		
		DEBUGP("ip_conntrack_quake3: registering helper for port %d\n",
		       ports[i]);

		ret=ip_conntrack_helper_register(&quake3[i]);
		if(ret) {
			fini();
			return(ret);
		}
		ports_c++;
	}

	return(0);
}

module_init(init);
module_exit(fini);
