/* vi: set sw=4 ts=4: */
/* strxfrm for uClibc
 *
 * Copyright (C) 2002 by Erik Andersen <andersen@uclibc.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Library General Public License
 * for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <string.h>

size_t strxfrm(char *dst, const char *src, size_t len)
{
	size_t length;
	register char *ptr1, *ptr2;

	length = len;
	ptr1 = (char *) dst;
	ptr2 = (char *) src;
	while (length--) {
		if (*ptr2)
			*ptr1++ = *ptr2++;
		else
			*ptr1++ = '\0';
	}
	/* The first while loop should have done much of the heavy
	 * lifting for us.  This second look will finish the job if 
	 * that is necessary */
	while (*ptr2)
		ptr2++;
	length = (ptr2 - src);

	if (length<len)
		return(length);
	return(len);
}
