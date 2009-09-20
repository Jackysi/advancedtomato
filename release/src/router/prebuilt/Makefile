include ../common.mak

all:

clean:

install:
	install -d $(INSTALLDIR)/usr/sbin

	install -m 0755 nas $(INSTALLDIR)/usr/sbin/nas
	$(STRIP) $(INSTALLDIR)/usr/sbin/nas
	# nas4not is now a symbolic link to nas
	ln -sf nas $(INSTALLDIR)/usr/sbin/nas4not
	
	install -m 0755 igmprt $(INSTALLDIR)/usr/sbin/igmprt
	$(STRIP) $(INSTALLDIR)/usr/sbin/igmprt

	install -m 0755 wl $(INSTALLDIR)/usr/sbin/wl
	$(STRIP) $(INSTALLDIR)/usr/sbin/wl
	

