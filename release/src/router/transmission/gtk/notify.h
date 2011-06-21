/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * This file is licensed by the GPL version 2. Works owned by the
 * Transmission project are granted a special exemption to clause 2(b)
 * so that the bulk of its code can remain under the MIT license.
 * This exemption does not extend to derived works not owned by
 * the Transmission project.
 *
 * $Id: notify.h 12068 2011-03-03 01:59:25Z jordan $
 */

#ifndef GTR_NOTIFY_H
#define GTR_NOTIFY_H

#include "tr-core.h"

void gtr_notify_init( void );

void gtr_notify_torrent_added     ( const char * name );

void gtr_notify_torrent_completed ( TrCore * core, int torrent_id );

#endif
