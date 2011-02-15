#
# Makefile for the linux squashfs routines.
#

O_TARGET := squashfs.o

obj-y  := inode.o LzmaDecode.o

obj-m := $(O_TARGET)

LZMADIR = $(SRCBASE)/lzma/C/Compress/Lzma
EXTRA_CFLAGS += -I$(LZMADIR)
vpath %.c $(LZMADIR)

include $(TOPDIR)/Rules.make
