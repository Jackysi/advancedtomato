/*
 * ip_nat_pptp.c	- Version 1.11
 *
 * NAT support for PPTP (Point to Point Tunneling Protocol).
 * PPTP is a a protocol for creating virtual private networks.
 * It is a specification defined by Microsoft and some vendors
 * working with Microsoft.  PPTP is built on top of a modified
 * version of the Internet Generic Routing Encapsulation Protocol.
 * GRE is defined in RFC 1701 and RFC 1702.  Documentation of
 * PPTP can be found in RFC 2637
 *
 * (C) 2000-2002 by Harald Welte <laforge@gnumonks.org>
 *
 * Development of this code funded by Astaro AG (http://www.astaro.com/)
 *
 * TODO: - Support for multiple calls within one session
 * 	   (needs netfilter newnat code)
 * 	 - NAT to a unique tuple, not to TCP source port
 * 	   (needs netfilter tuple reservation)
 * 	 - Support other NAT scenarios than SNAT of PNS
 * 
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <net/tcp.h>
#include <linux/netfilter_ipv4/ip_nat.h>
#include <linux/netfilter_ipv4/ip_nat_rule.h>
#include <linux/netfilter_ipv4/ip_nat_helper.h>
#include <linux/netfilter_ipv4/ip_nat_pptp.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_proto_gre.h>
#include <linux/netfilter_ipv4/ip_conntrack_pptp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harald Welte <laforge@gnumonks.org>");
MODULE_DESCRIPTION("Netfilter NAT helper module for PPTP");


#if 0
#include "ip_conntrack_pptp_priv.h"
#define DEBUGP(format, args...) printk(KERN_DEBUG __FILE__ ":" __FUNCTION__ \
				       ": " format, ## args)
#else
#define DEBUGP(format, args...)
#endif

static unsigned int
pptp_nat_expected(struct sk_buff **pskb,
		  unsigned int hooknum,
		  struct ip_conntrack *ct,
		  struct ip_nat_info *info)
{
	struct ip_conntrack *master = master_ct(ct);
	struct ip_nat_multi_range mr;
	struct ip_ct_pptp_master *ct_pptp_info;
	struct ip_nat_pptp *nat_pptp_info;
	u_int32_t newsrcip, newdstip, newcid;
	int ret;

	IP_NF_ASSERT(info);
	IP_NF_ASSERT(master);
	IP_NF_ASSERT(!(info->initialized & (1 << HOOK2MANIP(hooknum))));

	DEBUGP("we have a connection!\n");

	LOCK_BH(&ip_pptp_lock);
	ct_pptp_info = &master->help.ct_pptp_info;
	nat_pptp_info = &master->nat.help.nat_pptp_info;

	/* need to alter GRE tuple because conntrack expectfn() used 'wrong'
	 * (unmanipulated) values */
	if (hooknum == NF_IP_PRE_ROUTING) {
		DEBUGP("completing tuples with NAT info \n");
		/* we can do this, since we're unconfirmed */
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.gre.key ==
			htonl(ct_pptp_info->pac_call_id)) {	
			/* assume PNS->PAC */
			ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.gre.key =
				htonl(nat_pptp_info->pns_call_id);
//			ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.u.gre.key =
//				htonl(nat_pptp_info->pac_call_id);
			ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.gre.key =
				htonl(nat_pptp_info->pns_call_id);
		} else {
			/* assume PAC->PNS */
			DEBUGP("WRONG DIRECTION\n");
			ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.u.gre.key =
				htonl(nat_pptp_info->pac_call_id);
			ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.u.gre.key =
				htonl(nat_pptp_info->pac_call_id);
		}
	}

	if (HOOK2MANIP(hooknum) == IP_NAT_MANIP_DST)
	{
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.gre.key == htonl(ct_pptp_info->pac_call_id))
		{
			/* assume PNS->PAC */
			newdstip = master->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip;
			newcid = htonl(nat_pptp_info->pac_call_id);
		}
		else
		{
			/* assume PAC->PNS */
			newdstip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.src.ip;
			newcid = htonl(nat_pptp_info->pns_call_id);
		}
		mr.rangesize = 1;
		mr.range[0].flags = IP_NAT_RANGE_MAP_IPS | IP_NAT_RANGE_PROTO_SPECIFIED;
		mr.range[0].min_ip = mr.range[0].max_ip = newdstip;
		mr.range[0].min = mr.range[0].max = 
			((union ip_conntrack_manip_proto ) { newcid }); 
		DEBUGP("change dest ip to %u.%u.%u.%u\n", 
			NIPQUAD(newdstip));
		DEBUGP("change dest key to 0x%x\n", ntohl(newcid));
		ret = ip_nat_setup_info(ct, &mr, hooknum);
	} else {
		if (ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.u.gre.key == htonl(ct_pptp_info->pac_call_id))
		{
		newsrcip = master->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
			newcid = htonl(ct_pptp_info->pns_call_id);
		}
		else
		{
			/* assume PAC->PNS */
			newsrcip = master->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip;
			newcid = htonl(ct_pptp_info->pac_call_id);
		}

		mr.rangesize = 1;
		mr.range[0].flags = IP_NAT_RANGE_MAP_IPS
			            |IP_NAT_RANGE_PROTO_SPECIFIED;
		mr.range[0].min_ip = mr.range[0].max_ip = newsrcip;
		mr.range[0].min = mr.range[0].max = 
			((union ip_conntrack_manip_proto ) { newcid });
		DEBUGP("change src ip to %u.%u.%u.%u\n", 
			NIPQUAD(newsrcip));
		DEBUGP("change 'src' key to 0x%x\n", ntohl(newcid));
		ret = ip_nat_setup_info(ct, &mr, hooknum);
	}

	UNLOCK_BH(&ip_pptp_lock);

	return ret;

}

/* outbound packets == from PNS to PAC */
static inline unsigned int
pptp_outbound_pkt(struct tcphdr *tcph, struct pptp_pkt_hdr *pptph,
		  size_t datalen,
		  struct ip_conntrack *ct,
		  enum ip_conntrack_info ctinfo,
		  struct ip_conntrack_expect *exp)

{
	struct PptpControlHeader *ctlh;
	union pptp_ctrl_union pptpReq;
	struct ip_ct_pptp_master *ct_pptp_info = &ct->help.ct_pptp_info;
	struct ip_nat_pptp *nat_pptp_info = &ct->nat.help.nat_pptp_info;

	u_int16_t msg, *cid = NULL, new_callid;

	ctlh = (struct PptpControlHeader *) ((void *) pptph + sizeof(*pptph));
	pptpReq.rawreq = (void *) ((void *) ctlh + sizeof(*ctlh));

	new_callid = htons(ct_pptp_info->pns_call_id);
	
	switch (msg = ntohs(ctlh->messageType)) {
		case PPTP_OUT_CALL_REQUEST:
			cid = &pptpReq.ocreq->callID;

			/* save original call ID in nat_info */
			nat_pptp_info->pns_call_id = ct_pptp_info->pns_call_id;

			new_callid = tcph->source;
			/* save new call ID in ct info */
			ct_pptp_info->pns_call_id = ntohs(new_callid);
			break;
		case PPTP_IN_CALL_REPLY:
			cid = &pptpReq.icreq->callID;
			break;
		case PPTP_CALL_CLEAR_REQUEST:
			cid = &pptpReq.clrreq->callID;
			break;
		case PPTP_CALL_DISCONNECT_NOTIFY:
			cid = &pptpReq.disc->callID;
			break;

		default:
			DEBUGP("unknown outbound packet 0x%04x:%s\n", msg,
			      (msg <= PPTP_MSG_MAX)? strMName[msg]:strMName[0]);
			/* fall through */

		case PPTP_SET_LINK_INFO:
			/* only need to NAT in case PAC is behind NAT box */
		case PPTP_START_SESSION_REQUEST:
		case PPTP_START_SESSION_REPLY:
		case PPTP_STOP_SESSION_REQUEST:
		case PPTP_STOP_SESSION_REPLY:
		case PPTP_ECHO_REQUEST:
		case PPTP_ECHO_REPLY:
			/* no need to alter packet */
		DEBUGP("outbound control message %s\n", strMName[msg]);
		DEBUGP("ct->pac_call_id = %d\n", ct_pptp_info->pac_call_id);
		DEBUGP("ct->pns_call_id = %d\n", ct_pptp_info->pns_call_id);
		DEBUGP("nat->pac_call_id = %d\n", nat_pptp_info->pac_call_id);
		DEBUGP("nat->pns_call_id = %d\n", nat_pptp_info->pns_call_id);
			return NF_ACCEPT;
	}

	IP_NF_ASSERT(cid);

	DEBUGP("altering call id from 0x%04x to 0x%04x\n",
		ntohs(*cid), ntohs(new_callid));
	/* mangle packet */
	tcph->check = ip_nat_cheat_check(*cid^0xFFFF, 
					 new_callid, tcph->check);
	*cid = new_callid;

		DEBUGP("outbound control message %s\n", strMName[msg]);
		DEBUGP("ct->pac_call_id = %d\n", ct_pptp_info->pac_call_id);
		DEBUGP("ct->pns_call_id = %d\n", ct_pptp_info->pns_call_id);
		DEBUGP("nat->pac_call_id = %d\n", nat_pptp_info->pac_call_id);
		DEBUGP("nat->pns_call_id = %d\n", nat_pptp_info->pns_call_id);
	return NF_ACCEPT;
}

/* inbound packets == from PAC to PNS */
static inline unsigned int
pptp_inbound_pkt(struct tcphdr *tcph, struct pptp_pkt_hdr *pptph,
		 size_t datalen,
		 struct ip_conntrack *ct,
		 enum ip_conntrack_info ctinfo,
		 struct ip_conntrack_expect *oldexp)
{
	struct PptpControlHeader *ctlh;
	union pptp_ctrl_union pptpReq;
	struct ip_ct_pptp_master *ct_pptp_info = &ct->help.ct_pptp_info;
	struct ip_nat_pptp *nat_pptp_info = &ct->nat.help.nat_pptp_info;

	u_int16_t msg, new_cid = 0, new_pcid, *pcid = NULL, *cid = NULL;
	u_int32_t old_dst_ip;

	struct ip_conntrack_tuple t;

	ctlh = (struct PptpControlHeader *) ((void *) pptph + sizeof(*pptph));
	pptpReq.rawreq = (void *) ((void *) ctlh + sizeof(*ctlh));

	new_pcid = htons(nat_pptp_info->pns_call_id);

	switch (msg = ntohs(ctlh->messageType)) {
	case PPTP_OUT_CALL_REPLY:
		pcid = &pptpReq.ocack->peersCallID;	
		cid = &pptpReq.ocack->callID;
		if (!oldexp) {
			DEBUGP("outcall but no expectation\n");
			break;
		}
		old_dst_ip = oldexp->tuple.dst.ip;
		t = oldexp->tuple;

		/* save original PAC call ID in nat_info */
		nat_pptp_info->pac_call_id = ct_pptp_info->pac_call_id;

		/* store new callID in ct_info, so conntrack works */
		//ct_pptp_info->pac_call_id = ntohs(tcph->source);
		//new_cid = htons(ct_pptp_info->pac_call_id);

		/* alter expectation */
		if (t.dst.ip == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip) {
			/* expectation for PNS->PAC direction */
			t.dst.u.gre.key = htonl(ct_pptp_info->pac_call_id);
			t.src.u.gre.key = htonl(nat_pptp_info->pns_call_id);
		} else {
			/* expectation for PAC->PNS direction */
			t.dst.ip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
			DEBUGP("EXPECTATION IN WRONG DIRECTION!!!\n");
		}

		if (!ip_conntrack_change_expect(oldexp, &t)) {
			DEBUGP("successfully changed expect\n");
		} else {
			DEBUGP("can't change expect\n");
		}
		ip_ct_gre_keymap_change(oldexp->proto.gre.keymap_orig, &t);
		/* reply keymap */
		t.src.ip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.src.ip;
		t.dst.ip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
		t.src.u.gre.key = htonl(nat_pptp_info->pac_call_id);
		t.dst.u.gre.key = htonl(ct_pptp_info->pns_call_id);
		ip_ct_gre_keymap_change(oldexp->proto.gre.keymap_reply, &t);

		break;
	case PPTP_IN_CALL_CONNECT:
		pcid = &pptpReq.iccon->peersCallID;
		if (!oldexp)
			break;
		old_dst_ip = oldexp->tuple.dst.ip;
		t = oldexp->tuple;

		/* alter expectation, no need for callID */
		if (t.dst.ip == ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple.dst.ip) {
			/* expectation for PNS->PAC direction */
			t.src.ip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
		} else {
			/* expectation for PAC->PNS direction */
			t.dst.ip = ct->tuplehash[IP_CT_DIR_REPLY].tuple.dst.ip;
		}

		if (!ip_conntrack_change_expect(oldexp, &t)) {
			DEBUGP("successfully changed expect\n");
		} else {
			DEBUGP("can't change expect\n");
		}
		break;
	case PPTP_IN_CALL_REQUEST:
		/* only need to nat in case PAC is behind NAT box */
		break;
	case PPTP_WAN_ERROR_NOTIFY:
		pcid = &pptpReq.wanerr->peersCallID;
		break;
	case PPTP_SET_LINK_INFO:
		pcid = &pptpReq.setlink->peersCallID;
		break;
	default:
		DEBUGP("unknown inbound packet %s\n",
			(msg <= PPTP_MSG_MAX)? strMName[msg]:strMName[0]);
		/* fall through */

	case PPTP_START_SESSION_REQUEST:
	case PPTP_START_SESSION_REPLY:
	case PPTP_STOP_SESSION_REQUEST:
	case PPTP_ECHO_REQUEST:
	case PPTP_ECHO_REPLY:
		/* no need to alter packet */
		DEBUGP("inbound control message %s\n", strMName[msg]);
		DEBUGP("ct->pac_call_id = %d\n", ct_pptp_info->pac_call_id);
		DEBUGP("ct->pns_call_id = %d\n", ct_pptp_info->pns_call_id);
		DEBUGP("nat->pac_call_id = %d\n", nat_pptp_info->pac_call_id);
		DEBUGP("nat->pns_call_id = %d\n", nat_pptp_info->pns_call_id);
		return NF_ACCEPT;
	}

	/* mangle packet */
	IP_NF_ASSERT(pcid);
	DEBUGP("altering peer call id from 0x%04x to 0x%04x\n",
		ntohs(*pcid), ntohs(new_pcid));
	tcph->check = ip_nat_cheat_check(*pcid^0xFFFF, 
					 new_pcid, tcph->check);
	*pcid = new_pcid;

	if (new_cid) {
		IP_NF_ASSERT(cid);
		DEBUGP("altering call id from 0x%04x to 0x%04x\n",
			ntohs(*cid), ntohs(new_cid));
		tcph->check = ip_nat_cheat_check(*cid^0xFFFF,
						new_cid, tcph->check);
		*cid = new_cid;
	}

	/* great, at least we don't need to resize packets */
		DEBUGP("inbound control message %s\n", strMName[msg]);
		DEBUGP("ct->pac_call_id = %d\n", ct_pptp_info->pac_call_id);
		DEBUGP("ct->pns_call_id = %d\n", ct_pptp_info->pns_call_id);
		DEBUGP("nat->pac_call_id = %d\n", nat_pptp_info->pac_call_id);
		DEBUGP("nat->pns_call_id = %d\n", nat_pptp_info->pns_call_id);
	return NF_ACCEPT;
}


static unsigned int tcp_help(struct ip_conntrack *ct,
			     struct ip_conntrack_expect *exp,
			     struct ip_nat_info *info,
			     enum ip_conntrack_info ctinfo,
			     unsigned int hooknum, struct sk_buff **pskb)
{
	struct iphdr *iph = (*pskb)->nh.iph;
	struct tcphdr *tcph = (void *) iph + iph->ihl*4;
	unsigned int datalen = (*pskb)->len - iph->ihl*4 - tcph->doff*4;
	struct pptp_pkt_hdr *pptph;

	int dir;

	DEBUGP("entering\n");

	/* Only mangle things once: original direction in POST_ROUTING
	   and reply direction on PRE_ROUTING. */
	dir = CTINFO2DIR(ctinfo);
	if (!((HOOK2MANIP(hooknum) == IP_NAT_MANIP_SRC && dir == IP_CT_DIR_ORIGINAL)
		|| (HOOK2MANIP(hooknum) == IP_NAT_MANIP_DST && dir == IP_CT_DIR_REPLY)))
	{
		DEBUGP("Not touching dir %s at hook %s\n",
		       dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
		       hooknum == NF_IP_POST_ROUTING ? "POSTROUTING"
		       : hooknum == NF_IP_PRE_ROUTING ? "PREROUTING"
		       : hooknum == NF_IP_LOCAL_OUT ? "OUTPUT"
		       : hooknum == NF_IP_LOCAL_IN ? "INPUT" : "???");
		return NF_ACCEPT;
	}

	/* if packet is too small, just skip it */
	if (datalen < sizeof(struct pptp_pkt_hdr)+
		      sizeof(struct PptpControlHeader)) {
		DEBUGP("pptp packet too short\n");
		return NF_ACCEPT;	
	}


	pptph = (struct pptp_pkt_hdr *) ((void *)tcph + tcph->doff*4);

	/* if it's not a control message, we can't handle it */
	if (ntohs(pptph->packetType) != PPTP_PACKET_CONTROL ||
		ntohl(pptph->magicCookie) != PPTP_MAGIC_COOKIE)
	{
		DEBUGP("not a pptp control packet\n");
		return NF_ACCEPT;
	}

	LOCK_BH(&ip_pptp_lock);

	if (dir == IP_CT_DIR_ORIGINAL) {
		/* reuqests sent by client to server (PNS->PAC) */
		pptp_outbound_pkt(tcph, pptph, datalen, ct, ctinfo, exp);
	} else {
		/* response from the server to the client (PAC->PNS) */
		pptp_inbound_pkt(tcph, pptph, datalen, ct, ctinfo, exp);
	}

	UNLOCK_BH(&ip_pptp_lock);

	return NF_ACCEPT;
}

/* nat helper struct for control connection */
static struct ip_nat_helper pptp_tcp_helper = { 
	{ NULL, NULL },
	"pptp", IP_NAT_HELPER_F_ALWAYS, THIS_MODULE,
	{ { 0, { tcp: { port: __constant_htons(PPTP_CONTROL_PORT) } } },
	  { 0, { 0 }, IPPROTO_TCP } },
	{ { 0, { tcp: { port: 0xFFFF } } },
	  { 0, { 0 }, 0xFFFF } },
	tcp_help, pptp_nat_expected };

			  
static int __init init(void)
{
	DEBUGP("init_module\n" );

        if (ip_nat_helper_register(&pptp_tcp_helper))
		return -EIO;

        return 0;
}

static void __exit fini(void)
{
	DEBUGP("cleanup_module\n" );
        ip_nat_helper_unregister(&pptp_tcp_helper);
}

module_init(init);
module_exit(fini);
