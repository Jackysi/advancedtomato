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
RELEASEDIR := $(shell (cd $(SRCBASE)/.. && pwd -P))
PATH := $(RELEASEDIR)/tools:$(PATH)
export TPROFILE := N

include ./target.mak

# Normally you'd do something like "make V1=8516 V2=-sub-ver a b c d"
# But if you don't give anything for "V1", it'll get a default from tomato_version.
V1 ?= "--def"
VPN ?= "VPN"
ifeq ($(CONFIG_LINUX26),y)
ND = "K26"
else
ND = "ND"
endif

PPTPD ?= "n"

ifeq ($(NVRAM_SIZE),)
NVRAM_SIZE = 0
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
mips_rev = $(if $(filter $(MIPS32),r2),MIPSR2,MIPSR1)
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
	@$(MAKE) -C btools

	@rm -f image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx
	@rm -f image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
ifeq ($(LINKSYS_E),1)
	@rm -f image/tomato-E??00$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
ifeq ($(BELKIN),y)
ifneq ($(NVRAM_SIZE),60)
	@rm -f image/tomato-F7D????$(current_BUILD_USB)-$(current_TOMATO_VER).$(current_TOMATO_V1)$(mips_rev)$(beta)$(current_TOMATO_V2)-$(current_BUILD_DESC).bin
	@rm -f image/tomato-F5D8235v3$(current_BUILD_USB)-$(current_TOMATO_VER).$(current_TOMATO_V1)$(mips_rev)$(beta)$(current_TOMATO_V2)-$(current_BUILD_DESC).bin
endif
endif
	@echo "" >>fpkg.log
	@echo "***********************" `date` "************************" >>fpkg.log
	@cat router/shared/tomato_version >>fpkg.log
	@echo "" >>fpkg.log
	@cat router/target.info >>fpkg.log
ifeq ($(wildcard include/bcm20xx.h),)
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-t image/tomato.trx \
		-l W54G,image/WRT54G_WRT54GL.bin \
		-l W54S,image/WRT54GS.bin \
		-l W54s,image/WRT54GSv4.bin \
		-l W54U,image/WRTSL54GS.bin \
		-m 0x10577050,image/WR850G.bin \
		| tee -a fpkg.log
else
ifeq ($(LINKSYS_E),1)
	# Linksys E-series build plus generic TRX image
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-t image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
		-l 4200,image/tomato-E4200$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l 61XN,image/tomato-E3000$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l 32XN,image/tomato-E2000$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
else
ifeq ($(BELKIN),y)
ifneq ($(NVRAM_SIZE),60)
	# Create Belkin images
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-t image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
		-b 0x20100322,image/tomato-F7D3301$(current_BUILD_USB)-$(current_TOMATO_VER).$(current_TOMATO_V1)$(mips_rev)$(beta)$(current_TOMATO_V2)-$(current_BUILD_DESC).bin \
		-b 0x20090928,image/tomato-F7D3302$(current_BUILD_USB)-$(current_TOMATO_VER).$(current_TOMATO_V1)$(mips_rev)$(beta)$(current_TOMATO_V2)-$(current_BUILD_DESC).bin \
		-b 0x20091006,image/tomato-F7D4302$(current_BUILD_USB)-$(current_TOMATO_VER).$(current_TOMATO_V1)$(mips_rev)$(beta)$(current_TOMATO_V2)-$(current_BUILD_DESC).bin \
		-b 0x00017116,image/tomato-F5D8235v3$(current_BUILD_USB)-$(current_TOMATO_VER).$(current_TOMATO_V1)$(mips_rev)$(beta)$(current_TOMATO_V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
else
	# Create generic TRX image
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-t image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
		| tee -a fpkg.log
endif
endif
endif

	@cp fpkg.log image/fpkg-$(V1)$(mips_rev)$(beta)$(V2).log
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
	$(MAKE) distclean || true; \
	cp -p save-config .config || true

kernel:
	@$(MAKE) -C router kernel
	@ls -l $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/zImage

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
# MIPS32 (r2|r1)
# KERN_SIZE_OPT
# USB ("USB"|"")
# JFFSv1 | NO_JFFS
# NO_CIFS, NO_SSH, NO_ZEBRA, NO_SAMBA, NO_HTTPS, NO_XXTP, NO_LIBOPT
# SAMBA3, OPENVPN, IPV6SUPP, EBTABLES, NTFS, MEDIASRV, BBEXTRAS, USBEXTRAS, BCM57, SLIM, NOCAT, CTF, NGINX NANO
# NFS BTCLIENT BTGUI TR_EXTRAS SNMP SDHC HFS UPS DNSCRYPT PPTPD TOR IPSEC RAID

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
	sed -i "/TCONFIG_MULTIWAN/d" $(1); \
	if [ "$(NVRAM_SIZE)" = "60" ] || [ "$(NVRAM_SIZE)" = "64" ] || [ "$(NVRAM_64K)" = "y" ]; then \
		echo "TCONFIG_MULTIWAN=y" >>$(1); \
	else \
		echo "# TCONFIG_MULTIWAN is not set" >>$(1); \
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
	if [ "$(TOR)" = "y" ]; then \
		sed -i "/TCONFIG_TOR/d" $(1); \
		echo "TCONFIG_TOR=y" >>$(1); \
	fi; \
	if [ "$(CTF)" = "y" ]; then \
		sed -i "/TCONFIG_CTF/d" $(1); \
		echo "TCONFIG_CTF=y" >>$(1); \
	fi; \
	if [ "$(TINC)" = "y" ]; then \
		sed -i "/TCONFIG_TINC/d" $(1); \
		echo "TCONFIG_TINC=y" >>$(1); \
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
	if [ "$(CONFIG_LINUX26)" = "y" ] && [ "$(MIPS32)" = "r2" ]; then \
		sed -i "/CONFIG_CPU_MIPS32_R1/d" $(1); \
		echo "# CONFIG_CPU_MIPS32_R1 is not set" >>$(1); \
		sed -i "/CONFIG_CPU_MIPS32_R2/d" $(1); \
		echo "CONFIG_CPU_MIPS32_R2=y" >>$(1); \
		sed -i "/CONFIG_CPU_MIPSR1/d" $(1); \
		echo "CONFIG_CPU_MIPSR2=y" >>$(1); \
	fi; \
	if [ "$(UPS)" = "y" ]; then \
		sed -i "/CONFIG_INPUT=m/d" $(1); \
		echo "CONFIG_INPUT=y" >>$(1); \
		sed -i "/CONFIG_USB=m/d" $(1); \
		echo "CONFIG_USB=y" >>$(1); \
		sed -i "/CONFIG_HID=m/d" $(1); \
		echo "CONFIG_HID=y" >>$(1); \
		sed -i "/CONFIG_USB_HID=m/d" $(1); \
		echo "CONFIG_USB_HID=y" >>$(1); \
		sed -i "/CONFIG_USB_HIDDEV=m/d" $(1); \
		echo "CONFIG_USB_HIDDEV=y" >>$(1); \
		sed -i "/CONFIG_USB_DEVICEFS=m/d" $(1); \
		echo "CONFIG_USB_DEVICEFS=y" >>$(1); \
	fi; \
	if [ "$(USB)" = "" ]; then \
		sed -i "/CONFIG_EFI_PARTITION/d" $(1); \
		echo "# CONFIG_EFI_PARTITION is not set" >>$(1); \
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
			sed -i "/CONFIG_IP6_NF_RAW/d" $(1); \
			echo "CONFIG_IP6_NF_RAW=m" >>$(1); \
			sed -i "/CONFIG_IPV6_OPTIMISTIC_DAD/d" $(1); \
			echo "CONFIG_IPV6_OPTIMISTIC_DAD=y" >>$(1); \
			sed -i "/CONFIG_IPV6_MROUTE/d" $(1); \
			echo "CONFIG_IPV6_MROUTE=y" >>$(1); \
			sed -i "/CONFIG_IP6_NF_TARGET_ROUTE/d" $(1); \
			echo "CONFIG_IP6_NF_TARGET_ROUTE=m" >>$(1); \
			sed -i "/CONFIG_INET6_XFRM_TUNNEL/d" $(1); \
			echo "CONFIG_INET6_XFRM_TUNNEL=m" >>$(1); \
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
	sed -i "/CONFIG_BCM57XX/d" $(1); \
	if [ "$(BCM57)" = "y" ]; then \
		echo "CONFIG_BCM57XX=m" >>$(1); \
	else \
		echo "# CONFIG_BCM57XX is not set" >>$(1); \
	fi; \
	if [ "$(CONFIG_LINUX26)" = "y" ] && [ "$(EBTABLES)" = "y" ]; then \
		sed -i "/CONFIG_BRIDGE_NF_EBTABLES/d" $(1); \
		echo "CONFIG_BRIDGE_NF_EBTABLES=m" >>$(1); \
		if [ "$(IPV6SUPP)" = "y" ]; then \
			sed -i "/CONFIG_BRIDGE_EBT_IP6/d" $(1); \
			echo "CONFIG_BRIDGE_EBT_IP6=m" >>$(1); \
		fi; \
	fi; \
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

## mini-USB - K24 only
n:
ifneq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin BBEXTRAS=y USBEXTRAS=y B=E BUILD_DESC="mini-USB" USB="USB"
endif

## VPN
e:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y MULTIWAN=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="$(VPN)" USB="USB" DNSCRYPT=y PPTPD=y
else
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y B=E BUILD_DESC="$(VPN)" USB="USB" DNSCRYPT=y PPTPD=y
endif

## BTgui-VPN
c:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y OPENVPN=y BBEXTRAS=y USBEXTRAS=y MULTIWAN=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="BTgui-VPN" USB="USB" BTGUI=y DNSCRYPT=y PPTPD=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="BTgui" USB="USB" BTGUI=y DNSCRYPT=y PPTPD=y
endif

## small BTgui-VPN for Netgear 3500L
a:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y OPENVPN=y BBEXTRAS=y USBEXTRAS=y MULTIWAN=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="WNR3500L" USB="USB" BTGUI=y PPTPD=y NO_JFFS=y
endif

## Nocat-VPN
d:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MULTIWAN=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Nocat-VPN" USB="USB" BTGUI=y NOCAT=y OPENVPN=y DNSCRYPT=y PPTPD=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Nocat-VPN" USB="USB" BTGUI=y NOCAT=y OPENVPN=y DNSCRYPT=y PPTPD=y
endif

## TOR-VPN
g:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MULTIWAN=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Tor-VPN" USB="USB" BTGUI=y TOR=y OPENVPN=y PPTPD=y NO_CIFS=y NO_JFFS=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Tor-VPN" USB="USB" BTGUI=y TOR=y OPENVPN=y PPTPD=y NO_CIFS=y NO_JFFS=y
endif

## BT
u:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y MULTIWAN=y IPV6SUPP=y B=M BUILD_DESC="BT" USB="USB" BTCLIENT=y
else
	@$(MAKE) bin NTFS=y USBEXTRAS=y B=M BUILD_DESC="BT" USB="USB" BTCLIENT=y
endif

## BT-VPN
t:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y USBEXTRAS=y EBTABLES=y MULTIWAN=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="BT-VPN" USB="USB" BTCLIENT=y NO_JFFS=y PPTPD=y
else
	@$(MAKE) bin OPENVPN=y NTFS=y USBEXTRAS=y B=M BUILD_DESC="BT-VPN" USB="USB" BTCLIENT=y NO_JFFS=y PPTPD=y
endif

## Big-VPN
b:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MULTIWAN=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Big-VPN" USB="USB" BTGUI=y NFS=y NOCAT=y OPENVPN=y DNSCRYPT=y PPTPD=y NO_JFFS=y NO_CIFS=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Big-VPN" USB="USB" BTGUI=y NOCAT=y NFS=y OPENVPN=y DNSCRYPT=y PPTPD=y NO_JJFS=y NO_CIFS=y
endif

## Mega-VPN
o:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y MULTIWAN=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="Mega-VPN" USB="USB" NOCAT=y BTCLIENT=y DNSCRYPT=y SNMP=y UPS=y PPTPD=y TR_EXTRAS=y TOR=y IPSEC=y RAID=y DNSSEC=y TINC=y NANO=y

## All-In-One (AIO) MIPS1 - only for router with more than 8MB flash!! ZTE H618B
z:
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y MULTIWAN=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="AIO" USB="USB" NOCAT=y BTCLIENT=y TR_EXTRAS=y NFS=y SNMP=y HFS=y DNSCRYPT=y UPS=y PPTPD=y TOR=y IPSEC=y RAID=y NGINX=y DNSSEC=y TINC=y NANO=y
endif

#################################

ifeq ($(CONFIG_LINUX26),y)

## Mini - targeted for Netgear routers
f:
	@$(MAKE) bin JFFSv1=y NO_CIFS=y NO_ZEBRA=y B=F BUILD_DESC="Mini" USB="" MULTIWAN=y

## MiniIPv6 - for 4MB routers
i:
	@$(MAKE) bin NO_JFFS=y NO_CIFS=y NO_ZEBRA=y IPV6SUPP=y B=I BUILD_DESC="MiniIPv6" USB="" NO_HTTPS=y MULTIWAN=y

## MiniVPN - for 4MB routers
j:
	@$(MAKE) bin NO_JFFS=y NO_CIFS=y NO_ZEBRA=y B=V BUILD_DESC="MiniVPN" USB="" PPTPD=y OPENVPN=y MULTIWAN=y
endif

## VPN with no USB support (IPv6 in K26 build)
v:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NO_JFFS=y B=V BUILD_DESC="IPv6-$(VPN)" USB="" PPTPD=y IPV6SUPP=y MULTIWAN=y
else
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y BCM57=y B=V BUILD_DESC="$(VPN)" USB="" PPTPD=y
endif

## Max build for router with 8MB flash without USB Support (like WRT54GS v1)
m:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y EBTABLES=y IPV6SUPP=y B=E BUILD_DESC="Max" USB="" NOCAT=y NFS=y SNMP=y DNSCRYPT=y PPTPD=y TOR=y IPSEC=y  MULTIWAN=y
else
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y B=E BUILD_DESC="Max" USB="" NOCAT=y NFS=y SNMP=y DNSCRYPT=y PPTPD=y TOR=y
endif

## SD-VPN - Only k24
w:
ifneq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y BCM57=y B=V BUILD_DESC="SD-VPN" USB="" SDHC=y DNSCRYPT=y PPTPD=y
endif


## Make Mips32r2 builds ##

#AIO MIPS2 for RT-N16 and E4200
r2z:
	@$(MAKE) z MIPS32=r2

r2e:
	@$(MAKE) e MIPS32=r2

r2a:
	@$(MAKE) a MIPS32=r2

r2c:
	@$(MAKE) c MIPS32=r2

r2u:
	@$(MAKE) u MIPS32=r2

r2t:
	@$(MAKE) t MIPS32=r2

r2d:
	@$(MAKE) d MIPS32=r2

r2g:
	@$(MAKE) g MIPS32=r2

r2b:
	@$(MAKE) b MIPS32=r2

r2o:
	@$(MAKE) o MIPS32=r2

r2f:
	@$(MAKE) f MIPS32=r2 BELKIN=y

r2i:
	@$(MAKE) i MIPS32=r2

r2j:
	@$(MAKE) j MIPS32=r2

r2v:
	@$(MAKE) v MIPS32=r2

r2m:
	@$(MAKE) m MIPS32=r2

#AIO for E4200
n60z:
	@$(MAKE) r2z NVRAM_SIZE=60 LINKSYS_E=1

n60e:
	@$(MAKE) r2e NVRAM_SIZE=60 LINKSYS_E=1

n60c:
	@$(MAKE) r2c NVRAM_SIZE=60 LINKSYS_E=1

n60u:
	@$(MAKE) r2u NVRAM_SIZE=60 LINKSYS_E=1

n60t:
	@$(MAKE) r2t NVRAM_SIZE=60 LINKSYS_E=1

n60b:
	@$(MAKE) r2b NVRAM_SIZE=60 LINKSYS_E=1

n60d:
	@$(MAKE) r2d NVRAM_SIZE=60 LINKSYS_E=1

n60g:
	@$(MAKE) r2g NVRAM_SIZE=60 LINKSYS_E=1

n60o:
	@$(MAKE) r2o NVRAM_SIZE=60 LINKSYS_E=1

n60v:
	@$(MAKE) r2v NVRAM_SIZE=60 LINKSYS_E=1

n60m:
	@$(MAKE) r2m NVRAM_SIZE=60 LINKSYS_E=1

setprofile:
	echo '#ifndef TOMATO_PROFILE' > router/shared/tomato_profile.h
	echo '#define TOMATO_$(N) 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_G 1' >> router/shared/tomato_profile.h
	echo '#define PROFILE_N 2' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE PROFILE_$(N)' >> router/shared/tomato_profile.h
	echo '#define TOMATO_PROFILE_NAME "$(N)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_NAME "$(B)"' >> router/shared/tomato_profile.h
	echo '#define TOMATO_BUILD_DESC "$(DESC)"' >> router/shared/tomato_profile.h
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
	echo 'TOMATO_BUILD_DESC = "$(DESC)"' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_L = $(lowercase_N)' >> tomato_profile.mak
	echo 'TOMATO_PROFILE_U = $(uppercase_N)' >> tomato_profile.mak
	echo 'TOMATO_BUILD_USB = "$(USB)"' >> tomato_profile.mak

	echo 'export EXTRACFLAGS := $(EXTRA_CFLAGS) -DBCMWPA2 -funit-at-a-time -Wno-pointer-sign -mtune=mips32 -mips32 $(if $(filter $(NVRAM_SIZE),0),,-DCONFIG_NVRAM_SIZE=$(NVRAM_SIZE))' >> tomato_profile.mak

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
	@echo "n            Mini-USB - for oruter with 4MB flash and USB port - K24 only"
	@echo "e            VPN - (standard plus VPN, extra utilities and NTFS support)"
	@echo "c            BTgui-VPN - (Ext plus BT gui)"
	@echo "d            Nocat-VPN - (BTgui plus VPN plus Nocat)"
	@echo "g            Tor-VPN - (BTgui plus VPN plus TOR Project)"
	@echo "t            BT-VPN - (BT plus VPN)"
	@echo "b            Big-VPN - (Big plus VPN)"
	@echo "o            Mega-VPN - (Mega plus VPN plus NOCAT minus NFS)"
ifeq ($(CONFIG_LINUX26),y)
	@echo "z            MIPS Release 15F AIO (for routers +8MB flash)"
	@echo "i            MiniIPv6 - (IPv6 with no USB support minus CIFS and RIPv1/2)"
	@echo "j            MiniVPN - (VPN with no USB support minus CIFS and RIPv1/2)"
endif
	@echo "f            Mini - (no USB support minus CIFS and RIPv1/2)"
	@echo "v            VPN (no usb) - (VPN with no USB support)" / in K26 + IPv6 support
	@echo "m            Max (no usb) - (Max with no USB support)" / in K26 + IPv6 support
	@echo "w            SD-VPN (no usb) - (VPN with SD-MOD and no USB support)"
ifeq ($(CONFIG_LINUX26),y)
	@echo "r2e          MIPS Release 2 VPN"
	@echo "r2c          MIPS Release 2 BTgui-VPN"
	@echo "r2d          MIPS Release 2 Nocat-VPN"
	@echo "r2g          MIPS Release 2 Tor-VPN"
	@echo "r2t          MIPS Release 2 BT-VPN"
	@echo "r2b          MIPS Release 2 Big-VPN"
	@echo "r2o          MIPS Release 2 Mega-VPN (for routers +8MB flash)"
	@echo "r2z          MIPS Release 2 AIO (for routers +8MB flash)"
	@echo "r2v          MIPS Release 2 VPN (no usb)"
	@echo "r2m          MIPS Release 2 Max (no usb)"
	@echo "r2i          MIPS Release 2 MiniIPv6 (for 4MB flash)"
	@echo "r2j          MIPS Release 2 MiniVPN (for 4MB flash)"
	@echo "r2f          MIPS Release 2 Mini (for netgear)"
	@echo "n60e         Linksys E-series build VPN"
	@echo "n60c         Linksys E-series build BTGui-VPN"
	@echo "n60d         Linksys E-series build BTGui-VPN plus Nocat"
	@echo "n60g         Linksys E-series build BTGui-VPN plus TOR Project"
	@echo "n60t         Linksys E-series build BT-VPN"
	@echo "n60b         Linksys E-series build Big-VPN"
	@echo "n60o         Linksys E-series build Mega-VPN"
	@echo "n60z         Linksys E-series build AIO"
	@echo "n60v         Linksys E-series build IPv6-VPN for E2000"
	@echo "n60m         Linksys E-series build Max for E2000"
endif
	@echo "..etc..      other build configs"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C Linux distclean (but preserves .config)"
	@echo "distclean    distclean of Linux & busybox (but preserve .configs)"
	@echo "prepk        -C Linux oldconfig dep"
	
.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile help
.PHONY: a b c d m nc Makefile allversions tomato_profile.mak
