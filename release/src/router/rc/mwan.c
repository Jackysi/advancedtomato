/*
* Multi WAN
* By Arctic QQ:317869867 E-Mail:zengchen228@vip.qq.com
*/

#include "rc.h"

#define mwanlog(level,x...) if(nvram_get_int("mwan_debug")>=level) syslog(level, x)

#ifdef TCONFIG_MULTIWAN
static char mwan_curr[] = {'0', '0', '0', '0', '\0'};
static char mwan_last[] = {'0', '0', '0', '0', '\0'};
#else
static char mwan_curr[] = {'0', '0', '\0'};
static char mwan_last[] = {'0', '0', '\0'};
#endif
typedef struct
{
	char wan_iface[10];
	char wan_ifname[10];
	char wan_ipaddr[32];
	char wan_netmask[32];
	char wan_gateway[32];
	const dns_list_t *dns;
	int wan_weight;
}waninfo_t;

static waninfo_t wan_info;

void get_wan_prefix(int iWan_unit, char *sPrefix)
{
	if(1 == iWan_unit) strcpy(sPrefix, "wan");
	else if(2 == iWan_unit) strcpy(sPrefix, "wan2");
#ifdef TCONFIG_MULTIWAN
	else if(3 == iWan_unit) strcpy(sPrefix, "wan3");
	else if(4 == iWan_unit) strcpy(sPrefix, "wan4");
#endif
	else strcpy(sPrefix, "wan");
}

int get_wan_unit(char *sPrefix)
{
	if(!strcmp(sPrefix,"wan")) return 1;
	else if(!strcmp(sPrefix,"wan2")) return 2;
#ifdef TCONFIG_MULTIWAN
	else if(!strcmp(sPrefix,"wan3")) return 3;
	else if(!strcmp(sPrefix,"wan4")) return 4;
#endif
	else return 1;
}

void get_wan_info(char *sPrefix)
{
	char tmp[100];

	strncpy(wan_info.wan_iface, nvram_safe_get(strcat_r(sPrefix, "_iface",tmp)), sizeof(wan_info.wan_iface));
	strncpy(wan_info.wan_ifname, nvram_safe_get(strcat_r(sPrefix, "_ifname",tmp)), sizeof(wan_info.wan_ifname));
	switch (get_wanx_proto(sPrefix)) {
		case WP_L2TP:
		case WP_PPTP:
			strncpy(wan_info.wan_ipaddr, nvram_safe_get(strcat_r(sPrefix, "_ppp_get_ip",tmp)), sizeof(wan_info.wan_ipaddr));
			break;
		case WP_PPPOE:
			if (using_dhcpc(sPrefix))
				strncpy(wan_info.wan_ipaddr, nvram_safe_get(strcat_r(sPrefix, "_ppp_get_ip",tmp)), sizeof(wan_info.wan_ipaddr));
			else
				strncpy(wan_info.wan_ipaddr, nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)), sizeof(wan_info.wan_ipaddr));
			break;
		default:
			strncpy(wan_info.wan_ipaddr, nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)), sizeof(wan_info.wan_ipaddr));
			break;
	}
	strncpy(wan_info.wan_netmask, nvram_safe_get(strcat_r(sPrefix, "_netmask",tmp)), sizeof(wan_info.wan_netmask));
	strncpy(wan_info.wan_gateway, wan_gateway(sPrefix), sizeof(wan_info.wan_gateway));
	wan_info.dns = get_dns(sPrefix);	// static buffer
	wan_info.wan_weight = atoi(nvram_safe_get(strcat_r(sPrefix, "_weight",tmp)));
}

void get_cidr(char *ipaddr, char *netmask, char *cidr){
	struct in_addr in_ipaddr, in_netmask, in_network;
	int netmask_bit = 0;
	unsigned long int bits = 1;

	inet_aton(ipaddr, &in_ipaddr);
	inet_aton(netmask, &in_netmask);

	int i;
	for(i = 1; i < sizeof(bits) * 8; i++){
		if(in_netmask.s_addr & bits) netmask_bit++;
		bits = bits << 1;
	}

	in_network.s_addr = in_ipaddr.s_addr & in_netmask.s_addr;
	sprintf(cidr, "%s/%d", inet_ntoa(in_network), netmask_bit);
}

int checkConnect(char *sPrefix)
{
	char tmp[100];
	FILE *f;
	int result;

	if(check_wanup(sPrefix)){
		get_wan_info(sPrefix);
		mwanlog(LOG_DEBUG, "IN checkConnect, wan_prefix=%s, wan_iface=%s", sPrefix, wan_info.wan_iface);
		if(nvram_get_int("mwan_cktime") == 0){
			return 1;
		}

		sprintf(tmp, "/tmp/state_%s", sPrefix);
		f = fopen(tmp, "r");
		fscanf (f, "%d", &result);
		fclose(f);

		if (result == 1) {
			mwanlog(LOG_DEBUG, "OUT checkConnect, %s is connected", sPrefix);
			return 1;
		} else {
			mwanlog(LOG_DEBUG, "OUT checkConnect, %s is disconnected", sPrefix);
			return 0;
		}

	} else {
		mwanlog(LOG_EMERG, "OUT checkConnect, %s is disconnected", sPrefix);
		return 0;
	}
}

void mwan_table_del(char *sPrefix)
{
	mwanlog(LOG_DEBUG, "IN fun mwan_table_del");

	int wan_unit,table;
	int i;
	char cmd[256];

	wan_unit = table = get_wan_unit(sPrefix);
	get_wan_info(sPrefix);

	// ip rule del table WAN1 pref 101 (gateway)
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref 10%d", table, wan_unit);
	mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
	system(cmd);
	
	// ip rule del table WAN1 pref 111 (dns)
#ifdef TCONFIG_MULTIWAN
	for (i = 0 ; i < 3; ++i) {
#else
	for (i = 0 ; i < 1; ++i) {
#endif
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule del table %d pref 11%d", table, wan_unit);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
		system(cmd);
	}

	// ip rule del fwmark 0x100/0xf00 table 1 pref 121 (mark)
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref 12%d", table, wan_unit);
	mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
	system(cmd);

	mwanlog(LOG_DEBUG, "OUT fun mwan_table_del");
}

// set multiwan ip route table & ip rule table
void mwan_table_add(char *sPrefix)
{
	mwanlog(LOG_DEBUG, "IN fun mwan_table_add");

	int mwan_num;
	int proto;
	int wan_unit,table;
	//char dns[32];
	char cmd[256];
	int wanid;
	char ip_cidr[32];
	int i;

	// delete already table first
	mwan_table_del(sPrefix);

	wan_unit = table = get_wan_unit(sPrefix);
	mwan_num = nvram_get_int("mwan_num");
	if((mwan_num == 1 || mwan_num > MWAN_MAX)) return;

	get_wan_info(sPrefix);
	proto = get_wanx_proto(sPrefix);
	
	if(check_wanup(sPrefix)){
		// ip rule add from WAN_IP table route_id pref 10X
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule add from %s table %d pref 10%d", wan_info.wan_ipaddr, table, wan_unit);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
		system(cmd);
		
		// set the routing rules of DNS.
		for (i = 0 ; i < wan_info.dns->count; ++i) {
			memset(cmd, 0, 256);
			sprintf(cmd, "ip rule add to %s table %d pref 11%d", inet_ntoa(wan_info.dns->dns[i].addr), table, wan_unit);
			mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
			system(cmd);
		}

		// ip rule add fwmark 0x100/0xf00 table 1 pref 121
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule add fwmark 0x%d00/0xf00 table %d pref 12%d", wan_unit, table, wan_unit);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
		system(cmd);

		for(wanid = 1 ; wanid <= mwan_num; ++wanid){
			// ip route add 10.0.10.1 dev ppp3 proto kernel scope link table route_id
			memset(cmd, 0, 256);
			if(proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC){
				get_cidr(wan_info.wan_ipaddr, wan_info.wan_netmask, ip_cidr);
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", ip_cidr, wan_info.wan_iface, wan_info.wan_ipaddr, wanid);
			}
			else{
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", wan_info.wan_gateway, wan_info.wan_iface, wan_info.wan_ipaddr, wanid);
			}
			mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
			system(cmd);
		}

		// ip route add 192.168.1.0/24 dev br0 proto kernel scope link  src 192.168.1.1  table 2
		get_cidr(nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"), ip_cidr);
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", ip_cidr, nvram_safe_get("lan_ifname"), nvram_safe_get("lan_ipaddr"), table);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
		system(cmd);

		// ip route add 127.0.0.0/8 dev lo  scope link table 1
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append 127.0.0.0/8 dev lo scope link table %d", table);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
		system(cmd);			

		// ip route add default via 10.0.10.1 dev ppp3 table route_id
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append default via %s dev %s table %d",
			(proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) ? wan_info.wan_gateway : wan_info.wan_ipaddr,
			wan_info.wan_iface,
			table);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", sPrefix, cmd);
		system(cmd);
	}

	mwanlog(LOG_DEBUG, "OUT fun mwan_table_add");
}

void mwan_state_files(void)
{
	int mwan_num;
	int wan_unit;
	char prefix[] = "wanXX";
	char tmp[100];
	FILE *f;

	mwan_num = nvram_get_int("mwan_num");
	if(mwan_num == 1 || mwan_num > MWAN_MAX) return;
	for(wan_unit = 1; wan_unit <= mwan_num; ++wan_unit){
		get_wan_prefix(wan_unit, prefix);
		get_wan_info(prefix);

		sprintf(tmp, "/tmp/state_%s", prefix);
		if ( !(f = fopen(tmp, "r"))) {
			// if file does not exist then we create him will value "1"
			f = fopen(tmp, "w+");
			fprintf(f, "1\n");
			fclose(f);
		}
	}
}

void mwan_status_update(void)
{
	mwanlog(LOG_DEBUG, "IN fun mwan_status_update, mwan_curr=%s", mwan_curr);
	int mwan_num;
	int wan_unit;
	char prefix[] = "wanXX";
	char tmp[100];
	FILE *f;

	mwan_num = nvram_get_int("mwan_num");
	if(mwan_num == 1 || mwan_num > MWAN_MAX) return;
	for(wan_unit = 1; wan_unit <= mwan_num; ++wan_unit){
		get_wan_prefix(wan_unit, prefix);
		get_wan_info(prefix);

		if(checkConnect(prefix)){
			if(wan_info.wan_weight > 0){
				mwan_curr[wan_unit-1] = '2'; // connected, load balancing
			} else {
				mwan_curr[wan_unit-1] = '1'; // connected, failover
			}
		} else {
			mwan_curr[wan_unit-1] = '0'; // disconnected
		}
	}

	// write failover status to file
	f = fopen("/tmp/wan.failover", "w+");

#ifdef TCONFIG_MULTIWAN
	if ((mwan_curr[0] < '2') && (mwan_curr[1] < '2') && (mwan_curr[2] < '2') && (mwan_curr[3] < '2')) {
#else
	if ((mwan_curr[0] < '2') && (mwan_curr[1] < '2')) {
#endif
		// all connections down, searcing failover interfaces
		if (nvram_match("wan_weight", "0") && (mwan_curr[0] == '1')) {
			syslog(LOG_INFO, "mwan_status_update, failover in action - WAN1");
			mwan_curr[0] = '2';
			fprintf(f, "WAN:1\n");
		}
		if (nvram_match("wan2_weight", "0") && (mwan_curr[1] == '1')) {
			syslog(LOG_INFO, "mwan_status_update, failover in action - WAN2");
			mwan_curr[1] = '2';
			fprintf(f, "WAN2:1\n");
		}
#ifdef TCONFIG_MULTIWAN
		if (nvram_match("wan3_weight", "0") && (mwan_curr[2] == '1')) {
			syslog(LOG_INFO, "mwan_status_update, failover in action - WAN3");
			mwan_curr[2] = '2';
			fprintf(f, "WAN3:1\n");
		}
		if (nvram_match("wan4_weight", "0") && (mwan_curr[3] == '1')) {
			syslog(LOG_INFO, "mwan_status_update, failover in action - WAN4");
			mwan_curr[3] = '2';
			fprintf(f, "WAN4:1\n");
		}
#endif
	} else {
		fprintf(f, "NONE:0\n");
	}

	fclose(f);
	mwanlog(LOG_DEBUG, "OUT fun mwan_status_update, mwan_curr=%s", mwan_curr);
}

void mwan_load_balance(void)
{
	if(nvram_get_int("mwan_num") == 1) return;

	mwanlog(LOG_DEBUG, "IN fun mwan_load_balance, mwan_curr=%s", mwan_curr);
	int mwan_num,wan_unit;
	int proto;
	char prefix[] = "wanXX";
	char cmd[256];
	char lb_cmd[2048];
	mwan_num = nvram_get_int("mwan_num");
	if(mwan_num == 1 || mwan_num > MWAN_MAX) return;

	mwan_status_update();

	memset(lb_cmd, 0, 256);
	strncpy(lb_cmd, "ip route replace default scope global ", sizeof(lb_cmd));
	
	for(wan_unit = 1; wan_unit <= mwan_num; ++wan_unit){
		get_wan_prefix(wan_unit, prefix);
		get_wan_info(prefix);
		proto = get_wanx_proto(prefix);
		if(check_wanup(prefix) && (mwan_curr[wan_unit-1] == '2')){
			if(wan_info.wan_weight == 0) { // override weight for failover interface
				wan_info.wan_weight = 1;
			}
			if(wan_info.wan_weight > 0){
				memset(cmd, 0, 256);
				sprintf(cmd, " nexthop via %s dev %s weight %d",
					(proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) ? wan_info.wan_gateway : wan_info.wan_ipaddr,
					wan_info.wan_iface,
					wan_info.wan_weight);
				strcat(lb_cmd, cmd);
			}
		}
		// ip route del default via 10.0.10.1 dev ppp3 (from main route table)
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route del default via %s dev %s",
			(proto == WP_DHCP || proto == WP_LTE || proto == WP_STATIC) ? wan_info.wan_gateway : wan_info.wan_ipaddr,
			wan_info.wan_iface);
		mwanlog(LOG_DEBUG, "%s, cmd=%s", prefix, cmd);
		system(cmd);
	}
	mwanlog(LOG_DEBUG, "load_balance, cmd=%s", lb_cmd);
	system(lb_cmd);
	mwanlog(LOG_EMERG, "OUT fun mwan_load_balance, mwan_curr=%s", mwan_curr);
}

int mwan_route_main(int argc, char **argv)
{
	int check_time;
	int mwan_num;

	FILE *fp;

	mwanlog(LOG_DEBUG, "MultiWAN: mwanroute launched");

	mkdir("/etc/iproute2", 0744);
	if((fp = fopen("/etc/iproute2/rt_tables", "w")) != NULL) {
		fprintf(fp,
			"1 WAN1\n"
			"2 WAN2\n"
#ifdef TCONFIG_MULTIWAN
			"3 WAN3\n"
			"4 WAN4\n"
#endif
			);
		fclose(fp);
	}

	mwan_num = nvram_get_int("mwan_num");
	if(mwan_num == 1 || mwan_num > MWAN_MAX){
		return 0;
	}

	while(1){

		check_time = 30;
		mwan_status_update();
		
		if(strcmp(mwan_last,mwan_curr)){
			syslog(LOG_WARNING, "Multiwan status is changed, last_status=%s, now_status=%s, Update multiwan policy.", mwan_last, mwan_curr);
			mwan_load_balance();

			stop_dnsmasq();
			start_dnsmasq();
		}
		strcpy(mwan_last,mwan_curr);
		sleep(check_time);
	}
}
