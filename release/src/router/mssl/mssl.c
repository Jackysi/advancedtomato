/*

	Minimal MatrixSSL Helper
	Copyright (C) 2006-2009 Jonathan Zarate

	Licensed under GNU GPL v2 or later.

*/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>

#include <shutils.h>

#include "../matrixssl/matrixSsl.h"
/*
#include "mssl.h"
*/


#define _dprintf(args...)	do { } while(0)
//	#define _dprintf	cprintf


typedef struct {
	ssl_t *ssl;
	sslBuf_t inbuf;		// buffer for decoded data
	sslBuf_t insock;	// buffer for recv() data
	sslBuf_t outsock;	// buffer for send() data
	int sslend;
	int sd;
} mssl_cookie_t;

static sslKeys_t *keys;



inline int sb_used(sslBuf_t *b)
{
	return b->end - b->start;
}

inline int sb_unused(sslBuf_t *b)
{
	return (b->buf + b->size) - b->end;
}

static void sb_free(sslBuf_t *b)
{
	free(b->buf);
	b->start = b->end = b->buf = NULL;
	b->size = 0;
}

//  - expects ->buf to be valid or NULL
//  - malloc error is fatal
static int sb_alloc(sslBuf_t *b, int size)
{
	void *p;

	sb_free(b);
	if ((p = malloc(size)) == NULL) {
		syslog(LOG_CRIT, "Not enough memory");
		exit(1);
	}
	b->start = b->end = b->buf = p;
	b->size = size;
	return 1;
}

//  - expects ->buf to be valid or NULL
//  - malloc error is fatal
static int sb_realloc(sslBuf_t *b, int size)
{
	void *p;

	if ((p = realloc(b->buf, size)) == NULL) {
		syslog(LOG_CRIT, "Not enough memory");
		exit(1);
	}
	b->start = p + (b->start - b->buf);
	b->end = p + (b->end - b->buf);
	b->buf = p;
	b->size = size;
	return 1;
}

static void sb_pack(sslBuf_t *b)
{
	int n;

	if (b->start == b->end) {
		b->start = b->end = b->buf;
	}
	else {
		n = sb_used(b);
		memmove(b->buf, b->start, n);
		b->end = b->buf + n;
		b->start = b->buf;
	}
}

static int sb_read(sslBuf_t *b, unsigned char *buf, int len)
{
	int n;
	n = min(sb_used(b), len);
	memcpy(buf, b->start, n);
	b->start += n;
	if (b->start == b->end) b->start = b->end = b->buf;
	return n;
}


// -----------------------------------------------------------------------------


static ssize_t mssl_read(void *cookie, char *buf, size_t len)
{
	mssl_cookie_t *kuki = cookie;
	int r;
	unsigned char err, alevel, adesc;


	_dprintf("%s\n", __FUNCTION__);

	if (kuki->inbuf.buf) {
		if (kuki->inbuf.start < kuki->inbuf.end) {
			r = sb_read(&kuki->inbuf, buf, len);
			_dprintf("sb_read r=%d\n", r);
			return r;
		}
		sb_free(&kuki->inbuf);
	}

	sb_pack(&kuki->insock);

	if (kuki->insock.start == kuki->insock.end) {
READ:
	_dprintf("READ\n");

		while ((r = recv(kuki->sd, kuki->insock.end, sb_unused(&kuki->insock), 0)) == -1) {
			if (errno != EINTR) {
				_dprintf("recv failed errno=%d\n", errno);
				return -1;
			}
		}
		if (r == 0) {
			kuki->sslend = 1;
			return 0;
		}
		kuki->insock.end += r;
	}

	sb_alloc(&kuki->inbuf, len);

DECODE:
	_dprintf("DECODE\n");

	err = 0;
	alevel = 0;
	adesc = 0;

	switch (matrixSslDecode(kuki->ssl, &kuki->insock, &kuki->inbuf, &err, &alevel, &adesc)) {
	case SSL_SUCCESS:
		_dprintf("SSL_SUCCESS\n");
		return 0;
	case SSL_PROCESS_DATA:
		_dprintf("SSL_PROCESS_DATA\n");

		r = sb_used(&kuki->inbuf);
		_dprintf(" r = %d len = %d\n", r, len);
		r = min(r, len);
		memcpy(buf, kuki->inbuf.start, r);
		kuki->inbuf.start += r;
		return r;
	case SSL_SEND_RESPONSE:
		_dprintf("SSL_SEND_RESPONSE\n");
		_dprintf("send %d\n", sb_used(&kuki->inbuf));

		while ((r = send(kuki->sd, kuki->inbuf.start, sb_used(&kuki->inbuf), MSG_NOSIGNAL)) == -1) {
			if (errno != EINTR) {
				_dprintf("send error\n");
				return -1;
			}
		}
		kuki->inbuf.start += r;
		if (kuki->inbuf.start != kuki->inbuf.end) _dprintf("inbuf.start != inbuf.end\n");
		kuki->inbuf.start = kuki->inbuf.end = kuki->inbuf.buf;
		return 0;
	case SSL_ERROR:
		_dprintf("ssl error %d\n", err);

		if (kuki->inbuf.start < kuki->inbuf.end) {
			send(kuki->sd, kuki->inbuf.start, sb_used(&kuki->inbuf), MSG_NOSIGNAL);
		}
		errno = EIO;
		return -1;
	case SSL_ALERT:
		_dprintf("SSL_ALERT\n");

		if (adesc == SSL_ALERT_CLOSE_NOTIFY) {
			kuki->sslend = 1;
			return 0;
		}

		_dprintf("ssl closing on alert level=%d desc=%d\n", alevel, adesc);
		errno = EIO;
		return -1;
	case SSL_PARTIAL:
		_dprintf("SSL_PARTIAL insock.size=%d %d\n", kuki->insock.size, SSL_MAX_BUF_SIZE);

		if ((kuki->insock.start == kuki->insock.buf) && (kuki->insock.end == (kuki->insock.buf + kuki->insock.size))) {
			if (kuki->insock.size > SSL_MAX_BUF_SIZE) return -1;
			sb_realloc(&kuki->insock, kuki->insock.size * 2);
		}

		if (kuki->inbuf.start != kuki->inbuf.end) {
			_dprintf("!! inbuf.start != inbuf.end\n");
		}

		sb_free(&kuki->inbuf);
		goto READ;
	case SSL_FULL:
		_dprintf("SSL_FULL\n");

		sb_alloc(&kuki->inbuf, kuki->inbuf.size * 2);
		goto DECODE;
	}

	return 0;
}

static ssize_t mssl_write(void *cookie, const char *buf, size_t len)
{
	mssl_cookie_t *kuki = cookie;
	int r;
	int nw;

	_dprintf("%s\n", __FUNCTION__);

	nw = 0;
	sb_pack(&kuki->outsock);
	if (buf == NULL) goto PUMP;

RETRY:
	switch (matrixSslEncode(kuki->ssl, (unsigned char *)buf, len, &kuki->outsock)) {
	case SSL_ERROR:
		errno = EIO;
		_dprintf("SSL_ERROR\n");
		return -1;
	case SSL_FULL:
		if (kuki->outsock.size > SSL_MAX_BUF_SIZE) {
			errno = EFBIG;
			_dprintf("outsock.size > max\n");
			return -1;
		}
		sb_realloc(&kuki->outsock, kuki->outsock.size * 2);
		goto RETRY;
	}

PUMP:
	while ((r = send(kuki->sd, kuki->outsock.start, sb_used(&kuki->outsock), MSG_NOSIGNAL)) == -1) {
		if (errno != EINTR) {
			_dprintf("send error %d\n", errno);
			return -1;
		}
	}
	kuki->outsock.start += r;
	nw += r;

	if (kuki->outsock.start < kuki->outsock.end) {
		_dprintf("start < end\n");
		goto PUMP;
	}

	return nw;
}

static int mssl_seek(void *cookie, __offmax_t *pos, int whence)
{
	_dprintf("%s()\n", __FUNCTION__);
	errno = EIO;
	return -1;
}

static int mssl_close(void *cookie)
{
	mssl_cookie_t *kuki = cookie;

	_dprintf("%s()\n", __FUNCTION__);

	if (!kuki) return 0;

	if (kuki->ssl) {
		if (kuki->outsock.buf) {
			mssl_write(kuki, NULL, 0);

			kuki->outsock.start = kuki->outsock.end = kuki->outsock.buf;
			matrixSslEncodeClosureAlert(kuki->ssl, &kuki->outsock);
			fcntl(kuki->sd, F_SETFL, fcntl(kuki->sd, F_GETFL) | O_NONBLOCK);
			send(kuki->sd, kuki->outsock.start, sb_used(&kuki->outsock), MSG_NOSIGNAL);
		}

		matrixSslDeleteSession(kuki->ssl);
		kuki->ssl = NULL;
	}
	sb_free(&kuki->inbuf);
	sb_free(&kuki->insock);
	sb_free(&kuki->outsock);

	free(kuki);
	return 0;
}

static int cert_valid(sslCertInfo_t *cert, void *arg)
{
	// note: no validation!
	return SSL_ALLOW_ANON_CONNECTION;
}

static const cookie_io_functions_t mssl = {
	mssl_read, mssl_write, mssl_seek, mssl_close
};

static FILE *_ssl_fopen(int sd, int client)
{
	unsigned char buf[1024];
	int r;
	int n;
	mssl_cookie_t *kuki;
	FILE *f;

	_dprintf("%s()\n", __FUNCTION__);

	if ((kuki = calloc(1, sizeof(*kuki))) == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	kuki->sd = sd;

	if (matrixSslNewSession(&kuki->ssl, keys, NULL, client ? 0 : SSL_FLAGS_SERVER) < 0) {
		_dprintf("%s: matrixSslNewSession failed\n", __FUNCTION__);
		goto ERROR;
	}

	sb_alloc(&kuki->insock, 1024);
	sb_alloc(&kuki->outsock, 2048);

	if (client) {
		matrixSslSetCertValidator(kuki->ssl, cert_valid, NULL);

		n = matrixSslEncodeClientHello(kuki->ssl, &kuki->outsock, 0);
		if (n < 0) {
			_dprintf("%s: matrixSslEncodeClientHello failed\n", __FUNCTION__);
			goto ERROR;
		}
		if (mssl_write(kuki, NULL, 0) <= 0) {
			_dprintf("%s: error while writing HELLO\n", __FUNCTION__);
			goto ERROR;
		}
	}

MORE:
	r = mssl_read(kuki, buf, sizeof(buf));
	if (r == 0) {
		if (kuki->sslend) {
			_dprintf("%s: end reached\n", __FUNCTION__);
			errno = EIO;
			goto ERROR;
		}
		if (matrixSslHandshakeIsComplete(kuki->ssl) == 0) {
			_dprintf("%s: =0 goto more\n", __FUNCTION__);
			goto MORE;
		}
		if ((f = fopencookie(kuki, "r+", mssl)) == NULL) {
			_dprintf("%s: fopencookie failed\n", __FUNCTION__);
			goto ERROR;
		}
		return f;
	}

	_dprintf("%s: read error r=%d errno=%d\n", __FUNCTION__, r, errno);
	errno = EIO;

ERROR:
	mssl_close(kuki);
	return NULL;
}

FILE *ssl_server_fopen(int sd)
{
	_dprintf("%s()\n", __FUNCTION__);
	return _ssl_fopen(sd, 0);
}

FILE *ssl_client_fopen(int sd)
{
	_dprintf("%s()\n", __FUNCTION__);
	return _ssl_fopen(sd, 1);
}

int ssl_init(char *cert, char *priv)
{
	if (matrixSslOpen() < 0) {
		_dprintf("matrixSslOpen failed");
		return 0;
	}
	if (matrixSslReadKeys(&keys, cert, priv, NULL, NULL) < 0)  {
		matrixSslClose();
		_dprintf("matrixSslReadKeys failed");
		return 0;
	}
	return 1;
}
