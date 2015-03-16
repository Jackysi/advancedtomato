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

# Normally you'd do something like "make V1=8516 V2=-sub-ver a b c d"
# But if you don't give anything for "V1", it'll get a default from tomato_version.
V1 ?= "--def"
VPN ?= "VPN"
ifeq ($(CONFIG_LINUX26),y)
ifeq ($(CONFIG_BCMWL6),y)
ND = "K26AC"
else
ND = "K26"
endif
else
ND = "ND"
endif

PPTPD ?= "n"

ifeq ($(NVRAM_SIZE),)
NVRAM_SIZE = 0
endif

ifeq ($(ASUS_TRX),)
ASUS_TRX = 0
endif


ifeq ($(WNR3500LV2),)
WNR3500LV2 = 0
else
WNRTOOL=$(SRCBASE)/wnrtool
BOARD_FILE=$(WNRTOOL)/comp-wnr3500lv2.txt
fw_cfg_file := $(WNRTOOL)/ambitCfg-wnr3500lv2.h
endif

ifeq ($(NVRAM_64K),y)
EXTRA_64KDESC = ' -64K'
EXTRA_64KCFLAG = '-DTCONFIG_NVRAM_64K'
else
EXTRA_64KDESC = ''
EXTRA_64KCFLAG = ''
endif

-include tomato_profile.mak

# This could be simpler by just using $(TOMATO_PROFILE_NAME) like it used to be,
# but that's fragile.  If you make one certain innocuous change elsewhere in the
# makefile(s), the build will silently be wrong.  This way it works properly every time.
current_BUILD_NAME = $(strip $(shell grep "^TOMATO_BUILD_NAME" tomato_profile.mak  | cut -d"=" -f2))
current_BUILD_DESC = $(strip $(shell grep "^TOMATO_BUILD_DESC" tomato_profile.mak  | cut -d"=" -f2 | sed -e "s/ //g"))
current_BUILD_USB  = $(strip $(shell grep "^TOMATO_BUILD_USB"  tomato_profile.mak  | cut -d"=" -f2 | sed -e "s/ //g"))
current_TOMATO_VER = $(strip $(shell grep "TOMATO_MAJOR" router/shared/tomato_version.h  | cut -d"\"" -f2)).$(strip $(shell grep "TOMATO_MINOR" router/shared/tomato_version.h  | cut -d"\"" -f2))
current_TOMATO_V1 = $(V1)
current_TOMATO_V2 = $(V2)

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

ifneq ($(ASUS_TRX),0)
 ifeq ($(ASUS_TRX),R6300V1)
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/mipsel-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	# For mkchkimg, have to redirect stderr to stdout ... for some reason mkchkimg outputs to stderr (confirmed in source code!), 
	# and tee only reads from stdout (not stderr)
	@echo "*********************** Convert TRX to CHK (add Netgear Checksum) ************************" >>fpkg.log
	@echo "Creating Firmware for Netgear R6300 v1 .... "
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-Netgear-R6300V1_$(V1)$(V2)-$(current_BUILD_DESC).chk \
	-k image/linux-lzma.trx \
	-b U12H218T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@rm -f image/linux-lzma.trx
	@echo ""
 else
  ifeq ($(ASUS_TRX),WNDR4500V1)
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/mipsel-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	# For mkchkimg, have to redirect stderr to stdout ... for some reason mkchkimg outputs to stderr (confirmed in source code!), 
	# and tee only reads from stdout (not stderr)
	@echo "*********************** Convert TRX to CHK (add Netgear Checksum) ************************" >>fpkg.log
	@echo "Creating Firmware for Netgear WNDR4500 v1 .... "
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-Netgear-WNDR4500V1_$(V1)$(V2)-$(current_BUILD_DESC).chk \
	-k image/linux-lzma.trx \
	-b U12H189T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@rm -f image/linux-lzma.trx
	@echo ""
  else
   ifeq ($(ASUS_TRX),WNDR4500V2)
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/mipsel-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	# For mkchkimg, have to redirect stderr to stdout ... for some reason mkchkimg outputs to stderr (confirmed in source code!), 
	# and tee only reads from stdout (not stderr)
	@echo "*********************** Convert TRX to CHK (add Netgear Checksum) ************************" >>fpkg.log
	@echo "Creating Firmware for Netgear WNDR4500 v2 .... "
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-Netgear-WNDR4500V2_$(V1)$(V2)-$(current_BUILD_DESC).chk \
	-k image/linux-lzma.trx \
	-b U12H224T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@rm -f image/linux-lzma.trx
	@echo ""
   else
	$(MAKE) -C ctools
	ctools/objcopy -O binary -R .reginfo -R .note -R .comment -R .mdebug -S $(LINUXDIR)/vmlinux ctools/piggy
	ctools/lzma_4k e ctools/piggy  ctools/vmlinuz-lzma
	ctools/mksquashfs router/mipsel-uclibc/target ctools/target.squashfs -noappend -all-root
	ctools/trx -o image/linux-lzma.trx ctools/vmlinuz-lzma ctools/target.squashfs
	ctools/trx_asus -i image/linux-lzma.trx -r $(BUILD_NAME),$(KERNEL_VER).$(FS_VER),image/tomato-$(ASUS_TRX)_$(V1)$(V2)-$(current_BUILD_DESC).trx
	@rm -f image/linux-lzma.trx
	@echo ""
   endif #WNDR4500V2
  endif  #WNDR4500V1
 endif   #R6300v1
endif    #ASUS_TRX

ifeq ($(WNR3500LV2),1)
	@echo "Creating Firmware for Netgear WNR3500L v2 .... "
	mipsel-uclibc-objcopy -O binary -g $(LINUXDIR)/vmlinux image/vmlinux.bin
	$(WNRTOOL)/lzma e image/vmlinux.bin image/vmlinux.lzma
	$(WNRTOOL)/trx -o image/tomato-wnr3500lv2.trx image/vmlinux.lzma $(SRCBASE)/router/mipsel-uclibc/target.image
	rm -f image/vmlinux.bin image/vmlinux.lzma
	cd image && touch rootfs
	cd image && $(WNRTOOL)/packet -k tomato-wnr3500lv2.trx -f rootfs -b $(BOARD_FILE) -ok kernel_image -oall kernel_rootfs_image -or rootfs_image -i $(fw_cfg_file) && rm -f rootfs && \
	cp kernel_rootfs_image.chk tomato-Netgear-3500Lv2-$(ND)$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(V2)-$(current_BUILD_DESC).chk
	@echo "Cleanup ...."
#	rm -f image/*image.chk image/*.trx
endif

ifeq ($(WRT54),y)
ifneq ($(MIPS32),r2)
	@rm -f image/tomato-WRT54*$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
	@rm -f image/tomato-WRTSL54*$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
	@rm -f image/tomato-WR850G$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
endif
ifeq ($(LINKSYS_E),y)
	@rm -f image/tomato-E??????$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
ifeq ($(LINKSYS_E_64k),y)
	@rm -f image/tomato-E??????$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
ifeq ($(LINKSYS_E1200v1),y)
	@rm -f image/tomato-E1200v1$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
ifeq ($(LINKSYS_E1000v2),y)
	@rm -f image/tomato-E1000v2-v21$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
	@rm -f image/tomato-Cisco-M10v2$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
ifeq ($(LINKSYS_E2500),y)
	@rm -f image/tomato-E2500$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
endif
ifeq ($(LINKSYS_E3200),y)
	@rm -f image/tomato-E3200$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
	@rm -f image/tomato-E2500v3$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
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
ifeq ($(WRT54),y)
ifneq ($(MIPS32),r2)
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l W54G,image/tomato-WRT54G_WRT54GL$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
		-l W54S,image/tomato-WRT54GS$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
		-l W54s,image/tomato-WRT54GSv4$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
		-l W54U,image/tomato-WRTSL54GS$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
		-m 0x10577050,image/tomato-WR850G$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin
		| tee -a fpkg.log
endif
endif
ifeq ($(LINKSYS_E),y)
	# Linksys E-series(60k Nvram) images
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l 1550,image/tomato-E1550$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l 4200,image/tomato-E4200$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l 61XN,image/tomato-E3000$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l 32XN,image/tomato-E2000$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
ifeq ($(LINKSYS_E2500),y)
	# Linksys E2500(60k Nvram) image
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l E25X,image/tomato-E2500$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
ifeq ($(LINKSYS_E3200),y)
	# Linksys E3200/E2500v3(60k Nvram) image
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l 25V3,image/tomato-E2500v3$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l 3200,image/tomato-E3200$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
ifeq ($(LINKSYS_E_64k),y)
	# Linksys E-series(64k Nvram) images
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l E800,image/tomato-E800$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l E900,image/tomato-E900$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l E122,image/tomato-E1200v2$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l E150,image/tomato-E1500$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
ifeq ($(LINKSYS_E1000v2),y)
	# Linksys E1000v2/v2.1 images
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l E100,image/tomato-E1000v2-v21$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-l M010,image/tomato-Cisco-M10v2$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
ifeq ($(LINKSYS_E1200v1),y)
	# Linksys E1200v1 images
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-l E120,image/tomato-E1200v1$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
ifeq ($(BELKIN),y)
ifneq ($(NVRAM_SIZE),60)
	# Create Belkin images
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-b 0x20100322,image/tomato-F7D3301$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-b 0x20090928,image/tomato-F7D3302$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-b 0x20091006,image/tomato-F7D4302$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		-b 0x00017116,image/tomato-F5D8235v3$(current_BUILD_USB)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).bin \
		| tee -a fpkg.log
endif
endif
ifeq ($(WNDR),y)
	@echo "Creating Firmware for Netgear WNDR Routers ..."
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
	-t image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
	| tee -a fpkg.log
	# For mkchkimg, have to redirect stderr to stdout ... for some reason mkchkimg outputs to stderr (confirmed in source code!), 
	# and tee only reads from stdout (not stderr)
	@echo "*********************** Convert TRX to CHK (add Netgear Checksum) ************************" >>fpkg.log
	# Make multiple versions / files, as file is HW specific (HW information is captured in the .chk file itself!)
	# Make WNDR4000, Checksum starts at 0x6FFFF8 => Max size (to not touch the last 64kB block) = 7274496
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-WNDR4000-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk \
	-k image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
	-b U12H181T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@$(MAKE) netgear-check MAXFSIZE=7274496 NG_FNAME=image/tomato-Netgear-WNDR4000-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk

	# Make WNDR3700v3, Checksum starts at 0x6FFFF8 => Max size (to not touch the last 64kB block) = 7274496
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-WNDR3700v3-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk \
	-k image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
	-b U12H194T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@$(MAKE) netgear-check MAXFSIZE=7274496 NG_FNAME=image/tomato-Netgear-WNDR3700v3-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk

	# Make WNDR3400, Checksum starts at 0x6CFFF8 => Max size (to not touch the last 64kB block) = 7077888
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-WNDR3400-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk \
	-k image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
	-b U12H155T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@$(MAKE) netgear-check MAXFSIZE=7077888 NG_FNAME=image/tomato-Netgear-WNDR3400-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk
	
	# Make WNDR3400v2, Checksum starts at 0x6FFFF8 => Max size (to not touch the last 64kB block) = 7274496
	@$(SRCBASE)/wnrtool/mkchkimg -o image/tomato-WNDR3400v2-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk \
	-k image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
	-b U12H187T00_NETGEAR -r 2 2>&1 | tee -a fpkg.log
	@$(MAKE) netgear-check MAXFSIZE=7274496 NG_FNAME=image/tomato-Netgear-WNDR3400v2-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).chk

	@echo "Cleanup ...."
	@cp fpkg.log image/fpkg-$(V1)$(mips_rev)$(beta)$(V2).log
	@rm -f image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx
	@rm -f fpkg.log
else
	# Create generic TRX image
	@btools/fpkg -i lzma-loader/loader.gz -i $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz -a 1024 -i router/mipsel-uclibc/target.image \
		-t image/tomato-$(ND)$(current_BUILD_USB)$(if $(filter $(NVRAM_SIZE),0),,-NVRAM$(NVRAM_SIZE)K)-$(current_TOMATO_VER).$(V1)$(mips_rev)$(beta)$(V2)-$(current_BUILD_DESC).trx \
		| tee -a fpkg.log

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
endif

netgear-check:
	$(eval FSIZE = $(shell stat -c %s $(NG_FNAME)))
	@if [ ${FSIZE} -gt ${MAXFSIZE} ] ; then \
		echo "Router Filesize exceeds Netgear Hardware limits - file will be deleted!"; \
		echo "   -> File to be deleted: " $(NG_FNAME); \
		rm $(NG_FNAME); \
		echo; \
	else \
		echo "Router Filesize meets Netgear Hardware limits - no action required."; \
		echo; \
	fi

clean:
	@touch router/.config
	@rm -f router/config_[a-z]
	@rm -f router/busybox/config_[a-z]
	@$(MAKE) -C router $@
	@rm -rf router/mipsel-uclibc

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
# WNR3500LV2 (0|1)
# KERN_SIZE_OPT
# USB ("USB"|"")
# JFFSv1 | NO_JFFS
# NO_CIFS, NO_SSH, NO_ZEBRA, NO_SAMBA, NO_HTTPS, NO_XXTP, NO_LIBOPT
# SAMBA3, OPENVPN, IPV6SUPP, EBTABLES, NTFS, UFSD, MEDIASRV, BBEXTRAS, USBEXTRAS, BCM57, SLIM, NOCAT CTF NGINX
# NFS BTCLIENT BTGUI TR_EXTRAS SNMP SDHC HFS UPS DNSCRYPT PPTPD TOR IPSEC RAID MICROSD USBAP NO_USBAPP

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
		if [ "$(UFSD)" = "y" ]; then \
			sed -i "/TCONFIG_UFSD/d" $(1); \
			echo "TCONFIG_UFSD=y" >>$(1); \
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
	if [ "$(ASUS_TRX)" = "R6300V1" ]; then \
		sed -i "/TCONFIG_R6300V1/d" $(1); \
		echo "TCONFIG_R6300V1=y" >>$(1); \
	fi; \
	if [ "$(ASUS_TRX)" = "WNDR4500V1" ]; then \
		sed -i "/TCONFIG_WNDR4500V1/d" $(1); \
		echo "TCONFIG_WNDR4500V1=y" >>$(1); \
	fi; \
	if [ "$(ASUS_TRX)" = "WNDR4500V2" ]; then \
		sed -i "/TCONFIG_WNDR4500V2/d" $(1); \
		echo "TCONFIG_WNDR4500V2=y" >>$(1); \
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
		if [ "$(NAND)" = "y" ]; then \
			sed -i "/CONFIG_MTD_NFLASH/d" $(1); \
			echo "CONFIG_MTD_NFLASH=y" >>$(1); \
			sed -i "/CONFIG_MTD_NAND/d" $(1); \
			echo "CONFIG_MTD_NAND=y" >>$(1); \
			echo "CONFIG_MTD_NAND_IDS=y" >>$(1); \
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
		if [ "$(WNR3500LV2)" = "1" ]; then \
			sed -i "/CONFIG_MTD_NFLASH/d" $(1); \
			echo "CONFIG_MTD_NFLASH=y" >>$(1); \
		fi; \
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

## mini-USB - K24 only
n:
ifneq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin BBEXTRAS=y USBEXTRAS=y B=E BUILD_DESC="mini-USB" USB="USB"
endif

## VPN
e:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="$(VPN)" USB="USB" DNSCRYPT=y PPTPD=y
else
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y B=E BUILD_DESC="$(VPN)" USB="USB" DNSCRYPT=y PPTPD=y
endif

## BTgui-VPN
c:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y OPENVPN=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="BTgui-VPN" USB="USB" BTGUI=y DNSCRYPT=y PPTPD=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="BTgui" USB="USB" BTGUI=y DNSCRYPT=y PPTPD=y
endif

## Nocat-VPN
d:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Nocat-VPN" USB="USB" BTGUI=y NOCAT=y OPENVPN=y DNSCRYPT=y PPTPD=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Nocat-VPN" USB="USB" BTGUI=y NOCAT=y OPENVPN=y DNSCRYPT=y PPTPD=y
endif

## TOR-VPN
g:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Tor-VPN" USB="USB" BTGUI=y TOR=y OPENVPN=y PPTPD=y NO_CIFS=y NO_JFFS=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Tor-VPN" USB="USB" BTGUI=y TOR=y OPENVPN=y PPTPD=y NO_CIFS=y NO_JFFS=y
endif

## BT
u:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="BT" USB="USB" BTCLIENT=y
else
	@$(MAKE) bin NTFS=y USBEXTRAS=y B=M BUILD_DESC="BT" USB="USB" BTCLIENT=y
endif

## BT-VPN
t:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="BT-VPN" USB="USB" BTCLIENT=y NO_JFFS=y PPTPD=y
else
	@$(MAKE) bin OPENVPN=y NTFS=y USBEXTRAS=y B=M BUILD_DESC="BT-VPN" USB="USB" BTCLIENT=y NO_JFFS=y PPTPD=y
endif

## Big-VPN
b:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=M BUILD_DESC="Big-VPN" USB="USB" BTGUI=y NFS=y NOCAT=y OPENVPN=y DNSCRYPT=y PPTPD=y NO_JFFS=y NO_CIFS=y
else
	@$(MAKE) bin NTFS=y BBEXTRAS=y USBEXTRAS=y B=M BUILD_DESC="Big-VPN" USB="USB" BTGUI=y NOCAT=y NFS=y OPENVPN=y DNSCRYPT=y PPTPD=y NO_JJFS=y NO_CIFS=y
endif

## Mega-VPN
o:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="Mega-VPN" USB="USB" NOCAT=y BTCLIENT=y DNSCRYPT=y SNMP=y UPS=y PPTPD=y TR_EXTRAS=y TOR=y IPSEC=y RAID=y

## All-In-One (AIO) MIPS1 - only for router with more than 8MB flash!! ZTE H618B
z:
	@$(MAKE) bin OPENVPN=y NTFS=y BBEXTRAS=y USBEXTRAS=y EBTABLES=y MEDIASRV=y IPV6SUPP=y B=E BUILD_DESC="AIO" USB="USB" NOCAT=y BTCLIENT=y TR_EXTRAS=y NFS=y SNMP=y HFS=y DNSCRYPT=y UPS=y PPTPD=y TOR=y IPSEC=y RAID=y NGINX=y DNSSEC=y TINC=y
endif

#################################

ifeq ($(CONFIG_LINUX26),y)

## Mini - targeted for Netgear routers
f:
	@$(MAKE) bin NO_JFFS=y NO_CIFS=y NO_ZEBRA=y B=F BUILD_DESC="Mini" USB=""

## MiniIPv6 - for 4MB routers
i:
	@$(MAKE) bin NO_JFFS=y NO_CIFS=y NO_ZEBRA=y IPV6SUPP=y B=I BUILD_DESC="MiniIPv6" USB=""

## MiniVPN - for 4MB routers
j:
	@$(MAKE) bin NO_JFFS=y NO_CIFS=y NO_ZEBRA=y B=V BUILD_DESC="MiniVPN" USB="" PPTPD=y OPENVPN=y
endif

## VPN with no USB support (IPv6 in K26 build)
v:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y NO_JFFS=y B=V BUILD_DESC="IPv6-$(VPN)" USB="" PPTPD=y IPV6SUPP=y
else
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y BCM57=y B=V BUILD_DESC="$(VPN)" USB="" PPTPD=y
endif

## Max build for router with 8MB flash without USB Support
m:
ifeq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y EBTABLES=y IPV6SUPP=y B=E BUILD_DESC="Max" USB="" NOCAT=y NFS=y SNMP=y DNSCRYPT=y PPTPD=y TOR=y IPSEC=y
endif

## SD-VPN - Only k24
w:
ifneq ($(CONFIG_LINUX26),y)
	@$(MAKE) bin OPENVPN=y BBEXTRAS=y BCM57=y B=V BUILD_DESC="SD-VPN" USB="" SDHC=y DNSCRYPT=y PPTPD=y
endif


## Make Mips32r2 builds ##
ifeq ($(CONFIG_LINUX26),y)

#AIO MIPS2 for RT-N16 and E4200
r2z:
	@$(MAKE) z MIPS32=r2

r2e:
	@$(MAKE) e MIPS32=r2

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

#Linksys E-series witk 60K NVRAM builds
n60z:
	@$(MAKE) r2z NVRAM_SIZE=60 LINKSYS_E=y

n60e:
	@$(MAKE) r2e NVRAM_SIZE=60 LINKSYS_E=y

n60c:
	@$(MAKE) r2c NVRAM_SIZE=60 LINKSYS_E=y

n60u:
	@$(MAKE) r2u NVRAM_SIZE=60 LINKSYS_E=y

n60t:
	@$(MAKE) r2t NVRAM_SIZE=60 LINKSYS_E=y

n60b:
	@$(MAKE) r2b NVRAM_SIZE=60 LINKSYS_E=y

n60d:
	@$(MAKE) r2d NVRAM_SIZE=60 LINKSYS_E=y

n60g:
	@$(MAKE) r2g NVRAM_SIZE=60 LINKSYS_E=y

n60o:
	@$(MAKE) r2o NVRAM_SIZE=60 LINKSYS_E=y

n60i:
	@$(MAKE) r2i NVRAM_SIZE=60 LINKSYS_E=y

n60j:
	@$(MAKE) r2j NVRAM_SIZE=60 LINKSYS_E=y

n60v:
	@$(MAKE) r2v NVRAM_SIZE=60 LINKSYS_E=y

n60m:
	@$(MAKE) r2m NVRAM_SIZE=60 LINKSYS_E=y

#Linksys E-Series with 64K Nvram Builds

n64z:
	@$(MAKE) r2z NVRAM_SIZE=64 LINKSYS_E_64k=y

n64e:
	@$(MAKE) r2e NVRAM_SIZE=64 LINKSYS_E_64k=y

n64c:
	@$(MAKE) r2c NVRAM_SIZE=64 LINKSYS_E_64k=y

n64u:
	@$(MAKE) r2u NVRAM_SIZE=64 LINKSYS_E_64k=y

n64t:
	@$(MAKE) r2t NVRAM_SIZE=64 LINKSYS_E_64k=y

n64b:
	@$(MAKE) r2b NVRAM_SIZE=64 LINKSYS_E_64k=y

n64d:
	@$(MAKE) r2d NVRAM_SIZE=64 LINKSYS_E_64k=y

n64g:
	@$(MAKE) r2g NVRAM_SIZE=64 LINKSYS_E_64k=y

n64o:
	@$(MAKE) r2o NVRAM_SIZE=64 LINKSYS_E_64k=y

n64v:
	@$(MAKE) r2v NVRAM_SIZE=64 LINKSYS_E_64k=y

n64m:
	@$(MAKE) r2m NVRAM_SIZE=64 LINKSYS_E_64k=y


#Linksys E-Series with 4MB Flash (Max Flashsize=3866624)

e1000v2f:
	@$(MAKE) bin MIPS32=r2 NVRAM_SIZE=32 LINKSYS_E1000v2=y NO_JFFS=y NO_ZEBRA=y B=F BUILD_DESC="Mini" USB="" NO_HTTPS=y KERN_SIZE_OPT=y

e1000v2i:
	@$(MAKE) bin MIPS32=r2 NVRAM_SIZE=32 LINKSYS_E1000v2=y NO_JFFS=y NO_CIFS=y NO_ZEBRA=y IPV6SUPP=y B=I USB="" NO_HTTPS=y KERN_SIZE_OPT=y BUILD_DESC="MiniIPv6"

e1200v1f:
	@$(MAKE) bin MIPS32=r2 NVRAM_SIZE=64 LINKSYS_E1200v1=y NO_JFFS=y NO_ZEBRA=y B=F BUILD_DESC="Mini" USB="" NO_HTTPS=y KERN_SIZE_OPT=y

e1200v1i:
	@$(MAKE) bin MIPS32=r2 NVRAM_SIZE=64 LINKSYS_E1200v1=y NO_JFFS=y NO_CIFS=y NO_ZEBRA=y IPV6SUPP=y B=I USB="" NO_HTTPS=y KERN_SIZE_OPT=y BUILD_DESC="MiniIPv6"

#Builds for WNR3500L v2
v2z:
	@$(MAKE) z MIPS32=r2 WNR3500LV2=1 NO_JFFS=y

v2e:
	@$(MAKE) e MIPS32=r2 WNR3500LV2=1 NO_JFFS=y

#Builds with wl_high module
rtn53:
	@$(MAKE) bin MIPS32=r2 OPENVPN=y BBEXTRAS=y EBTABLES=y NO_LIBOPT=y IPV6SUPP=y B=E BUILD_DESC="RT-N53" USB="USB" SNMP=y DNSCRYPT=y PPTPD=y TOR=y NOCAT=y USBAP=y NO_USBAPP=y

e2500:
	@$(MAKE) bin MIPS32=r2 OPENVPN=y BBEXTRAS=y EBTABLES=y NO_LIBOPT=y IPV6SUPP=y B=E BUILD_DESC="Max" USB="USB" SNMP=y DNSCRYPT=y PPTPD=y TOR=y NOCAT=y USBAP=y NO_USBAPP=y NVRAM_SIZE=60 LINKSYS_E2500=y

e3200:
	@$(MAKE) r2o USBAP=y NVRAM_SIZE=60 LINKSYS_E3200=y

wndr64c:
	@$(MAKE) r2c BBEXTRAS=y USBEXTRAS=y EBTABLES=y IPV6SUPP=y B=E BUILD_DESC="BTgui-VPN" USB="USB" NO_JFFS=y NTFS=y MEDIASRV=y USBAP=y NVRAM_SIZE=64

n6:
	@$(MAKE) bin MIPS32=r2 OPENVPN=y BBEXTRAS=y EBTABLES=y IPV6SUPP=y B=E BUILD_DESC="TendaN6" USB="USB" TOR=y NOCAT=y DNSCRYPT=y PPTPD=y USBAP=y NO_USBAPP=y

n60:
	@$(MAKE) bin MIPS32=r2 BBEXTRAS=y USBEXTRAS=y EBTABLES=y IPV6SUPP=y B=E BUILD_DESC="TendaN60" USB="USB" NO_JFFS=y NTFS=y MEDIASRV=y USBAP=y

n60init:
	@$(MAKE) bin MIPS32=r2 B=E BUILD_DESC="TendaN60-init" USB="" NO_JFFS=y NO_CIFS=y NO_HTTPS=y NO_XXTP=y NO_ZEBRA=y NO_SSH=y

# it`s not possible to flash tomato on this router. Maybe in the future.
#l600n:
#	@$(MAKE) bin MIPS32=r2 OPENVPN=y BBEXTRAS=y EBTABLES=y NO_LIBOPT=y IPV6SUPP=y B=E BUILD_DESC="L600N" USB="USB" SNMP=y DNSCRYPT=y PPTPD=y TOR=y NOCAT=y USBAP=y

#Builds for Asus RT-N66u with 64k NVRAM support
ifeq ($(CONFIG_BCMWL6),y)
r64z:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y MICROSD=y ASUS_TRX="RT-N66U" UFSD=y

r64e:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y MICROSD=y ASUS_TRX="RT-N66U" UFSD=y
else
r64z:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y MICROSD=y

r64e:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y MICROSD=y
endif

ifeq ($(CONFIG_BCMWL6),y)
ac66e:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="RT-AC66U" UFSD=y

ac66z:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="RT-AC66U" UFSD=y

w1800re:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y ASUS_TRX="W1800R"

w1800rz:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y ASUS_TRX="W1800R"

r6300e:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="R6300V1"

r6300z:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="R6300V1"

wndr4500e:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="WNDR4500V1"

wndr4500z:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="WNDR4500V1"

wndr4500v2e:
	@$(MAKE) e MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="WNDR4500V2"

wndr4500v2z:
	@$(MAKE) z MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="WNDR4500V2"

n80o:
	@$(MAKE) o MIPS32=r2 NVRAM_64K=y ASUS_TRX="ASUS_TRX="TDN80"

#d1800he:
#	@$(MAKE) e MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="D1800H"

#d1800hz:
#	@$(MAKE) z MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="D1800H"

#ea6500e:
#	@$(MAKE) e MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="EA6500V1"

#ea6500z:
#	@$(MAKE) z MIPS32=r2 NVRAM_64K=y NAND=y ASUS_TRX="EA6500V1"
endif

# end of CONFIG_LINUX26
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

	echo 'export EXTRACFLAGS := $(EXTRA_CFLAGS) -DBCMWPA2 -funit-at-a-time -Wno-pointer-sign -mtune=mips32 -mips32 $(if $(filter $(NVRAM_SIZE),0),,-DCONFIG_NVRAM_SIZE=$(NVRAM_SIZE)) $(EXTRA_64KCFLAG)' >> tomato_profile.mak

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
	@echo "       Linksys E-Series with 4MB Flash(E1000v2/v2.1/E1200v1)"
	@echo "e1000v2f     Linksys E1000v2-v2.1/Cisco M10v2 Mini"
	@echo "e1000v2i     Linksys E1000v2-v2.1/Cisco M10v2 MiniIPv6"
	@echo "e1200v1f     Linksys E1200v1 Mini"
	@echo "e1200v1i     Linksys E1200v1 MiniIPv6"
	@echo "       Linksys E-Series with 60k Nvram"
	@echo "n60e         Linksys E-series build VPN"
	@echo "n60c         Linksys E-series build BTGui-VPN"
	@echo "n60d         Linksys E-series build BTGui-VPN plus Nocat"
	@echo "n60g         Linksys E-series build BTGui-VPN plus TOR Project"
	@echo "n60t         Linksys E-series build BT-VPN"
	@echo "n60b         Linksys E-series build Big-VPN"
	@echo "n60o         Linksys E-series build Mega-VPN"
	@echo "n60z         Linksys E-series build AIO"
	@echo "n60i         Linksys E-series build Mini for E2500"
	@echo "n60j         Linksys E-series build MiniIPv6 for E2500"
	@echo "n60v         Linksys E-series build IPv6-VPN for E2000"
	@echo "n60m         Linksys E-series build Max for E2000"
	@echo "       Linksys E-Series with 64k Nvram/8MB Flash(E800/E900/E1200v2/E1500)"
	@echo "n64e         Linksys E-series(64k Nvram) build VPN"
	@echo "n64c         Linksys E-series(64k Nvram) build BTGui-VPN"
	@echo "n64d         Linksys E-series(64k Nvram) build BTGui-VPN plus Nocat"
	@echo "n64g         Linksys E-series(64k Nvram) build BTGui-VPN plus TOR Project"
	@echo "n64u         Linksys E-series(64k Nvram) build BT"
	@echo "n64t         Linksys E-series(64k Nvram) build BT-VPN"
	@echo "n64b         Linksys E-series(64k Nvram) build Big-VPN"
	@echo "n64o         Linksys E-series(64k Nvram) build Mega-VPN"
	@echo "n64v         Linksys E-series(64k Nvram) build IPv6-VPN"
	@echo "n64m         Linksys E-series(64k Nvram) build Max"
	@echo "       Routers with wl_high module"
	@echo "rtn53        Asus RT-N53 with wl_high module (dualband support)"
	@echo "e2500        Linksys E2500 with wl_high module (dualband support)"
	@echo "e3200        Linksys E3200/E2500v3 with wl_high module (dualband support) / max 16MB"
	@echo "n6           Tenda N6 with wl_high module (dualband support)"
	@echo "n60          Tenda N60 with wl_high module (dualband support)"
#	@echo "l600n        Rosewill L600N Build with wl_high module (dualband support)"
	@echo "       Netgear WNR3500L v2 builds (128MB Flash)"
	@echo "v2e          Netgear WNR3500Lv2 build VPN"
	@echo "v2z          Netgear WNR3500Lv2 build AIO"
	@echo ""
	@echo "r64e         RT-N66u build VPN"
	@echo "r64z         RT-N66u build AIO"
ifeq ($(CONFIG_BCMWL6),y)
	@echo "       Routers with AC mode support"
	@echo "ac66e        RT-AC66u build VPN"
	@echo "ac66z        RT-AC66u build AIO"
	@echo "w1800re      Tenda W1800R build VPN"
	@echo "w1800rz      Tenda W1800R build AIO"
#	@echo "d1800he      Buffalo D1800H build VPN"
#	@echo "d1800hz      Buffalo D1800H build AIO"
#	@echo "ea6500e      Linksys EA6500v1 build VPN"
#	@echo "ea6500z      Linksys EA6500v1 build AIO"
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
