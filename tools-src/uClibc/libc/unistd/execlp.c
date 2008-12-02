
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

extern int execvep(const char *path, char *__const argv[], char *__const envp[]);

int execlp(__const char *file, __const char *arg, ...)
{
	const char *shortargv[16];
	const char **argv;
	const char *c;
	int i;
	va_list args;

	i = 1;

	va_start(args, arg);

	do {
		c = va_arg(args, const char *);

		i++;
	} while (c);

	va_end(args);

	if (i <= 16)
		argv = shortargv;
	else {
		argv = (const char **) alloca(sizeof(char *) * i);
	}

	argv[0] = arg;
	i = 1;

	va_start(args, arg);

	do {
		argv[i] = va_arg(args, const char *);
	} while (argv[i++]);

	va_end(args);

	i = execvep(file, (char *const *) argv, __environ);

	return i;
}
