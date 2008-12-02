#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

extern void _dlinfo();

int main(int argc, char **argv) {
	void *handle;
	int (*myhowdy)(const char *s);
	char *error;

#ifdef __UCLIBC__
	_dlinfo();   /* not supported by ld.so.2 */
#endif

	handle = dlopen ("./libhowdy.so", RTLD_LAZY);

	if (!handle) {
		fputs (dlerror(), stderr);
		exit(1);
	}

	myhowdy = dlsym(handle, "howdy");
	if ((error = dlerror()) != NULL)  {
		fputs(error, stderr);
		exit(1);
	}

	myhowdy("hello world!\n");

#ifdef __UCLIBC__
	_dlinfo();   /* not supported by ld.so.2 */
#endif

	dlclose(handle);

	return EXIT_SUCCESS;
}
