# Makefile for uClibc
#
# Copyright (C) 2000-2003 Erik Andersen <andersen@uclibc.org>
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

TOPDIR=../../../../
include $(TOPDIR)Rules.mak

CSRC=	waitpid.c getdnnm.c gethstnm.c getcwd.c ptrace.c \
	mkfifo.c setegid.c wait.c getpagesize.c seteuid.c \
	wait3.c setpgrp.c getdtablesize.c create_module.c \
	cmsg_nxthdr.c longjmp.c open64.c ftruncate64.c mmap64.c \
	truncate64.c getrlimit64.c setrlimit64.c creat64.c \
	llseek.c pread_write.c _exit.c sync.c getdirname.c \
	sendfile64.c xstatconv.c getdents.c getdents64.c
ifneq ($(strip $(EXCLUDE_BRK)),y)
CSRC+=sbrk.c
endif
COBJS=$(patsubst %.c,%.o, $(CSRC))

MSRC=syscalls.c
MOBJ=$(shell ./list_syscalls.sh)

CTOR_TARGETS=$(TOPDIR)lib/crti.o $(TOPDIR)lib/crtn.o
SAFECFLAGS=$(XWARNINGS) $(OPTIMIZATION) $(XARCH_CFLAGS) $(CPU_CFLAGS) -fno-builtin
#
# on SH4 addition of -g kills the initfini stuff, removed here for all
# platforms as I suspect no one want initfini compiled -g :-) - davidm
#
SAFECFLAGS := $(subst -g,,$(SAFECFLAGS))
ifeq ($(strip $(DOPIC)),y)
SAFECFLAGS+=-fPIC
endif

OBJ=$(COBJS) $(MOBJ)

all: $(STR_SYSCALLS) $(OBJ) $(LIBC)

$(LIBC): ar-target

ar-target: $(OBJ) $(CTOR_TARGETS)
	$(AR) $(ARFLAGS) $(LIBC) $(OBJ)

$(MOBJ): $(MSRC)
	$(CC) $(CFLAGS) -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

ifeq ($(strip $(UCLIBC_CTOR_DTOR)),y)
initfini.S: initfini.c
	$(CC) $(SAFECFLAGS) -I$(TOPDIR)include -c initfini.c -S -o initfini.S

crti.S crtn.S: initfini.S $(TOPDIR)/extra/scripts/initfini.awk
	$(AWK) -f $(TOPDIR)extra/scripts/initfini.awk initfini.S

crti.o: crti.S
	$(CC) $(SAFECFLAGS) -c crti.S -o crti.o

$(TOPDIR)lib/crti.o: crti.o
	mkdir -p $(TOPDIR)lib/
	cp crti.o $(TOPDIR)lib/

crtn.o: crtn.S
	$(CC) $(SAFECFLAGS) -c crtn.S -o crtn.o

$(TOPDIR)lib/crtn.o: crtn.o
	mkdir -p $(TOPDIR)lib/
	cp crtn.o $(TOPDIR)lib/
else
$(TOPDIR)lib/crti.o:
	mkdir -p $(TOPDIR)lib/
	$(AR) $(ARFLAGS) $(TOPDIR)lib/crti.o
$(TOPDIR)lib/crtn.o:
	mkdir -p $(TOPDIR)lib/
	$(AR) $(ARFLAGS) $(TOPDIR)lib/crtn.o
endif

clean:
	rm -f *.[oa] *~ core crt[in].* *.S

