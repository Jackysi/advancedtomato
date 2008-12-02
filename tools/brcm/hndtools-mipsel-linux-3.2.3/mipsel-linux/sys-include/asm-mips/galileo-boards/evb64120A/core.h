/*
 * This header file contains simple Read/Write macros for addressing the SDRAM,
 * devices, GT`s internal registers and PCI (using the PCI`s address space).
 *
 * Copyright Galileo Technology.
 */
#ifndef __INCcoreh
#define __INCcoreh

#include <linux/types.h>
#include <asm/byteorder.h>
#include <asm/gt64120.h>

#define INTERNAL_REG_BASE_ADDR 0x14000000

#define NO_BIT          0x00000000
#define BIT0            0x00000001
#define BIT1            0x00000002
#define BIT2            0x00000004
#define BIT3            0x00000008
#define BIT4            0x00000010
#define BIT5            0x00000020
#define BIT6            0x00000040
#define BIT7            0x00000080
#define BIT8            0x00000100
#define BIT9            0x00000200
#define BIT10           0x00000400
#define BIT11           0x00000800
#define BIT12           0x00001000
#define BIT13           0x00002000
#define BIT14           0x00004000
#define BIT15           0x00008000
#define BIT16           0x00010000
#define BIT17           0x00020000
#define BIT18           0x00040000
#define BIT19           0x00080000
#define BIT20           0x00100000
#define BIT21           0x00200000
#define BIT22           0x00400000
#define BIT23           0x00800000
#define BIT24           0x01000000
#define BIT25           0x02000000
#define BIT26           0x04000000
#define BIT27           0x08000000
#define BIT28           0x10000000
#define BIT29           0x20000000
#define BIT30           0x40000000
#define BIT31           0x80000000

#define _1K             0x00000400
#define _2K             0x00000800
#define _4K             0x00001000
#define _8K             0x00002000
#define _16K            0x00004000
#define _32K            0x00008000
#define _64K            0x00010000
#define _128K           0x00020000
#define _256K           0x00040000
#define _512K           0x00080000

#define _1M             0x00100000
#define _2M             0x00200000
#define _3M             0x00300000
#define _4M             0x00400000
#define _5M             0x00500000
#define _6M             0x00600000
#define _7M             0x00700000
#define _8M             0x00800000
#define _9M             0x00900000
#define _10M            0x00a00000
#define _11M            0x00b00000
#define _12M            0x00c00000
#define _13M            0x00d00000
#define _14M            0x00e00000
#define _15M            0x00f00000
#define _16M            0x01000000

typedef enum _bool{false,true} bool;

#ifndef NULL
#define NULL 0
#endif

/* The two following defines are according to MIPS architecture. */
#define NONE_CACHEABLE			0xa0000000
#define MIPS_CACHEABLE			0x80000000

/* Read/Write to/from GT`s internal registers */
#define GT_REG_READ(offset, pData)					\
do {									\
	*pData = (*((u32 *)(NONE_CACHEABLE |				\
		INTERNAL_REG_BASE_ADDR | (offset))));			\
	*pData = cpu_to_le32(*pData);					\
} while(0)

#define GT_REG_WRITE(offset, data)					\
	(*((u32 *)(NONE_CACHEABLE | INTERNAL_REG_BASE_ADDR |		\
		(offset))) = cpu_to_le32(data))

#define VIRTUAL_TO_PHY(y)	((u32)(y) & (u32)0x5fffffff)
#define PHY_TO_VIRTUAL(y)	((u32)(y) | NONE_CACHEABLE)

/* Write 32/16/8 bit Non-Cache-able */
#define WRITE_CHAR(address, data)					\
	(*((u8 *)((address) | NONE_CACHEABLE)) = (data))
#define WRITE_SHORT(address, data)					\
	(*((u16 *)((address) | NONE_CACHEABLE)) = (u16) data)
#define WRITE_WORD(address, data)					\
	(*((u32 *)((address) | NONE_CACHEABLE)) = (u32) data)

/* Write 32/16/8 bits Cacheable */
#define WRITE_CHAR_CACHEABLE(address, data)				\
	(*((u8 *)((address) | MIPS_CACHEABLE)) = (data))

#define WRITE_SHORT_CACHEABLE(address, data)				\
	(*((u16 *)((address) | MIPS_CACHEABLE)) = (u16) data)

#define WRITE_WORD_CACHEABLE(address, data)				\
	(*((u32 *)((address) | MIPS_CACHEABLE )) = (u32) data)

/* Read 32/16/8 bits NonCacheable - returns data in variable. */
#define READ_CHAR(address,pData)					\
	(*(pData) = *((u8 *)((address) | NONE_CACHEABLE)))

#define READ_SHORT(address,pData)					\
	(*(pData) = *((u16 *)((address) | NONE_CACHEABLE)))

#define READ_WORD(address,pData)					\
	(*(pData) = *((u32 *)((address) | NONE_CACHEABLE)))

/* Read 32/16/8 bit NonCacheable - returns data direct. */
#define READCHAR(address)						\
	(*((u8 *)((address) | NONE_CACHEABLE)))

#define READSHORT(address)						\
	(*((u16 *)((address) | NONE_CACHEABLE)))

#define READWORD(address)						\
        (*((u32 *)((address) | NONE_CACHEABLE)))

/* Read 32/16/8 bit Cacheable - returns data in variable. */
#define READ_CHAR_CACHEABLE(address,pData)				\
        (*(pData) = *((u8 *)((address) | MIPS_CACHEABLE)))

#define READ_SHORT_CACHEABLE(address,pData)				\
        (*(pData) = *((u16 *)((address) | MIPS_CACHEABLE)))
#define READ_WORD_CACHEABLE(address,pData)				\
        (*(pData) = *((u32 *)((address) | MIPS_CACHEABLE)))

/* Read 32/16/8 bit Cacheable - returns data direct. */
#define READCHAR_CACHEABLE(address)					\
	(*((u8 *)((address) | MIPS_CACHEABLE)))

#define READSHORT_CACHEABLE(address)					\
	(*((u16 *)((address) | MIPS_CACHEABLE)))

#define READWORD_CACHEABLE(address)					\
	(*((u32 *)((address) | MIPS_CACHEABLE)))

/*
 * SET_REG_BITS(regOffset,bits) -
 * gets register offset and bits: a 32bit value. It set to logic '1' in the
 * internal register the bits which given as an input example:
 * SET_REG_BITS(0x840,BIT3 | BIT24 | BIT30) - set bits: 3,24 and 30 to logic
 * '1' in register 0x840 while the other bits stays as is.
 */
#define SET_REG_BITS(regOffset,bits)					\
	(*(u32*)(NONE_CACHEABLE | INTERNAL_REG_BASE_ADDR |		\
		(regOffset)) |= (u32)cpu_to_le32(bits))

/*
 * RESET_REG_BITS(regOffset,bits) -
 * gets register offset and bits: a 32bit value. It set to logic '0' in the
 * internal register the bits which given as an input example:
 * RESET_REG_BITS(0x840,BIT3 | BIT24 | BIT30) - set bits: 3,24 and 30 to logic
 * '0' in register 0x840 while the other bits stays as is.
 */
#define RESET_REG_BITS(regOffset,bits)					\
	(*(u32 *)(NONE_CACHEABLE | INTERNAL_REG_BASE_ADDR |		\
		(regOffset)) &= ~((u32)cpu_to_le32(bits)))

#endif /* __INCcoreh */
