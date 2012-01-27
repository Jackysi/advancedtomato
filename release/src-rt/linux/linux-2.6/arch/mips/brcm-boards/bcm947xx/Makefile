#
# Makefile for Broadcom BCM947XX boards
#
# Copyright (C) 2009, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.7 2009/11/06 03:08:27 Exp $
#

SHARED := ../../../../../../shared

BCM947XX_OBJS = $(addprefix $(SHARED)/, hndmips.o hndchipc.o linux_gpio.o sflash.o)
BCM947XX_OBJS += $(addprefix $(SHARED)/nvram/, nvram.o)

ifeq ($(CONFIG_MTD_NFLASH),y)
BCM947XX_OBJS += $(addprefix $(SHARED)/, nflash.o)
endif

EXTRA_CFLAGS += -DLINUX_HYBRID_POSTPROCESSING_REMOVAL

ifeq ($(CONFIG_PCI),y)
BCM947XX_OBJS += $(addprefix $(SHARED)/, hndpci.o) pcibios.o
endif

ifeq ($(CONFIG_WAPI),y)
EXTRA_CFLAGS += -DBCMWAPI_WAI
endif

ifeq ($(CONFIG_GENERIC_GPIO),y)
BCM947XX_OBJS += gen_gpio.o
endif


BCM947XX_OBJS += setup.o prom.o time.o irq.o perfcntr.o

bcm947xx-objs += $(BCM947XX_OBJS)
obj-y += bcm947xx.o
