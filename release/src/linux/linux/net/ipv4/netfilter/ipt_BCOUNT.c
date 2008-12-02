/*

	BCOUNT target
	Copyright (C) 2006 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ipt_BCOUNT.h>

//	#define DEBUG_BCOUNT

static unsigned int target(struct sk_buff **pskb, unsigned int hooknum,
						   const struct net_device *in, const struct net_device *out,
						   const void *targinfo, void *userinfo)
{
	struct ip_conntrack *ct;
	enum ip_conntrack_info ctinfo;

	ct = ip_conntrack_get(*pskb, &ctinfo);
	if (ct) {
		ct->bcount += (*pskb)->len;
		if (ct->bcount >= 0x0FFFFFFF) ct->bcount = 0x0FFFFFFF;
#ifdef DEBUG_BCOUNT
		if (net_ratelimit())
			printf(KERN_DEBUG "BCOUNT %lx %lx\n", (*pskb)->len, ct->bcount);
#endif
	}
	return IPT_CONTINUE;
}

static int checkentry(const char *tablename, const struct ipt_entry *e, void *targinfo,
					  unsigned int targinfosize, unsigned int hook_mask)
{
	return (targinfosize == IPT_ALIGN(sizeof(struct ipt_BCOUNT_target)));
}

static struct ipt_target BCOUNT_target
= { { NULL, NULL }, "BCOUNT", target, checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	return ipt_register_target(&BCOUNT_target);
}

static void __exit fini(void)
{
	ipt_unregister_target(&BCOUNT_target);
}

module_init(init);
module_exit(fini);


MODULE_AUTHOR("Jonathan Zarate");
MODULE_DESCRIPTION("BCOUNT target");
MODULE_LICENSE("GPL");
