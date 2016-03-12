
#include <config.h>
#include <sys/types.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/util.h>

#include <sodium.h>

#ifdef HAVE_LIBSYSTEMD
# include <sys/socket.h>
# include <systemd/sd-daemon.h>
#endif

#include "app.h"
#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "logger.h"
#include "options.h"
#include "sandboxes.h"
#include "stack_trace.h"
#include "tcp_request.h"
#include "udp_request.h"
#ifdef PLUGINS
# include "plugin_support.h"
#endif

#ifndef INET6_ADDRSTRLEN
# define INET6_ADDRSTRLEN 46U
#endif

static AppContext            app_context;
static volatile sig_atomic_t skip_dispatch;

static int
sockaddr_from_ip_and_port(struct sockaddr_storage * const sockaddr,
                          ev_socklen_t * const sockaddr_len_p,
                          const char * const ip, const char * const port,
                          const char * const error_msg)
{
    char   sockaddr_port[INET6_ADDRSTRLEN + sizeof "[]:65535"];
    int    sockaddr_len_int;
    char  *pnt;
    _Bool  has_column = 0;
    _Bool  has_columns = 0;
    _Bool  has_brackets = *ip == '[';

    if ((pnt = strchr(ip, ':')) != NULL) {
        has_column = 1;
        if (strchr(pnt + 1, ':') != NULL) {
            has_columns = 1;
        }
    }
    sockaddr_len_int = (int) sizeof *sockaddr;
    if ((has_brackets != 0 || has_column != has_columns) &&
        evutil_parse_sockaddr_port(ip, (struct sockaddr *) sockaddr,
                                   &sockaddr_len_int) == 0) {
        *sockaddr_len_p = (ev_socklen_t) sockaddr_len_int;
        return 0;
    }
    if (has_columns != 0 && has_brackets == 0) {
        evutil_snprintf(sockaddr_port, sizeof sockaddr_port, "[%s]:%s",
                        ip, port);
    } else {
        evutil_snprintf(sockaddr_port, sizeof sockaddr_port, "%s:%s",
                        ip, port);
    }
    sockaddr_len_int = (int) sizeof *sockaddr;
    if (evutil_parse_sockaddr_port(sockaddr_port, (struct sockaddr *) sockaddr,
                                   &sockaddr_len_int) != 0) {
        logger(NULL, LOG_ERR, "%s: %s", error_msg, sockaddr_port);
        *sockaddr_len_p = (ev_socklen_t) 0U;

        return -1;
    }
    *sockaddr_len_p = (ev_socklen_t) sockaddr_len_int;

    return 0;
}

static int
proxy_context_init(ProxyContext * const proxy_context, int argc, char *argv[])
{
    memset(proxy_context, 0, sizeof *proxy_context);
    proxy_context->event_loop = NULL;
    proxy_context->log_file = NULL;
    proxy_context->max_log_level = LOG_INFO;
    proxy_context->tcp_accept_timer = NULL;
    proxy_context->tcp_conn_listener = NULL;
    proxy_context->udp_current_max_size = DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND;
    proxy_context->udp_max_size = (size_t) DNS_DEFAULT_EDNS_PAYLOAD_SIZE;
    proxy_context->udp_listener_event = NULL;
    proxy_context->udp_proxy_resolver_event = NULL;
    proxy_context->udp_proxy_resolver_handle = -1;
    proxy_context->udp_listener_handle = -1;
    proxy_context->tcp_listener_handle = -1;
    sodium_mlock(&proxy_context->dnscrypt_client,
                 sizeof proxy_context->dnscrypt_client);
    if (options_parse(&app_context, proxy_context, argc, argv) != 0) {
        return -1;
    }
#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
    if ((proxy_context->event_loop = event_base_new()) == NULL) {
        logger(NULL, LOG_ERR, "Unable to initialize the event loop");
        return -1;
    }
    if (sockaddr_from_ip_and_port(&proxy_context->resolver_sockaddr,
                                  &proxy_context->resolver_sockaddr_len,
                                  proxy_context->resolver_ip,
                                  DNS_DEFAULT_RESOLVER_PORT,
                                  "Unsupported resolver address") != 0) {
        return -1;
    }
    if (sockaddr_from_ip_and_port(&proxy_context->local_sockaddr,
                                  &proxy_context->local_sockaddr_len,
                                  proxy_context->local_ip,
                                  DNS_DEFAULT_LOCAL_PORT,
                                  "Unsupported local address") != 0) {
        return -1;
    }
    return 0;
}

static void
proxy_context_free(ProxyContext * const proxy_context)
{
    if (proxy_context == NULL) {
        return;
    }
    options_free(proxy_context);
    logger_close(proxy_context);
}

static int
init_locale(void)
{
    setlocale(LC_CTYPE, "C");
    setlocale(LC_COLLATE, "C");

    return 0;
}

static int
init_tz(void)
{
    static char  default_tz_for_putenv[] = "TZ=UTC+00:00";
    char         stbuf[10U];
    struct tm   *tm;
    time_t       now;

    tzset();
    time(&now);
    if ((tm = localtime(&now)) != NULL &&
        strftime(stbuf, sizeof stbuf, "%z", tm) == (size_t) 5U) {
        evutil_snprintf(default_tz_for_putenv, sizeof default_tz_for_putenv,
                        "TZ=UTC%c%c%c:%c%c", (*stbuf == '-' ? '+' : '-'),
                        stbuf[1], stbuf[2], stbuf[3], stbuf[4]);
    }
    putenv(default_tz_for_putenv);
    (void) localtime(&now);
    (void) gmtime(&now);

    return 0;
}

static void
revoke_privileges(ProxyContext * const proxy_context)
{
    (void) proxy_context;

    init_locale();
    init_tz();
    (void) strerror(ENOENT);
#ifndef DEBUG
    randombytes_stir();
# ifndef _WIN32
    if (proxy_context->user_dir != NULL) {
        if (chdir(proxy_context->user_dir) != 0 ||
            chroot(proxy_context->user_dir) != 0 || chdir("/") != 0) {
            logger(proxy_context, LOG_ERR, "Unable to chroot to [%s]",
                   proxy_context->user_dir);
            exit(1);
        }
    }
    if (sandboxes_app() != 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Unable to sandbox the main process");
        exit(1);
    }
    if (proxy_context->user_id != (uid_t) 0) {
        if (setgid(proxy_context->user_group) != 0 ||
            setegid(proxy_context->user_group) != 0 ||
            setuid(proxy_context->user_id) != 0 ||
            seteuid(proxy_context->user_id) != 0) {
            logger(proxy_context, LOG_ERR, "Unable to switch to user id [%lu]",
                   (unsigned long) proxy_context->user_id);
            exit(1);
        }
    }
# endif
#endif
}

int
dnscrypt_proxy_start_listeners(ProxyContext * const proxy_context)
{
    char local_addr_s[INET6_ADDRSTRLEN + sizeof "[]:65535"];
    char resolver_addr_s[INET6_ADDRSTRLEN + sizeof "[]:65535"];

    if (proxy_context->listeners_started != 0) {
        return 0;
    }
    if (udp_listener_start(proxy_context) != 0 ||
        tcp_listener_start(proxy_context) != 0) {
        exit(1);
    }
    evutil_format_sockaddr_port((const struct sockaddr *)
                                &proxy_context->local_sockaddr,
                                local_addr_s, sizeof local_addr_s);
    evutil_format_sockaddr_port((const struct sockaddr *)
                                &proxy_context->resolver_sockaddr,
                                resolver_addr_s, sizeof resolver_addr_s);
    logger(proxy_context, LOG_NOTICE, "Proxying from %s to %s",
           local_addr_s, resolver_addr_s);
    proxy_context->listeners_started = 1;
    systemd_notify(proxy_context, "READY=1");
    return 0;
}

int
dnscrypt_proxy_loop_break(void)
{
    if (app_context.proxy_context != NULL &&
        app_context.proxy_context->event_loop != NULL) {
        event_base_loopbreak(app_context.proxy_context->event_loop);
    } else {
        skip_dispatch = 1;
    }
    return 0;
}

#ifdef HAVE_LIBSYSTEMD
static int
init_descriptors_from_systemd(ProxyContext * const proxy_context)
{
    int num_sd_fds;
    int sock;

    num_sd_fds = sd_listen_fds(0);
    if (num_sd_fds == 0) {
        return 0;
    }
    if (num_sd_fds != 2) {
        logger(proxy_context, LOG_ERR, "Wrong number of systemd sockets: %d - "
               "should be 2", num_sd_fds);
        return -1;
    }
    assert(num_sd_fds <= INT_MAX - SD_LISTEN_FDS_START);
    for (sock = SD_LISTEN_FDS_START; sock < SD_LISTEN_FDS_START + num_sd_fds;
         ++sock) {
       if (sd_is_socket(sock, AF_INET, SOCK_DGRAM, 0) > 0 ||
           sd_is_socket(sock, AF_INET6, SOCK_DGRAM, 0) > 0) {
           proxy_context->udp_listener_handle = sock;
       }
       if (sd_is_socket(sock, AF_INET, SOCK_STREAM, 1) > 0 ||
           sd_is_socket(sock, AF_INET6, SOCK_STREAM, 1) > 0) {
           proxy_context->tcp_listener_handle = sock;
       }
    }
    if (proxy_context->udp_listener_handle < 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "No systemd UDP socket passed in");
        return -1;
    }
    if (proxy_context->tcp_listener_handle < 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "No systemd TCP socket passed in");
        return -1;
    }
    if (getsockname(proxy_context->udp_listener_handle,
                    (struct sockaddr *) &proxy_context->local_sockaddr,
                    &proxy_context->local_sockaddr_len) != 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Unable to get the local systemd UDP socket address");
        return -1;
    }
    return 0;
}
#endif

int
dnscrypt_proxy_main(int argc, char *argv[])
{
    ProxyContext proxy_context;

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    stack_trace_on_crash();
    if (sodium_init() != 0) {
        exit(1);
    }
#ifdef PLUGINS
    if ((app_context.dcps_context = plugin_support_context_new()) == NULL) {
        logger_noformat(NULL, LOG_ERR, "Unable to setup plugin support");
        exit(2);
    }
#endif
    if (proxy_context_init(&proxy_context, argc, argv) != 0) {
        logger_noformat(NULL, LOG_ERR, "Unable to start the proxy");
        exit(1);
    }
    logger_noformat(&proxy_context, LOG_NOTICE, "Starting " PACKAGE_STRING);
    sodium_mlock(&proxy_context, sizeof proxy_context);
    randombytes_set_implementation(&randombytes_salsa20_implementation);
#ifdef PLUGINS
    if (plugin_support_context_load(app_context.dcps_context) != 0) {
        logger_noformat(NULL, LOG_ERR, "Unable to load plugins");
        exit(2);
    }
#endif
    app_context.proxy_context = &proxy_context;
    proxy_context.dnscrypt_client.ephemeral_keys =
        proxy_context.ephemeral_keys;
    if (proxy_context.dnscrypt_client.ephemeral_keys != 0) {
        logger_noformat(&proxy_context, LOG_INFO, "Ephemeral keys enabled - generating a new seed");
        dnscrypt_client_init_with_new_session_key(&proxy_context.dnscrypt_client);
    } else if (proxy_context.client_key_file != NULL) {
        logger_noformat(&proxy_context, LOG_INFO, "Using a user-supplied client secret key");
        dnscrypt_client_init_with_client_key(&proxy_context.dnscrypt_client);
    } else {
        logger_noformat(&proxy_context, LOG_INFO, "Generating a new session key pair");
        dnscrypt_client_init_with_new_key_pair(&proxy_context.dnscrypt_client);
    }
    logger_noformat(&proxy_context, LOG_INFO, "Done");

    if (cert_updater_init(&proxy_context) != 0) {
        exit(1);
    }
#ifdef HAVE_LIBSYSTEMD
    if (init_descriptors_from_systemd(&proxy_context) != 0) {
        exit(1);
    }
#endif
    if (proxy_context.test_only == 0 &&
        (udp_listener_bind(&proxy_context) != 0 ||
         tcp_listener_bind(&proxy_context) != 0)) {
        exit(1);
    }
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif

    revoke_privileges(&proxy_context);
    if (cert_updater_start(&proxy_context) != 0) {
        exit(1);
    }

#ifdef HAVE_LIBSYSTEMD
    sd_notifyf(0, "MAINPID=%lu", (unsigned long) getpid());
#endif
    if (skip_dispatch == 0) {
        event_base_dispatch(proxy_context.event_loop);
    }
    logger_noformat(&proxy_context, LOG_NOTICE, "Stopping proxy");
    systemd_notify(0, "STOPPING=1");

    cert_updater_free(&proxy_context);
    udp_listener_stop(&proxy_context);
    tcp_listener_stop(&proxy_context);
    event_base_free(proxy_context.event_loop);
#ifdef PLUGINS
    plugin_support_context_free(app_context.dcps_context);
#endif
    proxy_context_free(&proxy_context);
    sodium_munlock(&proxy_context, sizeof proxy_context);
    app_context.proxy_context = NULL;
    randombytes_close();

    return 0;
}
