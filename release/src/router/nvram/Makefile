#
# Linux NVRAM Makefile
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.8 2005/03/07 08:35:32 kanki Exp $
#

CFLAGS	+= -I. -I$(SRCBASE)/include -Wall -I$(SRCBASE)/router/shared
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS += -L.

vpath %.c $(SRCBASE)/shared/nvram

all: libnvram.so nvram

install: all
	install -D libnvram.so $(INSTALLDIR)/usr/lib/libnvram.so
	$(STRIP) $(INSTALLDIR)/usr/lib/libnvram.so
	install -D nvram $(INSTALLDIR)/usr/sbin/nvram
	$(STRIP) $(INSTALLDIR)/usr/sbin/nvram

clean:
	rm -f *.o *.a *.so nvram

libnvram.so: nvram_linux.o nvram_convert.o
	$(LD) -shared -o $@ $^

nvram: main.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) -lnvram
