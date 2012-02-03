/*
 * snmplite-common.h
 *
 * Public header for the SNMP Lite UPS driver
 */

/*
 * Copyright (C) 2009 Adam Kropelin
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifndef _SNMPLITE_COMMON_H
#define _SNMPLITE_COMMON_H

#include "apc.h"
#include "snmp.h"

// Forward declaration
struct MibStrategy;

// For use by MIB strategies
void snmplite_trap_wait(UPSINFO *ups);

// Internal driver-specific structure
struct snmplite_ups_internal_data
{
   char device[MAXSTRING];       /* Copy of ups->device */
   const char *host;             /* hostname|IP of peer */
   unsigned short port;          /* Remote port, usually 161 */
   const char *vendor;           /* SNMP vendor: APC or APC_NOTRAP */
   const char *community;        /* Community name */
   Snmp::SnmpEngine *snmp;       /* SNMP engine instance */
   int error_count;              /* Number of consecutive SNMP network errors */
   time_t commlost_time;         /* Time at which we declared COMMLOST */
   const MibStrategy *strategy;  /* MIB strategy to use */
   bool traps;                   /* true if catching SNMP traps */
};

#endif   /* _SNMPLITE_COMMON_H */
