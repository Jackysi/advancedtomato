
/*
 *********************************************************
 *   Copyright 2003, CyberTAN  Inc.  All Rights Reserved *
 *********************************************************

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <broadcom.h>
#include <cyutils.h>
#include <sys/sysinfo.h>

int
show_default_info(webs_t wp)
{
	int ret = 0;
	
	websDone(wp, 200);	// Let header in first packet, and bellow information in second packet.
	
	ret = websWrite(wp, "Vendor:%s\n", CT_VENDOR);
	ret = websWrite(wp, "ModelName:%s\n", MODEL_NAME);
	ret = websWrite(wp, "Firmware Version:%s%s , %s\n", CYBERTAN_VERSION,MINOR_VERSION,__DATE__);
	ret = websWrite(wp, "#:%s\n", SERIAL_NUMBER);
	ret = websWrite(wp, "Boot Version:%s", nvram_safe_get("boot_ver"));
	if(nvram_get("bootnv_ver"))
		ret = websWrite(wp, ".%s\n", nvram_safe_get("bootnv_ver"));
	else
		ret = websWrite(wp, "\n");
	ret = websWrite(wp, "CodePattern:%s\n", CODE_PATTERN);
#if LOCALE == EUROPE
	ret = websWrite(wp, "Country:Europe\n");
#elif LOCALE == GERMANY
	ret = websWrite(wp, "Country:Europe\n");
#elif LOCALE == FRANCE
	ret = websWrite(wp, "Country:Europe\n");
#elif LOCALE == JAPAN
	ret = websWrite(wp, "Country:Japan\n");
#else
	ret = websWrite(wp, "Country:US\n");
#endif
	ret = websWrite(wp, "\n");
	
	ret = websWrite(wp, "RF Status:%s\n", (nvram_match("wl0_hwaddr","") || nvram_match("wl_gmode", "-1")) ? "disabled" :"enabled");
	ret = websWrite(wp, "RF Firmware Version:%s%s\n", CYBERTAN_VERSION,MINOR_VERSION);
#if LOCALE == EUROPE
	ret = websWrite(wp, "RF Domain:ETSI (channel 1~%s)\n", WL_MAX_CHANNEL);
#elif LOCALE == GERMANY
	ret = websWrite(wp, "RF Domain:ETSI (channel 1~%s)\n", WL_MAX_CHANNEL);
#elif LOCALE == FRANCE
	ret = websWrite(wp, "RF Domain:ETSI (channel 1~%s)\n", WL_MAX_CHANNEL);
#elif LOCALE == JAPAN
	ret = websWrite(wp, "RF Domain:JPN (channel 1~%s)\n", WL_MAX_CHANNEL);
#else
	ret = websWrite(wp, "RF Domain:US (channel 1~%s)\n", WL_MAX_CHANNEL);
#endif
	ret = websWrite(wp, "RF Channel:%s\n", nvram_safe_get("wl_channel"));
	ret = websWrite(wp, "RF SSID:%s\n", nvram_safe_get("wl_ssid"));
	ret = websWrite(wp, "\n-----Dynamic Information\n");

	ret = websWrite(wp, "RF Mac Address:%s\n", nvram_safe_get("wl0_hwaddr"));
	ret = websWrite(wp, "LAN Mac Address:%s\n", nvram_safe_get("lan_hwaddr"));
	ret = websWrite(wp, "WAN Mac Address:%s\n", nvram_safe_get("wan_hwaddr"));
	if(check_hw_type() == BCM4702_CHIP)
		ret = websWrite(wp, "Hardware Version:1.x\n");
	else
		ret = websWrite(wp, "Hardware Version:2.0\n");
	ret = websWrite(wp, "Device Serial No.:%s\n", nvram_safe_get("get_sn"));

	ret = websWrite(wp, "\n");	// The last char must be '\n'
	
	return ret;
}

static char *
exec_cmd(char *cmd)
{
	FILE * fp;
	static char line[254];

	bzero(line, sizeof(line));

	if((fp = popen(cmd, "r"))) {
		fgets(line, sizeof(line), fp);
		pclose(fp);
	}

	return line;	
}

int
show_other_info(webs_t wp)
{
	int ret = 0;
	struct sysinfo info;
	
	websDone(wp, 200);	// Let header in first packet, and bellow information in second packet.
	
	ret += websWrite(wp, "language=%s;\n", nvram_safe_get("language"));
	ret += websWrite(wp, "Flash Type=%s;\n", nvram_safe_get("flash_type"));
	ret += websWrite(wp, "CPU Clock=%s;\n", nvram_safe_get("clkfreq"));
	ret += websWrite(wp, "sdram_init=%s;\n", nvram_safe_get("sdram_init"));
	ret += websWrite(wp, "sdram_config=%s;\n", nvram_safe_get("sdram_config"));
	ret += websWrite(wp, "sdram_ncdl=%s;\n", nvram_safe_get("sdram_ncdl"));
	ret += websWrite(wp, "pa0b0=%s;\n", nvram_safe_get("pa0b0"));
	ret += websWrite(wp, "pa0b1=%s;\n", nvram_safe_get("pa0b1"));
	ret += websWrite(wp, "pa0b2=%s;\n", nvram_safe_get("pa0b2"));
	ret += websWrite(wp, "Write Mac Address=%s;\n", nvram_safe_get("et0macaddr"));
	ret += websWrite(wp, "\n");
	ret += websWrite(wp, "get wl_gmode=%s;\n", nvram_safe_get("wl_gmode"));
	ret += websWrite(wp, "\n");

	sysinfo(&info);
	ret += websWrite(wp, "totalram=%ld, freeram=%ld, bufferram=%ld;\n", info.totalram, info.freeram, info.bufferram);
	ret += websWrite(wp, "uptime=%ld;\n", info.uptime);
	ret += websWrite(wp, "\n");

	ret += websWrite(wp, "%s", exec_cmd("wl list_ie |grep Total"));
	ret += websWrite(wp, "eou_configured=%s;\n", nvram_safe_get("eou_configured"));
	ret += websWrite(wp, "get_eou_index=%s;\n", nvram_safe_get("get_eou_index"));
	ret += websWrite(wp, "get_sn_index=%s;\n", nvram_safe_get("get_sn_index"));
	ret += websWrite(wp, "get_sn=%s;\n", nvram_safe_get("get_sn"));
	ret += websWrite(wp, "\n");
	ret += websWrite(wp, "get_mac_index=%s;\n", nvram_safe_get("get_mac_index"));
	ret += websWrite(wp, "get_mac=%s;\n", nvram_safe_get("get_mac"));
	ret += websWrite(wp, "ses_status=%s;\n", nvram_safe_get("ses_status"));
	ret += websWrite(wp, "ses_count=%s;\n", nvram_safe_get("ses_count"));

	ret = websWrite(wp, "\n");	// The last char must be '\n'

	return ret;
}

int
show_wlan_info(webs_t wp)
{
	int ret;

	websDone(wp, 200);	// Let header in first packet, and bellow information in second packet.

	ret += websWrite(wp, "RouterName:%s\n", nvram_safe_get("router_name"));

	if(nvram_match("wl_net_mode", "disabled"))
		ret += websWrite(wp, "11gNetworkMode:Disabled\n");
	else if(nvram_match("wl_net_mode", "mixed"))
		ret += websWrite(wp, "11gNetworkMode:Mixed\n");
	else if(nvram_match("wl_net_mode", "b-only"))
		ret += websWrite(wp, "11gNetworkMode:B-only\n");
	else if(nvram_match("wl_net_mode", "g-only"))
		ret += websWrite(wp, "11gNetworkMode:G-only\n");

	ret += websWrite(wp, "11gSSID:%s\n", nvram_safe_get("wl_ssid"));
	ret += websWrite(wp, "11gChannel:%s\n", nvram_safe_get("wl_channel"));
	
	if(nvram_match("wl_closed", "1"))
		ret += websWrite(wp, "11gSSIDBroadcast:Disabled\n");
	else
		ret += websWrite(wp, "11gSSIDBroadcast:Enabled\n");

	if(nvram_match("security_mode2", "disabled")) {
		ret += websWrite(wp, "SecurityMode:Disable\n");
		ret += websWrite(wp, "Encryption:off\n");
		
	}
	else if(nvram_match("security_mode2", "wep")) {
		if(nvram_match("wl_wep_bit", "64"))
			ret += websWrite(wp, "SecurityMode:WEP64bits\n");
		else
			ret += websWrite(wp, "SecurityMode:WEP128bits\n");
		ret += websWrite(wp, "WEPPassphrase:%s\n", nvram_safe_get("wl_passphrase"));
		ret += websWrite(wp, "WEPDefaultKey:%s\n", nvram_safe_get("wl_key"));
		ret += websWrite(wp, "WEPKey1:%s\n", nvram_safe_get("wl_key1"));
		ret += websWrite(wp, "WEPKey2:%s\n", nvram_safe_get("wl_key2"));
		ret += websWrite(wp, "WEPKey3:%s\n", nvram_safe_get("wl_key3"));
		ret += websWrite(wp, "WEPKey4:%s\n", nvram_safe_get("wl_key4"));
	}
	else if(nvram_match("security_mode2", "wpa_personal")) {
		ret += websWrite(wp, "SecurityMode:WPA_Personal\n");
		ret += websWrite(wp, "Encryption:%s\n", nvram_safe_get("wl_crypto"));
		ret += websWrite(wp, "WPA_PSK:%s\n", nvram_safe_get("wl_wpa_psk"));
	}
	else if(nvram_match("security_mode2", "wpa2_personal")) {
		ret += websWrite(wp, "SecurityMode:WPA2_Personal\n");
		ret += websWrite(wp, "Encryption:%s\n", nvram_safe_get("wl_crypto"));
		ret += websWrite(wp, "WPA_PSK:%s\n", nvram_safe_get("wl_wpa_psk"));
	}
	
	if(nvram_match("ses_status", "success"))
		ret += websWrite(wp, "ses_status:Success\n");
	else if(nvram_match("ses_status", "failure"))
		ret += websWrite(wp, "ses_status:Failure\n");
	else if(nvram_match("ses_status", "unknown"))
		ret += websWrite(wp, "ses_status:Unknown\n");
	else
		ret += websWrite(wp, "ses_status:%s\n", nvram_safe_get("ses_status"));

	ret += websWrite(wp, "ses_count:%s\n", nvram_safe_get("ses_count"));

	ret = websWrite(wp, "\n");	// The last char must be '\n'

	return ret;
}

int
ej_show_sysinfo(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret;
	char *type;
	
	if (ejArgs(argc, argv, "%s", &type) < 1) {
		websError(wp, 400, "Insufficient args\n");
		return -1;
	}

	if(type && !strcmp(type, "other"))
		ret = show_other_info(wp);
	else if(type && !strcmp(type, "wlan"))
		ret = show_wlan_info(wp);
	else
		ret = show_default_info(wp);

	return ret;
}


// for Setup Wizard and others test
int
ej_show_miscinfo(int eid, webs_t wp, int argc, char_t **argv)
{
	int ret = 0;
	
	websDone(wp, 200);	// Let header in first packet, and bellow information in second packet.
	
	ret += websWrite(wp, "Module Name=%s;\n", MODEL_NAME);
	ret += websWrite(wp, "Firmware Version=%s%s,%s;\n", CYBERTAN_VERSION, MINOR_VERSION, __DATE__);
	ret += websWrite(wp, "Firmware Time=%s;\n", __TIME__);
	ret += websWrite(wp, "SWAOLstatus=%s;\n", nvram_safe_get("aol_block_traffic"));
	ret += websWrite(wp, "SWAT&Tstatus=0;\n");

	if(nvram_match("wan_proto","dhcp"))
		ret += websWrite(wp, "SWWanStatus=0;\n");
	if(nvram_match("wan_proto","static"))
		ret += websWrite(wp, "SWWanStatus=1;\n");
	if(nvram_match("wan_proto","pppoe"))
		ret += websWrite(wp, "SWWanStatus=2;\n");
	if(nvram_match("wan_proto","pptp"))
		ret += websWrite(wp, "SWWanStatus=3;\n");

	ret += websWrite(wp, "\n");
	
	ret += websWrite(wp, "SWGetRouterIP=%s;\n", nvram_safe_get("lan_ipaddr"));
	ret += websWrite(wp, "SWGetRouterDomain=%s;\n", nvram_safe_get("wan_domain"));
	ret += websWrite(wp, "SWpppoeUName=%s;\n", nvram_safe_get("ppp_username"));
		
	ret += websWrite(wp, "\n");

	ret += websWrite(wp, "SWGetRouterSSID=%s;\n", nvram_safe_get("wl_ssid"));
	ret += websWrite(wp, "SWGetRouterChannel=%s;\n", nvram_safe_get("wl_channel"));
	ret += websWrite(wp, "SWssidBroadcast=%s;\n", nvram_safe_get("wl_closed"));
	
	ret += websWrite(wp, "\n");
	ret = websWrite(wp, "RF Mac Address=%s;\n", nvram_safe_get("wl0_hwaddr"));
	
	if(nvram_match("security_mode", "disabled"))
		ret += websWrite(wp, "SWwirelessStatus=0;\n");
	if(nvram_match("security_mode", "wep"))
		ret += websWrite(wp, "SWwirelessStatus=1;\n");
	if(nvram_match("security_mode", "psk"))
		ret += websWrite(wp, "SWwirelessStatus=2;\n");
	if(nvram_match("security_mode", "radius"))
		ret += websWrite(wp, "SWwirelessStatus=3;\n");

	if(nvram_match("wl_wep", "off") || nvram_match("wl_wep", "disabled"))
		ret += websWrite(wp, "SWwlEncryption=off;\n");
	if(nvram_match("wl_wep", "on") || nvram_match("wl_wep", "restricted") || nvram_match("wl_wep", "enabled")){
		ret += websWrite(wp, "SWwlEncryption=wep;\n");
		ret += websWrite(wp, "SWwepEncryption=%s;\n", nvram_safe_get("wl_wep_bit"));
	}
	if(nvram_match("wl_crypto", "tkip"))
		ret += websWrite(wp, "SWwlEncryption=tkip;\n");
	if(nvram_match("wl_crypto", "aes"))
		ret += websWrite(wp, "SWwlEncryption=aes;\n");
	if(nvram_match("wl_crypto", "tkip+aes"))
		ret += websWrite(wp, "SWwlEncryption=tkip+aes;\n");

	/* Below for RF test 2003-10-29 */
	ret += websWrite(wp, "WL_tssi_result=%s;\n", nvram_safe_get("wl_tssi_result"));
	ret += websWrite(wp, "WL_cck_result=%s;\n", nvram_safe_get("wl_cck_result"));
	ret += websWrite(wp, "WL_ofdm_result=%s;\n", nvram_safe_get("wl_ofdm_result"));
	system("wl curpower > /tmp/curpower");
	ret += websWrite(wp, "WL_now_cck_result=%s;\n", get_curpower("cck"));
	ret += websWrite(wp, "WL_now_ofdm_result=%s;\n", get_curpower("ofdm"));
	
	return ret;
}
