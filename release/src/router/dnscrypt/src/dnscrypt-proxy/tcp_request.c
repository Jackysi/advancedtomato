
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
#endif

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "logger.h"
#include "probes.h"
#include "tcp_request.h"
#include "tcp_request_p.h"
#include "uv.h"
#include "uv_alloc.h"

static uv_buf_t
tcp_alloc_cb(uv_handle_t *handle, size_t size)
{
    TCPRequest * const tcp_request = handle->data;

    return uv_alloc_get_buffer(tcp_request->proxy_context, size);
}

static void tcp_request_free(TCPRequest * const tcp_request);

static void
proxy_resolver_close_cb(uv_handle_t *handle)
{
    (void) handle;
}

static void
proxy_resolver_close_and_free_cb(uv_handle_t *handle)
{
    TCPRequest * const tcp_request = handle->data;

    assert(tcp_request->status.has_proxy_resolver_handle == 0);
    proxy_resolver_close_cb(handle);
    tcp_request_free(tcp_request);
}

static void
client_proxy_close_cb(uv_handle_t *handle)
{
    (void) handle;
}

static void
client_proxy_close_and_free_cb(uv_handle_t *handle)
{
    TCPRequest * const tcp_request = handle->data;

    assert(tcp_request->status.has_client_proxy_handle == 0);
    client_proxy_close_cb(handle);
    tcp_request_free(tcp_request);
}

static void
tcp_request_free(TCPRequest * const tcp_request)
{
    ProxyContext *proxy_context;

    if (tcp_request->status.has_timeout_timer) {
        tcp_request->status.has_timeout_timer = 0;
        uv_timer_stop(&tcp_request->timeout_timer);
    }
    if (tcp_request->status.has_client_proxy_handle) {
        tcp_request->status.has_client_proxy_handle = 0;
        uv_close((uv_handle_t *) &tcp_request->client_proxy_handle,
                 client_proxy_close_and_free_cb);
        return;
    }
    if (tcp_request->status.has_proxy_resolver_handle) {
        tcp_request->status.has_proxy_resolver_handle = 0;
        uv_close((uv_handle_t *) &tcp_request->proxy_resolver_handle,
                 proxy_resolver_close_and_free_cb);
        logger(tcp_request->proxy_context, LOG_DEBUG,
               "Resolver is unreachable over TCP");
        return;
    }
    DNSCRYPT_PROXY_REQUEST_TCP_DONE(tcp_request);
    proxy_context = tcp_request->proxy_context;
    ngx_queue_remove(tcp_request);
    tcp_request->dns_packet_len = (size_t) 0U;
    tcp_request->proxy_context = NULL;
    free(tcp_request);
    assert(proxy_context->connections_count > 0U);
    proxy_context->connections_count--;
    DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                          proxy_context->connections_count_max);
}

static void
tcp_request_kill(TCPRequest * const tcp_request)
{
    if (tcp_request == NULL || tcp_request->status.is_dying) {
        return;
    }
    tcp_request->status.is_dying = 1;
    tcp_request_free(tcp_request);
}

static void
timeout_timer_cb(uv_timer_t *handle, int status)
{
    TCPRequest * const tcp_request = handle->data;

    (void) status;
    DNSCRYPT_PROXY_REQUEST_TCP_TIMEOUT(tcp_request);
    logger_noformat(tcp_request->proxy_context,
                    LOG_DEBUG, "resolver timeout (TCP)");
    tcp_request_kill(tcp_request);
}

static int
read_packet_size(TCPRequest * const tcp_request, ssize_t * const nread_p,
                 const uint8_t **packet_itr_p)
{
    assert(*nread_p > (ssize_t) 0);
    tcp_request->dns_packet_expected_len = (size_t) (**packet_itr_p << 8);
    (*packet_itr_p)++;
    (*nread_p)--;
    tcp_request->status.has_expected_size_partial = 1;
    if (*nread_p <= (ssize_t) 0) {
        return 1;
    }
    assert((tcp_request->dns_packet_expected_len & 0xff) == 0U);
    tcp_request->dns_packet_expected_len |= (size_t) **packet_itr_p;
    (*packet_itr_p)++;
    (*nread_p)--;
    tcp_request->status.has_expected_size_partial = 0;
    tcp_request->status.has_expected_size = 1;

    return 2;
}

static void
proxy_to_client_cb(uv_write_t *req, int status)
{
    TCPRequest * const tcp_request = req->data;

    (void) status;
    DNSCRYPT_PROXY_REQUEST_TCP_REPLIED(tcp_request);
    assert(req == &tcp_request->proxy_to_client_send_query);
    tcp_request_kill(tcp_request);
}

static void
resolver_to_proxy_cb(uv_stream_t *handle, ssize_t nread, uv_buf_t buf)
{
    TCPRequest    *tcp_request = handle->data;
    const uint8_t *packet_itr;
    size_t         uncurved_len;

    assert((uv_tcp_t *) handle == &tcp_request->proxy_resolver_handle);
    if (nread == (ssize_t) 0) {
        goto bye;
    }
    if (nread == (ssize_t) -1) {
        tcp_request_kill(tcp_request);
        goto bye;
    }
    packet_itr = (const uint8_t *) buf.base;
    if (tcp_request->status.has_expected_size == 0) {
        read_packet_size(tcp_request, &nread, &packet_itr);
    }
    if (nread <= (ssize_t) 0) {
        goto bye;
    }
    assert(nread > (ssize_t) 0);
    assert(tcp_request->status.has_expected_size_partial == 0);
    assert(tcp_request->status.has_expected_size == 1);
    assert(tcp_request->dns_packet_expected_len <= 0xffff);
    assert(tcp_request->dns_packet_len <= tcp_request->dns_packet_expected_len);
    if ((size_t) nread >
        tcp_request->dns_packet_expected_len - tcp_request->dns_packet_len) {
        nread = (ssize_t) (tcp_request->dns_packet_expected_len -
                           tcp_request->dns_packet_len);
    }
    assert(sizeof tcp_request->dns_packet >= (size_t) nread);
    assert(sizeof tcp_request->dns_packet - (size_t) nread
           >= tcp_request->dns_packet_len);
    memcpy(&tcp_request->dns_packet[tcp_request->dns_packet_len],
           packet_itr, (size_t) nread);
    tcp_request->dns_packet_len += (size_t) nread;
    if (tcp_request->dns_packet_len < tcp_request->dns_packet_expected_len) {
        goto bye;
    }
    if (tcp_request->dns_packet_len
        < (size_t) DNS_HEADER_SIZE + dnscrypt_response_header_size()) {
        logger_noformat(tcp_request->proxy_context, LOG_WARNING,
                        "Short query received");
        tcp_request_kill(tcp_request);
        goto bye;
    }
    uncurved_len = tcp_request->dns_packet_len;
    DNSCRYPT_PROXY_REQUEST_UNCURVE_START(tcp_request, uncurved_len);
    if (dnscrypt_client_uncurve
        (&tcp_request->proxy_context->dnscrypt_client,
            tcp_request->client_nonce,
            (uint8_t *) tcp_request->dns_packet, &uncurved_len) != 0) {
        DNSCRYPT_PROXY_REQUEST_UNCURVE_DONE(tcp_request, uncurved_len);
        DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_GOT_INVALID_REPLY(tcp_request);
        logger_noformat(tcp_request->proxy_context, LOG_WARNING,
                        "Received a suscpicious reply from the resolver");
        tcp_request_kill(tcp_request);
        goto bye;
    }
    DNSCRYPT_PROXY_REQUEST_UNCURVE_DONE(tcp_request, uncurved_len);
    memset(tcp_request->client_nonce, 0, sizeof tcp_request->client_nonce);
    assert(uncurved_len <= tcp_request->dns_packet_len);
    tcp_request->dns_packet_len = uncurved_len;
    assert(tcp_request->dns_packet_len <= 0xffff);
    tcp_request->dns_packet_nlen =
        htons((uint16_t) tcp_request->dns_packet_len);
    uv_write(&tcp_request->proxy_to_client_send_query,
             (uv_stream_t *) &tcp_request->client_proxy_handle,
             (uv_buf_t[]) {
                 { .base = (void *) &tcp_request->dns_packet_nlen,
                   .len  = sizeof tcp_request->dns_packet_nlen },
                 { .base = (void *) tcp_request->dns_packet,
                   .len  = tcp_request->dns_packet_len }
             }, 2, proxy_to_client_cb);
    tcp_request->proxy_to_client_send_query.data = tcp_request;

    tcp_request->status.has_proxy_resolver_handle = 0;
    uv_close((uv_handle_t *) &tcp_request->proxy_resolver_handle,
             proxy_resolver_close_cb);
    DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_DONE(tcp_request);

bye:
    uv_alloc_release_buffer(tcp_request->proxy_context, &buf);
}

static void
proxy_to_resolver_cb(uv_write_t *req, int status)
{
    TCPRequest * const tcp_request = req->data;

    assert(req == &tcp_request->proxy_to_resolver_send_query);
    if (status < 0) {
        tcp_request_kill(tcp_request);
        return;
    }
    tcp_request->dns_packet_len = (size_t) 0U;
    tcp_request->dns_packet_expected_len = (size_t) 0U;
    tcp_request->status.has_expected_size = 0;
    tcp_request->status.has_expected_size_partial = 0;
    uv_read_start((uv_stream_t *) &tcp_request->proxy_resolver_handle,
                  tcp_alloc_cb, resolver_to_proxy_cb);
}

static void
proxy_to_resolver_connect_cb(uv_connect_t *handle, int status)
{
    TCPRequest *tcp_request = handle->data;
    ssize_t     curve_ret;

    if (status < 0) {
        tcp_request_kill(tcp_request);
        return;
    }
    assert(SIZE_MAX - DNSCRYPT_MAX_PADDING - dnscrypt_query_header_size()
           > tcp_request->dns_packet_len);
    size_t max_len = tcp_request->dns_packet_len + DNSCRYPT_MAX_PADDING +
        dnscrypt_query_header_size();
    if (max_len > TCP_MAX_PACKET_SIZE) {
        max_len = TCP_MAX_PACKET_SIZE;
    }
    if (tcp_request->dns_packet_len + dnscrypt_query_header_size() > max_len) {
        return;
    }
    DNSCRYPT_PROXY_REQUEST_CURVE_START(tcp_request,
                                       tcp_request->dns_packet_len);
    curve_ret =
        dnscrypt_client_curve(&tcp_request->proxy_context->dnscrypt_client,
                              tcp_request->client_nonce,
                              tcp_request->dns_packet,
                              tcp_request->dns_packet_len, max_len);
    if (curve_ret <= (ssize_t) 0) {
        return;
    }
    tcp_request->dns_packet_len = (size_t) curve_ret;
    assert(tcp_request->dns_packet_len >= dnscrypt_query_header_size());
    DNSCRYPT_PROXY_REQUEST_CURVE_DONE(tcp_request, tcp_request->dns_packet_len);
    assert(tcp_request->dns_packet_len <= sizeof tcp_request->dns_packet);
    assert(tcp_request->dns_packet_len <= 0xffff);
    tcp_request->dns_packet_nlen =
        htons((uint16_t) tcp_request->dns_packet_len);
    uv_write(&tcp_request->proxy_to_resolver_send_query,
             (uv_stream_t *) &tcp_request->proxy_resolver_handle,
             (uv_buf_t[]) {
                 { .base = (void *) &tcp_request->dns_packet_nlen,
                   .len  = sizeof tcp_request->dns_packet_nlen },
                 { .base = (void *) tcp_request->dns_packet,
                   .len  = tcp_request->dns_packet_len }
             }, 2, proxy_to_resolver_cb);
    tcp_request->proxy_to_resolver_send_query.data = tcp_request;
}

#ifndef SO_RCVBUFFORCE
# define SO_RCVBUFFORCE SO_RCVBUF
#endif
#ifndef SO_SNDBUFFORCE
# define SO_SNDBUFFORCE SO_SNDBUF
#endif

static void
tcp_tune(const uv_tcp_t * const handle)
{
#ifdef _WIN32
    (void) handle;
#else
    const int sock = handle->fd;

    if (sock == -1) {
        return;
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE,
               (int []) { TCP_BUFFER_SIZE }, sizeof (int));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUFFORCE,
               (int []) { TCP_BUFFER_SIZE }, sizeof (int));
#endif
}

static void
client_to_proxy_cb(uv_stream_t *handle, ssize_t nread, uv_buf_t buf)
{
    TCPRequest    *tcp_request = handle->data;
    const uint8_t *packet_itr;

    assert((uv_tcp_t *) handle == &tcp_request->client_proxy_handle);
    if (nread == (ssize_t) 0) {
        goto bye;
    }
    if (nread == (ssize_t) -1) {
        tcp_request_kill(tcp_request);
        goto bye;
    }
    packet_itr = (const uint8_t *) buf.base;
    if (tcp_request->status.has_expected_size == 0) {
        read_packet_size(tcp_request, &nread, &packet_itr);
    }
    if (nread <= (ssize_t) 0) {
        goto bye;
    }
    assert(nread > (ssize_t) 0);
    assert(tcp_request->status.has_expected_size_partial == 0);
    assert(tcp_request->status.has_expected_size == 1);
    assert(tcp_request->dns_packet_expected_len <= 0xffff);
    assert(tcp_request->dns_packet_len <= tcp_request->dns_packet_expected_len);
    if ((size_t) nread >
        tcp_request->dns_packet_expected_len - tcp_request->dns_packet_len) {
        nread = (ssize_t) (tcp_request->dns_packet_expected_len -
                           tcp_request->dns_packet_len);
    }
    assert(sizeof tcp_request->dns_packet >= (size_t) nread);
    assert(sizeof tcp_request->dns_packet - (size_t) nread
           >= tcp_request->dns_packet_len);
    memcpy(&tcp_request->dns_packet[tcp_request->dns_packet_len],
           packet_itr, (size_t) nread);
    tcp_request->dns_packet_len += (size_t) nread;
    if (tcp_request->dns_packet_len < tcp_request->dns_packet_expected_len) {
        goto bye;
    }

    struct sockaddr_in resolver_addr;
    assert(sizeof resolver_addr
           == tcp_request->proxy_context->resolver_addr_len);
    memcpy(&resolver_addr, &tcp_request->proxy_context->resolver_addr,
           sizeof resolver_addr);

    uv_tcp_init(tcp_request->proxy_context->event_loop,
                &tcp_request->proxy_resolver_handle);
    tcp_request->status.has_proxy_resolver_handle = 1;
    tcp_request->proxy_resolver_handle.data = tcp_request;
    uv_tcp_nodelay(&tcp_request->proxy_resolver_handle, 1);
    uv_tcp_connect(&tcp_request->proxy_to_resolver_connect_query,
                   &tcp_request->proxy_resolver_handle,
                   resolver_addr, proxy_to_resolver_connect_cb);
    tcp_request->proxy_to_resolver_connect_query.data = tcp_request;
    tcp_tune(&tcp_request->proxy_resolver_handle);

    DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_START(tcp_request);

bye:
    uv_alloc_release_buffer(tcp_request->proxy_context, &buf);
}

static void
tcp_connection_cb(uv_stream_t *handle, int status)
{
    ProxyContext *proxy_context = handle->data;
    TCPRequest   *tcp_request;

    assert((uv_tcp_t *) handle == &proxy_context->tcp_listener_handle);
    if (status < 0) {
        return;
    }
    if (proxy_context->connections_count
        >= proxy_context->connections_count_max ||
        (tcp_request = malloc(sizeof *tcp_request)) == NULL) {
        if (! ngx_queue_empty(&proxy_context->tcp_request_queue)) {
            tcp_request_kill((TCPRequest *)
                             ngx_queue_last(&proxy_context->tcp_request_queue));
        }
        DNSCRYPT_PROXY_REQUEST_TCP_OVERLOADED();
        return;
    }
    tcp_request->proxy_context = proxy_context;
    tcp_request->proxy_resolver_handle.data = NULL;
    memset(&tcp_request->status, 0, sizeof tcp_request->status);
    tcp_request->status.has_client_proxy_handle = 1;
    tcp_request->dns_packet_len = (size_t) 0U;
    tcp_request->dns_packet_expected_len = (size_t) 0U;
    uv_tcp_init(tcp_request->proxy_context->event_loop,
                &tcp_request->client_proxy_handle);
    if (uv_accept((uv_stream_t *) &proxy_context->tcp_listener_handle,
                  (uv_stream_t *) &tcp_request->client_proxy_handle) != 0) {
        DNSCRYPT_PROXY_REQUEST_TCP_OVERLOADED();
        free(tcp_request);
        return;
    }
    proxy_context->connections_count++;
    assert(proxy_context->connections_count
           <= proxy_context->connections_count_max);
    DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                          proxy_context->connections_count_max);
    DNSCRYPT_PROXY_REQUEST_TCP_START(tcp_request);
    ngx_queue_insert_head(&proxy_context->tcp_request_queue,
                          (ngx_queue_t *) tcp_request);
    tcp_request->client_proxy_handle.data = tcp_request;
    tcp_request->dns_packet_len = (size_t) 0U;

    uv_timer_init(tcp_request->proxy_context->event_loop,
                  &tcp_request->timeout_timer);
    tcp_request->status.has_timeout_timer = 1;
    tcp_request->timeout_timer.data = tcp_request;
    uv_timer_start(&tcp_request->timeout_timer, timeout_timer_cb,
                   (int64_t) DNS_QUERY_TIMEOUT, (int64_t) 0);

    uv_read_start((uv_stream_t *) &tcp_request->client_proxy_handle,
                  tcp_alloc_cb, client_to_proxy_cb);
}

static void
listener_close_cb(uv_handle_t *handle)
{
    ProxyContext * const proxy_context = handle->data;
    logger_noformat(proxy_context, LOG_INFO, "TCP listener shut down");
}

int
tcp_listener_bind(ProxyContext * const proxy_context)
{
    struct sockaddr_in addr = uv_ip4_addr(proxy_context->listen_ip,
                                          proxy_context->local_port);
    uv_tcp_init(proxy_context->event_loop,
                &proxy_context->tcp_listener_handle);
    proxy_context->tcp_listener_handle.data = proxy_context;
    ngx_queue_init(&proxy_context->tcp_request_queue);
    if (uv_tcp_bind(&proxy_context->tcp_listener_handle, addr) != 0) {
        logger(proxy_context, LOG_ERR, "Unable to bind [%s] (TCP)",
               proxy_context->listen_ip);
        return -1;
    }
    return 0;
}

int
tcp_listener_start(ProxyContext * const proxy_context)
{
    uv_listen((uv_stream_t *) &proxy_context->tcp_listener_handle,
              TCP_REQUEST_BACKLOG, tcp_connection_cb);

    return 0;
}

void
tcp_listener_stop(ProxyContext * const proxy_context)
{
    uv_close((uv_handle_t *) &proxy_context->tcp_listener_handle,
             listener_close_cb);
}
