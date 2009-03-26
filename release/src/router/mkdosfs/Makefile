include ../common.mak

INSTALL_ROOT=$(INSTALLDIR)
export INSTALL_ROOT

all:
	$(MAKE) -C mkdosfs-2.8 CFLAGS="-Os"

install:
	mkdir -p $(INSTALLDIR)/sbin
	$(MAKE) -C mkdosfs-2.8 install CFLAGS="-Os"
	rm -rf $(INSTALLDIR)/usr/man
	$(STRIP) $(INSTALLDIR)/sbin/mkdosfs

clean:
	$(MAKE) -C mkdosfs-2.8 clean
