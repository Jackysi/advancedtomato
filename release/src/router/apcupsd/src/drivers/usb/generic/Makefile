topdir:=../../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)
EXTRAINCS += -I$(topdir)/src/libusbhid

all-targets: $(OBJS)

# Include dependencies
-include $(DEPS)
