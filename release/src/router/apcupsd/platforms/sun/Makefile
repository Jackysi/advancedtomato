topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-sun $(if $(DESTDIR),,install-$(USBDRV))
all-uninstall: uninstall-sun $(if $(DESTDIR),,uninstall-$(USBDRV))

install-sun:
	$(call DISTINST,Sun)
	$(call MKDIR,/etc/rc0.d)
	$(call MKDIR,/etc/rc1.d)
	$(call MKDIR,/etc/rc2.d)
	$(call MKDIR,/etc/init.d)
	$(call INSTPROG,744,apcupsd,/etc/init.d/apcupsd)
	$(call SYMLINK,../init.d/apcupsd,/etc/rc0.d/K21apcupsd)
	$(call SYMLINK,../init.d/apcupsd,/etc/rc1.d/S89apcupsd)
	$(call SYMLINK,../init.d/apcupsd,/etc/rc2.d/S89apcupsd)
	@echo "================================================="
	@echo "apcupsd script installation for Solaris $(DISTVER) complete."
	@echo "You should now edit $(sysconfdir)/apcupsd.conf  to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo 
	@echo "/etc/init.d/apcupsd start"
	@echo 
	@echo "Thereafter when you reboot, it will be stopped and started"
	@echo "automatically."
	@echo "================================================="

install-usb:
	@echo "Configuring ugen driver to match APC UPSes..."
	@echo
	$(V)-add_drv -f -i '"usbif51d,class3"' ugen || \
	  update_drv -a -i '"usbif51d,class3"' ugen
	@echo
	@echo "NOTE:"
	@echo "   \"(usbif51d,class3) already in use\" and"
	@echo "   \"Driver (ugen) is already installed\""
	@echo "   errors may be safely ignored."
	@echo
	@echo "================================================="
	@echo "Driver configured. You must PERFORM A RECONFIGURE"
	@echo "BOOT \"reboot -- -r\" before running Apcupsd."
	@echo "================================================="

uninstall-sun:
	$(call DISTUNINST,Sun)
	-$(call UNINST,/etc/rc0.d/K21apcupsd)
	-$(call UNINST,/etc/rc1.d/S89apcupsd)
	-$(call UNINST,/etc/rc2.d/S89apcupsd)
	-$(call UNINST,/etc/init.d/apcupsd)

uninstall-usb:
	@echo "Removing ugen binding (errors are safe to ignore)..."
	$(V)-update_drv -d -i '"usbif51d,class3"' ugen > /dev/null
