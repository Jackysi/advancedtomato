/*

	Experimental Netfilter Crap
	Copyright (C) 2006 Jonathan Zarate

*/
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/file.h>
#include <net/sock.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_exp.h>
#include "../../bridge/br_private.h"


static int match(const struct sk_buff *skb,	const struct net_device *in, const struct net_device *out,
				 const void *matchinfo, int offset, const void *hdr, u_int16_t datalen, int *hotdrop)
{
//	const struct ipt_exp_info *info = matchinfo;

	if ((skb->mac.raw >= skb->head) && ((skb->mac.raw + ETH_HLEN) <= skb->data)) {
		printk(KERN_INFO "exp src=%02X:%02X:%02X:%02X:%02X:%02X dst=%02X:%02X:%02X:%02X:%02X:%02X\n", 
			skb->mac.ethernet->h_source[0], skb->mac.ethernet->h_source[1], skb->mac.ethernet->h_source[2], 
			skb->mac.ethernet->h_source[3], skb->mac.ethernet->h_source[4], skb->mac.ethernet->h_source[5], 
			skb->mac.ethernet->h_dest[0], skb->mac.ethernet->h_dest[1], skb->mac.ethernet->h_dest[2], 
			skb->mac.ethernet->h_dest[3], skb->mac.ethernet->h_dest[4], skb->mac.ethernet->h_dest[5]);
		return 1;
	}
	printk(KERN_INFO "exp mac=%p head=%p in=%p\n", skb->mac.raw, skb->head, in);
	return 0;
}

static int checkentry(const char *tablename, const struct ipt_ip *ip, void *matchinfo,
					  unsigned int matchsize, unsigned int hook_mask)
{
	return (matchsize == IPT_ALIGN(sizeof(struct ipt_exp_info)));
}

static struct ipt_match exp_match
	= { { NULL, NULL }, "exp", &match, &checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	printk(KERN_INFO "exp init " __DATE__ " " __TIME__ "\n");
	return ipt_register_match(&exp_match);
}

static void __exit fini(void)
{
	printk(KERN_INFO "exp fini\n");
	ipt_unregister_match(&exp_match);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
