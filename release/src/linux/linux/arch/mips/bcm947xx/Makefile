#
# Makefile for the BCM947xx specific kernel interface routines
# under Linux.
#

vpath %.c $(SRCBASE)/shared $(SRCBASE)/shared/nvram

O_TARGET        := bcm947xx.o

export-objs     := nvram_linux.o setup.o

obj-y		:= prom.o setup.o time.o irq.o
obj-y           += hndmips.o hndchipc.o perfcntr.o gpio.o
obj-y		+= sflash.o nvram.o nvram_linux.o
obj-$(CONFIG_PCI) += hndpci.o pcibios.o

include $(TOPDIR)/Rules.make

