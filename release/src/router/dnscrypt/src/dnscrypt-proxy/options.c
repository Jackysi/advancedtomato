
#include <config.h>
#include <sys/types.h>
#ifdef _WIN32
# include <winsock2.h>
#else
# include <sys/socket.h>
#endif

#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#ifndef _WIN32
# include <pwd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dnscrypt_proxy.h"
#include "options.h"
#include "logger.h"
#include "pid_file.h"
#include "utils.h"

static struct option getopt_long_options[] = {
    { "local-address", 1, NULL, 'a' },
#ifndef _WIN32
    { "daemonize", 0, NULL, 'd' },
#endif
    { "edns-payload-size", 1, NULL, 'e' },
    { "help", 0, NULL, 'h' },
    { "provider-key", 1, NULL, 'k' },
#ifndef _WIN32
    { "logfile", 1, NULL, 'l' },
#endif
    { "max-active-requests", 1, NULL, 'n' },
#ifndef _WIN32
    { "pidfile", 1, NULL, 'p' },
#endif
    { "resolver-address", 1, NULL, 'r' },
    { "tcp-port", 1, NULL, 't' },
    { "user", 1, NULL, 'u' },
    { "provider-name", 1, NULL, 'N' },
    { "local-port", 1, NULL, 'P' },
    { "version", 0, NULL, 'V' },
    { NULL, 0, NULL, 0 }
};
#ifndef _WIN32
static const char   *getopt_options = "a:de:hk:l:n:p:r:t:u:N:P:V";
#else
static const char   *getopt_options = "a:e:hk:n:r:t:u:N:P:V";
#endif

#ifndef DEFAULT_CONNECTIONS_COUNT_MAX
# define DEFAULT_CONNECTIONS_COUNT_MAX 250U
#endif

#ifndef DEFAULT_PROVIDER_PUBLICKEY
# define DEFAULT_PROVIDER_PUBLICKEY \
    "B735:1140:206F:225D:3E2B:D822:D7FD:691E:" \
    "A1C3:3CC8:D666:8D0C:BE04:BFAB:CA43:FB79"
#endif
#ifndef DEFAULT_PROVIDER_NAME
# define DEFAULT_PROVIDER_NAME "2.dnscrypt-cert.opendns.com."
#endif
#ifndef DEFAULT_RESOLVER_IP
# define DEFAULT_RESOLVER_IP "208.67.220.220"
#endif

static void
options_version(void)
{
    puts(PACKAGE_STRING "\n"
        "Copyright (C) 2011-2012 OpenDNS, Inc.");
}

static void
options_usage(void)
{
    const struct option *options = getopt_long_options;

    options_version();
    puts("\nOptions:\n");
    do {
        printf("  -%c\t--%s%s\n", options->val, options->name,
               options->has_arg ? "=..." : "");
        options++;
    } while (options->name != NULL);
    puts("\nPlease consult the dnscrypt-proxy(8) man page for details.\n");
}

static
void options_init_with_default(AppContext * const app_context,
                               ProxyContext * const proxy_context)
{
    *proxy_context = (ProxyContext) {
        .app_context = app_context,
        .connections_count = 0U,
        .connections_count_max = DEFAULT_CONNECTIONS_COUNT_MAX,
        .edns_payload_size = (size_t) DNS_DEFAULT_EDNS_PAYLOAD_SIZE,
        .listen_ip = "127.0.0.1",
        .local_port = DNS_DEFAULT_PORT,
        .log_fd = -1,
        .log_file = NULL,
        .pid_file = NULL,
        .provider_name = DEFAULT_PROVIDER_NAME,
        .provider_publickey_s = DEFAULT_PROVIDER_PUBLICKEY,
        .resolver_ip = DEFAULT_RESOLVER_IP,
        .resolver_port = DNS_DEFAULT_PORT,
#ifndef _WIN32
        .user_id = (uid_t) 0,
        .user_group = (uid_t) 0,
#endif
        .user_dir = NULL,
        .daemonize = 0,
        .tcp_only = 0
    };
}

static int
options_apply(ProxyContext * const proxy_context)
{
    if (proxy_context->resolver_ip == NULL) {
        options_usage();
        exit(1);
    }
    if (proxy_context->provider_name == NULL ||
        *proxy_context->provider_name == 0) {
        logger_noformat(proxy_context, LOG_ERR, "Provider name required");
        exit(1);
    }
    if (proxy_context->provider_publickey_s == NULL) {
        logger_noformat(proxy_context, LOG_ERR, "Provider key required");
        exit(1);
    }
    if (dnscrypt_fingerprint_to_key(proxy_context->provider_publickey_s,
                                    proxy_context->provider_publickey) != 0) {
        logger_noformat(proxy_context, LOG_ERR, "Invalid provider key");
        exit(1);
    }
    if (proxy_context->daemonize) {
        do_daemonize();
    }
#ifndef _WIN32
    if (proxy_context->pid_file != NULL &&
        pid_file_create(proxy_context->pid_file,
                        proxy_context->user_id != (uid_t) 0) != 0) {
        logger_error(proxy_context, "Unable to create pid file");
    }
#endif
    if (proxy_context->log_file != NULL &&
        (proxy_context->log_fd = open(proxy_context->log_file,
                                      O_WRONLY | O_APPEND | O_CREAT,
                                      (mode_t) 0600)) == -1) {
        logger_error(proxy_context, "Unable to open log file");
        exit(1);
    }
    if (proxy_context->log_fd == -1 && proxy_context->daemonize) {
        logger_open_syslog(proxy_context);
    }
    return 0;
}

int
options_parse(AppContext * const app_context,
              ProxyContext * const proxy_context, int argc, char *argv[])
{
    int   opt_flag;
    int   option_index = 0;

    options_init_with_default(app_context, proxy_context);
    while ((opt_flag = getopt_long(argc, argv,
                                   getopt_options, getopt_long_options,
                                   &option_index)) != -1) {
        switch (opt_flag) {
        case 'a':
            proxy_context->listen_ip = optarg;
            break;
        case 'd':
            proxy_context->daemonize = 1;
            break;
        case 'e': {
            char *endptr;
            const unsigned long edns_payload_size = strtoul(optarg, &endptr, 10);

            if (*optarg == 0 || *endptr != 0 ||
                edns_payload_size > DNS_MAX_PACKET_SIZE_UDP_RECV) {
                logger(proxy_context, LOG_ERR,
                       "Invalid EDNS payload size: [%s]", optarg);
                exit(1);
            }
            if (edns_payload_size <= DNS_MAX_PACKET_SIZE_UDP_SEND) {
                proxy_context->edns_payload_size = (size_t) 0U;
            } else {
                proxy_context->edns_payload_size = (size_t) edns_payload_size;
            }
            break;
        }
        case 'h':
            options_usage();
            exit(0);
        case 'k':
            proxy_context->provider_publickey_s = optarg;
            break;
        case 'l':
            proxy_context->log_file = optarg;
            break;
        case 'n': {
            char *endptr;
            const unsigned long connections_count_max =
                strtoul(optarg, &endptr, 10);

            if (*optarg == 0 || *endptr != 0 ||
                connections_count_max <= 0U ||
                connections_count_max > UINT_MAX) {
                logger(proxy_context, LOG_ERR,
                       "Invalid max number of active request: [%s]", optarg);
                exit(1);
            }
            proxy_context->connections_count_max =
                (unsigned int) connections_count_max;
            break;
        }
        case 'p':
            proxy_context->pid_file = optarg;
            break;
        case 'r':
            proxy_context->resolver_ip = optarg;
            break;
        case 't': {
            char *endptr;
            const unsigned long port = strtoul(optarg, &endptr, 10);

            if (*optarg == 0 || *endptr != 0 || port <= 0UL || port > 65535UL) {
                logger(proxy_context, LOG_ERR, "Invalid TCP port: [%s]",
                        optarg);
                exit(1);
            }
            proxy_context->resolver_port = (uint16_t) port;
            proxy_context->tcp_only = 1;
            break;
        }
#ifndef _WIN32
        case 'u': {
            const struct passwd * const pw = getpwnam(optarg);
            if (pw == NULL) {
                logger(proxy_context, LOG_ERR, "Unknown user: [%s]", optarg);
                exit(1);
            }
            proxy_context->user_id = pw->pw_uid;
            proxy_context->user_group = pw->pw_gid;
            proxy_context->user_dir = strdup(pw->pw_dir);
            break;
        }
#endif
        case 'N':
            proxy_context->provider_name = optarg;
            break;
        case 'P': {
            char *endptr;
            const unsigned long port = strtoul(optarg, &endptr, 10);

            if (*optarg == 0 || *endptr != 0 || port <= 0UL || port > 65535UL) {
                logger(proxy_context, LOG_ERR, "Invalid local port: [%s]",
                        optarg);
                exit(1);
            }
            proxy_context->local_port = (uint16_t) port;
            break;
        }
        case 'V':
            options_version();
            exit(0);
        default:
            options_usage();
            exit(0);
        }
    }
    options_apply(proxy_context);

    return 0;
}

void
options_free(ProxyContext * const proxy_context)
{
    free(proxy_context->user_dir);
    proxy_context->user_dir = NULL;
}
