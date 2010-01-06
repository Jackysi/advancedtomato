/*
 * Copyright (C) 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: igsu_linux.h,v 1.1 2007/03/17 03:15:10 Exp $
 */

#ifndef _IGSU_LINUX_H_
#define _IGSU_LINUX_H_

#define NETLINK_IGSC 18

struct igs_cfg_request;
extern int igs_cfg_request_send(struct igs_cfg_request *buffer, int length);

#endif /* _IGSU_LINUX_H_ */
