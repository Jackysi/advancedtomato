/*
 * Copyright 2004, ASUSTek Inc.
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND ASUS GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id$
 */
/*
 * ip_nat_starcraft.c
 *
 * Basic Starcraft Application Layer Gateway
 *
 * For TCP packets, find the packets with dst port = 6112 for ip address embeded
 * in packet and replace with external IP.
 * 
 * For UDP packets, in PREROUTING
 *         find the packets with src port = 6112, redirect the ip to local one
 *         local ip = 192.168.1.x, x=(dst port-10000)
 * For UDP packets, in POSTROUTING
 *         find the packets with src port = 6112, redirect the src port to 
 *         mangle one
 *         src port = 10000 + x, x=(ip-192.168.1.1)+10000
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/netfilter_ipv4.h>
#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/brlock.h>
#include <linux/types.h>
#include <linux/ip.h>
#include <net/udp.h>
#include <asm/uaccess.h>
#include <asm/checksum.h>

#define BAT_PORT 6112
#define NOCT1(n) (u_int8_t )((n) & 0xff)

static int debug = 0;
static spinlock_t bat_lock = SPIN_LOCK_UNLOCKED;

/* 
 * NAT helper function, packets arrive here from NAT code.
 */
static unsigned int nat_help_tcp(struct ip_conntrack *ct,
			     struct ip_conntrack_expect *exp,
                             struct ip_nat_info *info,
                             enum ip_conntrack_info ctinfo,
                             unsigned int hooknum,
                             struct sk_buff **pskb)
{
	int dir = CTINFO2DIR(ctinfo);
	struct iphdr *iph = (*pskb)->nh.iph;
	struct tcphdr *tcph = (struct tcphdr *)((u_int32_t *)iph + iph->ihl);
	u_int16_t paylen = (*pskb)->len - iph->ihl * 4 - tcph->doff * 4;
	unsigned char *msg = (unsigned char *)((unsigned char *)tcph + sizeof(struct tcphdr));
	int i;
	unsigned int oip, *nip;


	spin_lock_bh(&bat_lock);

#ifdef REMOVE
	if (debug > 1) {
		printk("tcp: dir=%s hook=%d manip=%s len=%d "
		       "src=%u.%u.%u.%u:%u dst=%u.%u.%u.%u:%u "
		       "osrc=%u.%u.%u.%u odst=%u.%u.%u.%u "
		       "rsrc=%u.%u.%u.%u rdst=%u.%u.%u.%u "
		       "\n", 
		       dir == IP_CT_DIR_REPLY ? "reply" : "orig", hooknum, 
		       HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC ? "snat" :
		       "dnat", (*pskb)->len,
		       NIPQUAD(iph->saddr), ntohs(tcph->source),
		       NIPQUAD(iph->daddr), ntohs(tcph->dest),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip));
	}
#endif

	/* Replace IP address in POSTROUTING of ORIGINAL direction */
	if (hooknum != NF_IP_POST_ROUTING || dir!=IP_CT_DIR_ORIGINAL) 
	{
		spin_unlock_bh(&bat_lock);
		return NF_ACCEPT;
	}

	/* find original IP */
	oip = (unsigned int )ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
	
	// dump message
	//printk("find %x in payload: %x\n", oip, paylen);

	for(i=0;i<paylen-4;i++)
	{
		if (memcmp(msg+i, &oip, 4)==0)
		{
			ip_nat_mangle_tcp_packet(pskb, ct, ctinfo, i,
					sizeof(unsigned int), (unsigned int *)&(iph->saddr), sizeof(unsigned int));
			printk("change ip from %x to %x\n", oip, (unsigned int)iph->saddr);

#ifdef REMOVE

			iph = (*pskb)->nh.iph;
			tcph = (struct tcphdr *)((u_int32_t *)iph + iph->ihl);
			paylen = (*pskb)->len - iph->ihl * 4 - tcph->doff * 4;
			msg = (unsigned char *)((unsigned char *)tcph + sizeof(struct tcphdr));

			nip = msg+i;

			printk("new ip to : %x %x\n", *nip, paylen);

			for(i=0;i<paylen;i++)
			{
				printk("%02x ", (unsigned char )msg[i]);
				if (i!=0 && (i%16)==0) printk("\n");
			}
#endif			
			break;
		}
	}
	
	/* 
	 * Make sure the packet length is ok.  So far, we were only guaranteed
	 */
	spin_unlock_bh(&bat_lock);
	return NF_ACCEPT;
}

#ifdef REMOVE
static unsigned int 
nat_expected_udp(struct sk_buff **pskb,
		  unsigned int hooknum,
		  struct ip_conntrack *ct, 
		  struct ip_nat_info *info) 
{
	const struct ip_conntrack *master = ct->master->expectant;
	const struct ip_conntrack_tuple *orig = 
			&master->tuplehash[IP_CT_DIR_ORIGINAL].tuple;
	struct ip_nat_multi_range mr;
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph = (struct udphdr *)((u_int32_t *)iph + iph->ihl);

	printk("Expect : %x\n", hooknum);

	IP_NF_ASSERT(info);
	IP_NF_ASSERT(master);
	IP_NF_ASSERT(!(info->initialized & (1 << HOOK2MANIP(hooknum))));

	memset(&mr, 0, sizeof(mr));
	mr.rangesize = 1;
	mr.range[0].flags = IP_NAT_RANGE_MAP_IPS | IP_NAT_RANGE_PROTO_SPECIFIED;

	if (HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC) 
	{	
		/* change port to 10000 + 4th number of IP */
		printk("POST: %x %x\n", ntohs(udph->source), (unsigned int)(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip));

		if (ntohs(udph->source)==BAT_PORT)
		{	
			unsigned int newport = 10000 + (unsigned int)(((ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip)&0xFF000000)>>24);

			printk("change port to %x\n", newport);			
			mr.range[0].min.udp.port = mr.range[0].max.udp.port = newport;
			printk("orig: %u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u "
			"newsrc: %u.%u.%u.%u:%u\n",
                        NIPQUAD((*pskb)->nh.iph->saddr), ntohs(udph->source),
			NIPQUAD((*pskb)->nh.iph->daddr), ntohs(udph->dest),
			NIPQUAD((*pskb)->nh.iph->saddr), newport);
		}
	} else {
		printk("PRE: %x %x %x\n", ntohs(udph->dest), (unsigned int) iph->daddr, orig->src.ip);

		if (ntohs(udph->dest)>=10000&&ntohs(udph->dest)<=10253)
		{						
			printk("redirect ip to %x:6112\n", ntohs(udph->dest) - 10000);			

			mr.range[0].min_ip = mr.range[0].max_ip = orig->src.ip;
			mr.range[0].min.udp.port = mr.range[0].max.udp.port = 
							BAT_PORT;

			printk("orig: %u.%u.%u.%u:%u <-> %u.%u.%u.%u:%u "
				"newdst: %u.%u.%u.%u:%u\n",
                        NIPQUAD((*pskb)->nh.iph->saddr), ntohs(udph->source),
                        NIPQUAD((*pskb)->nh.iph->daddr), ntohs(udph->dest),
                        NIPQUAD(orig->src.ip), ntohs(BAT_PORT));
		}
	}
	return ip_nat_setup_info(ct,&mr,hooknum);
}
#endif

/* 
 * NAT helper function, packets arrive here from NAT code.
 */
static unsigned int nat_help_udp(struct ip_conntrack *ct,
			     struct ip_conntrack_expect *exp,
                             struct ip_nat_info *info,
                             enum ip_conntrack_info ctinfo,
                             unsigned int hooknum,
                             struct sk_buff **pskb)
{
	int dir = CTINFO2DIR(ctinfo);
	struct iphdr *iph = (*pskb)->nh.iph;
	struct udphdr *udph = (struct udphdr *)((u_int32_t *)iph + iph->ihl);
	unsigned int oip, *nip;

	spin_lock_bh(&bat_lock);

	if (debug > 1) {
		printk("udp: dir=%s hook=%d manip=%s len=%d "
		       "src=%u.%u.%u.%u:%u dst=%u.%u.%u.%u:%u "
		       "osrc=%u.%u.%u.%u odst=%u.%u.%u.%u "
		       "rsrc=%u.%u.%u.%u rdst=%u.%u.%u.%u "
		       "\n", 
		       dir == IP_CT_DIR_REPLY ? "reply" : "orig", hooknum, 
		       HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC ? "snat" :
		       "dnat", (*pskb)->len,
		       NIPQUAD(iph->saddr), ntohs(udph->source),
		       NIPQUAD(iph->daddr), ntohs(udph->dest),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip),
		       NIPQUAD(ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip));
	}

	if (hooknum == NF_IP_POST_ROUTING && dir == IP_CT_DIR_ORIGINAL)
	{
		printk("post: %x %x %x\n", iph->saddr, udph->source, ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip);

		/* replace 6112 to 10000 + i */
		if (ntohs(udph->source) == BAT_PORT)
		{
			unsigned int port;

			port = (((unsigned int)(ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip))&0xff000000) >> 24;

			printk("post 2: %x\n", port);

			udph->source = htons(10000 + port);

			printk("post 3: %x\n", udph->source);
		}
	}
	else if (hooknum == NF_IP_PRE_ROUTING)
	{
		printk("pre: %x %x\n", iph->daddr, udph->dest);
	}
	
	spin_unlock_bh(&bat_lock);
	return NF_ACCEPT;
}

// tcp, replace ip address for Game creater
static struct ip_nat_helper bat_tcp = { 
	{ NULL, NULL },
	"battcp",
	IP_NAT_HELPER_F_STANDALONE | IP_NAT_HELPER_F_ALWAYS,
	THIS_MODULE,
	{ { 0, { .tcp = { __constant_htons(BAT_PORT) } } },
	  { 0, { 0 }, IPPROTO_TCP } },
	{ { 0, { .tcp = { 0xFFFF} } },
	  { 0, { 0 }, 0xFFFF } },
	nat_help_tcp, NULL };

// udp, PREROUTING/POSTROUTING 
static struct ip_nat_helper bat_udp = { 
	{ NULL, NULL },
	"batudp",
	IP_NAT_HELPER_F_STANDALONE | IP_NAT_HELPER_F_ALWAYS,
	THIS_MODULE,
	{ { 0, { .udp = { __constant_htons(BAT_PORT) } } },
	  { 0, { 0 }, IPPROTO_UDP } },
	{ { 0, { .udp = { 0xFFFF } } },
	  { 0, { 0 }, 0xFFFF } },
	nat_help_udp, NULL };

/*****************************************************************************
 *
 * Module stuff.
 *
 *****************************************************************************/
 
static int __init init(void)
{
	int ret = 0;

	ret = ip_nat_helper_register(&bat_tcp);
	if (ret < 0)
		return ret;
#ifdef REMOVE
	ret = ip_nat_helper_register(&bat_udp);
	if (ret < 0) {
		ip_nat_helper_unregister(&bat_tcp);
		return ret;
	}
#endif

	return ret;
}

static void __exit fini(void)
{
	ip_nat_helper_unregister(&bat_tcp);
	//ip_nat_helper_unregister(&bat_udp);
	br_write_lock_bh(BR_NETPROTO_LOCK);
	br_write_unlock_bh(BR_NETPROTO_LOCK);
}

module_init(init);
module_exit(fini);

MODULE_PARM(debug, "i");
MODULE_DESCRIPTION("Battle.Net Application Layer Gateway");
MODULE_LICENSE("GPL");
