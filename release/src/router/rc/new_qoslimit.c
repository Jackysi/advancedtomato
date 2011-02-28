/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate
	rate limit & connection limit by conanxu
	2011 modified by Victek & Shibby for 2.6 kernel
	last changed: 20110210
*/

#include "rc.h"

#include <sys/stat.h>


// read nvram into files
void new_qoslimit_start(void)
{
	FILE *f;
	char *buf;
	char *g;
	char *p;
	char *ibw,*obw;//bandwidth
	char *seq;//mark number
	char *ipaddr;//ip address
	char *dlrate,*dlceil;//guaranteed rate & maximum rate for download
	char *ulrate,*ulceil;//guaranteed rate & maximum rate for upload
	char *priority;//priority
	int priority_num;
	char *lanipaddr; //lan ip address
	char *lanmask; //lan netmask
	char *waniface; //wan interface - shibby
	char *d_dlr; //default DLrate - shibby
	char *d_dlc; //default DLceil - shibby
	char *d_ulr; //default ULrate - shibby
	char *d_ulc; //default ULceil - shibby
	char *tcplimit,*udplimit;//tcp connection limit & udp packets per second
	char *s = "/tmp/new_qoslimit_start.sh";
	char *argv[3];
	int pid;
	int i;

	//is qos tomato enable?
	if (nvram_get_int("qos_enable")) return;

	//qos1 is enable
	if (!nvram_get_int("new_qoslimit_enable")) return;

	//read qos1rules from nvram
	g = buf = strdup(nvram_safe_get("new_qoslimit_rules"));

	ibw = nvram_safe_get("new_qoslimit_ibw");
	obw = nvram_safe_get("new_qoslimit_obw");
	
	lanipaddr = nvram_safe_get("lan_ipaddr");
	lanmask = nvram_safe_get("lan_netmask");
	waniface = nvram_safe_get("wan_iface"); //shibby

	d_dlr = nvram_safe_get("new_qoslimit_d_dlr"); //shibby
	d_dlc = nvram_safe_get("new_qoslimit_d_dlc"); //shibby
	d_ulr = nvram_safe_get("new_qoslimit_d_ulr"); //shibby
	d_ulc = nvram_safe_get("new_qoslimit_d_ulc"); //shibby

	//read qos1rules into file /tmp/new_qoslimit.sh
	if ((f = fopen(s, "w")) == NULL) return;

	if (nvram_get_int("new_qoslimit_d_enable")) {
	fprintf(f,
		"#!/bin/sh\n"
		"TCA=\"tc class add dev br0\"\n"
		"TFA=\"tc filter add dev br0\"\n"
		"TQA=\"tc qdisc add dev br0\"\n"
		"SFQ=\"sfq perturb 10\"\n"
		"IPT_POST=\"iptables -t mangle -A POSTROUTING ! -s %s/%s\"\n"
		"tc qdisc del dev br0 root\n"
		"tc qdisc add dev br0 root handle 1: htb\n"
		"tc class add dev br0 parent 1: classid 1:1 htb rate %skbit\n"
		"$TCA parent 1:1 classid 1:99 htb rate %skbit ceil %skbit prio 3\n"
		"$TQA parent 1:99 handle 99: $SFQ\n"
		"$TFA parent 1:0 prio 3 protocol ip handle 99 fw flowid 1:99\n"
		"$IPT_POST -j MARK --set-mark 99\n"
		"\n"
		"TCAU=\"tc class add dev %s\"\n"
		"TFAU=\"tc filter add dev %s\"\n"
		"TQAU=\"tc qdisc add dev %s\"\n"
		"IPT_PRE=\"iptables -t mangle -A PREROUTING ! -d %s/%s\"\n"
		"tc qdisc del dev %s root\n"
		"tc qdisc add dev %s root handle 2: htb\n"
		"tc class add dev %s parent 2: classid 2:1 htb rate %skbit\n"
		"$TCAU parent 2:1 classid 2:99 htb rate %skbit ceil %skbit prio 3\n"
		"$TQAU parent 2:99 handle 99: $SFQ\n"
		"$TFAU parent 2:0 prio 3 protocol ip handle 99 fw flowid 2:99\n"
		"$IPT_PRE -j MARK --set-mark 99\n"
		"\n"
		,lanipaddr,lanmask
		,ibw
		,d_dlr,d_dlc
		,waniface
		,waniface
		,waniface
		,lanipaddr,lanmask
		,waniface
		,waniface
		,waniface,obw
		,d_ulr,d_ulc
	);
	} else {
	fprintf(f,
		"#!/bin/sh\n"
		"TCA=\"tc class add dev br0\"\n"
		"TFA=\"tc filter add dev br0\"\n"
		"TQA=\"tc qdisc add dev br0\"\n"
		"SFQ=\"sfq perturb 10\"\n"
		"IPT_POST=\"iptables -t mangle -A POSTROUTING ! -s %s/%s\"\n"
		"tc qdisc del dev br0 root\n"
		"tc qdisc add dev br0 root handle 1: htb\n"
		"tc class add dev br0 parent 1: classid 1:1 htb rate %skbit\n"
		"\n"
		"TCAU=\"tc class add dev %s\"\n"
		"TFAU=\"tc filter add dev %s\"\n"
		"TQAU=\"tc qdisc add dev %s\"\n"
		"IPT_PRE=\"iptables -t mangle -A PREROUTING ! -d %s/%s\"\n"
		"tc qdisc del dev %s root\n"
		"tc qdisc add dev %s root handle 2: htb\n"
		"tc class add dev %s parent 2: classid 2:1 htb rate %skbit\n"
		"\n"
		,lanipaddr,lanmask
		,ibw
		,waniface
		,waniface
		,waniface
		,lanipaddr,lanmask
		,waniface
		,waniface
		,waniface,obw
	);
	}
	
	while (g) {
		/*
		seq<ipaddr<dlrate<dlceil<ulrate<ulceil<priority<tcplimit<udplimit
		*/
		if ((p = strsep(&g, ">")) == NULL) break;
		i = vstrsep(p, "<", &seq, &ipaddr, &dlrate, &dlceil, &ulrate, &ulceil, &priority, &tcplimit, &udplimit);

		priority_num = atoi(priority);
		if ((priority_num < 0) || (priority_num > 5)) continue;

		if (ipaddr == "") continue;

		if (dlceil == "") strcpy(dlceil, dlrate);
		if (dlrate != "" && dlceil != "") {
			fprintf(f,
				"$TCA parent 1:1 classid 1:%s htb rate %skbit ceil %skbit prio %s\n"
				"$TQA parent 1:%s handle %s: $SFQ\n"
				"$TFA parent 1:0 prio %s protocol ip handle %s fw flowid 1:%s\n"
				,seq,dlrate,dlceil,priority
				,seq,seq
				,priority,seq,seq);
	}
		if (strchr(ipaddr, '-') != NULL) {
			fprintf(f,
				"$IPT_POST -m iprange --dst-range %s -j MARK --set-mark %s\n"
				"\n"
				,ipaddr, seq);
	}
		else {
			fprintf(f,
				"$IPT_POST -d %s -j MARK --set-mark %s\n"
				"\n"
		,ipaddr,seq);
 	}


		
		if (ulceil == "") strcpy(ulceil, ulrate);
		if (ulrate != "" && ulceil != "") {
			fprintf(f,
				"$TCAU parent 2:1 classid 2:%s htb rate %skbit ceil %skbit prio %s\n"
				"$TQAU parent 2:%s handle %s: $SFQ\n"
				"$TFAU parent 2:0 prio %s protocol ip handle %s fw flowid 2:%s\n"
				,seq,ulrate,ulceil,priority
				,seq,seq
				,priority,seq,seq);
	}

		if (strchr(ipaddr, '-') != NULL) {
			fprintf(f,
				"$IPT_PRE -m iprange --src-range %s -j MARK --set-mark %s\n"
				"\n"
 				,ipaddr,seq);
	}
		
		else {
			fprintf(f,
				"$IPT_PRE -s %s -j MARK --set-mark %s\n"
				"\n"
				,ipaddr,seq);
	}

		if(atoi(tcplimit) > 0){
			if (strchr(ipaddr, '-') != NULL) {
			fprintf(f,
			"iptables -I FORWARD -m iprange --src-range %s -p tcp -m connlimit --connlimit-above %s --connlimit-mask 32 -j REJECT\n"
					"\n"
					,ipaddr,tcplimit);
	}
			else {
			fprintf(f,
			"iptables -I FORWARD -s %s -p tcp -m connlimit --connlimit-above %s --connlimit-mask 32 -j REJECT\n"
					"\n"
					,ipaddr,tcplimit);
	}
 }
		
		if(atoi(udplimit) > 0){
		if (strchr(ipaddr, '-') != NULL) {
			fprintf(f,
				"iptables -I FORWARD -m iprange --src-range %s -p udp -m limit --limit %s/sec -j DROP\n"
					"\n"
					,ipaddr,udplimit);
	}
			else {
				fprintf(f,
				"iptables -I FORWARD -s %s -p udp -m limit --limit %s/sec -j DROP\n"
					"\n"
					,ipaddr,udplimit);
	}	
}
}
			free(buf);
			fclose(f);
			chmod(s, 0700);
			chdir("/tmp");
			argv[0] = s;
			argv[1] = NULL;
			argv[2] = NULL;
		if (_eval(argv, NULL, 0, &pid) != 0) {
		pid = -1;
	}
		else {
		kill(pid, 0);
	}
			
	chdir("/");
}

void new_qoslimit_stop(void)
{
			FILE *f;
			char *s = "/tmp/new_qoslimit_stop.sh";
			char *waniface; //shibby
			char *argv[3];
			int pid;
			waniface = nvram_safe_get("wan_iface"); //shibby

		if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f,
		"#!/bin/sh\n"
		"tc qdisc del dev br0 root\n"
		"tc qdisc del dev %s root\n"
		"\n"
		,waniface
	);

		fclose(f);
		chmod(s, 0700);
		chdir("/tmp");
		argv[0] = s;
		argv[1] = NULL;
		argv[2] = NULL;
	if (_eval(argv, NULL, 0, &pid) != 0) {
		pid = -1;
	}
	else {
		kill(pid, 0);
	}
			
	chdir("/");
}
/*

PREROUTING (mn) ----> x ----> FORWARD (f) ----> + ----> POSTROUTING (n)
           QD         |                         ^
                      |                         |
                      v                         |
                    INPUT (f)                 OUTPUT (mnf)


*/
