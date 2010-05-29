#
# Makefile for the Broadcom IGS Layer
#
# Copyright (C) 2009, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.3 2008/08/13 20:20:16 Exp $
#

IGSSRCDIR := ../../../../../emf/igs
EMFDIR := ../../../../../emf

IGS_OBJS := $(IGSSRCDIR)/igsc.o $(IGSSRCDIR)/igsc_sdb.o $(IGSSRCDIR)/igs_linux.o \
	    $(IGSSRCDIR)/osl_linux.o

# Search for sources under src/emf/igs
ifneq ($(wildcard $(src)/$(IGSSRCDIR)),)
	EXTRA_CFLAGS += -I$(IGSSRCDIR) -I$(EMFDIR)/emf
	EXTRA_CFLAGS += -I../../include/emf/igs -I../../include/emf/emf
	igs-objs := $(IGS_OBJS)
else
	igs-objs := $(EMFDIR)/igs.o
endif

obj-$(CONFIG_EMF) := igs.o

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
else
# Normal makefile
KERNELDIR := ../../../../../linux/linux-2.6
all:
	$(MAKE) -C $(KERNELDIR) M=`pwd`
clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean
endif

clean-files += $(IGSSRCDIR)/*.o $(IGSSRCDIR)/.*.o.cmd
