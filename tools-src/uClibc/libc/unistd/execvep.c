#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

int execvep(const char *path, char *__const argv[], char *__const envp[])
{
    if (!strchr(path, '/')) {
	char *p = getenv("PATH");

	if (!p)
	    p = "/bin:/usr/bin";

	for (; p && *p;) {
	    char partial[FILENAME_MAX];
	    char *p2;

	    p2 = strchr(p, ':');
	    if (p2) {
		size_t len = p2 - p;
		strncpy(partial, p, len);
		partial[len] = 0;
	    } else {
		strcpy(partial, p);
	    }

	    if (strlen(partial))
		strcat(partial, "/");
	    strcat(partial, path);

	    execve(partial, argv, envp);

	    if (errno != ENOENT)
		return -1;

	    if (p2) {
		p = p2 + 1;
	    } else {
		p = 0;
	    }
	}
	return -1;
    } else
	return execve(path, argv, envp);
}
