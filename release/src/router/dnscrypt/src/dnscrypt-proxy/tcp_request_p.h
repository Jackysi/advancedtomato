
#ifndef __TCP_REQUEST_H_P__
#define __TCP_REQUEST_H_P__ 1

#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <stdint.h>

#include "dnscrypt.h"
#include "uv.h"

#define TCP_MAX_PACKET_SIZE 0xffff

typedef struct TCPRequestStatus_ {
    _Bool is_dying : 1;
    _Bool has_client_proxy_handle : 1;
    _Bool has_proxy_resolver_handle : 1;
    _Bool has_timeout_timer : 1;
    _Bool has_expected_size : 1;
    _Bool has_expected_size_partial : 1;
} TCPRequestStatus;

typedef struct TCPRequest_ {
    ngx_queue_t             *prev, *next;

    uint8_t                  dns_packet[TCP_MAX_PACKET_SIZE];
    uint8_t                  client_nonce[crypto_box_HALF_NONCEBYTES];
    struct sockaddr_storage  client_addr;
    socklen_t                client_addr_len;
    uv_tcp_t                 client_proxy_handle;
    uv_tcp_t                 proxy_resolver_handle;
    uv_connect_t             proxy_to_resolver_connect_query;
    uv_write_t               proxy_to_resolver_send_query;
    uv_write_t               proxy_to_client_send_query;
    uv_timer_t               timeout_timer;
    ProxyContext            *proxy_context;
    TCPRequestStatus         status;
    size_t                   dns_packet_expected_len;
    size_t                   dns_packet_len;
    uint16_t                 dns_packet_nlen;
} TCPRequest;

#endif
