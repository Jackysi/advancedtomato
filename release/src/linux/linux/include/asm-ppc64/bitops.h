/*
 * PowerPC64 atomic bit operations.
 * Dave Engebretsen, Todd Inglett, Don Reed, Pat McCarthy, Peter Bergner,
 * Anton Blanchard
 *
 * Originally taken from the 32b PPC code.  Modified to use 64b values for
 * the various counters & memory references.
 *
 * Bitops are odd when viewed on big-endian systems.  They were designed
 * on little endian so the size of the bitset doesn't matter (low order bytes
 * come first) as long as the bit in question is valid.
 *
 * Bits are "tested" often using the C expression (val & (1<<nr)) so we do
 * our best to stay compatible with that.  The assumption is that val will
 * be unsigned long for such tests.  As such, we assume the bits are stored
 * as an array of unsigned long (the usual case is a single unsigned long,
 * of course).  Here's an example bitset with bit numbering:
 *
 *   |63..........0|127........64|195.......128|255.......196|
 *
 * This leads to a problem. If an int, short or char is passed as a bitset
 * it will be a bad memory reference since we want to store in chunks
 * of unsigned long (64 bits here) size.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#ifndef _PPC64_BITOPS_H
#define _PPC64_BITOPS_H

#ifdef __KERNEL__

#include <asm/byteorder.h>
#include <asm/memory.h>

/*
 * clear_bit doesn't imply a memory barrier
 */
#define smp_mb__before_clear_bit()	smp_mb()
#define smp_mb__after_clear_bit()	smp_mb()

static __inline__ int test_bit(unsigned long nr, __const__ volatile void *addr)
{
	return (1UL & (((__const__ long *) addr)[nr >> 6] >> (nr & 63)));
}

static __inline__ void set_bit(unsigned long nr, volatile void *addr)
{
	unsigned long old;
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	__asm__ __volatile__(
"1:	ldarx	%0,0,%3		# set_bit\n\
	or	%0,%0,%2\n\
	stdcx.	%0,0,%3\n\
	bne-	1b"
	: "=&r" (old), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");
}

static __inline__ void clear_bit(unsigned long nr, volatile void *addr)
{
	unsigned long old;
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	__asm__ __volatile__(
"1:	ldarx	%0,0,%3		# clear_bit\n\
	andc	%0,%0,%2\n\
	stdcx.	%0,0,%3\n\
	bne-	1b"
	: "=&r" (old), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");
}

static __inline__ void change_bit(unsigned long nr, volatile void *addr)
{
	unsigned long old;
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	__asm__ __volatile__(
"1:	ldarx	%0,0,%3		# change_bit\n\
	xor	%0,%0,%2\n\
	stdcx.	%0,0,%3\n\
	bne-	1b"
	: "=&r" (old), "=m" (*p)
	: "r" (mask), "r" (p), "m" (*p)
	: "cc");
}

static __inline__ int test_and_set_bit(unsigned long nr, volatile void *addr)
{
	unsigned long old, t;
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	__asm__ __volatile__(
	EIEIO_ON_SMP
"1:	ldarx	%0,0,%3		# test_and_set_bit\n\
	or	%1,%0,%2 \n\
	stdcx.	%1,0,%3 \n\
	bne-	1b"
	ISYNC_ON_SMP
	: "=&r" (old), "=&r" (t)
	: "r" (mask), "r" (p)
	: "cc", "memory");

	return (old & mask) != 0;
}

static __inline__ int test_and_clear_bit(unsigned long nr, volatile void *addr)
{
	unsigned long old, t;
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	__asm__ __volatile__(
	EIEIO_ON_SMP
"1:	ldarx	%0,0,%3		# test_and_clear_bit\n\
	andc	%1,%0,%2\n\
	stdcx.	%1,0,%3\n\
	bne-	1b"
	ISYNC_ON_SMP
	: "=&r" (old), "=&r" (t)
	: "r" (mask), "r" (p)
	: "cc", "memory");

	return (old & mask) != 0;
}

static __inline__ int test_and_change_bit(unsigned long nr, volatile void *addr)
{
	unsigned long old, t;
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	__asm__ __volatile__(
	EIEIO_ON_SMP
"1:	ldarx	%0,0,%3		# test_and_change_bit\n\
	xor	%1,%0,%2\n\
	stdcx.	%1,0,%3\n\
	bne-	1b"
	ISYNC_ON_SMP
	: "=&r" (old), "=&r" (t)
	: "r" (mask), "r" (p)
	: "cc", "memory");

	return (old & mask) != 0;
}

/*
 * non-atomic versions
 */
static __inline__ void __set_bit(unsigned long nr, volatile void *addr)
{
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	*p |= mask;
}

static __inline__ void __clear_bit(unsigned long nr, volatile void *addr)
{
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	*p &= ~mask;
}

static __inline__ void __change_bit(unsigned long nr, volatile void *addr)
{
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);

	*p ^= mask;
}

static __inline__ int __test_and_set_bit(unsigned long nr, volatile void *addr)
{
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);
	unsigned long old = *p;

	*p = old | mask;
	return (old & mask) != 0;
}

static __inline__ int __test_and_clear_bit(unsigned long nr, volatile void *addr)
{
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);
	unsigned long old = *p;

	*p = old & ~mask;
	return (old & mask) != 0;
}

static __inline__ int __test_and_change_bit(unsigned long nr, volatile void *addr)
{
	unsigned long mask = 1UL << (nr & 0x3f);
	unsigned long *p = ((unsigned long *)addr) + (nr >> 6);
	unsigned long old = *p;

	*p = old ^ mask;
	return (old & mask) != 0;
}

/*
 * Return the zero-based bit position (from RIGHT TO LEFT, 63 -> 0) of the
 * most significant (left-most) 1-bit in a double word.
 */
static __inline__ int __ilog2(unsigned long x)
{
	int lz;

	asm ("cntlzd %0,%1" : "=r" (lz) : "r" (x));
	return 63 - lz;
}

/* Return the zero-based bit position
 *  from RIGHT TO LEFT  63 --> 0
 *   of the most significant (left-most) 1-bit in an 8-byte area.
 */
static __inline__ long cnt_trailing_zeros(unsigned long mask)
{
        long cnt;

	asm(
"	addi	%0,%1,-1	\n\
	andc	%0,%0,%1	\n\
	cntlzd	%0,%0		\n\
	subfic	%0,%0,64"
	: "=r" (cnt)
	: "r" (mask));
	return cnt;
}



/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 *    Determines the bit position of the LEAST significant
 *    (rightmost) 0 bit in the specified DOUBLE-WORD.
 *    The returned bit position will be zero-based, starting
 *    from the right side (63 - 0).
 *    the code should check against ~0UL first..
 */
static __inline__ unsigned long ffz(unsigned long x)
{
	u32  tempRC;

	/* Change all of x's 1s to 0s and 0s to 1s in x.
	 * And insure at least 1 zero exists in the 8 byte area.
	 */
	if ((x = ~x) == 0)
		/* no zero exists anywhere in the 8 byte area. */
		return 64;

	/* Calculate the bit position of the least significant '1' bit in x
	 * (since x has been changed this will actually be the least
	 * significant '0' bit in the original x).
	 * Note: (x & -x) gives us a mask that is the LEAST significant
	 * (RIGHT-most) 1-bit of the value in x.
	 */
	tempRC = __ilog2(x & -x);

	return tempRC;
}

/*
 * ffs: find first bit set. This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static __inline__ int ffs(int x)
{
	int result = ffz(~x);
	return x ? result+1 : 0;
}

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */
#define hweight32(x) generic_hweight32(x)
#define hweight16(x) generic_hweight16(x)
#define hweight8(x) generic_hweight8(x)

extern unsigned long find_next_zero_bit(void * addr, unsigned long size,
					unsigned long offset);
/*
 * The optimizer actually does good code for this case..
 */
#define find_first_zero_bit(addr, size) find_next_zero_bit((addr), (size), 0)

/* Bitmap functions for the ext2 filesystem. */
#define _EXT2_HAVE_ASM_BITOPS_

static __inline__ int ext2_set_bit(int nr, void* addr)
{
	/* This method needs to take into account the fact that the ext2 file system represents
	 *  it's bitmaps as "little endian" unsigned integers.
	 * Note: this method is not atomic, but ext2 does not need it to be.
	 */
	int mask;
	int oldbit;
	unsigned char* ADDR = (unsigned char*) addr;

	/* Determine the BYTE containing the specified bit
	 * (nr) - important as if we go to a byte there are no
	 * little endian concerns.
	 */
	ADDR += nr >> 3;
	mask = 1 << (nr & 0x07);  /* Create a mask to the bit within this byte. */
	oldbit = *ADDR & mask;  /* Save the bit's previous value. */
	*ADDR |= mask;  /* Turn the bit on. */
	return oldbit;  /* Return the bit's previous value. */
}

static __inline__ int ext2_clear_bit(int nr, void* addr)
{
	/* This method needs to take into account the fact that the ext2 file system represents
	 * | it's bitmaps as "little endian" unsigned integers.
	 * Note: this method is not atomic, but ext2 does not need it to be.
	 */
        int     mask;
        int oldbit;
        unsigned char* ADDR = (unsigned char*) addr;

	/* Determine the BYTE containing the specified bit (nr)
	 *  - important as if we go to a byte there are no little endian concerns.
	 */
        ADDR += nr >> 3;
        mask = 1 << (nr & 0x07);  /* Create a mask to the bit within this byte. */
        oldbit = *ADDR & mask;  /* Save the bit's previous value. */
        *ADDR = *ADDR & ~mask;  /* Turn the bit off. */
        return oldbit;  /* Return the bit's previous value. */
}

static __inline__ int ext2_test_bit(int nr, __const__ void * addr)
{
	/* This method needs to take into account the fact that the ext2 file system represents
	 * | it's bitmaps as "little endian" unsigned integers.
	 * Determine the BYTE containing the specified bit (nr),
	 *   then shift to the right the correct number of bits and return that bit's value.
	 */
	__const__ unsigned char	*ADDR = (__const__ unsigned char *) addr;
	return (ADDR[nr >> 3] >> (nr & 7)) & 1;
}

/* Returns the bit position of the most significant 1 bit in a WORD. */
static __inline__ int ext2_ilog2(unsigned int x)
{
        int lz;

        asm ("cntlzw %0,%1" : "=r" (lz) : "r" (x));
        return 31 - lz;
}

/* ext2_ffz = ext2's Find First Zero.
 *    Determines the bit position of the LEAST significant (rightmost) 0 bit in the specified WORD.
 *    The returned bit position will be zero-based, starting from the right side (31 - 0).
 */
static __inline__ int ext2_ffz(unsigned int x)
{
	u32  tempRC;
	/* Change all of x's 1s to 0s and 0s to 1s in x.  And insure at least 1 zero exists in the word. */
	if ((x = ~x) == 0)
		/* no zero exists anywhere in the 4 byte area. */
		return 32;
	/* Calculate the bit position of the least significant '1' bit in x
	 * (since x has been changed this will actually be the least
	 * significant '0' bit in the original x).
	 * Note: (x & -x) gives us a mask that is the LEAST significant
	 * (RIGHT-most) 1-bit of the value in x.
	 */
	tempRC = ext2_ilog2(x & -x);
	return tempRC;
}

static __inline__ u32 ext2_find_next_zero_bit(void* addr, u32 size, u32 offset)
{
	/* This method needs to take into account the fact that the ext2 file system represents
	 * | it's bitmaps as "little endian" unsigned integers.
	 */
        unsigned int *p = ((unsigned int *) addr) + (offset >> 5);
        unsigned int result = offset & ~31;
        unsigned int tmp;

        if (offset >= size)
                return size;
        size -= result;
        offset &= 31;
        if (offset) {
                tmp = cpu_to_le32p(p++);
                tmp |= ~0U >> (32-offset); /* bug or feature ? */
                if (size < 32)
                        goto found_first;
                if (tmp != ~0)
                        goto found_middle;
                size -= 32;
                result += 32;
        }
        while (size >= 32) {
                if ((tmp = cpu_to_le32p(p++)) != ~0)
                        goto found_middle;
                result += 32;
                size -= 32;
        }
        if (!size)
                return result;
        tmp = cpu_to_le32p(p);
found_first:
        tmp |= ~0 << size;
        if (tmp == ~0)          /* Are any bits zero? */
                return result + size; /* Nope. */
found_middle:
        return result + ext2_ffz(tmp);
}

#define ext2_find_first_zero_bit(addr, size) ext2_find_next_zero_bit((addr), (size), 0)

#endif /* __KERNEL__ */
#endif /* _PPC64_BITOPS_H */
