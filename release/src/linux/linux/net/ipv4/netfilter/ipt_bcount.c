/*

	bcount match (experimental)
	Copyright (C) 2006 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/
#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/sock.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ipt_bcount.h>

//	#define LOG			printk
#define LOG(...)	do { } while (0);


static int match(const struct sk_buff *skb,	const struct net_device *in, const struct net_device *out,
				 const void *matchinfo, int offset, const void *hdr, u_int16_t datalen, int *hotdrop)
{
	const struct ipt_bcount_match *info = matchinfo;
	struct ip_conntrack *ct;
	enum ip_conntrack_info ctinfo;

	ct = ip_conntrack_get((struct sk_buff *)skb, &ctinfo);
	if (!ct) return !info->invert;
	return ((ct->bcount >= info->min) && (ct->bcount <= info->max)) ^ info->invert;
}

static int checkentry(const char *tablename, const struct ipt_ip *ip, void *matchinfo,
					  unsigned int matchsize, unsigned int hook_mask)
{
	return (matchsize == IPT_ALIGN(sizeof(struct ipt_bcount_match)));
}


static struct ipt_match bcount_match
= { { NULL, NULL }, "bcount", &match, &checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	LOG(KERN_INFO "ipt_bcount <" __DATE__ " " __TIME__ "> loaded\n");
	return ipt_register_match(&bcount_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&bcount_match);
}

module_init(init);
module_exit(fini);


MODULE_AUTHOR("Jonathan Zarate");
MODULE_DESCRIPTION("bcount match");
MODULE_LICENSE("GPL");
