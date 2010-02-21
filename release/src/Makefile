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



export SRCBASE := $(shell pwd)
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)

include ./target.mak

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
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -i router/mipsel-uclibc/target.image \
		-t image/tomato.trx \
		-l W54G,image/WRT54G_WRT54GL.bin \
		-l W54S,image/WRT54GS.bin \
		-l W54s,image/WRT54GSv4.bin \
		-l W54U,image/WRTSL54GS.bin \
		-m 0x10577050,image/WR850G.bin \
		| tee -a fpkg.log
else
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -i router/mipsel-uclibc/target.image \
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
	@$(MAKE) -C $(LINUXDIR)/scripts/squashfs clean
	@$(MAKE) -C btools clean

cleankernel:
	@cd $(LINUXDIR) && \
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
	@cd $(LINUXDIR) ; \
		rm -f config_current ; \
		ln -s config_base config_current ; \
		cp -f config_current .config
	$(MAKE) -C $(LINUXDIR) oldconfig dep

what:
	@echo ""
	@echo "$(current_BUILD_DESC)-$(current_BUILD_NAME)-$(TOMATO_PROFILE_NAME) Profile"
	@echo ""


# The methodology for making the different builds is to
# copy the "base" config file to the "target" config file in
# the appropriate directory, and then edit it by removing and
# inserting the desired configuration lines.
# You can't just delete the "whatever=y" line, you must have
# a "...is not set" line, or the make oldconfig will stop and ask
# what to do.

# $(1) - router/config_$@

define RouterOptions
	@( \
	if [ "$(SAMBA3)" = "y" ]; then \
		sed -i "/TCONFIG_SAMBA3/d" $(1); \
		echo "TCONFIG_SAMBA3=y" >>$(1); \
	fi; \
	if [ "$(NO_JFFS)" = "y" ]; then \
		sed -i "/TCONFIG_JFFS2/d" $(1); \
		echo "# TCONFIG_JFFS2 is not set" >>$(1); \
	fi; \
	)
endef

# $(1) - router/busybox/config_$@

define BusyboxExtraOptions
	@sed -i "/CONFIG_UNZIP/d" $(1)
	@echo "CONFIG_UNZIP=y" >>$(1)
	@sed -i "/CONFIG_FEATURE_LS_COLOR/d" $(1)
	@echo "CONFIG_FEATURE_LS_COLOR=y" >>$(1)
	@sed -i "/CONFIG_FEATURE_LS_COLOR_IS_DEFAULT/d" $(1)
	@echo "CONFIG_FEATURE_LS_COLOR_IS_DEFAULT=y" >>$(1)
	@sed -i "/CONFIG_FEATURE_SORT_BIG/d" $(1)
	@echo "CONFIG_FEATURE_SORT_BIG=y" >>$(1)
	@( \
	if [ "$(CONFIG_LINUX26)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_WGET_STATUSBAR/d" $(1); \
		echo "CONFIG_FEATURE_WGET_STATUSBAR=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_VERBOSE_USAGE/d" $(1); \
		echo "CONFIG_FEATURE_VERBOSE_USAGE=y" >>$(1); \
	fi; \
	)
endef

define BusyboxExtraUSBOptions
	@sed -i "/CONFIG_E2FSCK/d" $(1)
	@echo "CONFIG_E2FSCK=y" >>$(1)
	@sed -i "/CONFIG_MKE2FS/d" $(1)
	@echo "CONFIG_MKE2FS=y" >>$(1)
	@sed -i "/CONFIG_FDISK/d" $(1)
	@echo "CONFIG_FDISK=y" >>$(1)
	@sed -i "/CONFIG_FEATURE_FDISK_WRITABLE/d" $(1)
	@echo "CONFIG_FEATURE_FDISK_WRITABLE=y" >>$(1)
	@sed -i "/CONFIG_MKFS_VFAT/d" $(1)
	@echo "CONFIG_MKFS_VFAT=y" >>$(1)
	@sed -i "/CONFIG_MKSWAP/d" $(1)
	@echo "CONFIG_MKSWAP=y" >>$(1)
	@( \
	if [ "$(CONFIG_LINUX26)" = "y" ]; then \
		sed -i "/CONFIG_LSUSB/d" $(1); \
		echo "CONFIG_LSUSB=y" >>$(1); \
	fi; \
	)
endef

define BusyboxNoUSBOptions
	@sed -i "/CONFIG_FEATURE_MOUNT_LOOP/d" $(1)
	@echo "# CONFIG_FEATURE_MOUNT_LOOP is not set" >>$(1)
	@sed -i "/CONFIG_FEATURE_DEVFS/d" $(1)
	@echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1)
	@sed -i "/CONFIG_FEATURE_MOUNT_LABEL/d" $(1)
	@echo "# CONFIG_FEATURE_MOUNT_LABEL is not set" >>$(1)
	@sed -i "/CONFIG_FEATURE_MOUNT_FSTAB/d" $(1)
	@echo "# CONFIG_FEATURE_MOUNT_FSTAB is not set" >>$(1)
	@sed -i "/CONFIG_VOLUMEID/d" $(1)
	@echo "# CONFIG_VOLUMEID is not set" >>$(1)
	@sed -i "/CONFIG_BLKID/d" $(1)
	@echo "# CONFIG_BLKID is not set" >>$(1)
	@sed -i "/CONFIG_SWAPONOFF/d" $(1)
	@echo "# CONFIG_SWAPONOFF is not set" >>$(1)
endef

ifeq ($(CONFIG_LINUX26),y)
define BusyboxKernelConfig
	@sed -i "/CONFIG_FEATURE_2_4_MODULES/d" $(1)
	@echo "# CONFIG_FEATURE_2_4_MODULES is not set" >>$(1)
	@sed -i "/CONFIG_FEATURE_LSMOD_PRETTY_2_6_OUTPUT/d" $(1)
	@echo "CONFIG_FEATURE_LSMOD_PRETTY_2_6_OUTPUT=y" >>$(1)
	@sed -i "/CONFIG_FEATURE_DEVFS/d" $(1)
	@echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1)
	@sed -i "/CONFIG_MKNOD/d" $(1)
	@echo "CONFIG_MKNOD=y" >>$(1)
endef
else
define BusyboxKernelConfig
endef
endif

# $(1) - linux/linux[-2.6]/config_$@

ifeq ($(CONFIG_LINUX26),y)
define KernelConfigMips
	@( \
	if [ "$(MIPS32)" = "r2" ]; then \
		sed -i "/CONFIG_CPU_MIPS32_R1/d" $(1); \
		echo "# CONFIG_CPU_MIPS32_R1 is not set" >>$(1); \
		sed -i "/CONFIG_CPU_MIPS32_R2/d" $(1); \
		echo "CONFIG_CPU_MIPS32_R2=y" >>$(1); \
		sed -i "/CONFIG_CPU_MIPSR1/d" $(1); \
		echo "CONFIG_CPU_MIPSR2=y" >>$(1); \
		sed -i "/CONFIG_HIGHMEM=/d" $(1); \
		echo "CONFIG_HIGHMEM=y" >>$(1); \
		sed -i "/CONFIG_FLATMEM_MANUAL/d" $(1); \
		echo "# CONFIG_FLATMEM_MANUAL is not set" >>$(1); \
		sed -i "/CONFIG_SPARSEMEM_MANUAL/d" $(1); \
		echo "CONFIG_SPARSEMEM_MANUAL=y" >>$(1); \
		sed -i "/CONFIG_SPARSEMEM=/d" $(1); \
		echo "CONFIG_SPARSEMEM=y" >>$(1); \
		sed -i "/CONFIG_SPARSEMEM_EXTREME/d" $(1); \
		echo "CONFIG_SPARSEMEM_EXTREME=y" >>$(1); \
		sed -i "/CONFIG_FLATMEM=/d" $(1); \
		echo "# CONFIG_FLATMEM is not set" >>$(1); \
		sed -i "/CONFIG_FLAT_NODE_MEM_MAP/d" $(1); \
		echo "# CONFIG_FLAT_NODE_MEM_MAP is not set" >>$(1); \
		sed -i "/CONFIG_HAVE_MEMORY_PRESENT/d" $(1); \
		echo "CONFIG_HAVE_MEMORY_PRESENT=y" >>$(1); \
	else \
		sed -i "/CONFIG_HIGHMEM=/d" $(1); \
		echo "# CONFIG_HIGHMEM is not set" >>$(1); \
		sed -i "/CONFIG_FLATMEM_MANUAL/d" $(1); \
		echo "CONFIG_FLATMEM_MANUAL=y" >>$(1); \
		sed -i "/CONFIG_SPARSEMEM=/d" $(1); \
		echo "# CONFIG_SPARSEMEM is not set" >>$(1); \
		sed -i "/CONFIG_SPARSEMEM_MANUAL/d" $(1); \
		echo "# CONFIG_SPARSEMEM_MANUAL is not set" >>$(1); \
		sed -i "/CONFIG_SPARSEMEM_EXTREME/d" $(1); \
		echo "# CONFIG_SPARSEMEM_EXTREME is not set" >>$(1); \
		sed -i "/CONFIG_FLATMEM=/d" $(1); \
		echo "CONFIG_FLATMEM=y" >>$(1); \
		sed -i "/CONFIG_FLAT_NODE_MEM_MAP/d" $(1); \
		echo "CONFIG_FLAT_NODE_MEM_MAP=y" >>$(1); \
		sed -i "/CONFIG_HAVE_MEMORY_PRESENT/d" $(1); \
		echo "# CONFIG_HAVE_MEMORY_PRESENT is not set" >>$(1); \
	fi; \
	if [ "$(BCM57)" = "y" ]; then \
		sed -i "/CONFIG_BCM57XX/d" $(1); \
		echo "CONFIG_BCM57XX=m" >>$(1); \
	fi; \
	if [ "$(JFFSv1)" = "y" ]; then \
		sed -i "/CONFIG_JFFS_FS/d" $(1); \
		echo "CONFIG_JFFS_FS=m" >>$(1); \
		sed -i "/CONFIG_JFFS_FS_VERBOSE/d" $(1); \
		echo "CONFIG_JFFS_FS_VERBOSE=0" >>$(1); \
		sed -i "/CONFIG_JFFS_PROC_FS/d" $(1); \
		echo "# CONFIG_JFFS_PROC_FS is not set" >>$(1); \
		sed -i "/CONFIG_JFFS2_FS/d" $(1); \
		echo "# CONFIG_JFFS2_FS is not set" >>$(1); \
	fi; \
	)
endef
else
define KernelConfigMips
#	nothing for kernel 2.4
endef
endif

## Make the "normal" build
a:
	@cp router/config_base router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@$(MAKE) setprofile N=$(TPROFILE) B=A DESC="Std" USB="USB"
	@$(MAKE) all

## Make the "normal but without SSH" build
b:
	@cp router/config_base router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_SSH/d" router/config_$@
	@echo "# TCONFIG_SSH is not set" >>router/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@$(MAKE) setprofile N=$(TPROFILE) B=B DESC="No SSH" USB="USB"
	@$(MAKE) all

## Make the "normal but without cifs" build
c:
	@cp router/config_base router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_CIFS/d" router/config_$@
	@echo "# TCONFIG_CIFS is not set" >>router/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@sed -i "/CONFIG_FEATURE_MOUNT_CIFS/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_MOUNT_CIFS is not set" >>router/busybox/config_$@

	@$(MAKE) setprofile N=$(TPROFILE) B=C DESC="No CIFS" USB="USB"
	@$(MAKE) all

## Make the "normal but without SAMBA" build
d:
	@cp router/config_base router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_SAMBASRV/d" router/config_$@
	@echo "# TCONFIG_SAMBASRV is not set" >>router/config_$@
ifeq ($(CONFIG_LINUX26),y)
	@sed -i "/TCONFIG_CIFS/d" router/config_$@
	@echo "# TCONFIG_CIFS is not set" >>router/config_$@
	@sed -i "/TCONFIG_JFFS2/d" router/config_$@
	@echo "# TCONFIG_JFFS2 is not set" >>router/config_$@
	@sed -i "/TCONFIG_ZEBRA/d" router/config_$@
	@echo "# TCONFIG_ZEBRA is not set" >>router/config_$@
endif

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@sed -i "/CONFIG_FEATURE_MOUNT_LOOP/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_MOUNT_LOOP is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_AWK/d" router/busybox/config_$@
	@echo "# CONFIG_AWK is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_FEATURE_DEVFS/d" router/busybox/config_$@
	@echo "# CONFIG_FEATURE_DEVFS is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_BLKID/d" router/busybox/config_$@
	@echo "# CONFIG_BLKID is not set" >>router/busybox/config_$@
	@sed -i "/CONFIG_ARPING/d" router/busybox/config_$@
	@echo "# CONFIG_ARPING is not set" >>router/busybox/config_$@

	@$(MAKE) setprofile N=$(TPROFILE) B=D DESC="Lite" USB="USB"
	@$(MAKE) all

## Make the "normal with Extras + VPN" build
e:
	@cp router/config_base router/config_$@	
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_LZO/d" router/config_$@
	@sed -i "/TCONFIG_OPENVPN/d" router/config_$@
	@echo "TCONFIG_LZO=y" >>router/config_$@
	@echo "TCONFIG_OPENVPN=y" >>router/config_$@
	@sed -i "/TCONFIG_USB_EXTRAS/d" router/config_$@
	@echo "TCONFIG_USB_EXTRAS=y" >>router/config_$@
	@sed -i "/TCONFIG_NTFS/d" router/config_$@
	@echo "TCONFIG_NTFS=y" >>router/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxExtraOptions, router/busybox/config_$@)
	$(call BusyboxExtraUSBOptions, router/busybox/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@$(MAKE) setprofile N=$(TPROFILE) B=E DESC="VPN" USB="USB"
	@$(MAKE) all

## Make the "normal with Extras" build
m:
	@cp router/config_base router/config_$@	
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_USB_EXTRAS/d" router/config_$@
	@echo "TCONFIG_USB_EXTRAS=y" >>router/config_$@
	@sed -i "/TCONFIG_NTFS/d" router/config_$@
	@echo "TCONFIG_NTFS=y" >>router/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxExtraOptions, router/busybox/config_$@)
	$(call BusyboxExtraUSBOptions, router/busybox/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@$(MAKE) setprofile N=$(TPROFILE) B=M DESC="Ext" USB="USB"
	@$(MAKE) all

## Make the "No USB" build
s:
	@cp router/config_base router/config_$@
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_SAMBASRV/d" router/config_$@
	@echo "# TCONFIG_SAMBASRV is not set" >>router/config_$@
	@sed -i "/TCONFIG_FTP/d" router/config_$@
	@echo "# TCONFIG_FTP is not set" >>router/config_$@
	@sed -i "/TCONFIG_USB/d" router/config_$@
	@echo "# TCONFIG_USB is not set" >>router/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxExtraOptions, router/busybox/config_$@)
	$(call BusyboxNoUSBOptions, router/busybox/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@$(MAKE) setprofile N=$(TPROFILE) B=S DESC="" USB=""
	@$(MAKE) all

## Make the "VPN witn no USB support" build
v:
	@cp router/config_base router/config_$@	
	@cp router/busybox/config_base router/busybox/config_$@
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$@

	@sed -i "/TCONFIG_SAMBASRV/d" router/config_$@
	@echo "# TCONFIG_SAMBASRV is not set" >>router/config_$@
	@sed -i "/TCONFIG_FTP/d" router/config_$@
	@echo "# TCONFIG_FTP is not set" >>router/config_$@
	@sed -i "/TCONFIG_USB/d" router/config_$@
	@echo "# TCONFIG_USB is not set" >>router/config_$@
	@sed -i "/TCONFIG_LZO/d" router/config_$@
	@sed -i "/TCONFIG_OPENVPN/d" router/config_$@
	@echo "TCONFIG_LZO=y" >>router/config_$@
	@echo "TCONFIG_OPENVPN=y" >>router/config_$@

	$(call RouterOptions, router/config_$@)
	$(call KernelConfigMips, $(LINUXDIR)/config_$@)
	$(call BusyboxExtraOptions, router/busybox/config_$@)
	$(call BusyboxNoUSBOptions, router/busybox/config_$@)
	$(call BusyboxKernelConfig, router/busybox/config_$@)

	@$(MAKE) setprofile N=$(TPROFILE) B=V DESC="VPN" USB=""
	@$(MAKE) all

## Make Mips32r2 Extras build (Asus RT-N16, Netgear WNR3500L)
r2m:
	@$(MAKE) m MIPS32=r2 BCM57=y SAMBA3=y

## Make Mips32r2 USB+VPN build
r2e:
	@$(MAKE) e MIPS32=r2 BCM57=y SAMBA3=y

## Make Mips32r2 "no USB" build (Asus RT-N12)
r2s:
	@$(MAKE) s MIPS32=r2 NO_JFFS=y

## Make Mips32r2 "no USB" VPN build
r2v:
	@$(MAKE) v MIPS32=r2 NO_JFFS=y

setprofile:
	echo '#ifndef TOMATO_PROFILE' > router/shared/tomato_profile.h
	echo '#define TOMATO_$(N) 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_G 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_N 2' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE PROFILE_$(N)' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE_NAME "$(N)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_NAME "$(B)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_DESC "$(DESC)"' >> router/shared/tomato_profile.h
	echo '#endif' >> router/shared/tomato_profile.h

	echo 'TOMATO_$(N) = 1' > tomato_profile.mak
	echo 'PROFILE_G = 1' >> tomato_profile.mak
	echo 'PROFILE_N = 2' >> tomato_profile.mak
	echo 'TOMATO_PROFILE = $$(PROFILE_$(N))' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_NAME = "$(N)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD = "$(B)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD_NAME = "$(B)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD_DESC = "$(DESC)"' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_L = $(lowercase_N)' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_U = $(uppercase_N)' >> tomato_profile.mak
	echo 'TOMATO_BUILD_USB = "$(USB)"' >> tomato_profile.mak


# Note that changes to variables in tomato_profile.mak don't
# get propogated to this invocation of make!
	@echo ""
	@echo "Using $(N) profile, $(B) build config."
	@echo ""

	@cd $(LINUXDIR) ; \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp -f config_current .config

	@cd router/busybox && \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp config_current .config

	@cd router ; \
		rm -f config_current ; \
		ln -s config_$(lowercase_B) config_current ; \
		cp config_current .config

	@$(MAKE) -C router oldconfig

cleanlibc:
#	@$(MAKE) -C ../../tools-src/uClibc clean

libc: cleanlibc
#	@$(MAKE) -C ../../tools-src/uClibc
#	@$(MAKE) -C ../../tools-src/uClibc install

help:
	@echo "a            A build (standard)"
	@echo "b            B build (standard minus SSH)"
	@echo "c            C build (standard minus cifs)"
	@echo "d            D build (standard minus Samba server)"
	@echo "m            M build (standard plus extra utilities)"
	@echo "e            E build (standard plus VPN and extra utilities)"
	@echo "s            S build (no USB support)"
	@echo "v            V build (VPN with no USB support)"
	@echo "..etc..      other build configs"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C Linux distclean (but preserves .config)"
	@echo "distclean    distclean of Linux & busybox (but preserve .configs)"
	@echo "prepk        -C Linux oldconfig dep"
	@echo "libc         -C uClibc clean, all, install"
	
.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile libc help
.PHONY: a b c d m Makefile allversions tomato_profile.mak
