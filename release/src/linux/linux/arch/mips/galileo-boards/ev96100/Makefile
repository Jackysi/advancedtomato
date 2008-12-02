#
#  Copyright 2000 MontaVista Software Inc.
#  Author: MontaVista Software, Inc.
#     	ppopov@mvista.com or source@mvista.com
#
# Makefile for the Galileo EV96100 board.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

USE_STANDARD_AS_RULE := true

O_TARGET:= ev96100.o

obj-y	:= init.o pci_ops.o pci_fixups.o time.o irq.o int-handler.o setup.o \
	    puts.o

include $(TOPDIR)/Rules.make
