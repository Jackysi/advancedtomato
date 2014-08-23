include ../common.mak

ifeq ($(TCONFIG_BCMARM),y)
SRCFILE = broadcom_arm
DSTDIR = $(INSTALLDIR)/lib/modules/2.6.36.4brcmarm/kernel/fs
endif

export DSTDIR
export DEBUG

all:

clean:

install:
	@echo "[ufsd] Installing..."
	@install -d $(INSTALLDIR)/usr/sbin
	@install -d $(DSTDIR)
ifneq ($(TCONFIG_BCMSMP),y)
	@cp $(SRCFILE)_up/ufsd.ko $(DSTDIR)
else
	@cp $(SRCFILE)/ufsd.ko $(DSTDIR)
	@cp $(SRCFILE)/jnl.ko $(DSTDIR)
	@chmod 664 $(DSTDIR)/jnl.ko
endif
	@chmod 664 $(DSTDIR)/ufsd.ko
	@cp $(SRCFILE)/mkntfs $(INSTALLDIR)/usr/sbin
	@cp $(SRCFILE)/chkntfs $(INSTALLDIR)/usr/sbin
	@cp $(SRCFILE)/mkhfs $(INSTALLDIR)/usr/sbin
	@cp $(SRCFILE)/chkhfs $(INSTALLDIR)/usr/sbin/
	@chmod 775 $(INSTALLDIR)/usr/sbin/mkntfs
	@chmod 775 $(INSTALLDIR)/usr/sbin/chkntfs
	@chmod 775 $(INSTALLDIR)/usr/sbin/mkhfs
	@chmod 775 $(INSTALLDIR)/usr/sbin/chkhfs

.phony: all clean install

