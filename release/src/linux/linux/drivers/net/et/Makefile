#
# Makefile for the Broadcom et driver
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.9 2005/03/07 07:30:48 kanki Exp $
#

O_TARGET	:= et.o

ET_OBJS		:= et_linux.o etc.o

ifeq ($(CONFIG_ET_47XX),y)
ET_OBJS		+= etc47xx.o etc_robo.o etc_adm.o
EXTRA_CFLAGS	+= -DBCM47XX_CHOPS -DETROBO -DETADM
endif
ifeq ($(CONFIG_ET_4413),y)
ET_OBJS		+= etc4413.o
EXTRA_CFLAGS	+= -DBCM4413_CHOPS
endif

export-objs	:=
obj-y		:= $(ET_OBJS)
obj-m		:= $(O_TARGET)

EXTRA_CFLAGS	+= -DDMA

# Search for sources under src/et/sys or objects under src/et/linux
ifneq ($(wildcard $(SRCBASE)/et/sys),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/et/sys
vpath %.c $(SRCBASE)/et/sys $(SRCBASE)/shared
else
#obj-y		:= $(foreach obj,$(ET_OBJS),$(SRCBASE)/et/linux/$(obj))
obj-y		:= $(SRCBASE)/et/linux/et.o
endif

include $(TOPDIR)/Rules.make
