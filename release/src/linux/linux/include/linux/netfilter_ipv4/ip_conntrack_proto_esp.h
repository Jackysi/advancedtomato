#ifndef _CONNTRACK_PROTO_ESP_H
#define _CONNTRACK_PROTO_ESP_H
#include <asm/byteorder.h>

/* ESP PROTOCOL HEADER */

/* ESP Variate field */
#define ESP_PROTOCOL	(0x32)

/* ESP is a mess*/
struct esp_hdr {
	__u32	spi;
	__u32	seq;
};


/* this is part of ip_conntrack */
struct ip_ct_esp {
	unsigned int stream_timeout;
	unsigned int timeout;
};

/* this is part of ip_conntrack_expect */
struct ip_ct_esp_expect {
	struct ip_ct_esp_spi *spi_orig, *spi_reply;
};

#ifdef __KERNEL__
struct ip_conntrack_expect;


/* conntrack private data */
struct ip_ct_esp_master
{
	unsigned int sip;
	unsigned int dip;
	unsigned int spi;
	unsigned int dir;
};

/* conntrack isakmp data */
struct isakmp_hdr
{
	unsigned int init_cookie[2];
	unsigned int resp_cookie[2];
	unsigned int next_payload :8,
			    maj_ver :4,
			    min_ver   :4,
			    exchang_type:8,
			    flag:8;
	unsigned int msg_id;
	unsigned int len;
};

/* structure for original ip <-> reply spi to find correct ip */
struct ip_ct_esp_spi 
{
	struct list_head list;
	struct timer_list timeout;	/* Timer for list destroying */
	//struct ip_ct_esp_master esp_master;
	struct ip_conntrack_tuple esp_master;
};

unsigned int esp_packet_in(const struct iphdr *iph, u_int32_t *sip, u_int32_t *dip);
unsigned int esp_packet_out(const struct iphdr *iph, unsigned int len,enum ip_conntrack_info ctinfo );

#endif /* __KERNEL__ */

#endif /* _CONNTRACK_PROTO_ESP_H */

