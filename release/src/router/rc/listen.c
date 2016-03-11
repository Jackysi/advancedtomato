/*
	listen.c -- Listen for any packet through an interface
	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE
*/

#include <rc.h>

#include <sys/ioctl.h>
#include <arpa/inet.h>

// for PF_PACKET
#include <features.h>
#if __GLIBC__ >=2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#endif

#define LOG _dprintf

enum {L_FAIL, L_ERROR, L_UPGRADE, L_ESTABLISHED, L_SUCCESS};

struct iphdr {
	u_int8_t version;
	u_int8_t tos;
	u_int16_t tot_len;
	u_int16_t id;
	u_int16_t frag_off;
	u_int8_t ttl;
	u_int8_t protocol;
	u_int16_t check;
	u_int8_t saddr[4];
	u_int8_t daddr[4];
} __attribute__((packed));

struct EthPacket {
	u_int8_t dst_mac[6];
	u_int8_t src_mac[6];
	u_int8_t type[2];
	//struct iphdr ip;	// size = 20
	u_int8_t version;
	u_int8_t tos;
	u_int16_t tot_len;
	u_int16_t id;
	u_int16_t frag_off;
	u_int8_t ttl;
	u_int8_t protocol;
	u_int16_t check;
	u_int8_t saddr[4];
	u_int8_t daddr[4];
	u_int8_t data[1500 - 20];
} __attribute__((packed));


static int read_interface(const char *interface, int *ifindex, unsigned char *mac)
{
	int fd;
	struct ifreq ifr;
	int r;

	memset(&ifr, 0, sizeof(struct ifreq));
	if ((fd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		LOG("socket failed!: \n");
		return -1;
	}

	r = -1;
	ifr.ifr_addr.sa_family = AF_INET;
	strcpy(ifr.ifr_name, interface);

	if (ioctl(fd, SIOCGIFINDEX, &ifr) == 0) {
		*ifindex = ifr.ifr_ifindex;
		LOG("adapter index %d \n", ifr.ifr_ifindex);

		if (ioctl(fd, SIOCGIFHWADDR, &ifr) == 0) {
			memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
			LOG("adapter hardware address %02x:%02x:%02x:%02x:%02x:%02x \n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
			r = 0;
		}
		else {
			LOG("SIOCGIFHWADDR failed!\n");
		}
	}
	else {
		LOG("SIOCGIFINDEX failed!\n");
	}
	close(fd);
	return r;
}

static int raw_socket(int ifindex)
{
	int fd;
	struct sockaddr_ll sock;

	LOG("Opening raw socket on ifindex %d\n", ifindex);
	if ((fd = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
		LOG("socket call failed: \n");
		return -1;
	}

	sock.sll_family = AF_PACKET;
	sock.sll_protocol = htons(ETH_P_IP);
	sock.sll_ifindex = ifindex;
	if (bind(fd, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
		LOG("bind call failed: \n");
		close(fd);
		return -1;
	}

	return fd;
}

static u_int16_t checksum(void *addr, int count)
{
	// Compute Internet Checksum for "count" bytes beginning at location "addr".
	register int32_t sum = 0;
	u_int16_t *source = (u_int16_t *) addr;

	while (count > 1)  {
		/*  This is the inner loop */
		sum += *source++;
		count -= 2;
	}

	/*  Add left-over byte, if any */
	if (count > 0) {
		/* Make sure that the left-over byte is added correctly both with little and big endian hosts */
		u_int16_t tmp = 0;
		*(unsigned char *)(&tmp) = *(unsigned char *)source;
		sum += tmp;
	}

	/*  Fold 32-bit sum to 16 bits */
	while (sum >> 16)
		sum = (sum & 0xffff) + (sum >> 16);
	return ~sum;
}

static int listen_interface(char *interface, int wan_proto, char *prefix)
{
	int ifindex = 0;
	fd_set rfds;
	struct EthPacket packet;
	struct timeval tv;
	int retval;
	unsigned char mac[6];
	static int fd;
	int ret = L_SUCCESS;
	int bytes;
	u_int16_t check;
	struct in_addr ipaddr, netmask;
	char tmp[100];


	if (read_interface(interface, &ifindex, mac) < 0) {
		return L_ERROR;
	}

	fd = raw_socket(ifindex);
	if (fd < 0) {
		LOG("FATAL: couldn't listen on socket\n");
		return L_ERROR;
	}

	while (1) {
		if (!wait_action_idle(5)) {	// Don't execute during upgrading
			ret = L_UPGRADE;
			break;
		}
		if (check_wanup(prefix)) {
			ret = L_ESTABLISHED;
			break;
		}

		tv.tv_sec = 100000;
		tv.tv_usec = 0;
		FD_ZERO(&rfds);
		FD_SET(fd, &rfds);
		LOG("Waitting for select... \n");
		retval = select(fd + 1, &rfds, NULL, NULL, &tv);

		if (retval == 0) {
			printf("no packet recieved! \n\n");
			continue;
		}

		memset(&packet, 0, sizeof(struct EthPacket));
		bytes = read(fd, &packet, sizeof(struct EthPacket));
		if (bytes < 0) {
			close(fd);
			LOG("couldn't read on raw listening socket -- ignoring\n");
			usleep(500000); // possible down interface, looping condition
			return L_FAIL;
		}

		if (bytes < (int) (sizeof(struct iphdr))) {
			LOG("message too short, ignoring\n");
			ret = L_FAIL;
			goto EXIT;
		}

		if (memcmp(mac, packet.dst_mac, 6) != 0) {
			LOG("dest %02x:%02x:%02x:%02x:%02x:%02x mac not the router\n",
				packet.dst_mac[0], packet.dst_mac[1], packet.dst_mac[2],
				packet.dst_mac[3], packet.dst_mac[4], packet.dst_mac[5]);
			ret = L_FAIL;
			goto EXIT;
		}

		if (inet_addr(nvram_safe_get("lan_ipaddr")) == *(u_int32_t *)packet.daddr) {
			LOG("dest ip equal to lan ipaddr\n");
			ret = L_FAIL;
			goto EXIT;
		}

		LOG("inet_addr=%x, packet.daddr=%x",inet_addr(nvram_safe_get("lan_ipaddr")),*(u_int32_t *)packet.daddr);

		//for (i=0; i<34;i++) {
		//	if (i%16==0) printf("\n");
		//	printf("%02x ",*(((u_int8_t *)packet)+i));
		//}
		//printf ("\n");

		LOG("%02X%02X%02X%02X%02X%02X,%02X%02X%02X%02X%02X%02X,%02X%02X\n",
			packet.dst_mac[0], packet.dst_mac[1], packet.dst_mac[2],
			packet.dst_mac[3], packet.dst_mac[4], packet.dst_mac[5],
			packet.src_mac[0], packet.src_mac[1], packet.src_mac[2],
			packet.src_mac[3], packet.src_mac[4], packet.src_mac[5],
			packet.type[0],packet.type[1]);

		LOG("ip.version = %x", packet.version);
		LOG("ip.tos = %x", packet.tos);
		LOG("ip.tot_len = %x", packet.tot_len);
		LOG("ip.id = %x", packet.id);
		LOG("ip.ttl= %x", packet.ttl);
		LOG("ip.protocol= %x", packet.protocol);
		LOG("ip.check=%04x", packet.check);
		LOG("ip.saddr=%08x", *(u_int32_t *)&(packet.saddr));
		LOG("ip.daddr=%08x", *(u_int32_t *)&(packet.daddr));

		if (*(u_int16_t *)packet.type == 0x0800) {
			LOG("not ip protocol");
			ret = L_FAIL;
			goto EXIT;
		}

		/* ignore any extra garbage bytes */
		bytes = ntohs(packet.tot_len);

		/* check IP checksum */
		check = packet.check;
		packet.check = 0;

		if (check != checksum(&(packet.version), sizeof(struct iphdr))) {
			LOG("bad IP header checksum, ignoring\n");
			LOG("check received = %X, should be %X",check, checksum(&(packet.version), sizeof(struct iphdr)));
			ret = L_FAIL;
			goto EXIT;
		}

		LOG("oooooh!!! got some!\n");

		switch (wan_proto) {
		case WP_PPTP:
			inet_aton(nvram_safe_get(strcat_r(prefix, "_pptp_server_ip", tmp)), &ipaddr);
			break;
		case WP_L2TP:
#ifdef TCONFIG_L2TP
			inet_aton(nvram_safe_get("lan_ipaddr"), &ipaddr);	// checkme: why?	zzz
#endif
			break;
		default:
			inet_aton(nvram_safe_get(strcat_r(prefix, "_ipaddr", tmp)), &ipaddr);
			break;
		}
		inet_aton(nvram_safe_get(strcat_r(prefix, "_netmask", tmp)), &netmask);
		LOG(strcat_r(prefix, "_gateway=%08x", tmp), ipaddr.s_addr);
		LOG(strcat_r(prefix, "netmask=%08x", tmp), netmask.s_addr);

		if ((ipaddr.s_addr & netmask.s_addr) != (*(u_int32_t *)&(packet.daddr) & netmask.s_addr)) {
			if (wan_proto == WP_L2TP) {
				ret = L_SUCCESS;
				goto EXIT;
			}
			else {
				ret = L_FAIL;
				goto EXIT;
			}
		}
	}

EXIT:
	if (fd) close(fd);
	return ret;
}

int listen_main(int argc, char *argv[])
{
	char *interface;
	char prefix[] = "wanXXXXXXXXXX_";

	if (argc < 2) {
		usage_exit(argv[0], "<interface> <wanN>");
	}

	interface = argv[1];
	strcpy(prefix,argv[2]);

	printf("Starting listen on %s\n", interface);

	if (fork() != 0) return 0;

	while (1) {
		switch (listen_interface(interface, get_wanx_proto(prefix), prefix)) {
		case L_SUCCESS:
			LOG("\n*** LAN to %s packet received\n\n", prefix);
			force_to_dial(prefix);

			if (check_wanup(prefix)) return 0;

			// Connect fail, we want to re-connect session
			sleep(3);
			break;
		case L_UPGRADE:
			LOG("listen: nothing to do...\n");
			return 0;
		case L_ESTABLISHED:
			LOG("The link had been established\n");
			return 0;
		case L_ERROR:
			LOG("ERROR\n");
			return 0;
/*		case L_FAIL:
			break;	*/
		}
	}
}
