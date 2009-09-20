#
# Makefile for the BCM947xx specific kernel interface routines
# under Linux.
#
EXTRA_CFLAGS += -fno-delayed-branch
USE_STANDARD_AS_RULE := true

O_TARGET        := brcm.o

obj-y           := int-handler.o
ifneq ($(strip $(CONFIG_BCM4704)),y)
#obj-y           += irq.o
endif

obj-$(CONFIG_REMOTE_DEBUG)      += gdb_hook.o

include $(TOPDIR)/Rules.make
