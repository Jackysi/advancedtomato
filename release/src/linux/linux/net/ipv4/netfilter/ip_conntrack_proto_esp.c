/*
 * ip_conntrack_proto_esp.c - Version 0.1
 *
 * Connection tracking protocol helper module for ESP.
 *
 * ESP is a generic encapsulation protocol, which is generally not very
 * suited for NAT, as it has no protocol-specific part as port numbers.
 *
 * It has an optional key field, which may help us distinguishing two 
 * connections between the same two hosts.
 *
 * ESP is defined in RFC2406 
 *
 * IPSec is built on top of a modified version of ESP, and has a mandatory
 * field called "SPI", which serves us for the same purpose as the key
 * field in plain ESP.
 *
 * Documentation about IPSEC can be found in RFC 2411
 *
 * (C) 2000-2003 by Harald Welte <laforge@gnumonks.org>
 * (C) 2005-2006 by xiaoqin 1. Multiple IPSec Passthrough. 2. CDRouter test
 *
 * Development of this code funded by Astaro AG (http://www.astaro.com/)
 *
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/timer.h>
#include <linux/netfilter.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/list.h>
#include <linux/netfilter_ipv4/lockhelp.h>

DECLARE_RWLOCK(esp_lock);
#define ASSERT_READ_LOCK(x) MUST_BE_READ_LOCKED(&esp_lock)
#define ASSERT_WRITE_LOCK(x) MUST_BE_WRITE_LOCKED(&esp_lock)

#include <linux/netfilter_ipv4/listhelp.h>
#include <linux/netfilter_ipv4/ip_conntrack_protocol.h>
#include <linux/netfilter_ipv4/ip_conntrack_helper.h>
#include <linux/netfilter_ipv4/ip_conntrack_core.h>
#include <linux/netfilter_ipv4/ip_conntrack_proto_esp.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harald Welte <laforge@gnumonks.org>");
MODULE_DESCRIPTION("netfilter connection tracking protocol helper for ESP");

/* reference from ip_conntrack_proto_gre.c */
#define ESP_TIMEOUT			(300*HZ)
#define ESP_STREAM_TIMEOUT	(300*HZ)

/* ESP KEYMAP HANDLING FUNCTIONS */
static LIST_HEAD(esp_spi_list);
/*keep data */
static u_int32_t last_spi =0;

#if 0
//#define DEBUGP(format, args...) printk(KERN_DEBUG __FILE__ ":" __FUNCTION__": " format, ## args)
#define DEBUGP printk
#define DUMP_TUPLE_ESP(x,str) printk("[%s].\n SRC:(%u.%u.%u.%u):SPI(0x%x) \n DST (%u.%u.%u.%u):SPI(0x%x) lastspi(0x%x)\n", \
			str,\
			NIPQUAD((x)->src.ip), ntohl((x)->src.u.esp.spi), \
			NIPQUAD((x)->dst.ip), ntohl((x)->dst.u.esp.spi),ntohl(last_spi))
#else
#define DEBUGP(x, args...)
#define DUMP_TUPLE_ESP(x,str)
#endif


static void __del_esp(struct ip_ct_esp_spi *trig)
{
	//DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	//DUMP_TUPLE_ESP(&trig->esp_master,"del");

	/* delete from 'trigger_list' */
	WRITE_LOCK(&esp_lock);
	list_del(&trig->list);
	kfree(trig);
	WRITE_UNLOCK(&esp_lock);
}

static void esp_timeout(unsigned long trig)
{
	struct ip_ct_esp_spi *esp_trig = (struct ip_ct_esp_spi *)trig;
	//DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	//DUMP_TUPLE_ESP(&esp_trig->esp_master,"timeout");

	WRITE_LOCK(&esp_lock);
	__del_esp(esp_trig);
	WRITE_UNLOCK(&esp_lock);
}

int esp_packet_matched2(const struct ip_ct_esp_spi* esp,const u_int32_t now_spi,u_int32_t *dir)
{
	u_int32_t ret = 0;
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	DEBUGP("now spi [0x%x] \n",ntohl(now_spi));
	*dir = IP_CT_DIR_MAX;
	
	if (esp->esp_master.src.u.esp.spi == now_spi)
	{
		*dir = IP_CT_DIR_ORIGINAL;
		DUMP_TUPLE_ESP(&esp->esp_master,"match src");
		ret = 1;
	}
	else if (esp->esp_master.dst.u.esp.spi == now_spi)
	{
		*dir = IP_CT_DIR_REPLY;
		DUMP_TUPLE_ESP(&esp->esp_master,"match dst");
		ret = 1;
	}
	else
	{
		DEBUGP("@@@ UnMatch @@@!\n");
	}
	return ret;
}

/*!
	Desc:add a single esp spi entry, associate with specified expect
	param ip_conntrack_expect:
	param ip_conntrack_tuple: (Every entry add tuple struct)
	param reply:
	return 0 is pass / -1 is fail.
*/
int ip_spi_add(struct ip_conntrack_tuple *new_tuple, int direct)
{
	struct ip_ct_esp_spi* esp_new = NULL;
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);

	//add new record for src_spi
	esp_new = kmalloc(sizeof(*esp_new), GFP_ATOMIC);
	if (!esp_new)
	{
		DEBUGP("ESP malloc memory is error.\n");
		return -1;
	}
	
	WRITE_LOCK(&esp_lock);
	/* initializing list head should be sufficient memory */
	memset(esp_new, 0, sizeof(*esp_new));
	memcpy(&(esp_new->esp_master),new_tuple,sizeof(struct ip_conntrack_tuple));
	/* add and start timer if required */
	init_timer(&esp_new->timeout);
	esp_new->timeout.data = (unsigned long)esp_new;
	esp_new->timeout.function = esp_timeout;
	esp_new->timeout.expires = jiffies + (ESP_TIMEOUT * HZ);
	add_timer(&esp_new->timeout);
	
	list_append(&esp_spi_list, esp_new);
	WRITE_UNLOCK(&esp_lock);
	DUMP_TUPLE_ESP(&esp_new->esp_master,"add new tuple");
	return NF_ACCEPT;
}

/*!
	Desc:change the tuple of a ESP spi entry (used by nat helper)
	Param ip_ct_esp_spi: from to ethernet packet
	Param ip_conntrack_tuple: keep old packet data.
	return NONE
*/
void ip_ct_esp_spi_change(struct ip_ct_esp_spi *org_esp,
			     struct ip_conntrack_tuple *new_tuple,int direct)
{
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	DUMP_TUPLE_ESP(new_tuple,"update tuple");

	WRITE_LOCK(&esp_lock);
	/* initializing list head should be sufficient memory */	
	memset(&org_esp->esp_master, 0, sizeof(struct ip_conntrack_tuple));
	memcpy(&org_esp->esp_master, new_tuple, sizeof(org_esp->esp_master));

	WRITE_UNLOCK(&esp_lock);
}

static void esp_refresh(struct ip_ct_esp_spi *trig, unsigned long extra_jiffies)
{
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	IP_NF_ASSERT(trig);
	WRITE_LOCK(&esp_lock);

	/* Need del_timer for race avoidance (may already be dying). */
	if (del_timer(&trig->timeout))
	{
	trig->timeout.expires = jiffies + extra_jiffies;
	add_timer(&trig->timeout);
	}

	WRITE_UNLOCK(&esp_lock);
}


/* look up the source key for a given tuple */
static u_int32_t esp_spi_lookup(struct ip_conntrack_tuple *orig_tuple,const u_int32_t now_spi,u_int32_t* dir)
{
	struct ip_ct_esp_spi *found =NULL, *found2 =NULL;
	struct ip_conntrack_tuple tuple;
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	READ_LOCK(&esp_lock);
	memset(&tuple, 0, sizeof(tuple));
	memcpy(&tuple,orig_tuple,sizeof(struct ip_conntrack_tuple));
	
	found = LIST_FIND(&esp_spi_list, esp_packet_matched2,
			struct ip_ct_esp_spi *,now_spi, dir);
	if (!found)  //not found in original
	{
		if (last_spi == 0 ) //we will keep new src data.
		{
			orig_tuple->src.u.esp.spi = now_spi;
			orig_tuple->dst.u.esp.spi = 0;

			tuple.src.u.esp.spi = now_spi;
			tuple.dst.u.esp.spi = 0;
			DEBUGP("Not found !! The new src spi...\n");
			last_spi = now_spi;
			*dir = IP_CT_DIR_ORIGINAL;
			ip_spi_add(&tuple,*dir);
			goto end;
		}

		//to search is corrent dst spi key
		found2 = LIST_FIND(&esp_spi_list, esp_packet_matched2,
						struct ip_ct_esp_spi *, last_spi, dir);
		if (!found2)
		{
			*dir = IP_CT_DIR_MAX;
			goto fail;
		}
		else
		{
			if(found2->esp_master.dst.ip == orig_tuple->dst.ip)//CDRouter test
			{
				found2->esp_master.dst.u.esp.spi = last_spi;
				esp_refresh(found2, ESP_TIMEOUT * HZ);

				orig_tuple->src.u.esp.spi = now_spi;
				orig_tuple->dst.u.esp.spi = 0;

				tuple.src.u.esp.spi = now_spi;
				tuple.dst.u.esp.spi = 0;
				DEBUGP("Not found !! The new src spi...\n");
				last_spi = now_spi;
				*dir = IP_CT_DIR_ORIGINAL;
				ip_spi_add(&tuple,*dir);
				goto end;
			}
			orig_tuple->src.u.esp.spi = 0;
			orig_tuple->dst.u.esp.spi = last_spi;
			/*update dst spi for first*/
			found2->esp_master.dst.u.esp.spi = now_spi;
			//found2->esp_master.src.u.esp.spi = last_spi;
			esp_refresh(found2, ESP_TIMEOUT * HZ);

			DEBUGP("found last_spi!! The new dst spi...\n");
		#if 0
			tuple.src.ip = found2->esp_master.src.ip;
			tuple.dst.ip = found2->esp_master.dst.ip;
			tuple.src.u.esp.spi = last_spi;
			tuple.dst.u.esp.spi = now_spi;
			*dir = IP_CT_DIR_REPLY;
			
			DEBUGP("found last_spi!! The new dst spi...\n");
			ip_spi_add(&tuple,*dir);
		#endif
			last_spi = 0;
			goto end;
		}
	}
	else //we guess it is origial will refresh data.
	{
		if(last_spi == now_spi && found->esp_master.src.ip != orig_tuple->src.ip)//CDRouter test
		{
			orig_tuple->src.u.esp.spi = 0;
			orig_tuple->dst.u.esp.spi = last_spi;
			/*update dst spi for first*/
			found->esp_master.dst.u.esp.spi = now_spi;
			//found->esp_master.src.u.esp.spi = last_spi;

			last_spi = 0;
		}
		else
		{
			if (*dir == IP_CT_DIR_ORIGINAL)
			{
				orig_tuple->src.u.esp.spi = found->esp_master.src.u.esp.spi;
				orig_tuple->dst.u.esp.spi = found->esp_master.dst.u.esp.spi;
			}
			else
			{
				orig_tuple->src.u.esp.spi = found->esp_master.dst.u.esp.spi;
				orig_tuple->dst.u.esp.spi = found->esp_master.src.u.esp.spi;
			}
		}
		esp_refresh(found, ESP_TIMEOUT * HZ);
		DEBUGP("found!!dir [%s]...\n"
					" sip[%u.%u.%u.%u] dip[%u.%u.%u.%u]\n",
					*dir == IP_CT_DIR_ORIGINAL ? "ORIG" : "REPLY",
					NIPQUAD(found->esp_master.src.ip),
					NIPQUAD(found->esp_master.dst.ip));
			goto end;
	}
	fail:
		DEBUGP("Invalid packet.\n");
	end:
		READ_UNLOCK(&esp_lock);
		return NF_ACCEPT;
}

unsigned int
esp_packet_in(const struct iphdr *iph, u_int32_t *sip, u_int32_t *dip)
{
	struct ip_ct_esp_spi *found;
	struct esp_hdr *esp_h = (void *)iph + iph->ihl*4;	/* Might be TCP, UDP */
	int dir = 0;
	
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);

	READ_LOCK(&esp_lock);

	*sip =0;
	*dip =0;
	/* Check if the trigger-ed range has already existed in 'trigger_list'. */
	found = LIST_FIND(&esp_spi_list, esp_packet_matched2,
			struct ip_ct_esp_spi *,esp_h->spi, &dir);

	if (found)
	{
		/* Yeah, it exists. We need to update(delay) the destroying timer. */
		*sip = found->esp_master.src.ip;
		*dip = found->esp_master.dst.ip;
		/* Accept it, or the imcoming packet could be dropped in the FORWARD chain */
		DEBUGP("Found it !! sip[%u.%u.%u.%u]dip[%u.%u.%u.%u]\n",
				NIPQUAD(*sip),NIPQUAD(*dip));

		esp_refresh(found, ESP_TIMEOUT * HZ);
		READ_UNLOCK(&esp_lock);
		return NF_ACCEPT;
	}
	
	/* Our job is the interception. Not Match.*/
	DEBUGP("##--Not Found...Fail--##\n");
	READ_UNLOCK(&esp_lock);
	
	return NF_DROP;
}


/*!
	Desc:update & checking esphdr info to tuple fill to dest spi number.
	        This is packet first entry.Here packet form to LAN or WAN Packet
	Param datah: esp_hdr 
	param datalen:esp_hdr size.
	param tuple: form to Lan or Wan packet 
	return only return 1;
*/
static int esp_pkt_to_tuple(const void *datah, size_t datalen,
			    struct ip_conntrack_tuple *tuple)
{
	//struct iphdr *iph = datah -20;
	struct esp_hdr *esph = (struct esp_hdr *) datah;
	u_int32_t now_spi = 0, dir = 0;
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);

	//READ_LOCK(&esp_lock);	
	now_spi = esp_spi_lookup(tuple, esph->spi, &dir);
	DUMP_TUPLE_ESP(tuple,"pkt2tuple");
	//READ_UNLOCK(&esp_lock);
	return 1;
}


/*!
	Desc:PUBLIC CONNTRACK PROTO HELPER FUNCTIONS 
	        invert esp part of tuple
	param tuple: linking-list data
	param orig: old data
	return only to 1
	@.@ why to do it. (because direction ??)
*/
static int esp_invert_tuple(struct ip_conntrack_tuple *tuple,
			    const struct ip_conntrack_tuple *orig)
{
	//DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	tuple->src.u.esp.spi = orig->dst.u.esp.spi;
	tuple->dst.u.esp.spi = orig->src.u.esp.spi;
	
	//DUMP_TUPLE_ESP(tuple,"invert tuple");
	return 1;
}


/*Desc:print esp part of tuple 
    Param buffer:
    param tuple
    return 
*/
static unsigned int esp_print_tuple(char *buffer,
				    const struct ip_conntrack_tuple *tuple)
{
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	
	return sprintf(buffer, "[esp_print_tuple] \n"
						"sip=[%d.%d.%d.%d] dip=[%d.%d.%d.%d] \n"
						"src_spi[0x%x]  dst now [0x%x] last[0x%x].\n", 
						NIPQUAD(tuple->src.ip),
						NIPQUAD(tuple->dst.ip),
			 			ntohl(tuple->src.u.esp.spi),
			 			ntohl(tuple->dst.u.esp.spi),
			 			ntohl(last_spi));
}

/* print private data for conntrack */
static unsigned int esp_print_conntrack(char *buffer,
					const struct ip_conntrack *ct)
{
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);
	
	return sprintf(buffer, "ESP timeout=%u, stream_timeout=%u ",
		       (ESP_TIMEOUT / HZ),//(ct->proto.esp.timeout / HZ),
		       (ESP_STREAM_TIMEOUT / HZ));//(ct->proto.esp.stream_timeout / HZ));
}

/* Returns verdict for packet, and may modify conntrack 
     Here is reference gre module
     iphdr: new packet
     ip_conntrack: old packet
     ip_conntrack_info: is this packet direction
*/
static int esp_packet(struct ip_conntrack *ct,
		      struct iphdr *iph, size_t len,
		      enum ip_conntrack_info conntrackinfo)
{
	/* If we've seen traffic both ways, this is a ESP connection.
	 * Extend timeout. */
	if (ct->status & IPS_SEEN_REPLY) {
		//ip_ct_refresh(ct, ct->proto.esp.stream_timeout);
		ip_ct_refresh(ct, ESP_STREAM_TIMEOUT);
		/* Also, more likely to be important, and not a probe. */
		set_bit(IPS_ASSURED_BIT, &ct->status);
	} else
		//ip_ct_refresh(ct, ct->proto.esp.timeout);
		ip_ct_refresh(ct, ESP_TIMEOUT);
	
	return NF_ACCEPT;
}


/*Desc:Called when a new connection for this protocol found. 
    Param ct:
    Param iph:
    Param len
    Return only 1;
*/
static int esp_new(struct ip_conntrack *ct,
		   struct iphdr *iph, size_t len)
{
#if 0
	struct esp_hdr *esph = (struct esp_hdr *)((u_int32_t *)iph + iph->ihl);
	DEBUGP("\n@@@@ %s @@@@\n", __FUNCTION__);	
	DUMP_TUPLE_ESP(&ct->tuplehash[IP_CT_DIR_ORIGINAL].tuple,"new ct->tuplehash[orig]");
	DUMP_TUPLE_ESP(&ct->tuplehash[IP_CT_DIR_REPLY].tuple,"new ct->tuplehash[reply]");
	DEBUGP("esp_hdr sip = [%u.%u.%u.%u] spi[0x%x] seq[%u]\n"
		        "dip=[%u.%u.%u.%u] \n",
		        NIPQUAD(iph->saddr),
		        ntohl(esph->spi),
		        ntohl(esph->seq),
		        NIPQUAD(iph->daddr));
	/* initialize to sane value.  Ideally a conntrack helper	 * (e.g. in case of pptp) is increasing them */
	ct->proto.esp.stream_timeout = ESP_STREAM_TIMEOUT;	
	ct->proto.esp.timeout = ESP_TIMEOUT;	
#endif
	return NF_ACCEPT;
}


/* Called when a conntrack entry has already been removed from the hashes
 * and is about to be deleted from memory */
static void esp_destroy(struct ip_conntrack *ct)
{
	return;
}

/* protocol helper struct */
static struct ip_conntrack_protocol esp_proto
		= {
					{ NULL, NULL },
					ESP_PROTOCOL,
					"esp", 
					esp_pkt_to_tuple,
					esp_invert_tuple,
					esp_print_tuple,
					esp_print_conntrack,
					esp_packet,
					esp_new,
					esp_destroy,
					NULL,
					THIS_MODULE
		};

/* ip_conntrack_proto_esp initialization */
static int __init init(void)
{
	int retcode;

	if ((retcode = ip_conntrack_protocol_register(&esp_proto))) {
		printk(KERN_ERR "Unable to register conntrack protocol "
			        "helper for esp: %d\n", retcode);
		return -EIO;
	}
	DEBUGP("Register conntrack protocol helper for ESP...\n");
	last_spi =0;
	return 0;
}

static void __exit fini(void)
{
	struct list_head *pos, *n;
	DEBUGP("ESP fini() entry free register.\n");

	/* delete all keymap entries */
	WRITE_LOCK(&esp_lock);
	list_for_each_safe(pos, n, &esp_spi_list) {
		DEBUGP("deleting spi %p at module unload time\n", pos);
		list_del(pos);
		kfree(pos);
	}
	WRITE_UNLOCK(&esp_lock);
	ip_conntrack_protocol_unregister(&esp_proto); 
}

module_init(init);
module_exit(fini);
