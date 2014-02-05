#
# Makefile for the Broadcom CTF
#
# Copyright (C) 2009, Broadcom Corporation
# All Rights Reserved.
# 
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
# $Id: Makefile,v 1.1 2009/10/27 01:27:44 Exp $
#

CTFSRC := ../../../../../ctf

CTFOBJS := $(CTFSRC)/hndctf.o $(CTFSRC)/hndctf_linux.o

# Search for sources under src/ctf
ifneq ($(wildcard $(src)/$(CTFSRC)/*.c),)
EXTRA_CFLAGS += -I$(CTFSRC)
ctf-objs := $(CTFOBJS) ctf_ksyms.o
else
ctf-objs := $(CTFSRC)/linux/ctf.o
endif

obj-$(CONFIG_BCM_CTF) := ctf.o

ifneq ($(KERNELRELEASE),)
# kbuild part of makefile
else
# Normal makefile
KERNELDIR := ../../../../../linux/linux-2.6
all:
	$(MAKE) -C $(KERNELDIR) M=`pwd`
clean:
	$(MAKE) -C $(KERNELDIR) M=`pwd` clean
endif

CTFKSYMOBJS := ../ctf/hndctf.o

PHONY += $(src)/ctf_ksyms.c

quiet_cmd_ksyms = CALL    $<
      cmd_ksyms = $(CONFIG_SHELL) -e $< $(CTFKSYMOBJS) > $@

$(src)/ctf_ksyms.c: $(src)/ctf_ksyms.sh $(CTFKSYMOBJS) FORCE
	$(call cmd,ksyms)

clean-files += $(CTFSRC)/*.o $(CTFSRC)/*.o $(CTFSRC)/.*.o.cmd
