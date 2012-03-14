
#ifdef _WIN32

#include <stdlib.h>

static void
srandom(unsigned seed)
{
    srand(seed);
}

static long
random(void)
{
    return (long) rand();
}

#endif
