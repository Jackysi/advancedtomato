/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include "tomato.h"

#include <time.h>


#ifdef BLACKHOLE
void wi_blackhole(char *url, int len, char *boundary)
{
	char buf[2048];
	int size;
	int n;
	time_t tick;
	int foo;
	const char *error;
	int blen;
	FILE *f;

	if (!post) {
		send_header(200, NULL, mime_html, 0);
		web_printf(
			"<h1>Upload Test</h1>"
			"<form method='post' action='blackhole.cgi?_http_id=%s' encType='multipart/form-data'>"
			"<input type='file' name='file'><input type='submit'>"
			"</form>",
			nvram_safe_get("http_id"));
		return;
	}

	check_id();

	cprintf("\nblackhole\n");
	cprintf("%s<\n", boundary);

	if ((blen = strlen(boundary)) == 0) {
		error = "no boundary";
ERROR:
		cprintf("ERROR: %s\n", error);

		web_eat(len);

		send_header(200, NULL, mime_html, 0);
		web_printf("ERROR: %s", error);
		return;
	}

	if (blen > (sizeof(buf) - 32)) {
		error = "boundary is too big";
		goto ERROR;
	}

	// --b\r\n
	// <data>\r\n
	// --b--\r\n
	if (len < ((blen * 2) + 12)) {
		error = "not enough data";
		goto ERROR;
	}

	foo = 1;
	while (len > 0) {
		if (!web_getline(buf, MIN(len, sizeof(buf)))) {
			break;
		}
		n = strlen(buf);
		len -= n;

		if (n < 2) {
			error = "n < 2";
			goto ERROR;
		}
		if (buf[--n] != '\n') {
			error = "\\n not found";
			goto ERROR;
		}
		if (buf[--n] != '\r') {
			error = "\\n without \\r";
			goto ERROR;
		}
		if (n == 0) break;
		buf[n] = 0;

		cprintf("%s<\n", buf);

		if (foo) {
			if ((buf[0] != '-') || (buf[1] != '-') || (strcmp(buf + 2, boundary) != 0)) {
				error = "boundary not found on first line";
				goto ERROR;
			}
			foo = 0;
		}
	}

	if (foo != 0) {
		error = "boundary not found before reaching end of header";
		goto ERROR;
	}

	blen += 6;
	len -= (blen + 2);
	if (len < 0) {
		error = "not enough data for end boundary";
		goto ERROR;
	}

	unlink("/tmp/blackhole");
/*	if (len < (1*1024*1024)) f = fopen("/tmp/blackhole", "w");
		else*/ f = NULL;

#if 0
	// read half, trigger tcp reset
	len >>= 1;
#endif

	size = len;
	tick = time(0);
	while (len > 0) {
		 if ((n = web_read(buf, MIN(len, sizeof(buf)))) <= 0) {
			 break;
		 }
		 if (f) fwrite(buf, n, 1, f);
		 len -= n;
	}
	tick = time(0) - tick;
	if (f) fclose(f);

	if (len > 0) {
		error = "not all data was read";
		goto ERROR;
	}

	if (web_read(buf, blen) != blen) {
		error = "error while reading end boundary";
		goto ERROR;
	}
	buf[blen] = 0;
	cprintf(">>%s<<\n", buf);
	if ((strncmp(buf, "\r\n--", 4) != 0) ||
		(buf[blen - 1] != '-') || (buf[blen - 2] != '-')
		|| (strncmp(buf + 4, boundary, blen - 6) != 0)) {
		error = "end boundary not found";
		goto ERROR;
	}

	len += 2;
	if (len > 0) {
		 if ((n = web_read(buf, MIN(len, sizeof(buf) - 1))) > 0) {
			len -= n;
			buf[n] = 0;
			cprintf("last >>%s<<\n", buf);
		 }
	}

	web_eat(len);
	cprintf("len=%d\n", len);

	if (tick > 0) n = size / tick;
		else n = 0;

	send_header(200, NULL, mime_html, 0);
	web_printf(
		"<pre>"
		"Size .......: %d bytes\n"
		"Time .......: %d seconds\n"
		"Speed ......: %.2f kb/s | %.2f mb/s | %.2f KB/s | %.2f MB/s\n",
		size,
		tick,
		n / 128.0, n / 131072.0,
		n / 1024.0, n / 1048576.0);
	if (f) {
		web_pipecmd("md5sum /tmp/blackhole", WOF_NONE);
	}
	web_puts("</pre>");

	cprintf("done...\n");
}
#endif
