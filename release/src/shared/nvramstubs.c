/*
 * Stubs for NVRAM functions for platforms without flash
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: nvramstubs.c,v 1.1.1.2 2005/03/07 07:31:12 kanki Exp $
 */

#include <osl.h>
#include <bcmnvram.h>

int
nvram_init(void *sbh)
{
	return 0;
}

void
nvram_exit(void)
{
}

char *
nvram_get(const char *name)
{
	return (char *) 0;
}

int
nvram_set(const char *name, const char *value)
{
	return 0;
}

int
nvram_unset(const char *name)
{
	return 0;
}

int
nvram_commit(void)
{
	return 0;
}

int
nvram_getall(char *buf, int count)
{
	return 0;
}
