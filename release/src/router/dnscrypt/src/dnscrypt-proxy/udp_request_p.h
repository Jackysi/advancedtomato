
#ifndef __UDP_REQUEST_H_P__
#define __UDP_REQUEST_H_P__ 1

#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <stdint.h>

#include "dnscrypt.h"
#include "uv.h"

typedef struct UDPRequestStatus_ {
    _Bool is_dying : 1;
    _Bool has_proxy_resolver_handle : 1;
    _Bool has_timeout_timer : 1;
} UDPRequestStatus;

typedef struct UDPRequest_ {
    ngx_queue_t             *prev, *next;

    uint8_t                  dns_packet[DNS_MAX_PACKET_SIZE];
    uint8_t                  client_nonce[crypto_box_HALF_NONCEBYTES];
    struct sockaddr_storage  client_addr;
    socklen_t                client_addr_len;
    uv_udp_t                 proxy_resolver_handle;
    uv_udp_send_t            proxy_to_resolver_send_query;
    uv_udp_send_t            proxy_to_client_send_query;
    uv_timer_t               timeout_timer;
    uv_udp_t                *client_proxy_handle_p;
    ProxyContext            *proxy_context;
    UDPRequestStatus         status;
    size_t                   dns_packet_len;
} UDPRequest;

#endif
