/*
 * CGI helper functions
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 *
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: cgi.c,v 1.10 2005/03/07 08:35:32 kanki Exp $
 */

#include "tomato.h"

#define __USE_GNU
#include <search.h>


/* CGI hash table */
static struct hsearch_data htab = { .table = NULL };

static void unescape(char *s)
{
	unsigned int c;

	while ((s = strpbrk(s, "%+"))) {
		if (*s == '%') {
			sscanf(s + 1, "%02x", &c);
			*s++ = (char) c;
			strcpy(s, s + 2);
		}
		else if (*s == '+') {
			*s++ = ' ';
		}
	}
}

int str_replace(char* str,char* str_src, char* str_des){
	char *ptr=NULL;
	char buff[10240];
	char buff2[10240];
	int i = 0;

	if(str != NULL){
		strcpy(buff2, str);
	}else{
		printf("str_replace err!\n");
		return -1;
	}
	memset(buff, 0x00, sizeof(buff));
	while((ptr = strstr( buff2, str_src)) !=0){
		if(ptr-buff2 != 0) memcpy(&buff[i], buff2, ptr - buff2);
		memcpy(&buff[i + ptr - buff2], str_des, strlen(str_des));
		i += ptr - buff2 + strlen(str_des);
		strcpy(buff2, ptr + strlen(str_src));
	}
	strcat(buff,buff2);
	strcpy(str,buff);
	return 0;
}

char *webcgi_get(const char *name)
{
	ENTRY e, *ep;

	if (!htab.table) return NULL;

	e.key = (char *)name;
	hsearch_r(e, FIND, &ep, &htab);

//	cprintf("%s=%s\n", name, ep ? ep->data : "(null)");

	return ep ? ep->data : NULL;
}

void webcgi_set(char *name, char *value)
{
	ENTRY e, *ep;

	if (!htab.table) {
		hcreate_r(16, &htab);
	}

	e.key = name;
	hsearch_r(e, FIND, &ep, &htab);
	if (ep) {
		ep->data = value;
	}
	else {
		e.data = value;
		hsearch_r(e, ENTER, &ep, &htab);
	}
}

void webcgi_init(char *query)
{
	int nel;
	char *q, *end, *name, *value;

	if (htab.table) hdestroy_r(&htab);
	if (query == NULL) return;

//	cprintf("query = %s\n", query);
	
	end = query + strlen(query);
	q = query;
	nel = 1;
	while (strsep(&q, "&;")) {
		nel++;
	}
	hcreate_r(nel, &htab);

	for (q = query; q < end; ) {
		value = q;
		q += strlen(q) + 1;
		str_replace(value, "%u", "~u");
		unescape(value);
		name = strsep(&value, "=");
		if (value) webcgi_set(name, value);
	}
}
