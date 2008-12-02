/*
 * pwent.c - This file is part of the libc-8086/pwd package for ELKS,
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
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
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
 * setpwent(), endpwent(), and getpwent() are included in the same object
 * file, since one cannot be used without the other two, so it makes sense to
 * link them all in together.
 */

/* file descriptor for the password file currently open */
static int pw_fd = -1;

void setpwent(void)
{
    LOCK;
    if (pw_fd != -1)
	close(pw_fd);

    pw_fd = open(_PATH_PASSWD, O_RDONLY);
    UNLOCK;
}

void endpwent(void)
{
    LOCK;
    if (pw_fd != -1)
	close(pw_fd);
    pw_fd = -1;
    UNLOCK;
}

int getpwent_r (struct passwd *password, char *buff, 
	size_t buflen, struct passwd **crap)
{
    LOCK;
    if (pw_fd != -1 && __getpwent_r(password, buff, buflen, pw_fd) != -1) {
	UNLOCK;
	return 0;
    }
    UNLOCK;
    return -1;
}

struct passwd *getpwent(void)
{
    static char line_buff[PWD_BUFFER_SIZE];
    static struct passwd pwd;

    if (getpwent_r(&pwd, line_buff, sizeof(line_buff), NULL) != -1) {
	return &pwd;
    }
    return NULL;
}

