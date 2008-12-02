#
# Makefile for common code of the NEC VR4100 series.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#

USE_STANDARD_AS_RULE := true

all: vr41xx.o

O_TARGET := vr41xx.o

obj-y := bcu.o cmu.o giu.o icu.o int-handler.o reset.o

export-objs := vrc4173.o

obj-$(CONFIG_PCI)		+= pciu.o
obj-$(CONFIG_SERIAL)		+= serial.o
obj-$(CONFIG_VR41XX_TIME_C)	+= time.o
obj-$(CONFIG_VRC4173)		+= vrc4173.o

include $(TOPDIR)/Rules.make
