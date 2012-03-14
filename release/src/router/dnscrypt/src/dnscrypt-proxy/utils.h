
#ifndef __UTILS_H__
#define __UTILS_H__ 1

#define COMPILER_ASSERT(X) (void) sizeof(char[(X) ? 1 : -1])

int closedesc_all(const int closestdin);
int do_daemonize(void);

#endif
