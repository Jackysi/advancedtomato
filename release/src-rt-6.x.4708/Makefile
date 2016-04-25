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
#  make V1=whatever V2=sub-whatever VPN=vpn3.6 a b c d m n o
# The 1st "whatever" would be the build number, the sub-whatever would
#	be the update to the version.
#
# Example:
# make V1=8516 V2="-jffs.1" a b c d m s n o

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


export ac_cv_func_malloc_0_nonnull=yes
export SRCBASE := $(shell pwd)
export SRCBASEDIR := $(shell pwd | sed 's/.*release\///g')
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)
export TPROFILE := N

include ./target.mak

export LD_LIBRARY_PATH := /opt/hndtools-arm-linux-2.6.36-uclibc-4.5.3/lib

CTAGS_EXCLUDE_OPT := --exclude=kernel_header --exclude=$(PLATFORM)
CTAGS_DEFAULT_DIRS := $(SRCBASE)/router/rc $(SRCBASE)/router/httpd $(SRCBASE)/src/router/shared $(SRCBASE)/src/router/www

# Normally you'd do something like "make V1=8516 V2=-sub-ver a b c d"
# But if you don't give anything for "V1", it'll get a default from tomato_version.
V1 ?= "--def"
VPN ?= "VPN"
ND = "K26ARM"

PPTPD ?= "n"

ifeq ($(NVRAM_SIZE),)
NVRAM_SIZE = 0
endif

ifeq ($(ASUS_TRX),)
ASUS_TRX = 0
endif

ifeq ($(NETGEAR_CHK),)
NETGEAR_CHK = 0
else
WNRTOOL=$(SRCBASE)/wnrtool
BOARD_FILE=$(WNRTOOL)/$(BOARDID_FILE)
FW_FILE=$(WNRTOOL)/$(FW_CFG_FILE)
endif

ifeq ($(XIAOMI_TRX),)
XIAOMI_TRX = 0
endif

ifeq ($(NVRAM_64K),y)
EXTRA_64KDESC = ' -64K'
EXTRA_64KCFLAG = '-DTCONFIG_NVRAM_64K'
else
EXTRA_64KDESC = ''
EXTRA_64KCFLAG = ''
endif

ifeq ($(ARM),y)
KERNEL_BINARY=$(LINUXDIR)/arch/arm/boot/zImage
else
KERNEL_BINARY=$(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/zImage
endif

-include tomato_profile.mak

# This could be simpler by just using $(TOMATO_PROFILE_NAME) like it used to be,
# but that's fragile.  If you make one certain innocuous change elsewhere in the
# makefile(s), the build will silently be wrong.  This way it works properly every time.
current_BUILD_NAME = $(strip $(shell grep "^TOMATO_BUILD_NAME" tomato_profile.mak  | cut -d"=" -f2))
current_BUILD_DESC = $(strip $(shell grep "^TOMATO_BUILD_DESC" tomato_profile.mak  | cut -d"=" -f2 | sed -e "s/ //g"))
current_BUILD_USB  = $(strip $(shell grep "^TOMATO_BUILD_USB"  tomato_profile.mak  | cut -d"=" -f2 | sed -e "s/ //g"))
current_TOMATO_VER = $(strip $(shell grep "TOMATO_MAJOR" router/shared/tomato_version.h  | cut -d"\"" -f2)).$(strip $(shell grep "TOMATO_MINOR" router/shared/tomato_version.h  | cut -d"\"" -f2))

uppercase_N = $(shell echo $(N) | tr a-z  A-Z)
lowercase_N = $(shell echo $(N) | tr A-Z a-z)
uppercase_B = $(shell echo $(B) | tr a-z  A-Z)
lowercase_B = $(shell echo $(B) | tr A-Z a-z)

ifeq ($(CONFIG_LINUX26),y)
mips_rev =
KERN_SIZE_OPT ?= n
else
mips_rev =
KERN_SIZE_OPT ?= y
endif

beta = $(if $(filter $(TOMATO_EXPERIMENTAL),1),-beta,)

tomato_ver:
	@echo ""
	@btools/uversion.pl --gen $(V1) $(mips_rev)$(beta)$(V2) $(ND) $(current_BUILD_USB) $(current_BUILD_DESC)

ifeq ($(TOMATO_BUILD),)

all:
	$(MAKE) a

else

all: tomato_ver
	@echo ""
	@echo "Building Tomato $(ND) $(current_BUILD_USB) $(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2) $(current_BUILD_DESC) $(current_BUILD_NAME) with $(TOMATO_PROFILE_NAME) Profile"

	@echo ""
	@echo ""

	@-mkdir image
	@$(MAKE) -C router all
	@$(MAKE) -C router install
ifeq ($(ARM),)
	@$(MAKE) -C router image
endif
	@$(MAKE) -C btools
	@$(MAKE) -C ctools

	@rm -f image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx
	@rm -f image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin

ifneq ($(ASUS_TRX),0)
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
# for asus RT-N18U, RT-AC56U, RT-AC68U, RT-AC68R, RT-AC68P
ifeq ($(ASUS_TRX),ASUS)
ifeq ($(CONFIG_BCM7),y)
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC3200,3.0.0.4,image/tomato-RT-AC3200-$(V1)$(V2)-$(current_BUILD_DESC).trx
else
ifeq ($(BCMSMP),y)
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC56U,3.0.0.4,image/tomato-RT-AC56U-$(V1)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-AC68U,3.0.0.4,image/tomato-RT-AC68U-$(V1)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r RT-N18U,3.0.0.4,image/tomato-RT-N18U-$(V1)$(V2)-$(current_BUILD_DESC).trx
else
	ctools/trx_asus -i image/linux-lzma.trx -r RT-N18U,3.0.0.4,image/tomato-RT-N18U-$(V1)$(V2)-$(current_BUILD_DESC)-NOSMP.trx
endif
endif
endif
# for dlink
ifeq ($(ASUS_TRX),DLINK)
	ctools/trx_asus -i image/linux-lzma.trx -r DIR868L,3.0.0.4,image/tomato-DIR868L-$(V1)$(V2)-$(current_BUILD_DESC).trx
endif
# for R7000, R6300v2, R6250
ifeq ($(ASUS_TRX),NETGEAR)
ifeq ($(CONFIG_BCM7),y)
	ctools/trx_asus -i image/linux-lzma.trx -r R8000,3.0.0.4,image/tomato-R8000-$(V1)$(V2)-$(current_BUILD_DESC).trx
else
	ctools/trx_asus -i image/linux-lzma.trx -r R7000,3.0.0.4,image/tomato-R7000-$(V1)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6300v2,3.0.0.4,image/tomato-R6300v2-$(V1)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r R6250,3.0.0.4,image/tomato-R6250-$(V1)$(V2)-$(current_BUILD_DESC).trx
endif
endif
# for WS880
ifeq ($(ASUS_TRX),HUAWEI)
	ctools/trx_asus -i image/linux-lzma.trx -r WS880,3.0.0.4,image/tomato-WS880-$(V1)$(V2)-$(current_BUILD_DESC).trx
endif
# for Buffalo
ifeq ($(ASUS_TRX),BUFFALO)
	ctools/trx_asus -i image/linux-lzma.trx -r WZR1750,3.0.0.4,image/tomato-WZR1750-$(V1)$(V2)-$(current_BUILD_DESC).trx
endif

#for linksys EA series -- EA6700 EA6500v2 EA6900
ifeq ($(ASUS_TRX),LINKSYS)
	ctools/trx_asus -i image/linux-lzma.trx -r EA6900,3.0.0.4,image/tomato-EA6900-$(V1)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r EA6700,3.0.0.4,image/tomato-EA6700-$(V1)$(V2)-$(current_BUILD_DESC).trx
	ctools/trx_asus -i image/linux-lzma.trx -r EA6500v2,3.0.0.4,image/tomato-EA6500v2-$(V1)$(V2)-$(current_BUILD_DESC).trx
endif
	@rm -f image/linux-lzma.trx
	@echo ""
endif

ifneq ($(NETGEAR_CHK),0)
	@echo "Creating Firmware for Netgear ARM devices .... "
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	cd image && touch rootfs
	cd image && $(WNRTOOL)/packet -k linux-lzma.trx -f rootfs -b $(BOARD_FILE) -ok kernel_image \
		-oall kernel_rootfs_image -or rootfs_image -i $(FW_FILE) && rm -f rootfs && \
		cp kernel_rootfs_image.chk tomato-$(NETGEAR_CHK)-$(current_TOMATO_VER).$(V1)$(V2)-$(current_BUILD_DESC).chk
	@echo "Cleanup ...."
	@rm -rf image/linux-lzma.trx image/*image.chk
endif

ifneq ($(XIAOMI_TRX),0)
	@echo "Creating Firmware for Xiaomi R1D .... "
	ctools/objcopy -O binary -R .note -R .note.gnu.build-id -R .comment -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/arm-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	cd image && cp linux-lzma.trx tomato-$(XIAOMI_TRX)-$(V1)$(V2)-$(current_BUILD_DESC).trx
	@echo "Cleanup ...."
	@rm -rf image/linux-lzma.trx
endif

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
	@-rmdir router/arm-uclibc

cleanimage:
	@rm -f fpkg.log
	@rm -fr image/*
	@rm -f router/.config
	@touch router/.config 
	@-mkdir image

cleantools:
#	@$(MAKE) -C $(LINUXDIR)/scripts/squashfs clean
	@$(MAKE) -C btools clean
	@$(MAKE) -C ctools clean

cleankernel:
	@cd $(LINUXDIR) && \
	mv .config save-config && \
	$(MAKE) distclean || true; \
	cp -p save-config .config || true

kernel:
	@$(MAKE) -C router kernel
	@[ ! -e $(KERNEL_BINARY) ] || ls -l $(KERNEL_BINARY)

distclean: clean cleanimage cleankernel cleantools
ifneq ($(INSIDE_MAK),1)
	@$(MAKE) -C router $@ INSIDE_MAK=1
endif
	mv router/busybox/.config busybox-saved-config || true
	@$(MAKE) -C router/busybox distclean
	@rm -f router/busybox/config_current
	@cp -p busybox-saved-config router/busybox/.config || true
	@cp -p router/busybox/.config  router/busybox/config_current || true
	@rm -f router/config_current
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
	$(MAKE) -C $(LINUXDIR) oldconfig
ifneq ($(CONFIG_LINUX26),y)
	$(MAKE) -C $(LINUXDIR) dep
endif

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

# Options for "make bin" :
# BUILD_DESC (Std|Lite|Ext|...)
# KERN_SIZE_OPT
# USB ("USB"|"")
# JFFSv1 | NO_JFFS
# NO_CIFS, NO_SSH, NO_ZEBRA, NO_SAMBA, NO_HTTPS, NO_XXTP, NO_LIBOPT
# SAMBA3, OPENVPN, IPV6SUPP, EBTABLES, NTFS, UFSD, MEDIASRV, BBEXTRAS, USBEXTRAS, BCM57, SLIM, NOCAT, NGINX, CTF
# NFS BTCLIENT BTGUI TR_EXTRAS SNMP SDHC HFS UPS DNSCRYPT PPTPD TOR IPSEC RAID MICROSD USBAP NO_USBAPP NANO

define RouterOptions
	@( \
	if [ "$(CONFIG_LINUX26)" = "y" ] || [ "$(SAMBA3)" = "y" ]; then \
		sed -i "/TCONFIG_SAMBA3/d" $(1); \
		echo "TCONFIG_SAMBA3=y" >>$(1); \
	fi; \
	sed -i "/TCONFIG_EMF/d" $(1); \
	if [ "$(CONFIG_LINUX26)" = "y" ]; then \
		if [ "$(SLIM)" = "y" ]; then \
			echo "# TCONFIG_EMF is not set" >>$(1); \
		else \
			echo "TCONFIG_EMF=y" >>$(1); \
		fi; \
	else \
		echo "# TCONFIG_EMF is not set" >>$(1); \
	fi; \
	sed -i "/TCONFIG_JFFSV1/d" $(1); \
	if [ "$(CONFIG_LINUX26)" = "y" ]; then \
		if [ "$(JFFSv1)" = "y" ]; then \
			echo "TCONFIG_JFFSV1=y" >>$(1); \
		else \
			echo "# TCONFIG_JFFSV1 is not set" >>$(1); \
		fi; \
	else \
		echo "TCONFIG_JFFSV1=y" >>$(1); \
	fi; \
	if [ "$(CONFIG_LINUX26)" = "y" ] && [ "$(IPSEC)" = "y" ]; then \
		echo "TCONFIG_IPSEC=y" >>$(1); \
	else \
		echo "# TCONFIG_IPSEC is not set" >>$(1); \
	fi; \
	if [ "$(CONFIG_LINUX26)" = "y" ] && [ "$(RAID)" = "y" ]; then \
		echo "TCONFIG_RAID=y" >>$(1); \
	else \
		echo "# TCONFIG_RAID is not set" >>$(1); \
	fi; \
	if [ "$(USB)" = "USB" ]; then \
		sed -i "/TCONFIG_USB is not set/d" $(1); \
		echo "TCONFIG_USB=y" >>$(1); \
		if [ "$(USBEXTRAS)" = "y" ]; then \
			sed -i "/TCONFIG_USB_EXTRAS/d" $(1); \
			echo "TCONFIG_USB_EXTRAS=y" >>$(1); \
		fi; \
		if [ "$(NTFS)" = "y" ]; then \
			sed -i "/TCONFIG_NTFS/d" $(1); \
			echo "TCONFIG_NTFS=y" >>$(1); \
		fi; \
		if [ "$(UFSD)" = "ASUS" ]; then \
			sed -i "/TCONFIG_UFSDA/d" $(1); \
			echo "TCONFIG_UFSDA=y" >>$(1); \
		fi; \
		if [ "$(UFSD)" = "NETGEAR" ]; then \
			sed -i "/TCONFIG_UFSDN/d" $(1); \
			echo "TCONFIG_UFSDN=y" >>$(1); \
		fi; \
		if [ "$(TUXERA)" = "y" ]; then \
			sed -i "/TCONFIG_TUXERA/d" $(1); \
			echo "TCONFIG_TUXERA=y" >>$(1); \
		fi; \
		if [ "$(MEDIASRV)" = "y" ]; then \
			sed -i "/TCONFIG_MEDIA_SERVER/d" $(1); \
			echo "TCONFIG_MEDIA_SERVER=y" >>$(1); \
		fi; \
	else \
		sed -i "/TCONFIG_USB=y/d" $(1); \
		echo "# TCONFIG_USB is not set" >>$(1); \
	fi; \
	if [ "$(NO_SAMBA)" = "y" ]; then \
		sed -i "/TCONFIG_SAMBASRV/d" $(1); \
		echo "# TCONFIG_SAMBASRV is not set" >>$(1); \
	fi; \
	if [ "$(NO_ZEBRA)" = "y" ]; then \
		sed -i "/TCONFIG_ZEBRA/d" $(1); \
		echo "# TCONFIG_ZEBRA is not set" >>$(1); \
	fi; \
	if [ "$(NO_JFFS)" = "y" ]; then \
		sed -i "/TCONFIG_JFFS2/d" $(1); \
		echo "# TCONFIG_JFFS2 is not set" >>$(1); \
		sed -i "/TCONFIG_JFFSV1/d" $(1); \
		echo "# TCONFIG_JFFSV1 is not set" >>$(1); \
	fi; \
	if [ "$(NO_CIFS)" = "y" ]; then \
		sed -i "/TCONFIG_CIFS/d" $(1); \
		echo "# TCONFIG_CIFS is not set" >>$(1); \
	fi; \
	if [ "$(NO_SSH)" = "y" ]; then \
		sed -i "/TCONFIG_SSH/d" $(1); \
		echo "# TCONFIG_SSH is not set" >>$(1); \
	fi; \
	if [ "$(NO_HTTPS)" = "y" ]; then \
		sed -i "/TCONFIG_HTTPS/d" $(1); \
		echo "# TCONFIG_HTTPS is not set" >>$(1); \
	fi; \
	if [ "$(NO_XXTP)" = "y" ]; then \
		sed -i "/TCONFIG_L2TP/d" $(1); \
		echo "# TCONFIG_L2TP is not set" >>$(1); \
		sed -i "/TCONFIG_PPTP/d" $(1); \
		echo "# TCONFIG_PPTP is not set" >>$(1); \
	fi; \
	if [ "$(NO_LIBOPT)" = "y" ]; then \
		sed -i "/TCONFIG_OPTIMIZE_SHARED_LIBS/d" $(1); \
		echo "# TCONFIG_OPTIMIZE_SHARED_LIBS is not set" >>$(1); \
	fi; \
	if [ "$(EBTABLES)" = "y" ]; then \
		sed -i "/TCONFIG_EBTABLES/d" $(1); \
		echo "TCONFIG_EBTABLES=y" >>$(1); \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/TCONFIG_IPV6/d" $(1); \
		echo "TCONFIG_IPV6=y" >>$(1); \
	fi; \
	if [ "$(NOCAT)" = "y" ]; then \
		sed -i "/TCONFIG_NOCAT/d" $(1); \
		echo "TCONFIG_NOCAT=y" >>$(1); \
	fi; \
	if [ "$(NGINX)" = "y" ]; then \
		sed -i "/TCONFIG_NGINX/d" $(1); \
		echo "TCONFIG_NGINX=y" >>$(1); \
	fi; \
	if [ "$(OPENVPN)" = "y" ]; then \
		sed -i "/TCONFIG_LZO/d" $(1); \
		echo "TCONFIG_LZO=y" >>$(1); \
		sed -i "/TCONFIG_OPENVPN/d" $(1); \
		echo "TCONFIG_OPENVPN=y" >>$(1); \
		if [ "$(CONFIG_LINUX26)" = "y" ]; then \
			sed -i "/TCONFIG_FTP_SSL/d" $(1); \
			echo "TCONFIG_FTP_SSL=y" >>$(1); \
		fi; \
	fi; \
	if [ "$(PPTPD)" = "y" ]; then \
		sed -i "/TCONFIG_PPTPD/d" $(1); \
		echo "TCONFIG_PPTPD=y" >>$(1); \
	fi; \
	if [ "$(BTCLIENT)" = "y" ]; then \
		sed -i "/TCONFIG_BT/d" $(1); \
		echo "TCONFIG_BT=y" >>$(1); \
		sed -i "/TCONFIG_BBT/d" $(1); \
		echo "TCONFIG_BBT=y" >>$(1); \
	fi; \
	if [ "$(BTGUI)" = "y" ]; then \
		sed -i "/TCONFIG_BT/d" $(1); \
		echo "TCONFIG_BT=y" >>$(1); \
	fi; \
	if [ "$(TR_EXTRAS)" = "y" ]; then \
		sed -i "/TCONFIG_TR_EXTRAS/d" $(1); \
		echo "TCONFIG_TR_EXTRAS=y" >>$(1); \
	fi; \
	if [ "$(NFS)" = "y" ]; then \
		sed -i "/TCONFIG_NFS/d" $(1); \
		echo "TCONFIG_NFS=y" >>$(1); \
	fi; \
	if [ "$(SNMP)" = "y" ]; then \
		sed -i "/TCONFIG_SNMP/d" $(1); \
		echo "TCONFIG_SNMP=y" >>$(1); \
	fi; \
	if [ "$(SDHC)" = "y" ]; then \
		sed -i "/TCONFIG_SDHC/d" $(1); \
		echo "TCONFIG_SDHC=y" >>$(1); \
	fi; \
	if [ "$(DNSSEC)" = "y" ]; then \
		sed -i "/TCONFIG_DNSSEC/d" $(1); \
		echo "TCONFIG_DNSSEC=y" >>$(1); \
	fi; \
	if [ "$(HFS)" = "y" ]; then \
		sed -i "/TCONFIG_HFS/d" $(1); \
		echo "TCONFIG_HFS=y" >>$(1); \
	fi; \
	if [ "$(UPS)" = "y" ]; then \
		sed -i "/TCONFIG_UPS/d" $(1); \
		echo "TCONFIG_UPS=y" >>$(1); \
	fi; \
	if [ "$(NANO)" = "y" ]; then \
		sed -i "/TCONFIG_NANO/d" $(1); \
		echo "TCONFIG_NANO=y" >>$(1); \
	fi; \
	if [ "$(DNSCRYPT)" = "y" ]; then \
		sed -i "/TCONFIG_DNSCRYPT/d" $(1); \
		echo "TCONFIG_DNSCRYPT=y" >>$(1); \
	fi; \
        if [ "$(NVRAM_64K)" = "y" ]; then \
                sed -i "/TCONFIG_NVRAM_64K/d" $(1); \
                echo "TCONFIG_NVRAM_64K=y" >>$(1); \
        fi; \
	if [ "$(TOR)" = "y" ]; then \
		sed -i "/TCONFIG_TOR/d" $(1); \
		echo "TCONFIG_TOR=y" >>$(1); \
	fi; \
	if [ "$(MICROSD)" = "y" ]; then \
		sed -i "/TCONFIG_MICROSD/d" $(1); \
		echo "TCONFIG_MICROSD=y" >>$(1); \
	fi; \
	if [ "$(USBAP)" = "y" ]; then \
		sed -i "/TCONFIG_USBAP/d" $(1); \
		echo "TCONFIG_USBAP=y" >>$(1); \
		if [ "$(NO_USBAPP)" = "y" ]; then \
			sed -i "/TCONFIG_REMOVE_USBAPP/d" $(1); \
			echo "TCONFIG_REMOVE_USBAPP=y" >>$(1); \
			sed -i "/TCONFIG_FTP/d" $(1); \
			echo "# TCONFIG_FTP is not set" >>$(1); \
			echo "# TCONFIG_FTP_SSL is not set" >>$(1); \
			sed -i "/TCONFIG_SAMBA/d" $(1); \
			echo "# TCONFIG_SAMBASRV is not set" >>$(1); \
			echo "# TCONFIG_SAMBA3 is not set" >>$(1); \
		fi; \
	fi; \
	if [ "$(ASUS_TRX)" = "RT-AC66U" ]; then \
		sed -i "/TCONFIG_AC66U/d" $(1); \
		echo "TCONFIG_AC66U=y" >>$(1); \
	else \
		sed -i "/TCONFIG_AC66U/d" $(1); \
		echo "# TCONFIG_AC66U is not set" >>$(1); \
	fi; \
	if [ "$(CTF)" = "y" ]; then \
		sed -i "/TCONFIG_CTF/d" $(1); \
		echo "TCONFIG_CTF=y" >>$(1); \
	fi; \
	if [ "$(NAND)" = "y" ]; then \
		sed -i "/TCONFIG_NAND/d" $(1); \
		echo "TCONFIG_NAND=y" >>$(1); \
	fi; \
	if [ "$(ARM)" = "y" ]; then \
		sed -i "/TCONFIG_BCMARM/d" $(1); \
		echo "TCONFIG_BCMARM=y" >>$(1); \
		sed -i "/TCONFIG_BCMWL6/d" $(1); \
		echo "TCONFIG_BCMWL6=y" >>$(1); \
		echo "TCONFIG_BCMWL6A=y" >>$(1); \
		if [ "$(BCM7)" = "y" ]; then \
			sed -i "/TCONFIG_BCM7/d" $(1); \
			echo "TCONFIG_BCM7=y" >>$(1); \
			sed -i "/TCONFIG_TOAD/d" $(1); \
			echo "TCONFIG_TOAD=y" >>$(1); \
			sed -i "/TCONFIG_BCMBSD/d" $(1); \
			echo "TCONFIG_BCMBSD=y" >>$(1); \
			sed -i "/TCONFIG_BCMDCS/d" $(1); \
			echo "TCONFIG_BCMDCS=y" >>$(1); \
			sed -i "/TCONFIG_TRAFFIC_MGMT_RSSI_POLICY/d" $(1); \
			echo "TCONFIG_TRAFFIC_MGMT_RSSI_POLICY=y" >>$(1); \
			sed -i "/TCONFIG_GMAC3/d" $(1); \
			if [ "$(GMAC3)" = "y" ]; then \
				echo "TCONFIG_GMAC3=y" >>$(1); \
			else \
				echo "# TCONFIG_GMAC3 is not set" >>$(1); \
			fi; \
			sed -i "/TCONFIG_5301X/d" $(1); \
			echo "TCONFIG_5301X=y" >>$(1); \
		fi; \
	fi; \
	if [ "$(DHDAP)" = "y" ]; then \
		sed -i "/TCONFIG_DHDAP/d" $(1); \
		echo "TCONFIG_DHDAP=y" >>$(1); \
	fi; \
	if [ "$(AC3200)" = "y" ]; then \
		sed -i "/TCONFIG_AC3200/d" $(1); \
		echo "TCONFIG_AC3200=y" >>$(1); \
	fi; \
	if [ "$(BCMSMP)" = "y" ]; then \
		sed -i "/TCONFIG_BCMSMP/d" $(1); \
		echo "TCONFIG_BCMSMP=y" >>$(1); \
	fi; \
	if [ "$(GRO)" = "y" ]; then \
		sed -i "/TCONFIG_GROCTRL/d" $(1); \
		echo "TCONFIG_GROCTRL=y" >>$(1); \
	fi; \
	if [ "$(BCMFA)" = "y" ]; then \
		sed -i "/TCONFIG_BCMFA/d" $(1); \
		echo "TCONFIG_BCMFA=y" >>$(1); \
	fi; \
	if [ "$(TINC)" = "y" ]; then \
		sed -i "/TCONFIG_TINC/d" $(1); \
		echo "TCONFIG_TINC=y" >>$(1); \
	fi; \
	if [ "$(FANCTRL)" = "y" ]; then \
		sed -i "/TCONFIG_FANCTRL/d" $(1); \
		echo "TCONFIG_FANCTRL=y" >>$(1); \
	fi; \
	)
endef

define BusyboxOptions
	@( \
	if [ "$(CONFIG_LINUX26)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_2_4_MODULES/d" $(1); \
		echo "# CONFIG_FEATURE_2_4_MODULES is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_LSMOD_PRETTY_2_6_OUTPUT/d" $(1); \
		echo "CONFIG_FEATURE_LSMOD_PRETTY_2_6_OUTPUT=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_DEVFS/d" $(1); \
		echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1); \
		sed -i "/CONFIG_MKNOD/d" $(1); \
		echo "CONFIG_MKNOD=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SYSLOGD_READ_BUFFER_SIZE/d" $(1); \
		echo "CONFIG_FEATURE_SYSLOGD_READ_BUFFER_SIZE=512" >>$(1); \
	else \
		sed -i "/CONFIG_UDHCPC=y/d" $(1); \
		sed -i "/CONFIG_UDHCPC_OLD/d" $(1); \
		echo "CONFIG_UDHCPC_OLD=y" >>$(1); \
		echo "# CONFIG_UDHCPC is not set" >>$(1); \
	fi; \
	if [ "$(NO_CIFS)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_MOUNT_CIFS/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_CIFS is not set" >>$(1); \
	fi; \
	if [ "$(BBEXTRAS)" = "y" ]; then \
		sed -i "/CONFIG_SENDMAIL/d" $(1); \
		echo "CONFIG_SENDMAIL=y" >>$(1); \
		sed -i "/CONFIG_WHOIS/d" $(1); \
		echo "CONFIG_WHOIS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SORT_BIG/d" $(1); \
		echo "CONFIG_FEATURE_SORT_BIG=y" >>$(1); \
		sed -i "/CONFIG_CLEAR/d" $(1); \
		echo "CONFIG_CLEAR=y" >>$(1); \
		sed -i "/CONFIG_NICE/d" $(1); \
		echo "CONFIG_NICE=y" >>$(1); \
		sed -i "/CONFIG_SETCONSOLE/d" $(1); \
		echo "CONFIG_SETCONSOLE=y" >>$(1); \
		sed -i "/CONFIG_MKFIFO/d" $(1); \
		echo "CONFIG_MKFIFO=y" >>$(1); \
		sed -i "/CONFIG_SEQ/d" $(1); \
		echo "CONFIG_SEQ=y" >>$(1); \
		sed -i "/CONFIG_STTY/d" $(1); \
		echo "CONFIG_STTY=y" >>$(1); \
	fi; \
	if [ "$(USB)" = "USB" ]; then \
		if [ "$(USBEXTRAS)" = "y" ]; then \
			sed -i "/CONFIG_E2FSCK/d" $(1); \
			echo "CONFIG_E2FSCK=y" >>$(1); \
			sed -i "/CONFIG_MKE2FS/d" $(1); \
			echo "CONFIG_MKE2FS=y" >>$(1); \
			sed -i "/CONFIG_FDISK/d" $(1); \
			echo "CONFIG_FDISK=y" >>$(1); \
			sed -i "/CONFIG_FEATURE_FDISK_WRITABLE/d" $(1); \
			echo "CONFIG_FEATURE_FDISK_WRITABLE=y" >>$(1); \
			sed -i "/CONFIG_MKFS_VFAT/d" $(1); \
			echo "CONFIG_MKFS_VFAT=y" >>$(1); \
			sed -i "/CONFIG_MKSWAP/d" $(1); \
			echo "CONFIG_MKSWAP=y" >>$(1); \
			sed -i "/CONFIG_FLOCK/d" $(1); \
			echo "CONFIG_FLOCK=y" >>$(1); \
			sed -i "/CONFIG_FSYNC/d" $(1); \
			echo "CONFIG_FSYNC=y" >>$(1); \
			sed -i "/CONFIG_TUNE2FS/d" $(1); \
			echo "CONFIG_TUNE2FS=y" >>$(1); \
			sed -i "/CONFIG_E2LABEL/d" $(1); \
			echo "CONFIG_E2LABEL=y" >>$(1); \
			if [ "$(CONFIG_LINUX26)" = "y" ]; then \
				sed -i "/CONFIG_LSUSB/d" $(1); \
				echo "CONFIG_LSUSB=y" >>$(1); \
				sed -i "/CONFIG_FEATURE_WGET_STATUSBAR/d" $(1); \
				echo "CONFIG_FEATURE_WGET_STATUSBAR=y" >>$(1); \
				sed -i "/CONFIG_FEATURE_VERBOSE_USAGE/d" $(1); \
				echo "CONFIG_FEATURE_VERBOSE_USAGE=y" >>$(1); \
			fi; \
		fi; \
	else \
		sed -i "/CONFIG_FEATURE_MOUNT_LOOP/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_LOOP is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_DEVFS/d" $(1); \
		echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_MOUNT_LABEL/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_LABEL is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_MOUNT_FSTAB/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_FSTAB is not set" >>$(1); \
		sed -i "/CONFIG_VOLUMEID/d" $(1); \
		echo "# CONFIG_VOLUMEID is not set" >>$(1); \
		sed -i "/CONFIG_BLKID/d" $(1); \
		echo "# CONFIG_BLKID is not set" >>$(1); \
		sed -i "/CONFIG_SWAPONOFF/d" $(1); \
		echo "# CONFIG_SWAPONOFF is not set" >>$(1); \
		sed -i "/CONFIG_CHROOT/d" $(1); \
		echo "# CONFIG_CHROOT is not set" >>$(1); \
		sed -i "/CONFIG_PIVOT_ROOT/d" $(1); \
		echo "# CONFIG_PIVOT_ROOT is not set" >>$(1); \
		sed -i "/CONFIG_TRUE/d" $(1); \
		echo "# CONFIG_TRUE is not set" >>$(1); \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_IPV6/d" $(1); \
		echo "CONFIG_FEATURE_IPV6=y" >>$(1); \
		sed -i "/CONFIG_PING6/d" $(1); \
		echo "CONFIG_PING6=y" >>$(1); \
		sed -i "/CONFIG_TRACEROUTE6/d" $(1); \
		echo "CONFIG_TRACEROUTE6=y" >>$(1); \
		if [ "$(CONFIG_LINUX26)" = "y" ]; then \
			sed -i "/CONFIG_FEATURE_UDHCP_RFC5969/d" $(1); \
			echo "CONFIG_FEATURE_UDHCP_RFC5969=y" >>$(1); \
		fi; \
	fi; \
	if [ "$(SLIM)" = "y" ]; then \
		sed -i "/CONFIG_AWK/d" $(1); \
		echo "# CONFIG_AWK is not set" >>$(1); \
		sed -i "/CONFIG_BASENAME/d" $(1); \
		echo "# CONFIG_BASENAME is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_DEVFS/d" $(1); \
		echo "# CONFIG_FEATURE_DEVFS is not set" >>$(1); \
		sed -i "/CONFIG_BLKID/d" $(1); \
		echo "# CONFIG_BLKID is not set" >>$(1); \
		sed -i "/CONFIG_TELNET=y/d" $(1); \
		echo "# CONFIG_TELNET is not set" >>$(1); \
		sed -i "/CONFIG_ARPING/d" $(1); \
		echo "# CONFIG_ARPING is not set" >>$(1); \
		sed -i "/CONFIG_FEATURE_LS_COLOR/d" $(1); \
		echo "# CONFIG_FEATURE_LS_COLOR is not set" >>$(1); \
		sed -i "/CONFIG_CHOWN/d" $(1); \
		echo "# CONFIG_CHOWN is not set" >>$(1); \
	else \
		sed -i "/CONFIG_FEATURE_LS_COLOR/d" $(1); \
		echo "CONFIG_FEATURE_LS_COLOR=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_LS_COLOR_IS_DEFAULT/d" $(1); \
		echo "CONFIG_FEATURE_LS_COLOR_IS_DEFAULT=y" >>$(1); \
	fi; \
	if [ "$(BCMSMP)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_TOP_SMP_CPU/d" $(1); \
		echo "CONFIG_FEATURE_TOP_SMP_CPU=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOP_DECIMALS/d" $(1); \
		echo "CONFIG_FEATURE_TOP_DECIMALS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOP_SMP_PROCESS/d" $(1); \
		echo "CONFIG_FEATURE_TOP_SMP_PROCESS=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_TOPMEM/d" $(1); \
		echo "CONFIG_FEATURE_TOPMEM=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SHOW_THREADS/d" $(1); \
		echo "CONFIG_FEATURE_SHOW_THREADS=y" >>$(1); \
	fi; \
	)
endef

define KernelConfig
	@( \
	sed -i "/CONFIG_NVRAM_SIZE/d" $(1); \
	echo "CONFIG_NVRAM_SIZE="$(NVRAM_SIZE) >>$(1); \
	sed -i "/CONFIG_CC_OPTIMIZE_FOR_SIZE/d" $(1); \
	if [ "$(KERN_SIZE_OPT)" = "y" ]; then \
		echo "CONFIG_CC_OPTIMIZE_FOR_SIZE=y" >>$(1); \
	else \
		echo "# CONFIG_CC_OPTIMIZE_FOR_SIZE is not set" >>$(1); \
	fi; \
	if [ "$(NAND)" = "y" ]; then \
		sed -i "/CONFIG_MTD_NFLASH/d" $(1); \
		echo "CONFIG_MTD_NFLASH=y" >>$(1); \
		sed -i "/CONFIG_MTD_NAND/d" $(1); \
		echo "CONFIG_MTD_NAND=y" >>$(1); \
		echo "CONFIG_MTD_NAND_IDS=y" >>$(1); \
		echo "# CONFIG_MTD_NAND_DENALI is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_RICOH is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_VERIFY_WRITE is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_ECC_SMC is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_MUSEUM_IDS is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_DISKONCHIP is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_CAFE is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_NANDSIM is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_PLATFORM is not set" >>$(1); \
		echo "# CONFIG_MTD_NAND_ONENAND is not set" >>$(1); \
		sed -i "/CONFIG_MTD_BRCMNAND/d" $(1); \
		echo "CONFIG_MTD_BRCMNAND=y" >>$(1); \
	fi; \
	if [ "$(UPS)" = "y" ]; then \
		sed -i "/CONFIG_INPUT=m/d" $(1); \
		echo "CONFIG_INPUT=y" >>$(1); \
		sed -i "/CONFIG_HID=m/d" $(1); \
		echo "CONFIG_HID=y" >>$(1); \
		sed -i "/CONFIG_USB_STORAGE_ONETOUCH/d" $(1); \
		echo "# CONFIG_USB_STORAGE_ONETOUCH is not set" >>$(1); \
	else \
		sed -i "/CONFIG_USB_STORAGE_ONETOUCH/d" $(1); \
		echo "# CONFIG_USB_STORAGE_ONETOUCH is not set" >>$(1); \
	fi; \
	if [ "$(USB)" = "" ]; then \
		sed -i "/CONFIG_EFI_PARTITION/d" $(1); \
		echo "# CONFIG_EFI_PARTITION is not set" >>$(1); \
	fi; \
	if [ "$(AC3200)" = "y" ]; then \
		sed -i "/CONFIG_RTAC3200/d" $(1); \
		echo "CONFIG_RTAC3200=y" >>$(1); \
	fi; \
	if [ "$(GMAC3)" = "y" ]; then \
		sed -i "/CONFIG_BCM_GMAC3/d" $(1); \
		echo "CONFIG_BCM_GMAC3=y" >>$(1); \
	else \
		sed -i "/CONFIG_BCM_GMAC3/d" $(1); \
		echo "# CONFIG_BCM_GMAC3 is not set" >>$(1); \
	fi; \
	if [ "$(BCM_RECVFILE)" = "y" ]; then \
		sed -i "/CONFIG_BCM_RECVFILE/d" $(1); \
		echo "CONFIG_BCM_RECVFILE=y" >>$(1); \
	fi; \
	if [ "$(DEBUGFS)" = "y" ]; then \
		sed -i "/CONFIG_DEBUG_FS/d" $(1); \
		echo "CONFIG_DEBUG_FS=y" >>$(1); \
		sed -i "/CONFIG_USB_MON/d" $(1); \
		echo "CONFIG_USB_MON=m" >>$(1); \
		if [ "$(ARM)" = "y" ]; then \
			sed -i "/CONFIG_GCOV_KERNEL/d" $(1); \
			echo "# CONFIG_GCOV_KERNEL is not set" >>$(1); \
			sed -i "/CONFIG_L2TP_DEBUGFS/d" $(1); \
			echo "# CONFIG_L2TP_DEBUGFS is not set" >>$(1); \
			sed -i "/CONFIG_JBD_DEBUG/d" $(1); \
			echo "# CONFIG_JBD_DEBUG is not set" >>$(1); \
			sed -i "/CONFIG_JBD2_DEBUG/d" $(1); \
			echo "# CONFIG_JBD2_DEBUG is not set" >>$(1); \
			sed -i "/CONFIG_LKDTM/d" $(1); \
			echo "# CONFIG_LKDTM is not set" >>$(1); \
			sed -i "/CONFIG_DYNAMIC_DEBUG/d" $(1); \
			echo "# CONFIG_DYNAMIC_DEBUG is not set" >>$(1); \
		fi; \
	fi; \
	if [ "$(NOWL)" = "y" ]; then \
		sed -i "/CONFIG_WL=m/d" $(1); \
		echo "# CONFIG_WL is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_APSTA/d" $(1); \
		echo "# CONFIG_WL_USE_APSTA is not set" >>$(1); \
		sed -i "/CONFIG_WL_ALL_PASSIVE_RUNTIME/d" $(1); \
		sed -i "/CONFIG_WAPI/d" $(1); \
		echo "# CONFIG_WAPI is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_AP/d" $(1); \
		echo "# CONFIG_WL_USE_AP is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_AP_SDSTD/d" $(1); \
		echo "# CONFIG_WL_USE_AP_SDSTD is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_STA/d" $(1); \
		echo "# CONFIG_WL_USE_STA is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_AP_ONCHIP_G/d" $(1); \
		echo "# CONFIG_WL_USE_AP_ONCHIP_G is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_STA_ONCHIP_G/d" $(1); \
		echo "# CONFIG_WL_USE_STA_ONCHIP_G is not set" >>$(1); \
		sed -i "/CONFIG_WL_USE_APSTA_ONCHIP_G/d" $(1); \
		echo "# CONFIG_WL_USE_APSTA_ONCHIP_G is not set" >>$(1); \
		sed -i "/CONFIG_WL_ALL_PASSIVE_ON/d" $(1); \
		echo "# CONFIG_WL_ALL_PASSIVE_ON is not set" >>$(1); \
		sed -i "/CONFIG_DPSTA/d" $(1); \
		echo "# CONFIG_DPSTA is not set" >>$(1); \
		sed -i "/CONFIG_PLC/d" $(1); \
		echo "# CONFIG_PLC is not set" >>$(1); \
	fi; \
	if [ "$(IPV6SUPP)" = "y" ]; then \
		sed -i "/CONFIG_IPV6 is not set/d" $(1); \
		echo "CONFIG_IPV6=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_IPTABLES/d" $(1); \
		echo "CONFIG_IP6_NF_IPTABLES=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_MATCH_RT/d" $(1); \
		echo "CONFIG_IP6_NF_MATCH_RT=y" >>$(1); \
		sed -i "/CONFIG_IP6_NF_FILTER/d" $(1); \
		echo "CONFIG_IP6_NF_FILTER=m" >>$(1); \
		sed -i "/CONFIG_IP6_NF_TARGET_LOG/d" $(1); \
		echo "CONFIG_IP6_NF_TARGET_LOG=m" >>$(1); \
		sed -i "/CONFIG_IP6_NF_TARGET_REJECT/d" $(1); \
		echo "CONFIG_IP6_NF_TARGET_REJECT=m" >>$(1); \
		sed -i "/CONFIG_IP6_NF_MANGLE/d" $(1); \
		echo "CONFIG_IP6_NF_MANGLE=m" >>$(1); \
		if [ "$(CONFIG_LINUX26)" = "y" ]; then \
			sed -i "/CONFIG_NF_CONNTRACK_IPV6/d" $(1); \
			echo "CONFIG_NF_CONNTRACK_IPV6=m" >>$(1); \
			sed -i "/CONFIG_NETFILTER_XT_MATCH_HL/d" $(1); \
			echo "CONFIG_NETFILTER_XT_MATCH_HL=m" >>$(1); \
			sed -i "/CONFIG_IPV6_ROUTER_PREF/d" $(1); \
			echo "CONFIG_IPV6_ROUTER_PREF=y" >>$(1); \
			sed -i "/CONFIG_IPV6_SIT/d" $(1); \
			echo "CONFIG_IPV6_SIT=m" >>$(1); \
			sed -i "/CONFIG_IPV6_SIT_6RD/d" $(1); \
			echo "CONFIG_IPV6_SIT_6RD=y" >>$(1); \
			sed -i "/CONFIG_IPV6_MULTIPLE_TABLES/d" $(1); \
			echo "CONFIG_IPV6_MULTIPLE_TABLES=y" >>$(1); \
			sed -i "/CONFIG_IP6_NF_TARGET_ROUTE/d" $(1); \
			echo "CONFIG_IP6_NF_TARGET_ROUTE=m" >>$(1); \
			sed -i "/CONFIG_IPV6_MROUTE\b/d" $(1); \
			echo "CONFIG_IPV6_MROUTE=y" >>$(1); \
		else \
			sed -i "/CONFIG_IP6_NF_CONNTRACK/d" $(1); \
			echo "CONFIG_IP6_NF_CONNTRACK=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_HL/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_HL=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_FTP/d" $(1); \
			echo "CONFIG_IP6_NF_FTP=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_LIMIT/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_LIMIT=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_CONDITION/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_CONDITION=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_MAC/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_MAC=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_MULTIPORT/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_MULTIPORT=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_MARK/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_MARK=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_LENGTH/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_LENGTH=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_MATCH_STATE/d" $(1); \
			echo "CONFIG_IP6_NF_MATCH_STATE=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_TARGET_MARK/d" $(1); \
			echo "CONFIG_IP6_NF_TARGET_MARK=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_TARGET_TCPMSS/d" $(1); \
			echo "CONFIG_IP6_NF_TARGET_TCPMSS=m" >>$(1); \
			sed -i "/CONFIG_IP6_NF_TARGET_ROUTE/d" $(1); \
			echo "CONFIG_IP6_NF_TARGET_ROUTE=m" >>$(1); \
		fi; \
	fi; \
	if [ "$(IPSEC)" = "y" ]; then \
		sed -i "/CONFIG_XFRM_USER/d" $(1); \
		echo "CONFIG_XFRM_USER=m" >>$(1); \
		echo "# CONFIG_XFRM_SUB_POLICY is not set" >>$(1); \
		echo "# CONFIG_XFRM_MIGRATE is not set" >>$(1); \
		echo "# CONFIG_XFRM_STATISTICS is not set" >>$(1); \
		sed -i "/CONFIG_NET_KEY/d" $(1); \
		echo "CONFIG_NET_KEY=m" >>$(1); \
		echo "# CONFIG_NET_KEY_MIGRATE is not set" >>$(1); \
		sed -i "/CONFIG_INET_AH/d" $(1); \
		echo "CONFIG_INET_AH=m" >>$(1); \
		sed -i "/CONFIG_INET_ESP/d" $(1); \
		echo "CONFIG_INET_ESP=m" >>$(1); \
		sed -i "/CONFIG_INET_IPCOMP/d" $(1); \
		echo "CONFIG_INET_IPCOMP=m" >>$(1); \
		sed -i "/CONFIG_INET_XFRM_MODE_TRANSPORT/d" $(1); \
		echo "CONFIG_INET_XFRM_MODE_TRANSPORT=m" >>$(1); \
		sed -i "/CONFIG_INET_XFRM_MODE_TUNNEL/d" $(1); \
		echo "CONFIG_INET_XFRM_MODE_TUNNEL=m" >>$(1); \
		sed -i "/CONFIG_INET_XFRM_MODE_BEET/d" $(1); \
		echo "CONFIG_INET_XFRM_MODE_BEET=m" >>$(1); \
		sed -i "/CONFIG_NETFILTER_XT_MATCH_POLICY/d" $(1); \
		echo "CONFIG_NETFILTER_XT_MATCH_POLICY=m" >>$(1); \
		if [ "$(IPV6SUPP)" = "y" ]; then \
			sed -i "/CONFIG_INET6_AH/d" $(1); \
			echo "CONFIG_INET6_AH=m" >>$(1); \
			sed -i "/CONFIG_INET6_ESP/d" $(1); \
			echo "CONFIG_INET6_ESP=m" >>$(1); \
			sed -i "/CONFIG_INET6_IPCOMP/d" $(1); \
			echo "CONFIG_INET6_IPCOMP=m" >>$(1); \
			sed -i "/CONFIG_INET6_XFRM_MODE_TRANSPORT/d" $(1); \
			echo "CONFIG_INET6_XFRM_MODE_TRANSPORT=m" >>$(1); \
			sed -i "/CONFIG_INET6_XFRM_MODE_TUNNEL/d" $(1); \
			echo "CONFIG_INET6_XFRM_MODE_TUNNEL=m" >>$(1); \
			sed -i "/CONFIG_INET6_XFRM_MODE_BEET/d" $(1); \
			echo "CONFIG_INET6_XFRM_MODE_BEET=m" >>$(1); \
		fi; \
	fi; \
	sed -i "/CONFIG_BCM57XX/d" $(1); \
	if [ "$(BCM57)" = "y" ]; then \
		echo "CONFIG_BCM57XX=m" >>$(1); \
	else \
		echo "# CONFIG_BCM57XX is not set" >>$(1); \
	fi; \
		sed -i "/CONFIG_LINUX_MTD/d" $(1); \
	if [ "$(LINUX_MTD)" = "" ]; then \
		echo "CONFIG_LINUX_MTD=32" >>$(1); \
	else \
		echo "CONFIG_LINUX_MTD=$(LINUX_MTD)" >>$(1); \
	fi; \
	if [ "$(ARMCPUSMP)" = "up" ]; then \
		sed -i "/CONFIG_GENERIC_CLOCKEVENTS_BROADCAST/d" $(1); \
		echo "CONFIG_HAVE_LATENCYTOP_SUPPORT=y" >>$(1); \
		sed -i "/CONFIG_GENERIC_LOCKBREAK/d" $(1); \
		echo "CONFIG_BROKEN_ON_SMP=y" >>$(1); \
		sed -i "/CONFIG_TREE_RCU=y/# CONFIG_TREE_RCU is not set/g" >>$(1); \
		echo "CONFIG_TREE_PREEMPT_RCU=y" >>$(1); \
		echo "# CONFIG_TINY_RCU is not set" >>$(1); \
		sed -i "/CONFIG_USE_GENERIC_SMP_HELPERS/d" $(1); \
		sed -i "/CONFIG_STOP_MACHINE/d" $(1); \
		sed -i "/CONFIG_MUTEX_SPIN_ON_OWNER/d" $(1); \
		echo "# CONFIG_MUTEX_SPIN_ON_OWNER is not set" >>$(1); \
		sed -i "/# CONFIG_ARM_ERRATA_742230 is not set/d" $(1); \
		sed -i "/# CONFIG_ARM_ERRATA_742231 is not set/d" $(1); \
		sed -i "/# CONFIG_ARM_ERRATA_720789 is not set/d" $(1); \
		sed -i "/CONFIG_SMP=y/d" $(1); \
		echo "# CONFIG_SMP is not set" >>$(1); \
		sed -i "/CONFIG_NR_CPUS=2/d" $(1); \
		sed -i "/# CONFIG_HOTPLUG_CPU is not set/d" $(1); \
		sed -i "/CONFIG_RPS=y/d" $(1); \
	fi; \
	if [ "$(USBAP)" = "y" ]; then \
		echo "CONFIG_WL_USBAP=y" >>$(1); \
		echo 'CONFIG_WL_APSTA="wlconfig_lx_router_high"' >>$(1); \
	else \
		echo "# CONFIG_WL_USBAP is not set" >>$(1); \
		echo 'CONFIG_WL_APSTA="wlconfig_lx_router_apsta"' >>$(1); \
	fi; \
	if [ "$(CONFIG_LINUX26)" = "y" ] && [ "$(EBTABLES)" = "y" ]; then \
		sed -i "/CONFIG_BRIDGE_NF_EBTABLES/d" $(1); \
		echo "CONFIG_BRIDGE_NF_EBTABLES=m" >>$(1); \
		if [ "$(IPV6SUPP)" = "y" ]; then \
			sed -i "/CONFIG_BRIDGE_EBT_IP6/d" $(1); \
			echo "CONFIG_BRIDGE_EBT_IP6=m" >>$(1); \
		fi; \
	fi; \
        sed -i "/CONFIG_NVRAM_64K/d" $(1); \
        if [ "$(NVRAM_64K)" = "y" ]; then \
                echo "CONFIG_NVRAM_64K=y" >>$(1); \
        else \
                echo "# CONFIG_NVRAM_64K is not set" >>$(1); \
	fi; \
	if [ "$(SATA)" = "y" ]; then \
		sed -i "/CONFIG_CMDLINE/d" $(1); \
		echo "CONFIG_CMDLINE=\"root=\/dev\/mtdblock2 console=ttyS0,115200 init=\/sbin\/preinit earlyprintk debug libata.force=3.0Gbps\"" >>$(1); \
		echo "# CONFIG_CMDLINE_FORCE is not set" >>$(1); \
		sed -i "/CONFIG_SCSI_MOD/d" $(1); \
		echo "CONFIG_SCSI_MOD=y" >>$(1); \
		sed -i "/CONFIG_SCSI=m/d" $(1); \
		echo "CONFIG_SCSI=y" >>$(1); \
		sed -i "/CONFIG_SCSI_PROC_FS/d" $(1); \
		echo "CONFIG_SCSI_PROC_FS=y" >>$(1); \
		sed -i "/CONFIG_SCSI_MULTI_LUN/d" $(1); \
		echo "CONFIG_SCSI_MULTI_LUN=y" >>$(1); \
		sed -i "/CONFIG_SCSI_CONSTANTS/d" $(1); \
		echo "CONFIG_SCSI_CONSTANTS=y" >>$(1); \
		sed -i "/CONFIG_SCSI_LOWLEVEL/d" $(1); \
		echo "CONFIG_SCSI_LOWLEVEL=y" >>$(1); \
		sed -i "/CONFIG_BLK_DEV_SD/d" $(1); \
		echo "CONFIG_BLK_DEV_SD=y" >>$(1); \
		sed -i "/CONFIG_ATA is not set/d" $(1); \
		echo "CONFIG_ATA=y" >>$(1); \
		sed -i "/CONFIG_SATA_PMP/d" $(1); \
		echo "CONFIG_SATA_PMP=y" >>$(1); \
		sed -i "/CONFIG_SATA_AHCI/d" $(1); \
		echo "CONFIG_SATA_AHCI=y" >>$(1); \
		sed -i "/CONFIG_SATA_AHCI_PLATFORM/d" $(1); \
		echo "CONFIG_SATA_AHCI_PLATFORM=y" >>$(1); \
		sed -i "/CONFIG_ATA_SFF/d" $(1); \
		echo "CONFIG_ATA_SFF=y" >>$(1); \
		sed -i "/CONFIG_ATA_BMDMA/d" $(1); \
		echo "CONFIG_ATA_BMDMA=y" >>$(1); \
		sed -i "/CONFIG_ATA_VERBOSE_ERROR/d" $(1); \
		echo "# CONFIG_ATA_VERBOSE_ERROR is not set" >>$(1); \
	fi; \
	sed -i "/CONFIG_FAILSAFE_UPGRADE/d" $(1); \
	if [ "$(FAILSAFE_UPGRADE)" = "y" ]; then \
		echo "CONFIG_FAILSAFE_UPGRADE=y" >>$(1); \
	else \
		echo "# CONFIG_FAILSAFE_UPGRADE is not set" >> $(1); \
	fi \
	)
endef


bin:
ifeq ($(B),)
	@echo $@" is not a valid target!"
	@false
endif
	@cp router/config_base router/config_$(lowercase_B)
	@cp router/busybox/config_base router/busybox/config_$(lowercase_B)
	@cp $(LINUXDIR)/config_base $(LINUXDIR)/config_$(lowercase_B)

	$(call RouterOptions, router/config_$(lowercase_B))
	$(call KernelConfig, $(LINUXDIR)/config_$(lowercase_B))
	$(call BusyboxOptions, router/busybox/config_$(lowercase_B))

	@$(MAKE) setprofile N=$(TPROFILE) B=$(B) DESC="$(BUILD_DESC)" USB="$(USB)"
	@$(MAKE) all

## targets
e:
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y IPV6SUPP=y MEDIASRV=y  B=E BUILD_DESC="$(VPN)" CTF=y GRO=y USB="USB" PPTPD=y OPENVPN=y NMP=y

ac68e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="ASUS" TUXERA=y

n18e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y ARMCPUSMP="up" ASUS_TRX="ASUS" UFSD="ASUS"

dir868l:
	@$(MAKE) bin ARM=y NVRAM_SIZE=32 NAND=y BCMSMP=y ASUS_TRX="DLINK" CTF=y GRO=y NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y IPV6SUPP=y MEDIASRV=y B=E BUILD_DESC="special" USB="USB" DNSSEC=y DNSCRYPT=y UPS=y NO_SSH=y NANO=y

r7000e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" UFSD="ASUS"

r7000init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y BUILD_DESC="initial" NETGEAR_CHK="R7000" BOARDID_FILE="compatible_r7000.txt" FW_CFG_FILE="ambitCfg-r7000.h"

r6250init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y BUILD_DESC="initial" NETGEAR_CHK="R6250" BOARDID_FILE="compatible_r6250.txt" FW_CFG_FILE="ambitCfg-r6250.h"

r6300v2init:
	@$(MAKE) bin ARM=y NVRAM_64K=y NAND=y BCMSMP=y B=E IPV6SUPP=y OPENVPN=y BUILD_DESC="initial" NETGEAR_CHK="R6300v2" BOARDID_FILE="compatible_r6300v2.txt" FW_CFG_FILE="ambitCfg-r6300v2.h"

ws880e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="HUAWEI"

wzr1750e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="BUFFALO"

ea6700e:
	@$(MAKE) e ARM=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="LINKSYS" FAILSAFE_UPGRADE=n

r1do:
	@$(MAKE) o ARM=y NVRAM_64K=y BCMSMP=y XIAOMI_TRX="R1D" SATA=y NO_JFFS=y FANCTRL=y

ifeq ($(CONFIG_BCM7),y)
r8000init:
	@$(MAKE) bin ARM=y BCM7=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" AC3200=y DHDAP=y GMAC3=y NOWL=y DEBUGFS=y BCM_RECVFILE=y LINUX_MTD=64 B=E IPV6SUPP=y OPENVPN=y BUILD_DESC="initial" NETGEAR_CHK="R8000" BOARDID_FILE="compatible_r8000.txt" FW_CFG_FILE="ambitCfg-r8000.h"

r8000e:
	@$(MAKE) e ARM=y BCM7=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="NETGEAR" AC3200=y DHDAP=y GMAC3=y NOWL=y DEBUGFS=y BCM_RECVFILE=y LINUX_MTD=64

ac3200e:
	@$(MAKE) e ARM=y BCM7=y NVRAM_64K=y NAND=y BCMSMP=y ASUS_TRX="ASUS" TUXERA=y AC3200=y DHDAP=y GMAC3=y NOWL=y DEBUGFS=y BCM_RECVFILE=y LINUX_MTD=64

endif

setprofile:
	echo '#ifndef TOMATO_PROFILE' > router/shared/tomato_profile.h
	echo '#define TOMATO_$(N) 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_G 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_N 2' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE PROFILE_$(N)' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE_NAME "$(N)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_NAME "$(B)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_DESC "$(DESC)$(EXTRA_64KDESC)"' >> router/shared/tomato_profile.h
	echo '#ifndef CONFIG_NVRAM_SIZE' >> router/shared/tomato_profile.h
	echo '#define CONFIG_NVRAM_SIZE $(NVRAM_SIZE)' >> router/shared/tomato_profile.h
	echo '#endif' >> router/shared/tomato_profile.h
	echo '#endif' >> router/shared/tomato_profile.h

	echo 'TOMATO_$(N) = 1' > tomato_profile.mak
	echo 'PROFILE_G = 1' >> tomato_profile.mak
	echo 'PROFILE_N = 2' >> tomato_profile.mak
	echo 'TOMATO_PROFILE = $$(PROFILE_$(N))' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_NAME = "$(N)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD = "$(B)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD_NAME = "$(B)"' >> tomato_profile.mak
	echo 'TOMATO_BUILD_DESC = "$(DESC)$(EXTRA_64KDESC)"' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_L = $(lowercase_N)' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_U = $(uppercase_N)' >> tomato_profile.mak
	echo 'TOMATO_BUILD_USB = "$(USB)"' >> tomato_profile.mak

	echo 'export EXTRACFLAGS := $(EXTRA_CFLAGS) -DBCMWPA2 -DBCMARM -marm $(if $(filter $(NVRAM_SIZE),0),,-DCONFIG_NVRAM_SIZE=$(NVRAM_SIZE)) $(EXTRA_64KCFLAG)' >> tomato_profile.mak

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

help:
ifeq ($(CONFIG_LINUX26),y)
	@echo "ASUS ARM Builds - RT-N18U, RT-AC56U, RT-AC68U, RT-AC68R, RT-AC68P"
	@echo "ac68e         RT-N18U/AC56U/AC68U(R/P) build VPN"
	@echo "n18e          RT-N18U without SMP build VPN"
	@echo "DLINK ARM Builds"
	@echo "dir868l       DLINK DIR868L"
	@echo "NETGEAR ARM Builds"
	@echo "r7000e        R7000/R6300v2/R6250 build VPN"
	@echo "r7000init     R7000 init build"
	@echo "r6250init     R6250 init build"
	@echo "r6300v2init   R6300v2 init build"
	@echo "Xiaomi ARM Builds"
	@echo "r1do          R1D custom build"
	@echo "HUAWEI ARM Builds"
	@echo "ws880e        WS880 build VPN"
	@echo "BUFFALO ARM Builds"
	@echo "wzr1750e      WZR-1750DHP build VPN"
	@echo "LINKSYS ARM Builds"
	@echo "ea6700e       EA6700/EA6500v2/EA6900 build VPN"
ifeq ($(CONFIG_BCM7),y)
	@echo "ARM7 Builds"
	@echo "r8000init      R8000 init build"
	@echo "r8000e         R8000 build VPN"
	@echo "ac3200e        RT-AC3200 build VPN"

endif
endif
	@echo ""
	@echo "..etc..      other build configs"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C Linux distclean (but preserves .config)"
	@echo "distclean    distclean of Linux & busybox (but preserve .configs)"
	@echo "prepk        -C Linux oldconfig dep"
	
.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile help
.PHONY: a b c d m nc Makefile allversions tomato_profile.mak
