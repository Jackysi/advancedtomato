/*****************************************************************************\
*  _  _       _          _              ___                                   *
* | || | ___ | |_  _ __ | | _  _  __ _ |_  )                                  *
* | __ |/ _ \|  _|| '_ \| || || |/ _` | / /                                   *
* |_||_|\___/ \__|| .__/|_| \_,_|\__, |/___|                                  *
*                 |_|            |___/                                        *
\*****************************************************************************/

#include <string.h>
#include <stdlib.h>

#include "mem_utils.h"
#include "parser_utils.h"

char *dup_line(char *start, char **nptr) {
	char *ptr, *rv;
	
	ptr = strchr(start, '\n');
	if (ptr == NULL)
		return NULL;
	
	rv = xmalloc(ptr - start + 1);
	memcpy(rv, start, ptr - start);
	rv[ptr-start] = '\0';
	
	if (nptr != NULL)
		*nptr = ptr + 1;
	
	return rv;
}

char *dup_token(char *start, char **nptr, int (*isdelimiter)(int)) {
	char *ptr, *rv;
	
	while (isdelimiter(*start) && *start)
		start++;
	
	ptr = start;
	
	while (!isdelimiter(*ptr) && *ptr)
		ptr++;
	
	if (ptr == start)
		return NULL;
	
	rv = xmalloc(ptr - start + 1);
	memcpy(rv, start, ptr - start);
	rv[ptr-start] = '\0';
	
	if (nptr != NULL) {
		while (isdelimiter(*ptr))
			ptr++;
		*nptr = ptr ;
	}
	
	return rv;
}

char *dup_token_r(char *start, char *start_string, char **nptr, int (*isdelimiter)(int)) {
	char *ptr, *rv;
	
	if (start <= start_string)
		return NULL;
	
	while (isdelimiter(*start) && (start > start_string))
		start--;
	
	if (start < start_string)
		start = start_string;
	
	ptr = start;
	
	while (!isdelimiter(*ptr) && (ptr > start_string))
		ptr--;
	
	if (ptr <= start_string)
		ptr = start_string;
	else
		ptr++;
	
	rv = xmalloc(start - ptr + 2);
	memcpy(rv, ptr, start - ptr + 1);
	rv[start - ptr + 1] = '\0';
	
	if (nptr != NULL) {
		ptr--;
		while ((ptr > start_string) && isdelimiter(*ptr))
			ptr--;
		
		if (ptr < start_string)
			ptr = start_string;
		
		*nptr = ptr;
	}
	
	return rv;
}
