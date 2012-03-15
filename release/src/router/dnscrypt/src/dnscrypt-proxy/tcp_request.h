
#ifndef __TCP_REQUEST_H__
#define __TCP_REQUEST_H__ 1

#include "dnscrypt_proxy.h"

#define TCP_BUFFER_SIZE     65536

#ifndef TCP_REQUEST_BACKLOG
# define TCP_REQUEST_BACKLOG 128
#endif

int tcp_listener_bind(ProxyContext * const proxy_context);
int tcp_listener_start(ProxyContext * const proxy_context);
void tcp_listener_stop(ProxyContext * const proxy_context);

#endif
