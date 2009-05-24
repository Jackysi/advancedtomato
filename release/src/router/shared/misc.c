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
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/sysinfo.h>

#include <bcmnvram.h>
#include <bcmdevs.h>
#include <wlutils.h>

#include "shutils.h"
#include "shared.h"

#if 0
#define _dprintf	cprintf
#else
#define _dprintf(args...)	do { } while(0)
#endif


int get_wan_proto(void)
{
	const char *names[] = {	// order must be synced with def at shared.h
		"static",
		"dhcp",
		"l2tp",
		"pppoe",
		"pptp",
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

int using_dhcpc(void)
{
	switch (get_wan_proto()) {
	case WP_DHCP:
	case WP_L2TP:
		return 1;
	}
	return 0;
}

int wl_client(void)
{
	return ((nvram_match("wl_mode", "sta")) || (nvram_match("wl_mode", "wet")));
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
	if (buf[0]) syslog(LOG_INFO, "notice: %s", buf);
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
	if (proto == WP_DISABLED) return 0;

	if ((proto == WP_PPTP) || (proto == WP_L2TP) || (proto == WP_PPPOE)) {
		if (f_read_string("/tmp/ppp/link", buf1, sizeof(buf1)) > 0) {
				// contains the base name of a file in /var/run/ containing pid of a daemon
				snprintf(buf2, sizeof(buf2), "/var/run/%s.pid", buf1);
				if (f_read_string(buf2, buf1, sizeof(buf1)) > 0) {
					name = psname(atoi(buf1), buf2, sizeof(buf2));
					if (proto == WP_PPPOE) {
						if (strcmp(name, "pppoecd") == 0) up = 1;
					}
					else {
						if (strcmp(name, "pppd") == 0) up = 1;
					}
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

	return up;
}


const dns_list_t *get_dns(void)
{
	static dns_list_t dns;
	char s[512];
	int n;
	int i, j;
	struct in_addr ia;
	char d[4][16];

	dns.count = 0;

	strlcpy(s, nvram_safe_get("wan_dns"), sizeof(s));
	if ((nvram_match("dns_addget", "1")) || (s[0] == 0)) {
		n = strlen(s);
		snprintf(s + n, sizeof(s) - n, " %s", nvram_safe_get("wan_get_dns"));
	}

	n = sscanf(s, "%15s %15s %15s %15s", d[0], d[1], d[2], d[3]);
	for (i = 0; i < n; ++i) {
		if (inet_pton(AF_INET, d[i], &ia) > 0) {
			for (j = dns.count - 1; j >= 0; --j) {
				if (dns.dns[j].s_addr == ia.s_addr) break;
			}
			if (j < 0) {
				dns.dns[dns.count++].s_addr = ia.s_addr;
				if (dns.count == 3) break;
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

const char *get_wanip(void)
{
	const char *p;

	if (!check_wanup()) return "0.0.0.0";
	switch (get_wan_proto()) {
	case WP_DISABLED:
		return "0.0.0.0";
	case WP_PPTP:
		p = "pptp_get_ip";
		break;
	case WP_L2TP:
		p = "l2tp_get_ip";
		break;
	default:
		p = "wan_ipaddr";
		break;
	}
	return nvram_safe_get(p);
}

long get_uptime(void)
{
	struct sysinfo si;
	sysinfo(&si);
	return si.uptime;
}

int get_radio(void)
{
	uint32 n;

	return (wl_ioctl(nvram_safe_get("wl_ifname"), WLC_GET_RADIO, &n, sizeof(n)) == 0) &&
		((n & WL_RADIO_SW_DISABLE)  == 0);
}

void set_radio(int on)
{
	uint32 n;

#ifndef WL_BSS_INFO_VERSION
#error WL_BSS_INFO_VERSION
#endif

#if WL_BSS_INFO_VERSION >= 108
	n = on ? (WL_RADIO_SW_DISABLE << 16) : ((WL_RADIO_SW_DISABLE << 16) | 1);
	wl_ioctl(nvram_safe_get("wl_ifname"), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_WLAN, 0);
		led(LED_DIAG, 0);
	}
#else
	n = on ? 0 : WL_RADIO_SW_DISABLE;
	wl_ioctl(nvram_safe_get("wl_ifname"), WLC_SET_RADIO, &n, sizeof(n));
	if (!on) {
		led(LED_DIAG, 0);
	}
#endif
}

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

/*
int time_ok(void)
{
	return time(0) > Y2K;
}
*/
