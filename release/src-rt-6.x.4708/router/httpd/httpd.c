/*

	micro_httpd/mini_httpd

	Copyright © 1999,2000 by Jef Poskanzer <jef@acme.com>.
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:
	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
	ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
	OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
	HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
	OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
	SUCH DAMAGE.

*/
/*

	Copyright 2003, CyberTAN  Inc.  All Rights Reserved

	This is UNPUBLISHED PROPRIETARY SOURCE CODE of CyberTAN Inc.
	the contents of this file may not be disclosed to third parties,
	copied or duplicated in any form without the prior written
	permission of CyberTAN Inc.

	This software should be used as a reference only, and it not
	intended for production use!

	THIS SOFTWARE IS OFFERED "AS IS", AND CYBERTAN GRANTS NO WARRANTIES OF ANY
	KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE.  CYBERTAN
	SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
	FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE

*/
/*

	Modified for Tomato Firmware
	Portions, Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <stdarg.h>
#include <sys/wait.h>
#include <error.h>
#include <sys/signal.h>
#include <netinet/tcp.h>
#include <sys/stat.h>

#include <wlutils.h>


#include "../mssl/mssl.h"
int do_ssl;

#define HTTP_MAX_LISTENERS 8

typedef struct {
	int count;
	fd_set lfdset;
	struct {
		int listenfd;
		int ssl;
	} listener[HTTP_MAX_LISTENERS];
} listeners_t;
static listeners_t listeners;
static int maxfd = -1;

int post;
int connfd = -1;
FILE *connfp = NULL;
struct sockaddr_storage clientsai;

int header_sent;
char *user_agent;
//	int hidok = 0;

const char mime_html[] = "text/html; charset=utf-8";
const char mime_plain[] = "text/plain";
const char mime_javascript[] = "text/javascript";
const char mime_binary[] = "application/tomato-binary-file";	// instead of "application/octet-stream" to make browser just "save as" and prevent automatic detection weirdness	-- zzz
const char mime_octetstream[] = "application/octet-stream";

static int match(const char* pattern, const char* string);
static int match_one(const char* pattern, int patternlen, const char* string);
static void handle_request(void);


// --------------------------------------------------------------------------------------------------------------------


static const char *http_status_desc(int status)
{
	switch (status) {
	case 200:
		return "OK";
	case 302:
		return "Found";
	case 400:
		return "Invalid Request";
	case 401:
		return "Unauthorized";
	case 404:
		return "Not Found";
	case 501:
		return "Not Implemented";
	}
	return "Unknown";
}

void send_header(int status, const char* header, const char* mime, int cache)
{
	time_t now;
	char tms[128];

	now = time(NULL);
	if (now < Y2K) now += Y2K;
	strftime(tms, sizeof(tms), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));	// RFC 1123
	web_printf("HTTP/1.0 %d %s\r\n"
			   "Date: %s\r\n",
			   status, http_status_desc(status),
			   tms);

	if (mime) web_printf("Content-Type: %s\r\n", mime);
	if (cache > 0) {
		web_printf("Cache-Control: max-age=%d\r\n", cache * 60);
	}
	else {
		web_puts("Cache-Control: no-cache, no-store, must-revalidate, private\r\n"
				 "Expires: Thu, 31 Dec 1970 00:00:00 GMT\r\n"
				 "Pragma: no-cache\r\n");
	}
	if (header) web_printf("%s\r\n", header);
	web_puts("Connection: close\r\n\r\n");

	header_sent = 1;
}

void send_error(int status, const char *header, const char *text)
{
	const char *s = http_status_desc(status);
	send_header(status, header, mime_html, 0);
	web_printf(
		"<html>"
		"<head><title>Error</title></head>"
		"<body>"
		"<h2>%d %s</h2> %s"
		"</body></html>",
		status, s, text ? text : s);
}

void redirect(const char *path)
{
	char s[512];

	snprintf(s, sizeof(s), "Location: %s", path);
	send_header(302, s, mime_html, 0);
	web_puts(s);

	_dprintf("Redirect: %s\n", path);
}

int skip_header(int *len)
{
	char buf[2048];

	while (*len > 0) {
		if (!web_getline(buf, MIN(*len, sizeof(buf)))) {
			break;
		}
		*len -= strlen(buf);
		if ((strcmp(buf, "\n") == 0) || (strcmp(buf, "\r\n") == 0)) {
			return 1;
		}
	}
	return 0;
}


// -----------------------------------------------------------------------------

static void eat_garbage(void)
{
	int i;
	int flags;

	// eat garbage \r\n (IE6, ...) or browser ends up with a tcp reset error message
	if ((!do_ssl) && (post)) {
		if (((flags = fcntl(connfd, F_GETFL)) != -1) && (fcntl(connfd, F_SETFL, flags | O_NONBLOCK) != -1)) {
//					if (fgetc(connfp) != EOF) fgetc(connfp);
			for (i = 0; i < 1024; ++i) {
				if (fgetc(connfp) == EOF) break;
			}
			fcntl(connfd, F_SETFL, flags);
		}
	}
}

// -----------------------------------------------------------------------------


static void send_authenticate(void)
{
	char header[128];
	const char *realm;

	realm = nvram_get("router_name");
	if ((realm == NULL) || (*realm == 0) || (strlen(realm) > 64)) realm = "unknown";

	sprintf(header, "WWW-Authenticate: Basic realm=\"%s\"", realm);
	send_error(401, header, NULL);
}

typedef enum { AUTH_NONE, AUTH_OK, AUTH_BAD } auth_t;

static auth_t auth_check(const char *authorization)
{
	char buf[512];
	const char *u, *p;
	char* pass;
	int len;

	if ((authorization != NULL) && (strncasecmp(authorization, "Basic ", 6) == 0)) {
		if (base64_decoded_len(strlen(authorization + 6)) <= sizeof(buf)) {
			len = base64_decode(authorization + 6, buf, strlen(authorization) - 6);
			buf[len] = 0;
			if ((pass = strchr(buf, ':')) != NULL) {
				*pass++ = 0;
				if (((u = nvram_get("http_username")) == NULL) || (*u == 0)) u = "admin";
				if ((strcmp(buf, "root") == 0) || (strcmp(buf, u) == 0)) {
					if (((p = nvram_get("http_passwd")) == NULL) || (*p == 0)) p = "admin";
					if (strcmp(pass, p) == 0) {
						return AUTH_OK;
					}
				}
			}
		}
		return AUTH_BAD;
	}

	return AUTH_NONE;
}

static void auth_fail(int clen)
{
	if (post) web_eat(clen);
	eat_garbage();
	send_authenticate();
}

static int check_wif(int idx, int unit, int subunit, void *param)
{
	sta_info_t *sti = param;
	return (wl_ioctl(nvram_safe_get(wl_nvname("ifname", unit, subunit)), WLC_GET_VAR, sti, sizeof(*sti)) == 0);
}

static int check_wlaccess(void)
{
	char mac[32];
	char ifname[32];
	sta_info_t sti;

	if (nvram_match("web_wl_filter", "1")) {
		if (get_client_info(mac, ifname)) {
			memset(&sti, 0, sizeof(sti));
			strcpy((char *)&sti, "sta_info");	// sta_info0<mac>
			ether_atoe(mac, (char *)&sti + 9);
			if (foreach_wif(1, &sti, check_wif)) {
				if (nvram_match("debug_logwlac", "1")) {
					syslog(LOG_WARNING, "Wireless access from %s blocked.", mac);
				}
				return 0;
			}
		}
	}
	return 1;
}

// -----------------------------------------------------------------------------

static int match_one(const char* pattern, int patternlen, const char* string)
{
	const char* p;

	for (p = pattern; p - pattern < patternlen; ++p, ++string) {
		if (*p == '?' && *string != '\0')
			continue;
		if (*p == '*') {
			int i, pl;
			++p;
			if (*p == '*') {
				/* Double-wildcard matches anything. */
				++p;
				i = strlen(string);
			} else
				/* Single-wildcard matches anything but slash. */
				i = strcspn(string, "/");
			pl = patternlen - (p - pattern);
			for (; i >= 0; --i)
				if (match_one(p, pl, &(string[i])))
					return 1;
			return 0;
		}
		if (*p != *string)
			return 0;
	}
	if (*string == '\0')
		return 1;
	return 0;
}

//	Simple shell-style filename matcher.  Only does ? * and **, and multiple
//	patterns separated by |.  Returns 1 or 0.
static int match(const char* pattern, const char* string)
{
	const char* p;

	for (;;) {
		p = strchr(pattern, '|');
		if (p == NULL) return match_one(pattern, strlen(pattern), string);
		if (match_one(pattern, p - pattern, string)) return 1;
		pattern = p + 1;
	}
}


void do_file(char *path)
{
	FILE *f;
	char buf[1024];
	int nr;
	if ((f = fopen(path, "r")) != NULL) {
		while ((nr = fread(buf, 1, sizeof(buf), f)) > 0)
			web_write(buf, nr);
		fclose(f);
	}
}

static void handle_request(void)
{
	char line[10000], *cur;
	char *method, *path, *protocol, *authorization, *boundary;
	char *cp;
	char *file;
	const struct mime_handler *handler;
	int cl = 0;
	auth_t auth;

	user_agent = "";
	header_sent = 0;
	authorization = boundary = NULL;
	bzero(line, sizeof(line));

	// Parse the first line of the request.
	if (!web_getline(line, sizeof(line))) {
		send_error(400, NULL, NULL);
		return;
	}

	_dprintf("%s\n", line);

	method = path = line;
	strsep(&path, " ");
	if (!path) {	// Avoid http server crash, added by honor 2003-12-08
		send_error(400, NULL, NULL);
		return;
	}
	while (*path == ' ') path++;

	if ((strcasecmp(method, "GET") != 0) && (strcasecmp(method, "POST") != 0)) {
		send_error(501, NULL, NULL);
		return;
	}

	protocol = path;
	strsep(&protocol, " ");
	if (!protocol) {	// Avoid http server crash, added by honor 2003-12-08
		send_error(400, NULL, NULL);
		return;
	}
	while (*protocol == ' ') protocol++;

	if (path[0] != '/') {
		send_error(400, NULL, NULL);
		return;
	}
	file = path + 1;

#if 0
	const char *hid;
	int n;

	hid = nvram_safe_get("http_id");
	n = strlen(hid);
	cprintf("file=%s hid=%s n=%d +n=%s\n", file, hid, n, path + n + 1);
	if ((strncmp(file, hid, n) == 0) && (path[n + 1] == '/')) {
		path += n + 1;
		file = path + 1;
		hidok = 1;

		cprintf("OK path=%s file=%s\n", path, file);
	}
#endif

	if ((cp = strchr(file, '?')) != NULL) {
		*cp = 0;
		setenv("QUERY_STRING", cp + 1, 1);
		webcgi_init(cp + 1);
	}

	if ((file[0] == '/') || (strncmp(file, "..", 2) == 0) || (strstr(file, "/../") != NULL) ||
		(strcmp(file + (strlen(file) - 3), "/..") == 0)) {
		send_error(400, NULL, NULL);
		return;
	}

	if ((file[0] == 0) || (strcmp(file, "index.asp") == 0)) {
		file = "status-overview.asp";
	}
	else if ((strcmp(file, "ext/") == 0) || (strcmp(file, "ext") == 0)) {
		file = "ext/index.asp";
	}

	cp = protocol;
	strsep(&cp, " ");
	cur = protocol + strlen(protocol) + 1;

	while (web_getline(cur, line + sizeof(line) - cur)) {
		if ((strcmp(cur, "\n") == 0) || (strcmp(cur, "\r\n") == 0)) {
			break;
		}

		if (strncasecmp(cur, "Authorization:", 14) == 0) {
			cp = &cur[14];
			cp += strspn(cp, " \t");
			authorization = cp;
			cur = cp + strlen(cp) + 1;
		}
		else if (strncasecmp(cur, "Content-Length:", 15) == 0) {
			cp = &cur[15];
			cp += strspn(cp, " \t");
			cl = strtoul(cp, NULL, 0);
			if ((cl < 0) || (cl >= INT_MAX)) {
				send_error(400, NULL, NULL);
				return;
			}
		}
		else if ((strncasecmp(cur, "Content-Type:", 13) == 0) && ((cp = strstr(cur, "boundary=")))) {
			boundary = &cp[9];
			for (cp = cp + 9; *cp && *cp != '\r' && *cp != '\n'; cp++);
			*cp = '\0';
			cur = ++cp;
		}
		else if (strncasecmp(cur, "User-Agent:", 11) == 0) {
			user_agent = cur + 11;
			user_agent += strspn(user_agent, " \t");
			cur = user_agent + strlen(user_agent);
			*cur++ = 0;
		}
	}

	post = (strcasecmp(method, "post") == 0);

	auth = auth_check(authorization);

#if 0
	cprintf("UserAgent: %s\n", user_agent);
	switch (auth) {
	case AUTH_NONE:
		cprintf("AUTH_NONE\n");
		break;
	case AUTH_OK:
		cprintf("AUTH_OK\n");
		break;
	case AUTH_BAD:
		cprintf("AUTH_BAD\n");
		break;
	default:
		cprintf("AUTH_?\n");
		break;
	}
#endif

	/*

	Chrome 1.0.154:
	- User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; en-US) AppleWebKit/525.19 (KHTML, like Gecko) Chrome/1.0.154.53 Safari/525.19
	- It *always* sends an 'AUTH_NONE request' first. This results in a
	  send_authenticate, then finally sends an 'AUTH_OK request'.
	- It does not clear the authentication even if AUTH_NONE request succeeds.
	- If user doesn't enter anything (blank) for credential, it sends the
	  previous credential.

	*/

	if (strcmp(file, "logout") == 0) {	// special case
		wi_generic(file, cl, boundary);
		eat_garbage();

		if (strstr(user_agent, "Chrome/") != NULL) {
			if (auth != AUTH_BAD) {
				send_authenticate();
				return;
			}
		}
		else {
			if (auth == AUTH_OK) {
				send_authenticate();
				return;
			}
		}
		send_error(404, NULL, "Goodbye");
		return;
	}

	if (auth == AUTH_BAD) {
		auth_fail(cl);
		return;
	}

	for (handler = &mime_handlers[0]; handler->pattern; handler++) {
		if (match(handler->pattern, file)) {
			if ((handler->auth) && (auth != AUTH_OK)) {
				auth_fail(cl);
				return;
			}

			if (handler->input) handler->input(file, cl, boundary);
			eat_garbage();
			if (handler->mime_type != NULL) send_header(200, NULL, handler->mime_type, handler->cache);
			if (handler->output) handler->output(file);
			return;
		}
	}

	if (auth != AUTH_OK) {
		auth_fail(cl);
		return;
	}

/*
	if ((!post) && (strchr(file, '.') == NULL)) {
		cl = strlen(file);
		if ((cl > 1) && (cl < 64)) {
			char alt[128];

			path = alt + 1;
			strcpy(path, file);

			cp = path + cl - 1;
			if (*cp == '/') *cp = 0;

			if ((cp = strrchr(path, '/')) != NULL) *cp = '-';

			strcat(path, ".asp");
			if (f_exists(path)) {
				alt[0] = '/';
				redirect(alt);
				return;
			}
		}
	}
*/

	send_error(404, NULL, NULL);
}

#ifdef TCONFIG_HTTPS

#define HTTPS_CRT_VER	"1"

static void save_cert(void)
{
	if (eval("tar", "-C", "/", "-czf", "/tmp/cert.tgz", "etc/cert.pem", "etc/key.pem") == 0) {
		if (nvram_set_file("https_crt_file", "/tmp/cert.tgz", 8192)) {
			nvram_set("crt_ver", HTTPS_CRT_VER);
			nvram_commit_x();
		}
	}
	unlink("/tmp/cert.tgz");
}

static void erase_cert(void)
{
	unlink("/etc/cert.pem");
	unlink("/etc/key.pem");
	nvram_unset("https_crt_file");
	nvram_unset("https_crt_gen");
}

static void start_ssl(void)
{
	int ok;
	int save;
	int retry;
	unsigned long long sn;
	char t[32];

	if (nvram_match("https_crt_gen", "1")) {
		erase_cert();
	}

	retry = 1;
	while (1) {
		save = nvram_match("https_crt_save", "1");

		if ((!f_exists("/etc/cert.pem")) || (!f_exists("/etc/key.pem"))) {
			ok = 0;
			if (save && nvram_match("crt_ver", HTTPS_CRT_VER)) {
				if (nvram_get_file("https_crt_file", "/tmp/cert.tgz", 8192)) {
					if (eval("tar", "-xzf", "/tmp/cert.tgz", "-C", "/", "etc/cert.pem", "etc/key.pem") == 0)
						ok = 1;
					unlink("/tmp/cert.tgz");
				}
			}
			if (!ok) {
				erase_cert();
				syslog(LOG_INFO, "Generating SSL certificate...");

				// browsers seems to like this when the ip address moves...	-- zzz
				f_read("/dev/urandom", &sn, sizeof(sn));

				sprintf(t, "%llu", sn & 0x7FFFFFFFFFFFFFFFULL);
				eval("gencert.sh", t);
			}
		}

		if ((save) && (*nvram_safe_get("https_crt_file")) == 0) {
			save_cert();
		}

		if (mssl_init("/etc/cert.pem", "/etc/key.pem")) return;

		erase_cert();

		syslog(retry ? LOG_WARNING : LOG_ERR, "Unable to start SSL");
		if (!retry) exit(1);
		retry = 0;
	}
}
#endif

static void init_id(void)
{
	char s[128];
	unsigned long long n;

	if (strncmp(nvram_safe_get("http_id"), "TID", 3) != 0) {
		f_read("/dev/urandom", &n, sizeof(n));
		sprintf(s, "TID%llx", n);
		nvram_set("http_id", s);
	}

	nvram_unset("http_id_warn");
}

void check_id(const char *url)
{
	if (!nvram_match("http_id", webcgi_safeget("_http_id", ""))) {
#if 0
		const char *hid = nvram_safe_get("http_id");
		if (url) {
			const char *a;
			int n;

			// http://xxx/yyy/TID/zzz
			if (((a = strrchr(url, '/')) != NULL) && (a != url)) {
				n = strlen(hid);
				a -= n;
				if ((a > url) && (*(a - 1) == '/')) {
					if (strncmp(a, hid, n) == 0) return;
				}
			}
		}

#endif

		char s[72];
		char *i;
		char *u;
#ifdef TCONFIG_IPV6
		char a[INET6_ADDRSTRLEN];
#else
		char a[INET_ADDRSTRLEN];
#endif
		void *addr = NULL;

		if (clientsai.ss_family == AF_INET)
			addr = &( ((struct sockaddr_in*) &clientsai)->sin_addr );
#ifdef TCONFIG_IPV6
		else if (clientsai.ss_family == AF_INET6)
			addr = &( ((struct sockaddr_in6*) &clientsai)->sin6_addr );
#endif
		inet_ntop(clientsai.ss_family, addr, a, sizeof(a));

		sprintf(s, "%s,%ld", a, time(NULL) & 0xFFC0);
		if (!nvram_match("http_id_warn", s)) {
			nvram_set("http_id_warn", s);

			strlcpy(s, webcgi_safeget("_http_id", ""), sizeof(s));
			i = js_string(s);	// quicky scrub

			strlcpy(s, url, sizeof(s));
			u = js_string(s);

			if ((i != NULL) && (u != NULL)) {
				syslog(LOG_WARNING, "Invalid ID '%s' from %s for /%s", i, a, u);
			}

			//	free(u);
			//	free(i);
		}

		exit(1);
	}
}

static void add_listen_socket(const char *addr, int server_port, int do_ipv6, int do_ssl)
{
	int listenfd, n;
	struct sockaddr_storage sai_stor;

#ifdef TCONFIG_IPV6
	sa_family_t HTTPD_FAMILY = do_ipv6 ? AF_INET6 : AF_INET;
#else
#define HTTPD_FAMILY AF_INET
#endif

	if (listeners.count >= HTTP_MAX_LISTENERS) {
		syslog(LOG_ERR, "number of listeners exceeded the max allowed (%d)", HTTP_MAX_LISTENERS);
		return;
	}

	if (server_port <= 0) {
		IF_TCONFIG_HTTPS(if (do_ssl) server_port = 443; else)
		server_port = 80;
	}

	if ((listenfd = socket(HTTPD_FAMILY, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_ERR, "create listening socket: %m");
		return;
	}
	fcntl(listenfd, F_SETFD, FD_CLOEXEC);

	n = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n));

#ifdef TCONFIG_IPV6
	if (do_ipv6) {
		struct sockaddr_in6 *sai = (struct sockaddr_in6 *) &sai_stor;
		sai->sin6_family = HTTPD_FAMILY;
		sai->sin6_port = htons(server_port);
		if (addr && *addr)
			inet_pton(HTTPD_FAMILY, addr, &(sai->sin6_addr));
		else
			sai->sin6_addr = in6addr_any;
		n = 1;
		setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, (char*)&n, sizeof(n));
	} else
#endif
	{
		struct sockaddr_in *sai = (struct sockaddr_in *) &sai_stor;
		sai->sin_family = HTTPD_FAMILY;
		sai->sin_port = htons(server_port);
		sai->sin_addr.s_addr = (addr && *addr) ? inet_addr(addr) : INADDR_ANY;
	}

	if (bind(listenfd, (struct sockaddr *)&sai_stor, sizeof(sai_stor)) < 0) {
		syslog(LOG_ERR, "bind: [%s]:%d: %m", (addr && *addr) ? addr : (IF_TCONFIG_IPV6(do_ipv6 ? "::" :) ""), server_port);
		close(listenfd);
		return;
	}

	if (listen(listenfd, 64) < 0) {
		syslog(LOG_ERR, "listen: %m");
		close(listenfd);
		return;
	}

	if (listenfd >= 0) {
		_dprintf("%s: added IPv%d listener [%d] for %s:%d, ssl=%d\n",
			__FUNCTION__, do_ipv6 ? 6 : 4, listenfd,
			(addr && *addr) ? addr : "addr_any", server_port, do_ssl);
		listeners.listener[listeners.count].listenfd = listenfd;
		listeners.listener[listeners.count++].ssl = do_ssl;
		FD_SET(listenfd, &listeners.lfdset);
		if (maxfd < listenfd) maxfd = listenfd;
	}
}

static void setup_listeners(int do_ipv6)
{
	char ipaddr[INET6_ADDRSTRLEN];
	char ipaddr1[INET6_ADDRSTRLEN];
	char ipaddr2[INET6_ADDRSTRLEN];
	char ipaddr3[INET6_ADDRSTRLEN];
	IF_TCONFIG_IPV6(const char *wanaddr);
	int wanport, p;
	IF_TCONFIG_IPV6(int wan6port);

	wanport = nvram_get_int("http_wanport");
#ifdef TCONFIG_IPV6
	wan6port = wanport;
	if (do_ipv6) {
		// get the configured routable IPv6 address from the lan iface.
		// add_listen_socket() will fall back to in6addr_any
		// if NULL or empty address is returned
		strlcpy(ipaddr, getifaddr(nvram_safe_get("lan_ifname"), AF_INET6, 0) ? : "", sizeof(ipaddr));
	}
	else
#endif
	strlcpy(ipaddr, nvram_safe_get("lan_ipaddr"), sizeof(ipaddr));
	strlcpy(ipaddr1, nvram_safe_get("lan1_ipaddr"), sizeof(ipaddr));
	strlcpy(ipaddr2, nvram_safe_get("lan2_ipaddr"), sizeof(ipaddr));
	strlcpy(ipaddr3, nvram_safe_get("lan3_ipaddr"), sizeof(ipaddr));

	if (!nvram_match("http_enable", "0")) {
		p = nvram_get_int("http_lanport");
		add_listen_socket(ipaddr, p, do_ipv6, 0);
		if (strcmp(ipaddr1,"")!=0)
			add_listen_socket(ipaddr1, p, do_ipv6, 0);
		if (strcmp(ipaddr2,"")!=0)
			add_listen_socket(ipaddr2, p, do_ipv6, 0);
		if (strcmp(ipaddr3,"")!=0)
			add_listen_socket(ipaddr3, p, do_ipv6, 0);

		IF_TCONFIG_IPV6(if (do_ipv6 && wanport == p) wan6port = 0);
	}

#ifdef TCONFIG_HTTPS
	if (!nvram_match("https_enable", "0")) {
		do_ssl = 1;
		p = nvram_get_int("https_lanport");
		add_listen_socket(ipaddr, p, do_ipv6, 1);
		if (strcmp(ipaddr1,"")!=0)
			add_listen_socket(ipaddr1, p, do_ipv6, 1);
		if (strcmp(ipaddr2,"")!=0)
			add_listen_socket(ipaddr2, p, do_ipv6, 1);
		if (strcmp(ipaddr3,"")!=0)
			add_listen_socket(ipaddr3, p, do_ipv6, 1);

		IF_TCONFIG_IPV6(if (do_ipv6 && wanport == p) wan6port = 0);
	}
#endif

	if ((wanport) && nvram_match("wk_mode","gateway") && nvram_match("remote_management", "1") && check_wanup()) {
		IF_TCONFIG_HTTPS(if (nvram_match("remote_mgt_https", "1")) do_ssl = 1);
#ifdef TCONFIG_IPV6
		if (do_ipv6) {
			if (*ipaddr && wan6port) {
				add_listen_socket(ipaddr, wan6port, 1, nvram_match("remote_mgt_https", "1"));
			}
			if (*ipaddr || wan6port) {
				// get the IPv6 address from wan iface
				wanaddr = getifaddr((char *)get_wan6face(), AF_INET6, 0);
				if (wanaddr && *wanaddr && strcmp(wanaddr, ipaddr) != 0) {
					add_listen_socket(wanaddr, wanport, 1, nvram_match("remote_mgt_https", "1"));
				}
			}
		} else
#endif
		{
			int i;
			char *ip;
			wanface_list_t wanfaces;

			memcpy(&wanfaces, get_wanfaces(), sizeof(wanfaces));
			for (i = 0; i < wanfaces.count; ++i) {
				ip = wanfaces.iface[i].ip;
				if (!(*ip) || strcmp(ip, "0.0.0.0") == 0)
					continue;
				add_listen_socket(ip, wanport, 0, nvram_match("remote_mgt_https", "1"));
			}
		}
	}
}

static void close_listen_sockets(void)
{
	int i;

	for (i = listeners.count - 1; i >= 0; --i) {
		if (listeners.listener[i].listenfd >= 0)
			close(listeners.listener[i].listenfd);
	}
	listeners.count = 0;
	FD_ZERO(&listeners.lfdset);
	maxfd = -1;
}

int main(int argc, char **argv)
{
	int c;
	int debug = 0;
	fd_set rfdset;
	int i, n;
	struct sockaddr_storage sai;
	char bind[128];
	char *port = NULL;
#ifdef TCONFIG_IPV6
	int ip6 = 0;
#else
#define ip6 0
#endif

	openlog("httpd", LOG_PID, LOG_DAEMON);

	do_ssl = 0;
	listeners.count = 0;
	FD_ZERO(&listeners.lfdset);
	memset(bind, 0, sizeof(bind));

	while ((c = getopt(argc, argv, "hdp:s:")) != -1) {
		switch (c) {
		case 'h':
			printf(
				"Usage: %s [options]\n"
				"  -d        Debug mode / do not demonize\n"
				, argv[0]);
			return 1;
		case 'd':
			debug = 1;
			break;
		case 'p':
		case 's':
			// [addr:]port
			if ((port = strrchr(optarg, ':')) != NULL) {
				if ((optarg[0] == '[') && (port > optarg) && (port[-1] == ']'))
					memcpy(bind, optarg + 1, MIN(sizeof(bind), (int)(port - optarg) - 2));
				else
					memcpy(bind, optarg, MIN(sizeof(bind), (int)(port - optarg)));
				port++;
			}
			else {
				port = optarg;
			}

			IF_TCONFIG_HTTPS(if (c == 's') do_ssl = 1);
			IF_TCONFIG_IPV6(ip6 = (*bind && strchr(bind, ':')));
			add_listen_socket(bind, atoi(port), ip6, (c == 's'));

			memset(bind, 0, sizeof(bind));
			break;
		}
	}

	setup_listeners(0);
	IF_TCONFIG_IPV6(if (ipv6_enabled()) setup_listeners(1));

	if (listeners.count == 0) {
		syslog(LOG_ERR, "can't bind to any address");
		return 1;
	}
	_dprintf("%s: initialized %d listener(s)\n", __FUNCTION__, listeners.count);

	IF_TCONFIG_HTTPS(if (do_ssl) start_ssl());

	init_id();

	if (!debug) {
		if (daemon(1, 1) == -1) {
			syslog(LOG_ERR, "daemon: %m");
			return 0;
		}

		char s[16];
		sprintf(s, "%d", getpid());
		f_write_string("/var/run/httpd.pid", s, 0, 0644);
	}
	else {
		printf("DEBUG mode, not daemonizing\n");
	}

	signal(SIGPIPE, SIG_IGN);
	signal(SIGALRM, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);

	for (;;) {

		/* Do a select() on at least one and possibly many listen fds.
		** If there's only one listen fd then we could skip the select
		** and just do the (blocking) accept(), saving one system call;
		** that's what happened up through version 1.18. However there
		** is one slight drawback to that method: the blocking accept()
		** is not interrupted by a signal call. Since we definitely want
		** signals to interrupt a waiting server, we use select() even
		** if there's only one fd.
		*/
		rfdset = listeners.lfdset;
		_dprintf("%s: calling select(maxfd=%d)...\n", __FUNCTION__, maxfd);
		if (select(maxfd + 1, &rfdset, NULL, NULL, NULL) < 0) {
			if (errno != EINTR && errno != EAGAIN) sleep(1);
			continue;
		}

		for (i = listeners.count - 1; i >= 0; --i) {
			if (listeners.listener[i].listenfd < 0 ||
			    !FD_ISSET(listeners.listener[i].listenfd, &rfdset))
				continue;

			do_ssl = 0;
			n = sizeof(sai);
			_dprintf("%s: calling accept(listener=%d)...\n", __FUNCTION__, listeners.listener[i].listenfd);
			connfd = accept(listeners.listener[i].listenfd,
				(struct sockaddr *)&sai, &n);
			if (connfd < 0) {
				_dprintf("accept: %m");
				continue;
			}
			_dprintf("%s: connfd = accept(listener=%d) = %d\n", __FUNCTION__, listeners.listener[i].listenfd, connfd);

			if (!wait_action_idle(10)) {
				_dprintf("router is busy");
				continue;
			}

			if (fork() == 0) {
				IF_TCONFIG_HTTPS(do_ssl = listeners.listener[i].ssl);
				close_listen_sockets();
				webcgi_init(NULL);

				clientsai = sai;
				if (!check_wlaccess()) {
					exit(0);
				}

				struct timeval tv;
				tv.tv_sec = 60;
				tv.tv_usec = 0;
				setsockopt(connfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
				setsockopt(connfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

				n = 1;
				setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *)&n, sizeof(n));

				fcntl(connfd, F_SETFD, FD_CLOEXEC);

				if (web_open()) handle_request();
				web_close();
				exit(0);
			}
			close(connfd);
		}
	}

	close_listen_sockets();
	return 0;
}
