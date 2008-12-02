/*

	Tomato Firmware
	Copyright (C) 2006-2008 Jonathan Zarate
	
*/

#include "rc.h"

#include <stdarg.h>
#include <sys/ioctl.h>
#include <net/if_arp.h>

#include <bcmdevs.h>
#include <wlutils.h>


void usage_exit(const char *cmd, const char *help)
{
	fprintf(stderr, "Usage: %s %s\n", cmd, help);
	exit(1);
}

int modprobe(const char *mod)
{
#if 1
	return eval("modprobe", "-s", (char *)mod);
#else
	int r = eval("modprobe", "-s", (char *)mod);
	cprintf("modprobe %s = %d\n", mod, r);
	return r;
#endif
}

int modprobe_r(const char *mod)
{
#if 1
	return eval("modprobe", "-r", (char *)mod);
#else
	int r = eval("modprobe", "-r", (char *)mod);
	cprintf("modprobe -r %s = %d\n", mod, r);
	return r;
#endif
}

int _xstart(const char *cmd, ...)
{
	va_list ap;
	char *argv[16];
	int argc;
	int pid;

	argv[0] = (char *)cmd;
	argc = 1;
	va_start(ap, cmd);
	while ((argv[argc++] = va_arg(ap, char *)) != NULL) {
		//
	}
	va_end(ap);

	return _eval(argv, NULL, 0, &pid);
}

void run_nvscript(const char *nv, const char *arg1, int wtime)
{
	FILE *f;
	char *script;
	char s[256];
	char *argv[3];
	int pid;

	script = nvram_get(nv);
	if ((script) && (*script != 0)) {
		sprintf(s, "/tmp/%s.sh", nv);
		if ((f = fopen(s, "w")) != NULL) {
			fputs("#!/bin/sh\n", f);
			fputs(script, f);
			fputs("\n", f);
			fclose(f);
			chmod(s, 0700);
			
			chdir("/tmp");

			argv[0] = s;
			argv[1] = (char *)arg1;
			argv[2] = NULL;
			if (_eval(argv, NULL, 0, &pid) != 0) {
				pid = -1;
			}
			else {
				while (wtime-- > 0) {
					if (kill(pid, 0) != 0) break;
					sleep(1);
				}
			}
			
			chdir("/");
		}
	}
}

void setup_conntrack(void)
{
	unsigned int v[10];
	const char *p;
	int i;
	
	p = nvram_safe_get("ct_tcp_timeout");
	if (sscanf(p, "%u%u%u%u%u%u%u%u%u%u",
		&v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9]) == 10) {	// lightly verify
		f_write_string("/proc/sys/net/ipv4/ip_conntrack_tcp_timeouts", p, 0, 0);
	}
	
	p = nvram_safe_get("ct_udp_timeout");
	if (sscanf(p, "%u%u", &v[0], &v[1]) == 2) {
		f_write_string("/proc/sys/net/ipv4/ip_conntrack_udp_timeouts", p, 0, 0);
	}
	
	p = nvram_safe_get("ct_max");
	i = atoi(p);
	if ((i >= 128) && (i <= 10240)) {
		f_write_string("/proc/sys/net/ipv4/ip_conntrack_max", p, 0, 0);
	}

	if (!nvram_match("nf_pptp", "0")) {
		modprobe("ip_conntrack_proto_gre");
		modprobe("ip_nat_proto_gre");
		modprobe("ip_conntrack_pptp");
		modprobe("ip_nat_pptp");
	}
	else {
		modprobe_r("ip_nat_pptp");
		modprobe_r("ip_conntrack_pptp");
		modprobe_r("ip_nat_proto_gre");
		modprobe_r("ip_conntrack_proto_gre");
	}

	if (!nvram_match("nf_h323", "0")) {
		modprobe("ip_conntrack_h323");
		modprobe("ip_nat_h323");
	}
	else {
		modprobe_r("ip_nat_h323");
		modprobe_r("ip_conntrack_h323");
	}
	
	if (!nvram_match("nf_rtsp", "0")) {
		modprobe("ip_conntrack_rtsp");
		modprobe("ip_nat_rtsp");
	}
	else {
		modprobe_r("ip_nat_rtsp");
		modprobe_r("ip_conntrack_rtsp");
	}

	if (!nvram_match("nf_ftp", "0")) {
		modprobe("ip_conntrack_ftp");
		modprobe("ip_nat_ftp");
	}
	else {
		modprobe_r("ip_nat_ftp");
		modprobe_r("ip_conntrack_ftp");
	}

}

void set_mac(const char *ifname, const char *nvname, int plus)
{
	int sfd;
	struct ifreq ifr;
	int up;
	int j;
	
	if ((sfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
		_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
		return;
	}

	strcpy(ifr.ifr_name, ifname);
	
	up = 0;
	if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0) {
		if ((up = ifr.ifr_flags & IFF_UP) != 0) {
			ifr.ifr_flags &= ~IFF_UP;
			if (ioctl(sfd, SIOCSIFFLAGS, &ifr) != 0) {
				_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
			}
		}
	}
	else {
		_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
	}
	
	if (!ether_atoe(nvram_safe_get(nvname), (unsigned char *)&ifr.ifr_hwaddr.sa_data)) {
		if (!ether_atoe(nvram_safe_get("et0macaddr"), (unsigned char *)&ifr.ifr_hwaddr.sa_data)) {

			// goofy et0macaddr, make something up
			nvram_set("et0macaddr", "00:01:23:45:67:89");
			ifr.ifr_hwaddr.sa_data[0] = 0;
			ifr.ifr_hwaddr.sa_data[1] = 0x01;
			ifr.ifr_hwaddr.sa_data[2] = 0x23;
			ifr.ifr_hwaddr.sa_data[3] = 0x45;
			ifr.ifr_hwaddr.sa_data[4] = 0x67;
			ifr.ifr_hwaddr.sa_data[5] = 0x89;
		}
		
		while (plus-- > 0) {
			for (j = 5; j >= 3; --j) {
				ifr.ifr_hwaddr.sa_data[j]++;
				if (ifr.ifr_hwaddr.sa_data[j] != 0) break;	// continue if rolled over
			}
		}
	}
	
	ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
	if (ioctl(sfd, SIOCSIFHWADDR, &ifr) == -1) {
		_dprintf("Error setting %s address\n", ifname);
	}

	if (up) {
		if (ioctl(sfd, SIOCGIFFLAGS, &ifr) == 0) {
			ifr.ifr_flags |= IFF_UP|IFF_RUNNING;
			if (ioctl(sfd, SIOCSIFFLAGS, &ifr) == -1) {
				_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
			}
		}
		else {
			_dprintf("%s: %s %d\n", ifname, __FUNCTION__, __LINE__);
		}
	}
	
	close(sfd);
}

const char *default_wanif(void)
{
	return ((strtoul(nvram_safe_get("boardflags"), NULL, 0) & BFL_ENETVLAN) ||
		(check_hw_type() == HW_BCM4712)) ? "vlan1" : "eth1";
}

const char *default_wlif(void)
{
	switch (check_hw_type()) {
	case HW_BCM4702:
	case HW_BCM4704_BCM5325F:
	case HW_BCM4704_BCM5325F_EWC:
		return "eth2";
	}
	return "eth1";
	
}

int _vstrsep(char *buf, const char *sep, ...)
{
	va_list ap;
	char **p;
	int n;
	
	n = 0;
	va_start(ap, sep);
	while ((p = va_arg(ap, char **)) != NULL) {
		if ((*p = strsep(&buf, sep)) == NULL) break;
		++n;
	}
	va_end(ap);
	return n;
}

void simple_unlock(const char *name)
{
	char fn[256];

	snprintf(fn, sizeof(fn), "/var/lock/%s.lock", name);
	f_write(fn, NULL, 0, 0, 0600);
}

void simple_lock(const char *name)
{
	int n;
	char fn[256];

	n = 5 + (getpid() % 10);
	snprintf(fn, sizeof(fn), "/var/lock/%s.lock", name);
	while (unlink(fn) != 0) {
		if (--n == 0) {
			syslog(LOG_DEBUG, "Breaking %s", fn);
			break;
		}
		sleep(1);
	}	
}

void killall_tk(const char *name)
{
	int n;
	
	if (killall(name, SIGTERM) == 0) {
		n = 5;
		while ((killall(name, 0) == 0) && (n-- > 0)) {
			_dprintf("%s: waiting name=%s n=%d\n", __FUNCTION__, name, n);
			sleep(1);
		}
		if (n < 0) {
			n = 5;
			while ((killall(name, SIGKILL) == 0) && (n-- > 0)) {
				_dprintf("%s: SIGKILL name=%s n=%d\n", __FUNCTION__, name, n);
				sleep(2);
			}
		}
	}
}
