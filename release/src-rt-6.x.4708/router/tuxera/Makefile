include ../common.mak
include $(SRCBASE)/router/.config

# Broadcom platform

ifeq ($(TCONFIG_BCMARM),y)
ifeq ($(TCONFIG_BCM7),y)
SRCFILE = broadcom_arm_7
else
SRCFILE = broadcom_arm
endif
DSTDIR = $(INSTALLDIR)/lib/modules/2.6.36.4brcmarm/kernel/fs
endif

export DSTDIR

vpath broadcom ralink

all:
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

clean:
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

install:
	@echo "[tuxera] Installing..."
	@install -d $(DSTDIR)
	@install -d $(INSTALLDIR)/usr/sbin
ifeq ($(TCONFIG_TFAT),y)
	@cp -f $(SRCFILE)/fat/kernel-module/tfat.ko $(DSTDIR)
	install -D $(SRCFILE)/fat/tools/fsck_msdos $(INSTALLDIR)/usr/sbin/fsck_msdos
ifneq ($(NO_MKTOOLS),y)
	install -D $(SRCFILE)/fat/tools/newfs_msdos $(INSTALLDIR)/usr/sbin/newfs_msdos
	#install -D $(SRCFILE)/fat/tools/fatdebug $(INSTALLDIR)/usr/sbin/fatdebug
	#install -D $(SRCFILE)/fat/tools/fatinfo $(INSTALLDIR)/usr/sbin/fatinfo
	#install -D $(SRCFILE)/fat/tools/dosfslabel $(INSTALLDIR)/usr/sbin/dosfslabel
	#install -D $(SRCFILE)/fat/tools/fatdump $(INSTALLDIR)/usr/sbin/fatdump
endif
endif
ifeq ($(TCONFIG_TUXERA),y)
	@cp -f $(SRCFILE)/ntfs/kernel-module/tntfs.ko $(DSTDIR)
	install -D $(SRCFILE)/ntfs/tools/ntfsck $(INSTALLDIR)/usr/sbin/ntfsck
ifneq ($(NO_MKTOOLS),y)
	install -D $(SRCFILE)/ntfs/tools/mkntfs $(INSTALLDIR)/usr/sbin/mkntfs
	#install -D $(SRCFILE)/ntfs/tools/ntfsdebug $(INSTALLDIR)/usr/sbin/ntfsdebug
	#install -D $(SRCFILE)/ntfs/tools/ntfsinfo $(INSTALLDIR)/usr/sbin/ntfsinfo
	#install -D $(SRCFILE)/ntfs/tools/ntfslabel $(INSTALLDIR)/usr/sbin/ntfslabel
endif
endif
ifeq ($(TCONFIG_TUXERA_HFS),y)
	cp -f $(SRCFILE)/hfsplus/kernel-module/thfsplus.ko $(DSTDIR)
	install -D $(SRCFILE)/hfsplus/tools/fsck_hfs $(INSTALLDIR)/usr/sbin/fsck_hfs
ifneq ($(NO_MKTOOLS),y)
	install -D $(SRCFILE)/hfsplus/tools/newfs_hfs $(INSTALLDIR)/usr/sbin/newfs_hfs
endif
endif

.phony: all clean install

