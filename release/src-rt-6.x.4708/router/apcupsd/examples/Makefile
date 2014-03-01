topdir:=..

SUBDIRS = 
include $(topdir)/autoconf/targets.mak

TARGETS = hid-ups hid-set client megaclient newslave upsapm \
          smartsim snoopdecode

SRCS = $(foreach target,$(TARGETS),$(target).c)

all-targets: client megaclient newslave upsapm smartsim snoopdecode

$(TARGETS): %: $(call SRC2OBJ,%.c) $(APCLIBS)
	$(LINK)

# Include dependencies
-include $(DEPS)

