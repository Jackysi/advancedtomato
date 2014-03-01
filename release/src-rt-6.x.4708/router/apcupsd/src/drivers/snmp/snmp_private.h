/*
 * snmp_private.h
 *
 * Private header for the SNMP UPS driver
 */

/*
 * Copyright (C) 2000-2004 Kern Sibbald
 * Copyright (C) 1999-2001 Riccardo Facchetti <riccardo@apcupsd.org>
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

#ifndef _SNMP_PRIVATE_H
#define _SNMP_PRIVATE_H

/* APC */
extern int powernet_snmp_ups_get_capabilities(UPSINFO *ups);
extern int powernet_snmp_ups_read_static_data(UPSINFO *ups);
extern int powernet_snmp_ups_read_volatile_data(UPSINFO *ups);
extern int powernet_snmp_ups_check_state(UPSINFO *ups);
extern int powernet_snmp_kill_ups_power(UPSINFO *ups);
extern int powernet_snmp_ups_open(UPSINFO *ups);

/* IETF */
extern int rfc1628_snmp_ups_get_capabilities(UPSINFO *ups);
extern int rfc1628_snmp_ups_read_static_data(UPSINFO *ups);
extern int rfc1628_snmp_ups_read_volatile_data(UPSINFO *ups);
extern int rfc1628_snmp_ups_check_state(UPSINFO *ups);
extern int rfc1628_snmp_kill_ups_power(UPSINFO *ups);

#endif   /* _SNMP_PRIVATE_H */
