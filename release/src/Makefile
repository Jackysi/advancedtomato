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
# $Id: Makefile,v 1.53 2005/04/25 03:54:37 tallest Exp $
#

SRCBASE := $(shell pwd)
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)

-include tomato_profile.mak

ifeq ($(TOMATO_PROFILE),)

all:
	$(MAKE) libc
	$(MAKE) g
	$(MAKE)

else

all:
	@echo ""
	@echo ""
	@echo "$(TOMATO_PROFILE_NAME) Profile"
	@btools/uversion.pl --gen
	@echo ""
	@echo ""
	
	@$(MAKE) -C router all
	@$(MAKE) -C router install
	@$(MAKE) -C btools

	@rm -rf image
	@mkdir image
ifeq ($(wildcard include/bcm20xx.h),)
	@btools/fpkg -i linux/linux/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -i router/mipsel-uclibc/target.image \
		-t image/tomato.trx \
		-l W54G,image/WRT54G_WRT54GL.bin \
		-l W54S,image/WRT54GS.bin \
		-l W54s,image/WRT54GSv4.bin \
		-l W54U,image/WRTSL54GS.bin \
		-m 0x10577050,image/WR850G.bin \
		| tee fpkg.log
else
	@btools/fpkg -i linux/linux/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -i router/mipsel-uclibc/target.image \
		-t image/tomato-ND.trx
endif


	@echo ""
	@echo "-----------------"
	@echo `cat router/shared/tomato_version` " ready"
	@echo "-----------------"

ifneq ($(NOVERSION),1)	
	@cp router/shared/tomato_version router/shared/tomato_version_last
	@btools/uversion.pl --bump
endif


clean: cleanimage
	@$(MAKE) -C router $@
	@rm -f fpkg.log

cleanimage:
	@rm -rf image

cleantools:
	@$(MAKE) -C linux/linux/scripts/squashfs clean
	@$(MAKE) -C btools clean
	
cleankernel:
	@cd linux/linux && \
	mv .config save-config && \
	$(MAKE) distclean; \
	mv save-config .config

distclean: clean cleankernel cleantools cleanlibc
ifneq ($(INSIDE_MAK),1)
	@$(MAKE) -C router $@ INSIDE_MAK=1
endif
	@rm -f router/config_current
	@rm -f router/.config.cmd router/.config.old
	@rm -f router/libfoo_xref.txt
	@rm -f tomato_profile.mak router/shared/tomato_profile.h

prepk:
	$(MAKE) -C linux/linux oldconfig dep

what:
	@echo ""
	@echo "$(TOMATO_PROFILE_NAME) Profile\n"

endif


g:
	@$(MAKE) setprofile U=G L=g N=G

setprofile:
	@echo "#ifndef TOMATO_PROFILE"                > router/shared/tomato_profile.h
	@echo "#define TOMATO_$(U) 1"                >> router/shared/tomato_profile.h
	@echo "#define PROFILE_G 1"                  >> router/shared/tomato_profile.h
	@echo "#define PROFILE_N 2"                  >> router/shared/tomato_profile.h
	@echo "#define TOMATO_PROFILE PROFILE_$(U)"  >> router/shared/tomato_profile.h
	@echo "#define TOMATO_PROFILE_NAME \"$(N)\"" >> router/shared/tomato_profile.h
	@echo "#endif"                               >> router/shared/tomato_profile.h

	@echo "TOMATO_$(U) = 1"                     > tomato_profile.mak
	@echo "PROFILE_G = 1"                      >> tomato_profile.mak
	@echo "PROFILE_N = 2"                      >> tomato_profile.mak
	@echo "TOMATO_PROFILE = \$$(PROFILE_$(U))" >> tomato_profile.mak
	@echo "TOMATO_PROFILE_NAME = \"$(N)\""     >> tomato_profile.mak
	@echo "TOMATO_PROFILE_L = $(L)"            >> tomato_profile.mak
	@echo "TOMATO_PROFILE_U = $(U)"            >> tomato_profile.mak

	@cd router && \
		[ -s .config ] && cat .config > config_current; \
		rm -f config_current && \
		ln -s config_$(L) config_current && \
		cp config_$(L) .config
	@$(MAKE) -C router oldconfig

	@echo ""
	@echo "Using $(N) profile."
	@echo ""


cleanlibc:
	@$(MAKE) -C ../../tools-src/uClibc clean

libc: cleanlibc
	@$(MAKE) -C ../../tools-src/uClibc
	@$(MAKE) -C ../../tools-src/uClibc install

echotest:
	@( echo -e foo | grep "\-e foo" ) && echo bar || echo foo

help:
	@echo "g            use G profile"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C linux/linux distclean (but preserves .config)"
	@echo "prepk        -C linux/linux oldconfig dep"
	@echo "libc         -C uClibc clean, all, install"
	
.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile libc help
.PHONY: g
