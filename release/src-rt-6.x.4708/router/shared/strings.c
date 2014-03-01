/*

	Tomato Firmware
	Copyright (C) 2006-2009 Jonathan Zarate

*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdarg.h>

#include <bcmnvram.h>
#include "shutils.h"
#include "shared.h"

const char *find_word(const char *buffer, const char *word)
{
	const char *p, *q;
	int n;

	n = strlen(word);
	p = buffer;
	while ((p = strstr(p, word)) != NULL) {
		if ((p == buffer) || (*(p - 1) == ' ') || (*(p - 1) == ',')) {
			q = p + n;
			if ((*q == ' ') || (*q == ',') || (*q == 0)) {
				return p;
			}
		}
		++p;
	}
	return NULL;
}

/*
static void add_word(char *buffer, const char *word, int max)
{
	if ((*buffer != 0) && (buffer[strlen(buffer) - 1] != ' '))
		strlcat(buffer, " ", max);
	strlcat(buffer, word, max);
}
*/

int remove_word(char *buffer, const char *word)
{
	char *p;
	char *q;

	if ((p = strstr(buffer, word)) == NULL) return 0;
	q = p;
	p += strlen(word);
	while (*p == ' ') ++p;
	while ((q > buffer) && (*(q - 1) == ' ')) --q;
	if (*q != 0) *q++ = ' ';
	strcpy(q, p);

	return 1;
}

/* Hyzoom bwq518 */
char * trimstr(char *str)
{
	int i,j, len, count;

	if (str == NULL) return NULL;
	len = strlen(str);
	if (len == 0) return str;

	for (i = 0; i < len; i ++)
	{
		if ((str[i] != ' ') && (str[i] != '\t'))  break;
	}
	if (i == (len - 1))
	{
		*str = '\0';
		return str;
	}

	for ( j = len - 1; j >= 0 ; j --)
	{
		if ((str[j] != ' ') && (str[j] != '\t')) break;
	}
	if ( (j == 0) || (j <= i))
	{
		*str = '\0';
		return str;
	}
	count = j - i + 1 ;
	for ( j = 0; j < count; j ++)
	{
		*(str + j) = *(str + j + i);
	}
	*(str + count) = '\0';
	return str;
}
