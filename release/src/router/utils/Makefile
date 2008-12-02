#
# Miscellaneous utilities Makefile
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.10 2005/03/07 08:35:32 kanki Exp $
#
ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

UTILS := epi_ttcp
CFLAGS += -s -O2 -I$(SRCBASE)/include

vpath %.c $(SRCBASE)/tools/misc

ifneq ($(wildcard $(SRCBASE)/et/linux/et.c),)
UTILS += et
vpath %.c $(SRCBASE)/et/linux
endif


all: $(UTILS)


clean:
	rm -f *.o *~ $(UTILS)

install: all
ifeq ($(EMI_TEST),1)
	install -d $(INSTALLDIR)/usr/sbin
	install $(UTILS) $(INSTALLDIR)/usr/sbin
endif
	$(STRIP) $(foreach file,$(UTILS),$(INSTALLDIR)/usr/sbin/$(file))
