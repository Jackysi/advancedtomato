all:

clean:

install:
	install -d $(INSTALLDIR)/usr/sbin

	install -m 0755 nas $(INSTALLDIR)/usr/sbin/nas
	install -m 0755 nas4not $(INSTALLDIR)/usr/sbin/nas4not
	
	install -m 0755 igmprt $(INSTALLDIR)/usr/sbin/igmprt
	$(STRIP) $(INSTALLDIR)/usr/sbin/igmprt

	install -m 0755 wl $(INSTALLDIR)/usr/sbin/wl
	$(STRIP) $(INSTALLDIR)/usr/sbin/wl
	
	install -m 0755 et $(INSTALLDIR)/usr/sbin/et
	$(STRIP) $(INSTALLDIR)/usr/sbin/et

	install -m 0755 wlconf $(INSTALLDIR)/usr/sbin/wlconf
	$(STRIP) $(INSTALLDIR)/usr/sbin/wlconf

	
	install -d $(INSTALLDIR)/usr/lib

	install libbcmcrypto.so $(INSTALLDIR)/usr/lib

