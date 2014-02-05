include ../common.mak

ifeq ($(TCONFIG_BCMARM),y)
SRCFILE = broadcom_arm
DSTDIR = $(INSTALLDIR)/lib/modules/2.6.36.4brcmarm/kernel/fs
else
SRCFILE = broadcom
DSTDIR = $(INSTALLDIR)/lib/modules/2.6.22.19/kernel/fs
endif

export DSTDIR

vpath broadcom

all:
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

clean:
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

install:
	echo "[ufsd] Installing..."
	@install -d $(INSTALLDIR)/usr/sbin
	@install -d $(DSTDIR)
ifeq ($(TCONFIG_NTFS),y)
#       @cp -f $(SRCFILE)/mkntfs $(INSTALLDIR)/usr/sbin
#       @chmod 775 $(INSTALLDIR)/usr/sbin/mkntfs
	@cp -f $(SRCFILE)/chkntfs $(INSTALLDIR)/usr/sbin
	@chmod 775 $(INSTALLDIR)/usr/sbin/chkntfs
endif
	@cp -f $(SRCFILE)/ufsd.ko $(DSTDIR)
	@chmod 664 $(DSTDIR)/ufsd.ko
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

.phony: all clean install

