
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
#include <time.h>
#include <sys/klog.h>
#include <netdb.h>
#include <signal.h>

#include <broadcom.h>
#include <cy_conf.h>

#define mynvram_set(name,value) do { \
	nvram_set(name,value); \
	cprintf("nvram_set(\"%s\", \"%s\")\n", name, value); \
} while (0)

extern int sys_commit(void);
extern void show_error(webs_t wp, char *message);

static int
valid_value(webs_t wp)
{

	char *setupwizard=NULL, *hostname=NULL, *domainname=NULL, *routername=NULL;
	char *mac=NULL, *mac0, *mac1, *mac2, *mac3, *mac4, *mac5;
	char dns[100]="", *dns0=NULL, *dns1=NULL, *dns2=NULL, *dns0_1, *dns0_2, *dns0_3, *dns0_4, *dns1_1, *dns1_2, *dns1_3, *dns1_4, *dns2_1, *dns2_2, *dns2_3, *dns2_4;
	char *wan_ip=NULL, *wan_ip0, *wan_ip1, *wan_ip2, *wan_ip3;
	char *wan_mask=NULL, *wan_mask0, *wan_mask1, *wan_mask2, *wan_mask3;
	char *wan_gw=NULL, *wan_gw0, *wan_gw1, *wan_gw2, *wan_gw3;
	char *lan_ip=NULL, *lan_ip0, *lan_ip1, *lan_ip2, *lan_ip3;
#ifdef AOL_SUPPORT
	char *aol_block_traffic;
#endif

	char wan_proto[20], *wan_status=NULL, *ppp_username=NULL, *ppp_passwd=NULL, *http_passwd=NULL, *http_passwdcom=NULL, *keepalive=NULL, *alivetime=NULL;
	char wl_wep_buf[100], *wl_status=NULL, *wl_key=NULL, *wl_wep_bit=NULL, *wl_passphrase=NULL, *wl_key1=NULL, *wl_ssid=NULL, *wl_channel=NULL, *wl_bcast=NULL, *wl_encryption=NULL, *wl_wpa_psk=NULL, *wl_gmode=NULL, *wl_net_mode=NULL;

	char buf[20];
	char *set_mode = NULL; //Added by Honor (2005-05-19)

	struct variable setup_wizard_variables[] = {
		{ longname: "MAC address", NULL },				// 0
		{ longname: "DNS 0 IP address", NULL },				// 1
		{ longname: "DNS 1 IP address", NULL },				// 2
		{ longname: "DNS 2 IP address", NULL },				// 3
		{ longname: "WAN IP address", NULL },				// 4
		{ longname: "netmask IP address", NULL },			// 5
		{ longname: "gateway IP address", NULL },			// 6
		{ longname: "802.11g Channel", argv: ARGV("1", "14") },		// 7
		{ longname: "Network Type", argv: ARGV("0", "1") },		// 8 (1:broadcast)
		{ longname: "wan status", argv: ARGV("0","5") },		// 9 (0:dhcp 1:static 2:pppoe 3:pptp 4:l2tp 5:heartbeat)
		{ longname: "Auth Mode", argv: ARGV("0","4") },		// 10 (0:off 1:restricted 2:psk 3:psk2 only 4:psk2 mixed)
		{ longname: "Network Key Index", argv: ARGV("1","4") },		// 11
		{ longname: "Encryption bit", argv: ARGV("64","128") },		// 12
		{ longname: "aol status", argv: ARGV("0","1","2") },		// 13
		{ longname: "Wireless Encryption", argv: ARGV("off","wep","tkip","aes","tkip+aes") },	// 14	// Add for New Setup Wizard, by honor 2003-07-24
		{ longname: "Wireless Mode", argv: ARGV("-1","0","1","2","6") },	// 15	// Add for Parental, by honor 2003-10-13
		{ longname: "Wireless Net Mode", argv: ARGV("disabled","mixed","g-only","b-only","speedbooster") },
		{ longname: "Keep Alive / Connect On Demand", argv: ARGV("1", "2") },	// 17 (1:KeepAlive 2:ConnectOnDemand)
		{ longname: "Redial Time", argv: ARGV("20", "180") },		// 18
		{ longname: "Idle Time", argv: ARGV("1", "9999") },		// 19
		{ longname: "LAN IP address", NULL },				// 20
		{ longname: "Set Mode", argv: ARGV("0", "1", "2") },		// 21	(0: both 1:wired 2:wireless)

	}, *which;

	which = &setup_wizard_variables[0];

	setupwizard = websGetVar(wp, "SetupWizard", NULL);
	if(!setupwizard || strcmp(setupwizard,"1")){
		show_error(wp, "Invalid setup wizard format!");
		goto fail;
	}

	set_mode = websGetVar(wp, "SWsetMode", "0");
	
	if(atoi(set_mode) == 0 || atoi(set_mode) == 1)
	{
		wan_status = websGetVar(wp, "SWwanStatus", NULL);
		hostname = websGetVar(wp, "SWhostName", NULL);
		domainname = websGetVar(wp, "SWDomainName", NULL);
		routername = websGetVar(wp, "SWRouterName", NULL);

		mac0 = websGetVar(wp, "SWwanMac0", NULL);
		mac1 = websGetVar(wp, "SWwanMac1", NULL);
		mac2 = websGetVar(wp, "SWwanMac2", NULL);
		mac3 = websGetVar(wp, "SWwanMac3", NULL);
		mac4 = websGetVar(wp, "SWwanMac4", NULL);
		mac5 = websGetVar(wp, "SWwanMac5", NULL);
		dns0_1 = websGetVar(wp, "SWdnsA1", NULL);
		dns0_2 = websGetVar(wp, "SWdnsA2", NULL);
		dns0_3 = websGetVar(wp, "SWdnsA3", NULL);
		dns0_4 = websGetVar(wp, "SWdnsA4", NULL);
		dns1_1 = websGetVar(wp, "SWdnsB1", NULL);
		dns1_2 = websGetVar(wp, "SWdnsB2", NULL);
		dns1_3 = websGetVar(wp, "SWdnsB3", NULL);
		dns1_4 = websGetVar(wp, "SWdnsB4", NULL);
		dns2_1 = websGetVar(wp, "SWdnsC1", NULL);
		dns2_2 = websGetVar(wp, "SWdnsC2", NULL);
		dns2_3 = websGetVar(wp, "SWdnsC3", NULL);
		dns2_4 = websGetVar(wp, "SWdnsC4", NULL);

		lan_ip0 = websGetVar(wp, "SWLanIP1", NULL);
		lan_ip1 = websGetVar(wp, "SWLanIP2", NULL);
		lan_ip2 = websGetVar(wp, "SWLanIP3", NULL);
		lan_ip3 = websGetVar(wp, "SWLanIP4", NULL);

		if(lan_ip0 && lan_ip1 && lan_ip2 && lan_ip3){
			lan_ip = malloc(20);
			snprintf(buf,sizeof(buf),"%s.%s.%s.%s",lan_ip0, lan_ip1, lan_ip2, lan_ip3);
			strcpy(lan_ip,buf);
			cprintf("lan_ip=[%s]\n",lan_ip);
			if(!valid_ipaddr(wp, lan_ip, &which[20])){
				goto fail;
			}
		}

#ifdef AOL_SUPPORT
		aol_block_traffic = websGetVar(wp, "SWblock_traffic", NULL);

		if(!aol_block_traffic || !valid_range(wp, aol_block_traffic, &which[13])){
			show_error(wp, "Cannot find aol_block_traffic or invalid value!");
			goto fail;
		}
#endif

		cprintf("setupwizard=[%s] wan_status=[%s] host=[%s] domain=[%s] routername=[%s]\n",
			setupwizard, wan_status, hostname, domainname, routername);
		cprintf("mac=[%s][%s][%s][%s][%s][%s] dns=[%s][%s][%s][%s] [%s][%s][%s][%s] [%s][%s][%s][%s]\n",
			mac0,mac1,mac2,mac3,mac4,mac5,dns0_1,dns0_2,dns0_3,dns0_4,dns1_1,dns1_2,dns1_3,dns1_4,dns2_1,dns2_2,dns2_3,dns2_4);

		http_passwd = websGetVar(wp, "SWsysPasswd", NULL);
		http_passwdcom = websGetVar(wp, "SWsysPasswdconfirm", NULL);

		if(!wan_status)			// Only set wireless parameters	2005-05-20 by honor
			goto valid_wireless;

		if(!valid_range(wp, wan_status, &which[9])){
			show_error(wp, "Cannot find wan_status or invalid value!");
			goto fail;
		}

		if(atoi(wan_status) == 0 ){	// DHCP mode	
			cprintf("Wan Status: DHCP mode\n");
			strcpy(wan_proto,"dhcp");
		}
		else if(atoi(wan_status) == 1 ){	// static mode
			cprintf("Wan Status: static mode\n");
			strcpy(wan_proto,"static");
		}
		else if(atoi(wan_status) == 2 ){	// pppoe mode
			cprintf("Wan Status: PPPoE mode\n");
			strcpy(wan_proto,"pppoe");
			ppp_username = websGetVar(wp, "SWpppoeUName", NULL);
			ppp_passwd = websGetVar(wp, "SWpppoePWD", NULL);
			cprintf("ppp_username=[%s] ppp_passwd=[%s]\n",ppp_username,ppp_passwd);
		}
		else if(atoi(wan_status) == 3 ){	// pptp mode
			cprintf("Wan Status: PPTP mode\n");
			strcpy(wan_proto,"pptp");
			ppp_username = websGetVar(wp, "SWpptpUName", NULL);
			ppp_passwd = websGetVar(wp, "SWpptpPWD", NULL);
			cprintf("ppp_username=[%s] ppp_passwd=[%s]\n",ppp_username,ppp_passwd);
		}
		else if(atoi(wan_status) == 4 ){	// l2tp mode
			cprintf("Wan Status: L2TP mode\n");
			strcpy(wan_proto,"l2tp");
			ppp_username = websGetVar(wp, "SWl2tpUName", NULL);
			ppp_passwd = websGetVar(wp, "SWl2tpPWD", NULL);
			cprintf("ppp_username=[%s] ppp_passwd=[%s]\n",ppp_username,ppp_passwd);
		}
		else if(atoi(wan_status) == 5 ){	// telstra mode
			cprintf("Wan Status: Telstra Cable mode\n");
			strcpy(wan_proto,"heartbeat");
			ppp_username = websGetVar(wp, "SWtelstraUName", NULL);
			ppp_passwd = websGetVar(wp, "SWtelstraPWD", NULL);
			cprintf("ppp_username=[%s] ppp_passwd=[%s]\n",ppp_username,ppp_passwd);
		}
		else {
			cprintf("Invalid wan_status: [%s]\n", wan_status);
			goto fail;
		}

		if(atoi(wan_status) == 0 ||
		   atoi(wan_status) == 1 ||
		   atoi(wan_status) == 3 ||
		   atoi(wan_status) == 4 ||
		   atoi(wan_status) == 5)  {
			wan_ip0 = websGetVar(wp, "SWaliasIP1", NULL);
			wan_ip1 = websGetVar(wp, "SWaliasIP2", NULL);
			wan_ip2 = websGetVar(wp, "SWaliasIP3", NULL);
			wan_ip3 = websGetVar(wp, "SWaliasIP4", NULL);
			wan_mask0 = websGetVar(wp, "SWaliasMaskIP0", NULL);
			wan_mask1 = websGetVar(wp, "SWaliasMaskIP1", NULL);
			wan_mask2 = websGetVar(wp, "SWaliasMaskIP2", NULL);
			wan_mask3 = websGetVar(wp, "SWaliasMaskIP3", NULL);
			wan_gw0 = websGetVar(wp, "SWrouterIP1", NULL);
			wan_gw1 = websGetVar(wp, "SWrouterIP2", NULL);
			wan_gw2 = websGetVar(wp, "SWrouterIP3", NULL);
			wan_gw3 = websGetVar(wp, "SWrouterIP4", NULL);

			if(wan_ip0 && wan_ip1 && wan_ip2 && wan_ip3){
				wan_ip = malloc(20);
				snprintf(buf,sizeof(buf),"%s.%s.%s.%s",wan_ip0, wan_ip1, wan_ip2, wan_ip3);
				strcpy(wan_ip,buf);
				cprintf("wan_ip=[%s]\n",wan_ip);
				if(!valid_ipaddr(wp, wan_ip, &which[4])){
					goto fail;
				}
			}
			if(wan_mask0 && wan_mask1 && wan_mask2 && wan_mask3){
				wan_mask = malloc(20);
				snprintf(buf,sizeof(buf),"%s.%s.%s.%s",wan_mask0, wan_mask1, wan_mask2, wan_mask3);
				strcpy(wan_mask,buf);
				cprintf("wan_mask=[%s]\n",wan_mask);
				if(!valid_ipaddr(wp, wan_mask, &which[5])){
					goto fail;
				}
			}
			if(wan_gw0 && wan_gw1 && wan_gw2 && wan_gw3){
				wan_gw = malloc(20);
				snprintf(buf,sizeof(buf),"%s.%s.%s.%s",wan_gw0, wan_gw1, wan_gw2, wan_gw3);
				strcpy(wan_gw,buf);
				cprintf("wan_gw=[%s]\n",wan_gw);
				if(!valid_ipaddr(wp, wan_gw, &which[6])){
					goto fail;
				}
			}
		}

		if(atoi(wan_status) == 2 ||
		   atoi(wan_status) == 3 ||
		   atoi(wan_status) == 4 ||
		   atoi(wan_status) == 5) {
			keepalive = websGetVar(wp, "SWkaliveStatus", NULL);
			alivetime = websGetVar(wp, "SWkaliveTime", NULL);
			cprintf("keepalive=[%s] alivetime=[%s]\n", keepalive, alivetime);

			if(!valid_choice(wp, keepalive, &which[17])){
				goto fail;
			}

			if(atoi(keepalive) == 1) {
				if(!valid_range(wp, alivetime, &which[18])){
       		         	        goto fail; 
		                }
			}
			else if(atoi(keepalive) == 2) {
				if(!valid_range(wp, alivetime, &which[19])){
					goto fail; 
	                        } 
	                }
		}

		/* valid wan MAC */
		if(mac0 && mac1 && mac2 && mac3 && mac4 && mac5 && strcmp(mac0,"")){
			mac = malloc(20);
			snprintf(buf,sizeof(buf),"%s:%s:%s:%s:%s:%s",mac0,mac1,mac2,mac3,mac4,mac5);
			strcpy(mac,buf);
			cprintf("mac=[%s]\n",mac);
			if(!valid_hwaddr(wp, mac, &which[0])){
				cprintf("MAC format is error!\n");
				goto fail;
			}
		}
	
		/* valid DNS 1 */
		if(dns0_1 && dns0_2 && dns0_3 && dns0_4){
			dns0 = malloc(20);
			snprintf(buf,sizeof(buf),"%s.%s.%s.%s",dns0_1, dns0_2, dns0_3, dns0_4);
			strcpy(dns0,buf);
			cprintf("dns0=[%s]\n",dns0);
			if(!valid_ipaddr(wp, dns0, &which[1])){
				goto fail;
			}
		}
		/* valid DNS 2 */
		if(dns1_1 && dns1_2 && dns1_3 && dns1_4){
			dns1 = malloc(20);
			snprintf(buf,sizeof(buf),"%s.%s.%s.%s",dns1_1, dns1_2, dns1_3, dns1_4);
			strcpy(dns1,buf);
			cprintf("dns1=[%s]\n",dns1);
			if(!valid_ipaddr(wp, dns1, &which[2])){
				goto fail;
			}
		}
		/* valid DNS 3 */
		if(dns2_1 && dns2_2 && dns2_3 && dns2_4){
			dns2 = malloc(20);
			snprintf(buf,sizeof(buf),"%s.%s.%s.%s",dns2_1, dns2_2, dns2_3, dns2_4);
			strcpy(dns2,buf);
			cprintf("dns2=[%s]\n",dns2);
			if(!valid_ipaddr(wp, dns2, &which[3])){
				goto fail;
			}
		}
	

		if(dns0 && strcmp(dns0,"0.0.0.0"))
			snprintf(dns+strlen(dns), sizeof(dns),"%s ",dns0);
		if(dns1 && strcmp(dns1,"0.0.0.0"))
			snprintf(dns+strlen(dns), sizeof(dns),"%s ",dns1);
		if(dns2 && strcmp(dns2,"0.0.0.0"))
			snprintf(dns+strlen(dns), sizeof(dns),"%s",dns2);

		cprintf("Validing http password\n");
		if(http_passwd && http_passwdcom){
			if(strcmp(http_passwd, http_passwdcom)){
				show_error(wp, "Invalid passwd!");
				goto fail;
			}
		}
	}

	/* Set wireless relative variable */
valid_wireless:

	if(atoi(set_mode) == 0 || atoi(set_mode) == 2)
	{
		wl_ssid = websGetVar(wp, "SWwirelessESSID", NULL);
		wl_gmode = websGetVar(wp, "SWwirelessMode", NULL);
		wl_net_mode = websGetVar(wp, "SWwirelessNetMode", NULL);
		wl_channel = websGetVar(wp, "SWwirelessChannel", NULL);
		wl_bcast = websGetVar(wp, "SWbroadcastSSID", "0");	// Setup Wizard 2.0 don't need

		wl_status = websGetVar(wp, "SWwirelessStatus", NULL);
		wl_key = websGetVar(wp, "SWwepKey", NULL);
		wl_wep_bit = websGetVar(wp, "SWwepEncryption", NULL);
		wl_passphrase = websGetVar(wp, "SWpassphrase", NULL);
		wl_key1 = websGetVar(wp, "SWwepKey1", NULL);
		wl_wpa_psk = websGetVar(wp, "SWwpaPSK", NULL);
		wl_encryption = websGetVar(wp, "SWwlEncryption", NULL);

		cprintf("http_passwd=[%s] http_passwdcom=[%s]\n",
			http_passwd, http_passwdcom);
		cprintf("wl_ssid=[%s] wl_channel=[%s] wl_bcast=[%s] wl_gmode=[%s] wl_net_mode=[%s]\n",
			wl_ssid, wl_channel, wl_bcast, wl_gmode, wl_net_mode);
		cprintf("wl_status=[%s] wl_encryption=[%s] wl_key=[%s] wl_wep_bit=[%s] wl_passphrase=[%s] wl_key1=[%s], wl_wpa_psk=[%s]\n",
			wl_status, wl_encryption, wl_key, wl_wep_bit, wl_passphrase, wl_key1, wl_wpa_psk);


		cprintf("Validing Wireless Channel\n");
		if(wl_channel && !valid_range(wp, wl_channel, &which[7])){
			show_error(wp, "Cannot find wireless channel or invalid value!");
			goto fail;
		}
		cprintf("Validing Wireless SSID Broadcast\n");
		if(wl_bcast && !valid_range(wp, wl_bcast, &which[8])){
			show_error(wp, "Cannot find wireless broadcast or invalid value!");
			goto fail;
		}
		cprintf("Validing Wireless Encryption\n");
		if(wl_encryption && !valid_choice(wp, wl_encryption, &which[14])){
			show_error(wp, "Cannot find wireless encryption or invalid value!");
			goto fail;
		}
		cprintf("Validing Wireless Mode\n");
		// Only for Parental Control
		if(wl_gmode && !valid_choice(wp, wl_gmode, &which[15])){
			show_error(wp, "Wireless Mode have invalid value!");
			goto fail;
		}
		// Only for Parental Control
		if(wl_net_mode && !valid_choice(wp, wl_net_mode, &which[16])){
			show_error(wp, "Wireless Net Mode have invalid value!");
			goto fail;
		}
	
		if(wl_status && atoi(wl_status) == 1){	// wep mode
			cprintf("Wireless Auth Mode: WEP mode\n");
			if(!wl_status || !valid_range(wp, wl_status, &which[10])){
				show_error(wp, "Cannot find wireless status or invalid value!");
				goto fail;
			}
			if(!wl_key || !valid_range(wp, wl_key, &which[11])){
				show_error(wp, "Cannot find wireless key or invalid value!");
				goto fail;
			}
			if(!wl_wep_bit || !valid_range(wp, wl_wep_bit, &which[12])){
				show_error(wp, "Cannot find wireless wep bit or invalid value!");
				goto fail;
			}
		}
		else if(wl_status && atoi(wl_status) == 2){	// psk mode
			cprintf("Wireless Auth Mode: PSK mode\n");
			if(!wl_wpa_psk)	
				cprintf("Warning: Cannot find wireless WPA PSK!\n");
	
		}
		//for wpa2 ***************************************************************************************
		else if(wl_status && atoi(wl_status) == 3){	// psk2 only mode
			cprintf("Wireless Auth Mode: PSK2 only mode\n");
			if(!wl_wpa_psk)	
				cprintf("Warning: Cannot find wireless WPA2 PSK!\n");
	
		}
		else if(wl_status && atoi(wl_status) == 4){	// psk mixed mode
			cprintf("Wireless Auth Mode: PSK2 mixed mode\n");
			if(!wl_wpa_psk)	
				cprintf("Warning: Cannot find wireless WPA2 PSK!\n");
			
		}
		//for wpa2 ***************************************************************************************
		else
			cprintf("Wireless Auth Mode: Disabled mode\n");

		cprintf("Valid OK, writing settings to nvram\n");
	}
      
        /********************************************************************************************************************************/
	/* Write value to nvram */
	if(atoi(set_mode) == 0 || atoi(set_mode) == 1)
	{
		/* basic setting */
		cprintf("Write basic settings\n");

		if(!wan_status)
			goto write_wireless;

		mynvram_set("wan_proto",wan_proto);
		if(hostname)
			mynvram_set("wan_hostname",hostname);
		if(domainname)
			mynvram_set("wan_domain",domainname);
		if(routername)
			mynvram_set("router_name",routername);
		if(dns0 && dns1 && dns2)
			mynvram_set("wan_dns",dns);
		if(mac){
			mynvram_set("def_hwaddr",mac);
			mynvram_set("mac_clone_enable","1");
		}

		if(lan_ip)
			mynvram_set("lan_ipaddr", lan_ip);

		/* for static mode */
		if(atoi(wan_status) == 1) {
			cprintf("Write static ip settings\n");
			if(wan_ip)
				mynvram_set("wan_ipaddr",wan_ip);
			if(wan_mask)
				mynvram_set("wan_netmask",wan_mask);
			if(wan_gw)
				mynvram_set("wan_gateway",wan_gw);
		}
		/* for pptp mode */
		else if(atoi(wan_status) == 3) {
			cprintf("Write pptp ip settings\n");
			if(wan_ip)
				mynvram_set("wan_ipaddr",wan_ip);
			if(wan_mask)
				mynvram_set("wan_netmask",wan_mask);
			if(wan_gw)
				mynvram_set("pptp_server_ip",wan_gw);
		}
		/* for l2tp mode */
		else if(atoi(wan_status) == 4) {
			cprintf("Write l2tp ip settings\n");
			if(wan_ip)
				mynvram_set("l2tp_server_ip",wan_ip);
		}
		/* for heartbeat mode */
		else if(atoi(wan_status) == 5) {
			cprintf("Write heartbeat ip settings\n");
			if(wan_ip)
				mynvram_set("hb_server_ip",wan_ip);
		}

		if(atoi(wan_status) == 2 ||
		   atoi(wan_status) == 3 ||
		   atoi(wan_status) == 4 ||
		   atoi(wan_status) == 5 ) {
			cprintf("Write pppoe/pptp/l2tp/telstra settings\n");
			if(ppp_username)
				mynvram_set("ppp_username",ppp_username);
			if(ppp_passwd)
				mynvram_set("ppp_passwd",ppp_passwd);
			if(keepalive) {
				if(atoi(keepalive) == 1) {	// Keep Alive
					mynvram_set("ppp_demand", "0");
					mynvram_set("ppp_redialperiod", alivetime);
				}
				else if(atoi(keepalive) == 2) {	// Connect On Demand
					mynvram_set("ppp_demand", "1");
					mynvram_set("ppp_idletime", alivetime);
				}
			}
		}

		mynvram_set("http_passwd",http_passwd);

		/* for aol */
#ifdef AOL_SUPPORT
		cprintf("Write AOL settings\n");
		if(atoi(aol_block_traffic) != 0)
			mynvram_set("aol_block_traffic2","1");
		else
			mynvram_set("aol_block_traffic2","0");
#endif
	}

write_wireless:
	if(atoi(set_mode) == 0 || atoi(set_mode) == 2) {
		/* for wireless */
		if(wl_gmode){
			if(!strcmp(wl_gmode, "-1")) {
				convert_wl_gmode("disabled");
				goto ok;
			}
			else if(!strcmp(wl_gmode, "0"))
				convert_wl_gmode("b-only");
			else if(!strcmp(wl_gmode, "1"))
				convert_wl_gmode("mixed");
			else if(!strcmp(wl_gmode, "2"))
				convert_wl_gmode("g-only");
		}

		if(wl_ssid)
			mynvram_set("wl_ssid",wl_ssid);
		if(wl_net_mode){
			convert_wl_gmode(wl_net_mode);
		}
		if(wl_channel)
			mynvram_set("wl_channel",wl_channel);
		if(wl_bcast)
			mynvram_set("wl_closed",wl_bcast);

		if(wl_status && atoi(wl_status) == 2){
			cprintf("Write PSK settings\n");
			if(wl_wpa_psk)
				mynvram_set("wl_wpa_psk", wl_wpa_psk);
			mynvram_set("wl_wep", "disabled");	// tkip or aes
			if(wl_encryption)
				mynvram_set("wl_crypto", wl_encryption);
			mynvram_set("wl_auth_mode","none");
			mynvram_set("security_mode","psk");
			mynvram_set("security_mode2","wpa_personal");
			mynvram_set("wl_akm", "psk");
		}
		else if(wl_status && atoi(wl_status) == 1){
			cprintf("Write WEP settings\n");
			mynvram_set("wl_wep","restricted");
			mynvram_set("wl_auth_mode","none");
			mynvram_set("security_mode","wep");
			mynvram_set("security_mode2","wep");
			if(wl_key)
		        	mynvram_set("wl_key",wl_key);
			if(wl_wep_bit)
		        	mynvram_set("wl_wep_bit",wl_wep_bit);
			if(wl_passphrase)
		        	mynvram_set("wl_passphrase",wl_passphrase);
			if(wl_key1)
		        	mynvram_set("wl_key1",wl_key1);
			snprintf(wl_wep_buf, sizeof(wl_wep_buf),"%s:%s::::%s",wl_passphrase,wl_key1,wl_key);
 	      		mynvram_set("wl_wep_buf",wl_wep_buf);
			mynvram_set("wl_akm", "");
		}
	//for wpa2 ***************************************************************************************
		else if(wl_status && atoi(wl_status) == 3){
			cprintf("Write PSK2 only settings\n");
			mynvram_set("wl_akm", "psk2");
			mynvram_set("wl_auth_mode", "none");
			mynvram_set("wl_wep", "disabled");
			mynvram_set("security_mode","psk2");
			mynvram_set("security_mode2","wpa2_personal");
			if(wl_wpa_psk)
				mynvram_set("wl_wpa_psk", wl_wpa_psk);
			if(wl_encryption)
				mynvram_set("wl_crypto", wl_encryption);
	
		}
		else if(wl_status && atoi(wl_status) == 4){
			cprintf("Write PSK2 mixed settings\n");
			mynvram_set("wl_akm", "psk psk2");
			mynvram_set("wl_auth_mode", "none");
			mynvram_set("wl_wep", "disabled");
			mynvram_set("security_mode","psk psk2");
			mynvram_set("security_mode2","wpa2_personal");
			if(wl_wpa_psk)
				mynvram_set("wl_wpa_psk", wl_wpa_psk);
			if(wl_encryption)
				mynvram_set("wl_crypto", wl_encryption);
		}
	//for wpa2 ***************************************************************************************
		else{
			cprintf("Write no security settings\n");
			mynvram_set("wl_wep","disabled");
			mynvram_set("wl_auth_mode","none");
			mynvram_set("security_mode","disabled");
			mynvram_set("security_mode2","disabled");
			mynvram_set("wl_crypto", "tkip");
			mynvram_set("wl_akm", "");
		}
	}

goto ok;
	
fail:
	error_value = 1; 
ok:
	if(mac)		free(mac);
	if(dns0)	free(dns0);
	if(dns1)	free(dns1);
	if(dns2)	free(dns2);
	if(wan_ip)	free(wan_ip);
	if(wan_mask)	free(wan_mask);
	if(wan_gw)	free(wan_gw);
	if(lan_ip)	free(lan_ip);
	
	return 0;

}

static int
setup_wizard(webs_t wp, char_t *urlPrefix, char_t *webDir, int arg, 
	     char_t *url, char_t *path, char_t *query)
{
	char *value;
	
	/* Below for production line test */
	value = websGetVar(wp, "sysAction_Reboot", NULL);
	if(value && atoi(value) == 1){
		do_ej("Success.asp",wp);
		websDone(wp, 200);
		sys_reboot();
		return 1;
	}

	value = websGetVar(wp, "sysAction_EraseNvram", NULL);
	if(value && atoi(value) == 1){
		ACTION("ACT_SW_RESTORE");
		do_ej("Success.asp",wp);
		websDone(wp, 200);
		eval("erase", "nvram");
		sys_reboot();
		return 1;
	}

	value = websGetVar(wp, "sysAction_NvramCommit", NULL);
	if(value && atoi(value) == 1){
		do_ej("Success.asp",wp);
		websDone(wp, 200);
		nvram_commit();
		return 1;
	}

	value = websGetVar(wp, "sysAction_FactoryDefault", NULL);
	if(value && atoi(value) == 1){
		ACTION("ACT_SW_RESTORE");
		mynvram_set("restore_defaults", "1");
                eval("killall", "-9", "udhcpc");
                sys_commit();
		do_ej("Success.asp",wp);
		websDone(wp, 200);
		sys_reboot();
		return 1;
	}

#ifdef SES_SUPPORT
	value = websGetVar(wp, "EnablePushButton", NULL);
	if(value && atoi(value) == 1){
		eval("killall", "ses");
		diag_led(SES_LED2,STOP_LED);
		buf_to_file("/tmp/EnablePushButton", "1");
		do_ej("Success.asp",wp);
		websDone(wp, 200);
		return 1;			
	}
#endif

	value = websGetVar(wp, "DetectWan", NULL);
	if(value && !strcmp(value,"1")) {
		struct detect_wans *detect = NULL;

		detect = detect_protocol(nvram_safe_get("wan_ifname"), nvram_safe_get("lan_ifname"),"AUTO");

		websWrite(wp, "Wan Protocol=%d;\r\n", detect->proto);	
		websWrite(wp, "Wan Protocol Count=%d;\r\n", detect->count);

		if(detect->proto == PROTO_DHCP)
			websWrite(wp, "Wan Protocol Name=DHCP;\r\n");
		else if(detect->proto == PROTO_STATIC)
			websWrite(wp, "Wan Protocol Name=Static;\r\n");
		else if(detect->proto == PROTO_PPPOE)
			websWrite(wp, "Wan Protocol Name=PPPoE;\r\n");
		else if(detect->proto == PROTO_ERROR)
			websWrite(wp, "Wan Protocol Name=ERROR;\r\n");
#ifdef EARTHLINK_SUPPORT
		else if(detect->proto == PROTO_EARTHLINK)
			websWrite(wp, "Wan Protocol Name=EARTHLINK;\r\n");
#endif

		websWrite(wp, "%s\r\n", detect->desc);

		if(detect)
			free(detect);

		return 0;
	}


#ifdef EOU_SUPPORT
	/* Verify eou key after burning */
	value = websGetVar(wp, "EOUKEY", NULL);
	if(value && atoi(value) == 1) {
		char *device_id = websGetVar(wp, "DEVICE_ID", NULL);
		char *public_key = websGetVar(wp, "PUBLIC_KEY", NULL);
		char *private_key = websGetVar(wp, "PRIVATE_KEY", NULL);

		if(!device_id || !public_key || !private_key) {
			show_error(wp, "Cann't find DEVICE_ID , PUBLIC_KEY or PRIVATE_KEY");	
			error_value = 1;
		}

		cprintf("device_id=[%s]\n", device_id);
		printASC(public_key, strlen(public_key));
		printASC(private_key, strlen(private_key));

		if(strcmp(device_id, nvram_safe_get("eou_device_id"))) {
			show_error(wp, "The device_id does not match");	
			error_value = 1;
		}
		else
			cprintf("The device_id match\n");
		
		if(strcmp(private_key, nvram_safe_get("eou_private_key"))) {
			show_error(wp, "The private_key does not match");	
			error_value = 1;
		}
		else
			cprintf("The private_key match\n");	
		
		if(strcmp(public_key, nvram_safe_get("eou_public_key"))) {
			show_error(wp, "The public_key does not match");	
			error_value = 1;
		}
		else
			cprintf("The public_key match\n");	
			
		goto Exit;	
	}
	else if(value && atoi(value) == 2) { //Added by Daniel (2004-08-26)
		//goto Exit;	
	}
#endif
		
	valid_value(wp);			// valid value

	if(error_value != 1){		// save to nvram if no any error
		sys_commit();	
		kill(1, SIGHUP);	// restart system
	}
Exit:
	if(!error_value)
        	do_ej("Success.asp",wp);
	else
        	do_ej("Fail.asp",wp);

	websDone(wp, 200);

	error_value = 0;

	return 1;
}

void
//do_setup_wizard(char *url, FILE *stream)
do_setup_wizard(char *url, webs_t stream) //jimmy, https, 8/4/2003
{
	char *path, *query;

#ifdef MY_DEBUG
	LOG(LOG_DEBUG,"%s(): url=[%s]",__FUNCTION__,url);
#endif
	query = url;
	path = strsep(&query, "?") ? : url;

	init_cgi(query); 
	setup_wizard(stream, NULL, NULL, 0, url, path, query);
	init_cgi(NULL); //Added by Daniel(2004-08-26)
}

void
show_error(webs_t wp, char *message)
{
	websWrite(wp, message);
	cprintf("%s\n", message);
}
#ifdef CYBERTAN_DEV
/* Example:
  hint:
  -- the first character of Gozila.cgi must be uppercase letter
  -- the SetupWizard value must be 1

  -- DHCP mode without wep key encryption
     http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-NB&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_wrt54g&SWwirelessChannel=11&SWbroadcastSSID=1&SWwirelessStatus=0

  -- DHCP mode with 128 bit wep key encryption
     http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-NB&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_wrt54g&SWwirelessChannel=11&SWbroadcastSSID=1&SWwirelessStatus=1&SWwepKey=1&SWwepEncryption=128&SWpassphrase=abcd&SWwepKey1=386f81fd57366030ae7ea0392a

==================================================================
Bellow for Setup Wizard 2.0

DHCP & PSK mode (tkip)
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=192&SWaliasIP2=168&SWaliasIP3=88&SWaliasIP4=200&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_123&SWwirelessChannel=5&SWwirelessStatus=2&SWwlEncryption=tkip&SWwpaPSK=1234567890&SWRouterName=WRT54G_123 

DHCP & PSK mode (aes)
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_111&SWwirelessChannel=1&SWwirelessStatus=2&SWwlEncryption=aes&SWwpaPSK=1111111111122222222223333333333&SWRouterName=WRT54G_111

DHCP & Disabled mode
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_222&SWwirelessChannel=6&SWwirelessStatus=0&SWwlEncryption=off&SWRouterName=WRT54G_222

DHCP & WEP 64bit
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_333&SWwirelessChannel=10&SWwirelessStatus=1&SWwlEncryption=wep&SWwepKey=1&SWwepEncryption=64&SWpassphrase=abcd&SWwepKey1=41192b542a&SWRouterName=WRT54G_333

DHCP & WEP 128bit
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_555&SWwirelessChannel=7&SWwirelessStatus=1&SWwlEncryption=wep&SWwepKey=1&SWwepEncryption=128&SWpassphrase=123&SWwepKey1=ae07938c6f3da12a9704d37241&SWRouterName=WRT54G_555

PPPoE & PSK
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWwanStatus=2&pppoeStatus=1&SWpppoeUName=84204099@hinet.net&SWpppoePWD=2xgigldl&MACClone_Status=0&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=&SWwanMac1=&SWwanMac2=&SWwanMac3=&SWwanMac4=&SWwanMac5=&SWwirelessESSID=linksys_666&SWwirelessChannel=5&SWwirelessStatus=2&SWwlEncryption=tkip&SWwpaPSK=1234567890&SWRouterName=WRT54G_666

SpeedBooster
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=testlab-wto54v5&SWDomainName=rrrrrrrrrrrrrrrrrrrrrrr&SWwanStatus=0&SWwanStatus=0&SWaliasIP1=0&SWaliasIP2=0&SWaliasIP3=0&SWaliasIP4=0&pppoeStatus=0&SWsysPasswd=aaa&SWsysPasswdconfirm=aaa&SWwanMac0=00&SWwanMac1=0C&SWwanMac2=6E&SWwanMac3=2C&SWwanMac4=65&SWwanMac5=BC&SWRouterName=WRT54GS&SWwirelessMode=6&SWwirelessESSID=eeeeeeeee&SWwirelessChannel=6&SWwirelessStatus=1&SWwlEncryption=wep&SWwepKey=1&SWwepEncryption=64&SWpassphrase=&SWwepKey1=1111111111

===========================================================================================================================================

PPTP & PSK mode (tkip)
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=3&SWaliasIP1=192&SWaliasIP2=168&SWaliasIP3=88&SWaliasIP4=200&SWaliasMaskIP0=255&SWaliasMaskIP1=255&SWaliasMaskIP2=255&SWaliasMaskIP3=0&SWrouterIP1=192&SWrouterIP2=168&SWrouterIP3=88&SWrouterIP4=39&pppoeStatus=0&SWkaliveStatus=1&SWkaliveTime=55&SWpptpUName=pptptest&SWpptpPWD=pptptest&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_123&SWwirelessChannel=5&SWwirelessStatus=2&SWwlEncryption=tkip&SWwpaPSK=1234567890&SWRouterName=WRT54G_123

L2TP & PSK mode (tkip)
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=4&SWaliasIP1=192&SWaliasIP2=168&SWaliasIP3=88&SWaliasIP4=200&pppoeStatus=0&SWkaliveStatus=2&SWkaliveTime=33&SWl2tpUName=l2tptest&SWl2tpPWD=l2tptest&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_123&SWwirelessChannel=5&SWwirelessStatus=2&SWwlEncryption=tkip&SWwpaPSK=1234567890&SWRouterName=WRT54G_123

HB & PSK mode (tkip)
http://192.168.1.1/Gozila.cgi?SetupWizard=1&SWhostName=honor-nb&SWDomainName=cybertan.com.tw&SWwanStatus=5&SWaliasIP1=192&SWaliasIP2=168&SWaliasIP3=88&SWaliasIP4=200&pppoeStatus=0&SWkaliveStatus=2&SWkaliveTime=60&SWtelstraUName=hbtest&SWtelstraPWD=hbtest&SWblock_traffic=0&SWsysPasswd=admin&SWsysPasswdconfirm=admin&SWwanMac0=00&SWwanMac1=00&SWwanMac2=39&SWwanMac3=A6&SWwanMac4=97&SWwanMac5=9B&SWwirelessESSID=linksys_123&SWwirelessChannel=5&SWwirelessStatus=2&SWwlEncryption=tkip&SWwpaPSK=1234567890&SWRouterName=WRT54G_123

*/

//Setup Wizard 2.0:
//-- Remove wireless broadcast
//-- Add Router Name
//-- Add WPA support
//-- Add SWwlEncryption, SWRouterName, SWwpaWPA
//-- Add SWwirelessMode for Parental Control

#endif
