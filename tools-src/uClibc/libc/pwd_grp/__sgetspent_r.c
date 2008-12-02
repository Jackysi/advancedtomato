/*
 * __sgetspent_r.c - Based on __getpwent_r.c
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
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "config.h"


int __sgetspent_r(const char * string, struct spwd * spwd, char * line_buff, size_t buflen)
{
	char *field_begin;
	char *endptr;
	char *lstchg_ptr=NULL;
	char *min_ptr=NULL;
	char *max_ptr=NULL;
	char *warn_ptr=NULL;
	char *inact_ptr=NULL;
	char *expire_ptr=NULL;
	char *flag_ptr=NULL;
	int i;

	if (string != line_buff) {
		if (strlen(string) >= buflen)
			return -1;
		strcpy(line_buff, string);
	}

	if (*line_buff == '#' || *line_buff == ' ' || *line_buff == '\n' ||
		*line_buff == '\t')
		return -1;

	field_begin = strchr(line_buff, '\n');
	if (field_begin != NULL)
		*field_begin = '\0';

	/* We've read the line; now parse it. */
	field_begin = line_buff;
	for (i = 0; i < 9; i++) {
		switch (i) {
		case 0:
			spwd->sp_namp = field_begin;
			break;
		case 1:
			spwd->sp_pwdp = field_begin;
			break;
		case 2:
			lstchg_ptr = field_begin;
			break;
		case 3:
			min_ptr = field_begin;
			break;
		case 4:
			max_ptr = field_begin;
			break;
		case 5:
			warn_ptr = field_begin;
			break;
		case 6:
			inact_ptr = field_begin;
			break;
		case 7:
			expire_ptr = field_begin;
			break;
		case 8:
			flag_ptr = field_begin;
			break;
		}
		if (i < 8) {
			field_begin = strchr(field_begin, ':');
			if (field_begin == NULL) {
				if (i==4 || i==7)
					break;
				return -1;
			}
			*field_begin++ = '\0';
		}
	}

	if (*lstchg_ptr == '\0') {
		spwd->sp_lstchg = -1;
	} else {
		spwd->sp_lstchg = (gid_t) strtoul(lstchg_ptr, &endptr, 10);
		if (*endptr != '\0')
			return -1;
	}

	if (*min_ptr == '\0') {
		spwd->sp_min = -1;
	} else {
		spwd->sp_min = (gid_t) strtoul(min_ptr, &endptr, 10);
		if (*endptr != '\0')
			return -1;
	}

	if (*max_ptr == '\0') {
		spwd->sp_max = -1;
	} else {
		spwd->sp_max = (gid_t) strtoul(max_ptr, &endptr, 10);
		if (*endptr != '\0')
			return -1;
	}

	if (warn_ptr == NULL) {
		/* Support for old format */
		spwd->sp_warn = -1;
		spwd->sp_inact = -1;
		spwd->sp_expire = -1;
		spwd->sp_flag = 0;
	}
	else {
		if (*warn_ptr == '\0') {
			spwd->sp_warn = -1;
		} else {
			spwd->sp_warn = (gid_t) strtoul(warn_ptr, &endptr, 10);
			if (*endptr != '\0')
				return -1;
		}

		if (*inact_ptr == '\0') {
			spwd->sp_inact = -1;
		} else {
			spwd->sp_inact = (gid_t) strtoul(inact_ptr, &endptr, 10);
			if (*endptr != '\0')
				return -1;
		}

		if (*expire_ptr == '\0') {
			spwd->sp_expire = -1;
		} else {
			spwd->sp_expire = (gid_t) strtoul(expire_ptr, &endptr, 10);
			if (*endptr != '\0')
				return -1;
		}

		if (flag_ptr==NULL || *flag_ptr=='\0') {
			spwd->sp_flag = ~0ul;
		} else {
			spwd->sp_flag = (gid_t) strtoul(flag_ptr, &endptr, 10);
			if (*endptr != '\0')
				return -1;
		}
	}

	return 0;
}
