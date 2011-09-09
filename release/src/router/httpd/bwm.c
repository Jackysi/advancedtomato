/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <sys/stat.h>

/*
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <typedefs.h>
*/

static const char *hfn = "/var/lib/misc/rstats-history.gz";
static const char *ifn = "/var/lib/misc/cstats-history.gz";

void wo_bwmbackup(char *url)
{
	struct stat st;
	time_t t;
	int i;

	if (stat(hfn, &st) == 0) {
		t = st.st_mtime;
		sleep(1);
	}
	else {
		t = 0;
	}
	killall("rstats", SIGHUP);
	for (i = 10; i > 0; --i) {
		if ((stat(hfn, &st) == 0) && (st.st_mtime != t)) break;
		sleep(1);
	}
	if (i == 0) {
		send_error(500, NULL, NULL);
		return;
	}
	send_header(200, NULL, mime_binary, 0);
	do_file((char *)hfn);
}

void wo_iptbackup(char *url)
{
	struct stat st;
	time_t t;
	int i;

	if (stat(ifn, &st) == 0) {
		t = st.st_mtime;
		sleep(1);
	}
	else {
		t = 0;
	}
	killall("cstats", SIGHUP);
	for (i = 10; i > 0; --i) {
		if ((stat(ifn, &st) == 0) && (st.st_mtime != t)) break;
		sleep(1);
	}
	if (i == 0) {
		send_error(500, NULL, NULL);
		return;
	}
	send_header(200, NULL, mime_binary, 0);
	do_file((char *)ifn);
}

void wi_bwmrestore(char *url, int len, char *boundary)
{
	char *buf;
	const char *error;
	int ok;
	int n;
	char tmp[64];

	check_id(url);

	tmp[0] = 0;
	buf = NULL;
	error = "Error reading file";
	ok = 0;

	if (!skip_header(&len)) {
		goto ERROR;
	}

	if ((len < 64) || (len > 10240)) {
		goto ERROR;
	}

	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

	n = web_read(buf, len);
	len -= n;

	sprintf(tmp, "%s.new", hfn);
	if (f_write(tmp, buf, n, 0, 0600) != n) {
		unlink(tmp);
		error = "Error writing temporary file";
		goto ERROR;
	}
	f_write("/var/tmp/rstats-load", NULL, 0, 0, 0600);
	killall("rstats", SIGHUP);
	sleep(1);

	error = NULL;
	rboot = 1;	// used as "ok"

ERROR:
	free(buf);
	web_eat(len);
	if (error != NULL) resmsg_set(error);
}

void wi_iptrestore(char *url, int len, char *boundary)
{
	char *buf;
	const char *error;
	int ok;
	int n;
	char tmp[64];

	check_id(url);

	tmp[0] = 0;
	buf = NULL;
	error = "Error reading file";
	ok = 0;

	if (!skip_header(&len)) {
		goto ERROR;
	}

	if ((len < 64) || (len > 10240)) {
		goto ERROR;
	}

	if ((buf = malloc(len)) == NULL) {
		error = "Not enough memory";
		goto ERROR;
	}

	n = web_read(buf, len);
	len -= n;

	sprintf(tmp, "%s.new", ifn);
	if (f_write(tmp, buf, n, 0, 0600) != n) {
		unlink(tmp);
		error = "Error writing temporary file";
		goto ERROR;
	}
	f_write("/var/tmp/cstats-load", NULL, 0, 0, 0600);
	killall("cstats", SIGHUP);
	sleep(1);

	error = NULL;
	rboot = 1;	// used as "ok"

ERROR:
	free(buf);
	web_eat(len);
	if (error != NULL) resmsg_set(error);
}

void wo_bwmrestore(char *url)
{
	if (rboot) {
		redirect("/bwm-daily.asp");
	}
	else {
		parse_asp("error.asp");
	}
}

void wo_iptrestore(char *url)
{
	if (rboot) {
		redirect("/ipt-daily.asp");
	}
	else {
		parse_asp("error.asp");
	}
}

void asp_netdev(int argc, char **argv)
{
	FILE *f;
	char buf[256];
	unsigned long rx, tx;
	char *p;
	char *ifname;
	char comma;
	char *exclude;

	exclude = nvram_safe_get("rstats_exclude");
	web_puts("\n\nnetdev={");
	if ((f = fopen("/proc/net/dev", "r")) != NULL) {
		fgets(buf, sizeof(buf), f);	// header
		fgets(buf, sizeof(buf), f);	// "
		comma = ' ';
		while (fgets(buf, sizeof(buf), f)) {
			if ((p = strchr(buf, ':')) == NULL) continue;
			*p = 0;
			if ((ifname = strrchr(buf, ' ')) == NULL) ifname = buf;
				else ++ifname;
//			if (strncmp(ifname, "ppp", 3) == 0) ifname = "ppp";
			if ((strcmp(ifname, "lo") == 0) || (find_word(exclude, ifname))) continue;

			// <rx bytes, packets, errors, dropped, fifo errors, frame errors, compressed, multicast><tx ...>
			if (sscanf(p + 1, "%lu%*u%*u%*u%*u%*u%*u%*u%lu", &rx, &tx) != 2) continue;
			web_printf("%c'%s':{rx:0x%lx,tx:0x%lx}", comma, ifname, rx, tx);
			comma = ',';
		}
		fclose(f);
	}
	web_puts("};\n");
}

void asp_climon(int argc, char **argv) {
	FILE *ti;
	FILE *to;

	char bi[512];
	char bo[512];

	char ip[64];
	unsigned long rx, tx;

	char comma;

	web_puts("\n\nclimon={");
	if ((to = popen("iptables -vnxL traffic_out", "r")) != NULL) {
		if ((ti = popen("iptables -vnxL traffic_in", "r")) != NULL) {
		comma = ' ';
		while ((fgets(bi, sizeof(bi), ti)) && (fgets(bo, sizeof(bo), to))) {
			if( (sscanf(bi, "%*s %lu %*s %*s %*s %*s %*s %s", &rx, ip)!=2) ||
				(sscanf(bo, "%*s %lu %*s %*s %*s %*s %*s %*s", &tx)!=1) ) continue;
				web_printf("%c'%s':{rx:0x%lx,tx:0x%lx}", comma, ip, rx, tx);
				comma = ',';
			}
		}
		pclose(ti);
	}
	pclose(to);
	web_puts("};\n");
}

void asp_iptmon(int argc, char **argv) {

	char comma;
	char sa[256];
	FILE *a;
	char *exclude;
	char *include;

	char ip[INET6_ADDRSTRLEN];

	unsigned long tx, rx;

	exclude = nvram_safe_get("cstats_exclude");
	include = nvram_safe_get("cstats_include");

	char br;
	char name[] = "/proc/net/ipt_account/lanX";

	web_puts("\n\niptmon={");
	comma = ' ';

	for(br=0 ; br<=3 ; br++) {

		char wholenetstatsline = 1;

		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(name, "/proc/net/ipt_account/lan%s", bridge);

		if ((a = fopen(name, "r")) == NULL) continue;

		if (!wholenetstatsline)
			fgets(sa, sizeof(sa), a); // network

		while (fgets(sa, sizeof(sa), a)) {
			if(sscanf(sa, 
				"ip = %s bytes_src = %lu %*u %*u %*u %*u packets_src = %*u %*u %*u %*u %*u bytes_dest = %lu %*u %*u %*u %*u packets_dest = %*u %*u %*u %*u %*u time = %*u",
				ip, &tx, &rx) != 3 ) continue;

			if (find_word(exclude, ip)) {
				wholenetstatsline = 0;
				continue;
			}

			if ((find_word(include, ip)) || (wholenetstatsline == 1)) {
//			if ((tx > 0) || (rx > 0) || (wholenetstatsline == 1)) {
//			if ((tx > 0) || (rx > 0)) {
				web_printf("%c'%s':{rx:0x%lx,tx:0x%lx}", comma, ip, rx, tx);
				comma = ',';
			}
			wholenetstatsline = 0;
		}
		fclose(a);
	}
	web_puts("};\n");
}

void asp_bandwidth(int argc, char **argv)
{
	char *name;
	int sig;

	if ((nvram_get_int("rstats_enable") == 1) && (argc == 1)) {
		if (strcmp(argv[0], "speed") == 0) {
			sig = SIGUSR1;
			name = "/var/spool/rstats-speed.js";
		}
		else {
			sig = SIGUSR2;
			name = "/var/spool/rstats-history.js";
		}
		unlink(name);
		killall("rstats", sig);
		f_wait_exists(name, 5);
		do_file(name);
		unlink(name);
	}
}

void asp_ipt_bandwidth(int argc, char **argv)
{
	char *name;
	int sig;

	if ((nvram_get_int("cstats_enable") == 1) && (argc == 1)) {
		if (strcmp(argv[0], "speed") == 0) {
			sig = SIGUSR1;
			name = "/var/spool/cstats-speed.js";
		}
		else {
			sig = SIGUSR2;
			name = "/var/spool/cstats-history.js";
		}
		unlink(name);
		killall("cstats", sig);
		f_wait_exists(name, 5);
		do_file(name);
		unlink(name);
	}
}

void asp_iptraffic(int argc, char **argv) {

	char comma;
	char sa[256];
	char sb[256];
	FILE *a;
	FILE *b;
	char *p;
	char ip[INET6_ADDRSTRLEN];

	char *exclude;

	unsigned long tx_bytes, rx_bytes;
	unsigned long tp_tcp, rp_tcp;
	unsigned long tp_udp, rp_udp;
	unsigned long tp_icmp, rp_icmp;
	unsigned long ct_tcp, ct_udp;

	exclude = nvram_safe_get("cstats_exclude");

// needs extra tweaks in the code before this works as it should
// so we'll stick to IPv4-only for now...
// #if defined(TCONFIG_IPV6) && defined(LINUX26)
//	const char conntrack[] = "/proc/net/nf_conntrack";
// #else
	const char conntrack[] = "/proc/net/ip_conntrack";
// #endif

	if ((b = fopen(conntrack, "r")) == NULL) return;

	char br;
	char name[] = "/proc/net/ipt_account/lanX";

	web_puts("\n\niptraffic=[");
	comma = ' ';

	for(br=0 ; br<=3 ; br++) {
		char bridge[2] = "0";
		if (br!=0)
			bridge[0]+=br;
		else
			strcpy(bridge, "");

		sprintf(name, "/proc/net/ipt_account/lan%s", bridge);

		if ((a = fopen(name, "r")) == NULL) continue;

		fgets(sa, sizeof(sa), a); // network
		while (fgets(sa, sizeof(sa), a)) {
			if(sscanf(sa, 
				"ip = %s bytes_src = %lu %*u %*u %*u %*u packets_src = %*u %lu %lu %lu %*u bytes_dest = %lu %*u %*u %*u %*u packets_dest = %*u %lu %lu %lu %*u time = %*u",
				ip, &tx_bytes, &tp_tcp, &tp_udp, &tp_icmp, &rx_bytes, &rp_tcp, &rp_udp, &rp_icmp) != 9 ) continue;

			if (find_word(exclude, ip)) continue ;

			if ((tx_bytes > 0) || (rx_bytes > 0)){
				rewind(b);
				ct_tcp = 0;
				ct_udp = 0;
				while (fgets(sb, sizeof(sb), b)) {
					if ((p = strstr(sb, ip)) == NULL) continue;
					if (strncmp(sb, "tcp", 3) == 0) ct_tcp++;
					if (strncmp(sb, "udp", 3) == 0) ct_udp++;
				}
				web_printf("%c['%s', %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu, %lu]", 
							comma, ip, rx_bytes, tx_bytes, rp_tcp, tp_tcp, rp_udp, tp_udp, rp_icmp, tp_icmp, ct_tcp, ct_udp);
				comma = ',';
			}
		}
		fclose(a);
	}
	web_puts("];\n");
}

