
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
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event2/util.h>

#include "dnscrypt_proxy.h"
#include "getpwnam.h"
#include "options.h"
#include "logger.h"
#include "minicsv.h"
#include "pid_file.h"
#include "utils.h"
#include "windows_service.h"
#ifdef PLUGINS
# include "plugin_options.h"
#endif

static struct option getopt_long_options[] = {
    { "local-address", 1, NULL, 'a' },
#ifndef _WIN32
    { "daemonize", 0, NULL, 'd' },
#endif
    { "edns-payload-size", 1, NULL, 'e' },
    { "help", 0, NULL, 'h' },
    { "resolvers-list", 1, NULL, 'L' },
    { "resolver-name", 1, NULL, 'R' },
#ifndef _WIN32
    { "logfile", 1, NULL, 'l' },
#endif
    { "loglevel", 1, NULL, 'm' },
    { "max-active-requests", 1, NULL, 'n' },
#ifndef _WIN32
    { "pidfile", 1, NULL, 'p' },
#endif
    { "plugin", 1, NULL, 'X' },
    { "provider-name", 1, NULL, 'N' },
    { "provider-key", 1, NULL, 'k' },
    { "resolver-address", 1, NULL, 'r' },
    { "user", 1, NULL, 'u' },
    { "test", 1, NULL, 't' },
    { "tcp-only", 0, NULL, 'T' },
    { "version", 0, NULL, 'V' },
#ifdef _WIN32
    { "install", 0, NULL, WIN_OPTION_INSTALL },
    { "reinstall", 0, NULL, WIN_OPTION_REINSTALL },
    { "uninstall", 0, NULL, WIN_OPTION_UNINSTALL },
#endif
    { NULL, 0, NULL, 0 }
};
#ifndef _WIN32
static const char *getopt_options = "a:de:hk:L:l:m:n:p:r:R:t:u:N:TVX";
#else
static const char *getopt_options = "a:e:hk:L:m:n:r:R:t:u:N:TVX";
#endif

#ifndef DEFAULT_CONNECTIONS_COUNT_MAX
# define DEFAULT_CONNECTIONS_COUNT_MAX 250U
#endif

static void
options_version(void)
{
    puts(PACKAGE_STRING);
}

static void
options_usage(void)
{
    const struct option *options = getopt_long_options;

    options_version();
    puts("\nOptions:\n");
    do {
        if (options->val < 256) {
            printf("  -%c\t--%s%s\n", options->val, options->name,
                   options->has_arg ? "=..." : "");
        } else {
            printf("    \t--%s%s\n", options->name,
                   options->has_arg ? "=..." : "");
        }
        options++;
    } while (options->name != NULL);
    puts("\nPlease consult the dnscrypt-proxy(8) man page for details.\n");
}

static
void options_init_with_default(AppContext * const app_context,
                               ProxyContext * const proxy_context)
{
    assert(proxy_context->event_loop == NULL);
    proxy_context->app_context = app_context;
    proxy_context->connections_count = 0U;
    proxy_context->connections_count_max = DEFAULT_CONNECTIONS_COUNT_MAX;
    proxy_context->edns_payload_size = (size_t) DNS_DEFAULT_EDNS_PAYLOAD_SIZE;
    proxy_context->local_ip = "127.0.0.1:53";
    proxy_context->log_fd = -1;
    proxy_context->log_file = NULL;
    proxy_context->pid_file = NULL;
    proxy_context->resolvers_list = DEFAULT_RESOLVERS_LIST;
    proxy_context->resolver_name = DEFAULT_RESOLVER_NAME;
    proxy_context->provider_name = NULL;
    proxy_context->provider_publickey_s = NULL;
    proxy_context->resolver_ip = NULL;
#ifndef _WIN32
    proxy_context->user_id = (uid_t) 0;
    proxy_context->user_group = (uid_t) 0;
#endif
    proxy_context->user_dir = NULL;
    proxy_context->daemonize = 0;
    proxy_context->test_cert_margin = (time_t) -1;
    proxy_context->test_only = 0;
    proxy_context->tcp_only = 0;
}

static int
options_check_protocol_versions(const char * const provider_name)
{
    const size_t dnscrypt_protocol_versions_len =
        sizeof DNSCRYPT_PROTOCOL_VERSIONS - (size_t) 1U;

    if (strncmp(provider_name, DNSCRYPT_PROTOCOL_VERSIONS,
                dnscrypt_protocol_versions_len) != 0 ||
        provider_name[dnscrypt_protocol_versions_len] != '.') {
        return -1;
    }
    return 0;
}

static char *
options_read_file(const char * const file_name)
{
    FILE   *fp;
    char   *file_buf;
    size_t  file_size = (size_t) 0U;

    assert(file_name != NULL);
    if ((fp = fopen(file_name, "rb")) == NULL) {
        return NULL;
    }
    while (fgetc(fp) != EOF && file_size < SIZE_MAX) {
        file_size++;
    }
    if (feof(fp) == 0 || file_size <= (size_t) 0U) {
        fclose(fp);
        return NULL;
    }
    rewind(fp);
    if ((file_buf = malloc(file_size)) == NULL) {
        fclose(fp);
        return NULL;
    }
    if (fread(file_buf, file_size, (size_t) 1U, fp) != 1U) {
        fclose(fp);
        free(file_buf);
        return NULL;
    }
    (void) fclose(fp);

    return file_buf;
}

static const char *
options_get_col(char * const * const headers, const size_t headers_count,
                char * const * const cols, const size_t cols_count,
                const char * const header)
{
    size_t i = (size_t) 0U;

    while (i < headers_count) {
        if (strcmp(header, headers[i]) == 0) {
            if (i < cols_count) {
                return cols[i];
            }
            break;
        }
        i++;
    }
    return NULL;
}

static int
options_parse_resolver(ProxyContext * const proxy_context,
                       char * const * const headers, const size_t headers_count,
                       char * const * const cols, const size_t cols_count)
{
    const char *provider_name;
    const char *provider_publickey_s;
    const char *resolver_ip;
    const char *resolver_name;

    resolver_name = options_get_col(headers, headers_count,
                                    cols, cols_count, "Name");
    if (evutil_ascii_strcasecmp(resolver_name,
                                proxy_context->resolver_name) != 0) {
        return 0;
    }
    provider_name = options_get_col(headers, headers_count,
                                    cols, cols_count, "Provider name");
    provider_publickey_s = options_get_col(headers, headers_count,
                                           cols, cols_count,
                                           "Provider public key");
    resolver_ip = options_get_col(headers, headers_count,
                                  cols, cols_count, "Resolver address");
    if (provider_name == NULL || *provider_name == 0) {
        logger(proxy_context, LOG_ERR,
               "Resolvers list is missing a provider name for [%s]",
               resolver_name);
        return -1;
    }
    if (provider_publickey_s == NULL || *provider_publickey_s == 0) {
        logger(proxy_context, LOG_ERR,
               "Resolvers list is missing a public key for [%s]",
               resolver_name);
        return -1;
    }
    if (resolver_ip == NULL || *resolver_ip == 0) {
        logger(proxy_context, LOG_ERR,
               "Resolvers list is missing a resolver address for [%s]",
               resolver_name);
        return -1;
    }
    proxy_context->provider_name = strdup(provider_name);
    proxy_context->provider_publickey_s = strdup(provider_publickey_s);
    proxy_context->resolver_ip = strdup(resolver_ip);
    if (proxy_context->provider_name == NULL ||
        proxy_context->provider_publickey_s == NULL ||
        proxy_context->resolver_ip == NULL) {
        logger_noformat(proxy_context, LOG_EMERG, "Out of memory");
        exit(1);
    }
    return 1;
}

static int
options_parse_resolvers_list(ProxyContext * const proxy_context, char *buf)
{
    char   *cols[OPTIONS_RESOLVERS_LIST_MAX_COLS];
    char   *headers[OPTIONS_RESOLVERS_LIST_MAX_COLS];
    size_t  cols_count;
    size_t  headers_count;

    assert(proxy_context->resolver_name != NULL);
    buf = minicsv_parse_line(buf, headers, &headers_count,
                             sizeof headers / sizeof headers[0]);
    if (headers_count < 4U) {
        return -1;
    }
    do {
        buf = minicsv_parse_line(buf, cols, &cols_count,
                                 sizeof cols / sizeof cols[0]);
        minicsv_trim_cols(cols, cols_count);
        if (cols_count < 4U || *cols[0] == 0 || *cols[0] == '#') {
            continue;
        }
        if (options_parse_resolver(proxy_context, headers, headers_count,
                                   cols, cols_count) > 0) {
            return 0;
        }
    } while (*buf != 0);

    return -1;
}

static int
options_use_resolver_name(ProxyContext * const proxy_context)
{
    char *file_buf;

    file_buf = options_read_file(proxy_context->resolvers_list);
    if (file_buf == NULL) {
        logger(proxy_context, LOG_ERR, "Unable to read [%s]",
               proxy_context->resolvers_list);
        exit(1);
    }
    assert(proxy_context->resolver_name != NULL);
    if (options_parse_resolvers_list(proxy_context, file_buf) < 0) {
        logger(proxy_context, LOG_ERR,
               "No resolver named [%s] found in the [%s] list",
               proxy_context->resolver_name, proxy_context->resolvers_list);
    }
    free(file_buf);

    return 0;
}

static int
options_apply(ProxyContext * const proxy_context)
{
    if (proxy_context->resolver_name != NULL) {
        if (proxy_context->resolvers_list == NULL) {
            logger_noformat(proxy_context, LOG_ERR,
                            "Resolvers list (-L command-line switch) required");
            exit(1);
        }
        if (options_use_resolver_name(proxy_context) != 0) {
            logger(proxy_context, LOG_ERR,
                   "Resolver name (-R command-line switch) required. "
                   "See [%s] for a list of public resolvers.",
                   proxy_context->resolvers_list);
            exit(1);
        }
    }
    if (proxy_context->resolver_ip == NULL ||
        *proxy_context->resolver_ip == 0 ||
        proxy_context->provider_name == NULL ||
        *proxy_context->provider_name == 0 ||
        proxy_context->provider_publickey_s == NULL ||
        *proxy_context->provider_publickey_s == 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Resolver information required.");
        logger_noformat(proxy_context, LOG_ERR,
                        "The easiest way to do so is to provide a resolver name.");
        logger_noformat(proxy_context, LOG_ERR,
                        "Example: dnscrypt-proxy -R mydnsprovider");
        logger(proxy_context, LOG_ERR,
               "See the file [%s] for a list of compatible public resolvers",
               proxy_context->resolvers_list);
        logger_noformat(proxy_context, LOG_ERR,
                        "The name is the first column in this table.");
        logger_noformat(proxy_context, LOG_ERR,
                        "Alternatively, an IP address, a provider name "
                        "and a provider key can be supplied.");
#ifdef _WIN32
        logger_noformat(proxy_context, LOG_ERR,
                        "Consult http://dnscrypt.org "
                        "and https://github.com/jedisct1/dnscrypt-proxy/blob/master/README-WINDOWS.markdown "
                        "for details.");
#else
        logger_noformat(proxy_context, LOG_ERR,
                        "Please consult http://dnscrypt.org "
                        "and the dnscrypt-proxy(8) man page for details.");
#endif
        exit(1);
    }
    if (proxy_context->provider_name == NULL ||
        *proxy_context->provider_name == 0) {
        logger_noformat(proxy_context, LOG_ERR, "Provider name required");
        exit(1);
    }
    if (options_check_protocol_versions(proxy_context->provider_name) != 0) {
        logger_noformat(proxy_context, LOG_ERR,
                        "Unsupported server protocol version");
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
#ifdef _WIN32
    _Bool option_install = 0;
#endif

    options_init_with_default(app_context, proxy_context);
    while ((opt_flag = getopt_long(argc, argv,
                                   getopt_options, getopt_long_options,
                                   &option_index)) != -1) {
        switch (opt_flag) {
        case 'a':
            proxy_context->local_ip = optarg;
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
            if (edns_payload_size <= DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND) {
                proxy_context->edns_payload_size = (size_t) 0U;
            } else {
                proxy_context->edns_payload_size = (size_t) edns_payload_size;
                assert(proxy_context->udp_max_size ==
                       DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND);
                if (proxy_context->edns_payload_size > DNS_MAX_PACKET_SIZE_UDP_NO_EDNS_SEND) {
                    proxy_context->udp_max_size =
                        proxy_context->edns_payload_size;
                }
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
        case 'L':
            proxy_context->resolvers_list = optarg;
            break;
        case 'R':
            proxy_context->resolver_name = optarg;
            break;
        case 'm': {
            char *endptr;
            const long max_log_level = strtol(optarg, &endptr, 10);

            if (*optarg == 0 || *endptr != 0 || max_log_level < 0) {
                logger(proxy_context, LOG_ERR,
                       "Invalid max log level: [%s]", optarg);
                exit(1);
            }
            proxy_context->max_log_level = max_log_level;
            break;
        }
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
            const unsigned long margin =
                strtoul(optarg, &endptr, 10);

            if (*optarg == 0 || *endptr != 0 ||
                margin > UINT32_MAX / 60U) {
                logger(proxy_context, LOG_ERR,
                       "Invalid certificate grace period: [%s]", optarg);
                exit(1);
            }
            proxy_context->test_cert_margin = (time_t) margin * (time_t) 60U;
            proxy_context->test_only = 1;
            break;
        }
#ifdef HAVE_GETPWNAM
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
        case 'T':
            proxy_context->tcp_only = 1;
            break;
        case 'V':
            options_version();
            exit(0);
        case 'X':
#ifndef PLUGINS
            logger_noformat(proxy_context, LOG_ERR,
                            "Support for plugins hasn't been compiled in");
            exit(1);
#else
            if (plugin_options_parse_str
                (proxy_context->app_context->dcps_context, optarg) != 0) {
                logger_noformat(proxy_context, LOG_ERR,
                                "Error while parsing plugin options");
                exit(2);
            }
#endif
            break;
#ifdef _WIN32
        case WIN_OPTION_INSTALL:
        case WIN_OPTION_REINSTALL:
            option_install = 1;
            break;
        case WIN_OPTION_UNINSTALL:
            if (windows_service_uninstall() != 0) {
                logger_noformat(NULL, LOG_ERR, "Unable to uninstall the service");
                exit(1);
            } else {
                logger_noformat(NULL, LOG_INFO, "The " WINDOWS_SERVICE_NAME
                                " service has been removed from this system");
                exit(0);
            }
            break;
#endif
        default:
            options_usage();
            exit(1);
        }
    }
    if (options_apply(proxy_context) != 0) {
        return -1;
    }
#ifdef _WIN32
    if (option_install != 0) {
        if (windows_service_install(proxy_context) != 0) {
            logger_noformat(NULL, LOG_ERR, "Unable to install the service");
            logger_noformat(NULL, LOG_ERR,
                            "Make sure that you are using an elevated command prompt "
                            "and that the service hasn't been already installed");
            exit(1);
        }
        logger_noformat(NULL, LOG_INFO, "The " WINDOWS_SERVICE_NAME
                        " service has been installed and started");
        logger_noformat(NULL, LOG_INFO, "The registry key used for this "
                        "service is " WINDOWS_SERVICE_REGISTRY_PARAMETERS_KEY);
        logger(NULL, LOG_INFO, "Now, change your resolver settings to %s",
               proxy_context->local_ip);
        exit(0);
    }
#endif
    return 0;
}

void
options_free(ProxyContext * const proxy_context)
{
    free(proxy_context->user_dir);
    proxy_context->user_dir = NULL;
}
