#
# Makefile for the linux Journalling Flash FileSystem (JFFS) routines.
#
# $Id: Makefile,v 1.1.1.4 2003/10/14 08:09:00 sparq Exp $
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definitions are now in the main makefile...

list-multi := jffs.o

jffs-objs := jffs_fm.o intrep.o jffs_gc.o

ifeq ($(PATCHLEVEL),2)
	jffs-objs += inode-v22.o
else
	jffs-objs += inode-v23.o
endif

ifeq ($(CONFIG_JFFS_PROC_FS),y)
	jffs-objs += jffs_proc.o
endif

O_TARGET := jffs.o

obj-y := $(jffs-objs)
obj-m := $(O_TARGET)

include $(TOPDIR)/Rules.make

