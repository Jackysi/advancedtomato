/* 
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2000-2002 Silicon Graphics, Inc. All rights reserved.
 */

#include <asm/io.h>

#ifdef CONFIG_IA64_SGI_SN1
#define mmiob sn1_mmiob
#else
#define mmiob sn2_mmiob
#endif
extern void mmiob(void);

extern void * sn_io_addr(unsigned long port); /* defined in sn[12]/iomv.c */

/**
 * sn_inb - read a byte from a port
 * @port: port to read from
 *
 * Reads a byte from @port and returns it to the caller.
 */
unsigned int
sn_inb (unsigned long port)
{
	volatile unsigned char *addr = sn_io_addr(port);
	unsigned char ret;

	ret = *addr;
	__ia64_mf_a();
	return ret;
}

/**
 * sn_inw - read a word from a port
 * @port: port to read from
 *
 * Reads a word from @port and returns it to the caller.
 */
unsigned int
sn_inw (unsigned long port)
{
	volatile unsigned short *addr = sn_io_addr(port);
	unsigned short ret;

	ret = *addr;
	__ia64_mf_a();
	return ret;
}

/**
 * sn_inl - read a word from a port
 * @port: port to read from
 *
 * Reads a word from @port and returns it to the caller.
 */
unsigned int
sn_inl (unsigned long port)
{
	volatile unsigned int *addr = sn_io_addr(port);
	unsigned int ret;

	ret = *addr;
	__ia64_mf_a();
	return ret;
}

/**
 * sn_outb - write a byte to a port
 * @port: port to write to
 * @val: value to write
 *
 * Writes @val to @port.
 */
void
sn_outb (unsigned char val, unsigned long port)
{
	volatile unsigned char *addr = sn_io_addr(port);

	*addr = val;
	mmiob();
}

/**
 * sn_outw - write a word to a port
 * @port: port to write to
 * @val: value to write
 *
 * Writes @val to @port.
 */
void
sn_outw (unsigned short val, unsigned long port)
{
	volatile unsigned short *addr = sn_io_addr(port);

	*addr = val;
	mmiob();
}

/**
 * sn_outl - write a word to a port
 * @port: port to write to
 * @val: value to write
 *
 * Writes @val to @port.
 */
void
sn_outl (unsigned int val, unsigned long port)
{
	volatile unsigned int *addr = sn_io_addr(port);

	*addr = val;
	mmiob();
}
