#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <utmp.h>

/* Write the given entry into utmp and wtmp.  */
void login (const struct utmp *entry)
{
    return;
}
link_warning (login, "the `login' function is stubbed out and will not write utmp or wtmp.")

