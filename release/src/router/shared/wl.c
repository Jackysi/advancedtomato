/*
 * Wireless network adapter utilities
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wl.c,v 1.1.1.9 2005/03/07 07:31:20 kanki Exp $
 */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <typedefs.h>
#include <wlutils.h>

int
wl_probe(char *name)
{
	int ret, val;
	
#if defined(linux)
	char buf[DEV_TYPE_LEN];
	if ((ret = wl_get_dev_type(name, buf, DEV_TYPE_LEN)) < 0)
		return ret;
	/* Check interface */
	if (strncmp(buf, "wl", 2))
		return -1;
#else
	/* Check interface */
	if ((ret = wl_ioctl(name, WLC_GET_MAGIC, &val, sizeof(val))))
		return ret;
#endif
	if ((ret = wl_ioctl(name, WLC_GET_VERSION, &val, sizeof(val))))
		return ret;
	if (val > WLC_IOCTL_VERSION)
		return -1;

	return ret;
}

int
wl_set_val(char *name, char *var, void *val, int len)
{
	char buf[WLC_IOCTL_SMLEN];
	int buf_len;

	/* check for overflow */
	if ((buf_len = strlen(var)) + 1 + len > sizeof(buf))
		return -1;
	
	strcpy(buf, var);
	buf_len += 1;

	/* append int value onto the end of the name string */
	memcpy(&buf[buf_len], val, len);
	buf_len += len;

	return wl_ioctl(name, WLC_SET_VAR, buf, buf_len);
}

int
wl_get_val(char *name, char *var, void *val, int len)
{
	char buf[WLC_IOCTL_SMLEN];
	int ret;

	/* check for overflow */
	if (strlen(var) + 1 > sizeof(buf) || len > sizeof(buf))
		return -1;
	
	strcpy(buf, var);
	if ((ret = wl_ioctl(name, WLC_GET_VAR, buf, sizeof(buf))))
		return ret;

	memcpy(val, buf, len);
	return 0;
}

int
wl_set_int(char *name, char *var, int val)
{
	return wl_set_val(name, var, &val, sizeof(val));
}

int
wl_get_int(char *name, char *var, int *val)
{
	return wl_get_val(name, var, val, sizeof(*val));
}

