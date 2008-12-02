#
# Linux router shared code Makefile
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.22 2005/03/07 08:35:32 kanki Exp $
#
ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

CFLAGS	+= -I. -I../httpd -I$(SRCBASE)/include -Wall -I$(SRCBASE)/
#CFLAGS	+= -g -DDEBUG
CFLAGS	+= -s -O2
LDFLAGS += -L.

OBJS := nvparse.o shutils.o wl.o wl_linux.o linux_timer.o defaults.o utils.o support.o detectwan.o

all: libshared.so libshared.a

install: all
	install -d $(INSTALLDIR)/usr/lib
	install -m 755 libshared.so $(INSTALLDIR)/usr/lib
	$(STRIP) $(INSTALLDIR)/usr/lib/libshared.so

clean:
	rm -f *.o *.so

libshared.so: $(OBJS)
	$(LD) -shared -o $@ $^

libshared.a: $(OBJS)
	$(AR) arc -o $@ $^

*.o: $(CY_DEPS)
