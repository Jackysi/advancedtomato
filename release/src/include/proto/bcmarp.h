/*
 * Copyright 2006, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * Fundamental constants relating to ARP Protocol
 *
 * $Id$
 */

#ifndef _bcmarp_h_
#define _bcmarp_h_

#define ARP_OPC_OFFSET	6	/* option code offset */
#define ARP_SRC_ETH_OFFSET	8	/* src h/w address offset */
#define ARP_SRC_IP_OFFSET	14	/* src IP address offset */
#define ARP_TGT_ETH_OFFSET	18	/* target h/w address offset */
#define ARP_TGT_IP_OFFSET	24	/* target IP address offset */

#define ARP_OPC_REQUEST	1	/* ARP request */
#define ARP_OPC_REPLY	2	/* ARP reply */

#endif	/* #ifndef _bcmarp_h_ */
