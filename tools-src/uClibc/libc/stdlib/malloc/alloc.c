#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>


#ifdef L_calloc_dbg

void *calloc_dbg(size_t num, size_t size, char *function, char *file,
				 int line)
{
	void *ptr;

	fprintf(stderr, "calloc of %ld bytes at %s @%s:%d = ",
			(long) (num * size), function, file, line);
	ptr = calloc(num, size);
	fprintf(stderr, "%p\n", ptr);
	return ptr;
}

#endif

#ifdef L_malloc_dbg

void *malloc_dbg(size_t len, char *function, char *file, int line)
{
	void *result;

	fprintf(stderr, "malloc of %ld bytes at %s @%s:%d = ", (long) len,
			function, file, line);
	result = malloc(len);
	fprintf(stderr, "%p\n", result);
	return result;
}

#endif

#ifdef L_free_dbg

void free_dbg(void *ptr, char *function, char *file, int line)
{
	fprintf(stderr, "free of %p at %s @%s:%d\n", ptr, function, file,
			line);
	free(ptr);
}

#endif

#ifdef L_realloc_dbg
void *realloc_dbg(void *ptr, size_t size, char *function, char *file, int line)
{
	fprintf(stderr, "realloc of %p to %ld bytes at %s @%s;%d = ", ptr,
			(long)size, function, file, line);
	ptr = realloc(ptr, size);
	fprintf(stderr, "%p\n", ptr);
	return ptr;
}
#endif
