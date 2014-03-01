include ../common.mak

ifeq ($(TCONFIG_BCMARM),y)
SRCFILE = broadcom_arm
DSTDIR = $(INSTALLDIR)/lib/modules/2.6.36.4brcmarm/kernel/fs
endif

ifeq ($(TCONFIG_UFSD_DEBUG),y)
DEBUG=y
endif

export DSTDIR
export DEBUG

all:
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

clean:
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

install:
	@echo "[ufsd] Installing..."
	@install -d $(INSTALLDIR)/usr/sbin
	@install -d $(DSTDIR)
	@cp -f $(SRCFILE)/package/ufsd.ko $(DSTDIR)
	@cp -f $(SRCFILE)/chkntfs $(INSTALLDIR)/usr/sbin
	@chmod 775 $(INSTALLDIR)/usr/sbin/chkntfs
	@[ ! -f $(SRCFILE)/Makefile ] || $(MAKE) -C $(SRCFILE) $@

.phony: all clean install

