# Makefile for uClibc
#
# Copyright (C) 2000,2001 Erik Andersen <andersen@uclibc.org>
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
TARGET_CC = $(TOPDIR)extra/gcc-uClibc/$(TARGET_ARCH)-uclibc-gcc

TARGETS = elf_header ldd
ifeq ($(OSTYPE),linux)
TARGETS += readelf
endif
ifneq ($(strip $(LIBRARY_CACHE)),)
TARGETS += ldconfig
endif
all: $(TARGETS)

ifeq ($(strip $(LDSO_LDD_SUPPORT)),y)
XXFLAGS=-D__LDSO_LDD_SUPPORT
endif

elf_header:
	ln -fs $(TOPDIR)include/elf.h

readelf: readelf.c
	$(HOSTCC) $(HOSTCFLAGS) -I . readelf.c -o $@
	strip -x -R .note -R .comment $@

readelf.target: readelf.c
	$(TARGET_CC) $(CFLAGS) --uclibc-use-build-dir -Wl,-s readelf.c -o $@
	$(STRIPTOOL) -x -R .note -R .comment $@

readsoname.o: readsoname.c readsoname2.c
	$(HOSTCC) $(HOSTCFLAGS) -I . -c $< -o $@
	strip -x -R .note -R .comment $*.o
    
ldconfig.o: ldconfig.c
	$(HOSTCC) $(HOSTCFLAGS) -I . \
		-DUCLIBC_TARGET_PREFIX=\"$(TARGET_PREFIX)\" -c $< -o $@
	strip -x -R .note -R .comment $*.o

ldconfig: ldconfig.o readsoname.o
	$(HOSTCC) $(HOSTCFLAGS) $^ -o $@
	strip -x -R .note -R .comment $@

ldd: ldd.c
	$(HOSTCC) $(HOSTCFLAGS) $(XXFLAGS) -DUCLIBC_TARGET_PREFIX=\"$(TARGET_PREFIX)\" \
		-DUCLIBC_DEVEL_PREFIX=\"$(DEVEL_PREFIX)\" \
		-DUCLIBC_BUILD_DIR=\"$(shell cd $(TOPDIR) && pwd)\" \
		-DUCLIBC_LDSO=\"$(UCLIBC_LDSO)\" -I . \
		ldd.c -o $@ 
	strip -x -R .note -R .comment $@

ldd.target: ldd.c
	$(TARGET_CC) $(CFLAGS) $(XXFLAGS) -Wl,-s -DUCLIBC_TARGET_PREFIX=\"$(TARGET_PREFIX)\" \
		-DUCLIBC_DEVEL_PREFIX=\"$(DEVEL_PREFIX)\" \
		-DUCLIBC_BUILD_DIR=\"$(shell cd $(TOPDIR) && pwd)\" \
		-DUCLIBC_LDSO=\"$(UCLIBC_LDSO)\" \
		ldd.c -o $@
	$(STRIPTOOL) -x -R .note -R .comment $@

clean:
	rm -f $(TARGETS) *.o *~ core ./elf.h *.target

