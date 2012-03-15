
#ifndef __DNSCRYPT_PROXY_H__
#define __DNSCRYPT_PROXY_H__ 1

#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <stdint.h>

#include "app.h"
#include "cert.h"
#include "crypto_box.h"
#include "crypto_sign_ed25519.h"
#include "dnscrypt_client.h"
#include "uv.h"

#ifdef _WIN32
# include "uv-private/ngx-queue.h"
#endif

#ifndef DNS_QUERY_TIMEOUT
# define DNS_QUERY_TIMEOUT (10 * 1000)
#endif

#define DNS_MAX_PACKET_SIZE_UDP_RECV (65535U - 20U - 8U)
#define DNS_MAX_PACKET_SIZE_UDP_SEND 512U

#if DNS_MAX_PACKET_SIZE_UDP_RECV > NS_MAX_PACKET_SIZE_UDP_SEND
# define DNS_MAX_PACKET_SIZE DNS_MAX_PACKET_SIZE_UDP_RECV
#else
# define DNS_MAX_PACKET_SIZE DNS_MAX_PACKET_SIZE_UDP_SEND
#endif

#ifndef DNS_DEFAULT_PORT
# define DNS_DEFAULT_PORT 53U
#endif

#define DNS_HEADER_SIZE  12U
#define DNS_FLAGS_TC      2U
#define DNS_FLAGS_QR    128U
#define DNS_FLAGS2_RA   128U

#define DNS_CLASS_IN      1U
#define DNS_TYPE_TXT     16U
#define DNS_TYPE_OPT     41U

#define DNS_OFFSET_QUESTION DNS_HEADER_SIZE
#define DNS_OFFSET_FLAGS    2U
#define DNS_OFFSET_FLAGS2   3U
#define DNS_OFFSET_QDCOUNT  4U
#define DNS_OFFSET_ANCOUNT  6U
#define DNS_OFFSET_NSCOUNT  8U
#define DNS_OFFSET_ARCOUNT 10U

#define DNS_OFFSET_EDNS_TYPE         0U
#define DNS_OFFSET_EDNS_PAYLOAD_SIZE 2U

#define DNS_DEFAULT_EDNS_PAYLOAD_SIZE 1280U

typedef struct ProxyContext_ {
    uint8_t                  dnscrypt_magic_query[DNSCRYPT_MAGIC_QUERY_LEN];
    uint8_t                  provider_publickey[crypto_sign_ed25519_PUBLICKEYBYTES];
    uint8_t                  resolver_publickey[crypto_box_PUBLICKEYBYTES];
    DNSCryptClient           dnscrypt_client;
    CertUpdater              cert_updater;
    struct sockaddr_storage  resolver_addr;
    socklen_t                resolver_addr_len;
    uv_tcp_t                 tcp_listener_handle;
    uv_udp_t                 udp_listener_handle;
    ngx_queue_t              tcp_request_queue;
    ngx_queue_t              udp_request_queue;
    AppContext              *app_context;
    uv_loop_t               *event_loop;
    const char              *listen_ip;
    const char              *log_file;
    const char              *pid_file;
    const char              *provider_name;
    const char              *provider_publickey_s;
    const char              *resolver_ip;
    char                    *user_dir;
    void                    *uv_alloc_buffer;
    size_t                   uv_alloc_buffer_size;
    size_t                   edns_payload_size;
#ifndef _WIN32
    uid_t                    user_id;
    gid_t                    user_group;
#endif
    unsigned int             connections_count;
    unsigned int             connections_count_max;
    int                      log_fd;
    uint16_t                 local_port;
    uint16_t                 resolver_port;
    _Bool                    daemonize;
    _Bool                    listeners_started;
    _Bool                    tcp_only;
} ProxyContext;

int dnscrypt_proxy_start_listeners(ProxyContext * const proxy_context);

#endif
