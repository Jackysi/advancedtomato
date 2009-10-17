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

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

# To rebuild everything and all configurations:
#  make distclean
#  make libc (usually doesn't need to be done ???)
#  make V1=whatever V2=sub-whatever a b c d m
# The 1st "whatever" would be the build number, the sub-whatever would
#	be the update to the version.
#
# Example:
# make V1=8516 V2="jffs.1" a b c d m s

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



SRCBASE := $(shell pwd)
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)


# Normally you'd do something like "make V1=8516 V2=sub-ver a b c d"
# But if you don't give anything for "V1", it'll get a default from tomato_version.
V1 ?= "--def"

-include tomato_profile.mak


# This could be simpler by just using $(TOMATO_PROFILE_NAME) like it used to be,
# but that's fragile.  If you make one certain innocuous change elsewhere in the
# makefile(s), the build will silently be wrong.  This way it works properly every time.
current_BUILD_NAME = $(strip $(shell grep "^TOMATO_BUILD_NAME" tomato_profile.mak  | cut -d"=" -f2))
current_BUILD_DESC = $(strip $(shell grep "^TOMATO_BUILD_DESC" tomato_profile.mak  | cut -d"=" -f2))
current_BUILD_USB  = $(strip $(shell grep "^TOMATO_BUILD_USB"  tomato_profile.mak  | cut -d"=" -f2))

uppercase_N = $(shell echo $(N) | tr a-z  A-Z)
lowercase_N = $(shell echo $(N) | tr A-Z a-z)
uppercase_B = $(shell echo $(B) | tr a-z  A-Z)
lowercase_B = $(shell echo $(B) | tr A-Z a-z)


ifeq ($(TOMATO_BUILD),)

all:
	$(MAKE) libc
	$(MAKE) a

else

all:
	@echo ""
	@echo ""
	@echo "Building $(current_BUILD_USB) $(V1)$(V2) $(current_BUILD_DESC) $(current_BUILD_NAME) with $(TOMATO_PROFILE_NAME) Profile"
	@btools/uversion.pl --gen $(V1) $(V2) "ND" $(current_BUILD_USB) $(current_BUILD_DESC)
	@echo ""
	@echo ""

	@-mkdir image
	@$(MAKE) -C router all
	@$(MAKE) -C router install
	@$(MAKE) -C btools

	@rm -f image/tomato-ND$(current_BUILD_USB)-$(V1)$(V2)-$(current_BUILD_DESC).trx
	@echo "" >>fpkg.log
	@echo "***********************" `date` "************************" >>fpkg.log
	@cat router/shared/tomato_version >>fpkg.log
	@echo "" >>fpkg.log
	@cat router/target.info >>fpkg.log
ifeq ($(wildcard include/bcm20xx.h),)
	@btools/fpkg -i linux/linux/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -i router/mipsel-uclibc/target.image \
		-t image/tomato.trx \
		-l W54G,image/WRT54G_WRT54GL.bin \
		-l W54S,image/WRT54GS.bin \
		-l W54s,image/WRT54GSv4.bin \
		-l W54U,image/WRTSL54GS.bin \
		-m 0x10577050,image/WR850G.bin \
		| tee -a fpkg.log
else
	@btools/fpkg -i linux/linux/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -i router/mipsel-uclibc/target.image \
		-t image/tomato-ND$(current_BUILD_USB)-$(V1)$(V2)-$(current_BUILD_DESC).trx 	| tee -a fpkg.log
endif

	@cp fpkg.log image/fpkg-$(V1)$(V2).log
	@echo ""
	@echo "-----------------"
	@echo `cat router/shared/tomato_version` " ready"
	@echo "-----------------"
ifneq ($(NOVERSION),1)	
	@cp router/shared/tomato_version router/shared/tomato_version_last
	@btools/uversion.pl --bump
endif
endif



clean:
	@touch router/.config
	@rm -f router/config_[a-z]
	@rm -f router/busybox/config_[a-z]
	@$(MAKE) -C router $@
	@-rmdir router/mipsel-uclibc

cleanimage:
	@rm -f fpkg.log
	@rm -fr image/*
	@rm -f router/.config
	@touch router/.config 
	@-mkdir image

cleantools:
	@$(MAKE) -C linux/linux/scripts/squashfs clean
	@$(MAKE) -C btools clean

cleankernel:
	@cd linux/linux && \
	mv .config save-config && \
	$(MAKE) distclean; \
	cp -p save-config .config

distclean: clean cleanimage cleankernel cleantools cleanlibc
ifneq ($(INSIDE_MAK),1)
	@$(MAKE) -C router $@ INSIDE_MAK=1
endif
	mv router/busybox/.config busybox-saved-config
	@$(MAKE) -C router/busybox distclean
	@rm -f router/busybox/config_current || true
	@cp -p busybox-saved-config router/busybox/.config
	@cp -p router/busybox/.config  router/busybox/config_current
	@rm -f router/config_current || true
	@rm -f router/.config.cmd router/.config.old router/.config
	@rm -f router/libfoo_xref.txt
	@rm -f tomato_profile.mak router/shared/tomato_profile.h
	@touch tomato_profile.mak
	@touch router/shared/tomato_profile.h


prepk:
	$(MAKE) -C linux/linux oldconfig dep

what:
	@echo ""
	@echo "$(current_BUILD_DESC)-$(current_BUILD_NAME)-$(TOMATO_PROFILE_NAME) Profile"


# The methodology for making the different builds is to
# copy the "base" config file to the "target" config file in
# the appropriate directory, and then edit it by removing and
# inserting the desired configuration lines.
# You can't just delete the "whatever=y" line, you must have
# a "...is not set" line, or the make oldconfig will stop and ask
# what to do.

## Make the "normal" build
a:
	@cp router/config_base router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@$(MAKE) setprofile N=G B=A DESC="Std" USB="USB"
	@$(MAKE) all


## Make the "normal but without SSH" build
b:
	@cp router/config_base router/config_$@
	@sed -i "/TCONFIG_SSH/d" router/config_$@
	@echo "# TCONFIG_SSH is not set" >>router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@$(MAKE) setprofile N=G B=B DESC="No SSH" USB="USB"
	@$(MAKE) all

## Make the "normal but without cifs" build
c:
	@cp router/config_base router/config_$@
	@sed -i "/TCONFIG_CIFS/d" router/config_$@
	@echo "# TCONFIG_CIFS is not set" >>router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@$(MAKE) setprofile N=G B=C DESC="No CIFS" USB="USB"
	@$(MAKE) all

## Make the "normal but without SAMBA" build
d:
	@cp router/config_base router/config_$@
	@sed -i "/TCONFIG_SAMBASRV/d" router/config_$@
	@echo "# TCONFIG_SAMBASRV is not set" >>router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_MOUNT_LOOP/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_MOUNT_LOOP is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_DEVFS/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_DEVFS is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_ARPING/d" router/busybox/config_$@
	@echo "# CONFIG_ARPING is not set" >>router/busybox/config_$@
	@$(MAKE) setprofile N=G B=D DESC="Lite" USB="USB"
	@$(MAKE) all

## Make the "normal + VPN" build
e:
	@cp router/config_base router/config_$@	
	@sed -i "/TCONFIG_LZO/d" router/config_$@
	@sed -i "/TCONFIG_OPENVPN/d" router/config_$@
	@echo "TCONFIG_LZO=y" >>router/config_$@
	@echo "TCONFIG_OPENVPN=y" >>router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@$(MAKE) setprofile N=G B=E DESC="VPN" USB="USB"
	@$(MAKE) all

## Make the "normal with Extras" build
m:
	@cp router/config_base router/config_$@	
	@sed -i "/TCONFIG_USB_EXTRAS/d" router/config_$@
	@echo "TCONFIG_USB_EXTRAS=y" >>router/config_$@
	@sed -i "/TCONFIG_NTFS/d" router/config_$@
	@echo "TCONFIG_NTFS=y" >>router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@sed -i "/CONFIG_UNZIP/d" router/busybox/config_$@
	@echo "CONFIG_UNZIP=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_AWK/d" router/busybox/config_$@
	@echo "CONFIG_AWK=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_AWK_LIBM/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_AWK_LIBM=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_LS_COLOR/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_LS_COLOR=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_LS_COLOR_IS_DEFAULT/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_LS_COLOR_IS_DEFAULT=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_SORT_BIG/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_SORT_BIG=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_E2FSCK/d" router/busybox/config_$@
	@echo "CONFIG_E2FSCK=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_MKE2FS/d" router/busybox/config_$@
	@echo "CONFIG_MKE2FS=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FDISK/d" router/busybox/config_$@
	@echo "CONFIG_FDISK=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_FDISK_WRITABLE/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_FDISK_WRITABLE=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_MKSWAP/d" router/busybox/config_$@
	@echo "CONFIG_MKSWAP=y" >>router/busybox/config_$@
	@$(MAKE) setprofile N=G B=M DESC="Ext" USB="USB"
	@$(MAKE) all

## Make the "No USB" build
s:
	@cp router/config_base router/config_$@
	@sed -i "/TCONFIG_SAMBASRV/d" router/config_$@
	@echo "# TCONFIG_SAMBASRV is not set" >>router/config_$@
	@sed -i "/TCONFIG_FTP/d" router/config_$@
	@echo "# TCONFIG_FTP is not set" >>router/config_$@
	@sed -i "/TCONFIG_USB/d" router/config_$@
	@echo "# TCONFIG_USB is not set" >>router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_MOUNT_LOOP/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_MOUNT_LOOP is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_DEVFS/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_DEVFS is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_UNZIP/d" router/busybox/config_$@
	@echo "CONFIG_UNZIP=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_AWK/d" router/busybox/config_$@
	@echo "CONFIG_AWK=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_AWK_LIBM/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_AWK_LIBM=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_LS_COLOR/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_LS_COLOR=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_LS_COLOR_IS_DEFAULT/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_LS_COLOR_IS_DEFAULT=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_SORT_BIG/d" router/busybox/config_$@
	@echo "CONFIG_FEATURE_SORT_BIG=y" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_MOUNT_LABEL/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_MOUNT_LABEL is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_MOUNT_FSTAB/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_MOUNT_FSTAB is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_VOLUMEID/d" router/busybox/config_$@
	@echo "# CONFIG_VOLUMEID is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_SWAPONOFF/d" router/busybox/config_$@
	@echo "# CONFIG_SWAPONOFF is not set" >>router/busybox/config_$@
	@$(MAKE) setprofile N=G B=S DESC="" USB=""
	@$(MAKE) all

setprofile:
	@echo "#ifndef TOMATO_PROFILE"                 > router/shared/tomato_profile.h
	@echo "#define TOMATO_$(N) 1"                 >> router/shared/tomato_profile.h
	@echo "#define PROFILE_G 1"                   >> router/shared/tomato_profile.h
	@echo "#define PROFILE_N 2"                   >> router/shared/tomato_profile.h
	@echo "#define TOMATO_PROFILE PROFILE_$(N)"   >> router/shared/tomato_profile.h
	@echo "#define TOMATO_PROFILE_NAME \"$(N)\""  >> router/shared/tomato_profile.h
	@echo "#define TOMATO_BUILD_NAME \"$(B)\""    >> router/shared/tomato_profile.h
	@echo "#define TOMATO_BUILD_DESC \"$(DESC)\"" >> router/shared/tomato_profile.h
	@echo "#endif"                               >> router/shared/tomato_profile.h

	@echo "TOMATO_$(N) = 1"                      > tomato_profile.mak
	@echo "PROFILE_G = 1"                       >> tomato_profile.mak
	@echo "PROFILE_N = 2"                       >> tomato_profile.mak
	@echo "TOMATO_PROFILE = \$$(PROFILE_$(N))"  >> tomato_profile.mak
	@echo "TOMATO_PROFILE_NAME = \"$(N)\""      >> tomato_profile.mak
	@echo "TOMATO_BUILD = \"$(B)\""             >> tomato_profile.mak
	@echo "TOMATO_BUILD_NAME = \"$(B)\""        >> tomato_profile.mak
	@echo "TOMATO_BUILD_DESC = \"$(DESC)\""     >> tomato_profile.mak
	@echo "TOMATO_PROFILE_L = $(lowercase_N)"   >> tomato_profile.mak
	@echo "TOMATO_PROFILE_U = $(uppercase_N)"   >> tomato_profile.mak
	@echo "TOMATO_BUILD_USB = \"$(USB)\""       >> tomato_profile.mak


# Note that changes to variables in tomato_profile.mak don't
# get propogated to this invocation of make!
	@cd router ; \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp config_current .config
	@$(MAKE) -C router oldconfig

	@echo ""
	@echo "Using $(N) profile, $(B) build config."
	@echo ""

	@cd router/busybox && \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp config_current .config
	@$(MAKE) -C router/busybox oldconfig

cleanlibc:
#	@$(MAKE) -C ../../tools-src/uClibc clean

libc: cleanlibc
#	@$(MAKE) -C ../../tools-src/uClibc
#	@$(MAKE) -C ../../tools-src/uClibc install

echotest:
	@( echo -e foo | grep "\-e foo" ) && echo bar || echo foo

help:
	@echo "a            A build (standard)"
	@echo "b            B build (standard minus SSH)"
	@echo "c            C build (standard minus cifs)"
	@echo "d            D build (standard minus Samba server)"
	@echo "m            M build (standard plus extra utilities)"
	@echo "..etc..      other build configs"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C linux/linux distclean (but preserves .config)"
	@echo "distclean    distclean of Linux & busybox (but preserve .configs)"
	@echo "prepk        -C linux/linux oldconfig dep"
	@echo "libc         -C uClibc clean, all, install"
	
.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile libc help
.PHONY: a b c d m Makefile allversions tomato_profile.mak
