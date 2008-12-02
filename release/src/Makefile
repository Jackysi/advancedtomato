#
# Toplevel Makefile for the BCM947xx Linux Router release
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.53.22.6 2006/07/20 01:22:37 honor Exp $
#

SRCBASE := $(shell pwd)
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)

install: cytools all
	install -d $(RELEASEDIR)/image
	$(MAKE) -C router install
	#-cp router/mipsel/linux.trx $(RELEASEDIR)/image/linux-glibc.trx
	-cp router/mipsel-uclibc/linux.trx $(RELEASEDIR)/image/linux.trx
	-cp router/mipsel-uclibc/linux.bin $(RELEASEDIR)/image/linux.bin
ifneq ($(wildcard pmon),)
	cp pmon/pmon_lg.bin $(RELEASEDIR)/image/
endif
ifneq ($(wildcard cfe),)
	cp cfe/build/broadcom/bcm947xx/cfe.bin $(RELEASEDIR)/image/
	# Make CFE again for self-decompression support
	$(MAKE) -C cfe
	cp cfe/build/broadcom/bcm947xx/compressed/cfez.bin $(RELEASEDIR)/image/
endif
	rm -rf shared/nvram/*4320*.txt 
	-cp shared/nvram/*.txt $(RELEASEDIR)/image/

	# add CyberTAN header
	addpattern -i $(RELEASEDIR)/image/linux.trx -o $(RELEASEDIR)/image/code.bin -g
	# below for multiple language
	if grep -q "MULTILANG_SUPPORT=1" cy_conf.mak ; then \
		rm -rf ../image/*lang.trx ; \
		rm -rf ../image/*lang.bin ; \
		rm -rf ../image/*lcode.* ; \
		cp router/mipsel-uclibc/lcode.trx $(RELEASEDIR)/image/lcode.trx ;\
		cp router/mipsel-uclibc/default_lang.trx $(RELEASEDIR)/image/default_lang.trx ;\
		addlang -i $(RELEASEDIR)/image/default_lang.trx -o $(RELEASEDIR)/image/default_lang.bin ;\
		appendfile -i $(RELEASEDIR)/image/lcode.trx -o $(RELEASEDIR)/image/lcode.bin -g ;\
		cat $(RELEASEDIR)/image/lcode.bin $(RELEASEDIR)/image/default_lang.bin > $(RELEASEDIR)/image/tmp_lcode.bin ;\
		addpattern -i $(RELEASEDIR)/image/tmp_lcode.bin -o $(RELEASEDIR)/image/def_code.bin ;\
	fi

all clean:
	$(MAKE) -C router $@

ifneq ($(wildcard pmon),)
	$(MAKE) -C pmon $@
endif
ifneq ($(wildcard cfe),)
	$(MAKE) -C cfe $@
endif

cytools:

.PHONY: all clean install
