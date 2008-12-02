# $Id: Makefile,v 1.1.1.4 2003/10/14 08:07:48 sparq Exp $
# Makefile for Sparc library files..
#

.S.s:
	$(CPP) $(AFLAGS) -ansi -DST_DIV0=0x2 $< -o $*.s

.S.o:
	$(CC) $(AFLAGS) -ansi -DST_DIV0=0x2 -c $< -o $*.o

L_TARGET = lib.a

obj-y := mul.o rem.o sdiv.o udiv.o umul.o urem.o ashrdi3.o memcpy.o memset.o \
         strlen.o checksum.o blockops.o memscan.o memcmp.o strncmp.o \
	 strncpy_from_user.o divdi3.o udivdi3.o strlen_user.o \
	 copy_user.o locks.o atomic.o bitops.o debuglocks.o lshrdi3.o \
	 ashldi3.o rwsem.o muldi3.o

include $(TOPDIR)/Rules.make
