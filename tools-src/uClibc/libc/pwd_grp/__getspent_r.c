/* __getspent_r.c - Based on __getpwent_r.c
 * 
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "config.h"


int __getspent_r(struct spwd * spwd, char * line_buff, size_t buflen, int spwd_fd)
{
    char *endptr;
    int line_len;

    /* We use the restart label to handle malformatted lines */
restart:
    /* Read the shadow line into the buffer using a minimum of syscalls. */
    if ((line_len = read(spwd_fd, line_buff, buflen)) <= 0)
	return -1;
    endptr = strchr(line_buff, '\n');
    if (endptr != NULL)
	lseek(spwd_fd, (long) (1 + endptr - (line_buff + line_len)), SEEK_CUR);
    else {
	/* The line is too long - skip it. :-\ */
	do {
	    if ((line_len = read(spwd_fd, line_buff, buflen)) <= 0)
		return -1;
	} while (!(endptr = strchr(line_buff, '\n')));
	lseek(spwd_fd, (long) (endptr - line_buff) - line_len + 1, SEEK_CUR);
	goto restart;
    }

    if (__sgetspent_r(line_buff, spwd, line_buff, buflen) < 0)
	goto restart;

    return 0;
}
