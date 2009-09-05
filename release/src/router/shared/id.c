/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include "shared.h"

/*

					HW_*                  boardtype    boardnum  boardrev  boardflags  others
					--------------------- ------------ --------- --------- ----------- ---------------
WRT54G 1.x			BCM4702               bcm94710dev  42
WRT54G 2.0			BCM4712               0x0101       42        0x10      0x0188
WRT54G 2.2, 3.x		BCM5325E              0x0708       42        0x10      0x0118
WRT54G 4.0			BCM5352E              0x0467       42        0x10      0x2558
WRT54GL 1.0, 1.1	BCM5352E              0x0467       42        0x10      0x2558
WRT54GS 1.0         BCM4712               0x0101       42        0x10      0x0388
WRT54GS 1.1, 2.x    BCM5325E              0x0708       42        0x10      0x0318
WRT54GS 3.0, 4.0    BCM5352E              0x0467       42        0x10      0x2758
WRT300N 1.0         BCM4704_BCM5325F_EWC  0x0472       42        0x10      0x10
WRTSL54GS           BCM4704_BCM5325F      0x042f       42        0x10      0x0018
WTR54GS v1, v2      BCM5350               0x456        56        0x10      0xb18       (source: BaoWeiQuan)

WHR-G54S            BCM5352E              0x467        00        0x13      0x2758      melco_id=30182
WHR-HP-G54S         BCM5352E              0x467        00        0x13      0x2758      melco_id=30189
WZR-G300N           BCM4704_BCM5325F_EWC  0x0472       ?         0x10      0x10        melco_id=31120
WZR-G54             BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=29115  melco_id=30061 (source: piggy)
WBR-G54                                   bcm94710ap   42                              melco_id=ca020906
WHR2-A54G54         BCM4704_BCM5325F      0x042f       42        0x10      0x0210      melco_id=290441dd
WBR2-G54            BCM4712               0x0101       00        0x10      0x0188      buffalo_id=29bb0332
WZR-G108            BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=30153  melco_id=31095 (source: BaoWeiQuan)

WHR-G125			BCM5354G              0x048E       00        0x11      0x750       melco_id=32093

SE505				BCM4712               0x0101                 0x10      0x0388

WR850G v1			BCM4702               bcm94710dev  2                               GemtekPmonVer=9
WR850G v2 (& v3?)	BCM4712               0x0101       44                  0x0188      CFEver=MotoWRv203
WR850G ?			BCM4712               0x0101       44        0x10      0x0188      CFEver=MotoWRv207
WR850G v3			BCM4712               0x0101       44        0x10      0x0188      CFEver=MotoWRv301

*WL-500G Deluxe		                      bcm95365r    45        0x10                  hardware_version=WL500gd-01-04-01-50 regulation_domain=0x30DE sdram_init=0x2008
WL-500G Premium		BCM4704_BCM5325F      0x042f       45        0x10      0x0110      hardware_version=WL500gp-01-02-00-00 regulation_domain=0X10US sdram_init=0x0009
WL-500G Premium		BCM4704_BCM5325F      0x042f       45        0x10      0x0110      hardware_version=WL500gH-01-00-00-00 regulation_domain=0X30DE sdram_init=0x000b

WL-500G Premium v2  HW_BCM5354G           0x48E        45        0x10      0x0750
WL-520GU			HW_BCM5354G           0x48E        45        0x10      0x0750      hardware_version=WL520GU-01-07-02-00


WL-550gE			BCM5352E              0x0467       45        0x10      0x0758      hardware_version=WL550gE-01-05-01-00 sdram_init=0x2000

*WL-700gE			BCM4704_BCM5325F      0x042f       44        0x10      0x0110      hardware_version=WL700g-01-10-01-00 regulation_domain=0X30DE
*WL-700gE			BCM4704_BCM5325F      0x042f       44        0x10      0x0110      hardware_version=WL700g-01-08-01-00 regulation_domain=0X30DE

WX-6615GT			BCM4712               0x0101       44        0x10      0x0188      CFEver=GW_WR110G_v100

MN-700                                    bcm94710ap   mn700                           hardware_version=WL500-02-02-01-00 regulation_domain=0X30DE

source: piggy
WZR-HP-G54          BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=30026
WZR-RS-G54          BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=30083
WZR-RS-G54HP        BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=30103
WVR-G54-NF          BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=28100
WHR2-A54-G54        BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=290441dd
WHR3-AG54           BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=29130
?	WZR SERIES          BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=29115
RT390W generic      BCM4702               bcm94710r4   100                             clkfreq=264
WR850G v1			BCM4702               bcm94710r4   100

*WRH54G				BCM5354G              0x048E       ?         0x10      ?

Viewsonic WR100		BCM4712               0x0101       44        0x10      0x0188      CFEver=SparkLanv100

* not supported/not tested


BFL_ENETADM		0x0080
BFL_ENETVLAN	0x0100

*/


int check_hw_type(void)
{
	const char *s;
	unsigned long bf;

	bf = strtoul(nvram_safe_get("boardflags"), NULL, 0);
	s = nvram_safe_get("boardtype");
	switch (strtoul(s, NULL, 0)) {
	case 0x467:
		return HW_BCM5352E;
	case 0x101:
		return ((bf & BFL_ENETADM) == 0) ? HW_BCM4712_BCM5325E : HW_BCM4712;
	case 0x708:
		return ((bf & BFL_ENETADM) == 0) ? HW_BCM5325E : HW_UNKNOWN;
	case 0x42f:
		return ((bf & BFL_ENETADM) == 0) ? HW_BCM4704_BCM5325F : HW_UNKNOWN;
	case 0x472:
		return ((bf & BFL_ENETADM) == 0) ? HW_BCM4704_BCM5325F_EWC : HW_UNKNOWN;
	case 0x478:
		return HW_BCM4705L_BCM5325E_EWC;
	case 0x48E:
		return HW_BCM5354G;
	case 0x456:
		return HW_BCM5350;
	}

	// WR850G may have "bcm94710dev " (extra space)
	if ((strncmp(s, "bcm94710dev", 11) == 0) || (strcmp(s, "bcm94710r4") == 0)) {
		return HW_BCM4702;
	}

	return HW_UNKNOWN;
}

int get_model(void)
{
	int hw;
	char *c;

	hw = check_hw_type();

	switch (strtoul(nvram_safe_get("melco_id"), NULL, 16)) {
	case 0x29115:
    case 0x30061:
		return MODEL_WZRG54;
	case 0x30182:
		return MODEL_WHRG54S;
	case 0x30189:
		return MODEL_WHRHPG54;
	case 0xCA020906:
		return MODEL_WBRG54;
	case 0x30026:
    	return MODEL_WZRHPG54;
    case 0x30083:
        return MODEL_WZRRSG54;
    case 0x30103:
    	return MODEL_WZRRSG54HP;
    case 0x28100:
    	return MODEL_WVRG54NF;
    case 0x29130:
    	return MODEL_WHR3AG54;
	case 0x290441DD:
		return MODEL_WHR2A54G54;
	case 0x32093:
		return MODEL_WHRG125;
	case 0x30153:
	case 0x31095:
		return MODEL_WZRG108;
#if TOMATO_N
	case 0x31120:
		return MODEL_WZRG300N;
#endif
	case 0:
		break;
	default:
		return MODEL_UNKNOWN;
	}

	switch (strtoul(nvram_safe_get("buffalo_id"), NULL, 16)) {
	case 0x29BB0332:
		return MODEL_WBR2G54;
	case 0:
		break;
	default:
		return MODEL_UNKNOWN;
	}

	if (nvram_match("boardnum", "mn700")) {
		return MODEL_MN700;
	}

	if (hw == HW_UNKNOWN) return MODEL_UNKNOWN;

/*
	if (hw == HW_BCM5354G) {
		if (nvram_match("boardrev", "0x11")) {
			return MODEL_WRH54G;
		}
	}
*/

	switch (strtoul(nvram_safe_get("boardnum"), NULL, 0)) {
	case 42:
		switch (hw) {
		case HW_BCM4704_BCM5325F:
			return MODEL_WRTSL54GS;
#if TOMATO_N
		case HW_BCM4704_BCM5325F_EWC:
			return MODEL_WRT300N;
#endif
		}
		return MODEL_WRT54G;
	case 45:
		switch (hw) {
		case HW_BCM4704_BCM5325F:
			return MODEL_WL500GP;
		case HW_BCM5352E:
			return MODEL_WL500GE;
		case HW_BCM5354G:
			if (strncmp(nvram_safe_get("hardware_version"), "WL520GU", 7) == 0) return MODEL_WL520GU;
			return MODEL_WL500GPv2;
		}
		break;
	case 0:
		switch (hw) {
		case HW_BCM5354G:
			if (nvram_match("boardrev", "0x35")) return MODEL_DIR320;
		}
		break;
	case 2:
		if (nvram_match("GemtekPmonVer", "9")) return MODEL_WR850GV1;
		break;
	case 44:
		c = nvram_safe_get("CFEver");
		if (strncmp(c, "MotoWRv", 7) == 0) return MODEL_WR850GV2;
		if (strncmp(c, "GW_WR110G", 9) == 0) return MODEL_WX6615GT;
		if (strcmp(c, "SparkLanv100") == 0) return MODEL_WR100;
		break;
	case 100:
		if (strcmp(nvram_safe_get("clkfreq"), "264") == 0) return MODEL_RT390W;
		break;
	case 56:
		if (hw == HW_BCM5350) return MODEL_WTR54GS;
		break;
	}

	return MODEL_UNKNOWN;
}

int supports(unsigned long attr)
{
	return (strtoul(nvram_safe_get("t_features"), NULL, 0) & attr) != 0;
}
