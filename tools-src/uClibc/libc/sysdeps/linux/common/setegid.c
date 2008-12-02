#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>

int setegid(gid_t gid)
{
	return setregid(-1, gid);
}
