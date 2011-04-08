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
#  make V1=whatever V2=sub-whatever VPN=vpn3.6 a b c d m
# The 1st "whatever" would be the build number, the sub-whatever would
#	be the update to the version.
#
# Example:
# make V1=8516 V2="-jffs.1" a b c d m s

# !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



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

	@rm -f image/tomato-$(ND)$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx
	@rm -f image/tomato-$(ND)$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
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
ifeq ($(NVRAM_SIZE),60)
ifeq ($(USB),USB)
	# Linksys E3000 build
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l 61XN,image/tomato-$(ND)$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
else
	# Linksys E2000 build
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l 32XN,image/tomato-$(ND)$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
else
	# Create generic TRX image
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-t image/tomato-$(ND)$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
		| tee -a fpkg.log
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

# Options for "make bin" :
# BUILD_DESC (Std|Lite|Ext|...)
# MIPS32 (r2|r1)
# KERN_SIZE_OPT
# USB ("USB"|"")
# JFFSv1 | NO_JFFS
# NO_CIFS, NO_SSH, NO_ZEBRA, NO_SAMBA, NO_HTTPS, NO_XXTP, NO_LIBOPT
# SAMBA3, OPENVPN, IPV6SUPP, EBTABLES, NTFS, MEDIASRV, BBEXTRAS, USBEXTRAS, BCM57, SLIM

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
	fi; \
	if [ "$(NO_CIFS)" = "y" ]; then \
		sed -i "/CONFIG_FEATURE_MOUNT_CIFS/d" $(1); \
		echo "# CONFIG_FEATURE_MOUNT_CIFS is not set" >>$(1); \
	fi; \
	if [ "$(BBEXTRAS)" = "y" ]; then \
		sed -i "/CONFIG_SENDMAIL/d" $(1); \
		echo "CONFIG_SENDMAIL=y" >>$(1); \
		sed -i "/CONFIG_FEATURE_SORT_BIG/d" $(1); \
		echo "CONFIG_FEATURE_SORT_BIG=y" >>$(1); \
		sed -i "/CONFIG_FTPPUT/d" $(1); \
		echo "CONFIG_FTPPUT=y" >>$(1); \
		sed -i "/CONFIG_FTPGET/d" $(1); \
		echo "CONFIG_FTPGET=y" >>$(1); \
		sed -i "/CONFIG_CLEAR/d" $(1); \
		echo "CONFIG_CLEAR=y" >>$(1); \
		sed -i "/CONFIG_NICE/d" $(1); \
		echo "CONFIG_NICE=y" >>$(1); \
		sed -i "/CONFIG_SETCONSOLE/d" $(1); \
		echo "CONFIG_SETCONSOLE=y" >>$(1); \
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
			sed -i "/CONFIG_UNZIP/d" $(1); \
			echo "CONFIG_UNZIP=y" >>$(1); \
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

## Make the "normal" build
a:
	@$(MAKE) bin B=A BUILD_DESC="Std" USB="USB"

## Make the "normal but without SSH" build
b:
	@$(MAKE) bin NO_SSH=y B=B BUILD_DESC="No SSH" USB="USB"

## Make the "normal but without CIFS" build
c:
	@$(MAKE) bin NO_CIFS=y B=C BUILD_DESC="No CIFS" USB="USB"

## Make the "normal but without SAMBA" (Lite) build
d:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NO_SAMBA=y SLIM=y NO_CIFS=y NO_JFFS=y NO_ZEBRA=y B=D BUILD_DESC="Lite" USB="USB"
else
	@$(MAKE) bin NO_SAMBA=y SLIM=y B=D BUILD_DESC="Lite" USB="USB"
endif

## Make the "normal (with NTFS support) + VPN" build
e:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y NO_LIBOPT=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="$(VPN)" USB="USB"
else
	@$(MAKE) bin OPENVPN=y NTFS=y B=E BUILD_DESC="$(VPN)" USB="USB"
endif

## Make the "normal with Extras and NTFS" build
m:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y NO_LIBOPT=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Ext" USB="USB"
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Ext" USB="USB"
endif

## Make the "No USB" build
s:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin JFFSv1=y $(if $(filter r2, $(MIPS32)),,BCM57=y) B=S BUILD_DESC="Std" USB=""
else
	@$(MAKE) bin BBEXTRAS=y BCM57=y B=S BUILD_DESC="Std" USB=""
endif

## Make the "Mini: no USB, no CIFS, no Zebra" build - targeted for Netgear routers
f:
	@$(MAKE) bin JFFSv1=y NO_CIFS=y NO_ZEBRA=y SLIM=y B=F BUILD_DESC="Mini" USB=""

ifeq ($(CONFIG_LINUX26),y)
## Make the "No USB with IPv6, no Zebra" build - for 4MB routers
i:
	@$(MAKE) bin JFFSv1=y NO_CIFS=y NO_ZEBRA=y IPV6SUPP=y B=I BUILD_DESC="MiniIPv6" USB=""
endif

## Make the "VPN witn no USB support" build
v:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NO_JFFS=y B=V BUILD_DESC="$(VPN)" USB=""
else
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y BCM57=y B=V BUILD_DESC="$(VPN)" USB=""
endif

## Make Mips32r2 builds
r2m:
	@$(MAKE) m MIPS32=r2

r2e:
	@$(MAKE) e MIPS32=r2

r2s:
	@$(MAKE) s MIPS32=r2

r2v:
	@$(MAKE) v MIPS32=r2

r2i:
	@$(MAKE) i MIPS32=r2

r2f:
	@$(MAKE) f MIPS32=r2

r2d:
	@$(MAKE) d MIPS32=r2

e3000:
	@$(MAKE) r2m ND=E3000 NVRAM_SIZE=60

e3000v:
	@$(MAKE) r2e ND=E3000 NVRAM_SIZE=60

e2000:
	@$(MAKE) bin MIPS32=r2 B=S USB="" ND=E2000 NVRAM_SIZE=60 BBEXTRAS=y EBTABLES=y NO_LIBOPT=y IPV6SUPP=y BUILD_DESC="Std"

e2000v:
	@$(MAKE) bin MIPS32=r2 B=V USB="" ND=E2000 NVRAM_SIZE=60 BBEXTRAS=y EBTABLES=y NO_LIBOPT=y IPV6SUPP=y OPENVPN=y BUILD_DESC="$(VPN)"

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
	@echo "a            A build (standard)"
	@echo "b            B build (standard minus SSH)"
	@echo "c            C build (standard minus CIFS)"
	@echo "d            D build (standard minus Samba server)"
	@echo "m            M build (standard plus extra utilities and NTFS support)"
ifeq ($(CONFIG_LINUX26),y)
	@echo "e            E build (standard plus VPN, extra utilities and NTFS support)"
	@echo "i            I build (IPv6 with no USB support minus CIFS and RIPv1/2)"
else
	@echo "e            E build (standard plus VPN and NTFS support)"
endif
	@echo "s            S build (no USB support)"
	@echo "f            F build (no USB support minus CIFS and RIPv1/2)"
	@echo "v            V build (VPN with no USB support)"
ifeq ($(CONFIG_LINUX26),y)
	@echo "r2m          MIPS Release 2 M build (standard plus extras)"
	@echo "r2e          MIPS Release 2 E build (standard plus VPN and extras)"
	@echo "r2v          MIPS Release 2 V build (VPN with no USB support)"
	@echo "r2s          MIPS Release 2 S build (no USB support)"
	@echo "r2i          MIPS Release 2 I build (IPv6 with no USB support minus CIFS and RIPv1/2)"
	@echo "r2f          MIPS Release 2 F build (no USB support minus CIFS and RIPv1/2)"
	@echo "e3000        Linksys E3000 build (standard plus extras)"
	@echo "e3000v       Linksys E3000 build (standard plus VPN and extras)"
	@echo "e2000        Linksys E2000 build (no USB support)"
	@echo "e2000v       Linksys E2000 build (VPN with no USB support)"
endif
	@echo "..etc..      other build configs"
	@echo "clean        -C router clean"
	@echo "cleanimage   rm -rf image"
	@echo "cleantools   clean btools, mksquashfs"
	@echo "cleankernel  -C Linux distclean (but preserves .config)"
	@echo "distclean    distclean of Linux & busybox (but preserve .configs)"
	@echo "prepk        -C Linux oldconfig dep"
	
.PHONY: all clean distclean cleanimage cleantools cleankernel prepk what setprofile help
.PHONY: a b c d m Makefile allversions tomato_profile.mak
