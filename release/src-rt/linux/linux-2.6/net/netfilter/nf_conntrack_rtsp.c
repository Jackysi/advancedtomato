/*
 * RTSP extension for IP connection tracking
 * (C) 2003 by Tom Marshall <tmarshall at real.com>
 * based on ip_conntrack_irc.c
 *
 *      This program is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU General Public License
 *      as published by the Free Software Foundation; either version
 *      2 of the License, or (at your option) any later version.
 *
 * Module load syntax:
 *   insmod nf_conntrack_rtsp.o ports=port1,port2,...port<MAX_PORTS>
 *                              max_outstanding=n setup_timeout=secs
 *
 * If no ports are specified, the default will be port 554.
 *
 * With max_outstanding you can define the maximum number of not yet
 * answered SETUP requests per RTSP session (default 8).
 * With setup_timeout you can specify how long the system waits for
 * an expected data channel (default 300 seconds).
 *
 * 2005-02-13: Harald Welte <laforge at netfilter.org>
 * 	- port to 2.6
 * 	- update to recent post-2.6.11 api changes
 * 2006-09-14: Steven Van Acker <deepstar at singularity.be>
 *      - removed calls to NAT code from conntrack helper: NAT no longer needed to use rtsp-conntrack
 * 2007-04-18: Michael Guntsche <mike at it-loops.com>
 * 			- Port to new NF API
 */

#include <linux/module.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/inet.h>
#include <net/tcp.h>

#include <net/netfilter/nf_conntrack.h>
#include <net/netfilter/nf_conntrack_expect.h>
#include <net/netfilter/nf_conntrack_helper.h>
#include <linux/netfilter/nf_conntrack_rtsp.h>

#define NF_NEED_STRNCASECMP
#define NF_NEED_STRTOU16
#define NF_NEED_STRTOU32
#define NF_NEED_NEXTLINE
#include <linux/netfilter_helpers.h>
#define NF_NEED_MIME_NEXTLINE
#include <linux/netfilter_mime.h>

#include <linux/ctype.h>
#define MAX_SIMUL_SETUP 8 /* XXX: use max_outstanding */
#define INFOP(fmt, args...) printk(KERN_INFO "%s: %s: " fmt, __FILE__, __FUNCTION__ , ## args)
#if 0
#define DEBUGP(fmt, args...) printk(KERN_DEBUG "%s: %s: " fmt, __FILE__, __FUNCTION__ , ## args)
#else
#define DEBUGP(fmt, args...)
#endif

#define MAX_PORTS 8
static int ports[MAX_PORTS];
static int num_ports = 0;
static int max_outstanding = 8;
static unsigned int setup_timeout = 300;

MODULE_AUTHOR("Tom Marshall <tmarshall at real.com>");
MODULE_DESCRIPTION("RTSP connection tracking module");
MODULE_LICENSE("GPL");
module_param_array(ports, int, &num_ports, 0400);
MODULE_PARM_DESC(ports, "port numbers of RTSP servers");
module_param(max_outstanding, int, 0400);
MODULE_PARM_DESC(max_outstanding, "max number of outstanding SETUP requests per RTSP session");
module_param(setup_timeout, int, 0400);
MODULE_PARM_DESC(setup_timeout, "timeout on for unestablished data channels");

static char *rtsp_buffer;
static DEFINE_SPINLOCK(rtsp_buffer_lock);


unsigned int (*nf_nat_rtsp_hook)(struct sk_buff **pskb,
				 enum ip_conntrack_info ctinfo,
				 unsigned int matchoff, unsigned int matchlen,
				 struct ip_ct_rtsp_expect* prtspexp,
				 struct nf_conntrack_expect *exp);
void (*nf_nat_rtsp_hook_expectfn)(struct nf_conn *ct, struct nf_conntrack_expect *exp);

EXPORT_SYMBOL_GPL(nf_nat_rtsp_hook);

/*
 * Max mappings we will allow for one RTSP connection (for RTP, the number
 * of allocated ports is twice this value).  Note that SMIL burns a lot of
 * ports so keep this reasonably high.  If this is too low, you will see a
 * lot of "no free client map entries" messages.
 */
#define MAX_PORT_MAPS 16

static u_int16_t g_tr_port = 7000;

#define PAUSE_TIMEOUT      (5 * HZ)
#define RTSP_PAUSE_TIMEOUT (6 * HZ)

/*** default port list was here in the masq code: 554, 3030, 4040 ***/

#define SKIP_WSPACE(ptr,len,off) while(off < len && isspace(*(ptr+off))) { off++; }

struct _rtsp_data_ports rtsp_data_ports[MAX_PORT_MAPS];
//EXPORT_SYMBOL_GPL(rtsp_data_ports);

/*
 * Performs NAT to client port mapping. Incoming UDP ports are looked up and
 * appropriate client ports are extracted from the table and returned.
 * Return client_udp_port or 0 when no matches found.
 */
static u_int16_t
rtsp_nat_to_client_pmap(u_int16_t nat_port)
{
    int i;
    u_int16_t tr_port = 0;

    for (i = 0; i < MAX_PORT_MAPS; i++) {
        if (!rtsp_data_ports[i].in_use)
            continue;
        /*
         * Check if the UDP ports match any of our NAT ports and return
         * the client UDP ports.
         */
        DEBUGP("Searching at index %d NAT_PORT %hu CLIENT PORTS (%hu-%hu)\n", i,
               ntohs(nat_port), rtsp_data_ports[i].client_udp_lo,
               rtsp_data_ports[i].client_udp_hi);
        if (ntohs(nat_port) == rtsp_data_ports[i].nat_udp_lo ||
            ntohs(nat_port) == rtsp_data_ports[i].client_udp_lo) {
            tr_port = rtsp_data_ports[i].client_udp_lo;
            DEBUGP("Found at index %d NAT_PORT %hu CLIENT PORTS (%hu-%hu) tr_port %hu\n", i,
                   nat_port, rtsp_data_ports[i].client_udp_lo,
                   rtsp_data_ports[i].client_udp_hi, tr_port);
        } else if (ntohs(nat_port) == rtsp_data_ports[i].nat_udp_hi ||
                   ntohs(nat_port) == rtsp_data_ports[i].client_udp_hi) {
            tr_port = rtsp_data_ports[i].client_udp_hi;
            DEBUGP("Found at index %d NAT_PORT %hu CLIENT PORTS %hu-%hu tr_port %hu\n", i,
                   nat_port, rtsp_data_ports[i].client_udp_lo,
                   rtsp_data_ports[i].client_udp_hi, tr_port);
            return tr_port;
        }
    }
    return tr_port;
}

static void
save_ct(struct nf_conn *ct)
{
    int i;
    struct nf_conntrack_tuple *tp = &ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple;

    for (i = 0; i < MAX_PORT_MAPS; i++) {
        if (!rtsp_data_ports[i].in_use)
            continue;
        if (rtsp_data_ports[i].nat_udp_lo == ntohs((tp)->dst.u.all)) {
            rtsp_data_ports[i].ct_lo = ct;
            break;
        }
        else if (rtsp_data_ports[i].nat_udp_hi == ntohs((tp)->dst.u.all)) {
            rtsp_data_ports[i].ct_hi = ct;
            break;
        }
    }
}

static void
rtp_expect(struct nf_conn *ct)
{
    u_int16_t nat_port = ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.udp.port;
    u_int16_t orig_port = rtsp_nat_to_client_pmap(nat_port);
    NF_CT_DUMP_TUPLE(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple);
    NF_CT_DUMP_TUPLE(&ct->tuplehash[IP_CT_DIR_REPLY].tuple);

    if (orig_port) {
        ct->proto.rtsp.orig_port = orig_port;
        DEBUGP("UDP client port %hu\n", ct->proto.rtsp.orig_port);
        save_ct(ct);
    }
}

static void
rtsp_pause_timeout(unsigned long data)
{
    int    index = (int)data;
    struct _rtsp_data_ports *rtsp_data = &rtsp_data_ports[index];
    struct nf_conn *ct_lo = rtsp_data->ct_lo;

    if (rtsp_data->in_use) {
        setup_timer(&rtsp_data->pause_timeout, rtsp_pause_timeout, (unsigned long)data);
        rtsp_data->pause_timeout.expires = jiffies + PAUSE_TIMEOUT;
        nf_ct_refresh(ct_lo, rtsp_data->skb, RTSP_PAUSE_TIMEOUT);
        add_timer(&rtsp_data->pause_timeout);
        rtsp_data->timeout_active = 1;
    }
}

static void
ip_conntrack_rtsp_proc_play(struct sk_buff **pskb, struct nf_conn *ct, const struct iphdr *iph)
{
    int i;
    struct tcphdr *tcph = (void *)iph + iph->ihl * 4;

    for (i = 0; i < MAX_PORT_MAPS; i++) {
        if (!rtsp_data_ports[i].in_use)
            continue;

        DEBUGP("Searching client info IP %u.%u.%u.%u->%hu PORTS (%hu-%hu)\n",
                NIPQUAD(iph->saddr), tcph->source, rtsp_data_ports[i].client_udp_lo,
                rtsp_data_ports[i].client_udp_hi);
        if ((rtsp_data_ports[i].client_ip == iph->saddr) &&
            (rtsp_data_ports[i].client_tcp_port == tcph->source))
        {
            DEBUGP("Found client info SRC IP %u.%u.%u.%u TCP PORT %hu UDP PORTS (%hu-%hu)\n",
                    NIPQUAD(iph->saddr), tcph->source, rtsp_data_ports[i].client_udp_lo,
                    rtsp_data_ports[i].client_udp_hi);
            if (rtsp_data_ports[i].timeout_active) {
                del_timer(&rtsp_data_ports[i].pause_timeout);
                rtsp_data_ports[i].timeout_active = 0;
            }
        }
    }
}

static void
ip_conntrack_rtsp_proc_pause(struct sk_buff **pskb, struct nf_conn *ct, const struct iphdr *iph)
{
    int i;
    struct tcphdr *tcph = (void *)iph + iph->ihl * 4;

    for (i = 0; i < MAX_PORT_MAPS; i++) {
        if (!rtsp_data_ports[i].in_use)
            continue;

        DEBUGP("Searching client info IP %u.%u.%u.%u->%hu PORTS (%hu-%hu)\n",
                NIPQUAD(iph->saddr), tcph->source, rtsp_data_ports[i].client_udp_lo,
                rtsp_data_ports[i].client_udp_hi);
        if ((rtsp_data_ports[i].client_ip == iph->saddr) &&
            (rtsp_data_ports[i].client_tcp_port == tcph->source))
        {
            DEBUGP("Found client info SRC IP %u.%u.%u.%u TCP PORT %hu UDP PORTS (%hu-%hu)\n",
                    NIPQUAD(iph->saddr), tcph->source, rtsp_data_ports[i].client_udp_lo,
                    rtsp_data_ports[i].client_udp_hi);
            if (rtsp_data_ports[i].timeout_active != 0 ||
                rtsp_data_ports[i].ct_lo == NULL)
                break;

            setup_timer(&rtsp_data_ports[i].pause_timeout, rtsp_pause_timeout, (unsigned long)i);
            rtsp_data_ports[i].pause_timeout.expires = jiffies + PAUSE_TIMEOUT;
            add_timer(&rtsp_data_ports[i].pause_timeout);
            rtsp_data_ports[i].timeout_active = 1;
            rtsp_data_ports[i].ct_lo = ct;
            rtsp_data_ports[i].skb = *pskb;
            nf_ct_refresh(ct, *pskb, RTSP_PAUSE_TIMEOUT);
        }
    }
}

/*
 * Maps client ports that are overlapping with other client UDP transport to
 * new NAT ports that will be tracked and converted back to client assigned
 * UDP ports.
 * Return (N/A)
 */
static int
rtsp_client_to_nat_pmap(struct sk_buff **pskb, struct ip_ct_rtsp_expect *prtspexp,
                        const struct iphdr *iph, struct nf_conn *ct)
{
    int i  = 0;
    int rc = 0;
    struct tcphdr *tcph   = (void *)iph + iph->ihl * 4;

    DEBUGP("IP %u.%u.%u.%u->%u.%u.%u.%u PORTS (%hu-%hu)\n", NIPQUAD(iph->saddr),
           NIPQUAD(iph->daddr), tcph->source, tcph->dest);

    for (i = 0; i < MAX_PORT_MAPS; i++) {
        if (rtsp_data_ports[i].in_use) {
            DEBUGP("Index %d in_use flag %d IP %u.%u.%u.%u CLIENT %hu-%hu NAT %hu-%hu\n", i,
                   rtsp_data_ports[i].in_use, NIPQUAD(rtsp_data_ports[i].client_ip),
                   rtsp_data_ports[i].client_udp_lo, rtsp_data_ports[i].client_udp_hi,
                   rtsp_data_ports[i].nat_udp_lo, rtsp_data_ports[i].nat_udp_hi);
            if (ntohl(iph->saddr) == rtsp_data_ports[i].client_ip &&
                ntohs(tcph->source) == rtsp_data_ports[i].client_tcp_port &&
                ntohs(prtspexp->loport) == rtsp_data_ports[i].client_udp_lo &&
                ntohs(prtspexp->hiport) == rtsp_data_ports[i].client_udp_hi)
            {
                prtspexp->loport  = rtsp_data_ports[i].nat_udp_lo;
                prtspexp->hiport  = rtsp_data_ports[i].nat_udp_hi;
                return rc = 2;
            }
            continue;
        }
        rtsp_data_ports[i].skb             = *pskb;
        rtsp_data_ports[i].client_ip       = ntohl(iph->saddr);
        rtsp_data_ports[i].client_tcp_port = ntohs(tcph->source);
        rtsp_data_ports[i].client_udp_lo   = ntohs(prtspexp->loport);
        rtsp_data_ports[i].client_udp_hi   = ntohs(prtspexp->hiport);
        rtsp_data_ports[i].pbtype          = prtspexp->pbtype;
        rtsp_data_ports[i].in_use          = 1;
        init_timer(&rtsp_data_ports[i].pause_timeout);
        DEBUGP("Mapped at index %d ORIGINAL PORTS %hu-%hu\n", i,
               ntohs(prtspexp->loport), ntohs(prtspexp->hiport));
        prtspexp->loport = rtsp_data_ports[i].nat_udp_lo = g_tr_port++;
        prtspexp->hiport = rtsp_data_ports[i].nat_udp_hi = g_tr_port++;
        DEBUGP("NEW PORTS %hu-%hu\n", ntohs(prtspexp->loport), ntohs(prtspexp->hiport));
        return rc = 1;
    }
    return rc;
}

static void
ip_conntrack_rtsp_proc_teardown(struct sk_buff **pskb, struct iphdr *iph)
{
    int i;
    struct tcphdr *tcph = (void *)iph + iph->ihl * 4;

    for (i = 0; i < MAX_PORT_MAPS; i++) {
        if (!rtsp_data_ports[i].in_use)
            continue;

        DEBUGP("Searching client info IP %u.%u.%u.%u->%hu PORTS (%hu-%hu)\n",
                NIPQUAD(iph->saddr), tcph->source, rtsp_data_ports[i].client_udp_lo,
                rtsp_data_ports[i].client_udp_hi);
        if ((rtsp_data_ports[i].client_ip == iph->saddr) &&
            (rtsp_data_ports[i].client_tcp_port == tcph->source))
        {
            DEBUGP("Found client info SRC IP %u.%u.%u.%u TCP PORT %hu UDP PORTS (%hu-%hu)\n",
                    NIPQUAD(iph->saddr), tcph->source, rtsp_data_ports[i].client_udp_lo,
                    rtsp_data_ports[i].client_udp_hi);
            if (rtsp_data_ports[i].timeout_active) {
                del_timer(&rtsp_data_ports[i].pause_timeout);
                rtsp_data_ports[i].timeout_active = 0;
            }
            memset(&rtsp_data_ports[i], 0, sizeof(struct _rtsp_data_ports));
            rtsp_data_ports[i].in_use = 0;
            //break;
        }
    }
}

static void *
find_char(void *str, int ch, size_t len)
{
    unsigned char *pStr = NULL;
    if (len != 0) {
        pStr = str;
        do {
            if (*pStr++ == ch)
                return (void *)(pStr - 1);
        } while (--len != 0);
    }
    return NULL;
}

/*
 * Parse an RTSP packet.
 *
 * Returns zero if parsing failed.
 *
 * Parameters:
 *  IN      ptcp        tcp data pointer
 *  IN      tcplen      tcp data len
 *  IN/OUT  ptcpoff     points to current tcp offset
 *  OUT     phdrsoff    set to offset of rtsp headers
 *  OUT     phdrslen    set to length of rtsp headers
 *  OUT     pcseqoff    set to offset of CSeq header
 *  OUT     pcseqlen    set to length of CSeq header
 */
static int
rtsp_parse_message(char* ptcp, uint tcplen, uint* ptcpoff,
                   uint* phdrsoff, uint* phdrslen,
                   uint* pcseqoff, uint* pcseqlen,
                   uint* transoff, uint* translen)
{
	uint    entitylen = 0;
	uint    lineoff;
	uint    linelen;

	if (!nf_nextline(ptcp, tcplen, ptcpoff, &lineoff, &linelen))
		return 0;

	*phdrsoff = *ptcpoff;
	while (nf_mime_nextline(ptcp, tcplen, ptcpoff, &lineoff, &linelen)) {
		if (linelen == 0) {
			if (entitylen > 0)
				*ptcpoff += min(entitylen, tcplen - *ptcpoff);
			break;
		}
		if (lineoff+linelen > tcplen) {
			INFOP("!! overrun !!\n");
			break;
		}

		if (nf_strncasecmp(ptcp+lineoff, "CSeq:", 5) == 0) {
			*pcseqoff = lineoff;
			*pcseqlen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Transport:", 10) == 0) {
			*transoff = lineoff;
			*translen = linelen;
		}

		if (nf_strncasecmp(ptcp+lineoff, "Content-Length:", 15) == 0) {
			uint off = lineoff+15;
			SKIP_WSPACE(ptcp+lineoff, linelen, off);
			nf_strtou32(ptcp+off, &entitylen);
		}
	}
	*phdrslen = (*ptcpoff) - (*phdrsoff);

	return 1;
}

/*
 * Find lo/hi client ports (if any) in transport header
 * In:
 *   ptcp, tcplen = packet
 *   tranoff, tranlen = buffer to search
 *
 * Out:
 *   pport_lo, pport_hi = lo/hi ports (host endian)
 *
 * Returns nonzero if any client ports found
 *
 * Note: it is valid (and expected) for the client to request multiple
 * transports, so we need to parse the entire line.
 */
static int
rtsp_parse_transport(char* ptran, uint tranlen,
                     struct ip_ct_rtsp_expect* prtspexp)
{
	int     rc = 0;
	uint    off = 0;

	if (tranlen < 10 || !iseol(ptran[tranlen-1]) ||
	    nf_strncasecmp(ptran, "Transport:", 10) != 0) {
		INFOP("sanity check failed\n");
		return 0;
	}

	DEBUGP("tran='%.*s'\n", (int)tranlen, ptran);
	off += 10;
	SKIP_WSPACE(ptran, tranlen, off);

	/* Transport: tran;field;field=val,tran;field;field=val,... */
	while (off < tranlen) {
		const char* pparamend;
		uint        nextparamoff;

		pparamend = find_char(ptran+off, ',', tranlen-off);
		pparamend = (pparamend == NULL) ? ptran+tranlen : pparamend+1;
		nextparamoff = pparamend-ptran;

		while (off < nextparamoff) {
			const char* pfieldend;
			uint        nextfieldoff;

			pfieldend = find_char(ptran+off, ';', nextparamoff-off);
			nextfieldoff = (pfieldend == NULL) ? nextparamoff : pfieldend-ptran+1;

			if (strncmp(ptran+off, "client_port=", 12) == 0) {
				u_int16_t   port;
				uint        numlen;

				off += 12;
				numlen = nf_strtou16(ptran+off, &port);
				off += numlen;
				if (prtspexp->loport != 0 && prtspexp->loport != port)
					DEBUGP("multiple ports found, port %hu ignored\n", port);
				else {
					DEBUGP("lo port found : %hu\n", port);
					prtspexp->loport = prtspexp->hiport = port;
					if (ptran[off] == '-') {
						off++;
						numlen = nf_strtou16(ptran+off, &port);
						off += numlen;
						prtspexp->pbtype = pb_range;
						prtspexp->hiport = port;

						// If we have a range, assume rtp:
						// loport must be even, hiport must be loport+1
						if ((prtspexp->loport & 0x0001) != 0 ||
						    prtspexp->hiport != prtspexp->loport+1) {
							DEBUGP("incorrect range: %hu-%hu, correcting\n",
							       prtspexp->loport, prtspexp->hiport);
							prtspexp->loport &= 0xfffe;
							prtspexp->hiport = prtspexp->loport+1;
						}
					} else if (ptran[off] == '/') {
						off++;
						numlen = nf_strtou16(ptran+off, &port);
						off += numlen;
						prtspexp->pbtype = pb_discon;
						prtspexp->hiport = port;
					}
					rc = 1;
				}
			}

			/*
			 * Note we don't look for the destination parameter here.
			 * If we are using NAT, the NAT module will handle it.  If not,
			 * and the client is sending packets elsewhere, the expectation
			 * will quietly time out.
			 */

			off = nextfieldoff;
		}

		off = nextparamoff;
	}

	return rc;
}

void expected(struct nf_conn *ct, struct nf_conntrack_expect *exp)
{
	typeof(nf_nat_rtsp_hook_expectfn) nf_nat_rtsp_expectfn;

	rtp_expect(ct);
	nf_nat_rtsp_expectfn = rcu_dereference(nf_nat_rtsp_hook_expectfn);

	if (nf_nat_rtsp_expectfn && ct->master->status & IPS_NAT_MASK) {
		nf_nat_rtsp_expectfn(ct,exp);
	}
}

/*** conntrack functions ***/

/* outbound packet: client->server */

static inline int
help_out(struct sk_buff **pskb, unsigned char *rb_ptr, unsigned int datalen,
	struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	struct iphdr *iph = ip_hdr(*pskb);
	struct ip_ct_rtsp_expect expinfo;

	int dir = CTINFO2DIR(ctinfo);   /* = IP_CT_DIR_ORIGINAL */
	//struct  tcphdr* tcph = (void*)iph + iph->ihl * 4;
	//uint    tcplen = pktlen - iph->ihl * 4;
	char*   pdata = rb_ptr;
	//uint    datalen = tcplen - tcph->doff * 4;
	uint    dataoff = 0;
	int ret = NF_ACCEPT;

	struct nf_conntrack_expect *exp;
	typeof(nf_nat_rtsp_hook) nf_nat_rtsp;

	__be16 be_loport;

	memset(&expinfo, 0, sizeof(expinfo));

	while (dataoff < datalen) {
		uint    cmdoff = dataoff;
		uint    hdrsoff = 0;
		uint    hdrslen = 0;
		uint    cseqoff = 0;
		uint    cseqlen = 0;
		uint    transoff = 0;
		uint    translen = 0;
		uint    off;
		uint    port = 0;
		struct  nf_conntrack_expect *new_exp = NULL;
		int     res = 0;

		if (!rtsp_parse_message(pdata, datalen, &dataoff,
					&hdrsoff, &hdrslen,
					&cseqoff, &cseqlen,
					&transoff, &translen))
			break;      /* not a valid message */

		if (strncmp(pdata+cmdoff, "PLAY ", 5) == 0) {
			ip_conntrack_rtsp_proc_play(pskb, ct, iph);
			continue;
		}
		if (strncmp(pdata+cmdoff, "PAUSE ", 6) == 0) {
			ip_conntrack_rtsp_proc_pause(pskb, ct, iph);
			continue;
		}
		if (strncmp(pdata+cmdoff, "TEARDOWN ", 6) == 0)	{
			ip_conntrack_rtsp_proc_teardown(pskb, iph);   /* TEARDOWN message */
			continue;
		}

		if (strncmp(pdata+cmdoff, "SETUP ", 6) != 0)
			continue;   /* not a SETUP message */
		DEBUGP("found a setup message\n");

		off = 0;
		if(translen) {
			rtsp_parse_transport(pdata+transoff, translen, &expinfo);
		}

		if (expinfo.loport == 0) {
			DEBUGP("no udp transports found\n");
			continue;   /* no udp transports found */
		}

		DEBUGP("udp transport found, ports=(%d,%hu,%hu)\n",
		       (int)expinfo.pbtype, expinfo.loport, expinfo.hiport);

		/*
		 * Translate the original ports to the NAT ports and note them
		 * down to translate back in the return direction.
		 */
		if (!(res = rtsp_client_to_nat_pmap(pskb, &expinfo, iph, ct)))
		{
			DEBUGP("Dropping the packet. No more space in the mapping table\n");
			ret = NF_DROP;
			goto out;
		}

		port = expinfo.loport;
		while (port <= expinfo.hiport) {
			/*
			 * Allocate expectation for tracking this connection
			 */
			new_exp = nf_conntrack_expect_alloc(ct);
			if (!new_exp) {
				ret = NF_DROP;
				goto out;
			}
			memcpy(new_exp, &exp, sizeof(struct nf_conntrack_expect));

			if (res == 2) {
				be_loport = htons(g_tr_port);
				g_tr_port++;
			} else
				be_loport = htons(port);

			nf_conntrack_expect_init(new_exp, ct->tuplehash[!dir].tuple.src.l3num,
				&ct->tuplehash[!dir].tuple.src.u3, &ct->tuplehash[!dir].tuple.dst.u3,
				IPPROTO_UDP, NULL, &be_loport);

			new_exp->master = ct;
			new_exp->expectfn = expected;
			new_exp->flags = 0;

			if (expinfo.pbtype == pb_range) {
				DEBUGP("Changing expectation mask to handle multiple ports\n");
				//new_exp->mask.src.u.udp.port  = 0xfffe;
			}

			DEBUGP("expect_related %u.%u.%u.%u:%u-%u.%u.%u.%u:%u\n",
			       NIPQUAD(new_exp->tuple.src.u3.ip),
			       ntohs(new_exp->tuple.src.u.udp.port),
			       NIPQUAD(new_exp->tuple.dst.u3.ip),
			       ntohs(new_exp->tuple.dst.u.udp.port));

			nf_nat_rtsp = rcu_dereference(nf_nat_rtsp_hook);
			if (nf_nat_rtsp && ct->status & IPS_NAT_MASK)
				/* pass the request off to the nat helper */
				ret = nf_nat_rtsp(pskb, ctinfo, hdrsoff, hdrslen, &expinfo, new_exp);
			else if (nf_conntrack_expect_related(new_exp) != 0) {
				INFOP("nf_conntrack_expect_related failed\n");
				ret  = NF_DROP;
			}
			nf_conntrack_expect_put(new_exp);

			port++;
		}
		goto out;
	}
out:

	return ret;
}


static inline int
help_in(struct sk_buff **pskb, size_t pktlen,
	struct nf_conn* ct, enum ip_conntrack_info ctinfo)
{
	return NF_ACCEPT;
}

static int help(struct sk_buff **pskb, unsigned int protoff,
		struct nf_conn *ct, enum ip_conntrack_info ctinfo)
{
	struct tcphdr _tcph, *th;
	unsigned int dataoff, datalen;
	char *rb_ptr;
	int ret = NF_DROP;

	/* Until there's been traffic both ways, don't look in packets. */
	if (ctinfo != IP_CT_ESTABLISHED &&
	    ctinfo != IP_CT_ESTABLISHED + IP_CT_IS_REPLY) {
		DEBUGP("conntrackinfo = %u\n", ctinfo);
		return NF_ACCEPT;
	}

	/* Not whole TCP header? */
	th = skb_header_pointer(*pskb, protoff, sizeof(_tcph), &_tcph);

	if (!th)
		return NF_ACCEPT;

	/* No data ? */
	dataoff = protoff + th->doff*4;
	datalen = (*pskb)->len - dataoff;
	if (dataoff >= (*pskb)->len)
		return NF_ACCEPT;

	spin_lock_bh(&rtsp_buffer_lock);
	rb_ptr = skb_header_pointer(*pskb, dataoff,
				    (*pskb)->len - dataoff, rtsp_buffer);
	BUG_ON(rb_ptr == NULL);

#if 0
	/* Checksum invalid?  Ignore. */
	/* FIXME: Source route IP option packets --RR */
	if (tcp_v4_check(tcph, tcplen, iph->saddr, iph->daddr,
			 csum_partial((char*)tcph, tcplen, 0)))
	{
		DEBUGP("bad csum: %p %u %u.%u.%u.%u %u.%u.%u.%u\n",
		       tcph, tcplen, NIPQUAD(iph->saddr), NIPQUAD(iph->daddr));
		return NF_ACCEPT;
	}
#endif

	switch (CTINFO2DIR(ctinfo)) {
	case IP_CT_DIR_ORIGINAL:
		ret = help_out(pskb, rb_ptr, datalen, ct, ctinfo);
		break;
	case IP_CT_DIR_REPLY:
		DEBUGP("IP_CT_DIR_REPLY\n");
		/* inbound packet: server->client */
		ret = NF_ACCEPT;
		break;
	default:
		/* oops */
		break;
	}

	spin_unlock_bh(&rtsp_buffer_lock);

	return ret;
}

static struct nf_conntrack_helper rtsp_helpers[MAX_PORTS];
static char rtsp_names[MAX_PORTS][10];

/* This function is intentionally _NOT_ defined as __exit */
static void
fini(void)
{
	int i;
	for (i = 0; i < num_ports; i++) {
		DEBUGP("unregistering port %d\n", ports[i]);
		nf_conntrack_helper_unregister(&rtsp_helpers[i]);
	}
	for (i = 0; i < MAX_PORT_MAPS; i++) {
		if (!rtsp_data_ports[i].in_use)
			continue;
		if (rtsp_data_ports[i].timeout_active == 1)
			del_timer(&rtsp_data_ports[i].pause_timeout);
		rtsp_data_ports[i].timeout_active = 0;
		rtsp_data_ports[i].in_use = 0;
	}
	kfree(rtsp_buffer);
}

static int __init
init(void)
{
	int i, ret;
	struct nf_conntrack_helper *hlpr;
	char *tmpname;

	printk("nf_conntrack_rtsp v" IP_NF_RTSP_VERSION " loading\n");

	if (max_outstanding < 1) {
		printk("nf_conntrack_rtsp: max_outstanding must be a positive integer\n");
		return -EBUSY;
	}
	if (setup_timeout < 0) {
		printk("nf_conntrack_rtsp: setup_timeout must be a positive integer\n");
		return -EBUSY;
	}

	rtsp_buffer = kmalloc(65536, GFP_KERNEL);
	if (!rtsp_buffer)
		return -ENOMEM;

	/* If no port given, default to standard rtsp port */
	if (ports[0] == 0) {
		ports[0] = RTSP_PORT;
	}

	for (i = 0; i < MAX_PORT_MAPS; i++) {
		memset(&rtsp_data_ports[i], 0, sizeof(struct _rtsp_data_ports));
		rtsp_data_ports[i].in_use = 0;
	}

	for (i = 0; (i < MAX_PORTS) && ports[i]; i++) {
		hlpr = &rtsp_helpers[i];
		memset(hlpr, 0, sizeof(struct nf_conntrack_helper));
		hlpr->tuple.src.l3num = AF_INET;
		hlpr->tuple.src.u.tcp.port = htons(ports[i]);
		hlpr->tuple.dst.protonum = IPPROTO_TCP;
		hlpr->max_expected = max_outstanding;
		hlpr->timeout = setup_timeout;
		hlpr->me = THIS_MODULE;
		hlpr->help = help;

		tmpname = &rtsp_names[i][0];
		if (ports[i] == RTSP_PORT) {
			sprintf(tmpname, "rtsp");
		} else {
			sprintf(tmpname, "rtsp-%d", i);
		}
		hlpr->name = tmpname;

		DEBUGP("port #%d: %d\n", i, ports[i]);

		ret = nf_conntrack_helper_register(hlpr);

		if (ret) {
			printk("nf_conntrack_rtsp: ERROR registering port %d\n", ports[i]);
			fini();
			return -EBUSY;
		}
		num_ports++;
	}
	return 0;
}

module_init(init);
module_exit(fini);

EXPORT_SYMBOL(nf_nat_rtsp_hook_expectfn);

