#include <linux/types.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/in.h>
#include <linux/udp.h>
#include <linux/netfilter_ipv4/ip_conntrack_protocol.h>
#include <linux/netfilter_ipv4/ip_conntrack_udp.h>

unsigned long ip_ct_udp_isakmp_timeout = (300*HZ);

static int udp_pkt_to_tuple(const void *datah, size_t datalen,
			    struct ip_conntrack_tuple *tuple)
{
	const struct udphdr *hdr = datah;
	struct isakmp_hdr *isakmp_h = (void *)hdr + 8;

	tuple->src.u.udp.port = hdr->source;
	tuple->dst.u.udp.port = hdr->dest;
	if(ntohs(hdr->source) == 500 && ntohs(hdr->dest) == 500)
	{
		if(NULL == isakmp_h)
			tuple->dst.u.udp.init_cookie = 0;
		else
			tuple->dst.u.udp.init_cookie = (unsigned int)(isakmp_h->init_cookie[0]);
	}
	else
		tuple->dst.u.udp.init_cookie = 0;

	return 1;
}

static int udp_invert_tuple(struct ip_conntrack_tuple *tuple,
			    const struct ip_conntrack_tuple *orig)
{
	tuple->src.u.udp.port = orig->dst.u.udp.port;
	tuple->dst.u.udp.port = orig->src.u.udp.port;
	tuple->dst.u.udp.init_cookie = orig->dst.u.udp.init_cookie;
	return 1;
}

/* Print out the per-protocol part of the tuple. */
static unsigned int udp_print_tuple(char *buffer,
				    const struct ip_conntrack_tuple *tuple)
{
	return sprintf(buffer, "sport=%hu dport=%hu ",
		       ntohs(tuple->src.u.udp.port),
		       ntohs(tuple->dst.u.udp.port));
}

/* Print out the private part of the conntrack. */
static unsigned int udp_print_conntrack(char *buffer,
					const struct ip_conntrack *conntrack)
{
	return 0;
}

/* Returns verdict for packet, and may modify conntracktype */
static int udp_packet(struct ip_conntrack *conntrack,
		      struct iphdr *iph, size_t len,
		      enum ip_conntrack_info conntrackinfo)
{
	u_int16_t *portptr;
	portptr = &conntrack->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.udp.port;
	/* If we've seen traffic both ways, this is some kind of UDP
	   stream.  Extend timeout. */
	if (conntrack->status & IPS_SEEN_REPLY)
	{
		if(ntohs(*portptr) == 500)
			ip_ct_refresh(conntrack, ip_ct_udp_isakmp_timeout);
		else
			ip_ct_refresh(conntrack, sysctl_ip_conntrack_udp_timeouts[UDP_STREAM_TIMEOUT]);
		/* Also, more likely to be important, and not a probe */
		set_bit(IPS_ASSURED_BIT, &conntrack->status);
	}
	else
		ip_ct_refresh(conntrack, sysctl_ip_conntrack_udp_timeouts[UDP_TIMEOUT]);

	return NF_ACCEPT;
}

/* Called when a new connection for this protocol found. */
static int udp_new(struct ip_conntrack *conntrack,
			     struct iphdr *iph, size_t len)
{
	return 1;
}

struct ip_conntrack_protocol ip_conntrack_protocol_udp
= { { NULL, NULL }, IPPROTO_UDP, "udp",
    udp_pkt_to_tuple, udp_invert_tuple, udp_print_tuple, udp_print_conntrack,
    udp_packet, udp_new, NULL, NULL, NULL };
