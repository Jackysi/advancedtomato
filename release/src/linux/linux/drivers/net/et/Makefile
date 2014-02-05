#
# Makefile for the Broadcom et driver
#
# Copyright 2006, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id$
#

O_TARGET	:= et.o

ET_OBJS		:= et_linux.o etc.o

ifeq ($(CONFIG_ET_47XX),y)
ET_OBJS		+= etc47xx.o etc_adm.o
EXTRA_CFLAGS	+= -DBCM47XX_CHOPS -DETROBO -DETADM
endif

export-objs	:=
obj-y		:= $(ET_OBJS)
obj-m		:= $(O_TARGET)

EXTRA_CFLAGS	+= -DDMA -Werror

# Search for sources under src/et/sys or objects under src/et/linux
ifneq ($(wildcard $(SRCBASE)/et/sys/*.c),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/et/sys
vpath %.c $(SRCBASE)/et/sys $(SRCBASE)/shared
else
#obj-y		:= $(foreach obj,$(ET_OBJS),$(SRCBASE)/et/linux/$(obj))
obj-y		:= $(SRCBASE)/et/linux/et.o
endif

include $(TOPDIR)/Rules.make
