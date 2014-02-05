# Rules for making the NTFS driver.

O_TARGET := ntfs.o

obj-y   := aops.o attrib.o compress.o debug.o dir.o file.o inode.o mft.o \
		mst.o namei.o super.o sysctl.o unistr.o upcase.o

obj-m   := $(O_TARGET)

EXTRA_CFLAGS = -DNTFS_VERSION=\"2.1.6b\"

ifeq ($(CONFIG_NTFS_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

ifeq ($(CONFIG_NTFS_RW),y)
EXTRA_CFLAGS += -DNTFS_RW
endif

include $(TOPDIR)/Rules.make

