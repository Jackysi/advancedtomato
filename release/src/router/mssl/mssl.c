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
	int outbufcnt;
	int sd;
} mssl_cookie_t;

static sslKeys_t *keys;



static inline int sb_used(sslBuf_t *b)
{
	return b->end - b->start;
}

static inline int sb_unused(sslBuf_t *b)
{
	return (b->buf + b->size) - b->end;
}

static inline void sb_free(sslBuf_t *b)
{
	free(b->buf);
	b->start = b->end = b->buf = NULL;
	b->size = 0;
}

//  - expects ->buf to be valid or NULL
//  - malloc error is fatal
static inline int sb_alloc(sslBuf_t *b, int size)
{
	_dprintf("%s()\n", __FUNCTION__);
	void *p = NULL;

	//sb_free(b);
	if (size > 0) {
		if ((p = malloc(size)) == NULL) {
			syslog(LOG_CRIT, "Not enough memory");
			exit(1);
		}
	}
	b->start = b->end = b->buf = p;
	b->size = size;
	return 1;
}

//  - expects ->buf to be valid or NULL
//  - malloc error is fatal
static inline int sb_realloc(sslBuf_t *b, int size)
{
	_dprintf("%s()\n", __FUNCTION__);
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

static inline void sb_pack(sslBuf_t *b)
{
	_dprintf("%s()\n", __FUNCTION__);
	if (b->buf < b->start) {
		if (b->start == b->end) {
			b->start = b->end = b->buf;
		}
		else {
			memmove(b->buf, b->start, sb_used(b));
			b->end -= b->start - b->buf;
			b->start = b->buf;
		}
	}
}

static inline int sb_read(sslBuf_t *b, unsigned char *buf, int len)
{
	_dprintf("%s()\n", __FUNCTION__);
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
	kuki->sslend = 0;
	if (kuki->ssl == NULL || len <= 0) {
		return -1;
	}

	/*
	If inbuf is valid, then we have previously decoded data that must be
	returned, return as much as possible.  Once all buffered data is
	returned, free the inbuf.
	*/
	if (kuki->inbuf.buf) {
		if (kuki->inbuf.start < kuki->inbuf.end) {
			r = sb_read(&(kuki->inbuf), buf, len);
			_dprintf("sb_read r=%d\n", r);
			return r;
		}
		sb_free(&(kuki->inbuf));
	}

	/* Pack the buffered socket data (if any) so that start is at zero. */
	sb_pack(&(kuki->insock));

	/*
	Read up to as many bytes as there are remaining in the buffer.  We could
	Have encrypted data already cached in conn->insock, but might as well read more
	if we can.
	*/
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

	/* Define a temporary sslBuf */
	if (kuki->inbuf.buf == (unsigned char*)buf) kuki->inbuf.buf = NULL;
	sb_alloc(&(kuki->inbuf), len);

DECODE:
	_dprintf("DECODE\n");

	/* Decode the data we just read from the socket */
	err = 0;
	alevel = 0;
	adesc = 0;

	r = matrixSslDecode(kuki->ssl, &kuki->insock, &kuki->inbuf, &err, &alevel, &adesc);
	switch (r) {
	case SSL_SUCCESS:
		/* Successfully decoded an application data record, and placed in tmp buf */
		_dprintf("SSL_SUCCESS\n");
		return 0;
	case SSL_PROCESS_DATA:
		/*
		Copy as much as we can from the temp buffer into the caller's buffer
		and leave the remainder in conn->inbuf until the next call to read
		It is possible that len > data in buffer if the encoded record
		was longer than len, but the decoded record isn't!
		*/
		_dprintf("SSL_PROCESS_DATA\n");

		r = sb_used(&kuki->inbuf);
		_dprintf(" r = %d len = %d\n", r, len);
		r = min(r, len);
		memcpy(buf, kuki->inbuf.start, r);
		kuki->inbuf.start += r;
		return r;
	case SSL_SEND_RESPONSE:
		/*
		We've decoded a record that requires a response into tmp
		If there is no data to be flushed in the out buffer, we can write out
		the contents of the tmp buffer.  Otherwise, we need to append the data 
		to the outgoing data buffer and flush it out.
		*/
		_dprintf("SSL_SEND_RESPONSE\n");
		_dprintf("send %d\n", sb_used(&kuki->inbuf));

		while ((r = send(kuki->sd, (char *)kuki->inbuf.start, sb_used(&kuki->inbuf), MSG_NOSIGNAL)) == -1) {
			if (errno != EINTR) {
				_dprintf("send error\n");
				goto readError;
			}
		}
		kuki->inbuf.start += r;
		if (kuki->inbuf.start != kuki->inbuf.end) {
			_dprintf("inbuf.start != inbuf.end\n");
		}
		kuki->inbuf.start = kuki->inbuf.end = kuki->inbuf.buf;
		return 0;
	case SSL_ERROR:
		/*
		There was an error decoding the data, or encoding the out buffer.
		There may be a response data in the out buffer, so try to send.
		We try a single hail-mary send of the data, and then close the socket.
		Since we're closing on error, we don't worry too much about a clean flush.
		*/
		_dprintf("ssl error %d\n", err);

		if (kuki->inbuf.start < kuki->inbuf.end) {
			fcntl(kuki->sd, F_SETFL, fcntl(kuki->sd, F_GETFL) | O_NONBLOCK);
			send(kuki->sd, kuki->inbuf.start, sb_used(&kuki->inbuf), MSG_NOSIGNAL);
		}
		errno = EIO;
		goto readError;
	case SSL_ALERT:
		/*
		We've decoded an alert.  The level and description passed into
		matrixSslDecode are filled in with the specifics.
		*/
		_dprintf("SSL_ALERT\n");

		if (adesc == SSL_ALERT_CLOSE_NOTIFY) {
			kuki->sslend = 1;
			goto readZero;
		}

		_dprintf("ssl closing on alert level=%d desc=%d\n", alevel, adesc);
		errno = EIO;
		goto readError;
	case SSL_PARTIAL:
		/*
		We have a partial record, we need to read more data off the socket.
		If we have a completely full conn->insock buffer, we'll need to grow it
		here so that we CAN read more data when called the next time.
		*/
		_dprintf("SSL_PARTIAL insock.size=%d %d\n", kuki->insock.size, SSL_MAX_BUF_SIZE);

		if ((kuki->insock.start == kuki->insock.buf) && (kuki->insock.end == (kuki->insock.buf + kuki->insock.size))) {
			if (kuki->insock.size > SSL_MAX_BUF_SIZE)
				goto readError;
			sb_realloc(&kuki->insock, kuki->insock.size * 2);
		}

		if (kuki->inbuf.start != kuki->inbuf.end) {
			_dprintf("!! inbuf.start != inbuf.end\n");
		}

		sb_free(&kuki->inbuf);
		goto READ;
	case SSL_FULL:
		/*
		The out buffer is too small to fit the decoded or response
		data.  Increase the size of the buffer and call decode again
		*/
		_dprintf("SSL_FULL\n");

		if (kuki->inbuf.buf == (unsigned char*)buf) kuki->inbuf.buf = NULL;
		sb_alloc(&(kuki->inbuf), kuki->inbuf.size * 2);
		goto DECODE;
	}

	/*
	We consolidated some of the returns here because we must ensure
	that conn->inbuf is cleared if pointing at caller's buffer, otherwise
	it will be freed later on.
	*/
readZero:
	if (kuki->inbuf.buf == (unsigned char*)buf) {
		kuki->inbuf.buf = NULL;
	}
	return 0;
readError:
	if (kuki->inbuf.buf == (unsigned char*)buf) {
		kuki->inbuf.buf = NULL;
	}
	return -1;
}

static int _socket_write(int sd, sslBuf_t *out)
{
	_dprintf("%s\n", __FUNCTION__);

	unsigned char *s;
	int r;

	s = out->start;
	while (out->start < out->end) {
		while ((r = send(sd, out->start, sb_used(out), MSG_NOSIGNAL)) == -1) {
			if (errno != EINTR) {
				_dprintf("send error %d\n", errno);
				return -1;
			}
		}
		out->start += r;
	}
	return (int)(out->start - s);
}

static ssize_t _mssl_write(void *cookie, const char *buf, size_t len)
{
	mssl_cookie_t *kuki = cookie;
	int r;

	_dprintf("%s(len=%d), outbufcnt=%d\n", __FUNCTION__, len);

	kuki->sslend = 0;
	/* Pack the buffered socket data (if any) so that start is at zero. */
	sb_pack(&kuki->outsock);

	/*
	If there is buffered output data, the caller must be trying to
	send the same amount of data as last time.  We don't support 
	sending additional data until the original buffered request has
	been completely sent.
	*/
	if (kuki->outbufcnt > 0 && len != kuki->outbufcnt)
		return -1;

	/* Encode the caller's data */
	if (kuki->outbufcnt == 0) {
RETRY:
		r = matrixSslEncode(kuki->ssl, (unsigned char *)buf, len, &kuki->outsock);
		_dprintf("%s: encode r=%d\n", __FUNCTION__, r);
		switch (r) {
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
	}

	/* We've got data to send. */
	_dprintf("%s: sending %d bytes\n", __FUNCTION__, sb_used(&kuki->outsock));
	while ((r = send(kuki->sd, kuki->outsock.start, sb_used(&kuki->outsock), MSG_NOSIGNAL)) == -1) {
		if (errno != EINTR) {
			_dprintf("send error %d\n", errno);
			return -1;
		}
	}
	kuki->outsock.start += r;

	/*
	If we wrote it all return the length, otherwise remember the number of
	bytes passed in, and return 0 to be called again later.
	*/
	_dprintf("exiting %s(): outbufcnt=%d, len=%d\n", __FUNCTION__, kuki->outbufcnt, len);
	if (kuki->outsock.start == kuki->outsock.end) {
		kuki->outbufcnt = 0;
		return len;
	}
	kuki->outbufcnt = len;
	return 0;
}

static ssize_t mssl_write(void *cookie, const char *buf, size_t len)
{
	_dprintf("%s\n", __FUNCTION__);

	int r = 0;
	int sum = 0;
	int offset = 0;
	int maxwrite = SSL_MAX_BUF_SIZE / 2;

	if (len > maxwrite) {
		while (offset < len && r >= 0) {
			r = 0;
			while (r == 0) {
				r = _mssl_write(cookie, (char*)(buf + offset), min(len - offset, maxwrite));
			}
			_dprintf("multiple write: r = %d\n", r);
			sum += r;
			offset += maxwrite;
		}
		if (r < 0) sum = r;
	}
	else {
		while (r == 0) {
			r = _mssl_write(cookie, buf, len);
			_dprintf("single write: r = %d\n", r);
		}
		sum = r;
	}
	return sum;
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
			sb_pack(&kuki->outsock);
			_socket_write(kuki->sd, &(kuki->outsock));

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
	kuki->outbufcnt = 0;

	if (matrixSslNewSession(&(kuki->ssl), keys, NULL, client ? 0 : SSL_FLAGS_SERVER) < 0) {
		_dprintf("%s: matrixSslNewSession failed\n", __FUNCTION__);
		goto ERROR;
	}

	sb_alloc(&(kuki->insock), 1024);
	sb_alloc(&(kuki->outsock), 2048);
	sb_alloc(&(kuki->inbuf), 0);
	
	if (client) {
		matrixSslSetCertValidator(kuki->ssl, cert_valid, keys);

		n = matrixSslEncodeClientHello(kuki->ssl, &(kuki->outsock), 0);
		if (n < 0) {
			_dprintf("%s: matrixSslEncodeClientHello failed\n", __FUNCTION__);
			goto ERROR;
		}
		if (_socket_write(kuki->sd, &(kuki->outsock)) < 0) {
			_dprintf("%s: error while writing HELLO\n", __FUNCTION__);
			goto ERROR;
		}
		kuki->outsock.start = kuki->outsock.end = kuki->outsock.buf;
	}
	
MORE:
	r = mssl_read(kuki, buf, sizeof(buf));
	if (r > 0 || (r == 0 && matrixSslHandshakeIsComplete(kuki->ssl) == 0)) {
		if (kuki->sslend) {
			_dprintf("%s: end reached\n", __FUNCTION__);
			errno = EIO;
			goto ERROR;
		}
		_dprintf("%s: =0 goto more\n", __FUNCTION__);
		goto MORE;
	}
	else if (r < 0) {
		_dprintf("%s: read error r=%d errno=%d\n", __FUNCTION__, r, errno);
		errno = EIO;
		goto ERROR;
	}

	if ((f = fopencookie(kuki, "r+", mssl)) == NULL) {
		_dprintf("%s: fopencookie failed\n", __FUNCTION__);
		goto ERROR;
	}
	return f;

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
	_dprintf("%s()\n", __FUNCTION__);
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
