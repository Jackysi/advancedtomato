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

TESTDIR=../
include $(TESTDIR)/Rules.mak


TARGETS=test_pwd test_pwd_glibc
TARGETS+=test_grp test_grp_glibc
TARGETS+=test_pwd_diff test_grp_diff

all: $(TARGETS)

test_pwd: test_pwd.c Makefile $(TESTDIR)/Config $(TESTDIR)/Rules.mak $(CC)
	-@ echo "-------"
	-@ echo " "
	-@ echo "Compiling vs uClibc: "
	-@ echo " "
	$(CC) $(CFLAGS) -c $< -o $@.o
	$(CC) $(LDFLAGS) $@.o -o $@ $(EXTRA_LIBS)
	$(STRIPTOOL) -x -R .note -R .comment $@
	-./$@ 2>&1 >test_pwd.out
	-@ echo " "

test_pwd_glibc: test_pwd.c Makefile
	-@ echo "-------"
	-@ echo " "
	-@ echo "Compiling vs GNU libc: "
	-@ echo " "
	$(HOSTCC) $(GLIBC_CFLAGS) -c $< -o $@.o
	$(HOSTCC) $(GLIBC_LDFLAGS) $@.o -o $@
	$(STRIPTOOL) -x -R .note -R .comment $@
	-./$@ 2>&1 >test_pwd_glibc.out
	-@ echo " "

test_grp: test_grp.c Makefile $(TESTDIR)/Config $(TESTDIR)/Rules.mak $(CC)
	-@ echo "-------"
	-@ echo " "
	-@ echo "Compiling vs uClibc: "
	-@ echo " "
	$(CC) $(CFLAGS) -c $< -o $@.o
	$(CC) $(LDFLAGS) $@.o -o $@ $(EXTRA_LIBS)
	$(STRIPTOOL) -x -R .note -R .comment $@
	-./$@ 2>&1 >test_grp.out
	-@ echo " "

test_grp_glibc: test_grp.c Makefile
	-@ echo "-------"
	-@ echo " "
	-@ echo "Compiling vs GNU libc: "
	-@ echo " "
	$(HOSTCC) $(GLIBC_CFLAGS) -c $< -o $@.o
	$(HOSTCC) $(GLIBC_LDFLAGS) $@.o -o $@
	$(STRIPTOOL) -x -R .note -R .comment $@
	-./$@ 2>&1 >test_grp_glibc.out
	-@ echo " "

test_pwd_diff: test_pwd_glibc test_pwd
	-@ echo "-------"
	-@ echo " "
	-@ echo "Diffing output: "
	-@ echo " "
	-diff -u test_pwd_glibc.out test_pwd.out
	-@ echo " "

test_grp_diff: test_grp_glibc test_grp
	-@ echo "-------"
	-@ echo " "
	-@ echo "Diffing output: "
	-@ echo " "
	-diff -u test_grp_glibc.out test_grp.out
	-@ echo " "

clean:
	rm -f *.[oa] *~ core $(TARGETS) *.out


