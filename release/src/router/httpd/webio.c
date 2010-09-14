/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/
#include "tomato.h"

#include "../mssl/mssl.h"
extern int do_ssl;

#include <errno.h>
#include <stdarg.h>

extern FILE *connfp;
extern int connfd;

int web_getline(char *buffer, int max)
{
	while (fgets(buffer, max, connfp) == NULL) {
		if (errno != EINTR) return 0;
	}
//	cprintf("%s", buffer);
	return 1;
}

void web_puts(const char *buffer)
{
	web_write(buffer, strlen(buffer));
}

void web_putj(const char *buffer)
{
	char *p;

	p = js_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

void web_putj_utf8(const char *buffer)
{
	char *p;

	p = utf8_to_js_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

void web_puth(const char *buffer)
{
	char *p;

	p = html_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

void web_puth_utf8(const char *buffer)
{
	char *p;

	p = utf8_to_html_string(buffer);
	if (p) {
		web_puts(p);
		free(p);
	}
}

int _web_printf(wofilter_t wof, const char *format, ...)
{
	va_list args;
	char *b, *p;
	int size;
	int n;

	size = 1024;
	while (1) {
		if ((b = malloc(size)) == NULL) return 0;

		va_start(args, format);
		n = vsnprintf(b, size, format, args);
		va_end(args);

		if (n > -1) {
			if (n < size) {
				switch (wof) {
				case WOF_JAVASCRIPT:
					p = js_string(b);
					free(b);
					break;
				case WOF_HTML:
					p = html_string(b);
					free(b);
					break;
				default:
					p = b;
					break;
				}
				if (!p) return 0;
				web_puts(p);
				free(p);
				return 1;
			}
			size = n + 1;
		}
		else size *= 2;

		free(b);
		if (size > (10 * 1024)) return 0;
	}
}

int web_write(const char *buffer, int len)
{
	int n = len;
	int r = 0;

	while (n > 0) {
		r = fwrite(buffer, 1, n, connfp);
		if ((r == 0) && (errno != EINTR)) return -1;
		buffer += r;
		n -= r;
	}
	return r;
}

int web_read(void *buffer, int len)
{
	int r;
	if (len <= 0) return 0;
	while ((r = fread(buffer, 1, len, connfp)) == 0) {
		if (errno != EINTR) return -1;
	}
	return r;
}

int web_read_x(void *buffer, int len)
{
	int n;
	int t = 0;
	while (len > 0) {
		n = web_read(buffer, len);
		if (n <= 0) return len;
		buffer += n;
		len -= n;
		t += n;
	}
	return t;
}

int web_eat(int max)
{
	char buf[512];
	int n;
	while (max > 0) {
		 if ((n = web_read(buf, (max < sizeof(buf)) ? max : sizeof(buf))) <= 0) return 0;
		 max -= n;
	}
	return 1;
}

int web_flush(void)
{
	return (fflush(connfp) == 0);
}

int web_open(void)
{
	if (do_ssl) {
#ifdef TCONFIG_HTTPS
		if ((connfp = ssl_server_fopen(connfd)) != NULL) return 1;
#endif
	}
	else {
		if ((connfp = fdopen(connfd, "r+")) != NULL) return 1;
	}
	return 0;
}

int web_close(void)
{
	if (connfp != NULL) {
		fflush(connfp);
		fclose(connfp);
		connfp = NULL;
	}
	if (connfd != -1) {
//		shutdown(connfd, SHUT_RDWR);
		close(connfd);
		connfd = -1;
	}
	return 1;
}


// --------------------------------------------------------------------------------------------------------------------


static void _web_putfile(FILE *f, wofilter_t wof)
{
	char buf[2048];
	int nr;

	while ((nr = fread(buf, 1, sizeof(buf) - 1, f)) > 0) {
		buf[nr] = 0;
		switch (wof) {
		case WOF_JAVASCRIPT:
			web_putj_utf8(buf);
			break;
		case WOF_HTML:
			web_puth_utf8(buf);
			break;
		default:
			web_puts(buf);
			break;
		}
	}
}

int web_putfile(const char *fname, wofilter_t wof)
{
	FILE *f;

	if ((f = fopen(fname, "r")) != NULL) {
		_web_putfile(f, wof);
		fclose(f);
		return 1;
	}
	return 0;
}

int web_pipecmd(const char *cmd, wofilter_t wof)
{
	FILE *f;

	if ((f = popen(cmd, "r")) != NULL) {
		_web_putfile(f, wof);
		pclose(f);
		return 1;
	}
	return 0;
}


