
#ifndef __UDP_REQUEST_H__
#define __UDP_REQUEST_H__ 1

#include "dnscrypt_proxy.h"

#ifndef UDP_BUFFER_SIZE
# define UDP_BUFFER_SIZE 2097152
#endif

int udp_listener_bind(ProxyContext * const proxy_context);
int udp_listener_start(ProxyContext * const proxy_context);
void udp_listener_stop(ProxyContext * const proxy_context);

#endif
