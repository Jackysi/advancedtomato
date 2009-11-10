/*
 * Copyright (C) 2008, Broadcom Corporation
 * All Rights Reserved.
 * 
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior
 * written permission of Broadcom Corporation.
 *
 * $Id: emfu_linux.h,v 1.1 2007/03/17 03:14:23 Exp $
 */

#ifndef _EMFU_LINUX_H_
#define _EMFU_LINUX_H_

#define NETLINK_EMFC   17

struct emf_cfg_request;
extern int emf_cfg_request_send(struct emf_cfg_request *buffer, int length);

#endif /* _EMFU_LINUX_H_ */
