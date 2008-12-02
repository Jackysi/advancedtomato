#  Copyright 2001, Broadcom Corporation
#  All Rights Reserved.
#  
#  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
#  the contents of this file may not be disclosed to third parties, copied or
#  duplicated in any form, in whole or in part, without the prior written
#  permission of Broadcom Corporation.
#
# Makefile for the Broadcom et driver
#
# $Id: Makefile,v 1.3 2004/11/17 07:50:40 honor Exp $
#

O_TARGET	:= 4702et.o

ET_OBJS		:= et_linux.o etc.o

ifeq ($(CONFIG_ET_47XX),y)
ET_OBJS		+= etc47xx.o etc_robo.o
EXTRA_CFLAGS	+= -DBCM47XX_CHOPS
endif
ifeq ($(CONFIG_ET_4413),y)
ET_OBJS		+= etc4413.o
EXTRA_CFLAGS	+= -DBCM4413_CHOPS
endif

export-objs	:=
obj-y		:= $(ET_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/include
EXTRA_CFLAGS	+= -DDMA

# Search for sources under src/et/sys or objects under src/et/linux
#ifneq ($(wildcard $(SRCBASE)/et.4702/sys),)
#EXTRA_CFLAGS	+= -I$(SRCBASE)/et.4702/sys
#vpath %.c $(SRCBASE)/et.4702/sys $(SRCBASE)/shared
#else
obj-y		:= $(foreach obj,$(ET_OBJS),$(SRCBASE)/et.4702/linux/$(obj))
#endif

include $(TOPDIR)/Rules.make
