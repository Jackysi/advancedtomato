/*
 * dnsr_vx.h - DNS relay vxWorks porting specific module. It includes 
 * vxWorks missing interfaces that are found in unix/linux environment.
 *
 * Copyright 2005, Broadcom Corporation
 * All Rights Reserved.
 * 
 * THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
 * KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
 * SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
 *
 * $Id: dnsr_vx.h,v 1.1.1.7 2005/03/07 07:31:13 kanki Exp $
 */

#ifndef __dnsr_vx_h__
#define __dnsr_vx_h__

/* logging utilities */
#include <logLib.h>

#define LOG_DEBUG	0
#define LOG_INFO	1
#define LOG_WARNING	2
#define LOG_ERR	3
#define LOG_CRIT	4

#define openlog(ident, option, facility)
int syslog(int level, char *format, ...);
#define closelog()

/* inet utilities */
#define INADDRSZ	sizeof(struct in_addr)
#define INET_ADDRSTRLEN	16
#define IF_NAMESIZE IFNAMSIZ
#define socklen_t int

const char* inet_ntop(int af, const void *src, char *dst, size_t size);
int inet_pton(int af, const char *src, void *dst);

/* time utilities */
#include "sys/times.h"

/* defined in src/vxWorks/target/config/bcm47xx/router/unix.c */
int gettimeofday(struct timeval *tv, struct timezone *tz);
int settimeofday(const struct timeval *tv , const struct timezone *tz);

/* task utilities */
#include <taskLib.h>

#define getpid taskIdSelf
#define die dnsr_die

#endif	/* #ifndef __log_vx_h__ */
