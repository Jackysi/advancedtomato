/*
 * Should be called bcmsromstubs.c .
 *
 * $Id: sromstubs.c,v 1.1.1.1 2004/08/26 06:56:18 honor Exp $
 */

#include <typedefs.h>
#include <osl.h>
#include <bcmsrom.h>

int
srom_var_init(void *sbh, uint bus, void *curmap, void *osh, char **vars, int *count)
{
	return 0;
}

int
srom_read(uint bus, void *curmap, void *osh, uint byteoff, uint nbytes, uint16 *buf)
{
	return 0;
}

int
srom_write(uint bus, void *curmap, void *osh, uint byteoff, uint nbytes, uint16 *buf)
{
	return 0;
}
