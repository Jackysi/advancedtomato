#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include "thread_db.h"

extern void _dlinfo();

int main(int argc, char **argv) {
	void *handle;
	td_err_e (*td_init_p) (void);

	fprintf(stderr, "Attempting to dlopen() libpthread.so with RTLD_NOW\n");
	handle = dlopen ("libpthread.so", RTLD_NOW);
	if (!handle) {
		fputs (dlerror(), stderr);
		exit(1);
	}

	td_init_p = dlsym (handle, "__pthread_initialize");
	if (td_init_p == NULL) {
	    fprintf(stderr, "yipe!  __pthread_initialize() failed!\n");
	    return EXIT_FAILURE;
	}
#if 0 //def __UCLIBC__
	_dlinfo();   /* not supported by ld.so.2 */
#endif
	dlclose(handle);


	fprintf(stderr, "Attempting to dlopen() libpthread.so with RTLD_LAZY\n");
	handle = dlopen ("libpthread.so", RTLD_LAZY);
	if (!handle) {
		fputs (dlerror(), stderr);
		exit(1);
	}

	td_init_p = dlsym (handle, "__pthread_initialize");
	if (td_init_p == NULL) {
	    fprintf(stderr, "yipe!  __pthread_initialize() failed!");
	    return EXIT_FAILURE;
	}
#if 0 //def __UCLIBC__
	_dlinfo();   /* not supported by ld.so.2 */
#endif
	dlclose(handle);

	fprintf(stderr, "Everything worked as expected.\n");
	return EXIT_SUCCESS;
}
