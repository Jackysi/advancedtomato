topdir:=../../..
SUBDIRS = $(USBTYPE)
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: $(OBJS)

# Include dependencies
-include $(DEPS)
