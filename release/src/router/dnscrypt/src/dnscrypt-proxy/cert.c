
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
# include <arpa/inet.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cert.h"
#include "cert_p.h"
#include "crypto_sign_ed25519.h"
#include "dnscrypt_proxy.h"
#include "logger.h"
#include "probes.h"
#include "utils.h"

static int
cert_parse_version(ProxyContext * const proxy_context,
                   const SignedBincert * const signed_bincert,
                   const size_t signed_bincert_len)
{
    if (signed_bincert_len <= (size_t) (signed_bincert->signed_data -
                                        signed_bincert->magic_cert) ||
        memcmp(signed_bincert->magic_cert, CERT_MAGIC_CERT,
               sizeof signed_bincert->magic_cert) != 0) {
        logger_noformat(proxy_context, LOG_DEBUG,
                        "TXT record with no certificates received");
        return -1;
    }
    if (signed_bincert->version_major[0] != 0U ||
        signed_bincert->version_major[1] != 1U) {
        logger_noformat(proxy_context, LOG_WARNING,
                        "Unsupported certificate version");
        return -1;
    }
    return 0;
}

static int
cert_parse_bincert(ProxyContext * const proxy_context,
                   const Bincert * const bincert,
                   const Bincert * const previous_bincert)
{
    uint32_t serial;
    memcpy(&serial, bincert->serial, sizeof serial);
    serial = htonl(serial);
    logger(proxy_context, LOG_INFO, "Server certificate #%lu received",
           (unsigned long) serial);

    uint32_t ts_begin;
    memcpy(&ts_begin, bincert->ts_begin, sizeof ts_begin);
    ts_begin = htonl(ts_begin);

    uint32_t ts_end;
    memcpy(&ts_end, bincert->ts_end, sizeof ts_end);
    ts_end = htonl(ts_end);

    uint32_t now_u32 = (uint32_t) time(NULL);

    if (now_u32 < ts_begin) {
        logger_noformat(proxy_context, LOG_INFO,
                        "This certificate has not been activated yet");
        return -1;
    }
    if (now_u32 > ts_end) {
        logger_noformat(proxy_context, LOG_INFO,
                        "This certificate has expired");
        return -1;
    }
    logger_noformat(proxy_context, LOG_INFO, "This certificate looks valid");
    if (previous_bincert == NULL) {
        return 0;
    }

    uint32_t previous_serial;
    memcpy(&previous_serial, previous_bincert->serial, sizeof previous_serial);
    previous_serial = htonl(previous_serial);
    if (previous_serial > serial) {
        logger(proxy_context, LOG_INFO,
               "Certificate #%lu has been superseded by certificate #%lu",
               (unsigned long) previous_serial, (unsigned long) serial);
        return -1;
    }
    logger(proxy_context, LOG_INFO,
           "This certificates supersedes certificate #%lu",
           (unsigned long) previous_serial);
    return 0;
}

static int
cert_open_bincert(ProxyContext * const proxy_context,
                  const SignedBincert * const signed_bincert,
                  const size_t signed_bincert_len,
                  Bincert ** const bincert_p)
{
    Bincert            *bincert;
    unsigned long long  bincert_data_len_ul;
    size_t              bincert_size;
    size_t              signed_data_len;

    if (cert_parse_version(proxy_context,
                           signed_bincert, signed_bincert_len) != 0) {
        DNSCRYPT_PROXY_CERTS_UPDATE_ERROR_COMMUNICATION();
        return -1;
    }
    bincert_size = signed_bincert_len;
    if ((bincert = malloc(bincert_size)) == NULL) {
        DNSCRYPT_PROXY_CERTS_UPDATE_ERROR_COMMUNICATION();
        return -1;
    }
    assert(signed_bincert_len >= (size_t) (signed_bincert->signed_data -
                                           signed_bincert->magic_cert));
    signed_data_len = signed_bincert_len -
        (size_t) (signed_bincert->signed_data - signed_bincert->magic_cert);
    assert(bincert_size - (size_t) (bincert->server_publickey -
                                    bincert->magic_cert) == signed_data_len);
    if (crypto_sign_ed25519_open(bincert->server_publickey, &bincert_data_len_ul,
                                 signed_bincert->signed_data, signed_data_len,
                                 proxy_context->provider_publickey) != 0) {
        free(bincert);
        logger_noformat(proxy_context, LOG_ERR,
                        "Suspicious certificate received");
        DNSCRYPT_PROXY_CERTS_UPDATE_ERROR_SECURITY();
        return -1;
    }
    if (cert_parse_bincert(proxy_context, bincert, *bincert_p) != 0) {
        memset(bincert, 0, sizeof *bincert);
        free(bincert);
        return -1;
    }
    if (*bincert_p != NULL) {
        memset(*bincert_p, 0, sizeof **bincert_p);
        free(*bincert_p);
    }
    *bincert_p = bincert;

    return 0;
}

static void
cert_print_server_key(ProxyContext * const proxy_context)
{
    char fingerprint[80U];

    dnscrypt_key_to_fingerprint(fingerprint,
                                proxy_context->resolver_publickey);
    logger(proxy_context, LOG_INFO,
           "Server key fingerprint is %s", fingerprint);
}

static void
cert_timer_cb(uv_timer_t *handle, int status)
{
    ProxyContext * const proxy_context = handle->data;
    CertUpdater  * const cert_updater = &proxy_context->cert_updater;

    (void) status;
    cert_updater->has_cert_timer = 0;
    logger_noformat(proxy_context, LOG_INFO,
                    "Refetching server certificates");
    cert_updater_start(proxy_context);
}

static void
cert_reschedule_query(ProxyContext * const proxy_context,
                      const int64_t query_retry_delay)
{
    CertUpdater *cert_updater = &proxy_context->cert_updater;

    if (cert_updater->has_cert_timer != 0) {
        return;
    }
    uv_timer_init(proxy_context->event_loop, &cert_updater->cert_timer);
    cert_updater->has_cert_timer = 1;
    cert_updater->cert_timer.data = proxy_context;
    uv_timer_start(&cert_updater->cert_timer, cert_timer_cb,
                   query_retry_delay, (int64_t) 0);
}

static void
cert_reschedule_query_after_failure(ProxyContext * const proxy_context)
{
    CertUpdater *cert_updater = &proxy_context->cert_updater;
    int64_t      query_retry_delay;

    if (cert_updater->has_cert_timer != 0) {
        return;
    }
    query_retry_delay = (int64_t)
        (CERT_QUERY_RETRY_MIN_DELAY +
            (int64_t) cert_updater->query_retry_step *
            (CERT_QUERY_RETRY_MAX_DELAY - CERT_QUERY_RETRY_MIN_DELAY) /
            CERT_QUERY_RETRY_STEPS);
    if (cert_updater->query_retry_step < CERT_QUERY_RETRY_STEPS) {
        cert_updater->query_retry_step++;
    }
    cert_reschedule_query(proxy_context, query_retry_delay);
    DNSCRYPT_PROXY_CERTS_UPDATE_RETRY();
}

static void
cert_reschedule_query_after_success(ProxyContext * const proxy_context)
{
    if (proxy_context->cert_updater.has_cert_timer != 0) {
        return;
    }
    cert_reschedule_query(proxy_context,
                          (int64_t) CERT_QUERY_RETRY_DELAY_AFTER_SUCCESS);
}

static void
cert_query_cb(void *arg, int status, int timeouts, unsigned char *abuf,
              int alen)
{
    Bincert               *bincert = NULL;
    ProxyContext          *proxy_context = arg;
    struct ares_txt_reply *txt_out;
    struct ares_txt_reply *txt_out_current;

    (void) timeouts;
    DNSCRYPT_PROXY_CERTS_UPDATE_RECEIVED();
    if (status != ARES_SUCCESS ||
        ares_parse_txt_reply(abuf, alen, &txt_out) != ARES_SUCCESS) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Unable to retrieve server certificates");
        cert_reschedule_query_after_failure(proxy_context);
        DNSCRYPT_PROXY_CERTS_UPDATE_ERROR_COMMUNICATION();
        return;
    }
    txt_out_current = txt_out;
    while (txt_out_current != NULL) {
        cert_open_bincert(proxy_context,
                          (const SignedBincert *) txt_out_current->txt,
                          txt_out_current->length, &bincert);
        txt_out_current = txt_out_current->next;
    }
    ares_free_data(txt_out);
    if (bincert == NULL) {
        logger_noformat(proxy_context, LOG_ERR,
                        "No useable certificates found");
        cert_reschedule_query_after_failure(proxy_context);
        DNSCRYPT_PROXY_CERTS_UPDATE_ERROR_NOCERTS();
        return;
    }
    COMPILER_ASSERT(sizeof proxy_context->resolver_publickey ==
                    sizeof bincert->server_publickey);
    memcpy(proxy_context->resolver_publickey, bincert->server_publickey,
           sizeof proxy_context->resolver_publickey);
    COMPILER_ASSERT(sizeof proxy_context->dnscrypt_magic_query ==
                    sizeof bincert->magic_query);
    memcpy(proxy_context->dnscrypt_magic_query, bincert->magic_query,
           sizeof proxy_context->dnscrypt_magic_query);
    cert_print_server_key(proxy_context);
    dnscrypt_client_init_magic_query(&proxy_context->dnscrypt_client,
                                     bincert->magic_query);
    memset(bincert, 0, sizeof *bincert);
    free(bincert);
    dnscrypt_client_init_nmkey(&proxy_context->dnscrypt_client,
                               proxy_context->resolver_publickey);
    dnscrypt_proxy_start_listeners(proxy_context);
    proxy_context->cert_updater.query_retry_step = 0U;
    cert_reschedule_query_after_success(proxy_context);
    DNSCRYPT_PROXY_CERTS_UPDATE_DONE((unsigned char *)
                                     proxy_context->resolver_publickey);
}

int
cert_updater_init(ProxyContext * const proxy_context)
{
    CertUpdater *cert_updater = &proxy_context->cert_updater;
    int          ar_options_mask;

    memset(cert_updater, 0, sizeof *cert_updater);
    if (ares_library_init(ARES_LIB_INIT_ALL) != ARES_SUCCESS) {
        return -1;
    }
    assert(proxy_context->event_loop != NULL);
    cert_updater->has_cert_timer = 0;
    cert_updater->query_retry_step = 0U;
    ar_options_mask = ARES_OPT_SERVERS;
    cert_updater->ar_options.nservers = 1;
    cert_updater->ar_options.servers =
        &((struct sockaddr_in *) &proxy_context->resolver_addr)->sin_addr;
    if (proxy_context->tcp_only) {
        ar_options_mask |= ARES_OPT_FLAGS | ARES_OPT_TCP_PORT;
        cert_updater->ar_options.flags = ARES_FLAG_USEVC;
        cert_updater->ar_options.tcp_port =
            htons(proxy_context->resolver_port);
    }
    if (uv_ares_init_options(proxy_context->event_loop,
                             &cert_updater->ar_channel,
                             &cert_updater->ar_options,
                             ar_options_mask) != ARES_SUCCESS) {
        return -1;
    }
    return 0;
}

int
cert_updater_start(ProxyContext * const proxy_context)
{
    CertUpdater *cert_updater = &proxy_context->cert_updater;

    DNSCRYPT_PROXY_CERTS_UPDATE_START();
    ares_query(cert_updater->ar_channel, proxy_context->provider_name,
               DNS_CLASS_IN, DNS_TYPE_TXT, cert_query_cb, proxy_context);
    return 0;
}

void
cert_updater_stop(ProxyContext * const proxy_context)
{
    CertUpdater * const cert_updater = &proxy_context->cert_updater;

    if (cert_updater->has_cert_timer) {
        cert_updater->has_cert_timer = 0;
        uv_timer_stop(&cert_updater->cert_timer);
    }
    uv_ares_destroy(proxy_context->event_loop, cert_updater->ar_channel);
    ares_destroy_options(&cert_updater->ar_options);
}
