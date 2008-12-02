# Makefile for uClibc
#
# Copyright (C) 2000 by Lineo, inc.
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

MSRCW= wstring.c
MOBJW=  basename.o bcopy.o bzero.o dirname.o ffs.o memccpy.o memchr.o memcmp.o \
	memcpy.o memmove.o mempcpy.o memrchr.o 	memset.o rawmemchr.o stpcpy.o \
	stpncpy.o strcasecmp.o strcasestr.o strcat.o strchrnul.o strchr.o \
	strcmp.o strcpy.o strcspn.o strdup.o strlen.o strncasecmp.o strncat.o \
	strncmp.o strncpy.o strndup.o strnlen.o strpbrk.o strrchr.o strsep.o \
	strspn.o strstr.o strtok.o strtok_r.o strerror.o _susv3_strerror_r.o \
	_string_syserrmsgs.o _glibc_strerror_r.o \
	_string_syssigmsgs.o sys_siglist.o strsignal.o psignal.o \
	 __xpg_basename.o strlcat.o strlcpy.o sys_errlist.o

MOBJW2= wcscasecmp.o wcscat.o wcschrnul.o wcschr.o wcscmp.o wcscpy.o wcscspn.o \
	wcsdup.o wcslen.o wcsncasecmp.o wcsncat.o wcsncmp.o wcsncpy.o \
	wcsnlen.o wcspbrk.o wcsrchr.o wcsspn.o wcsstr.o wcstok.o wmemchr.o \
	wmemcmp.o wmemcpy.o wmemmove.o wmempcpy.o wmemset.o wcpcpy.o wcpncpy.o \
	__wcslcpy.o \
	wcsxfrm.o strxfrm.o # wcscoll strcoll.o

OBJS=$(MOBJ) $(MOBJ1) $(MOBJ2) $(MOBJW)

ifeq ($(UCLIBC_HAS_WCHAR),y)
	OBJS += $(MOBJW2)
endif

all: $(OBJS) $(LIBC)

$(LIBC): ar-target

ar-target: $(OBJS)
	$(AR) $(ARFLAGS) $(LIBC) $(OBJS)

$(MOBJW): $(MSRCW)
	$(CC) $(CFLAGS) -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(MOBJW2): $(MSRCW)
	$(CC) $(CFLAGS) -DWANT_WIDE -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

clean:
	rm -f *.[oa] *~ core

