topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-debian
all-uninstall: uninstall-debian

install-debian:
	$(call DISTINST,debian)
	$(if $(DESTDIR),,$(V)/usr/sbin/update-rc.d apcupsd remove)
	$(call MKDIR,/etc/init.d)
	$(call INSTDATA,744,apcupsd,/etc/init.d/apcupsd)
	$(if $(DESTDIR),,$(V)/usr/sbin/update-rc.d apcupsd start 20 1 2 3 4 5 . stop 20 0 1 6 .) 
	$(call INSTORIG,744,ups-monitor,/etc/init.d/ups-monitor)
	@echo "---------------------------------------------------------------------"
	@echo " NOTE: APCUPSD will almost certainly not work \"out of the box.\""
	@echo "       You MUST properly edit $(sysconfdir)/apcupsd.conf or apcupsd"
	@echo "       will fail on startup.  Also, please see that your script"
	@echo "       at /etc/init.d/halt properly calls /etc/init.d/ups-monitor"
	@echo "       as shown in platforms/debian/examples/halt.ex."
	@echo "---------------------------------------------------------------------"

uninstall-debian:
	$(call DISTUNINST,debian)
	-$(call UNINST,/etc/init.d/apcupsd)
	-$(if $(DESTDIR),,$(V)/usr/sbin/update-rc.d apcupsd remove)
	-$(call UNINST,/etc/init.d/ups-monitor)

