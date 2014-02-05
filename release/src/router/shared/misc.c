/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <syslog.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"


int get_wan_proto(void)
{
	const char *names[] = {	// order must be synced with def at shared.h
		"static",
		"dhcp",
		"l2tp",
		"pppoe",
		"pptp",
		"ppp3g",
		NULL
	};
	int i;
	const char *p;

	p = nvram_safe_get("wan_proto");
	for (i = 0; names[i] != NULL; ++i) {
		if (strcmp(p, names[i]) == 0) return i + 1;
	}
	return WP_DISABLED;
}

#ifdef TCONFIG_IPV6
int get_ipv6_service(void)
{
	const char *names[] = {	// order must be synced with def at shared.h
		"native",	// IPV6_NATIVE
		"native-pd",	// IPV6_NATIVE_DHCP
		"6to4",		// IPV6_ANYCAST_6TO4
		"sit",		// IPV6_6IN4
		"other",	// IPV6_MANUAL
		"6rd",		// IPV6_6RD
		"6rd-pd",	// IPV6_6RD_DHCP
		NULL
	};
	int i;
	const char *p;

	p = nvram_safe_get("ipv6_service");
	for (i = 0; names[i] != NULL; ++i) {
		if (strcmp(p, names[i]) == 0) return i + 1;
	}
	return IPV6_DISABLED;
}

const char *ipv6_router_address(struct in6_addr *in6addr)
{
	char *p;
	struct in6_addr addr;
	static char addr6[INET6_ADDRSTRLEN];

	addr6[0] = '\0';

	if ((p = nvram_get("ipv6_rtr_addr")) && *p) {
		inet_pton(AF_INET6, p, &addr);
	}
	else if ((p = nvram_get("ipv6_prefix")) && *p) {
		inet_pton(AF_INET6, p, &addr);
		addr.s6_addr16[7] = htons(0x0001);
	}
	else {
		return addr6;
	}

	inet_ntop(AF_INET6, &addr, addr6, sizeof(addr6));
	if (in6addr)
		memcpy(in6addr, &addr, sizeof(addr));

	return addr6;
}

int calc_6rd_local_prefix(const struct in6_addr *prefix,
	int prefix_len, int relay_prefix_len,
	const struct in_addr *local_ip,
	struct in6_addr *local_prefix, int *local_prefix_len)
{
	// the following code is based on ipv6calc's code
	uint32_t local_ip_bits, j;
	int i;

	if (!prefix || !local_ip || !local_prefix || !local_prefix_len) {
		return 0;
	}

	*local_prefix_len = prefix_len + 32 - relay_prefix_len;
	if (*local_prefix_len > 64) {
		return 0;
	}

	local_ip_bits = ntohl(local_ip->s_addr) << relay_prefix_len;

	for (i=0; i<4; i++) {
		local_prefix->s6_addr32[i] = prefix->s6_addr32[i];
	}

	for (j = 0x80000000, i = prefix_len; i < *local_prefix_len; i++, j>>=1)
	{
		if (local_ip_bits & j)
			local_prefix->s6_addr[i>>3] |= (0x80 >> (i & 0x7));
	}

	return 1;
}
#endif

int using_dhcpc(void)
{
	switch (get_wan_proto()) {
	case WP_DHCP:
		return 1;
	case WP_L2TP:
	case WP_PPTP:
		return nvram_get_int("pptp_dhcp");
	}
	return 0;
}

int wl_client(int unit, int subunit)
{
	char *mode = nvram_safe_get(wl_nvname("mode", unit, subunit));

	return ((strcmp(mode, "sta") == 0) || (strcmp(mode, "wet") == 0));
}

int foreach_wif(int include_vifs, void *param,
	int (*func)(int idx, int unit, int subunit, void *param))
{
	char ifnames[256];
	char name[64], ifname[64], *next = NULL;
	int unit = -1, subunit = -1;
	int i;
	int ret = 0;

	snprintf(ifnames, sizeof(ifnames), "%s %s %s %s %s %s %s %s %s %s",
		nvram_safe_get("lan_ifnames"),
		nvram_safe_get("lan1_ifnames"),
		nvram_safe_get("lan2_ifnames"),
		nvram_safe_get("lan3_ifnames"),
		nvram_safe_get("wan_ifnames"),
		nvram_safe_get("wl_ifname"),
		nvram_safe_get("wl0_ifname"),
		nvram_safe_get("wl0_vifs"),
		nvram_safe_get("wl1_ifname"),
		nvram_safe_get("wl1_vifs"));
	remove_dups(ifnames, sizeof(ifnames));
	sort_list(ifnames, sizeof(ifnames));

	i = 0;
	foreach(name, ifnames, next) {
		if (nvifname_to_osifname(name, ifname, sizeof(ifname)) != 0)
			continue;

		if (wl_probe(ifname) || wl_ioctl(ifname, WLC_GET_INSTANCE, &unit, sizeof(unit)))
			continue;

		// Convert eth name to wl name
		if (osifname_to_nvifname(name, ifname, sizeof(ifname)) != 0)
			continue;

		// Slave intefaces have a '.' in the name
		if (strchr(ifname, '.') && !include_vifs)
			continue;

		if (get_ifname_unit(ifname, &unit, &subunit) < 0)
			continue;

		ret |= func(i++, unit, subunit, param);
	}
	return ret;
}

void notice_set(const char *path, const char *format, ...)
{
	char p[256];
	char buf[2048];
	va_list args;

	va_start(args, format);
	vsnprintf(buf, sizeof(buf), format, args);
	va_end(args);

	mkdir("/var/notice", 0755);
	snprintf(p, sizeof(p), "/var/notice/%s", path);
	f_write_string(p, buf, 0, 0);
	if (buf[0]) syslog(LOG_INFO, "notice[%s]: %s", path, buf);
}


//	#define _x_dprintf(args...)	syslog(LOG_DEBUG, args);
#define _x_dprintf(args...)	do { } while (0);

int check_wanup(void)
{
	int up = 0;
	int proto;
	char buf1[64];
	char buf2[64];
	const char *name;
	int f;
	struct ifreq ifr;

	proto = get_wan_proto();
	if (proto == WP_DISABLED)
	{
		if (nvram_match("boardrev", "0x11")) { // Ovislink 1600GL - led "connected" off
			led(LED_WHITE,LED_OFF);
		}
		if (nvram_match("boardtype", "0x052b") &&  nvram_match("boardrev", "0x1204")) { //rt-n15u wan led off
			led(LED_WHITE,LED_OFF);
		}
		 return 0;
	}

	if ((proto == WP_PPTP) || (proto == WP_L2TP) || (proto == WP_PPPOE) || (proto == WP_PPP3G)) {
		if (f_read_string("/tmp/ppp/link", buf1, sizeof(buf1)) > 0) {
				// contains the base name of a file in /var/run/ containing pid of a daemon
				snprintf(buf2, sizeof(buf2), "/var/run/%s.pid", buf1);
				if (f_read_string(buf2, buf1, sizeof(buf1)) > 0) {
					name = psname(atoi(buf1), buf2, sizeof(buf2));
					if (strcmp(name, "pppd") == 0) up = 1;
				}
				else {
					_dprintf("%s: error reading %s\n", __FUNCTION__, buf2);
				}
			if (!up) {
				unlink("/tmp/ppp/link");
				_x_dprintf("required daemon not found, assuming link is dead\n");
			}
		}
		else {
			_x_dprintf("%s: error reading %s\n", __FUNCTION__, "/tmp/ppp/link");
		}
	}
	else if (!nvram_match("wan_ipaddr", "0.0.0.0")) {
		up = 1;
	}
	else {
		_x_dprintf("%s: default !up\n", __FUNCTION__);
	}

	if ((up) && ((f = socket(AF_INET, SOCK_DGRAM, 0)) >= 0)) {
		strlcpy(ifr.ifr_name, nvram_safe_get("wan_iface"), sizeof(ifr.ifr_name));
		if (ioctl(f, SIOCGIFFLAGS, &ifr) < 0) {
			up = 0;
			_x_dprintf("%s: SIOCGIFFLAGS\n", __FUNCTION__);
		}
		close(f);
		if ((ifr.ifr_flags & IFF_UP) == 0) {
			up = 0;
			_x_dprintf("%s: !IFF_UP\n", __FUNCTION__);
		}
	}
	if (nvram_match("boardrev", "0x11")) { // Ovislink 1600GL - led "connected" on
		led(LED_WHITE,up);
	}
	if (nvram_match("boardtype", "0x052b") &&  nvram_match("boardrev", "0x1204")) { //rt-n15u wan led on
		led(LED_WHITE,up);
	}

	return up;
}


const dns_list_t *get_dns(void)
{
	static dns_list_t dns;
	char s[512];
	int n;
	int i, j;
	struct in_addr ia;
	char d[7][22];
	unsigned short port;
	char *c;

	dns.count = 0;

	strlcpy(s, nvram_safe_get("wan_dns"), sizeof(s));
	if ((nvram_get_int("dns_addget")) || (s[0] == 0)) {
		n = strlen(s);
		snprintf(s + n, sizeof(s) - n, " %s", nvram_safe_get("wan_get_dns"));
	}

	n = sscanf(s, "%21s %21s %21s %21s %21s %21s %21s", d[0], d[1], d[2], d[3], d[4], d[5], d[6]);
	for (i = 0; i < n; ++i) {
		port = 53;

		if ((c = strchr(d[i], ':')) != NULL) {
			*c++ = 0;
			if (((j = atoi(c)) < 1) || (j > 0xFFFF)) continue;
			port = j;
		}
		
		if (inet_pton(AF_INET, d[i], &ia) > 0) {
			for (j = dns.count - 1; j >= 0; --j) {
				if ((dns.dns[j].addr.s_addr == ia.s_addr) && (dns.dns[j].port == port)) break;
			}
			if (j < 0) {
				dns.dns[dns.count].port = port;
				dns.dns[dns.count++].addr.s_addr = ia.s_addr;
				if (dns.count == 6) break;
			}
		}
	}

	return &dns;
}

// -----------------------------------------------------------------------------

void set_action(int a)
{
	int r = 3;
	while (f_write("/var/lock/action", &a, sizeof(a), 0, 0) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return;
	}
	if (a != ACT_IDLE) sleep(2);
}

int check_action(void)
{
	int a;
	int r = 3;

	while (f_read("/var/lock/action", &a, sizeof(a)) != sizeof(a)) {
		sleep(1);
		if (--r == 0) return ACT_UNKNOWN;
	}
	return a;
}

int wait_action_idle(int n)
{
	while (n-- > 0) {
		if (check_action() == ACT_IDLE) return 1;
		sleep(1);
	}
	return 0;
}

// -----------------------------------------------------------------------------

const wanface_list_t *get_wanfaces(void)
{
	static wanface_list_t wanfaces;
	char *ip, *iface;
	int proto;

	wanfaces.count = 0;

	switch ((proto = get_wan_proto())) {
		case WP_PPTP:
		case WP_L2TP:
			while (wanfaces.count < 2) {
				if (wanfaces.count == 0) {
					ip = nvram_safe_get("ppp_get_ip");
					iface = nvram_safe_get("wan_iface");
					if (!(*iface)) iface = "ppp+";
				}
				else /* if (wanfaces.count == 1) */ {
					ip = nvram_safe_get("wan_ipaddr");
					if ((!(*ip) || strcmp(ip, "0.0.0.0") == 0) && (wanfaces.count > 0))
						iface = "";
					else
						iface = nvram_safe_get("wan_ifname");
				}
				strlcpy(wanfaces.iface[wanfaces.count].ip, ip, sizeof(wanfaces.iface[0].ip));
				strlcpy(wanfaces.iface[wanfaces.count].name, iface, IFNAMSIZ);
				++wanfaces.count;
			}
			break;
		default:
			ip = (proto == WP_DISABLED) ? "0.0.0.0" : nvram_safe_get("wan_ipaddr");
			if ((proto == WP_PPPOE) || (proto == WP_PPP3G)) {
				iface = nvram_safe_get("wan_iface");
				if (!(*iface)) iface = "ppp+";
			}
			else {
				iface = nvram_safe_get("wan_ifname");
			}
			strlcpy(wanfaces.iface[wanfaces.count].ip, ip, sizeof(wanfaces.iface[0].ip));
			strlcpy(wanfaces.iface[wanfaces.count++].name, iface, IFNAMSIZ);
			break;
	}

	return &wanfaces;
}

const char *get_wanface(void)
{
	return (*get_wanfaces()).iface[0].name;
}

#ifdef TCONFIG_IPV6
const char *get_wan6face(void)
{
	switch (get_ipv6_service()) {
	case IPV6_NATIVE:
	case IPV6_NATIVE_DHCP:
		return get_wanface();
	case IPV6_ANYCAST_6TO4:
		return "v6to4";
	case IPV6_6IN4:
		return "v6in4";
	}
	return nvram_safe_get("ipv6_ifname");
}
#endif

const char *get_wanip(void)
{
	if (!check_wanup()) return "0.0.0.0";

	return (*get_wanfaces()).iface[0].ip;
}

const char *getifaddr(char *ifname, int family, int linklocal)
{
	static char buf[INET6_ADDRSTRLEN];
	void *addr = NULL;
	struct ifaddrs *ifap, *ifa;

	if (getifaddrs(&ifap) != 0) {
		_dprintf("getifaddrs failed: %s\n", strerror(errno));
		return NULL;
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if ((ifa->ifa_addr == NULL) ||
		    (strncmp(ifa->ifa_name, ifname, IFNAMSIZ) != 0) ||
		    (ifa->ifa_addr->sa_family != family))
			continue;

#ifdef TCONFIG_IPV6
		if (ifa->ifa_addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)(ifa->ifa_addr);
			if (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr) ^ linklocal)
				continue;
			addr = (void *)&(s6->sin6_addr);
		}
		else
#endif
		{
			struct sockaddr_in *s = (struct sockaddr_in *)(ifa->ifa_addr);
			addr = (void *)&(s->sin_addr);
		}

		if ((addr) && inet_ntop(ifa->ifa_addr->sa_family, addr, buf, sizeof(buf)) != NULL) {
			freeifaddrs(ifap);
			return buf;
		}
	}

	freeifaddrs(ifap);
	return NULL;
}

// -----------------------------------------------------------------------------

long get_uptime(void)
{
	struct sysinfo si;
	sysinfo(&si);
	return si.uptime;
}

char *wl_nvname(const char *nv, int unit, int subunit)
{
	static char tmp[128];
	char prefix[] = "wlXXXXXXXXXX_";

	if (unit < 0)
		strcpy(prefix, "wl_");
	else if (subunit > 0)
		snprintf(prefix, sizeof(prefix), "wl%d.%d_", unit, subunit);
	else
		snprintf(prefix, sizeof(prefix), "wl%d_", unit);
	return strcat_r(prefix, nv, tmp);
}

int get_radio(int unit)
{
	uint32 n;

	return (wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), WLC_GET_RADIO, &n, sizeof(n)) == 0) &&
		((n & WL_RADIO_SW_DISABLE)  == 0);
}

void set_radio(int on, int unit)
{
	uint32 n;

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
	n = on ? (WL_RADIO_SW_DISABLE << 16) : ((WL_RADIO_SW_DISABLE << 16) | 1);
	wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_WLAN, 0);
		led(LED_DIAG, 0);
	}
#else
	n = on ? 0 : WL_RADIO_SW_DISABLE;
	wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, 0)), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_DIAG, 0);
	}
#endif
}

// -----------------------------------------------------------------------------

int mtd_getinfo(const char *mtdname, int *part, int *size)
{
	FILE *f;
	char s[256];
	char t[256];
	int r;

	r = 0;
	if ((strlen(mtdname) < 128) && (strcmp(mtdname, "pmon") != 0)) {
		sprintf(t, "\"%s\"", mtdname);
		if ((f = fopen("/proc/mtd", "r")) != NULL) {
			while (fgets(s, sizeof(s), f) != NULL) {
				if ((sscanf(s, "mtd%d: %x", part, size) == 2) && (strstr(s, t) != NULL)) {
					// don't accidentally mess with bl (0)
					if (*part > 0) r = 1;
					break;
				}
			}
			fclose(f);
		}
	}
	if (!r) {
		*size = 0;
		*part = -1;
	}
	return r;
}

// -----------------------------------------------------------------------------

int nvram_get_int(const char *key)
{
	return atoi(nvram_safe_get(key));
}

/*
long nvram_xget_long(const char *name, long min, long max, long def)
{
	const char *p;
	char *e;
	long n;

	p = nvram_get(name);
	if ((p != NULL) && (*p != 0)) {
		n = strtol(p, &e, 0);
		if ((e != p) && ((*e == 0) || (*e == ' ')) && (n > min) && (n < max)) {
			return n;
		}
	}
	return def;
}
*/

int nvram_get_file(const char *key, const char *fname, int max)
{
	int n;
	char *p;
	char *b;
	int r;

	r = 0;
	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		if ((b = malloc(base64_decoded_len(n) + 128)) != NULL) {
			n = base64_decode(p, b, n);
			if (n > 0) r = (f_write(fname, b, n, 0, 0644) == n);
			free(b);
		}
	}
	return r;
/*
	char b[2048];
	int n;
	char *p;

	p = nvram_safe_get(key);
	n = strlen(p);
	if (n <= max) {
		n = base64_decode(p, b, n);
		if (n > 0) return (f_write(fname, b, n, 0, 0700) == n);
	}
	return 0;
*/
}

int nvram_set_file(const char *key, const char *fname, int max)
{
	char *in;
	char *out;
	long len;
	int n;
	int r;

	if ((len = f_size(fname)) > max) return 0;
	max = (int)len;
	r = 0;
	if (f_read_alloc(fname, &in, max) == max) {
		if ((out = malloc(base64_encoded_len(max) + 128)) != NULL) {
			n = base64_encode(in, out, max);
			out[n] = 0;
			nvram_set(key, out);
			free(out);
			r = 1;
		}
		free(in);
	}
	return r;
/*
	char a[2048];
	char b[4096];
	int n;

	if (((n = f_read(fname, &a, sizeof(a))) > 0) && (n <= max)) {
		n = base64_encode(a, b, n);
		b[n] = 0;
		nvram_set(key, b);
		return 1;
	}
	return 0;
*/
}

int nvram_contains_word(const char *key, const char *word)
{
	return (find_word(nvram_safe_get(key), word) != NULL);
}

int nvram_is_empty(const char *key)
{
	char *p;
	return (((p = nvram_get(key)) == NULL) || (*p == 0));
}

void nvram_commit_x(void)
{
	if (!nvram_get_int("debug_nocommit")) nvram_commit();
}

int connect_timeout(int fd, const struct sockaddr *addr, socklen_t len, int timeout)
{
	fd_set fds;
	struct timeval tv;
	int flags;
	int n;
	int r;

	if (((flags = fcntl(fd, F_GETFL, 0)) < 0) ||
		(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)) {
		_dprintf("%s: error in F_*ETFL %d\n", __FUNCTION__, fd);
		return -1;
	}

	if (connect(fd, addr, len) < 0) {
//		_dprintf("%s: connect %d = <0\n", __FUNCTION__, fd);

		if (errno != EINPROGRESS) {
			_dprintf("%s: error in connect %d errno=%d\n", __FUNCTION__, fd, errno);
			return -1;
		}

		while (1) {
			tv.tv_sec = timeout;
			tv.tv_usec = 0;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			r = select(fd + 1, NULL, &fds, NULL, &tv);
			if (r == 0) {
				_dprintf("%s: timeout in select %d\n", __FUNCTION__, fd);
				return -1;
			}
			else if (r < 0) {
				if (errno != EINTR) {
					_dprintf("%s: error in select %d\n", __FUNCTION__, fd);
					return -1;
				}
				// loop
			}
			else {
				r = 0;
				n = sizeof(r);
				if ((getsockopt(fd, SOL_SOCKET, SO_ERROR, &r, &n) < 0) || (r != 0)) {
					_dprintf("%s: error in SO_ERROR %d\n", __FUNCTION__, fd);
					return -1;
				}
				break;
			}
		}
	}

	if (fcntl(fd, F_SETFL, flags) < 0) {
		_dprintf("%s: error in F_*ETFL %d\n", __FUNCTION__, fd);
		return -1;
	}

//	_dprintf("%s: OK %d\n", __FUNCTION__, fd);
	return 0;
}

void chld_reap(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0) {}
}

/*
int time_ok(void)
{
	return time(0) > Y2K;
}
*/
