#
# Makefile for the Broadcom wl driver
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1.1.9 2005/03/07 07:30:49 kanki Exp $
#

O_TARGET	:= wl.o

# get the variant for linking
ifeq ($(CONFIG_WL_LINK),"AP")
  WL_PREFIX := ap
  CONFIG_WL_CONF = $(CONFIG_WL_AP)
else
ifeq ($(CONFIG_WL_LINK),"STA")
    WL_PREFIX := sta
    CONFIG_WL_CONF = $(CONFIG_WL_STA)
else
ifeq ($(CONFIG_WL_LINK),"APSTA")
      WL_PREFIX := apsta
      CONFIG_WL_CONF = $(CONFIG_WL_APSTA)
else
ifeq ($(CONFIG_WL_LINK),"DNGL")
      WL_PREFIX := dngl
      CONFIG_WL_CONF = $(CONFIG_WL_DNGL)
else                                   
$(error CONFIG_WL_LINK=$(CONFIG_WL_LINK) is wrong, must be AP/STA/APSTA/DNGL)
endif
endif
endif
endif

# compile variants in sub directories if necessary


subdir-y += $(wlsubdirs)
subdir-m += $(wlsubdirs)


include ./wl_generic.mk
