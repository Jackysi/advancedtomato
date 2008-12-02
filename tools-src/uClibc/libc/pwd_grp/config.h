/*
 * config.h - This file is part of the libc-8086/grp package for ELKS,
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


#ifndef _CONFIG_GRP_H
#define _CONFIG_GRP_H

#include <features.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>

#define PWD_BUFFER_SIZE 256


/* These are used internally to uClibc */
extern struct group *__getgrent(int grp_fd, char *line_buff, char **members);
extern int __getpwent_r(struct passwd * passwd, char * line_buff, 
	size_t buflen, int pwd_fd);
extern int __getspent_r(struct spwd * spwd, char * line_buff, 
	size_t buflen, int spwd_fd);
extern int __sgetspent_r(const char * string, struct spwd * spwd, 
	char * line_buff, size_t buflen);


#endif /* !_CONFIG_GRP_H */
