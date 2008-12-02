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

CSRC=execl.c execlp.c execv.c execvep.c execvp.c execle.c \
	sleep.c usleep.c getpass.c sysconf_src.c getlogin.c \
	fpathconf.c confstr.c pathconf.c swab.c usershell.c \
	getopt.c getsubopt.c
ifeq ($(strip $(UCLIBC_HAS_MMU)),y)
    CSRC+=daemon.c
endif

# TESTING -- comment this out if it breaks for you
ifeq ($(TARGET_ARCH), $(HOST_ARCH))
	SYSCONF = sysconf_native
else
	SYSCONF = sysconf_$(TARGET_ARCH).c
endif

COBJS=$(patsubst %.c,%.o, $(CSRC))
OBJS=$(COBJS)

all: $(SYSCONF) $(OBJS) $(LIBC)

$(LIBC): ar-target subdirs

ar-target: $(OBJS)
	$(AR) $(ARFLAGS) $(LIBC) $(OBJS)

# We are cross-compiling so use the generic sysconf.c.
sysconf_$(TARGET_ARCH).c: sysconf.c
	@echo warning: sysconf_$(HOST_ARCH).c is older then sysconf.c so using generic sysconf.c
	@echo To build sysconf_$(HOST_ARCH).c run gen_sysconf \> sysconf_$(HOST_ARCH).c on
	@echo your target platform, place in the unistd directory, and rebuild
	cp -f sysconf.c sysconf_$(TARGET_ARCH).c

# We are compiling for the native platform, so build an optimized sysconf.c.
getpagesize_tester.o:
	$(CC) $(CFLAGS) -D_UCLIBC_GENERATE_SYSCONF_ARCH \
	    -c ../sysdeps/linux/common/getpagesize.c -o getpagesize_tester.o

sysconf_tester.o: sysconf.c
	$(CC) $(CFLAGS) -D_UCLIBC_GENERATE_SYSCONF_ARCH -c sysconf.c \
	    -o sysconf_tester.o

gen_sysconf_tester.o: sysconf_tester.o getpagesize_tester.o
	@ld -r -o gen_sysconf_tester.o sysconf_tester.o getpagesize_tester.o
	@if nm -s gen_sysconf_tester.o | grep -v "U errno" | grep " U "; then \
		set -e -x; \
		echo warning: missing symbols in gen_sysconf_tester.o so using generic sysconf.c ;\
		cp -f sysconf.c sysconf_src.c ;\
	else \
		set -e -x; \
		if ../extra/gcc-uClibc/gcc-uClibc-$(HOST_ARCH) -static \
			-D_UCLIBC_GENERATE_SYSCONF_MAIN sysconf.c sysconf_tester.o \
			-o gen_sysconf && ./gen_sysconf > sysconf_$(HOST_ARCH).c ;\
		then \
			echo successfully built sysconf_$(HOST_ARCH).c ;\
		else \
			echo warning: build of gen_sysconf failed so using generic sysconf.c ;\
			cp -f sysconf.c sysconf_$(HOST_ARCH).c ;\
		fi ;\
	fi

sysconf_native: gen_sysconf_tester.o

sysconf_src.c: sysconf_$(TARGET_ARCH).c
	cp -f sysconf_$(TARGET_ARCH).c sysconf_src.c


$(COBJS): %.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@
	$(STRIPTOOL) -x -R .note -R .comment $*.o

$(OBJ): Makefile

subdirs: $(patsubst %, _dir_%, $(DIRS))
subdirs_clean: $(patsubst %, _dirclean_%, $(DIRS))

$(patsubst %, _dir_%, $(DIRS)) : dummy
	$(MAKE) -C $(patsubst _dir_%, %, $@)

$(patsubst %, _dirclean_%, $(DIRS)) : dummy
	$(MAKE) -C $(patsubst _dirclean_%, %, $@) clean

.PHONY: dummy sysconf_native
dummy:

clean:
	rm -f *.[oa] *~ core gen_sysconf sysconf_*.c

