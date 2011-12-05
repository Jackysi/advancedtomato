#include <nvram_convert.h>

#define WL(a)	"wl_"a	
#define WL0(a)	"wl0_"a	

#define PPP(a)		"ppp_"a
#define PPPOE(a)	"pppoe_"a

struct nvram_convert nvram_converts[] = {
	{ WL0("ifname"), 		WL("ifname")},	//	wlconf sets wl0_ifname, but not wl_ifname -- zzz

	{ 0, 0 },
};
