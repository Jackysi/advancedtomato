
#ifndef __UV_ALLOC_H__
#define __UV_ALLOC_H__ 1

#include <stdlib.h>

#include "dnscrypt_proxy.h"

int uv_alloc_init(ProxyContext * const proxy_context);

void uv_alloc_free(ProxyContext * const proxy_context);

uv_buf_t uv_alloc_get_buffer(ProxyContext * const proxy_context,
                             const size_t size);

#define uv_alloc_release_buffer(CONTEXT, BUF) do { } while(0)

#endif
