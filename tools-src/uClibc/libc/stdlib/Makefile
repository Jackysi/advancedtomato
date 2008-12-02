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

DIRS:=
ifeq ($(MALLOC),y)
    DIRS+=malloc
endif
ifeq ($(MALLOC_930716),y)
    DIRS+=malloc-930716
endif


MSRC = stdlib.c
MOBJ = abs.o labs.o atoi.o atol.o strtol.o strtoul.o _stdlib_strto_l.o \
       qsort.o bsearch.o \
       llabs.o atoll.o strtoll.o strtoull.o _stdlib_strto_ll.o
# (aliases) strtoq.o strtouq.o
ifeq ($(UCLIBC_HAS_FLOATS),y)
	MOBJ += atof.o
endif

ifeq ($(UCLIBC_HAS_WCHAR),y)
	MOBJ += mblen.o mbtowc.o wctomb.o mbstowcs.o wcstombs.o \
		_stdlib_mb_cur_max.o _stdlib_wcsto_l.o _stdlib_wcsto_ll.o \
		wcstol.o wcstoul.o wcstoll.o wcstoull.o
# (aliases) wcstoq.o wcstouq.o
# wcstod wcstof wcstold
endif

MSRC2=atexit.c
MOBJ2=atexit.o on_exit.o __exit_handler.o exit.o
CSRC =	abort.c getenv.c mkdtemp.c mktemp.c realpath.c mkstemp.c mkstemp64.c \
	rand.c random.c random_r.c setenv.c system.c div.c ldiv.c getpt.c \
	ptsname.c grantpt.c unlockpt.c gcvt.c drand48.c drand48-iter.c \
	drand48_r.c erand48.c erand48_r.c jrand48.c jrand48_r.c lrand48.c \
	lrand48_r.c mrand48.c mrand48_r.c nrand48.c nrand48_r.c rand_r.c \
	srand48.c srand48_r.c calloc.c valloc.c
ifeq ($(UCLIBC_HAS_FLOATS),y)
	CSRC += strtod.c strtof.c strtold.c
endif
COBJS=$(patsubst %.c,%.o, $(CSRC))

OBJS=$(MOBJ) $(MOBJ2) $(COBJS)

all: $(OBJS) $(LIBC)

$(LIBC): ar-target subdirs

ar-target: $(OBJS)
	$(AR) $(ARFLAGS) $(LIBC) $(OBJS)

$(MOBJ): $(MSRC)
	$(CC) $(CFLAGS) -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(MOBJ2): $(MSRC2)
	$(CC) $(CFLAGS) -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(OBJ): Makefile

subdirs: $(patsubst %, _dir_%, $(DIRS))
subdirs_clean: $(patsubst %, _dirclean_%, $(ALL_SUBDIRS))

$(patsubst %, _dir_%, $(DIRS)) : dummy
	$(MAKE) -C $(patsubst _dir_%, %, $@)

$(patsubst %, _dirclean_%, $(ALL_SUBDIRS)) : dummy
	$(MAKE) -C $(patsubst _dirclean_%, %, $@) clean

clean: subdirs_clean
	rm -f *.[oa] *~ core

.PHONY: dummy


