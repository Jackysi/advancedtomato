# $Id: Makefile,v 1.1.1.4 2003/10/14 08:07:51 sparq Exp $
# Makefile for the Sun Boot PROM interface library under
# Linux.
#

USE_STANDARD_AS_RULE := true
EXTRA_AFLAGS := -ansi

L_TARGET = promlib.a
obj-y   := bootstr.o devops.o init.o memory.o misc.o \
	   tree.o console.o printf.o p1275.o map.o

include $(TOPDIR)/Rules.make

%.o: %.c
	$(CC) $(subst -pg,,$(CFLAGS)) -c $<
