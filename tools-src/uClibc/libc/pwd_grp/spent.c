/*
 * spent.c - Based on pwent.c
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

/*
 * setspent(), endspent(), and getspent() are included in the same object
 * file, since one cannot be used without the other two, so it makes sense to
 * link them all in together.
 */

/* file descriptor for the password file currently open */
static int spwd_fd = -1;

void setspent(void)
{
    LOCK;
    if (spwd_fd != -1)
	close(spwd_fd);
    spwd_fd = open(_PATH_SHADOW, O_RDONLY);
    UNLOCK;
}

void endspent(void)
{
    LOCK;
    if (spwd_fd != -1)
	close(spwd_fd);
    spwd_fd = -1;
    UNLOCK;
}

int getspent_r (struct spwd *spwd, char *buff, 
	size_t buflen, struct spwd **crap)
{
    LOCK;
    if (spwd_fd != -1 && __getspent_r(spwd, buff, buflen, spwd_fd) != -1) {
	UNLOCK;
	return 0;
    }
    UNLOCK;
    return -1;
}

struct spwd *getspent(void)
{
    static char line_buff[PWD_BUFFER_SIZE];
    static struct spwd spwd;

    LOCK;
    if (getspent_r(&spwd, line_buff, sizeof(line_buff), NULL) != -1) {
	UNLOCK;
	return &spwd;
    }
    UNLOCK;
    return NULL;
}

