/*
 *	$Id: io_dc.c,v 1.1.1.4 2003/10/14 08:07:47 sparq Exp $
 *	I/O routines for SEGA Dreamcast
 */

#include <asm/io.h>
#include <asm/machvec.h>

unsigned long dreamcast_isa_port2addr(unsigned long offset)
{
	return offset + 0xa0000000;
}
