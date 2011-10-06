#
# Makefile for the Broadcom wl driver
#
# Copyright (C) 2009, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.11 2010/02/23 07:32:54 Exp $
#

SRCBASE := ../../../../..

TARGET := wl
WLCONF_H := wlconf.h
WLCONF_O := wlconf.o

WL_PARTIAL_REBUILD := 0

CONFIG_WL_AP := "wlconfig_lx_router_ap"
CONFIG_WL_APSTA := "wlconfig_lx_router_apsta"
CONFIG_WL_STA := "wlconfig_lx_router_sta"
CONFIG_WL_AP_SDSTD := "wlconfig_lx_router_ap_sdstd"
CONFIG_WL_AP_ONCHIP_G := "wlconfig_lx_router_ap_1chipG"
CONFIG_WL_STA_ONCHIP_G := "wlconfig_lx_router_sta_1chipG"
CONFIG_WL_HIGH := "wlconfig_lx_router_high"

# Generate list of supported variants
VLIST_BASE := AP STA APSTA
ifeq ($(CONFIG_WL_USE_AP_SDSTD),y)
  VLIST_BASE   += AP_SDSTD
endif

VLIST          += $(VLIST_BASE)

ifeq ($(CONFIG_WL_USBAP),y)
# Do variants because we need two drivers for dual band operation
WL_VARIANTS	:= 1
VLIST		+= HIGH
# Note that we don't use CONFIG_WL_USE_HIGH because we need to leave WL_SUFFIX for
# the other driver. HIGH driver is always called wl_high and loaded upon hotplut
# event.
endif

ifeq ($(BUILD_WL_ONCHIP_G),true)
  VLIST        += $(foreach base,$(VLIST_BASE),$(base)_ONCHIP_G)
endif # BUILD_WL_ONCHIP_G

# Set suffix and config based on the selected variant
WL_SUFFIX := $(strip $(foreach v,$(VLIST),$(if $(CONFIG_WL_USE_$(v)),$(v))))
CONFIG_WL_CONF := $(CONFIG_WL_$(WL_SUFFIX))

# Module file names use lowercase of the suffix
wl_suffix := $(shell echo "$(WL_SUFFIX)" | tr A-Z a-z)


# For PREBUILD and postbuild w/WLSRC, compile variants in subdirs
# Subdir names use lowercase versions of the variant suffixes
wlsubdirs := $(addprefix wl_,$(foreach v,$(VLIST),$(shell echo "$(v)/" | tr A-Z a-z)))


#subdir-y += $(wlsubdirs)
#subdir-m += $(wlsubdirs)


include $(src)/wl_generic.mk
