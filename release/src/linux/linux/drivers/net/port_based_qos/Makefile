#  Copyright 2001, Cybertan Corporation
#  All Rights Reserved.
#  
#  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Cybertan Corporation;
#  the contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of Cybertan Corporation.
#
#
# $Id: Makefile,v 1.2 2004/03/02 13:23:33 cnhsieh Exp $
#

O_TARGET	:= port_based_qos_mod.o 

PORT_BASED_QOS_MOD_OBJS	:= port_based_qos.o Atan.o c47xx.o eeprom.o 

export-objs	:= 
obj-y		:= $(PORT_BASED_QOS_MOD_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include -Wall -I$(SRCBASE)/

vpath %.c $(SRCBASE)/shared

include $(TOPDIR)/Rules.make

