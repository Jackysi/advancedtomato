#
# Generic portion of the Broadcom wl driver makefile
#
# input: O_TARGET, CONFIG_WL_CONF and WL_PREFIX 
# output: obj-m, obj-y
#
# $Id: wl_generic.mk,v 1.1.1.1 2004/08/26 06:56:55 honor Exp $
#

# get the source file list from config file
ifeq ($(CONFIG_WL_CONF),)
$(error CONFIG_WL_CONF is undefined)
endif
WLCONFFILE :=$(strip $(subst ",,$(CONFIG_WL_CONF))) # remove quote ", added by parser

WLCFGDIR   := $(SRCBASE)/wl/config

# define OS flag to pick up wl osl file from wl.mk
WLLX=1
include $(WLCFGDIR)/$(WLCONFFILE)
include $(WLCFGDIR)/wl.mk

ifneq ($(WLFILES),)
WL_SOURCE	:= $(WLFILES)
else
$(error WLFILES is undefined in $(WLCFGDIR)/$(WLCONFFILE))
endif
WL_DFLAGS       := $(WLFLAGS)
WL_OBJS         := $(patsubst %.c,%.o,$(WL_SOURCE))

VARIANT_WL_OBJS  := $(foreach obj,$(WL_OBJS),$(WL_PREFIX)_$(obj))

# need -I. to pick up wlconf.h
EXTRA_CFLAGS	+= -DDMA -I. $(WL_DFLAGS)

# obj-y is for linking to wl.o
export-objs	:=
obj-y		:= $(VARIANT_WL_OBJS)
obj-m		:= $(O_TARGET)

# Search for sources under src/wl/sys or objects under src/wl/linux
ifneq ($(wildcard $(SRCBASE)/wl/sys/wlc.h),)
EXTRA_CFLAGS	+= -I$(SRCBASE)/wl/sys
vpath %.c $(SRCBASE)/wl/sys $(SRCBASE)/shared $(SRCBASE)/bcmcrypto
else
#obj-y		:= $(SRCBASE)/wl/linux/wl.o
obj-y		:= $(foreach objy,$(obj-y),$(SRCBASE)/wl/linux/$(objy))
endif



include $(TOPDIR)/Rules.make




