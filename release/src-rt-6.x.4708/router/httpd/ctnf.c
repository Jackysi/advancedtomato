/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <ctype.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <dirent.h>

#define IP6_PREFIX_NOT_MATCH( a, b, bits ) (memcmp(&a.s6_addr[0], &b.s6_addr[0], bits/8) != 0 || ( bits % 8 && (a.s6_addr[bits/8] ^ b.s6_addr[bits/8]) >> (8-(bits % 8)) ))


void ctvbuf(FILE *f) {
	int n;
	struct sysinfo si;
//	meminfo_t mem;

#if 1
	const char *p;

	if ((p = nvram_get("ct_max")) != NULL) {
		n = atoi(p);
		if (n == 0) n = 2048;
		else if (n < 1024) n = 1024;
		else if (n > 10240) n = 10240;
	}
	else {
		n = 2048;
	}
#else
	char s[64];

	if (f_read_string("/proc/sys/net/ipv4/ip_conntrack_max", s, sizeof(s)) > 0) n = atoi(s);
		else n = 1024;
	if (n < 1024) n = 1024;
		else if (n > 10240) n = 10240;
#endif

	n *= 170;	// avg tested

//	get_memory(&mem);
//	if (mem.maxfreeram < (n + (64 * 1024))) n = mem.maxfreeram - (64 * 1024);

	sysinfo(&si);
	if (si.freeram < (n + (64 * 1024))) n = si.freeram - (64 * 1024);

//	cprintf("free: %dK, buffer: %dK\n", si.freeram / 1024, n / 1024);

	if (n > 4096) {
//		n =
		setvbuf(f, NULL, _IOFBF, n);
//		cprintf("setvbuf = %d\n", n);
	}
}

void asp_ctcount(int argc, char **argv)
{
	static const char *states[10] = {
		"NONE", "ESTABLISHED", "SYN_SENT", "SYN_RECV", "FIN_WAIT",
		"TIME_WAIT", "CLOSE", "CLOSE_WAIT", "LAST_ACK", "LISTEN" };
	int count[13];	// tcp(10) + udp(2) + total(1) = 13 / max classes = 10
	FILE *f;
	char s[512];
	char *p;
	char *t;
	int i;
	int n;
	int mode;
	unsigned long rip;
	unsigned long lan;
	unsigned long mask;

	if (argc != 1) return;

#if defined(TCONFIG_IPV6) && defined(LINUX26)
	char src[INET6_ADDRSTRLEN];
	char dst[INET6_ADDRSTRLEN];
	struct in6_addr rip6;
	struct in6_addr lan6;
	struct in6_addr in6;
	int lan6_prefix_len;

	lan6_prefix_len = nvram_get_int("ipv6_prefix_length");
	if (ipv6_enabled()) {
		inet_pton(AF_INET6, nvram_safe_get("ipv6_prefix"), &lan6);
		ipv6_router_address(&rip6);
	}
#endif

	mode = atoi(argv[0]);

	memset(count, 0, sizeof(count));

#if defined(TCONFIG_IPV6) && defined(LINUX26)
	if ((f = fopen("/proc/net/nf_conntrack", "r")) != NULL) {
#else
	if ((f = fopen("/proc/net/ip_conntrack", "r")) != NULL) {
#endif
		ctvbuf(f);	// if possible, read in one go

		if (nvram_match("t_hidelr", "1")) {
			mask = inet_addr(nvram_safe_get("lan_netmask"));
			rip = inet_addr(nvram_safe_get("lan_ipaddr"));
			lan = rip & mask;
		}
		else {
			rip = lan = mask = 0;
		}

		while (fgets(s, sizeof(s), f)) {
#if defined(TCONFIG_IPV6) && defined(LINUX26)
			if (strncmp(s, "ipv4", 4) == 0) {
				t = s + 11;
#else
				t = s;
#endif
				if (rip != 0) {

					// src=x.x.x.x dst=x.x.x.x	// DIR_ORIGINAL
					if ((p = strstr(t + 14, "src=")) == NULL) continue;
					if ((inet_addr(p + 4) & mask) == lan) {
						if ((p = strstr(p + 13, "dst=")) == NULL) continue;
						if (inet_addr(p + 4) == rip) continue;
					}
				}
#if defined(TCONFIG_IPV6) && defined(LINUX26)
			}
			else if (strncmp(s, "ipv6", 4) == 0) {
				t = s + 12;

				if (rip != 0) {
					if ((p = strstr(t + 14, "src=")) == NULL) continue;
					if (sscanf(p, "src=%s dst=%s", src, dst) != 2) continue;

					if (inet_pton(AF_INET6, src, &in6) <= 0) continue;
					inet_ntop(AF_INET6, &in6, src, sizeof(src));

					if (!IP6_PREFIX_NOT_MATCH(lan6, in6, lan6_prefix_len)) continue;
				}
			}
			else {
				continue; // another proto family?!
			}
#endif

			if (mode == 0) {
				// count connections per state
				if (strncmp(t, "tcp", 3) == 0) {
					for (i = 9; i >= 0; --i) {
						if (strstr(s, states[i]) != NULL) {
							count[i]++;
							break;
						}
					}
				}
				else if (strncmp(t, "udp", 3) == 0) {
					if (strstr(s, "[ASSURED]") != NULL) {
						count[11]++;
					}
					else {
						count[10]++;
					}
				}
				count[12]++;
			}
			else {
				// count connections per mark
				if ((p = strstr(s, " mark=")) != NULL) {
					n = atoi(p + 6) & 0xFF;
					if (n <= 10) count[n]++;
				}
			}
		}

		fclose(f);
	}

	if (mode == 0) {
		p = s;
		for (i = 0; i < 12; ++i) {
			p += sprintf(p, ",%d", count[i]);
		}
		web_printf("\nconntrack = [%d%s];\n", count[12], s);
	}
	else {
		p = s;
		for (i = 1; i < 11; ++i) {
			p += sprintf(p, ",%d", count[i]);
		}
		web_printf("\nnfmarks = [%d%s];\n", count[0], s);
	}
}

void asp_ctdump(int argc, char **argv)
{
	FILE *f;
	char s[512];
	char *p, *q;
	int x;
	int mark;
	int rule;
	int findmark;
	unsigned int proto;
	unsigned int time;
#if defined(TCONFIG_IPV6) && defined(LINUX26)
	unsigned int family;
	char src[INET6_ADDRSTRLEN];
	char dst[INET6_ADDRSTRLEN];
#else
	const unsigned int family = 2;
	char src[INET_ADDRSTRLEN];
	char dst[INET_ADDRSTRLEN];
#endif
	char sport[16];
	char dport[16];
	char byteso[16];
	char bytesi[16];
	int dir_reply;

	unsigned long rip;
	unsigned long lan;
	unsigned long mask;
	char comma;

	if (argc != 1) return;

	findmark = atoi(argv[0]);

	mask = inet_addr(nvram_safe_get("lan_netmask"));
	rip = inet_addr(nvram_safe_get("lan_ipaddr"));
	lan = rip & mask;
	
#if defined(TCONFIG_IPV6) && defined(LINUX26)
	struct in6_addr rip6;
	struct in6_addr lan6;
	struct in6_addr in6;
	int lan6_prefix_len;

	lan6_prefix_len = nvram_get_int("ipv6_prefix_length");
	if (ipv6_enabled()) {
		inet_pton(AF_INET6, nvram_safe_get("ipv6_prefix"), &lan6);
		ipv6_router_address(&rip6);
	}
#endif

	if (nvram_match("t_hidelr", "0")) rip = 0;	// hide lan -> router?

/*

/proc/net/nf_conntrack prefix (compared to ip_conntrack):
"ipvx" + 5 spaces + "2" or "10" + 1 space

add bytes out/in to table

*/

	web_puts("\nctdump = [");
	comma = ' ';
#if defined(TCONFIG_IPV6) && defined(LINUX26)
	if ((f = fopen("/proc/net/nf_conntrack", "r")) != NULL) {
#else
	if ((f = fopen("/proc/net/ip_conntrack", "r")) != NULL) {
#endif
		ctvbuf(f);
		while (fgets(s, sizeof(s), f)) {
			dir_reply = 0;
			if ((p = strstr(s, " mark=")) == NULL) continue;
			mark = atoi(p + 6);
			rule = (mark >> 20) & 0xFF;
			if ((mark &= 0xFF) > 10) mark = 0;
			if ((findmark != -1) && (mark != findmark)) continue;
#if defined(TCONFIG_IPV6) && defined(LINUX26)
			if (sscanf(s, "%*s %u %*s %u %u", &family, &proto, &time) != 3) continue;
			if ((p = strstr(s + 25, "src=")) == NULL) continue;
#else
			if (sscanf(s, "%*s %u %u", &proto, &time) != 2) continue;
			if ((p = strstr(s + 14, "src=")) == NULL) continue;
#endif
			if (sscanf(p, "src=%s dst=%s %n", src, dst, &x) != 2) continue;
			p += x;

			if ((proto == 6) || (proto == 17)) {
				if (sscanf(p, "sport=%s dport=%s %*s bytes=%s %n", sport, dport, byteso, &x) != 3) continue;
				p += x;
				if ((q = strstr(p, "bytes=")) == NULL) continue;
				if (sscanf(q, "bytes=%s", bytesi) != 1) continue;
			}
			else {
				sport[0] = 0;
				dport[0] = 0;
				byteso[0] = 0;
				bytesi[0] = 0;
			}

			switch (family) {
			case 2:
				if ((inet_addr(src) & mask) != lan) {
					dir_reply = 1;
					// de-nat
					if ((p = strstr(p, "src=")) == NULL) continue;
					if ((proto == 6) || (proto == 17)) {
						if (sscanf(p, "src=%s dst=%s sport=%s dport=%s", dst, src, dport, sport) != 4) continue; // intentionally backwards
					}
					else {
						if (sscanf(p, "src=%s dst=%s", dst, src) != 2) continue;
					}
				}
				else if (rip != 0 && inet_addr(dst) == rip) continue; 
				break;
#if defined(TCONFIG_IPV6) && defined(LINUX26)
			case 10:
				if (inet_pton(AF_INET6, src, &in6) <= 0) continue;
				inet_ntop(AF_INET6, &in6, src, sizeof(src));

				if (IP6_PREFIX_NOT_MATCH(lan6, in6, lan6_prefix_len))
					dir_reply = 1;

				if (inet_pton(AF_INET6, dst, &in6) <= 0) continue;
				inet_ntop(AF_INET6, &in6, dst, sizeof(dst));
				
				if (dir_reply == 0 && rip != 0 && (IN6_ARE_ADDR_EQUAL(&rip6, &in6)))
					continue;
				break;
#endif
			}

			if (dir_reply == 1) {
				web_printf("%c[%u,%u,'%s','%s','%s','%s','%s','%s',%d,%d]", comma, proto, time, dst, src, dport, sport, bytesi, byteso, mark, rule );
			}
			else {
				web_printf("%c[%u,%u,'%s','%s','%s','%s','%s','%s',%d,%d]", comma, proto, time, src, dst, sport, dport, byteso, bytesi, mark, rule );
			} 
			comma = ',';
		}
	}
	web_puts("];\n");
}

void asp_ctrate(int argc, char **argv)
{
	unsigned int a_time, a_proto;
	unsigned int a_bytes_o, a_bytes_i;
	char a_sport[16];
	char a_dport[16];

	char *p, *q;
	int x;
	int len;

#if defined(TCONFIG_IPV6) && defined(LINUX26)
	unsigned int a_fam, b_fam;
	char a_src[INET6_ADDRSTRLEN];
	char a_dst[INET6_ADDRSTRLEN];
#else
	const unsigned int a_fam = 2;
	char a_src[INET_ADDRSTRLEN];
	char a_dst[INET_ADDRSTRLEN];
#endif

	unsigned int b_time, b_proto;
	unsigned int b_bytes_o, b_bytes_i;

	char sa[512];
	char sb[512];

	FILE *a;
	FILE *b;

	long b_pos;

	int delay;
	int thres;

	long outbytes;
	long inbytes;
	int n;
	char comma;

	int dir_reply;

	unsigned long rip;
	unsigned long lan;
	unsigned long mask;

	mask = inet_addr(nvram_safe_get("lan_netmask"));
	rip = inet_addr(nvram_safe_get("lan_ipaddr"));
	lan = rip & mask;

#if defined(TCONFIG_IPV6) && defined(LINUX26)
	struct in6_addr rip6;
	struct in6_addr lan6;
	struct in6_addr in6;
	int lan6_prefix_len;

	lan6_prefix_len = nvram_get_int("ipv6_prefix_length");
	if (ipv6_enabled()) {
		inet_pton(AF_INET6, nvram_safe_get("ipv6_prefix"), &lan6);
		ipv6_router_address(&rip6);
	}
#endif

	if (nvram_match("t_hidelr", "0")) rip = 0;	// hide lan -> router?

	web_puts("\nctrate = [");
	comma = ' ';

#if defined(TCONFIG_IPV6) && defined(LINUX26)
	const char name[] = "/proc/net/nf_conntrack";
#else
	const char name[] = "/proc/net/ip_conntrack";	
#endif

	if (argc != 2) return;

	delay = atoi(argv[0]);
	thres = atoi(argv[1]) * delay;

	if ((a = fopen(name, "r")) == NULL) return;
	if ((b = tmpfile()) == NULL) return;

	size_t count;
	char *buffer;

	buffer=(char *)malloc(1024);

	while (!feof(a)) {
		count = fread(buffer, 1, 1024, a);
		fwrite(buffer, 1, count, b);
	}

	rewind(b);
	rewind(a);

	usleep(1000000 * (int)delay);

#define MAX_SEARCH  10

	// a = current, b = previous
	while (fgets(sa, sizeof(sa), a)) {
#if defined(TCONFIG_IPV6) && defined(LINUX26)
		if (sscanf(sa, "%*s %u %*s %u %u", &a_fam, &a_proto, &a_time) != 3) continue;
#else
		if (sscanf(sa, "%*s %u %u", &a_proto, &a_time) != 2) continue;
#endif
		if ((a_proto != 6) && (a_proto != 17)) continue;
		if ((p = strstr(sa, "src=")) == NULL) continue;

		if (sscanf(p, "src=%s dst=%s sport=%s dport=%s%n %*s bytes=%u %n", a_src, a_dst, a_sport, a_dport, &len, &a_bytes_o, &x) != 5) continue;

		if ((q = strstr(p+x, "bytes=")) == NULL) continue;
		if (sscanf(q, "bytes=%u", &a_bytes_i) != 1) continue;

		dir_reply = 0;

		switch(a_fam) {
			case 2:
				if ((inet_addr(a_src) & mask) != lan)  dir_reply = 1;
				else if (rip != 0 && inet_addr(a_dst) == rip) continue;
				break;
#if defined(TCONFIG_IPV6) && defined(LINUX26)
			case 10:
				if (inet_pton(AF_INET6, a_src, &in6) <= 0) continue;
				inet_ntop(AF_INET6, &in6, a_src, sizeof(a_src));

				if (IP6_PREFIX_NOT_MATCH(lan6, in6, lan6_prefix_len))
					dir_reply = 1;

				if (inet_pton(AF_INET6, a_dst, &in6) <= 0) continue;
				inet_ntop(AF_INET6, &in6, a_dst, sizeof(a_dst));

				if (dir_reply == 0 && rip != 0 && (IN6_ARE_ADDR_EQUAL(&rip6, &in6)))
					continue;
				break;
			default:
				continue;
#endif
		}

		b_pos = ftell(b);
		n = 0;
		while (fgets(sb, sizeof(sb), b) && ++n < MAX_SEARCH) {
#if defined(TCONFIG_IPV6) && defined(LINUX26)
			if (sscanf(sb, "%*s %u %*s %u %u", &b_fam, &b_proto, &b_time) != 3) continue;
			if ((b_fam != a_fam)) continue;
#else
			if (sscanf(sb, "%*s %u %u", &b_proto, &b_time) != 2) continue;
#endif
			if ((b_proto != a_proto)) continue;
			if ((q = strstr(sb, "src=")) == NULL) continue;

			if (strncmp(p, q, (size_t)len)) continue;

			// Ok, they should be the same now. Grab the byte counts.
			if ((q = strstr(q+len, "bytes=")) == NULL) continue;
			if (sscanf(q, "bytes=%u", &b_bytes_o) != 1) continue;

			if ((q = strstr(q+len, "bytes=")) == NULL) continue;
			if (sscanf(q, "bytes=%u", &b_bytes_i) != 1) continue;

			break;
		}

		if (feof(b) || n >= MAX_SEARCH) {
			// Assume this is a new connection
			b_bytes_o = 0;
			b_bytes_i = 0;
			b_time = 0;

			// Reset the search so we don't miss anything
			fseek(b, b_pos, SEEK_SET);
			n = -n;
		}

		outbytes = ((long)a_bytes_o - (long)b_bytes_o);
		inbytes = ((long)a_bytes_i - (long)b_bytes_i);

		if ((outbytes < thres) && (inbytes < thres)) continue;

		if (dir_reply == 1 && a_fam == 2) {
			// de-nat
			if ((q = strstr(p+x, "src=")) == NULL) continue;
			if (sscanf(q, "src=%s dst=%s sport=%s dport=%s", a_dst, a_src, a_dport, a_sport) != 4) continue;
		}

		if (dir_reply == 1) {
			web_printf("%c[%u,'%s','%s','%s','%s',%li,%li]",
				comma, a_proto, a_dst, a_src, a_dport, a_sport, inbytes, outbytes );
		}
		else {
			web_printf("%c[%u,'%s','%s','%s','%s',%li,%li]",
				comma, a_proto, a_src, a_dst, a_sport, a_dport, outbytes, inbytes );				
		}
		comma = ',';
	}
	web_puts("];\n");

	fclose(a);
	fclose(b);
}
	
static void retrieveRatesFromTc(const char* deviceName, unsigned long ratesArray[])
{
	char s[256];
	FILE *f;
	unsigned long u;
	char *e;
	int n;
	
	sprintf(s, "tc -s class ls dev %s", deviceName);
	if ((f = popen(s, "r")) != NULL) {
		n = 1;
		while (fgets(s, sizeof(s), f)) {
			if (strncmp(s, "class htb 1:", 12) == 0) {
				n = atoi(s + 12);
			}
			else if (strncmp(s, " rate ", 6) == 0) {
				if ((n % 10) == 0) {
					n /= 10;
					if ((n >= 1) && (n <= 10)) {
						u = strtoul(s + 6, &e, 10);
						if (*e == 'K') u *= 1000;
							else if (*e == 'M') u *= 1000 * 1000;
						ratesArray[n - 1] = u;
						n = 1;
					}
				}
			}
		}
		pclose(f);
	}
}

void asp_qrate(int argc, char **argv)
{
	unsigned long rates[10];
	int n;
	char comma;
	char *a[1];

	a[0] = "1";
	asp_ctcount(1, a);

	memset(rates, 0, sizeof(rates));
	retrieveRatesFromTc(get_wanface(), rates);

	comma = ' ';
	web_puts("\nqrates_out = [0,");
	for (n = 0; n < 10; ++n) {
		web_printf("%c%lu", comma, rates[n]);
		comma = ',';
	}
	web_puts("];");
	
	memset(rates, 0, sizeof(rates));
	retrieveRatesFromTc("imq0", rates);

	comma = ' ';
	web_puts("\nqrates_in = [0,");
	for (n = 0; n < 10; ++n) {
		web_printf("%c%lu", comma, rates[n]);
		comma = ',';
	}
	web_puts("];");
}

static void layer7_list(const char *path, int *first)
{
	DIR *dir;
	struct dirent *de;
	char *p;
	char name[NAME_MAX];

	if ((dir = opendir(path)) != NULL) {
		while ((de = readdir(dir)) != NULL) {
			strlcpy(name, de->d_name, sizeof(name));
			if ((p = strstr(name, ".pat")) == NULL) continue;
			*p = 0;
			web_printf("%s'%s'", *first ? "" : ",", name);
			*first = 0;
		}
		closedir(dir);
	}
}

void asp_layer7(int argc, char **argv)
{
	int first = 1;
	web_puts("\nlayer7 = [");
	layer7_list("/etc/l7-extra", &first);
	layer7_list("/etc/l7-protocols", &first);
	web_puts("];\n");
}

void wo_expct(char *url)
{
	f_write_string("/proc/net/expire_early", "15", 0, 0);
}
