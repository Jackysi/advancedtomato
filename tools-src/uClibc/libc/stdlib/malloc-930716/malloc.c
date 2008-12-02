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
pthread_mutex_t __malloclock = PTHREAD_MUTEX_INITIALIZER;
# define LOCK	pthread_mutex_lock(&__malloclock)
# define UNLOCK	pthread_mutex_unlock(&__malloclock);
#else
# define LOCK
# define UNLOCK
#endif


/* Stuff that is shared across .o files */

/* Pointer to the base of the first block. */
char *_heapbase;
/* Block information table. */
union info *_heapinfo;
/* Search index in the info table. */
size_t _heapindex;
/* Limit of valid info table indices. */
size_t _heaplimit;
/* List of blocks allocated with memalign or valloc */
struct alignlist *_aligned_blocks;

    

/* Stuff that is local to this .o file only */

/* How to really get more memory. */
static void * __morecore(long size);
/* Number of info entries. */
static size_t heapsize;
/* Count of large blocks allocated for each fragment size. */
static size_t _fragblocks[BLOCKLOG];
/* Free lists for each fragment size. */
static struct list _fraghead[BLOCKLOG];
/* Are we experienced? */
static int initialized;


/* Aligned allocation.
 * Called within the lock in initialize() and morecore(), 
 * so no explicit locking needed... */
static void * align(size_t size)
{
    void *result;
    unsigned int adj;

    result = __morecore(size);
    adj = (unsigned int) ((char *) result - (char *) NULL) % BLOCKSIZE;
    if (adj != 0) {
	__morecore(adj = BLOCKSIZE - adj);
	result = (char *) result + adj;
    }
    return result;
}

/* Set everything up and remember that we have. 
 * Called within the lock in malloc(), so no
 * explicit locking needed... */
static int initialize(void)
{
    heapsize = HEAP / BLOCKSIZE;
    _heapinfo = align(heapsize * sizeof (union info));
    if (!_heapinfo) {
	return 0;
    }
    memset(_heapinfo, 0, heapsize * sizeof (union info));
    _heapinfo[0].free.size = 0;
    _heapinfo[0].free.next = _heapinfo[0].free.prev = 0;
    _heapindex = 0;
    _heapbase = (char *) _heapinfo;
    initialized = 1;
    return 1;
}

/* Get neatly aligned memory, initializing or growing the 
 * heap info table as necessary. 
 * Called within a lock in malloc() and free(), 
 * so no explicit locking needed... */
static void * morecore(size_t size)
{
    void *result;
    union info *newinfo, *oldinfo;
    size_t newsize;

    result = align(size);
    if (!result)
	return NULL;

    /* Check if we need to grow the info table. */
    if (BLOCK((char *) result + size) > heapsize) {
	newsize = heapsize;
	while (BLOCK((char *) result + size) > newsize)
	    newsize *= 2;
	newinfo = align(newsize * sizeof (union info));
	if (!newinfo) {
	    __morecore(-size);
	    return NULL;
	}
	memset(newinfo, 0, newsize * sizeof (union info));
	memcpy(newinfo, _heapinfo, heapsize * sizeof (union info));
	oldinfo = _heapinfo;
	newinfo[BLOCK(oldinfo)].busy.type = 0;
	newinfo[BLOCK(oldinfo)].busy.info.size
	    = BLOCKIFY(heapsize * sizeof (union info));
	_heapinfo = newinfo;
	__free_unlocked(oldinfo);
	heapsize = newsize;
    }

    _heaplimit = BLOCK((char *) result + size);
    return result;
}

/* Note that morecore has to take a signed argument so
   that negative values can return memory to the system. */
static void * __morecore(long size)
{
    void *result;

    result = sbrk(size);
    if (result == (void *) -1)
	return NULL;
    return result;
}

/* Allocate memory from the heap. */
void * malloc (size_t size)
{
    void * ptr;
    LOCK;
    ptr = __malloc_unlocked(size);
    UNLOCK;
    return(ptr);
}

void * __malloc_unlocked (size_t size)
{
    void *result;
    size_t log, block, blocks, i, lastblocks, start;
    struct list *next;

    /* Some programs will call malloc (0).  Lets be strict and return NULL */
    if (size == 0)
	return NULL;

    if (size < sizeof (struct list))
	size = sizeof (struct list);

    if (!initialized && !initialize()) {
	return NULL;
    }

    /* Determine the allocation policy based on the request size. */
    if (size <= BLOCKSIZE / 2) {
	/* Small allocation to receive a fragment of a block. Determine
	   the logarithm to base two of the fragment size. */
	--size;
	for (log = 1; (size >>= 1) != 0; ++log)
	    ;

	/* Look in the fragment lists for a free fragment of the
	   desired size. */
	if ((next = _fraghead[log].next) != 0) {
	    /* There are free fragments of this size.  Pop a fragment
	       out of the fragment list and return it.  Update the block's
	       nfree and first counters. */
	    result = next;
	    next->prev->next = next->next;
	    if (next->next)
		next->next->prev = next->prev;
	    block = BLOCK(result);
	    if (--_heapinfo[block].busy.info.frag.nfree)
		_heapinfo[block].busy.info.frag.first
		    = (unsigned int) ((char *) next->next - (char *) NULL)
		      % BLOCKSIZE >> log;
	} else {
	    /* No free fragments of the desired size, so get a new block
	       and break it into fragments, returning the first. */
	    result = __malloc_unlocked(BLOCKSIZE);
	    if (!result) {
		return NULL;
	    }
	    ++_fragblocks[log];

	    /* Link all fragments but the first into the free list. */
	    next = (struct list *) ((char *) result + (1 << log));
	    next->next = 0;
	    next->prev = &_fraghead[log];
	    _fraghead[log].next = next;

	    for (i = 2; i < BLOCKSIZE >> log; ++i) {
		next = (struct list *) ((char *) result + (i << log));
		next->next = _fraghead[log].next;
		next->prev = &_fraghead[log];
		next->prev->next = next;
		next->next->prev = next;
	    }

	    /* Initialize the nfree and first counters for this block. */
	    block = BLOCK(result);
	    _heapinfo[block].busy.type = log;
	    _heapinfo[block].busy.info.frag.nfree = i - 1;
	    _heapinfo[block].busy.info.frag.first = i - 1;
	}
    } else {
	/* Large allocation to receive one or more blocks.  Search
	   the free list in a circle starting at the last place visited.
	   If we loop completely around without finding a large enough
	   space we will have to get more memory from the system. */
	blocks = BLOCKIFY(size);
	start = block = _heapindex;
	while (_heapinfo[block].free.size < blocks) {
	    block = _heapinfo[block].free.next;
	    if (block == start) {
		/* Need to get more from the system.  Check to see if
		   the new core will be contiguous with the final free
		   block; if so we don't need to get as much. */
		block = _heapinfo[0].free.prev;
		lastblocks = _heapinfo[block].free.size;
		if (_heaplimit && block + lastblocks == _heaplimit
		    && __morecore(0) == ADDRESS(block + lastblocks)
		    && morecore((blocks - lastblocks) * BLOCKSIZE)) {
		    /* Note that morecore() can change the location of
		       the final block if it moves the info table and the
		       old one gets coalesced into the final block. */
		    block = _heapinfo[0].free.prev;
		    _heapinfo[block].free.size += blocks - lastblocks;
		    continue;
		}
		result = morecore(blocks * BLOCKSIZE);
		if (!result) {
		    return NULL;
		}
		block = BLOCK(result);
		_heapinfo[block].busy.type = 0;
		_heapinfo[block].busy.info.size = blocks;
		return result;
	    }
	}

	/* At this point we have found a suitable free list entry.
	   Figure out how to remove what we need from the list. */
	result = ADDRESS(block);
	if (_heapinfo[block].free.size > blocks) {
	    /* The block we found has a bit left over, so relink the
	       tail end back into the free list. */
	    _heapinfo[block + blocks].free.size
		= _heapinfo[block].free.size - blocks;
	    _heapinfo[block + blocks].free.next
		= _heapinfo[block].free.next;
	    _heapinfo[block + blocks].free.prev
		= _heapinfo[block].free.prev;
	    _heapinfo[_heapinfo[block].free.prev].free.next
		= _heapinfo[_heapinfo[block].free.next].free.prev
		    = _heapindex = block + blocks;
	} else {
	    /* The block exactly matches our requirements, so
	       just remove it from the list. */
	    _heapinfo[_heapinfo[block].free.next].free.prev
		= _heapinfo[block].free.prev;
	    _heapinfo[_heapinfo[block].free.prev].free.next
		= _heapindex = _heapinfo[block].free.next;
	}

	_heapinfo[block].busy.type = 0;
	_heapinfo[block].busy.info.size = blocks;
    }

    return result;
}

/* Return memory to the heap. */
void free(void *ptr)
{
    struct alignlist *l;

    if (ptr == NULL)
	return;

    LOCK;
    for (l = _aligned_blocks; l != NULL; l = l->next) {
	if (l->aligned == ptr) {
	    /* Mark the block as free */
	    l->aligned = NULL;
	    ptr = l->exact;
	    break;
	}
    }

    __free_unlocked(ptr);
    UNLOCK;
}

void __free_unlocked(void *ptr)
{
    int block, blocks, i, type;
    struct list *prev, *next;

    if (ptr == NULL)
	return;


    block = BLOCK(ptr);

    switch (type = _heapinfo[block].busy.type) {
	case 0:
	    /* Find the free cluster previous to this one in the free list.
	       Start searching at the last block referenced; this may benefit
	       programs with locality of allocation. */
	    i = _heapindex;
	    if (i > block)
		while (i > block)
		    i = _heapinfo[i].free.prev;
	    else {
		do
		    i = _heapinfo[i].free.next;
		while (i > 0 && i < block);
		i = _heapinfo[i].free.prev;
	    }

	    /* Determine how to link this block into the free list. */
	    if (block == i + _heapinfo[i].free.size) {
		/* Coalesce this block with its predecessor. */
		_heapinfo[i].free.size += _heapinfo[block].busy.info.size;
		block = i;
	    } else {
		/* Really link this block back into the free list. */
		_heapinfo[block].free.size = _heapinfo[block].busy.info.size;
		_heapinfo[block].free.next = _heapinfo[i].free.next;
		_heapinfo[block].free.prev = i;
		_heapinfo[i].free.next = block;
		_heapinfo[_heapinfo[block].free.next].free.prev = block;
	    }

	    /* Now that the block is linked in, see if we can coalesce it
	       with its successor (by deleting its successor from the list
	       and adding in its size). */
	    if (block + _heapinfo[block].free.size == _heapinfo[block].free.next) {
		_heapinfo[block].free.size
		    += _heapinfo[_heapinfo[block].free.next].free.size;
		_heapinfo[block].free.next
		    = _heapinfo[_heapinfo[block].free.next].free.next;
		_heapinfo[_heapinfo[block].free.next].free.prev = block;
	    }

	    /* Now see if we can return stuff to the system. */
	    blocks = _heapinfo[block].free.size;
	    if (blocks >= FINAL_FREE_BLOCKS && block + blocks == _heaplimit
		    && __morecore(0) == ADDRESS(block + blocks)) {
		_heaplimit -= blocks;
		__morecore(-blocks * BLOCKSIZE);
		_heapinfo[_heapinfo[block].free.prev].free.next
		    = _heapinfo[block].free.next;
		_heapinfo[_heapinfo[block].free.next].free.prev
		    = _heapinfo[block].free.prev;
		block = _heapinfo[block].free.prev;
	    }

	    /* Set the next search to begin at this block. */
	    _heapindex = block;
	    break;

	default:
	    /* Get the address of the first free fragment in this block. */
	    prev = (struct list *) ((char *) ADDRESS(block)
		    + (_heapinfo[block].busy.info.frag.first
			<< type));

	    if (_heapinfo[block].busy.info.frag.nfree == (BLOCKSIZE >> type) - 1
		    && _fragblocks[type] > 1) {
		/* If all fragments of this block are free, remove them
		   from the fragment list and free the whole block. */
		--_fragblocks[type];
		for (next = prev, i = 1; i < BLOCKSIZE >> type; ++i)
		    next = next->next;
		prev->prev->next = next;
		if (next)
		    next->prev = prev->prev;
		_heapinfo[block].busy.type = 0;
		_heapinfo[block].busy.info.size = 1;
		__free_unlocked(ADDRESS(block));
	    } else if (_heapinfo[block].busy.info.frag.nfree) {
		/* If some fragments of this block are free, link this fragment
		   into the fragment list after the first free fragment of
		   this block. */
		next = ptr;
		next->next = prev->next;
		next->prev = prev;
		prev->next = next;
		if (next->next)
		    next->next->prev = next;
		++_heapinfo[block].busy.info.frag.nfree;
	    } else {
		/* No fragments of this block are free, so link this fragment
		   into the fragment list and announce that it is the first
		   free fragment of this block. */
		prev = (struct list *) ptr;
		_heapinfo[block].busy.info.frag.nfree = 1;
		_heapinfo[block].busy.info.frag.first
		    = (unsigned int) ((char *) ptr - (char *) NULL) % BLOCKSIZE
		    >> type;
		prev->next = _fraghead[type].next;
		prev->prev = &_fraghead[type];
		prev->prev->next = prev;
		if (prev->next)
		    prev->next->prev = prev;
	    }
	    break;
    }
}

