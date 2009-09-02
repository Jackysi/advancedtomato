#
# Makefile for the linux squashfs routines.
#

O_TARGET := squashfs.o

obj-y  := inode.o 

ifdef CONFIG_LZMA_FS_INFLATE
obj-y += LzmaDecode.o
endif

obj-m := $(O_TARGET)

include $(TOPDIR)/Rules.make
