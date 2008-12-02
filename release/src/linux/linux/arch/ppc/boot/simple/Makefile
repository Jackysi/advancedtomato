# This is far from simple, but I couldn't think of a good name.  This is
# for making the 'zImage' or 'zImage.initrd' on a number of targets.
#
# Author: Tom Rini <trini@mvista.com>
#
# Copyright 2001-2002 MontaVista Software Inc.
#
# Notes:
# (1) For machine targets which produce more than one image, define
# ZNETBOOT and ZNETBOOTRD to the image which should be available for
# 'znetboot' and 'znetboot.initrd`
# (2) For machine targets which use the mktree program, define END to be
# the machine name you want in the image, and you can optionally set
# ENTRYPOINT which the image should be loaded at.  The optimal setting
# for ENTRYPOINT is the link address.
#
# This program is free software; you can redistribute  it and/or modify it
# under  the terms of  the GNU General  Public License as published by the
# Free Software Foundation;  either version 2 of the  License, or (at your
# option) any later version.

USE_STANDARD_AS_RULE		:= true

# Normally, we use the 'misc-simple.c' file for decompress_kernel and
# whatnot.  Sometimes we need to override this however.
MISC				:= ../common/misc-simple.o
ifeq ($(CONFIG_EMBEDDEDBOOT),y)
ZIMAGE				:= zImage-EMBEDDED
ZIMAGEINITRD			:= zImage.initrd-EMBEDDED
TFTPIMAGE			:= /tftpboot/zImage.embedded
MISC				:= misc-embedded.o
endif
ifeq ($(CONFIG_GEMINI),y)
ZIMAGE				:= zImage-SMON
ZIMAGEINITRD			:= zImage.initrd-SMON
EXTRA				:= direct.o
TFTPIMAGE			:= /tftpboot/zImage.gemini
endif
ifeq ($(CONFIG_PAL4),y)
ZIMAGE				:= zImage-PAL4
ZIMAGEINITRD			:= zImage.initrd-PAL4
EXTRA				:= direct.o
TFTPIMAGE			:= /tftpboot/zImage.pal4
endif
ifeq ($(CONFIG_SPRUCE),y)
ZIMAGE				:= zImage-TREE
ZIMAGEINITRD			:= zImage.initrd-TREE
EXTRA				:= direct.o
ENTRYPOINT			:= 0x00800000
MISC				:= misc-spruce.o
END				:= spruce
TFTPIMAGE			:= /tftpboot/zImage.$(END)
endif
ifeq ($(CONFIG_SMP),y)
TFTPIMAGE			+= .smp
endif

# Setup a default address to put ourselves, change it as needed.
LD_ARGS				= -T ../ld.script -Ttext 0x00800000 -Bstatic
ifdef CONFIG_8xx
LD_ARGS				:= -T ../ld.script -Ttext 0x00180000 -Bstatic
endif
ifeq ($(CONFIG_8260)$(CONFIG_4xx),y)
LD_ARGS				:= -T ../ld.script -Ttext 0x00400000 -Bstatic
endif
OBJCOPY_ARGS			:= -O elf32-powerpc

# head.o and ../common/relocate.o must be at the start.
obj-y				:= head.o ../common/relocate.o $(EXTRA) \
				$(MISC) ../common/misc-common.o \
				../common/string.o ../common/util.o
obj-$(CONFIG_4xx)		+= embed_config.o
obj-$(CONFIG_8xx)		+= embed_config.o
obj-$(CONFIG_8260)		+= embed_config.o
obj-$(CONFIG_BSEIP)		+= iic.o
obj-$(CONFIG_MBX)		+= iic.o
obj-$(CONFIG_RPXCLASSIC)	+= iic.o
obj-$(CONFIG_RPXLITE)		+= iic.o
# Different boards need different serial implementations.
ifeq ($(CONFIG_SERIAL_CONSOLE),y)
obj-$(CONFIG_8xx)		+= m8xx_tty.o
obj-$(CONFIG_8260)		+= m8260_tty.o
obj-$(CONFIG_SERIAL)		+= ../common/ns16550.o
endif

LIBS				:= ../lib/zlib.a

# Tools
MKPREP				:= ../utils/mkprep
MKTREE				:= ../utils/mktree

zvmlinux: $(obj-y) $(LIBS) ../ld.script ../images/vmlinux.gz ../common/dummy.o
	$(OBJCOPY) $(OBJCOPY_ARGS) \
		--add-section=.image=../images/vmlinux.gz \
		--set-section-flags=.image=contents,alloc,load,readonly,data \
		../common/dummy.o image.o
	$(LD) $(LD_ARGS) -o $@ $(obj-y) image.o $(LIBS)
	$(OBJCOPY) $(OBJCOPY_ARGS) $@ $@ -R .comment -R .stab -R .stabstr \
		-R .ramdisk -R .sysmap

zvmlinux.initrd: $(obj-y) $(LIBS) ../ld.script ../images/vmlinux.gz \
		../common/dummy.o
	$(OBJCOPY) $(OBJCOPY_ARGS) \
		--add-section=.ramdisk=../images/ramdisk.image.gz \
		--set-section-flags=.ramdisk=contents,alloc,load,readonly,data \
		--add-section=.image=../images/vmlinux.gz \
		--set-section-flags=.image=contents,alloc,load,readonly,data \
		../common/dummy.o image.o
	$(LD) $(LD_ARGS) -o $@ $(obj-y) image.o $(LIBS)
	$(OBJCOPY) $(OBJCOPY_ARGS) $@ $@ -R .comment -R .stab -R .stabstr \
		-R .sysmap

# Sort-of dummy rules, that let us format the image we want.
zImage: $(ZIMAGE)
	rm -f zvmlinux
zImage.initrd: $(ZIMAGEINITRD)
	rm -f zvmlinux.initrd

znetboot: zImage
ifneq ($(ZNETBOOT),)
	cp ../images/$(ZNETBOOT) $(TFTPIMAGE)
else
	cp ../images/zImage.* $(TFTPIMAGE)
endif

znetboot.initrd: zImage.initrd
ifneq ($(ZNETBOOTRD),)
	cp ../images/$(ZNETBOOTRD) $(TFTPIMAGE)
else
	cp ../images/zImage.* $(TFTPIMAGE)
endif

zImage-EMBEDDED: zvmlinux
	mv zvmlinux ../images/zImage.embedded

zImage.initrd-EMBEDDED: zvmlinux.initrd
	mv zvmlinux.initrd ../images/zImage.initrd.embedded

zImage-PAL4: zvmlinux
	cp zvmlinux ../images/zImage.pal4

zImage.initrd-PAL4: zvmlinux.initrd
	cp zvmlinux.initrd ../images/zImage.initrd.pal4

zImage-SMON: zvmlinux
	dd if=zvmlinux of=../images/zImage.gemini skip=64 bs=1k

zImage.initrd-SMON: zvmlinux.initrd
	dd if=zvmlinux.initrd of=../images/zImage.initrd.gemini skip=64 bs=1k

zImage-TREE: zvmlinux
	$(MKTREE) zvmlinux ../images/zImage.$(END) $(ENTRYPOINT)

zImage.initrd-TREE: zvmlinux.initrd
	$(MKTREE) zvmlinux.initrd ../images/zImage.initrd.$(END) $(ENTRYPOINT)

include $(TOPDIR)/Rules.make
