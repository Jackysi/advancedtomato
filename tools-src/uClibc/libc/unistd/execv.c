#include <unistd.h>

int execv(__const char *path, char *__const argv[])
{
	return execve(path, argv, __environ);
}
