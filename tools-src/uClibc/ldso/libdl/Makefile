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


TOPDIR=../../
include $(TOPDIR)Rules.mak

XXFLAGS+=-DUCLIBC_TARGET_PREFIX=\"$(TARGET_PREFIX)\" \
	-DUCLIBC_DEVEL_PREFIX=\"$(DEVEL_PREFIX)\" \
	-DUCLIBC_BUILD_DIR=\"$(shell cd $(TOPDIR) && pwd)\"
ifeq ($(strip $(SUPPORT_LD_DEBUG)),y)
XXFLAGS+=-D__SUPPORT_LD_DEBUG__
endif

LIBDL=libdl.a
LIBDL_PIC=libdl_pic.a
LIBDL_SHARED=libdl.so
LIBDL_SHARED_FULLNAME=libdl-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so

CSRC=dlib.c
OBJS=dlib.o
PIC_OBJS=dlib_pic.o

all: $(OBJS) $(LIBDL) shared

$(LIBDL): ar-target

ar-target: $(OBJS) $(PIC_OBJS)
	$(AR) $(ARFLAGS) $(LIBDL) ../ldso/$(TARGET_ARCH)/resolve.o $(OBJS)
	$(AR) $(ARFLAGS) $(LIBDL_PIC) $(PIC_OBJS)
	install -d $(TOPDIR)lib
	rm -f $(TOPDIR)lib/$(LIBDL)
	install -m 644 $(LIBDL) $(TOPDIR)lib


dlib.o: dlib.c
	$(CC) -I../ldso -I../ldso/$(TARGET_ARCH) $(CFLAGS_NOPIC) $(XXFLAGS) -c dlib.c -o dlib.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

dlib_pic.o: dlib.c
	$(CC) -I../ldso -I../ldso/$(TARGET_ARCH) $(CFLAGS) $(XXFLAGS) -c dlib.c -o dlib_pic.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(OBJ): Makefile

shared:
	$(LD) $(LDFLAGS) -soname=$(LIBDL_SHARED).$(MAJOR_VERSION) \
		-o $(LIBDL_SHARED_FULLNAME) --whole-archive $(LIBDL_PIC) \
		--no-whole-archive $(TOPDIR)/libc/misc/internals/interp.o \
		-L$(TOPDIR)/lib -lc;
	install -d $(TOPDIR)lib
	rm -f $(TOPDIR)lib/$(LIBDL_SHARED_FULLNAME) $(TOPDIR)lib/$(LIBDL_SHARED).$(MAJOR_VERSION)
	install -m 644 $(LIBDL_SHARED_FULLNAME) $(TOPDIR)lib;
	(cd $(TOPDIR)lib && ln -sf $(LIBDL_SHARED_FULLNAME) $(LIBDL_SHARED)); 
	(cd $(TOPDIR)lib && ln -sf $(LIBDL_SHARED_FULLNAME) $(LIBDL_SHARED).$(MAJOR_VERSION)); 

clean:
	$(RM) -f .depend $(LIBDL_SHARED)* $(LIBDL_SHARED_FULLNAME) core *.o *.a *.s *.i tmp_make foo *~

