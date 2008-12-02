/*
 * __getgrent.c - This file is part of the libc-8086/grp package for ELKS,
 * Copyright (C) 1995, 1996 Nat Friedman <ndf@linux.mit.edu>.
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

#include <features.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"


#ifdef __UCLIBC_HAS_THREADS__
#include <pthread.h>
/* This function should always be called under lock, so we
 * do not lock things in here... */
pthread_mutex_t __getgrent_lock = PTHREAD_MUTEX_INITIALIZER;
#endif

/*
 * This is the core group-file read function.  It behaves exactly like
 * getgrent() except that it is passed a file descriptor.  getgrent()
 * is just a wrapper for this function.
 */
struct group *__getgrent(int grp_fd, char *line_buff, char **members)
{
    short line_index;
    short buff_size;
    static struct group group;
    register char *ptr;
    char *field_begin;
    short member_num;
    char *endptr;
    int line_len;


    /* We use the restart label to handle malformatted lines */
restart:
    line_index = 0;
    buff_size = 256;

    line_buff = realloc(line_buff, buff_size);
    while (1) {
	if ((line_len = read(grp_fd, line_buff + line_index,
			buff_size - line_index)) <= 0) {
	    return NULL;
	}
	field_begin = strchr(line_buff, '\n');
	if (field_begin != NULL) {
	    lseek(grp_fd,
		    (long) (1 + field_begin -
			    (line_len + line_index + line_buff)), SEEK_CUR);
	    *field_begin = '\0';
	    if (*line_buff == '#' || *line_buff == ' '
		    || *line_buff == '\n' || *line_buff == '\t')
		goto restart;
	    break;
	} else {				/* Allocate some more space */

	    line_index = buff_size;
	    buff_size += 256;
	    line_buff = realloc(line_buff, buff_size);
	}
    }

    /* Now parse the line */
    group.gr_name = line_buff;
    ptr = strchr(line_buff, ':');
    if (ptr == NULL)
	goto restart;
    *ptr++ = '\0';

    group.gr_passwd = ptr;
    ptr = strchr(ptr, ':');
    if (ptr == NULL)
	goto restart;
    *ptr++ = '\0';

    field_begin = ptr;
    ptr = strchr(ptr, ':');
    if (ptr == NULL)
	goto restart;
    *ptr++ = '\0';

    group.gr_gid = (gid_t) strtoul(field_begin, &endptr, 10);
    if (*endptr != '\0')
	goto restart;

    member_num = 0;
    field_begin = ptr;

    if (members != NULL)
	free(members);
    members = (char **) malloc((member_num + 1) * sizeof(char *));
    for ( ; field_begin && *field_begin != '\0'; field_begin = ptr) {
	if ((ptr = strchr(field_begin, ',')) != NULL)
	    *ptr++ = '\0';
	members[member_num++] = field_begin;
	members = (char **) realloc(members,
		(member_num + 1) * sizeof(char *));
    }
    members[member_num] = NULL;

    group.gr_mem = members;
    return &group;
}
