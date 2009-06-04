/* CuSeeMe extension for UDP NAT alteration.
 * (C) 2002 by Filip Sneppe <filip.sneppe@cronos.be>
 * based on ip_masq_cuseeme.c in 2.2 kernels
 *
 * ip_nat_cuseeme.c v0.0.7 2003-02-18
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 *      Module load syntax:
 *      insmod ip_nat_cuseeme.o ports=port1,port2,...port<MAX_PORTS>
 *
 *      Please give the ports of the CuSeeMe traffic you want to track.
 *      If you don't specify ports, the default will be UDP port 7648.
 *
 *      CuSeeMe Protocol Documentation:
 *      http://cu-seeme.net/squeek/tech/contents.html
 *
 */

#include <linux/module.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/udp.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_cuseeme.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>

MODULE_AUTHOR("Filip Sneppe <filip.sneppe@cronos.be>");
MODULE_DESCRIPTION("Netfilter NAT helper for CuSeeMe");
MODULE_LICENSE("GPL");

#define MAX_PORTS 8

static int ports[MAX_PORTS];
static int ports_c = 0;
#ifdef MODULE_PARM
MODULE_PARM(ports,"1-" __MODULE_STRING(MAX_PORTS) "i");
MODULE_PARM_DESC(ports, "port numbers of CuSeeMe reflectors");
#endif

#if 0
#define DEBUGP printk
#else
#define DEBUGP(format, args...)
#endif

/* process packet from client->reflector, possibly manipulate client IP in payload */
void cuseeme_mangle_outgoing(struct ip_conntrack *ct,
                             struct ip_nat_info *info,
                             enum ip_conntrack_info ctinfo,
                             struct sk_buff **pskb,
                             char *data,
                             unsigned int datalen)
{
	char new_port_ip[6];
	struct cu_header *cu_head=(struct cu_header *)data;
	
	DEBUGP("ip_nat_cuseeme: outgoing packet, ID %u, dest_family %u\n", 
	       ntohs(cu_head->data_type), ntohs(cu_head->dest_family));
		
	/* At least check that the data at offset 10 is the client's port and IP address */
	if ((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip == cu_head->addr) &&
	    (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port == cu_head->port)) {
		DEBUGP("ip_nat_cuseeme: rewrite outgoing client %u.%u.%u.%u:%u->%u.%u.%u.%u:%u at offset 10\n", 
		       NIPQUAD(cu_head->addr),
		       ntohs(cu_head->port),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip),
		       ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port));
		*((u_int16_t *)new_port_ip) = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port;
		*((u_int32_t *)(new_port_ip+2)) = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
		/* at offset 10, replace 6 bytes containing port + IP address */
		ip_nat_mangle_udp_packet(pskb, ct, ctinfo,
		                         10, 6, (char *)(new_port_ip), 6);
	} else 
		DEBUGP("ip_nat_cuseeme: expected outgoing client %u.%u.%u.%u:%u, but got %u.%u.%u.%u:%u\n",
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),
		       ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port),
		       NIPQUAD(cu_head->addr),
		       ntohs(cu_head->port));
}

/* process packet from reflector->client, possibly manipulate client IP & reflector IP in payload */
void cuseeme_mangle_incoming(struct ip_conntrack *ct,
                             struct ip_nat_info *info,
                             enum ip_conntrack_info ctinfo,
                             struct sk_buff **pskb,
                             char *data,
                             unsigned int datalen)
{
	char new_port_ip[6];
	struct cu_header *cu_head = (struct cu_header *)data;
	struct oc_header *oc_head = (struct oc_header *)data; 
	struct client_info *ci; 
	int i, off;

	
	DEBUGP("ip_nat_cuseeme: incoming packet, ID %u, dest_family %u\n", 
	       ntohs(cu_head->data_type), ntohs(cu_head->dest_family));
		
	/* Check if we're really dealing with the client's port + IP address before rewriting */
	if((ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip == cu_head->dest_addr) &&
	   (ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port == cu_head->dest_port)) {
		DEBUGP("ip_nat_cuseeme: rewrite incoming client %u.%u.%u.%u:%u->%u.%u.%u.%u:%u at offset 2\n",
		       NIPQUAD(cu_head->dest_addr),
		       ntohs(cu_head->dest_port),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),
		       ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port));
		*((u_int16_t *)new_port_ip) = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port;
		*((u_int32_t *)(new_port_ip+2)) = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
		/* at offset 2, replace 6 bytes containing port + IP address */
		ip_nat_mangle_udp_packet(pskb, ct, ctinfo,
		                         2, 6, (char *)(new_port_ip), 6);
	} else 
		DEBUGP("ip_nat_cuseeme: expected incoming client %u.%u.%u.%u:%u, but got %u.%u.%u.%u:%u\n",
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip),
		       ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.udp.port),
		       NIPQUAD(cu_head->dest_addr),
		       ntohs(cu_head->dest_port));
	
	/* Check if we're really dealing with the server's port + IP address before rewriting. 
	   In some cases, the IP address == 0.0.0.0 so we don't rewrite anything */
	if((ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip == cu_head->addr) &&
	   (ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.udp.port == cu_head->port)) {
		DEBUGP("in_nat_cuseeme: rewrite incoming server %u.%u.%u.%u:%u->%u.%u.%u.%u:%u at offset 10\n",
		       NIPQUAD(cu_head->addr),
		       ntohs(cu_head->port),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip),
		       ntohs(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port));
		*((u_int16_t *)new_port_ip) = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.udp.port;
		*((u_int32_t *)(new_port_ip+2)) = ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip;
		/* at offset 10, replace 6 bytes containing port + IP address */
		ip_nat_mangle_udp_packet(pskb, ct, ctinfo,
		                         10, 6, (char *)(new_port_ip), 6);
	} else 
		/* Sometimes we find 0.0.0.0, sometimes an IP address - the docs say this field
		   is not that important so we're not logging this unless we're debugging */
		DEBUGP("ip_nat_cuseeme: no biggie, expected incoming server %u.%u.%u.%u:%u, but got %u.%u.%u.%u:%u\n",
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip),
		       ntohs(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.udp.port),
		       NIPQUAD(cu_head->addr),
		       ntohs(cu_head->port));
	
	/* Spin through client_info structs until we find our own */
	if((ntohs(cu_head->data_type) == 101) && (datalen >= sizeof(struct oc_header))) {
		DEBUGP("ip_nat_cuseeme: looping through %u client_info structs\n", oc_head->client_count);
		for(i=0, off=sizeof(struct oc_header);
		    (i < oc_head->client_count && 
		    off+sizeof(struct client_info) <= datalen); 
		    i++) {
			ci=(struct client_info *)(data+off);
			DEBUGP("ip_nat_cuseeme: comparing %u.%u.%u.%u with %u.%u.%u.%u at offset %u\n", 
			       NIPQUAD(ci->address), NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip), 
			       (unsigned int)((void *)&(ci->address) - (void *)cu_head));
			if(ci->address == ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip) {
				/* mangle this IP address */
				DEBUGP("ip_nat_cuseeme: changing %u.%u.%u.%u->%u.%u.%u.%u at offset %u\n",
				       NIPQUAD(ci->address), 
				       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),
				       (unsigned int)((void *)&(ci->address) - (void *)cu_head));
				ip_nat_mangle_udp_packet(pskb, ct, ctinfo,
				                         (unsigned int)((void *)&(ci->address) - (void *)cu_head), 4, 
				                         (char *)(&(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip)), 4);
				break;
			} else 
				off+=sizeof(struct client_info);
		}
	} else
		DEBUGP("ip_nat_cuseeme: data_type %u, datalen %u < sizeof(struct oc_header) %u\n", 
		       ntohs(cu_head->data_type), datalen, sizeof(struct oc_header));
}

static unsigned int 
cuseeme_nat_help(struct ip_conntrack *ct,
                 struct ip_conntrack_expect *exp,
                 struct ip_nat_info *info,
                 enum ip_conntrack_info ctinfo,
                 unsigned int hooknum,
                 struct sk_buff **pskb)
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph = (void *)iph + iph->ihl * 4;
	int dir = CTINFO2DIR(ctinfo);
	unsigned int datalen = (*pskb)->len - iph->ihl * 4 - sizeof(struct udphdr);
	char *data = (char *) &udph[1];
	
	DEBUGP("ip_nat_cuseeme: cuseeme_nat_help, direction: %s hook: %s\n",
	       dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
	       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
	       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
	       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "???"
	      );
	
	/* Only mangle things once: original direction in POST_ROUTING
	   and reply direction on PRE_ROUTING. */
	if (!((hooknum == NF_IP_POST_ROUTING && dir == IP_CT_DIR_ORIGINAL)
	    || (hooknum == NF_IP_PRE_ROUTING && dir == IP_CT_DIR_REPLY))) {
		DEBUGP("ip_nat_cuseeme: not touching dir %s at hook %s\n",
		       dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
		       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
		       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
		       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT" : "????");
		return NF_ACCEPT;
	}
	
	if(datalen < sizeof(struct cu_header)) {
		/* packet too small */
		if (net_ratelimit())
			printk("ip_nat_cuseeme: payload too small (%u, should be >= %u)\n", 
			       datalen, sizeof(struct cu_header));
		return NF_ACCEPT;
	}


	/* In the debugging output, "outgoing" is from client to server, and
	   "incoming" is from server to client */
	if(HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) 
		cuseeme_mangle_outgoing(ct, info, ctinfo, pskb, data, datalen);
	else 
		cuseeme_mangle_incoming(ct, info, ctinfo, pskb, data, datalen);

	return NF_ACCEPT;
}

static struct ip_nat_helper cuseeme[MAX_PORTS];
static char cuseeme_names[MAX_PORTS][14];  /* cuseeme-65535 */

static void fini(void)
{
	int i;
	
	for (i = 0 ; i < ports_c; i++) {
		DEBUGP("ip_nat_cuseeme: unregistering helper for port %d\n", ports[i]);
		       ip_nat_helper_unregister(&cuseeme[i]);
	}
}

static int __init init(void)
{
	int i, ret = 0;
	char *tmpname;

	if (!ports[0])
		ports[0] = CUSEEME_PORT;
		
	for (i = 0 ; (i < MAX_PORTS) && ports[i] ; i++) {
		memset(&cuseeme[i], 0, sizeof(struct ip_nat_helper));

		cuseeme[i].tuple.dst.protonum = IPPROTO_UDP;
		cuseeme[i].tuple.dst.u.udp.port = htons(ports[i]);
		cuseeme[i].mask.dst.protonum = 0xFFFF;
		cuseeme[i].mask.dst.u.udp.port = 0xFFFF;
		cuseeme[i].help = cuseeme_nat_help;
		cuseeme[i].flags = IP_NAT_HELPER_F_STANDALONE + 
		                   IP_NAT_HELPER_F_ALWAYS; /* dunno if IP_NAT_HELPER_F_ALWAYS
		                                              is stricly needed... */
		cuseeme[i].me = THIS_MODULE;
		cuseeme[i].expect = NULL; /* cuseeme_nat_expected; */
			
		tmpname = &cuseeme_names[i][0];
		if (ports[i] == CUSEEME_PORT)
			sprintf(tmpname, "cuseeme");
		else
			sprintf(tmpname, "cuseeme-%d", ports[i]);
		cuseeme[i].name = tmpname;
			
		DEBUGP("ip_nat_cuseeme: registering helper for port %d: name %s\n",
		       ports[i], cuseeme[i].name);
		ret = ip_nat_helper_register(&cuseeme[i]);
			
		if (ret) {
			printk("ip_nat_cuseeme: unable to register helper for port %d\n",
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
