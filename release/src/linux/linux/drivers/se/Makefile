#
# Makefile for the Broadcom streaming encryption device driver
#
# Copyright 2001-2004, Broadcom Corporation
# All Rights Reserved.
#
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#

O_TARGET	:= bcm_se.o

SE_OBJS		:= se_thread.o se_change.o se.o

export-objs	:=
obj-y		:= $(SE_OBJS)
obj-m		:= $(O_TARGET)

SRCBASE		:= $(TOPDIR)/../..
EXTRA_CFLAGS	+= -I$(SRCBASE)/linux/linux/include/linux -I$(SRCBASE)/linux/linux/include/asm
EXTRA_CFLAGS	+= -DLINUX=1 -DMODULE -D__KERNEL__

ifneq ($(wildcard $(SRCBASE)/bcmdrivers/se/*.c),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/bcmdrivers/se \
		   -I$(SRCBASE)/include -I$(SRCBASE)/include/security \
		   -I$(SRCBASE)/include/security/kern 
	
vpath %.c $(SRCBASE)/bcmdrivers/se $(SRCBASE)/shared
else
obj-y		:= $(foreach obj,$(SE_OBJS),$(SRCBASE)/bcmdrivers/se/linux/$(obj))
endif

include $(TOPDIR)/Rules.make
