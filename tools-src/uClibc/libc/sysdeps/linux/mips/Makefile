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

CRT0_SRC = crt0.S
CRT0_OBJ = crt0.o crt1.o

SSRC=bsd-_setjmp.S bsd-setjmp.S setjmp.S clone.S fork.S syscall.S pipe.S
SOBJS=$(patsubst %.S,%.o, $(SSRC))

CSRC=__longjmp.c  brk.c vfork.c setjmp_aux.c _mmap.c __syscall_error.c cacheflush.c
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
	cd $(TOPDIR)/include && ln -fs ../libc/sysdeps/linux/mips/sgidefs.h .
#	cd $(TOPDIR)/include && ln -fs ../libc/sysdeps/linux/mips/regdef.h .

clean:
	rm -f *.[oa] *~ core
	rm -f bits/sysnum.h
	rm -f $(TOPDIR)/include/sgidefs.h

