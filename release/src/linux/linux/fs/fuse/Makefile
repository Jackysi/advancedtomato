#
# Makefile for the FUSE FS
#

VERSION = 2.5.3
EXTRA_CFLAGS += -I. -D_LOOSE_KERNEL_NAMES -DFUSE_VERSION=\"$(VERSION)\" 

O_TARGET := fuse.o

obj-y    := dev.o dir.o file.o inode.o compat/parser.o
obj-m    := $(O_TARGET)

include $(TOPDIR)/Rules.make
