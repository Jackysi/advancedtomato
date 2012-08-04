
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
# include <netinet/in.h>
#endif

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/util.h>

#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "edns.h"
#include "logger.h"
#include "probes.h"
#include "queue.h"
#include "tcp_request.h"
#include "udp_request.h"
#include "udp_request_p.h"
#include "utils.h"

static void
udp_request_free(UDPRequest * const udp_request)
{
    ProxyContext *proxy_context;

    if (udp_request->sendto_retry_timer != NULL) {
        free(event_get_callback_arg(udp_request->sendto_retry_timer));
        event_free(udp_request->sendto_retry_timer);
        udp_request->sendto_retry_timer = NULL;
    }
    if (udp_request->timeout_timer != NULL) {
        event_free(udp_request->timeout_timer);
        udp_request->timeout_timer = NULL;
    }
    DNSCRYPT_PROXY_REQUEST_UDP_DONE(udp_request);
    proxy_context = udp_request->proxy_context;
    if (udp_request->status.is_in_queue != 0) {
        assert(! TAILQ_EMPTY(&proxy_context->udp_request_queue));
        TAILQ_REMOVE(&proxy_context->udp_request_queue, udp_request, queue);
        assert(proxy_context->connections_count > 0U);
        proxy_context->connections_count--;
        DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                              proxy_context->connections_count_max);
    }
    udp_request->proxy_context = NULL;
    free(udp_request);
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

static int sendto_with_retry(SendtoWithRetryCtx * const ctx);

#ifndef UDP_REQUEST_NO_RETRIES
static void
sendto_with_retry_timer_cb(evutil_socket_t retry_timer_handle, short ev_flags,
                           void * const ctx_)
{
    SendtoWithRetryCtx * const ctx = ctx_;

    (void) ev_flags;
    assert(retry_timer_handle ==
           event_get_fd(ctx->udp_request->sendto_retry_timer));

    DNSCRYPT_PROXY_REQUEST_UDP_RETRY(ctx->udp_request,
                                     ctx->udp_request->retries);
    sendto_with_retry(ctx);
}
#endif

static int
sendto_with_retry(SendtoWithRetryCtx * const ctx)
{
    SendtoWithRetryCtx *ctx_cb;
    UDPRequest         *udp_request = ctx->udp_request;
    int                 err;
    _Bool               retriable;

    if (sendto(ctx->handle, ctx->buffer, ctx->length, ctx->flags,
               ctx->dest_addr, ctx->dest_len) == (ssize_t) ctx->length) {
        if (ctx->cb) {
            ctx->cb(udp_request);
        }
        if (udp_request->sendto_retry_timer != NULL) {
            assert(event_get_callback_arg(udp_request->sendto_retry_timer)
                   == ctx);
            free(ctx);
            event_free(udp_request->sendto_retry_timer);
            udp_request->sendto_retry_timer = NULL;
        }
        return 0;
    }

    err = evutil_socket_geterror(udp_request->client_proxy_handle);
    logger(udp_request->proxy_context, LOG_WARNING,
           "sendto: [%s]", evutil_socket_error_to_string(err));
    DNSCRYPT_PROXY_REQUEST_UDP_NETWORK_ERROR(udp_request);

#ifdef UDP_REQUEST_NO_RETRIES
    (void) ctx_cb;
    (void) retriable;
    udp_request_kill(udp_request);

    return -1;

#else

# ifdef _WIN32
    retriable = (err == WSAENOBUFS ||
                 err == WSAEWOULDBLOCK || err == WSAEINTR);
# else
    retriable = (err == ENOBUFS || err == ENOMEM ||
                 err == EAGAIN || err == EINTR);
# endif
    if (retriable == 0) {
        udp_request_kill(udp_request);
        return -1;
    }
    COMPILER_ASSERT(DNS_QUERY_TIMEOUT < UCHAR_MAX);
    if (++(udp_request->retries) > DNS_QUERY_TIMEOUT) {
        udp_request_kill(udp_request);
        return -1;
    }
    if (udp_request->sendto_retry_timer != NULL) {
        ctx_cb = event_get_callback_arg(udp_request->sendto_retry_timer);
        assert(ctx_cb != NULL);
        assert(ctx_cb->udp_request == ctx->udp_request);
        assert(ctx_cb->buffer == ctx->buffer);
        assert(ctx_cb->cb == ctx->cb);
    } else {
        if ((ctx_cb = malloc(sizeof *ctx_cb)) == NULL) {
            logger_error(udp_request->proxy_context, "malloc");
            udp_request_kill(udp_request);
            return -1;
        }
        if ((udp_request->sendto_retry_timer =
             evtimer_new(udp_request->proxy_context->event_loop,
                         sendto_with_retry_timer_cb, ctx_cb)) == NULL) {
            free(ctx_cb);
            udp_request_kill(udp_request);
            return -1;
        }
        assert(ctx_cb ==
               event_get_callback_arg(udp_request->sendto_retry_timer));
        *ctx_cb = *ctx;
    }
    const struct timeval tv = {
        .tv_sec = (time_t) UDP_DELAY_BETWEEN_RETRIES, .tv_usec = 0
    };
    evtimer_add(udp_request->sendto_retry_timer, &tv);
    DNSCRYPT_PROXY_REQUEST_UDP_RETRY_SCHEDULED(udp_request,
                                               udp_request->retries);
    return -1;
#endif
}

static void
resolver_to_proxy_cb(evutil_socket_t proxy_resolver_handle, short ev_flags,
                     void * const proxy_context_)
{
    uint8_t                  dns_packet[DNS_MAX_PACKET_SIZE_UDP];
    ProxyContext            *proxy_context = proxy_context_;
    UDPRequest              *scanned_udp_request;
    UDPRequest              *udp_request = NULL;
    struct sockaddr_storage  resolver_sockaddr;
    ev_socklen_t             resolver_sockaddr_len = sizeof resolver_sockaddr;
    ssize_t                  nread;
    size_t                   dns_packet_len = (size_t) 0U;
    size_t                   uncurved_len;

    (void) ev_flags;
    nread = recvfrom(proxy_resolver_handle,
                     (void *) dns_packet, sizeof dns_packet, 0,
                     (struct sockaddr *) &resolver_sockaddr,
                     &resolver_sockaddr_len);
    if (nread < (ssize_t) 0) {
        const int err = evutil_socket_geterror(proxy_resolver_handle);
        logger(proxy_context, LOG_WARNING,
               "recvfrom(resolver): [%s]", evutil_socket_error_to_string(err));
        DNSCRYPT_PROXY_REQUEST_UDP_NETWORK_ERROR(NULL);
        return;
    }
    if (evutil_sockaddr_cmp((const struct sockaddr *) &resolver_sockaddr,
                            (const struct sockaddr *)
                            &proxy_context->resolver_sockaddr, 1) != 0) {
        logger_noformat(proxy_context, LOG_WARNING,
                        "Received a resolver reply from a different resolver");
        return;
    }
    TAILQ_FOREACH(scanned_udp_request,
                  &proxy_context->udp_request_queue, queue) {
        if (dnscrypt_cmp_client_nonce(scanned_udp_request->client_nonce,
                                      dns_packet, (size_t) nread) == 0) {
            udp_request = scanned_udp_request;
            break;
        }
    }
    if (udp_request == NULL) {
        logger(proxy_context, LOG_DEBUG,
               "Received a reply that doesn't match any active query");
        return;
    }
    if (nread < (ssize_t) (DNS_HEADER_SIZE + dnscrypt_response_header_size()) ||
        nread > (ssize_t) sizeof dns_packet) {
        udp_request_kill(udp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_REPLIED(udp_request);
    dns_packet_len = (size_t) nread;
    assert(dns_packet_len <= sizeof dns_packet);

    uncurved_len = dns_packet_len;
    DNSCRYPT_PROXY_REQUEST_UNCURVE_START(udp_request, uncurved_len);
    if (dnscrypt_client_uncurve
        (&udp_request->proxy_context->dnscrypt_client,
            udp_request->client_nonce, dns_packet, &uncurved_len) != 0) {
        DNSCRYPT_PROXY_REQUEST_UNCURVE_ERROR(udp_request);
        DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_GOT_INVALID_REPLY(udp_request);
        logger_noformat(udp_request->proxy_context, LOG_WARNING,
                        "Received a suspicious reply from the resolver");
        udp_request_kill(udp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UNCURVE_DONE(udp_request, uncurved_len);
    memset(udp_request->client_nonce, 0, sizeof udp_request->client_nonce);
    assert(uncurved_len <= dns_packet_len);
    dns_packet_len = uncurved_len;
    sendto_with_retry(& (SendtoWithRetryCtx) {
       .udp_request = udp_request,
       .handle = udp_request->client_proxy_handle,
       .buffer = dns_packet,
       .length = dns_packet_len,
       .flags = 0,
       .dest_addr = (struct sockaddr *) &udp_request->client_sockaddr,
       .dest_len = udp_request->client_sockaddr_len,
       .cb = udp_request_kill
    });
}

static void
proxy_client_send_truncated(UDPRequest * const udp_request,
                            uint8_t dns_packet[DNS_MAX_PACKET_SIZE_UDP],
                            size_t dns_packet_len)
{
    DNSCRYPT_PROXY_REQUEST_UDP_TRUNCATED(udp_request);

    assert(dns_packet_len > DNS_OFFSET_FLAGS2);
    dns_packet[DNS_OFFSET_FLAGS] |= DNS_FLAGS_TC | DNS_FLAGS_QR;
    dns_packet[DNS_OFFSET_FLAGS2] |= DNS_FLAGS2_RA;
    sendto_with_retry(& (SendtoWithRetryCtx) {
        .udp_request = udp_request,
        .handle = udp_request->client_proxy_handle,
        .buffer = dns_packet,
        .length = dns_packet_len,
        .flags = 0,
        .dest_addr = (struct sockaddr *) &udp_request->client_sockaddr,
        .dest_len = udp_request->client_sockaddr_len,
        .cb = udp_request_kill
    });
}

static void
timeout_timer_cb(evutil_socket_t timeout_timer_handle, short ev_flags,
                 void * const udp_request_)
{
    UDPRequest * const udp_request = udp_request_;

    (void) ev_flags;
    (void) timeout_timer_handle;
    DNSCRYPT_PROXY_REQUEST_UDP_TIMEOUT(udp_request);
    logger_noformat(udp_request->proxy_context, LOG_DEBUG,
                    "resolver timeout (UDP)");
    udp_request_kill(udp_request);
}

#ifndef SO_RCVBUFFORCE
# define SO_RCVBUFFORCE SO_RCVBUF
#endif
#ifndef SO_SNDBUFFORCE
# define SO_SNDBUFFORCE SO_SNDBUF
#endif

static void
udp_tune(evutil_socket_t const handle)
{
    if (handle == -1) {
        return;
    }
    setsockopt(handle, SOL_SOCKET, SO_RCVBUFFORCE,
               (void *) (int []) { UDP_BUFFER_SIZE }, sizeof (int));
    setsockopt(handle, SOL_SOCKET, SO_SNDBUFFORCE,
               (void *) (int []) { UDP_BUFFER_SIZE }, sizeof (int));
#if defined(IP_MTU_DISCOVER) && defined(IP_PMTUDISC_DONT)
    setsockopt(handle, IPPROTO_IP, IP_MTU_DISCOVER,
               (void *) (int []) { IP_PMTUDISC_DONT }, sizeof (int));
#elif defined(IP_DONTFRAG)
    setsockopt(handle, IPPROTO_IP, IP_DONTFRAG,
               (void *) (int []) { 0 }, sizeof (int));
#endif
}

static void
client_to_proxy_cb_sendto_cb(UDPRequest * const udp_request)
{
    (void) udp_request;
    DNSCRYPT_PROXY_REQUEST_UDP_PROXY_RESOLVER_START(udp_request);
}

static void
client_to_proxy_cb(evutil_socket_t client_proxy_handle, short ev_flags,
                   void * const proxy_context_)
{
    uint8_t       dns_packet[DNS_MAX_PACKET_SIZE_UDP];
    ProxyContext *proxy_context = proxy_context_;
    UDPRequest   *udp_request;
    ssize_t       curve_ret;
    ssize_t       nread;
    size_t        dns_packet_len = (size_t) 0U;
    size_t        max_packet_size;
    size_t        request_edns_payload_size;

    (void) ev_flags;
    assert(client_proxy_handle == proxy_context->udp_listener_handle);
    if ((udp_request = calloc((size_t) 1U, sizeof *udp_request)) == NULL) {
        return;
    }
    udp_request->proxy_context = proxy_context;
    udp_request->sendto_retry_timer = NULL;
    udp_request->timeout_timer = NULL;
    udp_request->client_proxy_handle = client_proxy_handle;
    udp_request->client_sockaddr_len = sizeof udp_request->client_sockaddr;
    nread = recvfrom(client_proxy_handle,
                     (void *) dns_packet, sizeof dns_packet, 0,
                     (struct sockaddr *) &udp_request->client_sockaddr,
                     &udp_request->client_sockaddr_len);
    if (nread < (ssize_t) 0) {
        const int err = evutil_socket_geterror(client_proxy_handle);
        logger(proxy_context, LOG_WARNING,
               "recvfrom(client): [%s]", evutil_socket_error_to_string(err));
        DNSCRYPT_PROXY_REQUEST_UDP_NETWORK_ERROR(udp_request);
        udp_request_kill(udp_request);
        return;
    }
    if (nread < (ssize_t) DNS_HEADER_SIZE ||
        (size_t) nread > sizeof dns_packet) {
        logger_noformat(proxy_context, LOG_WARNING, "Short query received");
        free(udp_request);
        return;
    }
    if (proxy_context->connections_count >=
        proxy_context->connections_count_max) {
        DNSCRYPT_PROXY_REQUEST_UDP_OVERLOADED();
        if (udp_listener_kill_oldest_request(proxy_context) != 0) {
            tcp_listener_kill_oldest_request(proxy_context);
        }
    }
    proxy_context->connections_count++;
    assert(proxy_context->connections_count
           <= proxy_context->connections_count_max);
    DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                          proxy_context->connections_count_max);
    DNSCRYPT_PROXY_REQUEST_UDP_START(udp_request);
    TAILQ_INSERT_TAIL(&proxy_context->udp_request_queue,
                      udp_request, queue);
    memset(&udp_request->status, 0, sizeof udp_request->status);
    udp_request->status.is_in_queue = 1;

    dns_packet_len = (size_t) nread;
    assert(dns_packet_len <= sizeof dns_packet);

    edns_add_section(proxy_context, dns_packet, &dns_packet_len,
                     sizeof dns_packet, &request_edns_payload_size);

    if (request_edns_payload_size < DNS_MAX_PACKET_SIZE_UDP_SEND) {
        max_packet_size = DNS_MAX_PACKET_SIZE_UDP_SEND;
    } else {
        max_packet_size = request_edns_payload_size;
    }
    if (max_packet_size > sizeof dns_packet) {
        max_packet_size = sizeof dns_packet;
    }
    assert(max_packet_size <= sizeof dns_packet);
    assert(SIZE_MAX - DNSCRYPT_MAX_PADDING - dnscrypt_query_header_size()
           > dns_packet_len);
    size_t max_len = dns_packet_len + DNSCRYPT_MAX_PADDING +
        dnscrypt_query_header_size();
    if (max_len > max_packet_size) {
        max_len = max_packet_size;
    }
    if (udp_request->proxy_context->tcp_only != 0 ||
        dns_packet_len + dnscrypt_query_header_size() > max_len) {
        proxy_client_send_truncated(udp_request, dns_packet, dns_packet_len);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_CURVE_START(udp_request, dns_packet_len);
    curve_ret =
        dnscrypt_client_curve(&udp_request->proxy_context->dnscrypt_client,
                              udp_request->client_nonce, dns_packet,
                              dns_packet_len, max_len);
    if (curve_ret <= (ssize_t) 0) {
        DNSCRYPT_PROXY_REQUEST_CURVE_ERROR(udp_request);
        return;
    }
    dns_packet_len = (size_t) curve_ret;
    assert(dns_packet_len >= dnscrypt_query_header_size());
    DNSCRYPT_PROXY_REQUEST_CURVE_DONE(udp_request, dns_packet_len);
    assert(dns_packet_len <= sizeof dns_packet);

    udp_request->timeout_timer =
        evtimer_new(udp_request->proxy_context->event_loop,
                    timeout_timer_cb, udp_request);
    if (udp_request->timeout_timer != NULL) {
        const struct timeval tv = {
            .tv_sec = (time_t) DNS_QUERY_TIMEOUT, .tv_usec = 0
        };
        evtimer_add(udp_request->timeout_timer, &tv);
    }
    sendto_with_retry(& (SendtoWithRetryCtx) {
        .udp_request = udp_request,
        .handle = proxy_context->udp_proxy_resolver_handle,
        .buffer = dns_packet,
        .length = dns_packet_len,
        .flags = 0,
        .dest_addr = (struct sockaddr *) &proxy_context->resolver_sockaddr,
        .dest_len = proxy_context->resolver_sockaddr_len,
        .cb = client_to_proxy_cb_sendto_cb
    });
}

int
udp_listener_kill_oldest_request(ProxyContext * const proxy_context)
{
    if (TAILQ_EMPTY(&proxy_context->udp_request_queue)) {
        return -1;
    }
    udp_request_kill(TAILQ_FIRST(&proxy_context->udp_request_queue));

    return 0;
}

int
udp_listener_bind(ProxyContext * const proxy_context)
{
    assert(proxy_context->udp_listener_handle == -1);
    if ((proxy_context->udp_listener_handle = socket
         (proxy_context->local_sockaddr.ss_family,
             SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        logger(NULL, LOG_ERR, "Unable to create a socket (UDP)");
        return -1;
    }
    evutil_make_socket_closeonexec(proxy_context->udp_listener_handle);
    evutil_make_socket_nonblocking(proxy_context->udp_listener_handle);
    if (bind(proxy_context->udp_listener_handle,
             (struct sockaddr *) &proxy_context->local_sockaddr,
             proxy_context->local_sockaddr_len) != 0) {
        logger(NULL, LOG_ERR, "Unable to bind (UDP) [%s]",
               evutil_socket_error_to_string
               (evutil_socket_geterror(proxy_context->udp_listener_handle)));
        evutil_closesocket(proxy_context->udp_listener_handle);
        proxy_context->udp_listener_handle = -1;
        return -1;
    }
    udp_tune(proxy_context->udp_listener_handle);

    if ((proxy_context->udp_proxy_resolver_handle = socket
         (proxy_context->resolver_sockaddr.ss_family, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Unable to create a socket to the resolver");
        evutil_closesocket(proxy_context->udp_listener_handle);
        proxy_context->udp_listener_handle = -1;
        return -1;
    }
    evutil_make_socket_closeonexec(proxy_context->udp_proxy_resolver_handle);
    evutil_make_socket_nonblocking(proxy_context->udp_proxy_resolver_handle);
    udp_tune(proxy_context->udp_proxy_resolver_handle);

    TAILQ_INIT(&proxy_context->udp_request_queue);

    return 0;
}

int
udp_listener_start(ProxyContext * const proxy_context)
{
    assert(proxy_context->udp_listener_handle != -1);
    if ((proxy_context->udp_listener_event =
         event_new(proxy_context->event_loop,
                   proxy_context->udp_listener_handle, EV_READ | EV_PERSIST,
                   client_to_proxy_cb, proxy_context)) == NULL) {
        return -1;
    }
    if (event_add(proxy_context->udp_listener_event, NULL) != 0) {
        udp_listener_stop(proxy_context);
        return -1;
    }

    assert(proxy_context->udp_proxy_resolver_handle != -1);
    if ((proxy_context->udp_proxy_resolver_event =
         event_new(proxy_context->event_loop,
                   proxy_context->udp_proxy_resolver_handle,
                   EV_READ | EV_PERSIST,
                   resolver_to_proxy_cb, proxy_context)) == NULL) {
        udp_listener_stop(proxy_context);
        return -1;
    }
    if (event_add(proxy_context->udp_proxy_resolver_event, NULL) != 0) {
        udp_listener_stop(proxy_context);
        return -1;
    }
    return 0;
}

void
udp_listener_stop(ProxyContext * const proxy_context)
{
    event_free(proxy_context->udp_proxy_resolver_event);
    proxy_context->udp_proxy_resolver_event = NULL;
    while (udp_listener_kill_oldest_request(proxy_context) != 0) { }
    logger_noformat(proxy_context, LOG_INFO, "UDP listener shut down");
}
