#
# Broadcom BCM57XX Gigabit Ethernet driver make file.
#
# $Id: Makefile,v 1.1.1.1 2008/10/15 03:26:42 james26_jang Exp $
#

O_TARGET = bcm57xx.o

# If source directory src/bcm57xx exists, build objects from there
ifneq ($(wildcard $(SRCBASE)/bcm57xx/sys),)

vpath %.c $(SRCBASE)/bcm57xx/linux $(SRCBASE)/bcm57xx/sys $(SRCBASE)/shared

EXTRA_CFLAGS += -DDBG=0 -DT3_JUMBO_RCV_RCB_ENTRY_COUNT=256 -DNICE_SUPPORT -DBCM_SMALL_DRV
EXTRA_CFLAGS += -DPCIX_TARGET_WORKAROUND=1 -DINCLUDE_TBI_SUPPORT -DINCLUDE_5701_AX_FIX=1
EXTRA_CFLAGS += -I$(SRCBASE)/bcm57xx/linux -I$(SRCBASE)/bcm57xx/sys -I$(SRCBASE)/include


export-objs :=

obj-y := b57um.o b57proc.o b57diag.o tigon3.o autoneg.o 5701rls.o tcp_seg.o hndgige.o
obj-m := $(O_TARGET)

# Otherwise, use object module(s) in this directory
else

obj-y := $(SRCBASE)/bcm57xx/linux/bcm57xx.o
obj-m := $(O_TARGET)

endif

include $(TOPDIR)/Rules.make
