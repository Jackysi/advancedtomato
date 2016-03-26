/*
 * This file Copyright (C) 2007-2014 Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 * $Id: natpmp_local.h 14369 2014-12-10 18:58:12Z mikedld $
 */

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#ifndef TR_NATPMP_H
#define TR_NATPMP_H 1

/**
 * @addtogroup port_forwarding Port Forwarding
 * @{
 */

typedef struct tr_natpmp tr_natpmp;

tr_natpmp * tr_natpmpInit (void);

void tr_natpmpClose (tr_natpmp *);

int tr_natpmpPulse (tr_natpmp *, tr_port port, bool isEnabled, tr_port * public_port);

/* @} */
#endif
