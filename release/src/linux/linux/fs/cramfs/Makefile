#
# Makefile for the linux cramfs routines.
#

O_TARGET := cramfs.o

obj-y  := inode.o

ifdef CONFIG_ZLIB_FS_INFLATE
obj-y += uncompress.o
CFLAGS_uncompress.o := -I $(TOPDIR)/fs/inflate_fs
else
ifdef CONFIG_LZMA_FS_INFLATE
obj-y += $(TOPDIR)/fs/inflate_lzma/7zlzma.o \
         $(TOPDIR)/fs/inflate_lzma/LZMADecoder.o \
         $(TOPDIR)/fs/inflate_lzma/IInOutStreams.o
endif
endif

obj-m := $(O_TARGET)

include $(TOPDIR)/Rules.make
