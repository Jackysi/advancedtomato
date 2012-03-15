
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "edns.h"
#include "logger.h"
#include "probes.h"
#include "udp_request.h"
#include "udp_request_p.h"
#include "utils.h"
#include "uv.h"
#include "uv_alloc.h"

static uv_buf_t
udp_listener_alloc_cb(uv_handle_t *handle, size_t size)
{
    ProxyContext * const proxy_context = handle->data;

    return uv_alloc_get_buffer(proxy_context, size);
}

static uv_buf_t
udp_alloc_cb(uv_handle_t *handle, size_t size)
{
    UDPRequest * const udp_request = handle->data;

    return uv_alloc_get_buffer(udp_request->proxy_context, size);
}

static void
proxy_resolver_close_cb(uv_handle_t *handle)
{
    UDPRequest * const udp_request = handle->data;

    (void) udp_request;
    DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_DONE(udp_request);
}

static void udp_request_free(UDPRequest * const udp_request);

static void
proxy_resolver_close_and_free_cb(uv_handle_t *handle)
{
    UDPRequest * const udp_request = handle->data;

    assert(udp_request->status.has_proxy_resolver_handle == 0);
    proxy_resolver_close_cb(handle);
    udp_request_free(udp_request);
}

static void
udp_request_free(UDPRequest * const udp_request)
{
    ProxyContext *proxy_context;

    if (udp_request->status.has_timeout_timer) {
        udp_request->status.has_timeout_timer = 0;
        uv_timer_stop(&udp_request->timeout_timer);
    }
    if (udp_request->status.has_proxy_resolver_handle) {
        udp_request->status.has_proxy_resolver_handle = 0;
        uv_close((uv_handle_t *) &udp_request->proxy_resolver_handle,
                 proxy_resolver_close_and_free_cb);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UDP_DONE(udp_request);
    proxy_context = udp_request->proxy_context;
    ngx_queue_remove(udp_request);
    udp_request->dns_packet_len = (size_t) 0U;
    udp_request->proxy_context = NULL;
    free(udp_request);
    assert(proxy_context->connections_count > 0U);
    proxy_context->connections_count--;
    DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                          proxy_context->connections_count_max);
}

static void
udp_request_kill(UDPRequest * const udp_request)
{
    if (udp_request == NULL || udp_request->status.is_dying) {
        return;
    }
    udp_request->status.is_dying = 1;
    udp_request_free(udp_request);
}

static void
proxy_to_client_cb(uv_udp_send_t *req, int status)
{
    UDPRequest * const udp_request = req->data;

    (void) status;
    DNSCRYPT_PROXY_REQUEST_UDP_REPLIED(udp_request);
    assert(req == &udp_request->proxy_to_client_send_query);
    udp_request_kill(udp_request);
}

static void
resolver_to_proxy_cb(uv_udp_t *handle, ssize_t nread, uv_buf_t buf,
                     struct sockaddr *resolver_addr, unsigned flags)
{
    UDPRequest *udp_request = handle->data;
    size_t      uncurved_len;

    (void) flags;
    assert(handle == &udp_request->proxy_resolver_handle);
    assert(((struct sockaddr_storage *) (void *) resolver_addr)->ss_family ==
           udp_request->proxy_context->resolver_addr.ss_family);
    uv_udp_recv_stop(&udp_request->proxy_resolver_handle);

    if (nread < (ssize_t) (DNS_HEADER_SIZE + dnscrypt_response_header_size()) ||
        nread > (ssize_t) sizeof udp_request->dns_packet) {
        uv_alloc_release_buffer(udp_request->proxy_context, &buf);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_REPLIED(udp_request);
    udp_request->dns_packet_len = (size_t) nread;
    assert(udp_request->dns_packet_len <= sizeof udp_request->dns_packet);
    memcpy(udp_request->dns_packet, buf.base, udp_request->dns_packet_len);
    uv_alloc_release_buffer(udp_request->proxy_context, &buf);

    uncurved_len = udp_request->dns_packet_len;
    DNSCRYPT_PROXY_REQUEST_UNCURVE_START(udp_request, uncurved_len);
    if (dnscrypt_client_uncurve
        (&udp_request->proxy_context->dnscrypt_client,
            udp_request->client_nonce,
            (uint8_t *) udp_request->dns_packet, &uncurved_len) != 0) {
        DNSCRYPT_PROXY_REQUEST_UNCURVE_DONE(udp_request, uncurved_len);
        DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_GOT_INVALID_REPLY(udp_request);
        logger_noformat(udp_request->proxy_context, LOG_WARNING,
                        "Received a suscpicious reply from the resolver");
        udp_request_kill(udp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UNCURVE_DONE(udp_request, uncurved_len);
    memset(udp_request->client_nonce, 0, sizeof udp_request->client_nonce);
    assert(uncurved_len <= udp_request->dns_packet_len);
    udp_request->dns_packet_len = uncurved_len;

    struct sockaddr_in client_addr;
    assert(sizeof client_addr == udp_request->client_addr_len);
    memcpy(&client_addr, &udp_request->client_addr, sizeof client_addr);

    uv_udp_send(&udp_request->proxy_to_client_send_query,
                udp_request->client_proxy_handle_p,
                & (uv_buf_t) { .base = (void *) udp_request->dns_packet,
                               .len  = udp_request->dns_packet_len }, 1,
                client_addr, proxy_to_client_cb);
    udp_request->proxy_to_client_send_query.data = udp_request;

    udp_request->status.has_proxy_resolver_handle = 0;
    uv_close((uv_handle_t *) &udp_request->proxy_resolver_handle,
             proxy_resolver_close_cb);
}

static void
proxy_to_resolver_cb(uv_udp_send_t *req, int status)
{
    UDPRequest * const udp_request = req->data;

    assert(req == &udp_request->proxy_to_resolver_send_query);
    if (status < 0) {
        udp_request_kill(udp_request);
        return;
    }
    uv_udp_recv_start(&udp_request->proxy_resolver_handle,
                      udp_alloc_cb, resolver_to_proxy_cb);
}

static void
proxy_client_send_truncated(UDPRequest * const udp_request)
{
    DNSCRYPT_PROXY_REQUEST_UDP_TRUNCATED(udp_request);

    COMPILER_ASSERT(sizeof udp_request->dns_packet > DNS_OFFSET_FLAGS2);
    udp_request->dns_packet[DNS_OFFSET_FLAGS] |= DNS_FLAGS_TC | DNS_FLAGS_QR;
    udp_request->dns_packet[DNS_OFFSET_FLAGS2] |= DNS_FLAGS2_RA;

    struct sockaddr_in client_addr;
    assert(sizeof client_addr == udp_request->client_addr_len);
    memcpy(&client_addr, &udp_request->client_addr, sizeof client_addr);
    uv_udp_send(&udp_request->proxy_to_client_send_query,
                udp_request->client_proxy_handle_p,
                & (uv_buf_t) { .base = (void *) udp_request->dns_packet,
                               .len  = udp_request->dns_packet_len }, 1,
                client_addr, proxy_to_client_cb);
    udp_request->proxy_to_client_send_query.data = udp_request;
}

static void
timeout_timer_cb(uv_timer_t *handle, int status)
{
    UDPRequest * const udp_request = handle->data;

    (void) status;
    DNSCRYPT_PROXY_REQUEST_UDP_TIMEOUT(udp_request);
    logger_noformat(udp_request->proxy_context,
                    LOG_DEBUG, "resolver timeout (UDP)");
    udp_request_kill(udp_request);
}

#ifndef SO_RCVBUFFORCE
# define SO_RCVBUFFORCE SO_RCVBUF
#endif
#ifndef SO_SNDBUFFORCE
# define SO_SNDBUFFORCE SO_SNDBUF
#endif

static void
udp_tune(uv_udp_t * const handle)
{
#ifdef _WIN32
    (void) handle;
#else
    const int sock = handle->fd;

    if (sock == -1) {
        return;
    }
    setsockopt(sock, SOL_SOCKET, SO_RCVBUFFORCE,
               (int []) { UDP_BUFFER_SIZE }, sizeof (int));
    setsockopt(sock, SOL_SOCKET, SO_SNDBUFFORCE,
               (int []) { UDP_BUFFER_SIZE }, sizeof (int));
# if defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_DONT)
    setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER,
               (int []) { IP_PMTUDISC_DONT }, sizeof (int));
# elif defined(IP_DONTFRAG)
    setsockopt(sock, IPPROTO_IP, IP_DONTFRAG,  (int []) { 0 }, sizeof (int));
# endif
#endif
}

static void
client_to_proxy_cb(uv_udp_t *handle, ssize_t nread, uv_buf_t buf,
                   struct sockaddr *client_addr, unsigned flags)
{
    ProxyContext *proxy_context = handle->data;
    UDPRequest   *udp_request;
    ssize_t       curve_ret;
    size_t        max_packet_size;
    size_t        request_edns_payload_size;

    (void) flags;
    assert(handle == &proxy_context->udp_listener_handle);
    if (nread == (ssize_t) 0) {
        uv_alloc_release_buffer(proxy_context, &buf);
        return;
    }
    if (nread < (ssize_t) DNS_HEADER_SIZE ||
        (size_t) nread > sizeof udp_request->dns_packet) {
        logger_noformat(proxy_context, LOG_WARNING, "Short query received");
        uv_alloc_release_buffer(proxy_context, &buf);
        return;
    }
    if (proxy_context->connections_count
        >= proxy_context->connections_count_max ||
        (udp_request = malloc(sizeof *udp_request)) == NULL) {
        uv_alloc_release_buffer(proxy_context, &buf);
        if (! ngx_queue_empty(&proxy_context->udp_request_queue)) {
            udp_request_kill((UDPRequest *)
                             ngx_queue_last(&proxy_context->udp_request_queue));
        }
        DNSCRYPT_PROXY_REQUEST_UDP_OVERLOADED();
        return;
    }
    proxy_context->connections_count++;
    assert(proxy_context->connections_count
           <= proxy_context->connections_count_max);
    DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                          proxy_context->connections_count_max);
    DNSCRYPT_PROXY_REQUEST_UDP_START(udp_request);
    ngx_queue_insert_head(&proxy_context->udp_request_queue,
                          (ngx_queue_t *) udp_request);
    udp_request->proxy_context = proxy_context;
    udp_request->client_proxy_handle_p = &proxy_context->udp_listener_handle;
    udp_request->proxy_resolver_handle.data = NULL;
    memset(&udp_request->status, 0, sizeof udp_request->status);

    udp_request->dns_packet_len = (size_t) nread;
    assert(udp_request->dns_packet_len <= sizeof udp_request->dns_packet);
    memcpy(udp_request->dns_packet, buf.base, udp_request->dns_packet_len);
    uv_alloc_release_buffer(proxy_context, &buf);

    edns_add_section(proxy_context,
                     udp_request->dns_packet, &udp_request->dns_packet_len,
                     sizeof udp_request->dns_packet,
                     &request_edns_payload_size);

    udp_request->client_addr_len = sizeof(struct sockaddr_in);
    memcpy(&udp_request->client_addr, client_addr,
           udp_request->client_addr_len);

    if (request_edns_payload_size < DNS_MAX_PACKET_SIZE_UDP_SEND) {
        max_packet_size = DNS_MAX_PACKET_SIZE_UDP_SEND;
    } else {
        max_packet_size = request_edns_payload_size;
    }
    if (max_packet_size > sizeof udp_request->dns_packet) {
        max_packet_size = sizeof udp_request->dns_packet;
    }
    assert(max_packet_size <= sizeof udp_request->dns_packet);
    assert(SIZE_MAX - DNSCRYPT_MAX_PADDING - dnscrypt_query_header_size()
           > udp_request->dns_packet_len);
    size_t max_len = udp_request->dns_packet_len + DNSCRYPT_MAX_PADDING +
        dnscrypt_query_header_size();
    if (max_len > max_packet_size) {
        max_len = max_packet_size;
    }
    if (udp_request->proxy_context->tcp_only != 0 ||
        udp_request->dns_packet_len + dnscrypt_query_header_size() > max_len) {
        proxy_client_send_truncated(udp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_CURVE_START(udp_request,
                                       udp_request->dns_packet_len);
    curve_ret =
        dnscrypt_client_curve(&udp_request->proxy_context->dnscrypt_client,
                              udp_request->client_nonce,
                              udp_request->dns_packet,
                              udp_request->dns_packet_len, max_len);
    if (curve_ret <= (ssize_t) 0) {
        return;
    }
    udp_request->dns_packet_len = (size_t) curve_ret;
    assert(udp_request->dns_packet_len >= dnscrypt_query_header_size());
    DNSCRYPT_PROXY_REQUEST_CURVE_DONE(udp_request, udp_request->dns_packet_len);
    assert(udp_request->dns_packet_len <= sizeof udp_request->dns_packet);

    uv_udp_init(udp_request->proxy_context->event_loop,
                &udp_request->proxy_resolver_handle);
    udp_request->status.has_proxy_resolver_handle = 1;

    struct sockaddr_in resolver_addr;
    memcpy(&resolver_addr, &udp_request->proxy_context->resolver_addr,
           udp_request->proxy_context->resolver_addr_len);

    udp_request->proxy_resolver_handle.data = udp_request;

    uv_timer_init(udp_request->proxy_context->event_loop,
                  &udp_request->timeout_timer);
    udp_request->status.has_timeout_timer = 1;
    udp_request->timeout_timer.data = udp_request;
    uv_timer_start(&udp_request->timeout_timer, timeout_timer_cb,
                   (int64_t) DNS_QUERY_TIMEOUT, (int64_t) 0);

    uv_udp_send(&udp_request->proxy_to_resolver_send_query,
                &udp_request->proxy_resolver_handle,
                & (uv_buf_t) { .base = (void *) udp_request->dns_packet,
                               .len  = udp_request->dns_packet_len }, 1,
                resolver_addr, proxy_to_resolver_cb);
    udp_request->proxy_to_resolver_send_query.data = udp_request;
    udp_tune(&udp_request->proxy_resolver_handle);

    DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_START(udp_request);
}

static void
listener_close_cb(uv_handle_t *handle)
{
    ProxyContext * const proxy_context = handle->data;
    logger_noformat(proxy_context, LOG_INFO, "UDP listener shut down");
}

int
udp_listener_bind(ProxyContext * const proxy_context)
{
    struct sockaddr_in addr = uv_ip4_addr(proxy_context->listen_ip,
                                          proxy_context->local_port);
    uv_udp_init(proxy_context->event_loop,
                &proxy_context->udp_listener_handle);
    proxy_context->udp_listener_handle.data = proxy_context;
    ngx_queue_init(&proxy_context->udp_request_queue);
    if (uv_udp_bind(&proxy_context->udp_listener_handle, addr, 0) != 0) {
        logger(proxy_context, LOG_ERR, "Unable to bind [%s] (UDP)",
               proxy_context->listen_ip);
        return -1;
    }
    udp_tune(&proxy_context->udp_listener_handle);

    return 0;
}

int
udp_listener_start(ProxyContext * const proxy_context)
{
    uv_udp_recv_start(&proxy_context->udp_listener_handle,
                      udp_listener_alloc_cb, client_to_proxy_cb);

    return 0;
}

void
udp_listener_stop(ProxyContext * const proxy_context)
{
    uv_udp_recv_stop(&proxy_context->udp_listener_handle);
    uv_close((uv_handle_t *) &proxy_context->udp_listener_handle,
             listener_close_cb);
}
