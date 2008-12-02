/*
 * initgroups.c - This file is part of the libc-8086/grp package for ELKS,
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

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <paths.h>
#include <stdlib.h>
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

int initgroups(__const char *user, gid_t gid)
{
    register struct group *group;

    gid_t *group_list = NULL;
    register char **tmp_mem;
    int num_groups;
    int grp_fd;


    if ((grp_fd = open(_PATH_GROUP, O_RDONLY)) < 0)
	return -1;

    num_groups = 0;
    group_list = (gid_t *) realloc(group_list, 1);
    group_list[num_groups] = gid;
    LOCK;
    while ((group = __getgrent(grp_fd, line_buff, members)) != NULL)
    {
	if (group->gr_gid != gid)
	{
	    tmp_mem = group->gr_mem;
	    while (*tmp_mem != NULL) {
		if (!strcmp(*tmp_mem, user)) {
		    num_groups++;
		    group_list = (gid_t *) realloc(group_list, num_groups *
			    sizeof(gid_t *));
		    group_list[num_groups-1] = group->gr_gid;
		}
		tmp_mem++;
	    }
	}
    }
    close(grp_fd);
    UNLOCK;
    return setgroups(num_groups, group_list);
}
