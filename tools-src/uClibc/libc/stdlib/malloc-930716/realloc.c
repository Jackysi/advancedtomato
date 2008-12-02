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

/* Resize the given region to the new size, returning a pointer
   to the (possibly moved) region.  This is optimized for speed;
   some benchmarks seem to indicate that greater compactness is
   achieved by unconditionally allocating and copying to a
   new region. */
void * realloc (void *ptr, size_t size)
{
    void *result, *previous;
    size_t block, blocks, type;
    size_t oldlimit;

    if (!ptr)
	return malloc(size);
    if (!size) {
	LOCK;
	__free_unlocked(ptr);
	result = __malloc_unlocked(0);
	UNLOCK;
	return(result);
    }

    LOCK;
    block = BLOCK(ptr);

    switch (type = _heapinfo[block].busy.type) {
	case 0:
	    /* Maybe reallocate a large block to a small fragment. */
	    if (size <= BLOCKSIZE / 2) {
		if ((result = __malloc_unlocked(size)) != NULL) {
		    memcpy(result, ptr, size);
		    __free_unlocked(ptr);
		}
		UNLOCK;
		return result;
	    }

	    /* The new size is a large allocation as well; see if
	       we can hold it in place. */
	    blocks = BLOCKIFY(size);
	    if (blocks < _heapinfo[block].busy.info.size) {
		/* The new size is smaller; return excess memory
		   to the free list. */
		_heapinfo[block + blocks].busy.type = 0;
		_heapinfo[block + blocks].busy.info.size
		    = _heapinfo[block].busy.info.size - blocks;
		_heapinfo[block].busy.info.size = blocks;
		__free_unlocked(ADDRESS(block + blocks));
		UNLOCK;
		return ptr;
	    } else if (blocks == _heapinfo[block].busy.info.size) {
		/* No size change necessary. */
		UNLOCK;
		return ptr;
	    } else {
		/* Won't fit, so allocate a new region that will.  Free
		   the old region first in case there is sufficient adjacent
		   free space to grow without moving. */
		blocks = _heapinfo[block].busy.info.size;
		/* Prevent free from actually returning memory to the system. */
		oldlimit = _heaplimit;
		_heaplimit = 0;
		__free_unlocked(ptr);
		_heaplimit = oldlimit;
		result = __malloc_unlocked(size);
		if (!result) {
		    /* Now we're really in trouble.  We have to unfree
		       the thing we just freed.  Unfortunately it might
		       have been coalesced with its neighbors. */
		    if (_heapindex == block)
			__malloc_unlocked(blocks * BLOCKSIZE);
		    else {
			previous = __malloc_unlocked((block - _heapindex) * BLOCKSIZE);
			__malloc_unlocked(blocks * BLOCKSIZE);
			__free_unlocked(previous);
		    }	    
		    UNLOCK;
		    return NULL;
		}
		if (ptr != result)
		    memmove(result, ptr, blocks * BLOCKSIZE);
		UNLOCK;
		return result;
	    }
	    break;

	default:
	    /* Old size is a fragment; type is logarithm to base two of
	       the fragment size. */
	    if ((size > 1 << (type - 1)) && (size <= 1 << type)) {
		/* New size is the same kind of fragment. */
		UNLOCK;
		return ptr;
	    }
	    else {
		/* New size is different; allocate a new space, and copy
		   the lesser of the new size and the old. */
		result = __malloc_unlocked(size);
		if (!result) {
		    UNLOCK;
		    return NULL;
		}
		memcpy(result, ptr, MIN(size, (size_t)(1 << type)));
		__free_unlocked(ptr);
		UNLOCK;
		return result;
	    }
	    break;
    }
    UNLOCK;
}

