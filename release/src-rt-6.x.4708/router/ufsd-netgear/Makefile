include ../common.mak

ifeq ($(TCONFIG_BCMARM),y)
DSTDIR = $(INSTALLDIR)/lib/modules/2.6.36.4brcmarm/kernel/fs
endif

install:
	@echo "[ufsd] Installing..."
	@install -d $(INSTALLDIR)/usr/sbin
	@install -d $(DSTDIR)
	@cp -f ufsd.ko $(DSTDIR)
	@cp -f chkntfs $(INSTALLDIR)/usr/sbin
	@chmod 775 $(INSTALLDIR)/usr/sbin/chkntfs

.phony: install

