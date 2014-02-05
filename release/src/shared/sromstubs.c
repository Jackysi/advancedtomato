/*
 * Should be called bcmsromstubs.c .
 *
 * $Id$
 */

#include <typedefs.h>
#include <osl.h>
#include <bcmutils.h>
#include <sbutils.h>
#include <bcmsrom.h>

int
srom_var_init(sb_t *sbh, uint bus, void *curmap, osl_t *osh, char **vars, uint *count)
{
	return 0;
}

int
srom_read(sb_t *sbh, uint bus, void *curmap, osl_t *osh, uint byteoff, uint nbytes, uint16 *buf)
{
	return 0;
}

int
srom_write(sb_t *sbh, uint bus, void *curmap, osl_t *osh, uint byteoff, uint nbytes, uint16 *buf)
{
	return 0;
}
