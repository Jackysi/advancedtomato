
#ifndef __OPTIONS_H__
#define __OPTIONS_H__ 1

int options_parse(AppContext * const app_context,
                  ProxyContext * const proxy_context, int argc, char *argv[]);

void options_free(ProxyContext * const proxy_context);

#define OPTIONS_RESOLVERS_LIST_MAX_COLS 50
#define OPTIONS_CLIENT_KEY_HEADER "\01\01"

#endif
