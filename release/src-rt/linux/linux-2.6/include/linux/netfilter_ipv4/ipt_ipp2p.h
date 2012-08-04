#ifndef __IPT_IPP2P_H
#define __IPT_IPP2P_H
#define IPP2P_VERSION "0.99.16"

struct ipt_p2p_info {
    int cmd;
    int debug;
};

#endif //__IPT_IPP2P_H

#define SHORT_HAND_IPP2P	1 /* --ipp2p switch*/
#define SHORT_HAND_NONE		5 /* no short hand*/

#define IPP2P_EDK		(1 << 1)
#define IPP2P_DC		(1 << 5)
#define IPP2P_GNU		(1 << 7)
#define IPP2P_KAZAA		(1 << 8)
#define IPP2P_BIT		(1 << 9)
#define IPP2P_APPLE		(1 << 10)
#define IPP2P_SOUL		(1 << 11)
#define IPP2P_WINMX		(1 << 12)
#define IPP2P_ARES		(1 << 13)
#define IPP2P_MUTE		(1 << 14)
#define IPP2P_WASTE		(1 << 15)
#define IPP2P_XDCC		(1 << 16)
#define IPP2P_PP		(1 << 17)
#define IPP2P_XUNLEI		(1 << 18)
