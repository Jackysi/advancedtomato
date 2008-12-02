#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

extern void _dlinfo();
extern int __pthread_return_0 (void);
#undef __UCLIBC__

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

#ifdef FORCE
	printf("main:   __pthread_return_0 = %p\n", __pthread_return_0);
#endif
	myhowdy("hello world!\n");

#ifdef __UCLIBC__
	_dlinfo();   /* not supported by ld.so.2 */
#endif

	dlclose(handle);

	return EXIT_SUCCESS;
}
