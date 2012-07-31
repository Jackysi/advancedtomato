
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
#endif

#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/util.h>

#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "logger.h"
#include "probes.h"
#include "tcp_request.h"
#include "tcp_request_p.h"
#include "udp_request.h"

static void
tcp_request_free(TCPRequest * const tcp_request)
{
    ProxyContext *proxy_context;

    if (tcp_request->timeout_timer != NULL) {
        event_free(tcp_request->timeout_timer);
        tcp_request->timeout_timer = NULL;
    }
    if (tcp_request->client_proxy_bev != NULL) {
        DNSCRYPT_PROXY_REQUEST_TCP_DONE(tcp_request);
        bufferevent_free(tcp_request->client_proxy_bev);
        tcp_request->client_proxy_bev = NULL;
    }
    if (tcp_request->proxy_resolver_bev != NULL) {
        DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_DONE(tcp_request);
        bufferevent_free(tcp_request->proxy_resolver_bev);
        tcp_request->proxy_resolver_bev = NULL;
    }
    if (tcp_request->proxy_resolver_query_evbuf != NULL) {
        evbuffer_free(tcp_request->proxy_resolver_query_evbuf);
        tcp_request->proxy_resolver_query_evbuf = NULL;
    }
    proxy_context = tcp_request->proxy_context;
    if (tcp_request->status.is_in_queue != 0) {
        assert(! TAILQ_EMPTY(&proxy_context->tcp_request_queue));
        TAILQ_REMOVE(&proxy_context->tcp_request_queue, tcp_request, queue);
        assert(proxy_context->connections_count > 0U);
        proxy_context->connections_count--;
        DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                              proxy_context->connections_count_max);
    }
    tcp_request->proxy_context = NULL;
    free(tcp_request);
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
tcp_tune(evutil_socket_t handle)
{
    if (handle == -1) {
        return;
    }
    setsockopt(handle, IPPROTO_TCP, TCP_NODELAY,
               (void *) (int []) { 1 }, sizeof (int));
}

static void
timeout_timer_cb(evutil_socket_t timeout_timer_handle, short ev_flags,
                 void * const tcp_request_)
{
    TCPRequest * const tcp_request = tcp_request_;

    (void) ev_flags;
    (void) timeout_timer_handle;
    DNSCRYPT_PROXY_REQUEST_TCP_TIMEOUT(tcp_request);
    logger_noformat(tcp_request->proxy_context, LOG_DEBUG,
                    "resolver timeout (TCP)");
    tcp_request_kill(tcp_request);
}

static void
proxy_resolver_event_cb(struct bufferevent * const proxy_resolver_bev,
                        const short events, void * const tcp_request_)
{
    TCPRequest * const tcp_request = tcp_request_;

    (void) proxy_resolver_bev;
    if ((events & BEV_EVENT_ERROR) != 0) {
        DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_NETWORK_ERROR(tcp_request);
        tcp_request_kill(tcp_request);
        return;
    }
    if ((events & BEV_EVENT_CONNECTED) == 0) {
        DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_CONNECTED(tcp_request);
        tcp_tune(bufferevent_getfd(proxy_resolver_bev));
        return;
    }
}

static void
resolver_proxy_read_cb(struct bufferevent * const proxy_resolver_bev,
                       void * const tcp_request_)
{
    uint8_t          dns_reply_len_buf[2];
    uint8_t          dns_uncurved_reply_len_buf[2];
    uint8_t         *dns_reply;
    TCPRequest      *tcp_request = tcp_request_;
    ProxyContext    *proxy_context = tcp_request->proxy_context;
    struct evbuffer *input = bufferevent_get_input(proxy_resolver_bev);
    size_t           available_size;
    size_t           uncurved_len;

    if (tcp_request->status.has_dns_reply_len == 0) {
        assert(evbuffer_get_length(input) >= (size_t) 2U);
        evbuffer_remove(input, dns_reply_len_buf, sizeof dns_reply_len_buf);
        tcp_request->dns_reply_len = (size_t)
            ((dns_reply_len_buf[0] << 8) | dns_reply_len_buf[1]);
        tcp_request->status.has_dns_reply_len = 1;
    }
    assert(tcp_request->status.has_dns_reply_len != 0);
    if (tcp_request->dns_reply_len <
        (size_t) DNS_HEADER_SIZE + dnscrypt_response_header_size()) {
        logger_noformat(proxy_context, LOG_WARNING, "Short reply received");
        DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_GOT_INVALID_REPLY(tcp_request);
        tcp_request_kill(tcp_request);
        return;
    }
    available_size = evbuffer_get_length(input);
    if (available_size < tcp_request->dns_reply_len) {
        bufferevent_setwatermark(tcp_request->proxy_resolver_bev,
                                 EV_READ, tcp_request->dns_reply_len,
                                 tcp_request->dns_reply_len);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_REPLIED(tcp_request);
    assert(available_size >= tcp_request->dns_reply_len);
    uncurved_len = tcp_request->dns_reply_len;
    dns_reply = evbuffer_pullup(input, (ssize_t) uncurved_len);
    if (dns_reply == NULL) {
        tcp_request_kill(tcp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UNCURVE_START(tcp_request, uncurved_len);
    if (dnscrypt_client_uncurve(&proxy_context->dnscrypt_client,
                                tcp_request->client_nonce,
                                dns_reply, &uncurved_len) != 0) {
        DNSCRYPT_PROXY_REQUEST_UNCURVE_ERROR(tcp_request);
        DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_GOT_INVALID_REPLY(tcp_request);
        logger_noformat(tcp_request->proxy_context, LOG_WARNING,
                        "Received a suspicious reply from the resolver");
        tcp_request_kill(tcp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_UNCURVE_DONE(tcp_request, uncurved_len);
    assert(uncurved_len <= tcp_request->dns_reply_len);
    dns_uncurved_reply_len_buf[0] = (uncurved_len >> 8) & 0xff;
    dns_uncurved_reply_len_buf[1] = uncurved_len & 0xff;
    if (bufferevent_write(tcp_request->client_proxy_bev,
                          dns_uncurved_reply_len_buf, (size_t) 2U) != 0 ||
        bufferevent_write(tcp_request->client_proxy_bev, dns_reply,
                          uncurved_len) != 0) {
        tcp_request_kill(tcp_request);
        return;
    }
    bufferevent_enable(tcp_request->client_proxy_bev, EV_WRITE);
    DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_DONE(tcp_request);
    bufferevent_free(tcp_request->proxy_resolver_bev);
    tcp_request->proxy_resolver_bev = NULL;
}

static void
client_proxy_event_cb(struct bufferevent * const client_proxy_bev,
                      const short events, void * const tcp_request_)
{
    TCPRequest * const tcp_request = tcp_request_;

    (void) client_proxy_bev;
    (void) events;
    tcp_request_kill(tcp_request);
}

static void
client_proxy_write_cb(struct bufferevent * const client_proxy_bev,
                      void * const tcp_request_)
{
    TCPRequest * const tcp_request = tcp_request_;

    (void) client_proxy_bev;
    DNSCRYPT_PROXY_REQUEST_TCP_REPLIED(tcp_request);
    tcp_request_kill(tcp_request);
}

static void
client_proxy_read_cb(struct bufferevent * const client_proxy_bev,
                     void * const tcp_request_)
{
    uint8_t          dns_query[DNS_MAX_PACKET_SIZE_TCP - 2U];
    uint8_t          dns_query_len_buf[2];
    uint8_t          dns_curved_query_len_buf[2];
    TCPRequest      *tcp_request = tcp_request_;
    ProxyContext    *proxy_context = tcp_request->proxy_context;
    struct evbuffer *input = bufferevent_get_input(client_proxy_bev);
    ssize_t          curve_ret;
    size_t           available_size;
    size_t           max_len;

    if (tcp_request->status.has_dns_query_len == 0) {
        assert(evbuffer_get_length(input) >= (size_t) 2U);
        evbuffer_remove(input, dns_query_len_buf, sizeof dns_query_len_buf);
        tcp_request->dns_query_len = (size_t)
            ((dns_query_len_buf[0] << 8) | dns_query_len_buf[1]);
        tcp_request->status.has_dns_query_len = 1;
    }
    assert(tcp_request->status.has_dns_query_len != 0);
    if (tcp_request->dns_query_len < (size_t) DNS_HEADER_SIZE) {
        logger_noformat(proxy_context, LOG_WARNING, "Short query received");
        tcp_request_kill(tcp_request);
        return;
    }
    available_size = evbuffer_get_length(input);
    if (available_size < tcp_request->dns_query_len) {
        bufferevent_setwatermark(tcp_request->client_proxy_bev,
                                 EV_READ, tcp_request->dns_query_len,
                                 tcp_request->dns_query_len);
        return;
    }
    assert(available_size >= tcp_request->dns_query_len);
    bufferevent_disable(tcp_request->client_proxy_bev, EV_READ);
    max_len = tcp_request->dns_query_len + DNSCRYPT_MAX_PADDING +
        dnscrypt_query_header_size();
    if (max_len > sizeof dns_query) {
        max_len = sizeof dns_query;
    }
    assert(max_len <= DNS_MAX_PACKET_SIZE_TCP - 2U);
    if (tcp_request->dns_query_len + dnscrypt_query_header_size() > max_len) {
        tcp_request_kill(tcp_request);
        return;
    }
    assert(tcp_request->proxy_resolver_query_evbuf == NULL);
    if ((tcp_request->proxy_resolver_query_evbuf = evbuffer_new()) == NULL) {
        tcp_request_kill(tcp_request);
        return;
    }
    if ((ssize_t)
        evbuffer_remove_buffer(input, tcp_request->proxy_resolver_query_evbuf,
                               tcp_request->dns_query_len)
        != (ssize_t) tcp_request->dns_query_len) {
        tcp_request_kill(tcp_request);
        return;
    }
    assert(tcp_request->dns_query_len <= sizeof dns_query);
    if ((ssize_t) evbuffer_remove(tcp_request->proxy_resolver_query_evbuf,
                                  dns_query, tcp_request->dns_query_len)
        != (ssize_t) tcp_request->dns_query_len) {
        tcp_request_kill(tcp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_CURVE_START(tcp_request,
                                       tcp_request->dns_query_len);
    assert(max_len <= sizeof dns_query);
    curve_ret =
        dnscrypt_client_curve(&proxy_context->dnscrypt_client,
                              tcp_request->client_nonce,
                              dns_query, tcp_request->dns_query_len, max_len);
    if (curve_ret <= (ssize_t) 0) {
        DNSCRYPT_PROXY_REQUEST_CURVE_ERROR(tcp_request);
        tcp_request_kill(tcp_request);
        return;
    }
    DNSCRYPT_PROXY_REQUEST_CURVE_DONE(tcp_request, (size_t) curve_ret);
    dns_curved_query_len_buf[0] = (curve_ret >> 8) & 0xff;
    dns_curved_query_len_buf[1] = curve_ret & 0xff;
    if (bufferevent_write(tcp_request->proxy_resolver_bev,
                          dns_curved_query_len_buf, (size_t) 2U) != 0 ||
        bufferevent_write(tcp_request->proxy_resolver_bev, dns_query,
                          (size_t) curve_ret) != 0) {
        tcp_request_kill(tcp_request);
        return;
    }
    bufferevent_enable(tcp_request->proxy_resolver_bev, EV_READ);
}

static void
tcp_connection_cb(struct evconnlistener * const tcp_conn_listener,
                  evutil_socket_t handle,
                  struct sockaddr * const client_sockaddr,
                  const int client_sockaddr_len_int,
                  void * const proxy_context_)
{
    ProxyContext *proxy_context = proxy_context_;
    TCPRequest   *tcp_request;

    (void) tcp_conn_listener;
    (void) client_sockaddr;
    (void) client_sockaddr_len_int;
    if ((tcp_request = calloc((size_t) 1U, sizeof *tcp_request)) == NULL) {
        return;
    }
    tcp_request->proxy_context = proxy_context;
    tcp_request->timeout_timer = NULL;
    tcp_request->proxy_resolver_query_evbuf = NULL;
    tcp_request->client_proxy_bev =
        bufferevent_socket_new(proxy_context->event_loop, handle,
                               BEV_OPT_CLOSE_ON_FREE);
    if (tcp_request->client_proxy_bev == NULL) {
        evutil_closesocket(handle);
        free(tcp_request);
        return;
    }
    tcp_request->proxy_resolver_bev = bufferevent_socket_new
        (proxy_context->event_loop, -1, BEV_OPT_CLOSE_ON_FREE);
    if (tcp_request->proxy_resolver_bev == NULL) {
        bufferevent_free(tcp_request->client_proxy_bev);
        tcp_request->client_proxy_bev = NULL;
        free(tcp_request);
        return;
    }
    if (proxy_context->connections_count >=
        proxy_context->connections_count_max) {
        DNSCRYPT_PROXY_REQUEST_TCP_OVERLOADED();
        if (tcp_listener_kill_oldest_request(proxy_context) != 0) {
            udp_listener_kill_oldest_request(proxy_context);
        }
    }
    proxy_context->connections_count++;
    assert(proxy_context->connections_count
           <= proxy_context->connections_count_max);
    DNSCRYPT_PROXY_STATUS_REQUESTS_ACTIVE(proxy_context->connections_count,
                                          proxy_context->connections_count_max);
    DNSCRYPT_PROXY_REQUEST_TCP_START(tcp_request);
    TAILQ_INSERT_TAIL(&proxy_context->tcp_request_queue,
                      tcp_request, queue);
    memset(&tcp_request->status, 0, sizeof tcp_request->status);
    tcp_request->status.is_in_queue = 1;
    if ((tcp_request->timeout_timer =
         evtimer_new(tcp_request->proxy_context->event_loop,
                     timeout_timer_cb, tcp_request)) == NULL) {
        tcp_request_kill(tcp_request);
        return;
    }
    const struct timeval tv = {
        .tv_sec = (time_t) DNS_QUERY_TIMEOUT, .tv_usec = 0
    };
    evtimer_add(tcp_request->timeout_timer, &tv);
    bufferevent_setwatermark(tcp_request->client_proxy_bev,
                             EV_READ, (size_t) 2U,
                             (size_t) DNS_MAX_PACKET_SIZE_TCP);
    bufferevent_setcb(tcp_request->client_proxy_bev,
                      client_proxy_read_cb, client_proxy_write_cb,
                      client_proxy_event_cb, tcp_request);
    if (bufferevent_socket_connect
        (tcp_request->proxy_resolver_bev,
            (struct sockaddr *) &proxy_context->resolver_sockaddr,
            (int) proxy_context->resolver_sockaddr_len) != 0) {
        tcp_request_kill(tcp_request);
        return;
    }
    bufferevent_setwatermark(tcp_request->proxy_resolver_bev,
                             EV_READ, (size_t) 2U,
                             (size_t) DNS_MAX_PACKET_SIZE_TCP);
    bufferevent_setcb(tcp_request->proxy_resolver_bev,
                      resolver_proxy_read_cb, NULL, proxy_resolver_event_cb,
                      tcp_request);
    DNSCRYPT_PROXY_REQUEST_TCP_PROXY_RESOLVER_START(tcp_request);
    bufferevent_enable(tcp_request->client_proxy_bev, EV_READ);
}

static void
tcp_accept_timer_cb(evutil_socket_t handle, const short event,
                    void * const proxy_context_)
{
    ProxyContext *proxy_context = proxy_context_;

    (void) handle;
    (void) event;
    event_free(proxy_context->tcp_accept_timer);
    proxy_context->tcp_accept_timer = NULL;
    evconnlistener_enable(proxy_context->tcp_conn_listener);
}

static void
tcp_accept_error_cb(struct evconnlistener * const tcp_conn_listener,
                    void * const proxy_context_)
{
    ProxyContext *proxy_context = proxy_context_;

    (void) tcp_conn_listener;
    DNSCRYPT_PROXY_REQUEST_TCP_NETWORK_ERROR(NULL);
    if (proxy_context->tcp_accept_timer == NULL) {
        proxy_context->tcp_accept_timer = evtimer_new
            (proxy_context->event_loop, tcp_accept_timer_cb, proxy_context);
        if (proxy_context->tcp_accept_timer == NULL) {
            return;
        }
    }
    if (evtimer_pending(proxy_context->tcp_accept_timer, NULL)) {
        return;
    }
    evconnlistener_disable(proxy_context->tcp_conn_listener);

    const struct timeval tv = { .tv_sec = (time_t) 1, .tv_usec = 0 };
    evtimer_add(proxy_context->tcp_accept_timer, &tv);
}

int
tcp_listener_kill_oldest_request(ProxyContext * const proxy_context)
{
    if (TAILQ_EMPTY(&proxy_context->tcp_request_queue)) {
        return -1;
    }
    tcp_request_kill(TAILQ_FIRST(&proxy_context->tcp_request_queue));

    return 0;
}

int
tcp_listener_bind(ProxyContext * const proxy_context)
{
    assert(proxy_context->tcp_conn_listener == NULL);
#ifndef LEV_OPT_DEFERRED_ACCEPT
# define LEV_OPT_DEFERRED_ACCEPT 0
#endif
    proxy_context->tcp_conn_listener =
        evconnlistener_new_bind(proxy_context->event_loop,
                                tcp_connection_cb, proxy_context,
                                LEV_OPT_CLOSE_ON_FREE |
                                LEV_OPT_CLOSE_ON_EXEC |
                                LEV_OPT_REUSEABLE |
                                LEV_OPT_DEFERRED_ACCEPT,
                                TCP_REQUEST_BACKLOG,
                                (struct sockaddr *)
                                &proxy_context->local_sockaddr,
                                (int) proxy_context->local_sockaddr_len);
    if (proxy_context->tcp_conn_listener == NULL) {
        logger(NULL, LOG_ERR, "Unable to bind (TCP)");
        return -1;
    }
    if (evconnlistener_disable(proxy_context->tcp_conn_listener) != 0) {
        evconnlistener_free(proxy_context->tcp_conn_listener);
        proxy_context->tcp_conn_listener = NULL;
        return -1;
    }
    evconnlistener_set_error_cb(proxy_context->tcp_conn_listener,
                                tcp_accept_error_cb);
    TAILQ_INIT(&proxy_context->tcp_request_queue);

    return 0;
}

int
tcp_listener_start(ProxyContext * const proxy_context)
{
    assert(proxy_context->tcp_conn_listener != NULL);
    if (evconnlistener_enable(proxy_context->tcp_conn_listener) != 0) {
        return -1;
    }
    return 0;
}

void
tcp_listener_stop(ProxyContext * const proxy_context)
{
    evconnlistener_free(proxy_context->tcp_conn_listener);
    proxy_context->tcp_conn_listener = NULL;
    while (tcp_listener_kill_oldest_request(proxy_context) != 0) { }
    logger_noformat(proxy_context, LOG_INFO, "TCP listener shut down");
}
