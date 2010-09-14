#
# Makefile for the linux squashfs routines.
#

LZMADIR = ../../../../lzma/C/Compress/Lzma
EXTRA_CFLAGS += -I$(src)/$(LZMADIR)
vpath %.c $(LZMADIR)

obj-$(CONFIG_SQUASHFS) += squashfs.o
squashfs-objs := inode.o
squashfs-objs += $(addprefix $(LZMADIR)/, LzmaDecode.o)
