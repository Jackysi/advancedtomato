#
# Makefile for Broadcom Home Networking Division (HND) shared driver code
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

O_TARGET	:= hnd.o

WLSHARED        := wlconfig_lx_shared
WLCFGDIR        := $(SRCBASE)/wl/config

ifneq ($(wildcard $(WLCFGDIR)),)
include         $(WLCFGDIR)/$(WLSHARED)
include         $(WLCFGDIR)/wl.mk
endif

HND_OBJS	:= $(WLFILES:.c=.o)

EXTRA_CFLAGS += $(WLFLAGS)

ifneq ($(CONFIG_BCM947XX),y)
HND_OBJS	+= nvramstubs.o
endif

ifeq ($(CONFIG_ET_47XX),y)
HND_OBJS	+= bcmrobo.o
else
ifeq ($(CONFIG_BCM57XX),y)
HND_OBJS	+= bcmrobo.o
endif
endif

ifdef CONFIG_USB_RNDIS
EXTRA_CFLAGS += -DCONFIG_USBRNDIS_RETAIL
endif                                  

ifeq ($(BCMDBG_MEM),1)
EXTRA_CFLAGS += -DBCMDBG_MEM
endif

export-objs	:= shared_ksyms.o
obj-y		:= shared_ksyms.o $(HND_OBJS)
obj-m		:= $(O_TARGET)

vpath %.c $(SRCBASE)/shared $(SRCBASE)/shared/nvram

include $(TOPDIR)/Rules.make

ifeq ($(wildcard $(SRCBASE)/shared/bcmutils.c),)
bcmutils.o: $(SRCBASE)/shared/linux/bcmutils.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/bcmrobo.c),)
bcmutils.o: $(SRCBASE)/shared/linux/bcmrobo.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/hnddma.c),)
hnddma.o: $(SRCBASE)/shared/linux/hnddma.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/linux_osl.c),)
linux_osl.o: $(SRCBASE)/shared/linux/linux_osl.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/sbutils.c),)
sbutils.o: $(SRCBASE)/shared/linux/sbutils.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/bcmsrom.c),)
bcmsrom.o: $(SRCBASE)/shared/linux/bcmsrom.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/nvramstubs.c),)
nvramstubs.o: $(SRCBASE)/shared/linux/nvramstubs.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/bcmwpa.c),)
bcmwpa.o: $(SRCBASE)/shared/linux/bcmwpa.o
	cp $< $@
endif

ifeq ($(wildcard $(SRCBASE)/shared/bcmotp.c),)
bcmotp.o: $(SRCBASE)/shared/linux/bcmotp.o
	cp $< $@
endif

shared_ksyms.c: shared_ksyms.sh $(HND_OBJS)
	sh -e $< $(HND_OBJS) > $@
