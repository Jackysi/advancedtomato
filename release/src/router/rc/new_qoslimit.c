/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate
	rate limit & connection limit by conanxu
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
	char *tcplimit,*udplimit;//tcp connection limit & udp packets per second
	int priority_num;
	char *s = "/tmp/new_qoslimit_start.sh";
	char *argv[3];
	int pid;
	int i;

	//qos1 is enable
	if (!nvram_get_int("new_qoslimit_enable")) return;

	//read qos1rules from nvram
	g = buf = strdup(nvram_safe_get("new_qoslimit_rules"));

	ibw = nvram_safe_get("new_qoslimit_ibw");
	obw = nvram_safe_get("new_qoslimit_obw");

	//read qos1rules into file /tmp/new_qoslimit.sh
	if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f,
		"#!/bin/sh\n"
		"TCA=\"tc class add dev br0\"\n"
		"TFA=\"tc filter add dev br0\"\n"
		"TQA=\"tc qdisc add dev br0\"\n"
		"SFQ=\"sfq perturb 10\"\n"
		"tc qdisc del dev br0 root\n"
		"tc qdisc add dev br0 root handle 1: htb\n"
		"tc class add dev br0 parent 1: classid 1:1 htb rate %skbit\n"
		"\n"
		"TCAU=\"tc class add dev imq0\"\n"
		"TFAU=\"tc filter add dev imq0\"\n"
		"TQAU=\"tc qdisc add dev imq0\"\n"
		"modprobe imq\n"
		"modprobe ipt_IMQ\n"
		"ip link set imq0 up\n"
		"tc qdisc del dev imq0 root\n"
		"tc qdisc add dev imq0 root handle 1: htb\n"
		"tc class add dev imq0 parent 1: classid 1:1 htb rate %skbit\n"
		"iptables -t mangle -A PREROUTING -j IMQ --todev 0\n"
		"\n"
		,ibw,obw
	);
	
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
				"iptables -t mangle -A POSTROUTING -d %s -j MARK --set-mark %s\n"
				"\n"
				,seq,dlrate,dlceil,priority
				,seq,seq
				,priority,seq,seq
				,ipaddr,seq);
		}
		
		if (ulceil == "") strcpy(ulceil, ulrate);
		if (ulrate != "" && ulceil != "") {
			fprintf(f,
				"$TCAU parent 1:1 classid 1:%s htb rate %skbit ceil %skbit prio %s\n"
				"$TQAU parent 1:%s handle %s: $SFQ\n"
				"$TFAU parent 1:0 prio %s protocol ip handle %s fw flowid 1:%s\n"
				"iptables -t mangle -A PREROUTING -s %s -j MARK --set-mark %s\n"
				"\n"
				,seq,ulrate,ulceil,priority
				,seq,seq
				,priority,seq,seq
				,ipaddr,seq);
		}
		
		if(atoi(tcplimit) > 0){
			fprintf(f,
				"iptables -I FORWARD -s %s -p tcp -m connlimit --connlimit-above %s -j DROP\n"
				"\n"
				,ipaddr,tcplimit);
		}
		
		if(atoi(udplimit) > 0){
			fprintf(f,
				"iptables -I FORWARD -s %s -p udp -m limit --limit %s/sec -j DROP\n"
				"\n"
				,ipaddr,udplimit);
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
	char *argv[3];
	int pid;

	if ((f = fopen(s, "w")) == NULL) return;

	fprintf(f,
		"#!/bin/sh\n"
		"tc qdisc del dev br0 root\n"
		"tc qdisc del dev imq0 root\n"
		"\n"
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
