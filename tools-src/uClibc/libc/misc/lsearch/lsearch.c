/*
 * This file lifted in toto from 'Dlibs' on the atari ST  (RdeBath)
 *
 * 
 *    Dale Schumacher                         399 Beacon Ave.
 *    (alias: Dalnefre')                      St. Paul, MN  55104
 *    dal@syntel.UUCP                         United States of America
 *  "It's not reality that's important, but how you perceive things."
 */

#include <string.h>
#include <stdio.h>

char *lfind(key, base, num, size, cmp)
register char *key, *base;
unsigned int *num;
register unsigned int size;
register int (*cmp) ();
{
	register int n = *num;

	while (n--) {
		if ((*cmp) (base, key) == 0)
			return (base);
		base += size;
	}
	return (NULL);
}

char *lsearch(key, base, num, size, cmp)
char *key, *base;
register unsigned int *num;
register unsigned int size;
int (*cmp) ();
{
	register char *p;

	if ((p = lfind(key, base, num, size, cmp)) == NULL) {
		p = memcpy((base + (size * (*num))), key, size);
		++(*num);
	}
	return (p);
}
