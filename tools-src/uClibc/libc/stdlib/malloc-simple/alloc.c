
/*
 *	For MMU hosts we need to track the size of the allocations otherwise
 *	munmap will fail to free the memory (EINVAL).
 */

#include <features.h>
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

	fprintf(stderr, "calloc of %d bytes at %s @%s:%d = ", (int) (num * size),
			function, file, line);
	ptr = calloc(num, size);
	fprintf(stderr, "%p\n", ptr);
	return ptr;
}

#endif

#ifdef L_malloc_dbg

void *malloc_dbg(size_t size, char *function, char *file, int line)
{
	void *result;

	fprintf(stderr, "malloc of %d bytes at %s @%s:%d = ", (int) size, function,
			file, line);
	result = malloc(size);
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


#ifdef L_calloc

void *calloc(size_t num, size_t size)
{
	void *ptr = malloc(num * size);

	if (ptr)
		memset(ptr, 0, num * size);
	return ptr;
}

#endif

#ifdef L_malloc

void *malloc(size_t size)
{
	void *result;

    /* Some programs will call malloc (0).  Lets be strict and return NULL */
    if (size == 0)
		return NULL;

#ifdef __UCLIBC_HAS_MMU__
	result = mmap((void *) 0, size + sizeof(size_t), PROT_READ | PROT_WRITE,
						MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
#else
	result = mmap((void *) 0, size, PROT_READ | PROT_WRITE,
						MAP_SHARED | MAP_ANONYMOUS, 0, 0);
#endif

	if (result == MAP_FAILED)
		return 0;
	
#ifdef __UCLIBC_HAS_MMU__
	* (size_t *) result = size;
	return(result + sizeof(size_t));
#else
	return(result);
#endif
}

#endif

#ifdef L_free

void free(void *ptr)
{
#ifdef __UCLIBC_HAS_MMU__
	if (ptr) {
		ptr -= sizeof(size_t);
		munmap(ptr, * (size_t *) ptr);
	}
#else
	munmap(ptr, 0);
#endif
}

#endif

#ifdef L_realloc

void *realloc(void *ptr, size_t size)
{
	void *newptr = NULL;

	if (size > 0) {
		newptr = malloc(size);
		if (newptr && ptr) {
#ifdef __UCLIBC_HAS_MMU__
			memcpy(newptr, ptr, * ((size_t *) (ptr - sizeof(size_t))));
#else
			memcpy(newptr, ptr, size);
#endif
			free(ptr);
		}
	}
	else
		free(ptr);
	return newptr;
}

#endif
