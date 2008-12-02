/* malloc.c - C standard library routine.
   Copyright (c) 1989, 1993  Michael J. Haertel
   You may redistribute this library under the terms of the
   GNU Library General Public License (version 2 or any later
   version) as published by the Free Software Foundation.
   THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED
   WARRANTY.  IN PARTICULAR, THE AUTHOR MAKES NO REPRESENTATION OR
   WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
   SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE. */

#define _GNU_SOURCE
#include <features.h>
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "malloc.h"

#ifdef __UCLIBC_HAS_THREADS__
#include <pthread.h>
extern pthread_mutex_t __malloclock;
# define LOCK	pthread_mutex_lock(&__malloclock)
# define UNLOCK	pthread_mutex_unlock(&__malloclock);
#else
# define LOCK
# define UNLOCK
#endif


__ptr_t memalign (size_t alignment, size_t size)
{
    __ptr_t result;
    unsigned long int adj;

    result = malloc (size + alignment - 1);
    if (result == NULL)
	return NULL;
    adj = (unsigned long int) ((unsigned long int) ((char *) result -
		(char *) NULL)) % alignment;
    if (adj != 0)
    {
	struct alignlist *l;
	LOCK;
	for (l = _aligned_blocks; l != NULL; l = l->next)
	    if (l->aligned == NULL)
		/* This slot is free.  Use it.  */
		break;
	if (l == NULL)
	{
	    l = (struct alignlist *) malloc (sizeof (struct alignlist));
	    if (l == NULL) {
		__free_unlocked (result);
		UNLOCK;
		return NULL;
	    }
	    l->next = _aligned_blocks;
	    _aligned_blocks = l;
	}
	l->exact = result;
	result = l->aligned = (char *) result + alignment - adj;
	UNLOCK;
    }

    return result;
}

