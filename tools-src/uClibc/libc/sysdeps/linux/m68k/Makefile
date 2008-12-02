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
ASFLAGS=$(CFLAGS)

# If you're looking for vfork(), it is defined in include/unistd.h


ifeq ($(UCLIBC_HAS_MMU),y)
CRT0_SRC = crt0.c
else
CRT0_SRC = crt0.S
endif
CRT0_OBJ = crt0.o crt1.o

SSRC= __longjmp.S bsd-_setjmp.S bsd-setjmp.S clone.S setjmp.S vfork.S
SOBJS=$(patsubst %.S,%.o, $(SSRC))

CSRC=ptrace.c brk.c
COBJS=$(patsubst %.c,%.o, $(CSRC))

OBJS=$(SOBJS) $(MOBJ) $(COBJS)



all: $(OBJS) $(LIBC)

$(LIBC): ar-target 

ar-target: $(OBJS) $(CRT0_OBJ)
	$(AR) $(ARFLAGS) $(LIBC) $(OBJS)
	cp $(CRT0_OBJ) $(TOPDIR)lib/

$(CRT0_OBJ): $(CRT0_SRC)
	$(CC) $(CFLAGS) -DL_$* $< -c -o $*.o
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(SOBJS): %.o : %.S
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

headers:
ifneq ($(strip $(HAVE_ELF)),y)
	echo "Working around compiler bug in the m68k-pic-coff toolchain"
	cd $(TOPDIR)/include && ln -fs ../libc/sysdeps/linux/m68k/float.h .
endif


clean:
	rm -f *.[oa] *~ core
	rm -f bits/sysnum.h
ifneq ($(strip $(HAVE_ELF)),y)
	rm -f $(TOPDIR)/include/float.h
endif

