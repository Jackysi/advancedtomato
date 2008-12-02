/*
 * BK Id: SCCS/s.byteorder.h 1.8 10/11/01 13:02:49 trini
 */
#ifndef _PPC_BYTEORDER_H
#define _PPC_BYTEORDER_H

#include <asm/types.h>

#ifdef __GNUC__
#ifdef __KERNEL__

extern __inline__ unsigned ld_le16(const volatile unsigned short *addr)
{
	unsigned val;

	__asm__ __volatile__ ("lhbrx %0,0,%1" : "=r" (val) : "r" (addr), "m" (*addr));
	return val;
}

extern __inline__ void st_le16(volatile unsigned short *addr, const unsigned val)
{
	__asm__ __volatile__ ("sthbrx %1,0,%2" : "=m" (*addr) : "r" (val), "r" (addr));
}

extern __inline__ unsigned ld_le32(const volatile unsigned *addr)
{
	unsigned val;

	__asm__ __volatile__ ("lwbrx %0,0,%1" : "=r" (val) : "r" (addr), "m" (*addr));
	return val;
}

extern __inline__ void st_le32(volatile unsigned *addr, const unsigned val)
{
	__asm__ __volatile__ ("stwbrx %1,0,%2" : "=m" (*addr) : "r" (val), "r" (addr));
}

/* alas, egcs sounds like it has a bug in this code that doesn't use the
   inline asm correctly, and can cause file corruption. Until I hear that
   it's fixed, I can live without the extra speed. I hope. */

/* The same, but returns converted value from the location pointer by addr. */
#define __arch__swab16p(addr) ld_le16(addr)
#define __arch__swab32p(addr) ld_le32(addr)

/* The same, but do the conversion in situ, ie. put the value back to addr. */
#define __arch__swab16s(addr) st_le16(addr,*addr)
#define __arch__swab32s(addr) st_le32(addr,*addr)

#endif /* __KERNEL__ */

#if !defined(__STRICT_ANSI__) || defined(__KERNEL__)
#  define __BYTEORDER_HAS_U64__
#  define __SWAB_64_THRU_32__
#endif

#endif /* __GNUC__ */

#include <linux/byteorder/big_endian.h>

#endif /* _PPC_BYTEORDER_H */
