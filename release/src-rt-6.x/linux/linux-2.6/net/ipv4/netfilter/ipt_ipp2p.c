#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif
#include <linux/module.h>
#include <linux/version.h>
#include <net/netfilter/nf_conntrack_core.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21)
#	include <linux/netfilter/x_tables.h>
#	define ipt_register_match xt_register_match
#	define ipt_unregister_match xt_unregister_match
#	define ipt_match xt_match
#else
#	include <linux/netfilter_ipv4/ip_tables.h>
//#include <linux/netfilter_ipv4/ipt_ipp2p.h>
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21) */

#include <linux/netfilter_ipv4/ipt_ipp2p.h>
#include <net/tcp.h>
#include <net/udp.h>

#define get_u8(X,O)  (*(__u8 *)(X + O))
#define get_u16(X,O)  (*(__u16 *)(X + O))
#define get_u32(X,O)  (*(__u32 *)(X + O))
#define TOTAL_PACKETS conntrack->counters[IP_CT_DIR_ORIGINAL].packets + \
		      conntrack->counters[IP_CT_DIR_REPLY].packets

MODULE_AUTHOR("Lei Liu <liulei@syiae.edu.cn>");
MODULE_DESCRIPTION("An extension to iptables to identify P2P traffic. Base on ipp2p.org");
MODULE_LICENSE("GPL");

static int match_packets = 15;
module_param(match_packets, int, 0);
MODULE_PARM_DESC(match_packets, "Numbers of packets for per conntrack to match");

/*Search for UDP eDonkey/eMule/Kad commands*/
int
udp_search_edk (unsigned char *t, int packet_len)
{
	/* Vagaa */
	if(packet_len == 4 && get_u32(t,0) == __constant_htonl(0xff0a0000)) return (IPP2P_EDK * 100 + 42);
	if(packet_len == 8 && get_u16(t,0) == __constant_htons(0xff0a) && get_u16(t,4) == __constant_htons(0x0c02)) return (IPP2P_EDK * 100 + 47);
	if(memcmp(t+12,"POST / HTTP/1.1",15) == 0 && memcmp(t+29,"Host: vagaa.com",15) == 0 && memcmp(t+46,"VAGAA-OPERATION:",16) == 0) return (IPP2P_EDK * 100 + 48);
	switch (*t) {
		case 0xf1:
		{	if ( get_u16(t,3) == __constant_htons(0x0000) )
			{
				/* Search Result */
				if(packet_len == 22 && *(t+1) == 0x11) return (IPP2P_EDK * 100 + 40);
				if(packet_len == 32 && *(t+1) == 0x00) return (IPP2P_EDK * 100 + 41);
				/* Identify Reply */
				if(packet_len == 26 && *(t+1) == 0x15) return (IPP2P_EDK * 100 + 43);
				/* Identify Ack */
				if(packet_len == 27 && *(t+1) == 0x16) return (IPP2P_EDK * 100 + 44);
				if(packet_len == 6 && *(t+1) == 0x01) return (IPP2P_EDK * 100 + 45);
				if(packet_len == 144 && *(t+1) == 0x8b) return (IPP2P_EDK * 100 + 46);
			}
			break;
		}
		case 0xe3:
		{	/*edonkey*/
			switch (*(t+1))
			{
				/* client -> server status request */
				case 0x96:
					if (packet_len > 5) return ((IPP2P_EDK * 100) + 50);
					break;
				/* server -> client status request */
				case 0x97: if (packet_len > 33) return ((IPP2P_EDK * 100) + 51);
					break;
						/* server description request */
						/* e3 2a ff f0 .. | size == 6 */
				case 0xa2: if ( (packet_len > 5) && ( get_u16(t,2) == __constant_htons(0xfff0) ) ) return ((IPP2P_EDK * 100) + 52);
					break;
						/* server description response */
						/* e3 a3 ff f0 ..  | size > 40 && size < 200 */
				//case 0xa3: return ((IPP2P_EDK * 100) + 53);
				//	break;
				case 0x9a: if (packet_len > 17) return ((IPP2P_EDK * 100) + 54);
					break;
				case 0x9b: if (packet_len > 24) return ((IPP2P_EDK * 100) + 56);
					break;
				case 0x92: if (packet_len > 9) return ((IPP2P_EDK * 100) + 55);
					break;
			}
			break;
		}
		case 0xe4:
		{
			switch (*(t+1))
			{
				case 0x01: if (packet_len == 2) return ((IPP2P_EDK * 100) + 70);
					break;
					/* e4 19 .. Firewall Connection ACK */
				case 0x19: if (packet_len > 21 ) return ((IPP2P_EDK * 100) + 71);
					break;
					/* e4 20 .. | size == 43 */
				case 0x20: if ((packet_len > 33) && (*(t+2) != 0x00) && (*(t+33) != 0x00)) return ((IPP2P_EDK * 100) + 60);
					break;
				case 0x21: if ((packet_len > 33) && (*(t+2) != 0x00) && (*(t+33) != 0x00)) return ((IPP2P_EDK * 100) + 60);
					break;
					/* e4 00 .. 00 | size == 35 ? */
				case 0x00: if ((packet_len > 26) && (*(t+26) == 0x00)) return ((IPP2P_EDK * 100) + 61);
					break;
					/* e4 10 .. 00 | size == 35 ? Search Info */
				case 0x10: if ((packet_len > 26) && (*(t+26) == 0x00)) return ((IPP2P_EDK * 100) + 62);
					break;
					/* e4 11 .. Search Result */
				case 0x11: if ((packet_len > 26) && (*(t+26) == 0x00)) return ((IPP2P_EDK * 100) + 62);
					break;
					/* e4 18 .. 00 | size == 35 ? */
				case 0x18: if ((packet_len > 26) && (*(t+26) == 0x00)) return ((IPP2P_EDK * 100) + 63);
					break;
					/* e4 52 .. | size = 44 */
				case 0x52: if (packet_len > 35 ) return ((IPP2P_EDK * 100) + 64);
					break;
					/* e4 58 .. | size == 6 */
				case 0x58: if (packet_len > 5 ) return ((IPP2P_EDK * 100) + 65);
					break;
					/* e4 59 .. | size == 2 */
				case 0x59: if (packet_len > 1 )return ((IPP2P_EDK * 100) + 66);
					break;
					/* e4 28 .. | packet_len == 52,77,102,127... */
				case 0x28: if (((packet_len-44) % 25) == 0) return ((IPP2P_EDK * 100) + 67);
					break;
				case 0x29: if (((packet_len-44) % 25) == 0) return ((IPP2P_EDK * 100) + 67);
					break;
					/* e4 50 xx xx | size == 4 */
				case 0x50: if (packet_len > 3) return ((IPP2P_EDK * 100) + 68);
					break;
					/* e4 40 xx xx | size == 48 */
				case 0x40: if (packet_len > 47) return ((IPP2P_EDK * 100) + 69);
					break;
			}
			break;
		}
	} /* end of switch (*t) */
    return 0;
}/*udp_search_edk*/


/*Search for UDP Gnutella commands*/
int
udp_search_gnu (unsigned char *t, int packet_len)
{
    if (memcmp(t, "GND", 3) == 0) return ((IPP2P_GNU * 100) + 51);
    if (memcmp(t, "GNUTELLA ", 9) == 0) return ((IPP2P_GNU * 100) + 52);
    return 0;
}/*udp_search_gnu*/


/*Search for UDP KaZaA commands*/
int
udp_search_kazaa (unsigned char *t, int packet_len)
{
    if (*(t+packet_len-1) == 0x00){
	if (memcmp((t+packet_len-6), "KaZaA", 5) == 0) return (IPP2P_KAZAA * 100 +50);
    }

    return 0;
}/*udp_search_kazaa*/

/*Search for UDP DirectConnect commands*/
int
udp_search_directconnect (unsigned char *t, int packet_len)
{
    if ((*t == 0x24) && (*(t+packet_len-1) == 0x7c)) {
    	if (memcmp(t, "SR ", 3) == 0)	return ((IPP2P_DC * 100) + 60);
    	if (memcmp(t, "Ping ", 5) == 0)	return ((IPP2P_DC * 100) + 61);
    }
    return 0;
}/*udp_search_directconnect*/



/*Search for UDP BitTorrent commands*/
int
udp_search_bit (unsigned char *t, int packet_len)
{
	switch(packet_len)
	{
		case 16:
			/* ^ 00 00 04 17 27 10 19 80 */
			if ((ntohl(get_u32(t, 0)) == 0x00000417) && (ntohl(get_u32(t, 4)) == 0x27101980))
				return (IPP2P_BIT * 100 + 50);
			break;
		case 36:
			if (get_u32(t, 8) == __constant_htonl(0x00000400) && get_u32(t, 28) == __constant_htonl(0x00000104))
				return (IPP2P_BIT * 100 + 51);
			if (get_u32(t, 8) == __constant_htonl(0x00000400))
				return (IPP2P_BIT * 100 + 61);
			break;
		case 57:
			if (get_u32(t, 8) == __constant_htonl(0x00000404) && get_u32(t, 28) == __constant_htonl(0x00000104))
				return (IPP2P_BIT * 100 + 52);
			if (get_u32(t, 8) == __constant_htonl(0x00000404))
				return (IPP2P_BIT * 100 + 62);
			break;
		case 59:
			if (get_u32(t, 8) == __constant_htonl(0x00000406) && get_u32(t, 28) == __constant_htonl(0x00000104))
				return (IPP2P_BIT * 100 + 53);
			if (get_u32(t, 8) == __constant_htonl(0x00000406))
				return (IPP2P_BIT * 100 + 63);
			break;
		case 203:
			if (get_u32(t, 0) == __constant_htonl(0x00000405))
				return (IPP2P_BIT * 100 + 54);
			break;
		case 21:
			if ((get_u32(t, 0) == __constant_htonl(0x00000401)))
				return (IPP2P_BIT * 100 + 55);
			break;
		case 44:
			if (get_u32(t,0)  == __constant_htonl(0x00000827) &&
			get_u32(t,4) == __constant_htonl(0x37502950))
				return (IPP2P_BIT * 100 + 80);
			break;
		default:
			/* this packet does not have a constant size */
			if (packet_len >= 32 && get_u32(t, 8) == __constant_htonl(0x00000402) && get_u32(t, 28) == __constant_htonl(0x00000104))
				return (IPP2P_BIT * 100 + 56);
			break;
	}

	/* some extra-bitcomet rules:
	* "d1:" [a|r] "d2:id20:"
	*/
	if (packet_len > 22 && get_u8(t, 0) == 'd' && get_u8(t, 1) == '1' && get_u8(t, 2) == ':' )
	{
		if (get_u8(t, 3) == 'a' || get_u8(t, 3) == 'r')
		{
			if (memcmp(t+4,"d2:id20:",8)==0)
				return (IPP2P_BIT * 100 + 57);
		}
	}

#if 0
	/* bitlord rules */
	/* packetlen must be bigger than 40 */
	/* first 4 bytes are zero */
	if (packet_len > 40 && get_u32(t, 8) == 0x00000000)
	{
		/* first rule: 00 00 00 00 01 00 00 xx xx xx xx 00 00 00 00*/
		if (get_u32(t, 12) == 0x00000000 &&
		    get_u32(t, 16) == 0x00010000 &&
		    get_u32(t, 24) == 0x00000000 )
			return (IPP2P_BIT * 100 + 71);

		/* 00 01 00 00 0d 00 00 xx xx xx xx 00 00 00 00*/
		if (get_u32(t, 12) == 0x00000001 &&
		    get_u32(t, 16) == 0x000d0000 &&
		    get_u32(t, 24) == 0x00000000 )
			return (IPP2P_BIT * 100 + 71);


	}
#endif

    return 0;
}/*udp_search_bit*/



/*Search for Ares commands*/
//#define IPP2P_DEBUG_ARES
int
search_ares (const unsigned char *payload, const u16 plen)
//int search_ares (unsigned char *haystack, int packet_len, int head_len)
{
//	const unsigned char *t = haystack + head_len;

	/* all ares packets start with  */
	if (*(payload+1) == 0 && (plen - *payload) == 3)
	{
		switch (*(payload+2))
		{
			case 0x5a:
				/* ares connect */
				if ( plen == 6 && *(payload+5) == 0x05 ) return ((IPP2P_ARES * 100) + 1);
				break;
			case 0x09:
				/* ares search, min 3 chars --> 14 bytes
				 * lets define a search can be up to 30 chars --> max 34 bytes
				 */
				if ( plen >= 14 && plen <= 34 ) return ((IPP2P_ARES * 100) + 1);
				break;
#ifdef IPP2P_DEBUG_ARES
			default:
			printk(KERN_DEBUG "Unknown Ares command %x recognized, len: %u \n", (unsigned int) *(payload+2),plen);
#endif /* IPP2P_DEBUG_ARES */
		}
	}

#if 0
	/* found connect packet: 03 00 5a 04 03 05 */
	/* new version ares 1.8: 03 00 5a xx xx 05 */
    if ((plen) == 6){	/* possible connect command*/
	if ((payload[0] == 0x03) && (payload[1] == 0x00) && (payload[2] == 0x5a) && (payload[5] == 0x05))
	    return ((IPP2P_ARES * 100) + 1);
    }
    if ((plen) == 60){	/* possible download command*/
	if ((payload[59] == 0x0a) && (payload[58] == 0x0a)){
	    if (memcmp(t, "PUSH SHA1:", 10) == 0) /* found download command */
	    	return ((IPP2P_ARES * 100) + 2);
	}
    }
#endif

    return 0;
} /*search_ares*/

/*Search for SoulSeek commands*/
int
search_soul (const unsigned char *payload, const u16 plen)
{
//#define IPP2P_DEBUG_SOUL
    /* match: xx xx xx xx | xx = sizeof(payload) - 4 */
    if (get_u32(payload, 0) == (plen - 4)){
	const __u32 m = get_u32(payload, 4);
	/* match 00 yy yy 00, yy can be everything */
        if ( get_u8(payload, 4) == 0x00 && get_u8(payload, 7) == 0x00 )
	{
#ifdef IPP2P_DEBUG_SOUL
	printk(KERN_DEBUG "0: Soulseek command 0x%x recognized\n",get_u32(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 1);
	}

        /* next match: 01 yy 00 00 | yy can be everything */
        if ( get_u8(payload, 4) == 0x01 && get_u16(payload, 6) == 0x0000 )
	{
#ifdef IPP2P_DEBUG_SOUL
	printk(KERN_DEBUG "1: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 2);
	}

	/* other soulseek commandos are: 1-5,7,9,13-18,22,23,26,28,35-37,40-46,50,51,60,62-69,91,92,1001 */
	/* try to do this in an intelligent way */
	/* get all small commandos */
	switch(m)
	{
		case 7:
		case 9:
		case 22:
		case 23:
		case 26:
		case 28:
		case 50:
		case 51:
		case 60:
		case 91:
		case 92:
		case 1001:
#ifdef IPP2P_DEBUG_SOUL
		printk(KERN_DEBUG "2: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 3);
	}

	if (m > 0 && m < 6 )
	{
#ifdef IPP2P_DEBUG_SOUL
		printk(KERN_DEBUG "3: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 4);
	}
	if (m > 12 && m < 19 )
	{
#ifdef IPP2P_DEBUG_SOUL
		printk(KERN_DEBUG "4: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 5);
	}

	if (m > 34 && m < 38 )
	{
#ifdef IPP2P_DEBUG_SOUL
		printk(KERN_DEBUG "5: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 6);
	}

	if (m > 39 && m < 47 )
	{
#ifdef IPP2P_DEBUG_SOUL
		printk(KERN_DEBUG "6: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 7);
	}

	if (m > 61 && m < 70 )
	{
#ifdef IPP2P_DEBUG_SOUL
		printk(KERN_DEBUG "7: Soulseek command 0x%x recognized\n",get_u16(payload, 4));
#endif /* IPP2P_DEBUG_SOUL */
		return ((IPP2P_SOUL * 100) + 8);
	}

#ifdef IPP2P_DEBUG_SOUL
	printk(KERN_DEBUG "unknown SOULSEEK command: 0x%x, first 16 bit: 0x%x, first 8 bit: 0x%x ,soulseek ???\n",get_u32(payload, 4),get_u16(payload, 4) >> 16,get_u8(payload, 4) >> 24);
#endif /* IPP2P_DEBUG_SOUL */
    }

	/* match 14 00 00 00 01 yy 00 00 00 STRING(YY) 01 00 00 00 00 46|50 00 00 00 00 */
	/* without size at the beginning !!! */
	if ( get_u32(payload, 0) == 0x14 && get_u8(payload, 4) == 0x01 )
	{
		__u32 y = get_u32(payload, 5);
		/* we need 19 chars + string */
		if ( (y + 19) <= (plen) )
		{
			const unsigned char *w=payload+9+y;
			if (get_u32(w, 0) == 0x01 && ( get_u16(w, 4) == 0x4600 || get_u16(w, 4) == 0x5000) && get_u32(w, 6) == 0x00);
#ifdef IPP2P_DEBUG_SOUL
	    		printk(KERN_DEBUG "Soulssek special client command recognized\n");
#endif /* IPP2P_DEBUG_SOUL */
	    		return ((IPP2P_SOUL * 100) + 9);
		}
	}
    return 0;
}


/*Search for WinMX commands*/
int
search_winmx (const unsigned char *payload, const u16 plen)
{
//#define IPP2P_DEBUG_WINMX
    if ((plen == 4) && (memcmp(payload, "SEND", 4) == 0))  return ((IPP2P_WINMX * 100) + 1);
    if ((plen == 3) && (memcmp(payload, "GET", 3) == 0))  return ((IPP2P_WINMX * 100) + 2);
    //if (packet_len < (head_len + 10)) return 0;
    if (plen < 10) return 0;

    if ((memcmp(payload, "SEND", 4) == 0) || (memcmp(payload, "GET", 3) == 0)){
        u16 c = 4;
        const u16 end = plen-2;
        u8 count = 0;
        while (c < end)
        {
        	if (*(payload+c)== 0x20 && *(payload+c+1) == 0x22)
        	{
        		c++;
        		count++;
        		if (count >= 2) return ((IPP2P_WINMX * 100) + 3);
        	}
        	c++;
        }
    }

    if ( plen == 149 && *payload == '8' )
    {
#ifdef IPP2P_DEBUG_WINMX
    	printk(KERN_INFO "maybe WinMX\n");
#endif
    	if (get_u32(payload,17) == 0 && get_u32(payload,21) == 0 && get_u32(payload,25) == 0 &&
//    	    get_u32(payload,33) == __constant_htonl(0x71182b1a) && get_u32(payload,37) == __constant_htonl(0x05050000) &&
//    	    get_u32(payload,133) == __constant_htonl(0x31097edf) && get_u32(payload,145) == __constant_htonl(0xdcb8f792))
    	    get_u16(payload,39) == 0 && get_u16(payload,135) == __constant_htons(0x7edf) && get_u16(payload,147) == __constant_htons(0xf792))

    	{
#ifdef IPP2P_DEBUG_WINMX
    		printk(KERN_INFO "got WinMX\n");
#endif
    		return ((IPP2P_WINMX * 100) + 4);
    	}
    }
    return 0;
} /*search_winmx*/


/*Search for appleJuice commands*/
int
search_apple (const unsigned char *payload, const u16 plen)
{
    if ( (plen > 7) && get_u16(payload,6) == __constant_htons(0x0d0a) && (memcmp(payload, "ajprot", 6) == 0))  return (IPP2P_APPLE * 100);

    return 0;
}


/*Search for BitTorrent commands*/
int
search_bittorrent (const unsigned char *payload, const u16 plen)
{
    if (plen > 20)
    {
	/* test for match 0x13+"BitTorrent protocol" */
	if (*payload == 0x13)
	{
		if (memcmp(payload+1, "BitTorrent protocol", 19) == 0) return (IPP2P_BIT * 100);
	}

	/* get tracker commandos, all starts with GET /
	* then it can follow: scrape| announce
	* and then ?hash_info=
	*/
	if (memcmp(payload,"GET /",5) == 0)
	{
		/* message scrape */
		if ( memcmp(payload+5,"scrape?info_hash=",17)==0 ) return (IPP2P_BIT * 100 + 1);
		/* message announce */
		if ( memcmp(payload+5,"announce",8)==0 ) return (IPP2P_BIT * 100 + 2);
		if ( memcmp(payload+5,"?info_hash=",11)==0 ) return (IPP2P_BIT * 100 + 3);
		if ( memcmp(payload+5,"data?fid=",9)==0 ) return (IPP2P_BIT * 100 + 5);
	}
    }
    else
    {
    	/* bitcomet encryptes the first packet, so we have to detect another
    	 * one later in the flow */
    	 /* first try failed, too many missdetections */
    	//if ( size == 5 && get_u32(t,0) == __constant_htonl(1) && t[4] < 3) return (IPP2P_BIT * 100 + 3);

    	/* second try: block request packets */
    	if ( plen == 17 && get_u32(payload,0) == __constant_htonl(0x0d) && *(payload+4) == 0x06 && get_u32(payload,13) == __constant_htonl(0x4000) ) return (IPP2P_BIT * 100 + 4);
    }

    return 0;
}

/* check for xunlei */
int
search_xunlei (const unsigned char *payload, const u16 plen)
{
	if ( memcmp(payload,"POST / HTTP/1.1",15) ==0 )
	{
		unsigned char *t = strstr((payload+93), "Connection: Keep-Alive"); //min length of Connection
		if (t)
		{
			t += 26;
			if ( (*t < 0x40) && (*(t+1) == 0x00) && get_u16(t,2) == __constant_htons(0x0000) && get_u16(t,5) == __constant_htons(0x0000) && (*(t+7) == 0x00) && (*(t+8) == (payload+plen-t-12)) ) return (IPP2P_XUNLEI *100 + 1);
		}
	}
	//QQ cyclone
	if ( plen < 400 )
	{
		if ( get_u16(payload,0) == __constant_htons(0x0200) && ( *(payload+3) == 0x01 || *(payload+3) == 0x00 ) && *(payload+5) == 0x00 && ( *(payload+6) == 0x01 || *(payload+6) == 0x77 ) && get_u16(payload,21) == __constant_htons(0x0038) ) return (IPP2P_XUNLEI *100 + 2);
	}
	return 0;
}

int
udp_search_xunlei (unsigned char *t, int packet_len)
{
	/* baidu xiaba */
	if (packet_len == 24 && get_u32(t,0) == __constant_htonl(0x01000101) && get_u32(t,4) == __constant_htonl(0xfefffeff) && get_u32(t,8) == __constant_htonl(0x00)) return (IPP2P_XUNLEI * 100 + 11);
	if (packet_len == 38 && get_u32(t,0) == __constant_htonl(0x010011a0) && get_u32(t,4) == __constant_htonl(0xfefffeff) && get_u32(t,8) == __constant_htonl(0x00)) return (IPP2P_XUNLEI * 100 + 12);
	return 0;
}
/* check for PPLive & PPStream */
int
search_pp (const unsigned char *payload, const u16 plen)
{
	/* message pplive */
	if ( memcmp(payload,"GET /zh-cn/xml/default.xml",26)==0 ) return (IPP2P_PP *100 + 1 );
	/* message PPStream */
	if ( memcmp(payload,"PSProtocol",10)==0 ) return (IPP2P_PP * 100 + 2);
	/* message PPLive */
	if (get_u16(payload,0) == __constant_htons(0xe903) && get_u32(payload,4) == __constant_htonl(0x98ab0102)) return (IPP2P_PP * 100 + 3);
	if (get_u16(payload,4) == __constant_htons(0xe903) && get_u32(payload,8) == __constant_htonl(0x98ab0102)) return (IPP2P_PP * 100 + 4);
	/* message UUSee */
	if (((*payload + *(payload+1) * 256) == (plen-4) || ((*payload + *(payload+1) * 256) < (plen-4) && *payload + *(payload+1) * 256 + *(payload + *payload + *(payload+1) * 256 + 4) + *(payload + *payload + *(payload+1) * 256 + 5) * 256 + 8 == plen )) && get_u16(payload,2) == __constant_htons(0x0000)) {
	if (*(payload+18) == 0x68 && *(payload+20) == 0x74 && *(payload+22) == 0x74 && *(payload+24) == 0x70 && *(payload+26)==0x3a && *(payload+28) == 0x2f && *(payload+30) == 0x2f) return 0;
	return (IPP2P_PP * 100 + 5);
	}
	/* QQLive */
	if ( (plen < 150) && (*payload == 0xfe) && (*(payload+1) == *(payload+4)) && get_u16(payload,2) == __constant_htons(0x0000) ) return (IPP2P_PP * 100 + 6);
	/* feidian */
	if ( (plen == 4 && get_u32(payload,0) == __constant_htonl(0x291c3201)) || (plen == 61 && get_u32(payload,0) == __constant_htonl(0x291c3201) && get_u32(payload,4) == __constant_htonl(0x39000000))) return (IPP2P_PP * 100 + 7);
	/* POCO */
	if ( (*payload + *(payload+1)*256) == plen && get_u16(payload,2) == __constant_htons(0x0000) && (*(payload+4) + *(payload+5)) == (plen - 13) && get_u16(payload,6) == __constant_htons(0x0000)) return (IPP2P_PP * 100 + 8);
	/* QVOD */
	if(memcmp((payload + 1),"QVOD protocol",13) == 0) return (IPP2P_PP * 100 + 9);
	return 0;
}

/* check for PPLive & PPStream UDP pkg */
int
udp_search_pp (unsigned char *t, int packet_len)
{
    if (get_u16(t,0) == __constant_htons(0xe903) && get_u32(t,4) == __constant_htonl(0x98ab0102)) return (IPP2P_PP * 100 + 11);
    if ( (memcmp(t+8,"[bsinfo]",8) ==0) && (*(t+16) == 0x0d) && (*(t+17) == 0x0a) && (memcmp(t+18,"mf=",3) ==0) ) return (IPP2P_PP * 100 + 12);
    if ( (packet_len == 22 || packet_len == 8) && get_u16(t,0) == __constant_htons(0x0909) && (*(t+2) == 0x08 || *(t+2) == 0x09) && ((*(t+3) == 0x00) || (*(t+3) == 0x01)) ) return (IPP2P_PP * 100 + 13);
    if ( packet_len == (*t+4) && get_u32(t,1) == __constant_htonl(0x00430000)) return (IPP2P_PP * 100 + 15 );
    /* QQLive */
    if ( (packet_len < 150) && (*t == 0xfe) && (*(t+1) == *(t+4)) && get_u16(t,2) == __constant_htons(0x0000) ) return (IPP2P_PP * 100 + 14);
    /* feidian */
    if ( (packet_len == 112 || packet_len == 116) && get_u32(t,0) == __constant_htonl(0x1c1c3201) && (get_u16(t,4) == __constant_htons(0x0b00) || get_u16(t,4) == __constant_htons(0x0c00))) return (IPP2P_PP * 100 + 16);
    /* POCO */
    if (packet_len == 6 && get_u16(t,0) == __constant_htons(0x8095) && (get_u16(t,2) == __constant_htons(0x0462) || get_u16(t,2) == __constant_htons(0x0565)) ) return (IPP2P_PP * 100 + 17);
    if (packet_len == 22 && get_u16(t,0) == __constant_htons(0x8094) && (get_u16(t,2) == __constant_htons(0x0429) || get_u16(t,4) == __constant_htons(0x0429)) ) return (IPP2P_PP * 100 + 18);
    /* QVOD */
    if (packet_len == ntohl(get_u32(t,0)) && memcmp((t + 14),"QVOD protocol",13) == 0) return (IPP2P_PP * 100 + 19);
    return 0;
}

/*check for gnutella get commands and other typical data*/
int
search_all_gnu (const unsigned char *payload, const u16 plen)
{

    if (get_u16(payload,(plen-2)) == __constant_htons(0x0d0a))
    {

	if (memcmp(payload, "GNUTELLA CONNECT/", 17) == 0) return ((IPP2P_GNU * 100) + 1);
	if (memcmp(payload, "GNUTELLA/", 9) == 0) return ((IPP2P_GNU * 100) + 2);


	if ((memcmp(payload, "GET /get/", 9) == 0) || (memcmp(payload, "GET /uri-res/", 13) == 0))
	{
		u16 c = 8;
		const u16 end = plen-22;
		while (c < end) {
			if ( get_u16(payload,c) == __constant_htons(0x0a0d) && ((memcmp((payload+c+2), "X-Gnutella-", 11) == 0) || (memcmp((payload+c+2), "X-Queue:", 8) == 0)))
				return ((IPP2P_GNU * 100) + 3);
			c++;
		}
	}
    }
    return 0;
}

/*check for KaZaA download commands and other typical data*/
int
search_all_kazaa (const unsigned char *payload, const u16 plen)
{
    if (get_u16(payload,(plen-2)) == __constant_htons(0x0d0a))
    {

	if (memcmp(payload, "GIVE ", 5) == 0) return ((IPP2P_KAZAA * 100) + 1);

    	if (memcmp(payload, "GET /", 5) == 0) {
		u16 c = 8;
		const u16 end=plen-22;
		while (c < end) {
			if ( get_u16(payload,c) == __constant_htons(0x0a0d) && ((memcmp((payload+c+2), "X-Kazaa-Username: ", 18) == 0) || (memcmp((payload+c+2), "User-Agent: PeerEnabler/", 24) == 0)))
				return ((IPP2P_KAZAA * 100) + 2);
			c++;
		}
	}
    }
    return 0;
}

/*intensive but slower search for some edonkey packets including size-check*/
int
search_all_edk (const unsigned char *payload, const u16 plen)
{
    if (memcmp(payload, "POST / HTTP/1.1", 15) == 0 && memcmp(payload+17, "Host: vagaa.com", 15) == 0 && memcmp(payload+34, "VAGAA-OPERATION: ", 17) == 0) return (IPP2P_EDK * 100 + 10);
    if (*payload != 0xe3)
	return 0;
    else {
	//t += head_len;
	const u16 cmd = get_u16(payload, 1);
	if (cmd == (plen - 5)) {
	    switch (*(payload+5)) {
		case 0x01: return ((IPP2P_EDK * 100) + 1);	/*Client: hello or Server:hello*/
		break;
		case 0x47: return ((IPP2P_EDK * 100) + 2);	/*fast check for edonkey file segment transfer command*/
		break;
		case 0x4c: return ((IPP2P_EDK * 100) + 9);	/*Client: Hello-Answer*/
		break;
	    }
	}
	return 0;
     }
}

/*intensive but slower check for all direct connect packets*/
int
search_all_dc (const unsigned char *payload, const u16 plen)
{
//    unsigned char *t = haystack;

    if (*payload == 0x24 && *(payload+plen-1) == 0x7c)
    {
    	const unsigned char *t = payload+1;
    		/* Client-Hub-Protocol */
	if (memcmp(t, "Lock ", 5) == 0)	 		return ((IPP2P_DC * 100) + 1);
	/*fast check for Direct Connect send command*/
	if (memcmp(t, "Send|", 5) == 0)			return ((IPP2P_DC * 100) + 5);
	/* Client-Client-Protocol, some are already recognized by client-hub (like lock) */
	if (memcmp(t, "MyNick ", 7) == 0)	 	return ((IPP2P_DC * 100) + 38);
    }
    return 0;
}

/*check for mute*/
int
search_mute (const unsigned char *payload, const u16 plen)
{
	if ( plen == 209 || plen == 345 || plen == 473 || plen == 609 || plen == 1121 )
	{
		//printk(KERN_DEBUG "size hit: %u",size);
		if (memcmp(payload,"PublicKey: ",11) == 0 )
		{
			return ((IPP2P_MUTE * 100) + 0);

/*			if (memcmp(t+size-14,"\x0aEndPublicKey\x0a",14) == 0)
			{
				printk(KERN_DEBUG "end pubic key hit: %u",size);

			}*/
		}
	}
	return 0;
}


/* check for xdcc */
int
search_xdcc (const unsigned char *payload, const u16 plen)
{
	/* search in small packets only */
	if (plen > 20 && plen < 200 && get_u16(payload,(plen-2)) == __constant_htons(0x0d0a) && memcmp(payload,"PRIVMSG ",8) == 0)
	{

		u16 x=10;
		const u16 end = plen - 13;

		/* is seems to be a irc private massage, chedck for xdcc command */
		while (x < end)
		{
			if (*(payload+x) == ':')
			{
				if ( memcmp((payload+x+1),"xdcc send #",11) == 0 )
					return ((IPP2P_XDCC * 100) + 0);
			}
			x++;
		}
	}
	return 0;
}

/* search for waste */
int search_waste(const unsigned char *payload, const u16 plen)
{
	if ( plen >= 8 && memcmp(payload,"GET.sha1:",9) == 0)
		return ((IPP2P_WASTE * 100) + 0);

	return 0;
}


static struct {
    int command;
    __u8 short_hand;			/*for fucntions included in short hands*/
    int packet_len;
    int (*function_name) (const unsigned char *, const u16);
} matchlist[] = {
    {IPP2P_EDK,SHORT_HAND_IPP2P,20, &search_all_edk},
    {IPP2P_DC,SHORT_HAND_IPP2P,5, &search_all_dc},
    {IPP2P_GNU,SHORT_HAND_IPP2P,5, &search_all_gnu},
    {IPP2P_KAZAA,SHORT_HAND_IPP2P,5, &search_all_kazaa},
    {IPP2P_BIT,SHORT_HAND_IPP2P,20, &search_bittorrent},
    {IPP2P_PP,SHORT_HAND_IPP2P,3, &search_pp},
    {IPP2P_APPLE,SHORT_HAND_IPP2P,5, &search_apple},
    {IPP2P_SOUL,SHORT_HAND_IPP2P,5, &search_soul},
    {IPP2P_WINMX,SHORT_HAND_IPP2P,2, &search_winmx},
    {IPP2P_ARES,SHORT_HAND_IPP2P,5, &search_ares},
    {IPP2P_MUTE,SHORT_HAND_NONE,200, &search_mute},
    {IPP2P_WASTE,SHORT_HAND_NONE,5, &search_waste},
    {IPP2P_XDCC,SHORT_HAND_NONE,5, &search_xdcc},
    {IPP2P_XUNLEI,SHORT_HAND_NONE,100,&search_xunlei},
    {0,0,0,NULL}
};


static struct {
    int command;
    __u8 short_hand;			/*for fucntions included in short hands*/
    int packet_len;
    int (*function_name) (unsigned char *, int);
} udp_list[] = {
    {IPP2P_KAZAA,SHORT_HAND_IPP2P,6, &udp_search_kazaa},
    {IPP2P_BIT,SHORT_HAND_IPP2P,15, &udp_search_bit},
    {IPP2P_PP,SHORT_HAND_IPP2P,3, &udp_search_pp},
    {IPP2P_GNU,SHORT_HAND_IPP2P,3, &udp_search_gnu},
    {IPP2P_EDK,SHORT_HAND_IPP2P,1, &udp_search_edk},
    {IPP2P_DC,SHORT_HAND_IPP2P,4, &udp_search_directconnect},
    {IPP2P_XUNLEI,SHORT_HAND_NONE,5,&udp_search_xunlei},
    {0,0,0,NULL}
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
static bool
#else
static int
#endif
match(const struct sk_buff *skb,
      const struct net_device *in,
      const struct net_device *out,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
      const struct xt_match  *mymatch,
      const void *matchinfo,
      int offset,
      unsigned int myprotoff,
#else
      const void *matchinfo,
      int offset,
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
      const void *hdr,
      u_int16_t datalen,
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
      bool *hotdrop)
#else
      int *hotdrop)
#endif
{
    const struct ipt_p2p_info *info = matchinfo;
    unsigned char  *haystack;
    enum ip_conntrack_info ctinfo;
    struct nf_conn *conntrack;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
    struct iphdr *ip = ip_hdr(skb);
#else
    struct iphdr *ip = skb->nh.iph;
#endif
    int p2p_result = 0, i = 0;
//    int head_len;
    int hlen = ntohs(ip->tot_len)-(ip->ihl*4);	/*hlen = packet-data length*/

    /*must not be a fragment*/
    if (offset) {
	if (info->debug) printk("IPP2P.match: offset found %i \n",offset);
	return 0;
    }
    /*make sure that skb is linear*/
    if(skb_is_nonlinear(skb)){
	if (info->debug) printk("IPP2P.match: nonlinear skb found\n");
	return 0;
    }
    if(!(conntrack = nf_ct_get(skb, &ctinfo))) {
    	if (info->debug) printk("IPP2P.match: couldn't get conntrack\n");
	return 0;
    }

    if(TOTAL_PACKETS == 1)
    	conntrack->ipp2p = 0;

    if(conntrack->ipp2p)
    	return conntrack->ipp2p;

    if((TOTAL_PACKETS > match_packets) && !conntrack->ipp2p)
    	return 0;

//    else
//    	printk("IPP2P: conntrack %d packets\n",TOTAL_PACKETS);
    haystack=(char *)ip+(ip->ihl*4);		/*haystack = packet data*/
    switch (ip->protocol){
	case IPPROTO_TCP:		/*what to do with a TCP packet*/
	{
	    struct tcphdr *tcph = (void *) ip + ip->ihl * 4;

	    if (tcph->fin) return 0;  /*if FIN bit is set bail out*/
	    if (tcph->syn) return 0;  /*if SYN bit is set bail out*/
	    if (tcph->rst) return 0;  /*if RST bit is set bail out*/

	    haystack += tcph->doff * 4; /*get TCP-Header-Size*/
	    hlen -= tcph->doff * 4;
	    while (matchlist[i].command) {
		if ((((info->cmd & matchlist[i].command) == matchlist[i].command) ||
		    ((info->cmd & matchlist[i].short_hand) == matchlist[i].short_hand)) &&
		    (hlen > matchlist[i].packet_len)) {
			    p2p_result = matchlist[i].function_name(haystack, hlen);
			    if (p2p_result)
			    {
				if (info->debug) printk("IPP2P.debug:TCP-match: %i from: %u.%u.%u.%u:%i to: %u.%u.%u.%u:%i Length: %i\n",
				    p2p_result, NIPQUAD(ip->saddr),ntohs(tcph->source), NIPQUAD(ip->daddr),ntohs(tcph->dest),hlen);
				return p2p_result;
    			    }
    		}
	    i++;
	    }
	    conntrack->ipp2p = p2p_result;
	    return p2p_result;
	}

	case IPPROTO_UDP:		/*what to do with an UDP packet*/
	{
	    struct udphdr *udph = (void *) ip + ip->ihl * 4;
	    haystack += 8;
	    hlen -= 8;
	    while (udp_list[i].command){
		if ((((info->cmd & udp_list[i].command) == udp_list[i].command) ||
		    ((info->cmd & udp_list[i].short_hand) == udp_list[i].short_hand)) &&
		    (hlen > udp_list[i].packet_len)) {
			    p2p_result = udp_list[i].function_name(haystack, hlen);
			    if (p2p_result){
				if (info->debug) printk("IPP2P.debug:UDP-match: %i from: %u.%u.%u.%u:%i to: %u.%u.%u.%u:%i Length: %i\n",
				    p2p_result, NIPQUAD(ip->saddr),ntohs(udph->source), NIPQUAD(ip->daddr),ntohs(udph->dest),hlen);
				return p2p_result;
			    }
		}
	    i++;
	    }
	    conntrack->ipp2p = p2p_result;
	    return p2p_result;
	}

	default: return 0;
    }
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,23)
static bool
#else
static int
#endif
checkentry(const char *tablename,
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17)
           const void *ip,
           const struct xt_match *mymatch,
#else
           const struct ipt_ip *ip,
#endif
	   void *matchinfo,
#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
	   unsigned int matchsize,
#endif
	   unsigned int hook_mask)
{
        /* Must specify -p tcp */
/*    if (ip->proto != IPPROTO_TCP || (ip->invflags & IPT_INV_PROTO)) {
 *	printk("ipp2p: Only works on TCP packets, use -p tcp\n");
 *	return 0;
 *    }*/
	return 1;
}


static struct ipt_match ipp2p_match = {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
	{ NULL, NULL },
	"ipp2p",
	&match,
	&checkentry,
	NULL,
	THIS_MODULE
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17)
	.name		= "ipp2p",
	.match		= &match,
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#else /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,17) */
	.name		= "ipp2p",
	.match		= &match,
	.family         = AF_INET,
	.matchsize      = XT_ALIGN(sizeof(struct ipt_p2p_info)),
	.checkentry	= &checkentry,
	.me		= THIS_MODULE,
#endif
};


static int __init init(void)
{
    need_conntrack();
    printk(KERN_INFO "IPP2P v%s loading, with %d packets to match\n", IPP2P_VERSION, match_packets);
    return ipt_register_match(&ipp2p_match);
}

static void __exit fini(void)
{
    ipt_unregister_match(&ipp2p_match);
    printk(KERN_INFO "IPP2P v%s unloaded\n", IPP2P_VERSION);
}

module_init(init);
module_exit(fini);


