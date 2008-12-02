/*
 * getpwuid.c - This file is part of the libc-8086/pwd package for ELKS,
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
#include <stdlib.h>
#include <unistd.h>
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

int getpwuid_r (uid_t uid, struct passwd *password,
	char *buff, size_t buflen, struct passwd **crap)
{
    int passwd_fd;

    if ((passwd_fd = open(_PATH_PASSWD, O_RDONLY)) < 0)
	return -1;

    while (__getpwent_r(password, buff, buflen, passwd_fd) != -1)
	if (password->pw_uid == uid) {
	    close(passwd_fd);
	    return 0;
	}

    close(passwd_fd);
    return -1;
}

struct passwd *getpwuid(uid_t uid)
{
    /* file descriptor for the password file currently open */
    static char line_buff[PWD_BUFFER_SIZE];
    static struct passwd pwd;

    LOCK;
    if (getpwuid_r(uid, &pwd, line_buff,  sizeof(line_buff), NULL) != -1) {
	UNLOCK;
	return &pwd;
    }
    UNLOCK;
    return NULL;
}

