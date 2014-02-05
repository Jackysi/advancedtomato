#
# Miscellaneous utilities Makefile
#
# Copyright 2004, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id$
#

include ../common.mak

UTILS := mii-tool robocfg ttcp
CFLAGS += -s -Os -I$(SRCBASE)/include -I$(LINUXDIR)/include -Wall $(EXTRACFLAGS)
ifneq ($(CONFIG_LINUX26),y)
CFLAGS += -U CONFIG_NVRAM_SIZE
endif

all: $(UTILS)

mii-tool: mii-tool.c mii.h

robocfg: robocfg.c etc53xx.h

ttcp: epi_ttcp.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o *~ $(UTILS)

install: all
	install -d $(INSTALLDIR)/usr/sbin
	install $(UTILS) $(INSTALLDIR)/usr/sbin
	$(STRIP) $(foreach file,$(UTILS),$(INSTALLDIR)/usr/sbin/$(file))
