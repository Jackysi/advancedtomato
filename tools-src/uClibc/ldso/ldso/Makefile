# Makefile for uClibc
#
# Copyright (C) 2000 by Lineo, inc.
# Copyright (C) 2000-2002 Erik Andersen <andersen@uclibc.org>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Library General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Library General Public License for more
# details.
#
# You should have received a copy of the GNU Library General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
#
# Derived in part from the Linux-8086 C library, the GNU C Library, and several
# other sundry sources.  Files within this library are copyright by their
# respective copyright holders.


TOPDIR=../../
include $(TOPDIR)Rules.mak
LDSO_FULLNAME=ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so



XXFLAGS+=-DUCLIBC_TARGET_PREFIX=\"$(TARGET_PREFIX)\" \
	-DUCLIBC_DEVEL_PREFIX=\"$(DEVEL_PREFIX)\" \
	-DUCLIBC_BUILD_DIR=\"$(shell cd $(TOPDIR) && pwd)\"
LDFLAGS=$(CPU_LDFLAGS-y) -shared --warn-common --export-dynamic --sort-common \
	-z combreloc --discard-locals --discard-all
CSRC= ldso.c #hash.c readelflib1.c $(TARGET_ARCH)/elfinterp.c
COBJS=$(patsubst %.c,%.o, $(CSRC))
ASRC=$(shell ls $(TARGET_ARCH)/*.S)
AOBJS=$(patsubst %.S,%.o, $(ASRC))
OBJS=$(AOBJS) $(COBJS)

ifeq ($(strip $(TARGET_ARCH)),cris)
LDFLAGS+=-mcrislinux
endif

ifneq ($(strip $(DODEBUG)),y)
LDFLAGS+=-s
endif

ifeq ($(strip $(SUPPORT_LD_DEBUG)),y)
XXFLAGS+=-D__SUPPORT_LD_DEBUG__
endif

ifeq ($(strip $(SUPPORT_LD_DEBUG_EARLY)),y)
XXFLAGS+=-D__SUPPORT_LD_DEBUG_EARLY__
endif

ifeq ($(strip $(FORCE_SHAREABLE_TEXT_SEGMENTS)),y)
XXFLAGS+=-DFORCE_SHAREABLE_TEXT_SEGMENTS
endif

all: lib

lib:: ldso.h $(OBJS) $(DLINK_OBJS)
	$(LD) $(LDFLAGS) -e _dl_boot -soname=$(UCLIBC_LDSO) \
		-o $(LDSO_FULLNAME) $(OBJS) $(LIBGCC);
	install -d $(TOPDIR)lib 
	install -m 755 $(LDSO_FULLNAME) $(TOPDIR)lib
	(cd $(TOPDIR)lib && ln -sf $(LDSO_FULLNAME) $(UCLIBC_LDSO))

ldso.h: Makefile
	echo "const char *_dl_progname=\""$(UCLIBC_LDSO)"\";" > ldso.h
	echo "#include \"$(TARGET_ARCH)/elfinterp.c\"" >> ldso.h


$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) $(XXFLAGS) -I. -I./$(TARGET_ARCH) -I../libdl -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(AOBJS): %.o : %.S
	$(CC) $(CFLAGS) -I. -I./$(TARGET_ARCH) -I../libdl -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

ldso.o: ldso.c hash.c readelflib1.c $(TARGET_ARCH)/elfinterp.c \
	ld_hash.h ld_string.h ld_syscall.h ldso.h linuxelf.h

clean::
	$(RM) -f $(UCLIBC_LDSO)* $(OBJS) $(LDSO_FULLNAME)* core *.o *.a *.s *.i ldso.h *~

