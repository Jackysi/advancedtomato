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

all: dltest2 dltest libhowdy.so run
all: dlttest dlttest2 dltest2 dltest libhowdy.so run

dlttest.o: dlttest.c
	$(CC) $(CFLAGS) -c dlttest.c -o dlttest.o

dlttest2.o: dlttest.c
	$(CC) $(CFLAGS) -DFORCE -c dlttest.c -o dlttest2.o

dltest2: dltest2.c
	$(CC) $(CFLAGS) dltest2.c -o dltest2 -ldl
	./dltest2

dltest.o: dltest.c
	$(CC) $(CFLAGS) -c dltest.c -o dltest.o

howdy.o: howdy.c
	$(CC) $(CFLAGS) -fPIC -c howdy.c -o howdy.o

libhowdy.so: howdy.o
	$(CC) $(CFLAGS) -shared -o libhowdy.so -Wl,-soname,libhowdy.so howdy.o

dltest: dltest.o
	$(CC) $(CFLAGS) -o dltest dltest.o -ldl

dlttest: dlttest.o
	$(CC) $(CFLAGS) -o dlttest dlttest.o -ldl -lpthread

dlttest2: dlttest2.o
	$(CC) $(CFLAGS) -o dlttest2 dlttest2.o -ldl -lpthread

run: dltest dlttest dlttest2 libhowdy.so
	@echo Running dltest
	./dltest
	@echo Running dlttest
	./dlttest
	@echo Running dlttest2
	./dlttest2

clean:
	rm -f *.o *.so dltest2 dltest core libhowdy.so dlttest dlttest2
