/*

	MACSAVE target
	Copyright (C) 2006 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ipt_MACSAVE.h>

static unsigned int target(struct sk_buff **pskb, unsigned int hooknum,
						   const struct net_device *in, const struct net_device *out,
						   const void *targinfo, void *userinfo)
{
//	const struct ipt_MACSAVE_target_info *info = targinfo;
	struct sk_buff *skb = *pskb;
	struct ip_conntrack *ct;
	enum ip_conntrack_info ctinfo;
	
	if ((skb->mac.raw >= skb->head) && ((skb->mac.raw + ETH_HLEN) <= skb->data)) {
		ct = ip_conntrack_get(skb, &ctinfo);
		if (ct) {
			memcpy(ct->macsave, skb->mac.ethernet->h_source, sizeof(ct->macsave));
		}
	}
	return IPT_CONTINUE;
}

static int checkentry(const char *tablename, const struct ipt_entry *e, void *targinfo,
					  unsigned int targinfosize, unsigned int hook_mask)
{
	if (targinfosize != IPT_ALIGN(sizeof(struct ipt_MACSAVE_target_info))) {
		printk(KERN_ERR "MACSAVE: Invalid data size\n");
		return 0;
	}
	
	if (hook_mask & ~((1 << NF_IP_PRE_ROUTING) | (1 << NF_IP_FORWARD) | (1 << NF_IP_LOCAL_IN))) {
		printk(KERN_ERR "MACSAVE: Valid only in PREROUTING, FORWARD and INPUT\n");
		return 0;
	}
	return 1;
}

static struct ipt_target macsave_target
= { { NULL, NULL }, "MACSAVE", target, checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	return ipt_register_target(&macsave_target);
}

static void __exit fini(void)
{
	ipt_unregister_target(&macsave_target);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
