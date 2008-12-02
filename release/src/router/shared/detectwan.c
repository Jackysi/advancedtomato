#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netinet/udp.h>
#include <netinet/ip.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <net/ethernet.h>

#include <shutils.h>
#include <utils.h>
#include <bcmnvram.h>

#define EARTHLINK_SUPPORT

#define DHCP_OFFER	0x02
#define	DHCP_XID_LOC	46

#define IP_LEN	sizeof(struct iphdr)
#define UDP_LEN	sizeof(struct udphdr)

#define MAC_BCAST_ADDR               (unsigned char *) "\xff\xff\xff\xff\xff\xff"

#define TIMEOUT		2
#define RETRY_COUNT	2

#define MY_DEBUG

# define LOG(str, args...) do { printf("DEBUG, "); \
                                printf(str, ## args); \
                                printf("\n"); } while(0)

# define DEBUG(args...) do {;} while(0)

struct dhcpMessage {
        u_int8_t op;
        u_int8_t htype;
        u_int8_t hlen;
        u_int8_t hops;
        u_int32_t xid;
        u_int16_t secs;
        u_int16_t flags;
        u_int32_t ciaddr;
        u_int32_t yiaddr;
        u_int32_t siaddr;
        u_int32_t giaddr;
        u_int8_t chaddr[16];
        u_int8_t sname[64];
        u_int8_t file[128];
        u_int32_t cookie;
        u_int8_t options[308]; /* 312 - cookie */
};

unsigned long xid;
char desc_buf[1024];
char *wan_face = NULL, *lan_face = NULL;

int
compare_subnet(char *dhcp_server_ip, char *dhcp_server_mask, char *lan_face)
{
	struct in_addr ipaddr, netaddr, netmask;
	struct ifreq ifr;	
	int s;
	int ret = 0;

	if ((s = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0)
		return ret;
	strncpy(ifr.ifr_name, lan_face, IFNAMSIZ);
	if (ioctl(s, SIOCGIFADDR, &ifr))
		perror(lan_face);	
	
	netaddr.s_addr = ((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr.s_addr;
        inet_aton(dhcp_server_mask, &netmask);
        inet_aton(dhcp_server_ip, &ipaddr);
					

	netaddr.s_addr &= netmask.s_addr;
	if (netaddr.s_addr == (ipaddr.s_addr & netmask.s_addr))
			ret = 1;

	close(s);

	return ret;
}

int analyse_packet(int type, unsigned char *mac, char *buf, int len, struct detect_wans *detect, int count)
{
	struct ether_header *eptr = (struct ether_header *) &buf[0];
	struct iphdr *ip = (struct iphdr *) &buf[ETH_HLEN];
	struct dhcpMessage *dhcp = (struct dhcpMessage *) &buf[ETH_HLEN+IP_LEN+UDP_LEN];
	int match = 0;
	char server_hwaddr[20];

	printf("Receive:\n");
	printHEX(buf, len);

	/* 
		For Offer of DHCP, the destination MAC is FF:FF:FF:FF:FF:FF, the IP is 255.255.255.255.
		For PADO of PPPoE, the destionaion MAC is Client'MAC.
	*/

	ether_etoa(eptr->ether_shost, server_hwaddr);

	switch(type) 
	{
		case PROTO_DHCP:
			if(eptr->ether_type == htons(ETH_P_IP) &&
			    ip->protocol == IPPROTO_UDP &&
			    dhcp->xid == xid &&
			    dhcp->op == DHCP_OFFER) {			// Add chaddr compare.
				char dhcp_server_ip[20];
				char dhcp_server_mask[20];
				int options_len;
				int i;
				cprintf("Match DHCP Offer\n");

				bzero(dhcp_server_ip, sizeof(dhcp_server_ip));
				bzero(dhcp_server_mask, sizeof(dhcp_server_mask));
				match = 1;
				sprintf(desc_buf+strlen(desc_buf), "DHCP Server MAC%d=%s;\r\n", count, server_hwaddr);	
				/* Calculate options length */
				/* The ip->tot_len is alwyas return error value. ??? */
				options_len = buf[38]*256+buf[39] - UDP_LEN - (sizeof(struct dhcpMessage) - sizeof(dhcp->options));
				for(i=0 ; i<options_len ; i++) {
					//DEBUG("Options[%d]=%02X\n", i, dhcp->options[i]);	
					/* 0x03 is DHCP Server IP Address */
					if(dhcp->options[i] == 0x03) {
						sprintf(dhcp_server_ip, "%d.%d.%d.%d", dhcp->options[i+2], dhcp->options[i+3], dhcp->options[i+4], dhcp->options[i+5]);
						sprintf(desc_buf+strlen(desc_buf), "DHCP Server IP%d=%s;\r\n", count, dhcp_server_ip);	
					}
					/* 0x01 is DHCP Server Subnet Mask */
					else if(dhcp->options[i] == 0x01) {
						sprintf(dhcp_server_mask, "%d.%d.%d.%d", dhcp->options[i+2], dhcp->options[i+3], dhcp->options[i+4], dhcp->options[i+5]);
						sprintf(desc_buf+strlen(desc_buf), "DHCP Server Mask%d=%s;\r\n", count, dhcp_server_mask);	
					}
					/* End options */
					else if(dhcp->options[i] == 0xff)
						break;
					
					i = i+dhcp->options[i+1]+1;

				}

				/* Compare whether the Client's Router have the same subnet mask as Server's Router. If the subnet is the same, the connection will be failed. */
				if(lan_face && dhcp_server_ip[0] && dhcp_server_mask[0]) {
					if(compare_subnet(dhcp_server_ip, dhcp_server_mask, lan_face)) {
						sprintf(desc_buf+strlen(desc_buf), "DHCP Same Subnet=1;\r\n");	
						sprintf(desc_buf+strlen(desc_buf), "DHCP Same Subnet%d=1;\r\n", count);	
					}
					else
						sprintf(desc_buf+strlen(desc_buf), "DHCP Same Subnet%d=0;\r\n", count);	
				}	
			}
			break;
		case PROTO_PPPOE:
			if(!strncmp(mac, eptr->ether_dhost, ETH_ALEN) &&
			   eptr->ether_type == htons(ETH_P_PPP_DISC)) {
				printf("Match PPPoE PADO\n");
				match = 1;
				sprintf(desc_buf+strlen(desc_buf), "PPPoE Server MAC%d=%s;\r\n", count, server_hwaddr);
			}
			break;
	}	

	return match;
}

int
wait_reply(int s, unsigned char *mac, int type, struct detect_wans *detect)
{	
	int     timeout = TIMEOUT;
	int     rv = 1;
	struct timeval  tm;
	time_t          prevTime;
	fd_set          fdset;
	int len = -1;
	static char buf[1512];
	int match = 0;

	tm.tv_usec = 0;
	time(&prevTime);
	while (timeout > 0) {
		FD_ZERO(&fdset);
		FD_SET(s, &fdset);
		tm.tv_sec = timeout;
		if (select(s + 1, &fdset, (fd_set *) NULL, (fd_set *) NULL, &tm) < 0) {
			printf("select error\n");
			if (errno != EINTR) rv = 0;
		} else if (FD_ISSET(s, &fdset)) {
			memset(&buf, 0, sizeof(buf));
			DEBUG("select ok\n");
			len = recv(s, buf, sizeof(buf), 0);
			printf("Receive %d bytes\n", len);
			match += analyse_packet(type, mac, buf, len, detect, match);
		}
		timeout -= time(NULL) - prevTime;
		time(&prevTime);
	}

	close(s);
	return match;
}

int
send_pppoe(char *interface, unsigned char *mac, int ifindex, struct detect_wans *detect)
{
	int s;
	int len;
	struct sockaddr addr;		/* for interface name */
	unsigned char pppoe[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			          0x88, 0x63,
			          0x11, 0x09, 0x00, 0x00, 
			          0x00, 0x0c, 0x01, 0x03, 0x00, 0x04, 0x00, 0x00, 0x02, 0x10, 0x01, 0x01, 0x00, 0x00};

	memset(&addr, 0, sizeof(addr));
	strcpy(addr.sa_data, interface);

	memcpy(&pppoe[6], mac, ETH_ALEN);

	if ((s = socket (PF_PACKET, SOCK_PACKET, htons(ETH_P_PPP_DISC))) == -1) {
                printf("Could not open raw socket\n");
        }   
	
	printf("Send PPPoE PADI\n");
	len = sendto(s, &pppoe, sizeof(pppoe), 0, &addr, sizeof(addr));
	printf("Send %d bytes\n", len);
	return wait_reply(s, mac, PROTO_PPPOE, detect);
}

int read_interface(char *interface, int *ifindex, u_int32_t *addr, unsigned char *arp)
{
        int fd;
        struct ifreq ifr;
        struct sockaddr_in *sin;

        memset(&ifr, 0, sizeof(struct ifreq));
        if((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) >= 0) {
                ifr.ifr_addr.sa_family = AF_INET;
                strcpy(ifr.ifr_name, interface);

                if (addr) {
                        if (ioctl(fd, SIOCGIFADDR, &ifr) == 0) {
                                sin = (struct sockaddr_in *) &ifr.ifr_addr;
                                *addr = sin->sin_addr.s_addr;
                                DEBUG("%s (our ip) = %s \n", ifr.ifr_name, inet_ntoa(sin->sin_addr));
                        } else {
                                DEBUG("SIOCGIFADDR failed!: \n");
                                return -1;
                        }
                }

                if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
                        DEBUG("adapter index %d \n", ifr.ifr_ifindex);
                        *ifindex = ifr.ifr_ifindex;
                } else {
                        DEBUG("SIOCGIFINDEX failed!: \n");
                        return -1;
                }
                if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
                        memcpy(arp, ifr.ifr_hwaddr.sa_data, 6);
                        DEBUG("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x \n",
                                arp[0], arp[1], arp[2], arp[3], arp[4], arp[5]);
                } else {
                        DEBUG("SIOCGIFHWADDR failed!: \n");
                        return -1;
                }
        } else {
                DEBUG("socket failed!: \n");
                return -1;
        }
        close(fd);
        return 0;
}

u_int16_t checksum(void *addr, int count)
{
        /* Compute Internet Checksum for "count" bytes
         *         beginning at location "addr".
         */
        register int32_t sum = 0;
        u_int16_t *source = (u_int16_t *) addr;

        while (count > 1)  {
                /*  This is the inner loop */
                sum += *source++;
                count -= 2;
        }

        /*  Add left-over byte, if any */
        if (count > 0) {
                /* Make sure that the left-over byte is added correctly both
                 * with little and big endian hosts */
                u_int16_t tmp = 0;
                *(unsigned char *) (&tmp) = * (unsigned char *) source;
                sum += tmp;
        }
        /*  Fold 32-bit sum to 16 bits */
        while (sum >> 16) 
                sum = (sum & 0xffff) + (sum >> 16);

        return ~sum;
}

/* Create a random xid */
unsigned long random_xid(void)
{
        static int initialized;
        if (!initialized) {
                int fd;
                unsigned long seed;

                fd = open("/dev/urandom", 0);
                if (fd < 0 || read(fd, &seed, sizeof(seed)) < 0) {
                        printf("Could not load seed from /dev/urandom: %s",
                                strerror(errno));
                        seed = time(0);
                }
                if (fd >= 0) close(fd);
                srand(seed);
                initialized++;
        }
        return rand();
}

int send_dhcp(char *interface, unsigned char *mac, int ifindex, struct detect_wans *detect)
{
	int s;
	int len;
	struct sockaddr addr;		/* for interface name */
	unsigned char dhcp[590];
	unsigned char data1[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x45, 0x00,
				  0x02, 0x40, 0x00, 0x00, 0x00, 0x00, 0x40, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff,
				  0xff, 0xff, 0x00, 0x44, 0x00, 0x43, 0x02, 0x2c, 0x00, 0x00, 0x01, 0x01, 0x06, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x17, 0x37, 0x66, 0x29, 0x00, 0x00, 0x00, 0x00 };
	unsigned char data2[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x63, 0x82, 0x53, 0x63, 0x35, 0x01, 0x01, 0x3d, 0x07, 0x01,
				  0x00, 0x12, 0x17, 0x37, 0x66, 0x29, 0x3c, 0x0b, 0x75, 0x64, 0x68, 0x63, 0x70, 0x20, 0x30, 0x2e,
				  0x39, 0x2e, 0x38, 0x37, 0x07, 0x01, 0x03, 0x06, 0x0c, 0x0f, 0x1c, 0x2c, 0xff, 0x00, 0x00, 0x00 };
	unsigned long sum=0;

	memset(&addr, 0, sizeof(addr));
	memset(&dhcp, 0, sizeof(dhcp));

	strcpy(addr.sa_data, interface);

	memcpy(&dhcp[0], data1, sizeof(data1));
	memcpy(&dhcp[272], data2, sizeof(data2));

	memcpy(&dhcp[6], mac, ETH_ALEN);	// Source
	memcpy(&dhcp[70], mac, ETH_ALEN);	// Client hardware address
	memcpy(&dhcp[288], mac, ETH_ALEN);	// Client hardware address

	/* Create xid */
	xid = random_xid();
	printf("xid= 0x%8lx\n", xid);

	dhcp[DHCP_XID_LOC] = xid;
	dhcp[DHCP_XID_LOC+1] = xid >> (unsigned long)8;
	dhcp[DHCP_XID_LOC+2] = xid >> 16;
	dhcp[DHCP_XID_LOC+3] = xid >> 24;

	/* Calculate IP checksum */
	sum  = checksum(&dhcp[ETH_HLEN], IP_LEN);

	dhcp[24] = sum;
	dhcp[25] = sum >> 8;

	printf("IP hecksum=[%lx]\n", sum);

	/* Calculate UDP checksum */
	sum  = checksum(&dhcp[ETH_HLEN+IP_LEN], sizeof(dhcp)-ETH_HLEN-IP_LEN);

	/* The UDP checksum is always zero. (Change later) */
	//dhcp[40] = sum;
	//dhcp[41] = sum >> 8;

	printf("UDP checksum=[%lx]\n", sum);

	if ((s = socket(PF_PACKET, SOCK_PACKET, htons(ETH_P_IP))) < 0) {
                DEBUG("socket call failed: %s", strerror(errno));
                return -1;
        }

	printHEX(dhcp, sizeof(dhcp));
	
	printf("Send DHCP Discover\n");
	len = sendto(s, &dhcp, sizeof(dhcp), 0, &addr, sizeof(addr));
	printf("Send %d bytes\n", len);
	return wait_reply(s, mac, PROTO_DHCP, detect);
	
}

#ifdef EARTHLINK_SUPPORT
int
detect_earthlink(void)
{
	if(nvram_match("wan_gateway", "172.16.0.254")) {	// Maybe, this is a earthlink modem.
		FILE *fp;
		char line[1024];
		static char page[254];
		char *next;
		char cmd[254];
		char buf[254];

		// showtime.html for new firmware
		// ver.html for old firmware
		foreach(page, "showtime.html ver.html", next) {

			snprintf(cmd, sizeof(cmd), "wget http://172.16.0.254/%s -i 15 -O /tmp/%s\n", page, page);
			snprintf(buf, sizeof(buf), "/tmp/%s", page);
		
			cprintf("CMD: %s\n", cmd);
			system(cmd);

			if((fp = fopen(buf, "r"))) {	
				while( fgets(line, sizeof(line), fp) != NULL) {	
					if(strstr(line, "<auth>") && strstr(line, "</auth>")) {
						if(strstr(line, "bridge")) {	// <auth>bridge</auth>
							sprintf(desc_buf+strlen(desc_buf), "EARTHLINK Settings=%s;\r\n", chomp(line));	
							sprintf(desc_buf+strlen(desc_buf), "EARTHLINK Page=%s;\r\n", page);	
							fclose(fp);
							return 0;
						}
						else {				// <auth>[up/down/failed]</auth>
							sprintf(desc_buf+strlen(desc_buf), "EARTHLINK Settings=%s;\r\n", chomp(line));	
							sprintf(desc_buf+strlen(desc_buf), "EARTHLINK Page=%s;\r\n", page);	
							fclose(fp);
							return 1;
						}
					}
				}
			}

		}
	}
	return 0;
}
#endif

struct detect_wans *
detect_protocol(char *wface, char *lface, char *type)
{
	int ifindex;
	unsigned char mac[6];
	int ret;
	int retry_count;
	struct detect_wans *detect = NULL;

	lan_face = lface;
	
	detect = (struct detect_wans *) malloc(sizeof(struct detect_wans));
	memset(desc_buf, 0, sizeof(desc_buf));
	memset(detect, 0, sizeof(struct detect_wans));

	read_interface(wface, &ifindex, NULL, mac);

#ifdef EARTHLINK_SUPPORT
	if(detect_earthlink()) {	// Request 
		detect->proto = PROTO_EARTHLINK;	
		memcpy(&detect->desc, &desc_buf, strlen(desc_buf));
		return detect;
	}
#endif

	retry_count = 1;
retry_pppoe:
	ret = send_pppoe(wface, mac, ifindex, detect);
	printf("Detect %d PPPoE server\n", ret);
	if(!ret && retry_count < RETRY_COUNT) {
		retry_count ++;
		goto retry_pppoe;
	}
	else if (ret) {
		detect->count = ret;
		detect->proto = PROTO_PPPOE;
		memcpy(&detect->desc, &desc_buf, strlen(desc_buf));
		if(!strcmp(type, "AUTO"))
			return detect;
	}


	retry_count = 1;
retry_dhcp:
	ret = send_dhcp(wface, mac, ifindex, detect);
	printf("Detect %d DHCP server\n", ret);
	if(!ret && retry_count < RETRY_COUNT) {
		retry_count ++;
		goto retry_dhcp;
	}
	else if (ret) {
		detect->count = ret;
		detect->proto = PROTO_DHCP;
		memcpy(&detect->desc, &desc_buf, strlen(desc_buf));
		if(!strcmp(type, "AUTO"))
			return detect;
	}

	detect->count = 0;
	detect->proto = PROTO_STATIC;
	return detect;
}

int
detectwan_main(int argc, char *argv[])
{

	struct detect_wans *detect = NULL;
	
	wan_face = argv[1];
	lan_face = argv[2];

	if(!wan_face) {
		cprintf("You must assign a interface to send packet.\n");
		exit(0);
	}

	detect = (struct detect_wans *) malloc(sizeof(struct detect_wans));

	detect = detect_protocol(wan_face, lan_face, "ALL");

	if(detect)
		free(detect);
	return 0;
}
