topdir:=../../../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

all-targets: $(OBJS)
EXTRAINCS += -I$(topdir)/src/libusbhid

# Include dependencies
-include $(DEPS)
