topdir:=../..

SUBDIRS = $(SMARTDRV) $(DUMBDRV) $(NETDRV) $(PCNETDRV) $(USBDRV) \
          $(SNMPDRV) $(SNMPLTDRV) $(TESTDRV)

include $(topdir)/autoconf/targets.mak

# Append to OBJS the path to each driver object
OBJS += $(foreach subdir,$(SUBDIRS),$(call SRC2OBJ,$(wildcard $(subdir)/*.c)))
OBJS += $(foreach subdir,$(SUBDIRS),$(call SRC2OBJ,$(wildcard $(subdir)/*.cpp)))
ifneq ($(strip $(USBDRV)),)
  OBJS += $(call SRC2OBJ,$(wildcard $(USBDRV)/$(USBTYPE)/*.c))
endif

SRCS = drivers.c

all-targets: libdrivers.a

libdrivers.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
