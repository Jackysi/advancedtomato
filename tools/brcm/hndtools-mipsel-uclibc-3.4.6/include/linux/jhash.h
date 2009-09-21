#ifndef _LINUX_JHASH_H
#define _LINUX_JHASH_H

/* jhash.h: Jenkins hash support.
 *
 * Copyright (C) 2006 Bob Jenkins (bob_jenkins@burtleburtle.net)
 *
 * http://burtleburtle.net/bob/hash/
 *
 * These are the credits from Bob's sources:
 *
 * lookup3.c, by Bob Jenkins, May 2006, Public Domain.
 *
 * These are functions for producing 32-bit hashes for hash table lookup.
 * hashword(), hashlittle(), hashlittle2(), hashbig(), mix(), and final() 
 * are externally useful functions. Routines to test the hash are included 
 * if SELF_TEST is defined. You can use this free for any purpose. It's in
 * the public domain. It has no warranty.
 *
 * Copyright (C) 2009 Jozsef Kadlecsik (kadlec@xxxxxxxxxxxxxxxxx)
 *
 * I've modified Bob's hash to be useful in the Linux kernel, and
 * any bugs present are my fault. Jozsef
 */

// SpeedMod: Replaced jenkins lookup3 hash with murmurhash2
 
/* The most generic version, hashes an arbitrary sequence
 * of bytes.  No alignment or length assumptions are made about
 * the input key. The result depends on endianness.
 */

//-----------------------------------------------------------------------------
// MurmurHashAligned2, by Austin Appleby

// Same algorithm as MurmurHash2, but only does aligned reads - should be safer
// on certain platforms. 

// Performance will be lower than MurmurHash2

#define MIX(h,k,m) { k *= m; k ^= k >> r; k *= m; h *= m; h ^= k; }

static inline u32 jhash ( const void * key, u32 len, u32 seed )
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	const unsigned char * data = (const unsigned char *)key;

	unsigned int h = seed ^ len;

	int align = (int)data & 3;

	if(align && (len >= 4))
	{
		// Pre-load the temp registers

		unsigned int t = 0, d = 0;

		switch(align)
		{
			case 1: t |= data[2] << 16;
			case 2: t |= data[1] << 8;
			case 3: t |= data[0];
		}

		t <<= (8 * align);

		data += 4-align;
		len -= 4-align;

		int sl = 8 * (4-align);
		int sr = 8 * align;

		// Mix

		while(len >= 4)
		{
			d = *(unsigned int *)data;
			t = (t >> sr) | (d << sl);

			unsigned int k = t;

			MIX(h,k,m);

			t = d;

			data += 4;
			len -= 4;
		}

		// Handle leftover data in temp registers

		d = 0;

		if(len >= align)
		{
			switch(align)
			{
			case 3: d |= data[2] << 16;
			case 2: d |= data[1] << 8;
			case 1: d |= data[0];
			}

			unsigned int k = (t >> sr) | (d << sl);
			MIX(h,k,m);

			data += align;
			len -= align;

			//----------
			// Handle tail bytes

			switch(len)
			{
			case 3: h ^= data[2] << 16;
			case 2: h ^= data[1] << 8;
			case 1: h ^= data[0];
					h *= m;
			};
		}
		else
		{
			switch(len)
			{
			case 3: d |= data[2] << 16;
			case 2: d |= data[1] << 8;
			case 1: d |= data[0];
			case 0: h ^= (t >> sr) | (d << sl);
					h *= m;
			}
		}

		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;

		return h;
	}
	else
	{
		while(len >= 4)
		{
			unsigned int k = *(unsigned int *)data;

			MIX(h,k,m);

			data += 4;
			len -= 4;
		}

		//----------
		// Handle tail bytes

		switch(len)
		{
		case 3: h ^= data[2] << 16;
		case 2: h ^= data[1] << 8;
		case 1: h ^= data[0];
				h *= m;
		};

		h ^= h >> 13;
		h *= m;
		h ^= h >> 15;

		return h;
	}
}

/* A special optimized version that handles 1 or more of u32s.
 * The length parameter here is the number of u32s in the key.
 */
 
//-----------------------------------------------------------------------------
// MurmurHash2, by Austin Appleby

// Note - This code makes a few assumptions about how your machine behaves -

// 1. We can read a 4-byte value from any address without crashing
// 2. sizeof(int) == 4

// And it has a few limitations -

// 1. It will not work incrementally.
// 2. It will not produce the same results on little-endian and big-endian
//    machines.

static inline jhash2 ( u32 * key, u32 len, u32 seed )
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995;
	const int r = 24;

	// Initialize the hash to a 'random' value

	unsigned int h = seed ^ len;

	// Mix 4 bytes at a time into the hash

	const unsigned char * data = (const unsigned char *)key;

	while(len >= 4)
	{
		unsigned int k = *(unsigned int *)data;

		k *= m; 
		k ^= k >> r; 
		k *= m; 
		
		h *= m; 
		h ^= k;

		data += 4;
		len -= 4;
	}
	
	// Handle the last few bytes of the input array

	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	        h *= m;
	};

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 
 
/* A special ultra-optimized versions that knows they are hashing exactly
 * 3, 2 or 1 word(s).
 */

static inline u32 jhash_3words(u32 a, u32 b, u32 c, u32 initval)
{
	// 'm' and 'r' are mixing constants generated offline.
	// They're not really 'magic', they just happen to work well.

	const unsigned int m = 0x5bd1e995;

	// Initialize the hash to a 'random' value

	unsigned int h = initval ^ 3;

	h ^= a << 16;
	h ^= b << 8;
	h ^= c;
	h *= m;

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
} 
 
static inline u32 jhash_2words(u32 a, u32 b, u32 initval)
{
	//return jhash_3words(a, b, 0x5bd1e995, initval);
	
	const unsigned int m = 0x5bd1e995;

	// Initialize the hash to a 'random' value

	unsigned int h = initval ^ 2;

	h ^= a << 8;
	h ^= b;
	h *= m;

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

static inline u32 jhash_1word(u32 a, u32 initval)
{
	//return jhash_3words(a, 0x5bd1e995, 0x5bd1e995, initval);
	
	const unsigned int m = 0x5bd1e995;

	// Initialize the hash to a 'random' value

	unsigned int h = initval ^ 1;

	h ^= a;
	h *= m;

	// Do a few final mixes of the hash to ensure the last few
	// bytes are well-incorporated.

	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;

	return h;
}

#endif /* _LINUX_JHASH_H */
