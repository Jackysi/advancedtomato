#include <unistd.h>

extern int execvep(const char *path, char *__const argv[], char *__const envp[]);

int execvp(__const char *path, char *__const argv[])
{
	return execvep(path, argv, __environ);
}
