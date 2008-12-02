/*
 * getspnam.c - Based on getpwnam.c
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
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "config.h"

#ifdef __UCLIBC_HAS_THREADS__
#include <pthread.h>
static pthread_mutex_t mylock = PTHREAD_MUTEX_INITIALIZER;
# define LOCK   pthread_mutex_lock(&mylock)
# define UNLOCK pthread_mutex_unlock(&mylock);
#else       
# define LOCK
# define UNLOCK
#endif      

int getspnam_r (const char *name, struct spwd *spwd,
	char *buff, size_t buflen, struct spwd **crap)
{
    int spwd_fd;

    if (name == NULL) {
	__set_errno(EINVAL);
	return -1;
    }

    if ((spwd_fd = open(_PATH_SHADOW, O_RDONLY)) < 0)
	return -1;

    while (__getspent_r(spwd, buff, buflen, spwd_fd) != -1)
	if (!strcmp(spwd->sp_namp, name)) {
	    close(spwd_fd);
	    return 0;
	}

    close(spwd_fd);
    return -1;
}

struct spwd *getspnam(const char *name)
{
    static char line_buff[PWD_BUFFER_SIZE];
    static struct spwd spwd;

    LOCK;
    if (getspnam_r(name, &spwd, line_buff,  sizeof(line_buff), NULL) != -1) {
	UNLOCK;
	return &spwd;
    }
    UNLOCK;
    return NULL;
}

