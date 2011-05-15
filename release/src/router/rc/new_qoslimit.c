/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate
	rate limit & connection limit by conanxu
	2011 modified by Victek & Shibby for 2.6 kernel
	last changed: 20110210
*/

#include "rc.h"

#include <sys/stat.h>
static const char *qoslimitfn = "/etc/qoslimit";

/*int  chain
1 = MANGLE
2 = NAT
*/

#define IP_ADDRESS 0
#define MAC_ADDRESS 1
#define IP_RANGE 2

void address_checker (int * address_type, char *ipaddr_old, char *ipaddr)
{
	char * second_part, *last_dot;
	int length_to_minus, length_to_dot;
	
	second_part = strchr(ipaddr_old, '-');
	if (second_part != NULL) {
		/* ip range */
		*address_type = IP_RANGE;
		if (strchr(second_part+1, '.') != NULL) {
			/* long notation */
			strcpy (ipaddr, ipaddr_old);
		}
		else {
			/* short notation */
			last_dot = strrchr(ipaddr_old, '.');
			length_to_minus = second_part - ipaddr_old;
			length_to_dot = last_dot- ipaddr_old;
			strncpy(ipaddr, ipaddr_old, length_to_minus + 1);
			strncpy(ipaddr + length_to_minus + 1, ipaddr, length_to_dot + 1);
			strcpy(ipaddr + length_to_minus + length_to_dot + 2, second_part +1); 
		}
	}
	else {
		/* mac address of ipaddres */
		if (strlen(ipaddr_old) != 17) {
			/* IP_ADDRESS */
			*address_type = IP_ADDRESS;
		}
		else {
			/* MAC ADDRESS */
			*address_type = MAC_ADDRESS;
		}
		strcpy (ipaddr, ipaddr_old);
	}	
}
		
void ipt_qoslimit(int chain)
{
	char *buf;
	char *g;
	char *p;
	char *ibw,*obw;//bandwidth
	char seq[4];//mark number
	int iSeq = 10;
	char *ipaddr_old;
	char ipaddr[30];//ip address
	char *dlrate,*dlceil;//guaranteed rate & maximum rate for download
	char *ulrate,*ulceil;//guaranteed rate & maximum rate for upload
	char *priority;//priority
	char *lanipaddr; //lan ip address
	char *lanmask; //lan netmask
	char *tcplimit,*udplimit;//tcp connection limit & udp packets per second
	char *laninface; // lan interface
	int priority_num;
	char *qosl_tcp,*qosl_udp;
	int i, address_type;

	//qos1 is enable
	if (!nvram_get_int("new_qoslimit_enable")) return;
	
	//read qos1rules from nvram
	g = buf = strdup(nvram_safe_get("new_qoslimit_rules"));

	ibw = nvram_safe_get("qos_ibw");  // Read from QOS setting - KRP
	obw = nvram_safe_get("qos_obw");  // Read from QOS setting - KRP
	
	lanipaddr = nvram_safe_get("lan_ipaddr");
	lanmask = nvram_safe_get("lan_netmask");
	laninface = nvram_safe_get("lan_ifname");
	
	qosl_tcp = nvram_safe_get("qosl_tcp");
	qosl_udp = nvram_safe_get("qosl_udp");
	
	//MANGLE
	if (chain == 1)
	{
		ipt_write(
			"-A PREROUTING -j IMQ -i %s --todev 0\n"
			"-A POSTROUTING -j IMQ -o %s --todev 1\n"
			,laninface,laninface);
		if (nvram_get_int("qosl_enable") == 1) {
			ipt_write(
			"-A POSTROUTING ! -s %s/%s -j MARK --set-mark 100\n"
			"-A PREROUTING  ! -d %s/%s -j MARK --set-mark 100\n"
			,lanipaddr,lanmask
			,lanipaddr,lanmask);
		}
	}
	
	//NAT
	if (chain == 2)
	{
		if (nvram_get_int("qosl_enable") == 1) {
			if (nvram_get_int("qosl_tcp") > 0) {
				ipt_write(
					"-A PREROUTING -s %s/%s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n"
				,lanipaddr,lanmask,qosl_tcp);
			}
			
			if (nvram_get_int("qosl_udp") > 0) {
				ipt_write(
					"-A PREROUTING -s %s/%s -p udp -m limit --limit %s/sec -j ACCEPT\n"
				,lanipaddr,lanmask,qosl_udp);
			}
		}
	}
	
	while (g) {
		/*
		ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit
		*/
		if ((p = strsep(&g, ">")) == NULL) break;
		i = vstrsep(p, "<", &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit);
		if (i!=8) continue;

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5)) continue;

		if (!strcmp(ipaddr_old,"")) continue;
		
		address_checker (&address_type, ipaddr_old, ipaddr);
		sprintf(seq,"%d",iSeq);
		iSeq++; 

		if (!strcmp(dlceil,"")) strcpy(dlceil, dlrate);
		if (strcmp(dlrate,"") && strcmp(dlceil, "")) {
			if(chain == 1) {
				switch (address_type)
				{
					case IP_ADDRESS:
						ipt_write(
							"-A POSTROUTING ! -s %s/%s -d %s -j MARK --set-mark %s\n"
							,lanipaddr,lanmask,ipaddr,seq);
						break;
					case MAC_ADDRESS:
						break;
					case IP_RANGE:
						ipt_write(
							"-A POSTROUTING ! -s %s/%s -m iprange --dst-range  %s -j MARK --set-mark %s\n"
							,lanipaddr,lanmask,ipaddr,seq);
						break;
				}
			}
		}
		
		if (!strcmp(ulceil,"")) strcpy(ulceil, ulrate);
		if (strcmp(ulrate,"") && strcmp(ulceil, "")) {
			if (chain == 1) {
				switch (address_type)
				{
					case IP_ADDRESS:
						ipt_write(
							"-A PREROUTING -s %s ! -d %s/%s -j MARK --set-mark %s\n"
							,ipaddr,lanipaddr,lanmask,seq);
						break;
					case MAC_ADDRESS:
						ipt_write(
							"-A PREROUTING -m mac --mac-source %s ! -d %s/%s  -j MARK --set-mark %s\n"
							,ipaddr,lanipaddr,lanmask,seq);
						break;
					case IP_RANGE:
						ipt_write(
							"-A PREROUTING -m iprange --src-range %s ! -d %s/%s -j MARK --set-mark %s\n"
							,ipaddr,lanipaddr,lanmask,seq);
						break;
				}
			}
		}
		
		if(atoi(tcplimit) > 0){
			if (chain == 2) {
				switch (address_type)
				{
						case IP_ADDRESS:
							ipt_write(
							"-A PREROUTING -s %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n"
							,ipaddr,tcplimit);
							break;
						case MAC_ADDRESS:
							ipt_write(
							"-A PREROUTING -m mac --mac-source %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n"
							,ipaddr,tcplimit);
							break;
						case IP_RANGE:
							ipt_write(
							"-A PREROUTING -m iprange --src-range %s -p tcp --syn -m connlimit --connlimit-above %s -j DROP\n"
							,ipaddr,tcplimit);
							break;
				}
			}
		}
		if(atoi(udplimit) > 0){
			if (chain == 2) {
				switch (address_type)
				{
					case IP_ADDRESS:
						ipt_write(
							"-A PREROUTING -s %s -p udp -m limit --limit %s/sec -j ACCEPT\n"
							,ipaddr,udplimit);
						break;
					case MAC_ADDRESS:
						ipt_write(
							"-A PREROUTING -m iprange --src-range %s -p udp -m limit --limit %s/sec -j ACCEPT\n"
							,ipaddr,udplimit);
						break;
					case IP_RANGE:
						ipt_write(
							"-A PREROUTING -m iprange --src-range %s -p udp -m limit --limit %s/sec -j ACCEPT\n"
							,ipaddr,udplimit);
						break;
				}
			}
		}
	}
	free(buf);
}

// read nvram into files
void new_qoslimit_start(void)
{
	FILE *tc;
	char *buf;
	char *g;
	char *p;
	char *ibw,*obw;//bandwidth
	char seq[4];//mark number
	int iSeq = 10;
	char *ipaddr_old; 
	char ipaddr[30];//ip address
	char *dlrate,*dlceil;//guaranteed rate & maximum rate for download
	char *ulrate,*ulceil;//guaranteed rate & maximum rate for upload
	char *priority;//priority
	char *lanipaddr; //lan ip address
	char *lanmask; //lan netmask
	char *tcplimit,*udplimit;//tcp connection limit & udp packets per second
	int priority_num;
	char *dlr,*dlc,*ulr,*ulc; //download / upload - rate / ceiling
	int i, address_type;
	int s[6];

	//qos1 is enable
	if (!nvram_get_int("new_qoslimit_enable")) return;

	//read qos1rules from nvram
	g = buf = strdup(nvram_safe_get("new_qoslimit_rules"));

	ibw = nvram_safe_get("qos_ibw");  
	obw = nvram_safe_get("qos_obw");  
	
	lanipaddr = nvram_safe_get("lan_ipaddr");
	lanmask = nvram_safe_get("lan_netmask");

	dlr = nvram_safe_get("qosl_dlr"); //Qos limit download rate
	dlc = nvram_safe_get("qosl_dlc"); //download ceiling
	ulr = nvram_safe_get("qosl_ulr"); //upload rate
	ulc = nvram_safe_get("qosl_ulc"); //upload ceiling
	
	if ((tc = fopen(qoslimitfn, "w")) == NULL) return;

	fprintf(tc,
		"#!/bin/sh\n"
		"ip link set imq0 up\n"
		"ip link set imq1 up\n"
		"\n"
		"tc qdisc del dev imq0 root 2>/dev/null\n"
		"tc qdisc del dev imq1 root 2>/dev/null\n"
		"tc qdisc del dev br0 root 2>/dev/null\n" //fix me [why should mac get filter here??]
		"\n"
		"TCAM=\"tc class add dev br0\"\n" //fix me
		"TFAM=\"tc filter add dev br0\"\n" //fix me
		"TQAM=\"tc qdisc add dev br0\"\n" //fix me
		"\n"
		"TCA=\"tc class add dev imq1\"\n"
		"TFA=\"tc filter add dev imq1\"\n"
		"TQA=\"tc qdisc add dev imq1\"\n"
		"\n"
		"SFQ=\"sfq perturb 10\"\n"
		"\n"
		"TCAU=\"tc class add dev imq0\"\n"
		"TFAU=\"tc filter add dev imq0\"\n"
		"TQAU=\"tc qdisc add dev imq0\"\n"
		"\n"
		"tc qdisc add dev imq1 root handle 1: htb\n"
		"tc class add dev imq1 parent 1: classid 1:1 htb rate %skbit\n"
		"\n"
		"tc qdisc add dev br0 root handle 1: htb\n"
		"tc class add dev br0 parent 1: classid 1:1 htb rate %skbit\n"
		"\n"
		"tc qdisc add dev imq0 root handle 1: htb\n"
		"tc class add dev imq0 parent 1: classid 1:1 htb rate %skbit\n"
		"\n"
		,ibw,ibw,obw
	);
	
	if ((nvram_get_int("qosl_enable") == 1) && strcmp(dlr,"") && strcmp(ulr,"")) {
		if (!strcmp(dlc,"")) strcpy(dlc, dlr);
		if (!strcmp(ulc,"")) strcpy(ulc, ulr);
		fprintf(tc,
		"$TCA parent 1:1 classid 1:100 htb rate %skbit ceil %skbit prio 3\n"
		"$TQA parent 1:100 handle 100: $SFQ\n"
		"$TFA parent 1:0 prio 3 protocol ip handle 100 fw flowid 1:100\n"
		"\n"
		"$TCAU parent 1:1 classid 1:100 htb rate %skbit ceil %skbit prio 3\n"
		"$TQAU parent 1:100 handle 100: $SFQ\n"
		"$TFAU parent 1:0 prio 3 protocol ip handle 100 fw flowid 1:100\n"
		"\n"
		,dlr,dlc
		,ulr,ulc);
	}
		
	while (g) {
		/*
		ipaddr_old<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit
		*/
		if ((p = strsep(&g, ">")) == NULL) break;
		i = vstrsep(p, "<", &ipaddr_old, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit);
		if (i!=8) continue;

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5)) continue;

		if (!strcmp(ipaddr_old,"")) continue;
		
		address_checker(&address_type, ipaddr_old, ipaddr);
		sprintf(seq,"%d",iSeq);
		iSeq++;
		if (!strcmp(dlceil,"")) strcpy(dlceil, dlrate);
		if (strcmp(dlrate,"") && strcmp(dlceil, "")) {
			if (address_type != MAC_ADDRESS) {
				fprintf(tc,
					"$TCA parent 1:1 classid 1:%s htb rate %skbit ceil %skbit prio %s\n"
					"$TQA parent 1:%s handle %s: $SFQ\n"
					"$TFA parent 1:0 prio %s protocol ip handle %s fw flowid 1:%s\n"
					"\n"
					,seq,dlrate,dlceil,priority
					,seq,seq
					,priority,seq,seq);
			}
			else if (address_type == MAC_ADDRESS ) {
				sscanf(ipaddr, "%02X:%02X:%02X:%02X:%02X:%02X",&s[0],&s[1],&s[2],&s[3],&s[4],&s[5]);
				
				fprintf(tc,
					"$TCAM parent 1:1 classid 1:%s htb rate %skbit ceil %skbit prio %s\n"
					"$TQAM parent 1:%s handle %s: $SFQ\n"
					"$TFAM parent 1:0 protocol ip prio %s u32 match u16 0x0800 0xFFFF at -2 match u32 0x%02X%02X%02X%02X 0xFFFFFFFF at -12 match u16 0x%02X%02X 0xFFFF at -14 flowid 1:%s\n"
					"\n"
					,seq,dlrate,dlceil,priority
					,seq,seq
					,priority,s[2],s[3],s[4],s[5],s[0],s[1],seq);
			}
		}
		
		if (!strcmp(ulceil,"")) strcpy(ulceil, dlrate);
		if (strcmp(ulrate,"") && strcmp(ulceil, "")) {
			fprintf(tc,
				"$TCAU parent 1:1 classid 1:%s htb rate %skbit ceil %skbit prio %s\n"
				"$TQAU parent 1:%s handle %s: $SFQ\n"
				"$TFAU parent 1:0 prio %s protocol ip handle %s fw flowid 1:%s\n"
				"\n"
				,seq,ulrate,ulceil,priority
				,seq,seq
				,priority,seq,seq);
		}
	}
	free(buf);

	fclose(tc);
	chmod(qoslimitfn, 0700);
	
	//fake start
	eval((char *)qoslimitfn, "start");
}

void new_qoslimit_stop(void)
{
	FILE *f;
	char *s = "/tmp/qoslimittc_stop.sh";

	if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f,
		"#!/bin/sh\n"
		"tc qdisc del dev imq1 root\n"
		"tc qdisc del dev imq0 root\n"
		"tc qdisc del dev br0 root\n" //fix me
		"ip link set imq0 down\n"
		"ip link set imq1 down\n"
		"\n"
	);

	fclose(f);
	chmod(s, 0700);
	//fake stop
	eval((char *)s, "stop");
}
/*

PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
           QD         |                         ^
                      |                         |
                      v                         |
                    INPUT (f)                 OUTPUT (mnf)


*/
