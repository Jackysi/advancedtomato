/*
 * dnsr_vx.c - DNS relay vxWorks porting specific module. It includes 
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
 * $Id: dnsr_vx.c,v 1.1.1.7 2005/03/07 07:31:13 kanki Exp $
 */

#include "vx_osl.h"
#include "dnsmasq.h"

/*
* log level in string form - corresponding to LOG_XXXXX 
* defined in the header file
*/
static char *log_level[] =
{
	"DEBUG",
	"INFO",
	"WARN",
	"ERR",
	"FATAL",
};
static int  num_log_levels = sizeof(log_level)/sizeof(log_level[0]);

/*
* current log level - only messages with equal or greater levels are printed
*/
int dnsr_log_level = LOG_WARNING;

/*
* log message to serial console
*/
int syslog(int level, char *format, ...)
{
	if (level >= dnsr_log_level)
	{
#define MAX_MSG_LEN	512
		char message[MAX_MSG_LEN+2];
		int flen, mlen;
		flen = snprintf(message, MAX_MSG_LEN, "[%s] ", level<num_log_levels?log_level[level]:"UNK");
		if (flen > 0)
		{
			va_list args;
			va_start(args, format);
			mlen = vsnprintf(&message[flen], MAX_MSG_LEN-flen, format, args);
			if (mlen > 0 && message[flen+mlen-1] != '\n')
			{
				message[flen+mlen] = '\n';
				message[flen+mlen+1] = 0;
			}
			va_end (args);
			logMsg(message, 0, 0, 0, 0, 0, 0);
		}
	}
	return OK;
}

/*
 * Copyright (C) 1996-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND INTERNET SOFTWARE CONSORTIUM
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * INTERNET SOFTWARE CONSORTIUM BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#define NS_INADDRSZ INADDRSZ

/* const char *
 * inet_ntop4(src, dst, size)
 *      format an IPv4 address
 * return:
 *      `dst' (as a const)
 * notes:
 *      (1) uses no statics
 *      (2) takes a unsigned char* not an in_addr as input
 * author:
 *      Paul Vixie, 1996.
 */
static const char *inet_ntop4(const unsigned char *src, char *dst, size_t size)
{
         static const char *fmt = "%u.%u.%u.%u";
         char tmp[sizeof "255.255.255.255"];
 
         if ((size_t)sprintf(tmp, fmt, src[0], src[1], src[2], src[3]) >= size)
         {
                 errno = ENOSPC;
                 return (NULL);
         }
         strcpy(dst, tmp);
 
         return (dst);
 }

/* char *
 * isc_net_ntop(af, src, dst, size)
 *      convert a network format address to presentation format.
 * return:
 *      pointer to presentation format address (`dst'), or NULL (see errno).
 * author:
 *      Paul Vixie, 1996.
 */
const char* inet_ntop  (int af, const void *src, char *dst, size_t size)
{
         switch (af) {
         case AF_INET:
                 return (inet_ntop4(src, dst, size));
 #ifdef AF_INET6
         case AF_INET6:
                 return (inet_ntop6(src, dst, size));
 #endif
         default:
                 errno = EAFNOSUPPORT;
                 return (NULL);
         }
         /* NOTREACHED */
}

/* int
 * inet_pton4(src, dst)
 *      like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *      1 if `src' is a valid dotted quad, else 0.
 * notice:
 *      does not touch `dst' unless it's returning 1.
 * author:
 *      Paul Vixie, 1996.
 */
static int inet_pton4(const char *src, unsigned char *dst)
{
        static const char digits[] = "0123456789";
        int saw_digit, octets, ch;
        unsigned char tmp[NS_INADDRSZ], *tp;

        saw_digit = 0;
        octets = 0;
        *(tp = tmp) = 0;
        while ((ch = *src++) != '\0' && !isspace(ch)) {
                const char *pch;

                if ((pch = strchr(digits, ch)) != NULL) {
                        unsigned int new = *tp * 10 + (pch - digits);

                        if (new > 255)
                                return (0);
                        *tp = new;
                        if (! saw_digit) {
                               if (++octets > 4)
                                        return (0);
                                saw_digit = 1;
                        }
                } else if (ch == '.' && saw_digit) {
                        if (octets == 4)
                                return (0);
                        *++tp = 0;
                        saw_digit = 0;
                } else
                        return (0);
        }
        if (octets < 4)
                return (0);
        memcpy(dst, tmp, NS_INADDRSZ);
        return (1);
}

/* int
 * isc_net_pton(af, src, dst)
 *      convert from presentation format (which usually means ASCII printable)
 *      to network format (which is usually some kind of binary format).
 * return:
 *      1 if the address was valid for the specified address family
 *      0 if the address wasn't valid (`dst' is untouched in this case)
 *      -1 if some other error occurred (`dst' is untouched in this case, too)
 * author:
 *      Paul Vixie, 1996.
 */
int inet_pton(int af, const char *src, void *dst)
{
       switch (af) {
       case AF_INET:
               return (inet_pton4(src, dst));
#ifdef AF_INET6
       case AF_INET6:
               return (inet_pton6(src, dst));
#endif
       default:
               errno = EAFNOSUPPORT;
               return (-1);
       }
       /* NOTREACHED */
}

