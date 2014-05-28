
#ifndef __WINDOWS_SERVICE_H__
#define __WINDOWS_SERVICE_H__ 1

#ifndef WINDOWS_SERVICE_NAME
# define WINDOWS_SERVICE_NAME "dnscrypt-proxy"
#endif

#ifndef WINDOWS_SERVICE_REGISTRY_PARAMETERS_KEY
# define  WINDOWS_SERVICE_REGISTRY_PARAMETERS_KEY \
    "SYSTEM\\CurrentControlSet\\Services\\" \
    WINDOWS_SERVICE_NAME "\\Parameters"
#endif

typedef enum WinOption_ {
    WIN_OPTION_INSTALL = 256,
    WIN_OPTION_REINSTALL,
    WIN_OPTION_UNINSTALL
} WinOption;

int windows_service_install(ProxyContext * const proxy_context);
int windows_service_uninstall(void);

#endif
