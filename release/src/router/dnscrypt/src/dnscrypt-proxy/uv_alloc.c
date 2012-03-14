
#include <config.h>
#include <stdlib.h>

#include "dnscrypt_proxy.h"
#include "uv.h"
#include "uv_alloc.h"

int
uv_alloc_init(ProxyContext * const proxy_context)
{
    proxy_context->uv_alloc_buffer = NULL;
    proxy_context->uv_alloc_buffer_size = (size_t) 0U;

    return 0;
}

void
uv_alloc_free(ProxyContext * const proxy_context)
{
    free(proxy_context->uv_alloc_buffer);
    proxy_context->uv_alloc_buffer = NULL;
    proxy_context->uv_alloc_buffer_size = (size_t) 0U;
}

uv_buf_t
uv_alloc_get_buffer(ProxyContext * const proxy_context, const size_t size)
{
    void *tmp_uv_alloc_buffer;

    if (proxy_context->uv_alloc_buffer != NULL &&
        proxy_context->uv_alloc_buffer_size > size) {
        return (uv_buf_t) { .base = proxy_context->uv_alloc_buffer,
                            .len  = size };
    }
    if ((tmp_uv_alloc_buffer =
         realloc(proxy_context->uv_alloc_buffer, size)) == NULL) {
        free(proxy_context->uv_alloc_buffer);
        proxy_context->uv_alloc_buffer = NULL;
        proxy_context->uv_alloc_buffer_size = (size_t) 0U;

        return (uv_buf_t) { .base = NULL, .len  = (size_t) 0U };
    }
    proxy_context->uv_alloc_buffer = tmp_uv_alloc_buffer;
    proxy_context->uv_alloc_buffer_size = size;

    return (uv_buf_t) { .base = proxy_context->uv_alloc_buffer,
                        .len  = size };
}
