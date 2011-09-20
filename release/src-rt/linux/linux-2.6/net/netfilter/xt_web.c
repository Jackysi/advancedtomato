/*

	web (experimental)
	HTTP client request match
	Copyright (C) 2006 Jonathan Zarate

	Linux Kernel 2.6 Port (ipt->xt)
	Portions copyright (C) 2010 Fedor Kozhevnikov

	Licensed under GNU GPL v2 or later.

*/
#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/version.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <net/ipv6.h>
#include <net/sock.h>

#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_web.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv6/ip6_tables.h>

MODULE_AUTHOR("Jonathan Zarate");
MODULE_DESCRIPTION("Xtables: HTTP client request match (experimental)");
MODULE_LICENSE("GPL");
MODULE_ALIAS("ipt_web");
MODULE_ALIAS("ip6t_web");

//#define DEBUG

#ifdef DEBUG
#define LOG		printk
#else
#define LOG(...)	do { } while (0);
#endif

#define MAX_PAYLOAD_LEN	2048

static int find(const char *data, const char *tail, const char *text)
{
	int n, o;
	int dlen;
	const char *p, *e;

	while ((data < tail) && (*data == ' ')) ++data;
	while ((tail > data) && (*(tail - 1) == ' ')) --tail;

	dlen = tail - data;

#ifdef DEBUG
	{
		char tmp[128];
		int z;
		z = sizeof(tmp) - 1;
		if (z > dlen) z = dlen;
		memcpy(tmp, data, z);
		tmp[z] = 0;
		LOG(KERN_INFO "find in '%s'\n", tmp);
	}
#endif

	// 012345
	// text
	// ^text
	// text$
	// ^text$
	// 012345

	while (*text) {
		n = o = strlen(text);
		if (*text == '^') {
			--n;
			if (*(text + n) == '$') {
				// exact
				--n;
				if ((dlen == n) && (memcmp(data, text + 1, n) == 0)) {
					LOG(KERN_INFO "matched %s\n", text);
					return 1;
				}
			}
			else {
				// begins with
				if ((dlen >= n) && (memcmp(data, text + 1, n) == 0)) {
					LOG(KERN_INFO "matched %s\n", text);
					return 1;
				}
			}
		}
		else if (*(text + n - 1) == '$') {
			// ends with
			--n;
			if (memcmp(tail - n, text, n) == 0) {
				LOG(KERN_INFO "matched %s\n", text);
				return 1;
			}
		}
		else {
			// contains
			p = data;
			e = tail - n;
			while (p <= e) {
				if (memcmp(p, text, n) == 0) {
					LOG(KERN_INFO "matched %s\n", text);
					return 1;
				}
				++p;
			}
		}

		text += o + 1;
	}
	return 0;
}

static inline const char *findend(const char *data, const char *tail, int min)
{
	int n = tail - data;
	if (n >= min) {
		while (data < tail) {
			if (*data == '\r') return data;
			++data;
		}
	}
	return NULL;
}

static int match_payload(const struct xt_web_info *info, const char *data, int dlen)
{
	const char *tail;
	const char *p, *q;
	__u32 sig;
#ifdef DEBUG
	char tmp[64];
#endif

	// POST / HTTP/1.0$$$$
	// GET / HTTP/1.0$$$$
	// 1234567890123456789
	if (dlen < 18) return info->invert;
	
#ifdef DEBUG
	printk(KERN_INFO "dlen=%d\n", dlen);
	memcpy(tmp, data, min(63, dlen));
	tmp[min(63, dlen)] = 0;
	printk(KERN_INFO "[%s]\n", tmp);
#endif

	// "GET " or "POST"
	sig = *(__u32 *)data;
	if ((sig != __constant_htonl(0x47455420)) && (sig != __constant_htonl(0x504f5354))) {
		return info->invert;
	}
		
	tail = data + min(dlen, MAX_PAYLOAD_LEN);

	// POST / HTTP/1.0$$$$
	// GET / HTTP/1.0$$$$	-- minimum
	// 0123456789012345678
	//      9876543210
	if (((p = findend(data + 14, tail, 18)) == NULL) || (memcmp(p - 9, " HTTP/", 6) != 0))
		return info->invert;

#ifdef DEBUG
	{
		const char *qq = info->text;
		while (*qq) {
			printk(KERN_INFO "text=%s\n", qq);
			qq += strlen(qq) + 1;
		}
	}
#endif

	switch (info->mode) {
	case XT_WEB_HTTP:
		return !info->invert;
	case XT_WEB_HORE:
		// entire request line, else host line
		if (find(data + 4, p - 9, info->text)) return !info->invert;
		break;
	case XT_WEB_PATH:
		// left side of '?' or entire line
		q = data += 4;
		p -= 9;
		while ((q < p) && (*q != '?')) ++q;
		return find(data, q, info->text) ^ info->invert;
	case XT_WEB_QUERY:
		// right side of '?' or none
		q = data + 4;
		p -= 9;
		while ((q < p) && (*q != '?')) ++q;
		if (q >= p) return info->invert;
		return find(q + 1, p, info->text) ^ info->invert;
	case XT_WEB_RURI:
		// entire request line
		return find(data + 4, p - 9, info->text) ^ info->invert;
	default:
		// shutup compiler
		break;
	}

	// else, XT_WEB_HOST

	while (1) {
		data = p + 2;			// skip previous \r\n
		p = findend(data, tail, 8);	// p = current line's \r
		if (p == NULL) return 0;

#ifdef DEBUG
			memcpy(tmp, data, 32);
			tmp[32] = 0;
			printk(KERN_INFO "data=[%s]\n", tmp);
#endif

		if (memcmp(data, "Host: ", 6) == 0)
			return find(data + 6, p, info->text) ^ info->invert;
	}

	return !info->invert;
}

static int match(const struct sk_buff *skbin,
		 const struct net_device *in,
		 const struct net_device *out,
		 const struct xt_match *match,
		 const void *matchinfo,
		 int offset,
		 unsigned int protoff,
		 int *hotdrop)
{
	/* sidestep const without getting a compiler warning... */
	struct sk_buff *skb = (struct sk_buff *)skbin;

	const struct xt_web_info *info = matchinfo;
	struct tcphdr *tcph;
	const char *data;
	int len;

	if (offset != 0) return info->invert;

	if (skb_is_nonlinear(skb)) {
		if (unlikely(skb_linearize(skb))) {
			// failed to linearize packet, bailing
			return info->invert;
		}
	}

#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
	if (match->family == AF_INET6)
	{
		const struct ipv6hdr *iph = ipv6_hdr(skb);
		u8 nexthdr = iph->nexthdr;

		tcph = (void *)iph + ipv6_skip_exthdr(skb, sizeof(*iph), &nexthdr);
		data = (void *)tcph + (tcph->doff * 4);
		len = ntohs(iph->payload_len) - (data - (char *)tcph);
	}
	else
#endif
	{
		const struct iphdr *iph = ip_hdr(skb);

		tcph = (void *)iph + (iph->ihl * 4);
		data = (void *)tcph + (tcph->doff * 4);
		len = ntohs(iph->tot_len) - (data - (char *)iph);
	}

	return match_payload(info, data, len);
}

static struct xt_match web_match[] = {
	{
		.name		= "web",
		.family		= AF_INET,
		.match		= match,
		.matchsize	= sizeof(struct xt_web_info),
		.proto		= IPPROTO_TCP,
		.me		= THIS_MODULE,
	},
#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
	{
		.name		= "web",
		.family		= AF_INET6,
		.match		= match,
		.matchsize	= sizeof(struct xt_web_info),
		.proto		= IPPROTO_TCP,
		.me		= THIS_MODULE,
	},
#endif
};

static int __init init(void)
{
	LOG(KERN_INFO "xt_web <" __DATE__ " " __TIME__ "> loaded\n");
	return xt_register_matches(web_match, ARRAY_SIZE(web_match));
}

static void __exit fini(void)
{
	xt_unregister_matches(web_match, ARRAY_SIZE(web_match));
}

module_init(init);
module_exit(fini);
