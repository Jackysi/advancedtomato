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
#include <trxhdr.h>
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
WRT160Nv1           BCM4704_BCM5325F_EWC  0x0472       42        0x11      0x0010      boot_hw_model=WRT160N boot_hw_ver=1.0
WRT160Nv3, M10      BCM4716               0x04cd       42        0x1700                boot_hw_model=WRT160N boot_hw_ver=3.0 (M10: boot_hw_model=M10 boot_hw_ver=1.0)
E900                BCM53572              0x058e       42        0x1155                (64k/8MB flash)
E1000v2/E1500       BCM5357               0xf53b       42        0x1100    0x0710      E1500(64k/8MB flash)
E1000v2.1/E1200v1   BCM5357               0xf53a       42        0x1100    0x0710      E1200v1(64k/4MB flash)
E1550               BCM5358U              0xc550       42        0x1100    0x0710      (60k/16MB flash)
E2500               BCM5358U              0xf550       42        0x1101    0x0710      (60k/16MB)
E3200               BCM47186              0xf52a       42        0x1100                (60k/16MB)
WRT320N/E2000       BCM4717               0x04ef       42/66     0x1304/0x1305/0x1307  boardflags: 0x0040F10 / 0x00000602 (??)
WRT610Nv2/E3000     BCM4718               0x04cf       42/??     ??                    boot_hw_model=WRT610N/E300
E4200               BCM4718               0xf52c       42        0x1101                boot_hw_model=E4200
EA6500v1            BCM4706               0xC617       ${serno}  0x1103    0x00000110  modelNumber=EA6500, serial_number=12N10C69224778
EA6500v2	    BCM4708		  0xF646	01	 0x1100	   0x0110	0:devid=0x4332
EA6700		    BCM4708		  0xF646	01	 0x1100	   0x0110	0:devid=0x4332
EA6900		    BCM4708		  0xD646	01	 0x1100	   0x0110	

WHR-G54S            BCM5352E              0x467        00        0x13      0x2758      melco_id=30182
WHR-HP-G54S         BCM5352E              0x467        00        0x13      0x2758      melco_id=30189
WZR-G300N           BCM4704_BCM5325F_EWC  0x0472       ?         0x10      0x10        melco_id=31120
WZR-G54             BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=29115  melco_id=30061 (source: piggy)
WBR-G54                                   bcm94710ap   42                              melco_id=ca020906
WHR2-A54G54         BCM4704_BCM5325F      0x042f       42        0x10      0x0210      melco_id=290441dd
WBR2-G54            BCM4712               0x0101       00        0x10      0x0188      buffalo_id=29bb0332
WZR-G108            BCM4704_BCM5325F      0x042f       42        0x10      0x10        melco_id=30153  melco_id=31095 (source: BaoWeiQuan)
WZR-1750DHP	    BCM4708		  0xF646	00	 0x1100	   0x0110	0:devid=0x4332

WHR-G125			BCM5354G              0x048E       00        0x11      0x750       melco_id=32093

SE505				BCM4712               0x0101                 0x10      0x0388

WR850G v1			BCM4702               bcm94710dev  2                               GemtekPmonVer=9
WR850G v2 (& v3?)	BCM4712               0x0101       44                  0x0188      CFEver=MotoWRv203
WR850G ?			BCM4712               0x0101       44        0x10      0x0188      CFEver=MotoWRv207
WR850G v3			BCM4712               0x0101       44        0x10      0x0188      CFEver=MotoWRv301

WL-500G Deluxe		BCM5365               bcm95365r    45        0x10                  hardware_version=WL500gd-01-04-01-50 regulation_domain=0x30DE sdram_init=0x2008
WL-500G Premium		BCM4704_BCM5325F      0x042f       45        0x10      0x0110      hardware_version=WL500gp-01-02-00-00 regulation_domain=0X10US sdram_init=0x0009
WL-500G Premium		BCM4704_BCM5325F      0x042f       45        0x10      0x0110      hardware_version=WL500gH-01-00-00-00 regulation_domain=0X30DE sdram_init=0x000b
WL-500W			BCM4704_BCM5325F_EWC  0x0472       45        0x23      0x0010      hardware_version=WL500gW-01-00-00-00 regulation_domain=0X10US sdram_init=0x0009

WL-500G Premium v2		HW_BCM5354G           0x48E        45        0x10      0x0750
WL-330GE			HW_BCM5354G           0x048e       45        0x10      0x0650      hardware_version=WL330GE-02-06-00-05 //MIPSR1, 4MB flash w/o USB
WL-520GU			HW_BCM5354G           0x48E        45        0x10      0x0750      hardware_version=WL520GU-01-07-02-00
ZTE H618B			HW_BCM5354G           0x048e     1105        0x35      0x0750
Tenda N60                      BCM47186              0x052B       60        0x1400    0x00000710 //8MB/64MB/2.4/5G/USB
Tenda N6                       BCM5357               0x0550       6         0x1444    0x710 //8MB/64MB/2.4/5G/USB
TENDA W1800R                   HW_BCM4706            0x05d8       18/21(EU)/60(CN)   0x1200  0x00000110

TrendNET			BCM4708               0x0646       1234      0x1100    0x80001200

Buffalo WZR-D1800H             HW_BCM4706            0xf52e       00        0x1204    0x110 //NAND/128M/128M/2.4-5G/USB

Ovislink WL1600GL		HW_BCM5354G           0x048E        8        0x11

RT-N16				BCM4718               0x04cf       45        0x1218    0x0310      hardware_version=RT-N16-00-07-01-00 regulation_domain=0X10US sdram_init=0x419
RT-N15U				BCM5357               0x052b       45        0x1204    0x80001710|0x1000
RT-N12				BCM4716               0x04cd       45        0x1201    0x????
RT-N12B1			BCM5357               0x054d       45        0x1101    0x710
RT-N10				BCM5356               0x04ec       45        0x1402    0x????
RT-N10U				BCM5357               0x0550       45        0x1102    0x710
RT-N10P				BCM53572              0x058e       45        0x1153    0x710
RT-N53				BCM5357               0x0550       45        0x1442    0x710
RT-N53A1			BCM5358U              0x0550       45        0x1446    0x710
RT-N66U				BCM4706               0xf5b2       00        0x1100    0x0110
RT-N18U				BCM4708               0x0646       00        0x1100    0x00000110
RT-AC56U			BCM4708               0x0646	   00	     0x1100    0x00000110
RT-AC68U			BCM4708               0x0646       <MAC>     0x1100    0x00001000
RT-AC68P			BCM4709               0x0665       <MAC>     0x1103    0x00001000


WNR3500L			BCM4718               0x04cf       3500      0x1213|02 0x0710|0x1710
WNR3500Lv2			BCM47186              0x052b       3500(L)   02        0x710|0x1000
WNR2000v2			BCM4716B0             0xe4cd       1         0x1700
R7000				BCM4709               0x0665       32        0x1301    0x1000
R6250				BCM4708               0x0646       679       0x1110 //same as R6300v2 well we use the same MODEL definition
R6300v2				BCM4708               0x0646       679       0x1110 // CH/Charter version has the same signature
R6400				BCM4708               0x0646       32        0x1601

DIR-868L			BCM4708               0x0646       24        0x1110
WS880				BCM4708               0x0646       1234      0x1101
R1D				BCM4709               0x0665       32        0x1301 //same as R7000

F7D4301 v1			BCM4718               0xd4cf       12345     0x1204
F7D3301/F7D3302/F7D4302 v1	BCM4718               0xa4cf       12345     0x1102
F5D8235-4 v3			BCM4718               0xa4cf       12345     0x1100

Dir-620C1			BCM5358U              0x0550       0015      0x1446    0x710 //530MHz/8MB/64MB
Rosewill L600N                  BCM5358U              0x0550	   1015      0x1400    0x710 //500MHz/8MB/64MB/2.4-5GHz/USB
CW-5358U			BCM5357               0x0550       1234      0x1100    0x710 //500MHz/8MB/32MB/2.4G/USB
FiberHome HG320			BCM5357               0x053d       0527      0x1202    0x610 //16MB/64MB/2.4G/USB
ChinaNet RG-200E		BCM5357               0x053d       0504      0x1202    0x610 //16MB/64MB/2.4G/USB/FE
ZTE H218N			BCM5357               0x053d       1234      0x1202    0x710 //16MB/64MB/2.4G/USB

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
WLA2-G54L			BCM4712               0x0101       00        0x10      0x0188      buffalo_id=29129

TrueMobile 2300		                      bcm94710ap   44                              "ModelId=WX-5565", Rev A00


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
	case 0x4ec:
		return HW_BCM5356;
	case 0x489:
		return HW_BCM4785;
#ifdef CONFIG_BCMWL5
	case 0x04cd:
	case 0xe4cd:
	case 0x04fb:
		return HW_BCM4716;
	case 0x04ef:
		return HW_BCM4717;
	case 0x04cf:
	case 0xa4cf:
	case 0xd4cf:
	case 0xf52c:
		return HW_BCM4718;
	case 0x05d8:
	case 0xf5b2:
	case 0xf52e: //WZR-D1800H,D1100H
	case 0xc617: //Linksys EA6500v1
		return HW_BCM4706;
	case 0x052b:
		if (nvram_match("boardrev", "0x1204")) return HW_BCM5357; //rt-n15u
		if (nvram_match("boardrev", "02")) return HW_BCM47186; //WNR3500Lv2
	case 0xf53a:
	case 0xf53b:
	case 0x0550: //RT-N10U and RT-N53 and CW-5358U
		if (nvram_match("boardrev", "0x1400")) return HW_BCM5358U; //L600N
		if (nvram_match("boardrev", "0x1446")) return HW_BCM5358U; //DIR-620C1
		if (nvram_match("boardrev", "0x1444")) return HW_BCM5357; //Tenda N6
	case 0x054d:
	case 0x053d:
		return HW_BCM5357;
	case 0xf52a:
		return HW_BCM47186;
	case 0xf550:
	case 0xc500:
	case 0xc550:
		return HW_BCM5358U;
	case 0x058e:
		if (nvram_match("boardrev", "0x1153")) return HW_BCM5357; //RG100E-CA
		if (nvram_match("boardrev", "0x1155")) return HW_BCM53572; //E900
#endif
#ifdef CONFIG_BCMWL6
	case 0x0646:
	case 0x0665: //R7000,R1D
	case 0xf646: //EA6700,WZR-1750, R6400
	case 0xd646: //EA6900
		return HW_BCM4708;
#endif
	}

	// WR850G may have "bcm94710dev " (extra space)
	if ((strncmp(s, "bcm94710dev", 11) == 0) || (strcmp(s, "bcm94710r4") == 0)) {
		return HW_BCM4702;
	}

	if ((strcmp(s, "bcm95365r") == 0)) {
		return HW_BCM5365;
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
	case 0x31120:
		return MODEL_WZRG300N;
	case 0:
		break;
	default:
		return MODEL_UNKNOWN;
	}

	switch (strtoul(nvram_safe_get("buffalo_id"), NULL, 16)) {
	case 0x29BB0332:
		return MODEL_WBR2G54;
	case 0x29129:
		return MODEL_WLA2G54L;
	case 0:
		break;
	default:
		return MODEL_UNKNOWN;
	}

	if (nvram_match("boardtype", "bcm94710ap")) {
		if (nvram_match("boardnum", "mn700")) return MODEL_MN700;
		if (nvram_match("ModelId", "WX-5565")) return MODEL_TM2300;
	}
	
	if (hw == HW_UNKNOWN) return MODEL_UNKNOWN;

/*
	if (hw == HW_BCM5354G) {
		if (nvram_match("boardrev", "0x11")) {
			return MODEL_WRH54G;
		}
	}
*/

#ifdef CONFIG_BCMWL5
	//added by bwq518
	if (hw == HW_BCM4706) {
		if (nvram_match("modelNumber", "EA6500")) return MODEL_EA6500V1;
	} //bwq518 end

	if (hw == HW_BCM4718) {
		if (nvram_match("boot_hw_model", "WRT610N") ||
		    nvram_match("boot_hw_model", "E300"))
			return MODEL_WRT610Nv2;
		if (nvram_match("boot_hw_model", "E4200"))
			return MODEL_E4200;
		switch (strtoul(nvram_safe_get("boardtype"), NULL, 0)) {
		case 0xd4cf:
			if (nvram_match("boardrev", "0x1204")) return MODEL_F7D4301;
			break;
		case 0xa4cf:
			if (nvram_match("boardrev", "0x1100")) return MODEL_F5D8235v3;
			if (nvram_match("boardrev", "0x1102")) {
				FILE *fp;
				unsigned char s[18];
				uint32 sig = TRX_MAGIC;
				sprintf(s, MTD_DEV(%dro), 1);
				if ((fp = fopen(s, "rb"))) {
					fread(&sig, sizeof(sig), 1, fp);
					fclose(fp);
				}
#ifndef CONFIG_BCMWL6
				switch (sig) {
				case TRX_MAGIC_F7D3301:
					return MODEL_F7D3301;
				case TRX_MAGIC_F7D3302:
					return MODEL_F7D3302;
				default:
					return MODEL_F7D4302;
				}
#endif
			}
			break;
		}
	}
#endif //CONFIG_BCMWL5
#ifdef CONFIG_BCMWL6
	if (hw == HW_BCM4708) {
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("model", "R1D"))) return MODEL_R1D;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-N18U"))) return MODEL_RTN18U;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC56U"))) return MODEL_RTAC56U;
		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC68U"))) return MODEL_RTAC68U;
//REMOVE: Same as RT-AC68U, no nvram "model=RT-AC68R" according to CFE for RT-AC68R
//		if ((nvram_match("boardrev", "0x1100")) && (nvram_match("model", "RT-AC68R"))) return MODEL_RTAC68U;
		if ((nvram_match("boardrev", "0x1103")) && (nvram_match("model", "RT-AC68U"))) return MODEL_RTAC68U;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H245T00_NETGEAR"))) return MODEL_R6250;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T00_NETGEAR"))) return MODEL_R6300v2;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "679")) && (nvram_match("board_id", "U12H240T70_NETGEAR"))) return MODEL_R6300v2;
		if ((nvram_match("boardrev", "0x1601")) && (nvram_match("boardnum", "32"))) return MODEL_R6400;
		if ((nvram_match("boardrev", "0x1301")) && (nvram_match("boardnum", "32"))) return MODEL_R7000;
		if ((nvram_match("boardrev", "0x1110")) && (nvram_match("boardnum", "24"))) return MODEL_DIR868L;
		if ((nvram_match("boardrev", "0x1101")) && (nvram_match("boardnum", "1234"))) return MODEL_WS880;
		if ((nvram_match("boardtype","0xF646")) && (nvram_match("boardnum", "01"))) return MODEL_EA6700;
		if ((nvram_match("boardtype","0xF646")) && (nvram_match("boardnum", "00"))) return MODEL_WZR1750;
		if ((nvram_match("boardtype","0xD646")) && (nvram_match("boardrev", "0x1100"))) return MODEL_EA6900;
	}
#endif
	switch (strtoul(nvram_safe_get("boardnum"), NULL, 0)) {
	case 42:
		switch (hw) {
		case HW_BCM4704_BCM5325F:
			return MODEL_WRTSL54GS;
		case HW_BCM4704_BCM5325F_EWC:
			if (nvram_match("boardrev", "0x10")) return MODEL_WRT300N;
			if (nvram_match("boardrev", "0x11")) return MODEL_WRT160Nv1;
			break;
		case HW_BCM4785:
			if (nvram_match("boardrev", "0x10")) return MODEL_WRT310Nv1;
			break;
#ifdef CONFIG_BCMWL5
		case HW_BCM4716:
			return MODEL_WRT160Nv3;
		case HW_BCM4717:
			return MODEL_WRT320N;
		case HW_BCM4718:
			return MODEL_WRT610Nv2;
		case HW_BCM47186:
			if (nvram_match("boot_hw_model", "E3200")) return MODEL_E3200;
			break;
		case HW_BCM5357:
			if (nvram_match("boot_hw_model", "M10") && nvram_match("boot_hw_ver", "2.0"))
				return MODEL_E1000v2;
			if (nvram_match("boot_hw_model", "E1000")) return MODEL_E1000v2;
			if (nvram_match("boot_hw_model", "E1200") && nvram_match("boot_hw_ver", "1.0"))
				return MODEL_E1500;
			if (nvram_match("boot_hw_model", "E1500")) return MODEL_E1500;
			break;
		case HW_BCM53572:
			if (nvram_match("boot_hw_model", "E800")) return MODEL_E900;
			if (nvram_match("boot_hw_model", "E900")) return MODEL_E900;
			if (nvram_match("boot_hw_model", "E1200") && nvram_match("boot_hw_ver", "2.0"))
				return MODEL_E900;
			break;
		case HW_BCM5358U:
			if (nvram_match("boot_hw_model", "E1550")) return MODEL_E1550;
			if (nvram_match("boot_hw_model", "E2500")) return MODEL_E2500;
			break;
#endif
		}
		return MODEL_WRT54G;
	case 45:
		switch (hw) {
		case HW_BCM4704_BCM5325F:
			return MODEL_WL500GP;
		case HW_BCM4704_BCM5325F_EWC:
			return MODEL_WL500W;
		case HW_BCM5352E:
			return MODEL_WL500GE;
		case HW_BCM5354G:
			if (strncmp(nvram_safe_get("hardware_version"), "WL520GU", 7) == 0) return MODEL_WL520GU;
			if (strncmp(nvram_safe_get("hardware_version"), "WL330GE", 7) == 0) return MODEL_WL330GE;
			return MODEL_WL500GPv2;
		case HW_BCM5365:
			return MODEL_WL500GD;
#ifdef CONFIG_BCMWL5
		case HW_BCM5356:
			if (nvram_match("boardrev", "0x1402")) return MODEL_RTN10;
			break;
		case HW_BCM5357:
			if (nvram_match("boardrev", "0x1102")) return MODEL_RTN10U;
			if (nvram_match("boardrev", "0x1153")) return MODEL_RTN10P;
			if (nvram_match("boardrev", "0x1101")) return MODEL_RTN12B1;
			if (nvram_match("boardrev", "0x1204")) return MODEL_RTN15U;
			if (nvram_match("boardrev", "0x1442")) return MODEL_RTN53;
			break;
		case HW_BCM5358U:
			if (nvram_match("boardrev", "0x1446")) return MODEL_RTN53A1;
			break;
		case HW_BCM4716:
			if (nvram_match("boardrev", "0x1201")) return MODEL_RTN12;
			break;
		case HW_BCM4718:
			if (nvram_match("boardrev", "0x1218")) return MODEL_RTN16;
			break;
#endif
		}
		break;
#ifdef CONFIG_BCMWL5
	case 60:
		switch (hw) {
		case HW_BCM47186:
			return MODEL_TDN60;
		case HW_BCM4706:
			if (nvram_match("boardrev", "0x1200")) return MODEL_W1800R;
		}
		break;
	case 66:
		switch (hw) {
		case HW_BCM4717:
			return MODEL_WRT320N;
		}
		break;
	case 1015:
		switch (hw) {
		case HW_BCM5358U:
			if (nvram_match("boardrev", "0x1400")) return MODEL_L600N;
		}
		break;
	case 0015:
		switch (hw) {
		case HW_BCM5358U:
			if (nvram_match("boardrev", "0x1446")) return MODEL_DIR620C1;
		}
		break;
	case 1234:
		switch (hw) {
		case HW_BCM5357:
			if (nvram_match("boardrev", "0x1100")) return MODEL_CW5358U;
			if (nvram_match("boardrev", "0x1202")) return MODEL_H218N;
		}
		break;
	case 6:
		switch (hw) {
		case HW_BCM5357:
			if (nvram_match("boardrev", "0x1444")) return MODEL_TDN6;
		}
		break;
	case 0527:
		switch (hw) {
		case HW_BCM5357:
			//if (nvram_match("boardrev", "0x1202"))
			return MODEL_HG320;
		}
		break;
	case 0504: // This is the exact original CFE parameter. BWQ
		switch (hw) {
		case HW_BCM5357:
			//if (nvram_match("boardrev", "0x1202"))
			return MODEL_RG200E_CA;
		}
		break;
	case 1:
		switch (hw) {
		case HW_BCM4716:
			//if (nvram_match("boardrev", "0x1700"))
			return MODEL_WNR2000v2;
			break;
		}
		/* fall through */
	case 0543:
		switch (hw) {
		case HW_BCM5357:
			//if (nvram_match("boardrev", "0x1202"))
			return MODEL_H218N;
		}
		break;
	case 3500:
		switch (hw) {
		case HW_BCM4718:
			//if (nvram_match("boardrev", "0x1213") || nvram_match("boardrev", "02"))
			return MODEL_WNR3500L;
			break;
		case HW_BCM47186:
			return MODEL_WNR3500LV2;
			break;
		}
		break;
#endif
	case 0:
		switch (hw) {
		case HW_BCM5354G:
			if (nvram_match("boardrev", "0x35")) return MODEL_DIR320;
			break;
		case HW_BCM4706:
			if (nvram_match("boardrev", "0x1100")) return MODEL_RTN66U;
			if (nvram_match("boardrev", "0x1204")) return MODEL_D1800H;
			break;
		}
		break;
	case 1105:
		if ((hw == HW_BCM5354G) && nvram_match("boardrev", "0x35")) return MODEL_H618B;
		break;
	case 8:
		if ((hw == HW_BCM5354G) && nvram_match("boardrev", "0x11")) return MODEL_WL1600GL;
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
	case 18:
	case 21:
		switch (hw) {
		case HW_BCM4706:
			if (nvram_match("boardrev", "0x1200")) return MODEL_W1800R;
		}
		break;
	}

	return MODEL_UNKNOWN;
}

int supports(unsigned long attr)
{
	return (strtoul(nvram_safe_get("t_features"), NULL, 0) & attr) != 0;
}
