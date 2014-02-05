/*

	macsave match
	Copyright (C) 2006 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/

#include <linux/module.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ipt_macsave.h>

#define DEBUG	1

#ifdef DEBUG
#define DLOG			printk
#else
#define DLOG(...)	do { } while (0);
#endif


static int match(const struct sk_buff *skb,	const struct net_device *in, const struct net_device *out,
				 const void *matchinfo, int offset, const void *hdr, u_int16_t datalen, int *hotdrop)
{
	const struct ipt_macsave_match_info *info = matchinfo;
	struct ip_conntrack *ct;
	enum ip_conntrack_info ctinfo;

	ct = ip_conntrack_get((struct sk_buff *)skb, &ctinfo);	// note about cast: ip_conntrack_get() will not modify skb
	if (ct) return (memcmp(ct->macsave, info->mac, sizeof(ct->macsave)) == 0) ^ info->invert;
	return info->invert;
}

static int checkentry(const char *tablename, const struct ipt_ip *ip, void *matchinfo,
					  unsigned int matchsize, unsigned int hook_mask)
{
	return (matchsize == IPT_ALIGN(sizeof(struct ipt_macsave_match_info)));
}


static struct ipt_match macsave_match
= { { NULL, NULL }, "macsave", &match, &checkentry, NULL, THIS_MODULE };

static int __init init(void)
{
	DLOG(KERN_INFO "macsave match init " __DATE__ " " __TIME__ "\n");
	return ipt_register_match(&macsave_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&macsave_match);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
