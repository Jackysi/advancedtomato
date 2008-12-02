# $Id: Makefile,v 1.1.1.4 2003/10/14 08:07:29 sparq Exp $
# Makefile for the Sun Boot PROM interface library under
# Linux.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

OBJS  = init.o console.o printf.o  misc.o
#bootstr.o init.o misc.o segment.o console.o printf.o
all: promlib.a

promlib.a: $(OBJS)
	$(AR) rcs promlib.a $(OBJS)
	sync

dep:
	$(CPP) $(CPPFLAGS) -M *.c > .depend

include $(TOPDIR)/Rules.make
