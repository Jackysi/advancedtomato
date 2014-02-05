/* Kernel module to match one of a list of TCP/UDP ports: ports are in
   the same place so we can treat them as equal. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/skbuff.h>

#include <linux/netfilter_ipv4/ipt_multiport.h>
#include <linux/netfilter_ipv4/ip_tables.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netfilter Core Team <coreteam@netfilter.org>");
MODULE_DESCRIPTION("iptables multiple port match module");

#if 0
#define duprintf(format, args...) printk(format , ## args)
#else
#define duprintf(format, args...)
#endif

/* from linux 2.6 skbuff.h */
static inline void *skb_header_pointer(const struct sk_buff *skb, int offset,
				       int len, void *buffer)
{
	int hlen = skb_headlen(skb);

	if (hlen - offset >= len)
		return skb->data + offset;

	if (skb_copy_bits(skb, offset, buffer, len) < 0)
		return NULL;

	return buffer;
}


/* Returns 1 if the port is matched by the test, 0 otherwise. */
static inline int
ports_match_v1(const struct ipt_multiport_v1 *minfo,
	       u_int16_t src, u_int16_t dst)
{
	unsigned int i;
	u_int16_t s, e;

	for (i=0; i < minfo->count; i++) {
		s = minfo->ports[i];

		if (minfo->pflags[i]) {
			/* range port matching */
			e = minfo->ports[++i];
			duprintf("src or dst matches with %d-%d?\n", s, e);

			if (minfo->flags == IPT_MULTIPORT_SOURCE
			    && src >= s && src <= e)
				return 1 ^ minfo->invert;
			if (minfo->flags == IPT_MULTIPORT_DESTINATION
			    && dst >= s && dst <= e)
				return 1 ^ minfo->invert;
			if (minfo->flags == IPT_MULTIPORT_EITHER
			    && ((dst >= s && dst <= e)
				|| (src >= s && src <= e)))
				return 1 ^ minfo->invert;
		} else {
			/* exact port matching */
			duprintf("src or dst matches with %d?\n", s);

			if (minfo->flags == IPT_MULTIPORT_SOURCE
			    && src == s)
				return 1 ^ minfo->invert;
			if (minfo->flags == IPT_MULTIPORT_DESTINATION
			    && dst == s)
				return 1 ^ minfo->invert;
			if (minfo->flags == IPT_MULTIPORT_EITHER
			    && (src == s || dst == s))
				return 1 ^ minfo->invert;
		}
	}
 
 	return minfo->invert;
}

static int
match_v1(const struct sk_buff *skb,
	 const struct net_device *in,
	 const struct net_device *out,
	 const void *matchinfo,
	 int offset,
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	 const void *hdr,
	 u_int16_t datalen,
#endif
	 int *hotdrop)
{
	u16 _ports[2], *pptr;
	const struct ipt_multiport_v1 *multiinfo = matchinfo;

	if (offset)
		return 0;

	pptr = skb_header_pointer(skb, skb->nh.iph->ihl * 4,
				  sizeof(_ports), _ports);
	if (pptr == NULL) {
		/* We've been asked to examine this packet, and we
		 * can't.  Hence, no choice but to drop.
		 */
		duprintf("ipt_multiport:"
			 " Dropping evil offset=0 tinygram.\n");
		*hotdrop = 1;
		return 0;
	}

	return ports_match_v1(multiinfo, ntohs(pptr[0]), ntohs(pptr[1]));
}

static int
checkentry_v1(const char *tablename,
	      const struct ipt_ip *ip,
	      void *matchinfo,
	      unsigned int matchsize,
	      unsigned int hook_mask)
{
	const struct ipt_multiport_v1 *multiinfo = matchinfo;

	/* Must specify supported protocol, no unknown flags or bad count */
	return (ip->proto == IPPROTO_TCP || ip->proto == IPPROTO_UDP
		|| ip->proto == IPPROTO_SCTP)
		&& !(ip->invflags & IPT_INV_PROTO)
		&& matchsize == IPT_ALIGN(sizeof(struct ipt_multiport_v1))
		&& (multiinfo->flags == IPT_MULTIPORT_SOURCE
		    || multiinfo->flags == IPT_MULTIPORT_DESTINATION
		    || multiinfo->flags == IPT_MULTIPORT_EITHER)
		&& multiinfo->count <= IPT_MULTI_PORTS;
}

static struct ipt_match multiport_match_v1 = {
	.name		= "multiport",
	.match		= &match_v1,
	.checkentry	= &checkentry_v1,
	.me		= THIS_MODULE,
};

static int __init init(void)
{
	return ipt_register_match(&multiport_match_v1);
}

static void __exit fini(void)
{
	ipt_unregister_match(&multiport_match_v1);
}

module_init(init);
module_exit(fini);
