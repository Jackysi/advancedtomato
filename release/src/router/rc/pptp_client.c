/*
  PPTP CLIENT start/stop and configuration for Tomato
  by Jean-Yves Avenard (c) 2008-2011
*/

#include "rc.h"
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>

#define BUF_SIZE 128
#define IF_SIZE 8

// Line number as text string
#define __LINE_T__ __LINE_T_(__LINE__)
#define __LINE_T_(x) __LINE_T(x)
#define __LINE_T(x) # x

#define vpnlog(x...) syslog(LOG_DEBUG, __LINE_T__ ": " x)

void start_pptp_client(void)
{
    FILE *fd;
    int ok = 0;
    int i;
    char *p;
    char buffer[BUF_SIZE];
    char *argv[5];
    int argc = 0;

	struct hostent *he;
	struct in_addr **addr_list;
	
	char *prefix = nvram_safe_get("pptp_client_usewan");
	char *srv_addr = nvram_safe_get("pptp_client_srvip");
	
	if (nvram_get_int("pptp_client_enable") == 0){
		stop_pptp_client();
		return;
	}

    sprintf(buffer, "pptpclient");
    if ( pidof(buffer) >= 0 )
    {
        // PPTP already running
        //return;
		stop_pptp_client();
    }
    unlink("/etc/vpn/pptpc_ip-up");
    unlink("/etc/vpn/pptpc_ip-down");
    unlink("/etc/vpn/ip-vpn");
    unlink("/etc/vpn/options.vpn");
    unlink("/etc/vpn");
    unlink("/tmp/ppp");
    mkdir("/tmp/ppp",0700);
    mkdir("/etc/vpn",0700);
    ok |= symlink("/sbin/rc", "/etc/vpn/pptpc_ip-up");
    ok |= symlink("/sbin/rc", "/etc/vpn/pptpc_ip-down");
    // Make sure symbolic link exists
    sprintf(buffer, "/etc/vpn/pptpclient");
    unlink(buffer);
    ok |= symlink("/usr/sbin/pppd", buffer);

    if (ok)
    {
        stop_pptp_client();
        return;
    }

	if(inet_addr(srv_addr) == INADDR_NONE){
		if ((he = gethostbyname(srv_addr)) == NULL)
		{
			return;
		}
		addr_list = (struct in_addr **)he->h_addr_list;
		if (inet_ntoa(*addr_list[0]) == NULL){
			return;
		}
		srv_addr = inet_ntoa(*addr_list[0]);
	}

    if ( (fd = fopen("/etc/vpn/options.vpn", "w")) != NULL )
    {
        ok = 1;
        fprintf(fd,
            "lock\n"
            "noauth\n"
            "refuse-eap\n"
            "lcp-echo-failure 3\n"
            "lcp-echo-interval 2\n"
            "maxfail 0\n"
            "persist\n"
            "plugin pptp.so\n"
            "pptp_server %s\n", srv_addr);
        i = nvram_get_int("pptp_client_peerdns"); //0: disable, 1 enable
        if (i > 0)
            fprintf(fd,"usepeerdns\n");
        fprintf(fd,"idle 0\n"
            "ip-up-script /etc/vpn/pptpc_ip-up\n"
            "ip-down-script /etc/vpn/pptpc_ip-down\n"
            "ipparam kelokepptpd\n");

        if ((p = nvram_get("pptp_client_mtu")) == NULL)
            p = "1450";
        if (!nvram_get_int("pptp_client_mtuenable"))
            p = "1450";
        fprintf(fd,"mtu %s\n", p);

        if (!nvram_get_int("pptp_client_mruenable"))
        {
            if ((p = nvram_get("pptp_client_mru")) == NULL)
                p = "1450";
            fprintf(fd,"mru %s\n", p);
        }

        if ((p = nvram_get("pptp_client_username")) == NULL)
            ok = 0;
        else
            fprintf(fd,"user %s\n", p);

        if ((p = nvram_get("pptp_client_passwd")) == NULL)
            ok = 0;
        else
            fprintf(fd,"password %s\n", p);
		/*
        switch (get_wan_proto())
        {
            case WP_PPPOE:
            case WP_PPTP:
            case WP_L2TP:
                p = "1";
                break;
            default:
                p = "0";
                break;
        }
		*/
        strcpy(buffer,"");
        switch (nvram_get_int("pptp_client_crypt"))
        {
            case 1:
                fprintf(fd, "nomppe\n");
                break;
            case 2:
                fprintf(fd, "nomppe-40\n");
                fprintf(fd, "require-mppe-128\n");
                break;
            case 3:
                fprintf(fd, "require-mppe-40\n");
				fprintf(fd, "require-mppe-128\n");
                break;
            default:
                break;
        }
        if (!nvram_get_int("pptp_client_stateless"))
			fprintf(fd, "mppe-stateful\n");
		else
            fprintf(fd, "nomppe-stateful\n");
        fprintf(fd, "unit 4\n");
        fprintf(fd, "%s\n", nvram_safe_get("pptp_client_custom"));
        fclose(fd);
    }
    if (ok)
    {
        // force route to PPTP server via select wan
		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "ip rule del lookup %d pref 120", get_wan_unit(prefix));
		//syslog(LOG_DEBUG, "cmd=%s", buffer);
		system(buffer);			

		memset(buffer, 0, sizeof(buffer));
		sprintf(buffer, "ip rule add to %s lookup %d pref 120", srv_addr, get_wan_unit(prefix));
		//syslog(LOG_DEBUG, "cmd=%s", buffer);
		system(buffer);			

		//eval("ip", "rule", "del", "lookup", (char *)get_wan_unit(prefix), "pref", "120");
		//eval("ip", "rule", "add", "to", srv_addr, "lookup", (char *)get_wan_unit(prefix), "pref", "120");
		
		sprintf(buffer, "/etc/vpn/pptpclient file /etc/vpn/options.vpn");
        for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
        if ( _eval(argv, NULL, 0, NULL) )
        {
            stop_pptp_client();
            return;
        }
		f_write("/etc/vpn/pptp_client_connecting", NULL, 0, 0, 0);
    }
    else
        stop_pptp_client();
}


void stop_pptp_client(void)
{
    int argc;
    char *argv[8];
    char buffer[BUF_SIZE];

    //killall("pptpclient", SIGTERM);
	eval("/etc/vpn/pptpc_ip-down");
	killall_tk("pptpc_ip-up");
	killall_tk("pptpc_ip-down");
	killall_tk("pptpclient");

    sprintf(buffer, "rm -rf /etc/vpn/pptpclient /etc/vpn/pptpc_ip-down /etc/vpn/pptpc_ip-up /etc/vpn/options.vpn /tmp/ppp/resolv.conf");
    for (argv[argc=0] = strtok(&buffer[0], " "); argv[argc] != NULL; argv[++argc] = strtok(NULL, " "));
    _eval(argv, NULL, 0, NULL);

    rmdir("/etc/vpn");
    rmdir("/tmp/ppp");
}

void clear_pptp_route(void)
{
	/*
	struct hostent *he;
	struct in_addr **addr_list;
	*/
	
	char *prefix = nvram_safe_get("pptp_client_usewan");
	char cmd[256];
	/*
	char *srv_addr = nvram_safe_get("pptp_client_srvip");

	if(inet_addr(srv_addr) == INADDR_NONE){
		if ((he = gethostbyname(srv_addr)) == NULL)
		{
			return;
		}
		addr_list = (struct in_addr **)he->h_addr_list;
		if (inet_ntoa(*addr_list[0]) == NULL){
			return;
		}
		srv_addr = inet_ntoa(*addr_list[0]);
	}

    // remove route to PPTP server
	eval("route", "del", srv_addr, "gw", wan_gateway(prefix), "dev", nvram_safe_get("pptp_client_iface"));
	*/
	//eval("ip", "rule", "del", "lookup", (char *)get_wan_unit(prefix), "pref", "120");
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "ip rule del lookup %d pref 120", get_wan_unit(prefix));
	//syslog(LOG_DEBUG, "cmd=%s", cmd);
	system(cmd);
}

int write_pptpvpn_resolv(FILE* f)
{
    FILE *dnsf;
    int usepeer;
    char ch;
  
    if ((usepeer = nvram_get_int("pptp_client_peerdns")) <= 0)
    {
        vpnlog("pptp peerdns disabled");
        return 0;
    }
    dnsf = fopen("/tmp/ppp/resolv.conf", "r");
    if (dnsf == NULL)
    {
        vpnlog("/tmp/ppp/resolv.conf can't be opened");
        return 0;
    }
    while( !feof(dnsf) )
    {
        ch = fgetc(dnsf);
		fputc(ch==EOF?'\n':ch, f);
    }
    fclose(dnsf);

    return (usepeer == 2) ? 1 : 0;
}

void pptp_rtbl_clear(void)
{
	int i;
	char cmd[256];
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref %d", PPTP_CLIENT_TABLE_ID, 115);
	for(i = 0; i <= 1000; i++){ // clean old rule
		system(cmd);
	}
}

void pptp_rtbl_export(void)
{
	char *nvp, *b;
	char cmd[256];

	pptp_rtbl_clear();
	nvp = strdup(nvram_safe_get("pptp_client_rtbl"));
	while ((b = strsep(&nvp, "\n")) != NULL) {
		if(!strcmp(b,"")) continue;
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule add to %s table %d pref %d", b, PPTP_CLIENT_TABLE_ID, 115);
		//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
		system(cmd);
	}
}

void pptp_client_table_del(void)
{
	char cmd[256];

	system("ip rule del from all to all lookup PPTP pref 120");

	// ip rule del from WAN_IP table route_id pref 10X
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route flush table %d", PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	// ip rule del from WAN_IP table route_id pref 10X
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref 10%d", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	// ip rule del to PPTP_DNS table route_id pref 110
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref 110", PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd); // del PPTP DNS1
	system(cmd); // del PPTP DNS2

	// ip rule del fwmark 0x500/0xf00 table 1 pref 125
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule del table %d pref 12%d", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);
}

// set pptp_client ip route table & ip rule table
void pptp_client_table_add(void)
{
	//syslog(LOG_DEBUG, "IN fun mwan_table_add");

	int mwan_num,i;
	char cmd[256];
	int wanid;
	char ip_cidr[32];
	char remote_cidr[32];
	char sPrefix[] = "wanXX";
	char tmp[100];
	int proto;
	
	char *pptp_client_ipaddr = nvram_safe_get("pptp_client_ipaddr");
	char *pptp_client_gateway = nvram_safe_get("pptp_client_gateway");
	char *pptp_client_iface = nvram_safe_get("pptp_client_iface");	
	const dns_list_t *pptp_dns = get_dns("pptp_client");

	pptp_client_table_del();

	mwan_num = nvram_get_int("mwan_num");

	if(nvram_get_int("pptp_client_dfltroute") == 1){
		system("ip rule add from all to all lookup PPTP pref 120");
	}
	else{
		system("ip rule del from all to all lookup PPTP pref 120");
	}
	
	// ip rule add from WAN_IP table route_id pref 10X
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule add from %s table %d pref 10%d", pptp_client_ipaddr, PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);
	
	for (i = 0 ; i < pptp_dns->count; ++i) {
		// ip rule add to PPTP_DNS table route_id pref 110
		memset(cmd, 0, 256);
		sprintf(cmd, "ip rule add to %s table %d pref 110", inet_ntoa(pptp_dns->dns[i].addr), PPTP_CLIENT_TABLE_ID);
		//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
		system(cmd);
	}

	// ip rule add fwmark 0x500/0xf00 table 1 pref 125
	memset(cmd, 0, 256);
	sprintf(cmd, "ip rule add fwmark 0x%d00/0xf00 table %d pref 12%d", PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_ID, PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	for(wanid = 1 ; wanid <= mwan_num; ++wanid){
		get_wan_prefix(wanid, sPrefix);
		if(check_wanup(sPrefix)){
			proto = get_wanx_proto(sPrefix);
			memset(cmd, 0, 256);
			if(proto == WP_DHCP || proto == WP_STATIC){
				get_cidr(nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)), nvram_safe_get(strcat_r(sPrefix, "_netmask",tmp)), ip_cidr);
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d",
					ip_cidr, 
					nvram_safe_get(strcat_r(sPrefix, "_iface",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)),
					PPTP_CLIENT_TABLE_ID);
			}
			else{
				sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d",
					wan_gateway(sPrefix), 
					nvram_safe_get(strcat_r(sPrefix, "_iface",tmp)),
					nvram_safe_get(strcat_r(sPrefix, "_ipaddr",tmp)),
					PPTP_CLIENT_TABLE_ID);
			}
			//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
			system(cmd);
		}
	}

	// ip route add 172.16.36.1 dev ppp4  proto kernel  scope link  src 172.16.36.13
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	for(wanid = 1 ; wanid <= mwan_num; ++wanid){
		get_wan_prefix(wanid, sPrefix);
		if(check_wanup(sPrefix)){
			memset(cmd, 0, 256);
			sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, wanid);
			//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
			system(cmd);
		}
	}

	// ip route add 172.16.36.1 dev ppp4  proto kernel  scope link  src 172.16.36.13 table 5
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", pptp_client_gateway, pptp_client_iface, pptp_client_ipaddr, PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	// ip route add 192.168.1.0/24 dev br0 proto kernel scope link  src 192.168.1.1  table 5
	get_cidr(nvram_safe_get("lan_ipaddr"), nvram_safe_get("lan_netmask"), ip_cidr);
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append %s dev %s proto kernel scope link src %s table %d", ip_cidr, nvram_safe_get("lan_ifname"), nvram_safe_get("lan_ipaddr"), PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	// ip route add 127.0.0.0/8 dev lo  scope link table 5
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append 127.0.0.0/8 dev lo scope link table %d", PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);			

	// ip route add default via 10.0.10.1 dev ppp3 table 5
	memset(cmd, 0, 256);
	sprintf(cmd, "ip route append default via %s dev %s table %d", pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_ID);
	//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
	system(cmd);

	if(!nvram_match("pptp_client_srvsub", "0.0.0.0") && !nvram_match("pptp_client_srvsubmsk", "0.0.0.0")){
		// add to table main
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr);
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append %s via %s dev %s scope link table %s", remote_cidr, pptp_client_ipaddr, pptp_client_iface, "main");
		//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
		system(cmd);

		// add to table WANX
		for(wanid = 1 ; wanid <= mwan_num; ++wanid){
			get_wan_prefix(wanid, sPrefix);
			if(check_wanup(sPrefix)){
				memset(cmd, 0, 256);
				sprintf(cmd, "ip route append %s via %s dev %s scope link table %d", remote_cidr, pptp_client_ipaddr, pptp_client_iface, wanid);
				//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
				system(cmd);
			}
		}
		
		// add to table PPTP
		get_cidr(nvram_safe_get("pptp_client_srvsub"), nvram_safe_get("pptp_client_srvsubmsk"), remote_cidr);
		memset(cmd, 0, 256);
		sprintf(cmd, "ip route append %s via %s dev %s scope link table %d", remote_cidr, pptp_client_ipaddr, pptp_client_iface, PPTP_CLIENT_TABLE_ID);
		//syslog(LOG_DEBUG, "pptp_client, cmd=%s", cmd);
		system(cmd);
	}
	
	//syslog(LOG_DEBUG, "OUT fun pptp_client_table_add");
}

int pptpc_ipup_main(int argc, char **argv)
{
	char *pptp_client_ifname;
	const char *p;
	char buf[256];
	struct sysinfo si;

	TRACE_PT("begin\n");

	sysinfo(&si);
	f_write("/etc/vpn/pptp_client_time", &si.uptime, sizeof(si.uptime), 0, 0);
	unlink("/etc/vpn/pptp_client_connecting");

	killall("listen", SIGKILL);
	
	if (!wait_action_idle(10)) return -1;

	// ipup receives six arguments:
	//   <interface name>  <tty device>  <speed> <local IP address> <remote IP address> <ipparam>
	//   ppp1 vlan1 0 71.135.98.32 151.164.184.87 0

	pptp_client_ifname = safe_getenv("IFNAME");
	if ((!pptp_client_ifname) || (!*pptp_client_ifname)) return -1;
	nvram_set("pptp_client_iface", pptp_client_ifname);	// ppp#
	nvram_set("pptp_client_pppd_pid", safe_getenv("PPPD_PID"));	

	f_write_string("/etc/vpn/pptp_client_link", argv[1], 0, 0);

	if ((p = getenv("IPLOCAL"))) {
		_dprintf("IPLOCAL=%s\n", p);
		nvram_set("pptp_client_ipaddr", p);
		nvram_set("pptp_client_netmask", "255.255.255.255");
	}

	if ((p = getenv("IPREMOTE"))) {
		_dprintf("IPREMOTE=%s\n", p);
		nvram_set("pptp_client_gateway", p);
	}

	buf[0] = 0;
	if ((p = getenv("DNS1")) != NULL){
		strlcpy(buf, p, sizeof(buf));
	}
	if ((p = getenv("DNS2")) != NULL) {
		if (buf[0]) strlcat(buf, " ", sizeof(buf));
		strlcat(buf, p, sizeof(buf));
	}
	nvram_set("pptp_client_get_dns", buf);
	TRACE_PT("DNS=%s\n", buf);

	pptp_client_table_add();
	pptp_rtbl_export();
	stop_firewall();
	start_firewall();
	
	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	TRACE_PT("end\n");
	return 0;
}

int pptpc_ipdown_main(int argc, char **argv)
{
	TRACE_PT("begin\n");

	if (!wait_action_idle(10)) return -1;

	pptp_client_table_del();
	clear_pptp_route();
	pptp_rtbl_clear();

	unlink("/etc/vpn/pptp_client_link");
	unlink("/etc/vpn/pptp_client_time");
	unlink("/etc/vpn/pptp_client_connecting");
	nvram_set("pptp_client_iface", "");
	nvram_set("pptp_client_pppd_pid", "");	
	nvram_set("pptp_client_ipaddr", "0.0.0.0");
	nvram_set("pptp_client_netmask", "0.0.0.0");
	nvram_set("pptp_client_gateway", "0.0.0.0");
	nvram_set("pptp_client_get_dns", "");

	killall("listen", SIGKILL);
	stop_firewall();
	start_firewall();

	stop_dnsmasq();
	dns_to_resolv();
	start_dnsmasq();

	return 1;
}
