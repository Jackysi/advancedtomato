/*
 * Broadcom wireless network adapter utility functions
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: wlutils.h,v 1.1.1.10 2005/03/07 07:31:20 kanki Exp $
 */

#ifndef _wlutils_h_
#define _wlutils_h_

#include <typedefs.h>
#include <wlioctl.h>

/*
 * Pass a wlioctl request to the specified interface.
 * @param	name	interface name
 * @param	cmd	WLC_GET_MAGIC <= cmd < WLC_LAST
 * @param	buf	buffer for passing in and/or receiving data
 * @param	len	length of buf
 * @return	>= 0 if successful or < 0 otherwise
 */
extern int wl_ioctl(char *name, int cmd, void *buf, int len);

/*
 * Get device type.
 * @param	name	interface name
 * @param	buf	buffer for passing in and/or receiving data
 * @param	len	length of buf
 * @return	>= 0 if successful or < 0 otherwise
 */
#define DEV_TYPE_LEN 3
extern int wl_get_dev_type(char *name, void *buf, int len);

/*
 * Get the MAC (hardware) address of the specified interface.
 * @param	name	interface name
 * @param	hwaddr	6-byte buffer for receiving address
 * @return	>= 0 if successful or < 0 otherwise
 */
extern int wl_hwaddr(char *name, unsigned char *hwaddr);

/*
 * Probe the specified interface.
 * @param	name	interface name
 * @return	>= 0 if a Broadcom wireless device or < 0 otherwise
 */
extern int wl_probe(char *name);

/*
 * Set/Get named variable.
 * @param	name	interface name
 * @param	var	variable name
 * @param	val	variable value/buffer
 * @param	len	variable value/buffer length
 * @return	success == 0, failure != 0
 */
extern int wl_set_val(char *name, char *var, void *val, int len);
extern int wl_get_val(char *name, char *var, void *val, int len);
extern int wl_set_int(char *name, char *var, int val);
extern int wl_get_int(char *name, char *var, int *val);

#endif /* _wlutils_h_ */
