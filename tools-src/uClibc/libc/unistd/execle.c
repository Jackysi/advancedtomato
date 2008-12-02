
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

extern int execvep(const char *path, char *const argv[], char *const envp[]);

int execle(const char *file, const char *arg, ...)
{
	const char *shortargv[16];
	const char **argv;
	const char *c;
	int i;
	va_list args;
	const char *const *envp;

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

	envp = va_arg (args, const char *const *);
	va_end(args);

	i = execvep(file, (char *const *) argv, (char *const *) envp);

	return i;
}
