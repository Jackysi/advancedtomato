/* String matching match for iptables
 * 
 * (C) 2005 Pablo Neira Ayuso <pablo@eurodev.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_string.h>
#include "textsearch/textsearch.h"
#include "textsearch/textsearch.c"
#include "textsearch/ts_bm.c"
#include "textsearch/ts_kmp.c"

MODULE_AUTHOR("Pablo Neira Ayuso <pablo@eurodev.net>");
MODULE_DESCRIPTION("IP tables string match module");
MODULE_LICENSE("GPL");

static int match(const struct sk_buff *skb,
		 const struct net_device *in,
		 const struct net_device *out,
		 const void *matchinfo,
		 int offset,
		 const void *hdr,
		 u_int16_t datalen,
		 int *hotdrop)
{
	struct iphdr *ip = skb->nh.iph;
	struct ts_state state;
	struct ipt_string_info *conf = (struct ipt_string_info *) matchinfo;
	char *buf = (char *)ip+(ip->ihl*4);
	int len = ntohs(ip->tot_len)-(ip->ihl*4);

	memset(&state, 0, sizeof(struct ts_state));

	return (textsearch_find_continuous(conf->config, &state, buf, len) != UINT_MAX) && !conf->invert;
}

#define STRING_TEXT_PRIV(m) ((struct ipt_string_info *) m)

static int checkentry(const char *tablename,
		      const struct ipt_ip *ip,
		      void *matchinfo,
		      unsigned int matchsize,
		      unsigned int hook_mask)
{
	struct ipt_string_info *conf = matchinfo;
	struct ts_config *ts_conf;

	if (matchsize != IPT_ALIGN(sizeof(struct ipt_string_info)))
		return 0;

	/* Damn, can't handle this case properly with iptables... */
	if (conf->from_offset > conf->to_offset)
		return 0;

	ts_conf = textsearch_prepare(conf->algo, conf->pattern, conf->patlen,
				     GFP_KERNEL, TS_AUTOLOAD);
	if (IS_ERR(ts_conf))
		return 0;

	conf->config = ts_conf;

	return 1;
}

static void destroy(void *matchinfo, unsigned int matchsize)
{
	textsearch_destroy(STRING_TEXT_PRIV(matchinfo)->config);
}

static struct ipt_match string_match = {
	.name 		= "string",
	.match 		= match,
	.checkentry	= checkentry,
	.destroy 	= destroy,
	.me 		= THIS_MODULE
};

static int __init init(void)
{
	init_bm();
	init_kmp();
	return ipt_register_match(&string_match);
}

static void __exit fini(void)
{
	exit_kmp();
	exit_bm();
	ipt_unregister_match(&string_match);
}

module_init(init);
module_exit(fini);
