#
# Makefile for Broadcom BCM947XX boards
#
# Copyright 2006, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id$
#


# Link at 3 MB offset in RAM
TEXT_START	?= 0x80300000


OBJCOPY		:= $(CROSS_COMPILE)objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S
OBJCOPYSREC	:= $(CROSS_COMPILE)objcopy -O srec -R .reginfo -R .note -R .comment -R .mdebug -S

vpath %.c $(SRCBASE)/shared
vpath %.S $(SRCBASE)/shared
vpath %.lds.in $(SRCBASE)/shared

ASFLAGS		+= -D__ASSEMBLY__ -DLOADADDR=$(LOADADDR)
CFLAGS		+= -DLOADADDR=$(LOADADDR)
# The self-decompresor is standalone, get rid of the linux flags
CFLAGS		:= $(subst -Dlinux,,$(CFLAGS))
CFLAGS		+= -Ulinux -D_MINOSL_ -ffreestanding
ifdef CONFIG_MCOUNT
CFLAGS		:= $(subst -pg,,$(CFLAGS))
endif
CFLAGS		+= -ffunction-sections $(call check_gcc, -fvtable-gc, )

ASFLAGS		+=-I$(SRCBASE)/include -DBCMDRIVER

SYSTEM		?= $(TOPDIR)/vmlinux
OBJECTS         := boot.o sbsdram.o load.o sflash.o hndmips.o hndchipc.o hndpmu.o \
		bcmutils.o sbutils.o min_osl.o sromstubs.o nvramstubs.o bcmstdlib.o

# Default to bzip2
COMPRESS	?= bzip2 -c

ifneq ($(findstring gzip,$(COMPRESS)),)
CFLAGS		+= -DUSE_GZIP
else
ifneq ($(findstring bzip2,$(COMPRESS)),)
CFLAGS		+= -DUSE_BZIP2
else
COMPRESS	:= cat
endif
endif

all: zImage vmlinuz

# Don't build dependencies, this may die if $(CC) isn't gcc
dep:

zImage: vmlinux
	$(OBJCOPY) $< $@

# Link the loader and the kernel binary together
vmlinux: vmlinux.lds $(OBJECTS) piggy.o
	$(LD) -static --gc-sections -no-warn-mismatch -T vmlinux.lds -o $@ $(OBJECTS) piggy.o

vmlinux.lds: hndrte.lds.in Makefile
	@sed -e s/TEXT_START/$(TEXT_START)/ \
	     -e s/TARGET_ARCH/mips/ < $< > $@

# Create a linkable version of the (possibly compressed) kernel binary
piggy.o: piggz piggy.lds
	$(LD) -no-warn-mismatch -T piggy.lds -r -o $@ -b binary piggz -b elf32-tradlittlemips

piggy.lds:
	@echo "SECTIONS { .data : { input_len = .; LONG(input_data_end - input_data) input_data = .; *(.data) input_data_end = .; }}" > $@

# Always create a gzipped version named vmlinuz for compatibility
vmlinuz: piggy
	$(MAKE) -C $(SRCBASE)/lzma/CPP/7zip/Compress/LZMA_Alone -f makefile.gcc
	$(SRCBASE)/lzma/CPP/7zip/Compress/LZMA_Alone/lzma e piggy vmlinuz -eos -lc1 -lp2 -pb2

piggz: piggy
	$(COMPRESS) $< > $@

piggy: $(SYSTEM)
	$(OBJCOPY) $< $@

mrproper: clean

clean:
	rm -f vmlinux vmlinuz zImage piggz piggy *.lds *.o
