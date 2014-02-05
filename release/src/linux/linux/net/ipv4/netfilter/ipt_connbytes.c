/* Kernel module to match connection tracking byte counter.
 * GPL (C) 2002 Martin Devera (devik@cdi.cz).
 *
 * 2004-07-20 Harald Welte <laforge at netfilter.org>
 *      - reimplemented to use per-connection accounting counters
 *      - add functionality to match number of packets
 *      - add functionality to match average packet size
 *      - add support to match directions seperately
 *
 * 2004-10-24 Piotr Chytla <pch at fouk.org>
 * 	- Connbytes with per-connection accouting backported to 2.4
 * 	
 */

#include <linux/module.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netfilter_ipv4/ip_conntrack.h>
#include <linux/netfilter_ipv4/ip_tables.h>
#include <linux/netfilter_ipv4/ipt_connbytes.h>

#include <asm/div64.h>

static u_int64_t mydiv(u_int64_t arg1,u_int32_t arg2)
{
	do_div(arg1,arg2);
	return arg1;
}

static int
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
      const void *matchinfo,
      int offset,
      const void *hdr,
      u_int16_t datalen,
      int *hotdrop)
{
	static u_int64_t what;
	const struct ipt_connbytes_info *sinfo = matchinfo;
	enum ip_conntrack_info ctinfo;
	struct ip_conntrack *ct;

	if (!(ct = ip_conntrack_get((struct sk_buff *)skb, &ctinfo)))
		return 0; /* no match */
        switch (sinfo->what) {
        case IPT_CONNBYTES_PKTS:
                switch (sinfo->direction) {
                case IPT_CONNBYTES_DIR_ORIGINAL:
                        what = ct->counters[IP_CT_DIR_ORIGINAL].packets;
                        break;
                case IPT_CONNBYTES_DIR_REPLY:
                        what = ct->counters[IP_CT_DIR_REPLY].packets;
			break;
                case IPT_CONNBYTES_DIR_BOTH:
                        what = ct->counters[IP_CT_DIR_ORIGINAL].packets;
                        what += ct->counters[IP_CT_DIR_REPLY].packets;
                        break;
                }
		break;
        case IPT_CONNBYTES_BYTES:
                switch (sinfo->direction) {
                case IPT_CONNBYTES_DIR_ORIGINAL:
                        what = ct->counters[IP_CT_DIR_ORIGINAL].bytes;
                        break;
                case IPT_CONNBYTES_DIR_REPLY:
                        what = ct->counters[IP_CT_DIR_REPLY].bytes;
                        break;
                case IPT_CONNBYTES_DIR_BOTH:
                        what = ct->counters[IP_CT_DIR_ORIGINAL].bytes;
                        what += ct->counters[IP_CT_DIR_REPLY].bytes;
                        break;
                }
                break;
        case IPT_CONNBYTES_AVGPKT:
                switch (sinfo->direction) {
                case IPT_CONNBYTES_DIR_ORIGINAL:
                        {
                                u_int32_t pkts32;

                                if (ct->counters[IP_CT_DIR_ORIGINAL].packets > 0xfffffffff)
                                        pkts32 = 0xffffffff;
                                else
                                        pkts32 = ct->counters[IP_CT_DIR_ORIGINAL].packets;
				what = mydiv(ct->counters[IP_CT_DIR_ORIGINAL].bytes,pkts32);
                        }
                        break;
                case IPT_CONNBYTES_DIR_REPLY:
                        {
                                u_int32_t pkts32;

                                if (ct->counters[IP_CT_DIR_REPLY].packets > 0xffffffff)
                                        pkts32 = 0xffffffff;
                                else
                                        pkts32 = ct->counters[IP_CT_DIR_REPLY].packets;
				what = mydiv(ct->counters[IP_CT_DIR_REPLY].bytes,pkts32);
                        }
                        break;
                case IPT_CONNBYTES_DIR_BOTH:
                        {
                                u_int64_t bytes;
                                u_int64_t pkts;
                                u_int32_t pkts32;
                                bytes = ct->counters[IP_CT_DIR_ORIGINAL].bytes +
                                        ct->counters[IP_CT_DIR_REPLY].bytes;
                                pkts = ct->counters[IP_CT_DIR_ORIGINAL].packets +
                                        ct->counters[IP_CT_DIR_REPLY].packets;
                                if (pkts > 0xffffffff)
                                        pkts32 =  0xffffffff;
                                else
                                        pkts32 = pkts;
				what = mydiv(bytes,pkts);
                        }
                        break;
                }
                break;
        }
        if (sinfo->count.to)
                return (what <= sinfo->count.to && what >= sinfo->count.from);
        else
                return (what >= sinfo->count.from);
}

static int check(const char *tablename,
		 const struct ipt_ip *ip,
		 void *matchinfo,
		 unsigned int matchsize,
		 unsigned int hook_mask)
{
	const struct ipt_connbytes_info *sinfo = matchinfo;

	if (matchsize != IPT_ALIGN(sizeof(struct ipt_connbytes_info)))
		return 0;
        if (sinfo->what != IPT_CONNBYTES_PKTS &&
			sinfo->what != IPT_CONNBYTES_BYTES &&
		        sinfo->what != IPT_CONNBYTES_AVGPKT)
		        return 0;

	if (sinfo->direction != IPT_CONNBYTES_DIR_ORIGINAL &&
			sinfo->direction != IPT_CONNBYTES_DIR_REPLY &&
	                sinfo->direction != IPT_CONNBYTES_DIR_BOTH)
			return 0;

	return 1;
}

static struct ipt_match state_match
= { { NULL, NULL }, "connbytes", &match, &check, NULL, THIS_MODULE };

static int __init init(void)
{
	return ipt_register_match(&state_match);
}

static void __exit fini(void)
{
	ipt_unregister_match(&state_match);
}

module_init(init);
module_exit(fini);
MODULE_LICENSE("GPL");
