
#ifndef __CERT_H__
#define __CERT_H__ 1

#include "uv.h"

#define CERT_QUERY_RETRY_MIN_DELAY           (1 * 1000)
#define CERT_QUERY_RETRY_MAX_DELAY           (5 * 60 * 1000)
#define CERT_QUERY_RETRY_STEPS               100
#define CERT_QUERY_RETRY_DELAY_AFTER_SUCCESS (60 * 60 * 1000)

typedef struct CertUpdater_ {
    struct ares_options ar_options;
    ares_channel        ar_channel;
    uv_timer_t          cert_timer;
    unsigned int        query_retry_step;
    _Bool               has_cert_timer;
} CertUpdater;

struct ProxyContext_;
int cert_updater_init(struct ProxyContext_ * const proxy_context);
int cert_updater_start(struct ProxyContext_ * const proxy_context);
void cert_updater_stop(struct ProxyContext_ * const proxy_context);

#endif
