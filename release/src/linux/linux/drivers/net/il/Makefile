#
# Makefile for the Broadcom il driver
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.8 2005/03/07 07:30:48 kanki Exp $
#

O_TARGET	:= il.o

IL_OBJS		:= il_linux.o ilc.o cert.o plarq.o pe_select_maxse.o

ifeq ($(CONFIG_IL_47XX),y)
IL_OBJS		+= ilc47xx.o
EXTRA_CFLAGS	+= -DBCM47XX_CHOPS
endif
ifeq ($(CONFIG_IL_42XX),y)
IL_OBJS		+= ilc42xx.o
EXTRA_CFLAGS	+= -DBCM42XX_CHOPS
endif

export-objs	:=
obj-y		:= $(IL_OBJS)
obj-m		:= $(O_TARGET)

EXTRA_CFLAGS	+= -DDMA -DIL_CERT -DIL_PROTOS

ifneq ($(CONFIG_BRIDGE),n)
EXTRA_CFLAGS	+= -DIL_BRIDGE
endif

ifeq ($(CONFIG_LARQ_BUF),0)
EXTRA_CFLAGS	+= -DLARQ_TINY
endif

# Search for sources under src/il/sys or objects under src/il/linux
ifneq ($(wildcard $(SRCBASE)/il/sys),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/il/sys
vpath %.c $(SRCBASE)/il/sys $(SRCBASE)/shared
else
#obj-y		:= $(foreach obj,$(IL_OBJS),$(SRCBASE)/il/linux/$(obj))
obj-y		:= $(SRCBASE)/il/linux/il.o
endif

include $(TOPDIR)/Rules.make
