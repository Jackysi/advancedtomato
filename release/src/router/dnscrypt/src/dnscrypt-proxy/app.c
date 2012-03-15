
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "app.h"
#include "dnscrypt_client.h"
#include "dnscrypt_proxy.h"
#include "salsa20_random.h"
#include "logger.h"
#include "options.h"
#include "stack_trace.h"
#include "tcp_request.h"
#include "udp_request.h"
#include "uv.h"
#include "uv_alloc.h"

static AppContext app_context;

static int
proxy_context_init(ProxyContext * const proxy_context, int argc, char *argv[])
{
    struct sockaddr_in resolver_addr;

    memset(proxy_context, 0, sizeof *proxy_context);
    options_parse(&app_context, proxy_context, argc, argv);
    proxy_context->event_loop = uv_loop_new();
    resolver_addr = uv_ip4_addr(proxy_context->resolver_ip,
                                proxy_context->resolver_port);
    proxy_context->resolver_addr_len = sizeof(struct sockaddr_in);
    memcpy(&proxy_context->resolver_addr, &resolver_addr,
           proxy_context->resolver_addr_len);
    uv_alloc_init(proxy_context);

    return 0;
}

static void
proxy_context_free(ProxyContext * const proxy_context)
{
    if (proxy_context == NULL) {
        return;
    }
    uv_alloc_free(proxy_context);
    options_free(proxy_context);
    logger_close(proxy_context);
}

static
int init_tz(void)
{
    static char  default_tz_for_putenv[] = "TZ=UTC+00:00";
    char         stbuf[10U];
    struct tm   *tm;
    time_t       now;

    tzset();
    time(&now);
    if ((tm = localtime(&now)) != NULL &&
        strftime(stbuf, sizeof stbuf, "%z", tm) == (size_t) 5U) {
        snprintf(default_tz_for_putenv, sizeof default_tz_for_putenv,
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
#ifndef DEBUG
    salsa20_random_stir();
    init_tz();
    (void) strerror(ENOENT);
# ifndef _WIN32
    if (proxy_context->user_dir != NULL) {
        if (chdir(proxy_context->user_dir) != 0 ||
            chroot(proxy_context->user_dir) != 0 || chdir("/") != 0) {
            logger(proxy_context, LOG_ERR, "Unable to chroot to [%s]",
                   proxy_context->user_dir);
            exit(1);
        }
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
    if (proxy_context->listeners_started != 0) {
        return 0;
    }
    if (tcp_listener_start(proxy_context) != 0 ||
        udp_listener_start(proxy_context) != 0) {
        exit(1);
    }
    logger(proxy_context, LOG_INFO,
           PACKAGE " is ready: proxying from [%s] to [%s]",
           proxy_context->listen_ip, proxy_context->resolver_ip);

    proxy_context->listeners_started = 1;

    return 0;
}

int
main(int argc, char *argv[])
{
    ProxyContext  proxy_context;

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    stack_trace_on_crash();
    proxy_context_init(&proxy_context, argc, argv);
    app_context.proxy_context = &proxy_context;
    logger_noformat(&proxy_context, LOG_INFO, "Generating a new key pair");
    dnscrypt_client_init_with_new_key_pair(&proxy_context.dnscrypt_client);
    logger_noformat(&proxy_context, LOG_INFO, "Done");

    if (cert_updater_init(&proxy_context) != 0 ||
        tcp_listener_bind(&proxy_context) != 0 ||
        udp_listener_bind(&proxy_context) != 0) {
        exit(1);
    }
#ifdef SIGPIPE
    signal(SIGPIPE, SIG_IGN);
#endif
    revoke_privileges(&proxy_context);

    if (cert_updater_start(&proxy_context) != 0) {
        exit(1);
    }
    uv_run(proxy_context.event_loop);

    logger_noformat(&proxy_context, LOG_INFO, "Stopping proxy");
    cert_updater_stop(&proxy_context);
    tcp_listener_stop(&proxy_context);
    udp_listener_stop(&proxy_context);
    uv_loop_delete(proxy_context.event_loop);
    proxy_context_free(&proxy_context);
    app_context.proxy_context = NULL;
    salsa20_random_close();

    return 0;
}
