ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

install: 
	install -d $(INSTALLDIR)/etc
	install -d $(INSTALLDIR)/sbin
	install -d $(INSTALLDIR)/usr/sbin
	install -d $(INSTALLDIR)/lib

	install services $(INSTALLDIR)/etc
	install protocols $(INSTALLDIR)/etc
	ln -sf /tmp/resolv.conf $(INSTALLDIR)/etc/resolv.conf
	ln -sf /tmp/hosts $(INSTALLDIR)/etc/hosts
	ln -sf /tmp/cron.d $(INSTALLDIR)/etc/cron.d


clean:

