/*

	MDU -- Mini DDNS Updater
	Copyright (C) 2007-2009 Jonathan Zarate

	Licensed under GNU GPL v2 or later versions.

*/

//	#define DEBUG
#define _GNU_SOURCE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <time.h>

#include <shared.h>
#include <shutils.h>
#include <tomato_version.h>

#include "md5.h"
#include "mssl.h"



#ifdef DEBUG
#define AGENT		"MDU - TEST DDNS CLIENT"
#else
#define AGENT		"MDU - Tomato Firmware " TOMATO_MAJOR "." TOMATO_MINOR
#endif

#define MAX_OPTION_LENGTH	256
#define BLOB_SIZE	(4 * 1024)


#define M_UNKNOWN_ERROR__D		"Unknown error (%d)."
#define M_UNKNOWN_RESPONSE__D		"Unknown response (%d)."
#define M_INVALID_HOST			"Invalid hostname."
#define M_INVALID_AUTH			"Invalid authentication."
#define M_INVALID_PARAM__D		"Invalid parameter (%d)."
#define M_INVALID_PARAM__S		"Invalid parameter (%s)."
#define M_TOOSOON				"Update was too soon or too frequent. Please try again later."
#define M_ERROR_GET_IP			"Error obtaining IP address."
#define M_SAME_IP				"The IP address is the same."
#define M_DOWN					"Server temporarily down or under maintenance."

char *blob = NULL;
int error_exitcode = 1;

int g_argc;
char **g_argv;

char *f_argv[32];
int f_argc = -1;
int refresh = 0;

static void save_cookie(void);
static void update_noip_refresh(void);


static void trimamp(char *s)
{
	int n;

	n = strlen(s);
	if ((n > 0) && (s[--n] == '&')) s[n] = 0;
}

static const char *get_option(const char *name)
{
	char *p;
	int i;
	int n;
	FILE *f;
	const char *c;
	char s[384];

	if (f_argc < 0) {
		f_argc = 0;
		if ((c = get_option("conf")) != NULL) {
			if ((f = fopen(c, "r")) != NULL) {
				while (fgets(s, sizeof(s), f)) {
					p = s;
					if ((s[0] == '-') && (s[1] == '-')) p += 2;
					if ((c = strchr(p, ' ')) != NULL) {
						n = strlen(p);
						if (p[n - 1] == '\n') p[n - 1] = 0;

						n = strlen(c + 1);
						if (n <= 0) continue;
						if (n >= MAX_OPTION_LENGTH) exit(88);

						if ((p = strdup(p)) == NULL) exit(99);
						f_argv[f_argc++] = p;
						if (f_argc >= (sizeof(f_argv) / sizeof(f_argv[0]))) break;
					}
				}
				fclose(f);
			}
		}
	}

	n = strlen(name);
	for (i = 0; i < f_argc; ++i) {
		c = f_argv[i];
		if ((strncmp(c, name, n) == 0) && (c[n] == ' ')) {
			return c + n + 1;
		}
	}

	for (i = 0; i < g_argc; ++i) {
		p = g_argv[i];
		if ((p[0] == '-') && (p[1] == '-')) {
			if (strcmp(p + 2, name) == 0) {
				++i;
				if ((i >= g_argc) || (strlen(g_argv[i]) >= MAX_OPTION_LENGTH)) break;
				return g_argv[i];
			}
		}
	}
	return NULL;
}

/*
static const char *get_option_safe(const char *name)
{
	return get_option(name) ? : "";
}
*/

static const char *get_option_required(const char *name)
{
	const char *p;

	if ((p = get_option(name)) != NULL) return p;
	fprintf(stderr, "Required option --%s is missing.\n", name);
	exit(2);
}

static const char *get_option_or(const char *name, const char *alt)
{
	return get_option(name) ? : alt;
}

static int get_option_onoff(const char *name, int def)
{
	const char *p;

	if ((p = get_option(name)) == NULL) return def;
	if ((strcmp(p, "on") == 0) || (strcmp(p, "1") == 0)) return 1;
	if ((strcmp(p, "off") == 0) || (strcmp(p, "0") == 0)) return 0;

	fprintf(stderr, "--%s requires the value off/on or 0/1.\n", name);
	exit(2);
}

static const char *md5_string(const char *value)
{
	static char buf[(MD5_DIGEST_BYTES + 1) * 2];
	unsigned char digestbuf[MD5_DIGEST_BYTES];
	int i;

	md5_buffer(value, strlen(value), digestbuf);
	for (i = 0; i < MD5_DIGEST_BYTES; i++)
		sprintf(&buf[i * 2], "%02x", digestbuf[i]);
	return buf;
}


static void save_msg(const char *s)
{
	const char *path;

	if ((path = get_option("msg")) != NULL) {
		f_write_string(path, s, FW_NEWLINE, 0);
	}
}

static void error(const char *fmt, ...)
{
	va_list args;
	char s[512];

	va_start(args, fmt);
	vsnprintf(s, sizeof(s), fmt, args);
	s[sizeof(s) - 1] = 0;
	va_end(args);

	_dprintf("%s: %s\n", __FUNCTION__, s);

	printf("%s\n", s);
	save_msg(s);
	exit(error_exitcode);
}

static void success_msg(const char *msg)
{
	save_cookie();

	_dprintf("%s\n", __FUNCTION__);

	printf("%s\n", msg);
	save_msg(msg);
	exit(0);
}

static void success(void)
{
	success_msg("Update successful.");
}



static const char *get_dump_name(void)
{
#ifdef DEBUG
	return get_option_or("dump", "/tmp/mdu.txt");
#else
	return get_option("dump");
#endif
}

static int _wget(int ssl, const char *host, int port, const char *request, char *buffer, int bufsize, char **body)
{
	struct hostent *he;
	struct sockaddr_in sa;
	int sd;
	FILE *f;
	int i;
	int trys;
	char *p;
	const char *c;
	struct timeval tv;

	_dprintf("\n*** %s\n", host);

	sd = -1;	// for gcc warning

	for (trys = 4; trys > 0; --trys) {
		_dprintf("_wget trys=%d\n", trys);

		for (i = 4; i > 0; --i) {
			if ((he = gethostbyname(host)) != NULL) {
				if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					return -1;
				}
				memset(&sa, 0, sizeof(sa));
				sa.sin_family = AF_INET;
				sa.sin_port = htons(port);
				memcpy(&sa.sin_addr, he->h_addr, sizeof(sa.sin_addr));

#ifdef DEBUG
				struct in_addr ia;
				ia.s_addr = sa.sin_addr.s_addr;

				_dprintf("[%s][%d]\n", inet_ntoa(ia), port);
				_dprintf("connecting...\n");
#endif

				if (connect_timeout(sd, (struct sockaddr *)&sa, sizeof(sa), 10) == 0) {
					_dprintf("connected\n");
					break;
				}
#ifdef DEBUG
				perror("connect");
#endif
				close(sd);
				sleep(2);
			}
		}
		if (i <= 0) return -1;

		tv.tv_sec = 10;
		tv.tv_usec = 0;
		setsockopt(sd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
		setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

		if (ssl) {
			mssl_init(NULL, NULL);
			f = ssl_client_fopen(sd);
		}
		else {
			f = fdopen(sd, "r+");
		}
		if (f == NULL) {
			_dprintf("error opening\n");
			close(sd);
			continue;
		}

		i = strlen(request);
		if (fwrite(request, 1, i, f) != i) {
			_dprintf("error writing i=%d\n", i);
			fclose(f);
			close(sd);
			continue;
		}
		_dprintf("sent request\n");

		i = fread(buffer, 1, bufsize, f);
		if (i <= 0) {
			fclose(f);
			close(sd);
			_dprintf("error reading i=%d\n", i);
			continue;
		}
		buffer[i] = 0;

		_dprintf("recvd=[%s], i=%d\n", buffer, i);

		fclose(f);
		close(sd);

		if ((c = get_dump_name()) != NULL) {
			if ((f = fopen(c, "a")) != NULL) {
				fprintf(f, "\n[%s:%d]\nREQUEST\n", host, port);
				fputs(request, f);
				fputs("\nREPLY\n", f);
				fputs(buffer, f);
				fputs("\nEND\n", f);
				fclose(f);
			}
		}

		if ((sscanf(buffer, "HTTP/1.%*d %d", &i) == 1) && (i >= 100) && (i <= 999)) {
			_dprintf("HTTP/1.* i=%d\n", i);
			if ((p = strstr(buffer, "\r\n\r\n")) != NULL) p += 4;
				else if ((p = strstr(buffer, "\n\n")) != NULL) p += 2;
			if (p) {
				if (body) {
					*body = p;
					_dprintf("body=[%s]\n", p);
				}
				return i;
			}
			else {
				_dprintf("!p\n");
			}
		}
	}

	return -1;
}

static int wget(int ssl, int static_host, const char *host, const char *get, const char *header, int auth, char **body)
{
	char *p;
	int port;
	char a[512];
	char b[512];
	int n;

	if (!static_host) host = get_option_or("server", host);

	n = strlen(get);
	if (header) n += strlen(header);
	if (n > (BLOB_SIZE - 512)) return -1;	// just don't go over 512 below...

	sprintf(blob,
		"GET %s HTTP/1.0\r\n"
		"Host: %s\r\n"
		"User-Agent: " AGENT "\r\n",
		get, host);
	if (auth) {
		sprintf(a, "%s:%s", get_option_required("user"), get_option_required("pass"));
		n = base64_encode(a, b, strlen(a));
		b[n] = 0;
		sprintf(blob + strlen(blob), "Authorization: Basic %s\r\n", b);
	}
	if ((header) && ((n = strlen(header)) > 0)) {
		strcat(blob, header);
		if (header[n - 1] != '\n') strcat(blob, "\r\n");
	}
	strcat(blob, "\r\n");


	_dprintf("blob=[%s]\n", blob);

	port = ssl ? 443 : 80;
	strlcpy(a, host, sizeof(a));
	if ((p = strrchr(a, ':')) != NULL) {
		*p = 0;
		if ((n = atoi(p + 1)) > 0) port = n;
	}

	if ((p = strdup(blob)) == NULL) return -1;
	n = _wget(ssl, a, port, p, blob, BLOB_SIZE, body);
	free(p);

	_dprintf("%s: n=%d\n", __FUNCTION__, n);
	return n;
}



int read_tmaddr(const char *name, long *tm, char *addr)
{
	char s[64];

	if (f_read_string(name, s, sizeof(s)) > 0) {
		if (sscanf(s, "%ld,%15s", tm, addr) == 2) {
			_dprintf("%s: s=%s tm=%ld addr=%s\n", __FUNCTION__, s, *tm, addr);
			if ((tm > 0) && (inet_addr(addr) != -1)) return 1;
		}
		else {
			_dprintf("%s: unknown=%s\n", __FUNCTION__, s);
		}
	}
	return 0;
}

const char *get_address(int required)
{
	char *body;
	struct in_addr ia;
	const char *c, *d;
	char *p, *q;
	char s[64];
	char cache_name[64];
	static char addr[16];
	long ut, et;

	if ((c = get_option("addr")) != NULL) {
		if (*c == '@') {
			++c;
			if ((*c != 0) && (strlen(c) < 20)) {
				ut = get_uptime();

				if ((d = get_option("addrcache")) != NULL) strlcpy(cache_name, d, sizeof(cache_name));
					else sprintf(cache_name, "%s.ip", c);
				if (read_tmaddr(cache_name, &et, addr)) {
					if ((et > ut) && ((et - ut) <= (10 * 60))) {
						_dprintf("%s: Using cached address %s from %s. Expires in %ld seconds.\n", __FUNCTION__, addr, cache_name, et - ut);
						return addr;
					}
				}

				if (strcmp(c, "dyndns") == 0) {
					if ((wget(0, 1, "checkip.dyndns.org:8245", "/", NULL, 0, &body) != 200) &&
						(wget(0, 1, "checkip.dyndns.org", "/", NULL, 0, &body) != 200)) {
						// Current IP Address: 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "zoneedit") == 0 || strcmp(c, "szoneedit") == 0) {
					if (wget(0, 1, "dynamic.zoneedit.com", "/checkip.html", NULL, 0, &body) != 200) {
						// Current IP Address: 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "tzo") == 0) {
					if ((wget(0, 1, "echo.tzo.com:21333", "/ip.shtml", NULL, 0, &body) != 200) &&
						(wget(0, 1, "echo.tzo.com", "/ip.shtml", NULL, 0, &body) != 200)) {
						// IPAddress:1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "noip") == 0) {
					if (wget(0, 1, "ip1.dynupdate.no-ip.com", "/", NULL, 0, &body) != 200) {
						// 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "dnsomatic") == 0) {
					if (wget(0, 1, "myip.dnsomatic.com", "/", NULL, 0, &body) != 200) {
						// 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "pairnic") == 0) {
					if (wget(0, 1, "myip.pairnic.com", "/", NULL, 0, &body) != 200) {
						// Current IP Address: 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "ovh") == 0) {
					if (wget(0, 1, "www.ovh.com", "/", NULL, 0, &body) != 200) {
						// Current IP Address: 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}
				else if (strcmp(c, "changeip") == 0) {
					if (wget(0, 1, "nic.changeip.com", "/", NULL, 0, &body) != 200) {
						// Current IP Address: 1.2.3.4
						error(M_ERROR_GET_IP);
					}
				}

				if ((p = strstr(body, "Address:")) != NULL) {
					// dyndns, zoneedit, tzo, pairnic, ovh, changeip
					p += 8;	// note: tzo doesn't have a space
				}
				else {
					// noip, dnsomatic
					p = body;
				}

				while (*p == ' ') ++p;

				q = p;
				while (((*q >= '0') && (*q <= '9')) || (*q == '.')) ++q;
				*q = 0;

				if ((ia.s_addr = inet_addr(p)) != -1) {
					q = inet_ntoa(ia);
					sprintf(s, "%ld,%s", ut + (10 * 60), p);
					f_write_string(cache_name, s, 0, 0);

					_dprintf("%s: saved '%s'\n", __FUNCTION__, s);
					return p;
				}
			}
			error(M_ERROR_GET_IP);
		}
		return c;
	}

	return required ? get_option_required("addr") : NULL;
}

static void append_addr_option(char *buffer, const char *format)
{
	const char *c;

	if ((c = get_address(0)) != NULL) {
		sprintf(buffer + strlen(buffer), format, c);
	}
}

// -----------------------------------------------------------------------------


/*

	DNS Update API
	http://www.dyndns.com/developers/specs/

	---

	DynDNS:
		http: 80, 8245
		https: 443

	http://test:test@members.dyndns.org/nic/update?system=dyndns&hostname=test.shacknet.nu

	GET /nic/update?
	    system=statdns&
	    hostname=yourhost.ourdomain.ext,yourhost2.dyndns.org&
	    myip=ipaddress&
	    wildcard=OFF&
	    mx=mail.exchanger.ext&
	    backmx=NO&
	    offline=NO
	    HTTP/1.0
	Host: members.dyndns.org
	Authorization: Basic username:pass
	User-Agent: Company - Device - Version Number

*/
static void update_dua(const char *type, int ssl, const char *server, const char *path, int reqhost)
{
	const char *p;
	char query[2048];
	int r;
	char *body;

	// +opt
	sprintf(query, "%s?", path ? path : get_option_required("path"));

	// +opt
	if (type) sprintf(query + strlen(query), "system=%s&", type);

	// +opt
	p = reqhost ? get_option_required("host") : get_option("host");
	if (p) sprintf(query + strlen(query), "hostname=%s&", p);

	// +opt
	if (((p = get_option("mx")) != NULL) && (*p)) {
		sprintf(query + strlen(query),
			"mx=%s&"
			"backmx=%s&",
			p,
			(get_option_onoff("backmx", 0)) ? "YES" : "NO");
	}

	// +opt
	append_addr_option(query, "myip=%s&");

	if (get_option_onoff("wildcard", 0)) {
		strcat(query, "wildcard=ON");
	}

	trimamp(query);

	r = wget(ssl, 0, server ? server : get_option_required("server"), query, NULL, 1, &body);
	switch (r) {
	case 200:
		if ((strstr(body, "dnserr")) || (strstr(body, "911"))) {
			error_exitcode = 2;
			error(M_DOWN);
		}

		if ((strstr(body, "badsys")) || (strstr(body, "numhost")) || (strstr(body, "ILLEGAL"))) {
			error(M_INVALID_PARAM__D, -1);
		}
		if (strstr(body, "badagent")) {
			error(M_INVALID_PARAM__D, -2);
		}
		if ((strstr(body, "badauth")) || (strstr(body, "NOACCESS")) || (strstr(body, "!donator"))) {
			error(M_INVALID_AUTH);
		}
		if ((strstr(body, "notfqdn")) || (strstr(body, "!yours")) || strstr(body, "nohost") ||
			(strstr(body, "abuse")) || strstr(body, "NOSERVICE")) {
			error(M_INVALID_HOST);
		}
		if (strstr(body, "TOOSOON")) {
			error(M_TOOSOON);
		}

		if (strstr(body, "nochg")) {
			if ((strcmp(get_option("service"), "noip") == 0) && (refresh)) {
				update_noip_refresh();
			}
			success();
			return;
		}

		if ((strstr(body, "good")) || (strstr(body, "NOERROR"))) {
			success();
			return;
		}

		error(M_UNKNOWN_RESPONSE__D, -1);
		break;
	case 401:
		error(M_INVALID_AUTH);
		break;
	}
	error(M_UNKNOWN_ERROR__D, r);
}


/*

	namecheap.com
	http://namecheap.simplekb.com/kb.aspx?show=article&articleid=27&categoryid=22

	---

http://dynamicdns.park-your-domain.com/update?host=host_name&domain=domain.com&password=domain_password[&ip=your_ip]

ok response:

"HTTP/1.1 200 OK
...

<?xml version="1.0"?>
<interface-response>
<IP>12.123.123.12</IP>
<Command>SETDNSHOST</Command>
<Language>eng</Language>
<ErrCount>0</ErrCount>
<ResponseCount>0</ResponseCount>
<MinPeriod>1</MinPeriod>
<MaxPeriod>10</MaxPeriod>
<Server>Reseller9</Server>
<Site>Namecheap</Site>
<IsLockable>True</IsLockable>
<IsRealTimeTLD>True</IsRealTimeTLD>
<TimeDifference>+03.00</TimeDifference>
<ExecTime>0.0625</ExecTime>
<Done>true</Done>
<debug><![CDATA[]]></debug>
</interface-response>"


error response:

"HTTP/1.1 200 OK
...

<?xml version="1.0"?>
<interface-response>
<Command>SETDNSHOST</Command>
<Language>eng</Language>
<ErrCount>1</ErrCount>
<errors>
<Err1>Passwords do not match</Err1>
</errors>
<ResponseCount>1</ResponseCount>
<responses>
<response>
<ResponseNumber>304156</ResponseNumber>
<ResponseString>Validation error; invalid ; password</ResponseString>
</response>
</responses>
<MinPeriod>1</MinPeriod>
<MaxPeriod>10</MaxPeriod>
<Server>Reseller1</Server>
<Site></Site>
<IsLockable>True</IsLockable>
<IsRealTimeTLD>True</IsRealTimeTLD>
<TimeDifference>+03.00</TimeDifference>
8<ExecTime>0.0625</ExecTime>
<Done>true</Done>
<debug><![CDATA[]]></debug>
</interface-response>"

*/
static void update_namecheap(void)
{
	int r;
	char *p;
	char *q;
	char *body;
	char query[2048];

	// +opt +opt +opt
	sprintf(query, "/update?host=%s&domain=%s&password=%s",
		get_option_required("host"), get_option("user") ? : get_option_required("domain"), get_option_required("pass"));

	// +opt
	append_addr_option(query, "&ip=%s");

	r = wget(0, 0, "dynamicdns.park-your-domain.com", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "<ErrCount>0<") != NULL) {
			success();
		}
		if ((p = strstr(body, "<Err1>")) != NULL) {
			p += 6;
			if ((q = strstr(p, "</")) != NULL) {
				*q = 0;
				if ((q - p) >= 64) p[64] = 0;
				error("%s", p);
			}
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}


/*

	eNom
	http://www.enom.com/help/faq_dynamicdns.asp

	---

good:
;URL Interface<br>
;Machine is Reseller5<br>
IP=127.0.0.1
Command=SETDNSHOST
Language=eng

ErrCount=0
ResponseCount=0
MinPeriod=1
MaxPeriod=10
Server=Reseller5
Site=eNom
IsLockable=True
IsRealTimeTLD=True
TimeDifference=+08.00
ExecTime=0.500

bad:
;URL Interface<br>
;Machine is Reseller4<br>
-Command=SETDNSHOST
Language=eng
ErrCount=1
Err1=Passwords do not match
ResponseCount=1
ResponseNumber1=304156
ResponseString1=Validation error; invalid ; password
MinPeriod=1
MaxPeriod=10
Server=Reseller4
Site=
IsLockable=True
IsRealTimeTLD=True
TimeDifference=+08.00
ExecTime=0.235
Done=true

*/
static void update_enom(void)
{
	int r;
	char *p;
	char *q;
	char *body;
	char query[2048];

	// http://dynamic.name-services.com/interface.asp?Command=SetDNSHost&HostName=test&Zone=test.com&Address=1.2.3.4&DomainPassword=password

	// +opt +opt +opt
	sprintf(query, "/interface.asp?Command=SetDNSHost&HostName=%s&Zone=%s&DomainPassword=%s",
		get_option_required("host"), get_option("user") ? : get_option_required("domain"), get_option_required("pass"));

	// +opt
	append_addr_option(query, "&Address=%s");

	r = wget(0, 0, "dynamic.name-services.com", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "ErrCount=0") != NULL) {
			success();
		}
		if ((p = strstr(body, "Err1=")) != NULL) {
			p += 5;
			if ((q = strchr(p, '\n')) != NULL) {
				*q = 0;
				if ((q - p) >= 64) p[64] = 0;
				if ((q = strchr(p, '\r')) != NULL) *q = 0;
				error("%s", p);
			}
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}





/*

	dnsExit
	http://www.dnsexit.com/Direct.sv?cmd=ipClients

	---

"HTTP/1.1 200 OK
...

 HTTP/1.1 200 OK
0=Success"

" HTTP/1.1 200 OK
11=fail to find foo.bar.com"

" HTTP/1.1 200 OK
4=Update too often. Please wait at least 8 minutes since the last update"

" HTTP/1.1 200 OK" <-- extra in body?

*/
static void update_dnsexit(void)
{
	int r;
	char *body;
	char query[2048];

	// +opt +opt +opt
	sprintf(query, "/RemoteUpdate.sv?action=edit&login=%s&password=%s&host=%s",
		get_option_required("user"), get_option_required("pass"), get_option_required("host"));

	// +opt
	append_addr_option(query, "&myip=%s");

	r = wget(0, 0, "www.dnsexit.com", query, NULL, 0, &body);
	if (r == 200) {
		// (\d+)=.+

		if ((strstr(body, "0=Success") != NULL) || (strstr(body, "1=IP") != NULL)) {
			success();
		}
		if ((strstr(body, "2=Invalid") != NULL) || (strstr(body, "3=User") != NULL)) {
			error(M_INVALID_AUTH);
		}
		if ((strstr(body, "10=Host") != NULL) || (strstr(body, "11=fail") != NULL)) {
			error(M_INVALID_HOST);
		}
		if (strstr(body, "4=Update") != NULL) {
			error(M_TOOSOON);
		}

		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}


/*

	no-ip.com

	---

	Example response:

	mytest.testdomain.com:4

*/
/*
static void update_noip(void)
{
	int r;
	const char *c;
	char *body;
	char query[2048];

	// +opt +opt
	sprintf(query, "/dns?username=%s&password=%s&",
		get_option_required("user"), get_option_required("pass"));

	// +opt
	if (((c = get_option("group")) != NULL) && (*c)) {
		sprintf(query + strlen(query), "group=%s", c);
	}
	else {
		sprintf(query + strlen(query), "hostname=%s", get_option_required("host"));
	}

	// +opt
	append_addr_option(query, "&ip=%s");

	r = wget(0, 0, "dynupdate.no-ip.com", query, NULL, 0, &body);
	if (r == 200) {
		if ((c = strchr(body, ':')) != NULL) {
			++c;
			r = atoi(c);
			switch(r) {
			case 0:		// host same addr
				while (*c == ' ') ++c;
				if (*c != '0') {
					error(M_UNKNOWN_RESPONSE__D, -1);
					break;
				}
				// drop
			case 12:	// group same addr
			case 1:		// host updated
			case 11:	// group updated
				success();
				break;
			case 2:		// invalid hostname
			case 10:	// invalid group
			case 6:		// account disabled
			case 8:		// disabled hostname
				error(M_INVALID_HOST);
				break;
			case 3:		// invalid password
			case 4:		// invalid username
				error(M_INVALID_AUTH);
				break;
			case 5:
				error(M_TOOSOON);
				break;
			case 7:		// invalid IP supplied
			case 99:	// client banned
			case 100:	// invalid parameter
			case 9:		// redirect type
			case 13:	//
				error(M_INVALID_PARAM__D, r);
				break;
			default:
				error(M_UNKNOWN_RESPONSE__D, r);
				break;
			}
		}
		else {
			r = -200;
		}
	}

	error(M_UNKNOWN_ERROR__D, r);
}
*/


/*

	No-IP.com -- refresh

	http://www.no-ip.com/hostactive.php?host=<host>&domain=<dom>

*/
static void update_noip_refresh(void)
{
	char query[2048];
	char host[256];
	char *domain;

	strlcpy(host, get_option_required("host"), sizeof(host));
	if ((domain = strchr(host, '.')) != NULL) {
		*domain++ = 0;
	}

	sprintf(query, "/hostactive.php?host=%s", host);
	if (domain) sprintf(query + strlen(query), "&domain=%s", domain);

	wget(0, 1, "www.no-ip.com", query, NULL, 0, NULL);
	// return ignored
}



/*

	ieserver.net
	http://www.ieserver.net/tools.html

	---

	http://ieserver.net/cgi-bin/dip.cgi?username=XXX&domain=XXX&password=XXX&updatehost=1

	username = hostname
	domain = dip.jp, fam.cx, etc.

*/

static void update_ieserver(void)
{
	int r;
	char *body;
	char query[2048];
	char *p;

	// +opt +opt
	sprintf(query, "/cgi-bin/dip.cgi?username=%s&domain=%s&password=%s&updatehost=1",
		get_option_required("user"), get_option_required("host"), get_option_required("pass"));

	r = wget(0, 0, "ieserver.net", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "<title>Error") != NULL) {

			//	<p>yuuzaa na mata pasuwoodo (EUC-JP)
			if ((p = strstr(body, "<p>\xA5\xE6\xA1\xBC\xA5\xB6\xA1\xBC")) != NULL) {	// <p>user
				error(M_INVALID_AUTH);
			}

			error(M_UNKNOWN_RESPONSE__D, -1);
		}

		success();
	}

	error(M_UNKNOWN_ERROR__D, r);
}



/*

	dyns.cx
	http://www.dyns.cx/documentation/technical/protocol/v1.1.php

	---

"HTTP/1.1 200 OK
...

401 testuser not authenticated"

*/

static void update_dyns(void)
{
	int r;
	char *body;
	char query[2048];


	// +opt +opt +opt
	sprintf(query, "/postscript011.php?username=%s&password=%s&host=%s",
		get_option_required("user"), get_option_required("pass"), get_option_required("host"));

	// +opt
	append_addr_option(query, "&ip=%s");

#if 0
	sprintf(query + strlen(query), "&devel=1");
#endif

	r = wget(0, 0, "www.dyns.net", query, NULL, 0, &body);
	if (r == 200) {
		while ((*body == ' ') || (*body == '\r') || (*body == '\n')) {
			++body;
		}
		switch (r = atoi(body)) {
		case 200:
			success();
			break;
		case 400:
			error(M_INVALID_PARAM__D, r);
			break;
		case 401:
			error(M_INVALID_AUTH);
			break;
		case 402:
			error(M_TOOSOON);
			break;
		case 405:
			error(M_INVALID_HOST);
			break;
		}

		error(M_UNKNOWN_RESPONSE__D, r);
	}

	error(M_UNKNOWN_ERROR__D, r);
}



/*

	TZO
	http://www.tzo.com/

	---

"HTTP/1.1 200 OK
...

                <td valign="top"><font size="4">Congratulations!
                You've successfully signed on with TZO.<br>
                </font><h1 align="center"><font size="2"
                face="Verdana">Here's information about your
                account:</font><br>
                </h1>
                <h3 align="center"><font face="Verdana"><p>TZO Name: foobartest.tzo.com<br>
IP Address: 1.2.3.4<br>
Expiration: 2007-01-01 1:01:01</p>
</font> <br>

..."


Invalid hostname or password:
"HTTP/1.1 200 OK
...

                <td valign="top"><font size="4">Congratulations!
                You've successfully signed on with TZO.<br>
                </font><h1 align="center"><font size="2"
                face="Verdana">Here's information about your
                account:</font><br>
                </h1>
                <h3 align="center"><font face="Verdana"><p>Error=bad authentication
</p>
..."


Update too soon:
"...
                face="Verdana">Here's information about your
                account:</font><br>
                </h1>
                <h3 align="center"><font face="Verdana"><p>Error=Try again later. Please wait at least 1 minute before any additional requests.1.2.3.4
</p>
</font> <br>
..."

*/
static void update_tzo(void)
{
	int r;
	char *body;
	char query[2048];
	char *p;

	// +opt +opt +opt
	sprintf(query, "/webclient/signedon.html?TZOName=%s&Email=%s&TZOKey=%s",
		get_option_required("host"), get_option_required("user"), get_option_required("pass"));

	// +opt
	append_addr_option(query, "&IPAddress=%s");

	r = wget(0, 0, "cgi.tzo.com", query, NULL, 0, &body);
	if (r == 200) {
		if ((p = strstr(body, "Error=")) != NULL) {
			p += 6;
			if (strncmp(p, "bad auth", 8) == 0) {
				error(M_INVALID_AUTH);
			}
			if (strncmp(p, "Try again", 9) == 0) {
				error(M_TOOSOON);
			}
			error(M_UNKNOWN_ERROR__D, -1);
		}
		success();
	}

	error(M_UNKNOWN_ERROR__D, r);
}




/*

	ZoneEdit
	http://www.zoneedit.com/doc/dynamic.html

	---

"HTTP/1.1 200 OK
...
<SUCCESS CODE="200" TEXT="Update succeeded." ZONE="test123.com" HOST="www.test123.com" IP="1.2.3.4">"

"<ERROR CODE="707" TEXT="Duplicate updates for the same host/ip, adjust client settings" ZONE="testexamplesite4321.com" HOST="test.testexamplesite4321.com">"

"HTTP/1.1 401 Authorization Required
...
<title>Authentication Failed </title>"

ERROR CODE="[701-799]" TEXT="Description of the error" ZONE="Zone that Failed"
ERROR CODE="702" TEXT="Update failed." ZONE="%zone%"
ERROR CODE="703" TEXT="one of either parameters 'zones' or 'host' are required."
ERROR CODE="705" TEXT="Zone cannot be empty" ZONE="%zone%"
ERROR CODE="707" TEXT="Duplicate updates for the same host/ip, adjust client settings" ZONE="%zone%"
ERROR CODE="707" TEXT="Too frequent updates for the same host, adjust client settings" ZONE="%zone%"
ERROR CODE="704" TEXT="Zone must be a valid 'dotted' internet name." ZONE="%zone%"
ERROR CODE="701" TEXT="Zone is not set up in this account." ZONE="%zone%"
ERROR CODE="708" TEXT="Login/authorization error"
SUCCESS CODE="[200-201]" TEXT="Description of the success" ZONE="Zone that Succeeded"
SUCCESS CODE="200" TEXT="Update succeeded." ZONE="%zone%" IP="%dnsto%"
SUCCESS CODE="201" TEXT="No records need updating." ZONE="%zone%"

*/
static void update_zoneedit(int ssl)
{
	int r;
	char *body;
	char *c;
	char query[2048];

	// +opt
	sprintf(query, "/auth/dynamic.html?host=%s", get_option_required("host"));

	// +opt
	append_addr_option(query, "&dnsto=%s");

	r = wget(ssl, 0, "dynamic.zoneedit.com", query, NULL, 1, &body);
	switch (r) {
	case 200:
		if (strstr(body, "<SUCCESS CODE")) {
			success();
		}
		if ((c = strstr(body, "<ERROR CODE=\"")) != NULL) {
			r = atoi(c + 13);
			switch (r) {
			case 701:	// invalid "zone"
				error(M_INVALID_HOST);
				break;
			case 707:	// update is the same ip address? / too frequent updates
				if (strstr(c, "Duplicate") != NULL) success();
					else error(M_TOOSOON);
				break;
			case 708:	// authorization error
				error(M_INVALID_AUTH);
				break;
			}
			error(M_UNKNOWN_RESPONSE__D, r);
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
		break;
	case 401:
		error(M_INVALID_AUTH);
		break;
	}

	error(M_UNKNOWN_ERROR__D, r);
}


/*

	FreeDNS.afraid.org

	---


http://freedns.afraid.org/dynamic/update.php?XXXXXXXXXXYYYYYYYYYYYZZZZZZZ1111222

"HTTP/1.0 200 OK
...

ERROR: Address 1.2.3.4 has not changed."

"Updated 1 host(s) foobar.mooo.com to 1.2.3.4 in 1.234 seconds"

"ERROR: Missing S/key and DataID, check your update URL."

"fail, make sure you own this record, and the address does not already equal 1.2.3.4"
??

*/

static void update_afraid(void)
{
	int r;
	char *body;
	char query[2048];

	// +opt
	sprintf(query, "/dynamic/update.php?%s", get_option_required("ahash"));

	r = wget(0, 0, "freedns.afraid.org", query, NULL, 0, &body);
	if (r == 200) {
		if ((strstr(body, "ERROR") != NULL) || (strstr(body, "fail") != NULL)) {
			if (strstr(body, "has not changed") != NULL) {
				success();
			}
			error(M_INVALID_AUTH);
		}
		else if ((strstr(body, "Updated") != NULL) && (strstr(body, "host") != NULL)) {
			success();
		}
		else {
			error(M_UNKNOWN_RESPONSE__D, -1);
		}
	}

	error(M_UNKNOWN_ERROR__D, r);
}

/*

	everydns.net

HTTP/1.1 200 OK
Server: Apache
X-Powered-By: PHP/5.0.1-dev
Connection: close
Content-Type: text/html; charset=utf-8

Authentication given
Authentication failed: Bad Username/password
Exit code: 2

*/
static void update_everydns(void)
{
	int r;
	char *body;
	char query[2048];
	const char *p;

	strcpy(query, "/index.php?ver=0.1");
	append_addr_option(query, "&ip=%s");

	p = get_option("host");
	if (p) sprintf(query + strlen(query), "&domain=%s", p);

	r = wget(0, 0, "dyn.everydns.net", query, NULL, 1, &body);
	if (r == 200) {
		if ((p = strstr(body, "Exit code:")) != NULL) {
			r = atoi(p + 10);
			switch (r) {
			case 0:
				success();
				break;
			case 2:
				error(M_INVALID_AUTH);
				break;
			default:
				error(M_UNKNOWN_RESPONSE__D, r);
				break;
			}
		}
		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}


/*

	miniDNS.net
	http://www.minidns.net/areg.php?opcode=ADD&host=bar.minidns.net&username=foo&password=topsecret&ip=1.2.3.4

	---

"okay. BAR.MINIDNS.NET mapped to 1.2.3.4."
"auth_fail. Incorrect username/password/hostname."
"auth_fail. Host name format error."

*/
static void update_minidns(void)
{
	int r;
	char *body;
	char query[2048];

	// +opt +opt +opt
	sprintf(query, "/areg.php?opcode=ADD&host=%s&username=%s&password=%s",
		get_option_required("host"),
		get_option_required("user"),
		get_option_required("pass"));

	// +opt
	append_addr_option(query, "&ip=%s");

	r = wget(0, 0, "www.minidns.net", query, NULL, 1, &body);
	if (r == 200) {
		if (strstr(body, "okay.") != NULL) {
			success();
		}
		else if (strstr(body, "Host name format error") != NULL) {
			error(M_INVALID_HOST);
		}
		else if (strstr(body, "auth_fail") != NULL) {
			error(M_INVALID_AUTH);
		}
		else {
			error(M_UNKNOWN_RESPONSE__D, -1);
		}
	}

	error(M_UNKNOWN_ERROR__D, r);
}


/*

	editdns.net
	http://www.editdns.net/

	source: Keith M.

	---

	http://DynDNS.EditDNS.net/api/dynLinux.php?p=XXX&r=XXX

	p = password
	r = hostname

*/
static void update_editdns(void)
{
	int r;
	char *body;
	char query[2048];

	// +opt +opt
	sprintf(query, "/api/dynLinux.php?p=%s&r=%s",
		get_option_required("pass"), get_option_required("host"));

	r = wget(0, 1, "DynDNS.EditDNS.net", query, NULL, 0, &body);
	if (r == 200) {
		if (strstr(body, "Record has been updated") != NULL) {
			success();
		}
		if (strstr(body, "Record already exists") != NULL) {
			error(M_SAME_IP);
		}
		else if (strstr(body, "Invalid Username") != NULL) {
			error(M_INVALID_AUTH);
		}
		else if (strstr(body, "Invalid DynRecord") != NULL) {
			error(M_INVALID_HOST);
		}
		else {
			error(body);
		}
	}

	error(M_UNKNOWN_ERROR__D, r);
}



/*

	HE.net IPv6 TunnelBroker
	https://ipv4.tunnelbroker.net/ipv4_end.php?ip=$IPV4ADDR&pass=$MD5PASS&apikey=$USERID&tid=$TUNNELID

	---

"HTTP/1.1 200 OK
...

Good responses:
	+OK: Tunnel endpoint updated to: XXX.XXX.XXX.XXX
	-ERROR: This tunnel is already associated with this IP address. Please try and limit your updates to IP changes.
Bad responses:
	-ERROR: Tunnel not found
	-ERROR: Invalid API key or password
	-ERROR: IP is not in a valid format
	-ERROR: Missing parameter(s).
	-ERROR: IP is not ICMP pingable. Please make sure ICMP is not blocked. If you are blocking ICMP, please allow 66.220.2.74 through your firewall.
	-ERROR: IP is blocked. (RFC1918 Private Address Space)

*/
static void update_heipv6tb(void)
{
	int r;
	char *body, *p;
	const char *serr = "-ERROR: ";
	char query[2048];

	// +opt +opt +opt
	sprintf(query, "/ipv4_end.php?pass=%s&apikey=%s&tid=%s",
		md5_string(get_option_required("pass")),
		get_option_required("user"),
		get_option_required("host"));

	// +opt
	append_addr_option(query, "&ip=%s");

	r = wget(1, 0, "ipv4.tunnelbroker.net", query, NULL, 0, &body);
	if (r == 200) {
		if ((strstr(body, "endpoint updated to") != NULL) || (strstr(body, "is already associated") != NULL)) {
			success();
		}
		if (strstr(body, "Invalid API key or password") != NULL) {
			error(M_INVALID_AUTH);
		}
		if (strstr(body, "Tunnel not found") != NULL) {
			error(M_INVALID_PARAM__S, "Tunnel ID");
		}
		if (strstr(body, "IP is not in a valid format") != NULL) {
			error(M_INVALID_PARAM__S, "IPv4 endpoint");
		}

		error(M_UNKNOWN_RESPONSE__D, -1);
	}

	error(M_UNKNOWN_ERROR__D, r);
}



/*

	wget/custom

*/
static void update_wget(void)
{
	int r;
	char *c;
	char url[256];
	char s[256];
	char he[256];
	char *header;
	int https;
	char *host;
	char path[256];
	char *p;
	char *body;

	// http://user:pass@domain:port/path?query
	// https://user:pass@domain:port/path?query

	strcpy(url, get_option_required("url"));
	https = 0;
	host = url + 7;
	if (strncasecmp(url, "https://", 8) == 0) {
		https = 1;
		++host;
	}
	else if (strncasecmp(url, "http://", 7) != 0) {
		error(M_INVALID_PARAM__S, "url");
	}

	if ((p = strchr(host, '/')) == NULL) {
		error(M_INVALID_PARAM__S, "url");
	}
	strcpy(path, p);
	*p = 0;

	if ((c = strstr(path, "@IP")) != NULL) {
		strcpy(s, c + 3);
		strcpy(c, get_address(1));
		strcat(c, s);
	}

	if ((c = strrchr(host, '@')) != NULL) {
		*c = 0;
		s[base64_encode(host, s, c - host)] = 0;
		sprintf(he, "Authorization: Basic %s\r\n", s);
		header = he;
		host = c + 1;
	}
	else {
		header = NULL;
	}

	r = wget(https, 1, host, path, header, 0, &body);
	switch (r) {
	case 200:
	case 302:	// redirect -- assume ok
		if (body && *body) success_msg((char *)body);
		else success();
		break;
	case 401:
		error(M_INVALID_AUTH);
		break;
	}

	error(M_UNKNOWN_ERROR__D, r);
}

// -----------------------------------------------------------------------------



static void check_cookie(void)
{
	const char *c;
	char addr[16];
	long tm;

	if (((c = get_option("cookie")) == NULL) || (!read_tmaddr(c, &tm, addr))) {
		_dprintf("%s: no cookie\n", __FUNCTION__);
		refresh = 1;
		return;
	}

	if ((c = get_address(0)) == NULL) {
		_dprintf("%s: no address specified\n", __FUNCTION__);
		return;
	}
	if (strcmp(c, addr) != 0) {
		_dprintf("%s: address is different (%s != %s)\n", __FUNCTION__, c, addr);
		return;
	}

#if 0
	long now;
	long u;

	now = time(NULL);
	if ((now < Y2K) || (now < tm)) {
		_dprintf("%s: time rolled back (now=%ld, tm=%ld)\n", __FUNCTION__, now, tm);
		return;
	}
	tm = now - tm;

	_dprintf("%s: addr=%s tm=%ld (relative)\n", __FUNCTION__, addr, tm);

	if ((c = get_option("maxtime")) != NULL) {
		u = strtol(c, NULL, 0);
		if (u > 0) {
			if (tm > u) {
				_dprintf("%s: %s expired (%ld > %ld)\n", __FUNCTION__, addr, tm, u);
				return;
			}
			_dprintf("%s: maxtime=%ld tm=%ld\n", __FUNCTION__, u, tm);

			puts(M_TOOSOON);
			exit(3);
		}
	}

	if ((c = get_option("mintime")) != NULL) {
		u = strtol(c, NULL, 0);
		if ((u > 0) && (tm < u)) {
			_dprintf("%s: %s recently updated (%ld < %ld)\n", __FUNCTION__, addr, tm, u);

			puts(M_TOOSOON);
			exit(3);
		}
	}

#endif

	puts(M_SAME_IP);
	exit(3);
}

static void save_cookie(void)
{
	const char *cookie;
	const char *c;
	long now;
	char s[256];

	now = time(NULL);
	if (now < Y2K) {
		_dprintf("%s: no time", __FUNCTION__);
		return;
	}

	if ((cookie = get_option("cookie")) == NULL) {
		_dprintf("%s: no cookie\n", __FUNCTION__);
		return;
	}

	if ((c = get_address(0)) == NULL) {
		_dprintf("%s: no address specified\n", __FUNCTION__);
		return;
	}

	sprintf(s, "%ld,%s", now, c);
	f_write_string(cookie, s, FW_NEWLINE, 0);

	_dprintf("%s: cookie=%s\n", __FUNCTION__, s);
}

int main(int argc, char *argv[])
{
	const char *p;

	g_argc = argc;
	g_argv = argv;

	printf("MDU\nCopyright (C) 2007-2009 Jonathan Zarate\n\n");
	_dprintf("DEBUG\n");

	if ((blob = malloc(BLOB_SIZE)) == NULL) {
		return 1;
	}

	mkdir("/var/lib/mdu", 0700);
	chdir("/var/lib/mdu");
/*
	if ((p = get_dump_name()) != NULL) {
		unlink(p);
	}
*/
	check_cookie();

	p = get_option_required("service");
	if (strcmp(p, "dua") == 0) {
		update_dua("dyndns", 0, NULL, NULL, 1);
	}
	else if (strcmp(p, "duas") == 0) {
		update_dua("dyndns", 1, NULL, NULL, 1);
	}
	else if (strcmp(p, "dyndns") == 0) {
		//	test ok 9/14 -- zzz
		update_dua("dyndns", 0, "members.dyndns.org", "/nic/update", 1);
	}
	else if (strcmp(p, "dyndns-static") == 0) {
		// test ok 9/14 -- zzz
		update_dua("statdns", 0, "members.dyndns.org", "/nic/update", 1);
	}
	else if (strcmp(p, "dyndns-custom") == 0) {
		// test ok 9/14 -- zzz
		update_dua("custom", 0, "members.dyndns.org", "/nic/update", 1);
	}
	else if (strcmp(p, "sdyndns") == 0) {
		update_dua("dyndns", 1, "members.dyndns.org", "/nic/update", 1);
	}
	else if (strcmp(p, "sdyndns-static") == 0) {
		update_dua("statdns", 1, "members.dyndns.org", "/nic/update", 1);
	}
	else if (strcmp(p, "sdyndns-custom") == 0) {
		update_dua("custom", 1, "members.dyndns.org", "/nic/update", 1);
	}
	else if (strcmp(p, "easydns") == 0) {
		// no account, test output ok, test 401 error parse ok 9/15 -- zzz
		update_dua(NULL, 0, "members.easydns.com", "/dyn/dyndns.php", 1);
	}
	else if (strcmp(p, "seasydns") == 0) {
		update_dua(NULL, 1, "members.easydns.com", "/dyn/dyndns.php", 1);
	}
	else if (strcmp(p, "3322") == 0) {
		// no account, test output ok, test 401 error parse ok 9/16 -- zzz
		update_dua(NULL, 0, "members.3322.org", "/dyndns/update", 1);
	}
	else if (strcmp(p, "3322-static") == 0) {
		// no account, test output ok, test 401 error parse ok 9/16 -- zzz
		update_dua("statdns", 0, "members.3322.org", "/dyndns/update", 1);
	}
	else if (strcmp(p, "opendns") == 0) {
		// test ok 9/15 -- zzz
		update_dua(NULL, 1, "updates.opendns.com", "/nic/update", 0);
	}
	else if (strcmp(p, "dnsomatic") == 0) {
		// test ok 12/02 -- zzz
		update_dua(NULL, 1, "updates.dnsomatic.com", "/nic/update", 0);
	}
	else if (strcmp(p, "noip") == 0) {
		update_dua(NULL, 0, "dynupdate.no-ip.com", "/nic/update", 1);
//		update_noip();
	}
	else if (strcmp(p, "namecheap") == 0) {
		// test ok 9/14 -- zzz
		update_namecheap();
	}
	else if (strcmp(p, "enom") == 0) {
		// no account, test output ok, 12/03 -- zzz
		update_enom();
	}
	else if (strcmp(p, "dnsexit") == 0) {
		// test ok 9/14 -- zzz
		update_dnsexit();
	}
	else if (strcmp(p, "ieserver") == 0) {
		// test ok 9/14 -- zzz
		update_ieserver();
	}
	else if (strcmp(p, "dyns") == 0) {
		// no account, test output ok, test 401 error parse ok 9/15 -- zzz
		update_dyns();
	}
	else if (strcmp(p, "tzo") == 0) {
		// test ok 9/15 -- zzz
		update_tzo();
	}
	else if (strcmp(p, "zoneedit") == 0) {
		// test ok 9/16 -- zzz
		update_zoneedit(0);
	}
	else if (strcmp(p, "szoneedit") == 0) {
		update_zoneedit(1);
	}
	else if (strcmp(p, "afraid") == 0) {
		// test ok 9/16 -- zzz
		update_afraid();
	}
	else if (strcmp(p, "everydns") == 0) {
		// 07/2008 -- zzz
		update_everydns();
	}
	else if (strcmp(p, "editdns") == 0) {
		update_editdns();
	}
	else if (strcmp(p, "minidns") == 0) {
		update_minidns();
	}
	else if (strcmp(p, "heipv6tb") == 0) {
		update_heipv6tb();
	}
	else if (strcmp(p, "pairnic") == 0) {
		// pairNIC uses the same API as DynDNS
		update_dua("pairnic", 0, "dynamic.pairnic.com", "/nic/update", 1);
	}
	else if (strcmp(p, "spairnic") == 0) {
		// pairNIC uses the same API as DynDNS
		update_dua("pairnic", 1, "dynamic.pairnic.com", "/nic/update", 1);
	}
	else if (strcmp(p, "ovh") == 0) {
		// OVH uses the same API as DynDNS
		update_dua("dyndns", 0, "www.ovh.com", "/nic/update", 1);
	}
	else if (strcmp(p, "sovh") == 0) {
		// OVH uses the same API as DynDNS
		update_dua("dyndns", 1, "www.ovh.com", "/nic/update", 1);
	}
	else if (strcmp(p, "schangeip") == 0) {
		// ChangeIP uses the same API as DynDNS
		update_dua("dyndns", 1, "nic.changeip.com", "/nic/update", 1);
	}
	else if ((strcmp(p, "wget") == 0) || (strcmp(p, "custom") == 0)) {
		// test ok 9/15 -- zzz
		update_wget();
	}
	else {
		error("Unknown service");
	}

//	free(blob);
	return 1;
}
