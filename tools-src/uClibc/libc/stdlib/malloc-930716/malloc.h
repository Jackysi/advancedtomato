/* malloc.h - declarations for the allocator.
   Copyright (c) 1989, 1993  Michael J. Haertel
   You may redistribute this library under the terms of the
   GNU Library General Public License (version 2 or any later
   version) as published by the Free Software Foundation.
   THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY EXPRESS OR IMPLIED
   WARRANTY.  IN PARTICULAR, THE AUTHOR MAKES NO REPRESENTATION OR
   WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
   SOFTWARE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE. */

#include <sys/cdefs.h>


#define MIN(x,y) ({ \
	const typeof(x) _x = (x);       \
	const typeof(y) _y = (y);       \
	(void) (&_x == &_y);            \
	_x < _y ? _x : _y; })



/* The allocator divides the heap into blocks of fixed size; large
   requests receive one or more whole blocks, and small requests
   receive a fragment of a block.  Fragment sizes are powers of two,
   and all fragments of a block are the same size.  When all the
   fragments in a block have been freed, the block itself is freed.
   */
#define INT_BIT (CHAR_BIT * sizeof (size_t))
#define BLOCKLOG (INT_BIT > 16 ? 12 : 9)
#define BLOCKSIZE (1 << BLOCKLOG)
#define BLOCKIFY(SIZE) (((SIZE) + BLOCKSIZE - 1) / BLOCKSIZE)

/* Determine the amount of memory spanned by the initial heap table
   (not an absolute limit). */
#define HEAP (INT_BIT > 16 ? 4194304 : 65536)

/* Number of contiguous free blocks allowed to build up at the end of
   memory before they will be returned to the system. */
#define FINAL_FREE_BLOCKS 8

/* Data structure giving per-block information. */
union info {
    struct {
	size_t type;		/* Zero for a large block, or positive
				   giving the logarithm to the base two
				   of the fragment size. */
	union {
	    struct {
		size_t nfree;	/* Free fragments in a fragmented block. */
		size_t first;	/* First free fragment of the block. */
	    } frag;
	    size_t size;	/* Size (in blocks) of a large cluster. */
	} info;
    } busy;
    struct {
	size_t size;		/* Size (in blocks) of a free cluster. */
	size_t next;		/* Index of next free cluster. */
	size_t prev;		/* Index of previous free cluster. */
    } free;
};

/* Address to block number and vice versa. */
#define BLOCK(A) (((char *) (A) - _heapbase) / BLOCKSIZE + 1)
#define ADDRESS(B) ((void *) (((B) - 1) * BLOCKSIZE + _heapbase))

/* Doubly linked lists of free fragments. */
struct list {
    struct list *next;
    struct list *prev;
};

/* List of blocks allocated with memalign or valloc */
struct alignlist
{ 
    struct alignlist *next;
    __ptr_t aligned;	/* The address that memaligned returned.  */
    __ptr_t exact;	/* The address that malloc returned.  */
};
extern struct alignlist *_aligned_blocks;
extern char *_heapbase;
extern union info *_heapinfo;
extern size_t _heapindex;
extern size_t _heaplimit;


extern void *__malloc_unlocked (size_t size);
extern void __free_unlocked(void *ptr);


