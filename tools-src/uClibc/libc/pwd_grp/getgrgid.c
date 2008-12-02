/*
 * getgrgid.c - This file is part of the libc-8086/grp package for ELKS,
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

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <paths.h>
#include "config.h"


#ifdef __UCLIBC_HAS_THREADS__
#include <pthread.h>
extern pthread_mutex_t __getgrent_lock;
# define LOCK   pthread_mutex_lock(&__getgrent_lock)
# define UNLOCK pthread_mutex_unlock(&__getgrent_lock);
#else
# define LOCK
# define UNLOCK
#endif
static char *line_buff = NULL;
static char **members = NULL;


struct group *getgrgid(const gid_t gid)
{
    struct group *group;
    int grp_fd;

    if ((grp_fd = open(_PATH_GROUP, O_RDONLY)) < 0)
	return NULL;

    LOCK;
    while ((group = __getgrent(grp_fd, line_buff, members)) != NULL)
	if (group->gr_gid == gid) {
	    close(grp_fd);
	    UNLOCK;
	    return group;
	}

    close(grp_fd);
    UNLOCK;
    return NULL;
}

#if 0
/* Search for an entry with a matching group ID.  */
int getgrgid_r (gid_t gid, struct group *resultbuf, char *buffer, 
	size_t buflen, struct group **result)
{

}
#endif
