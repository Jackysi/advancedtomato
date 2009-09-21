/* Kernel module to match one of a list of TCP/UDP ports: ports are in
   the same place so we can treat them as equal. */

/* (C) 1999-2001 Paul `Rusty' Russell
 * (C) 2002-2004 Netfilter Core Team <coreteam@netfilter.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/udp.h>
#include <linux/skbuff.h>
#include <linux/in.h>

#include <linux/netfilter_ipv6/ip6t_multiport.h>
#include <linux/netfilter_ipv6/ip6_tables.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Netfilter Core Team <coreteam@netfilter.org>");
MODULE_DESCRIPTION("iptables multiple port match module");

#if 0
#define duprintf(format, args...) printk(format , ## args)
#else
#define duprintf(format, args...)
#endif

/* Returns 1 if the port is matched by the test, 0 otherwise. */
static inline int
ports_match_v1(const struct ip6t_multiport_v1 *minfo,
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

			if (minfo->flags == IP6T_MULTIPORT_SOURCE
			    && src >= s && src <= e)
				return 1 ^ minfo->invert;
			if (minfo->flags == IP6T_MULTIPORT_DESTINATION
			    && dst >= s && dst <= e)
				return 1 ^ minfo->invert;
			if (minfo->flags == IP6T_MULTIPORT_EITHER
			    && ((dst >= s && dst <= e)
				|| (src >= s && src <= e)))
				return 1 ^ minfo->invert;
		} else {
			/* exact port matching */
			duprintf("src or dst matches with %d?\n", s);

			if (minfo->flags == IP6T_MULTIPORT_SOURCE
			    && src == s)
				return 1 ^ minfo->invert;
			if (minfo->flags == IP6T_MULTIPORT_DESTINATION
			    && dst == s)
				return 1 ^ minfo->invert;
			if (minfo->flags == IP6T_MULTIPORT_EITHER
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
	const struct udphdr *udp = hdr;
	const struct ip6t_multiport_v1 *multiinfo = matchinfo;

	/* Must be big enough to read ports. */
	if (offset == 0 && datalen < sizeof(struct udphdr)) {
		/* We've been asked to examine this packet, and we
		   can't.  Hence, no choice but to drop. */
			duprintf("ip6t_multiport:"
				 " Dropping evil offset=0 tinygram.\n");
			*hotdrop = 1;
			return 0;
	}

	/* Must not be a fragment. */
	return !offset
		&& ports_match_v1(multiinfo,
			ntohs(udp->source), ntohs(udp->dest));
}

/* Called when user tries to insert an entry of this type. */
static int
checkentry_v1(const char *tablename,
	   const struct ip6t_ip6 *ip,
	   void *matchinfo,
	   unsigned int matchsize,
	   unsigned int hook_mask)
{
	const struct ip6t_multiport_v1 *multiinfo = matchinfo;

	/* Must specify supported protocol, no unknown flags or bad count */
	return (ip->proto == IPPROTO_TCP || ip->proto == IPPROTO_UDP
		|| ip->proto == IPPROTO_SCTP)
		&& !(ip->invflags & IP6T_INV_PROTO)
		&& matchsize == IP6T_ALIGN(sizeof(struct ip6t_multiport_v1))
		&& (multiinfo->flags == IP6T_MULTIPORT_SOURCE
		    || multiinfo->flags == IP6T_MULTIPORT_DESTINATION
		    || multiinfo->flags == IP6T_MULTIPORT_EITHER)
		&& multiinfo->count <= IP6T_MULTI_PORTS;
}

static struct ip6t_match multiport_match_v1 = {
	.name		= "multiport",
	.match		= &match_v1,
	.checkentry	= &checkentry_v1,
	.me		= THIS_MODULE,
};

static int __init init(void)
{
	return ip6t_register_match(&multiport_match_v1);
}

static void __exit fini(void)
{
	ip6t_unregister_match(&multiport_match_v1);
}

module_init(init);
module_exit(fini);
