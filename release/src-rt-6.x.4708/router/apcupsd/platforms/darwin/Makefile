topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

# PackageMaker variables
PACKAGEMAKER=/Developer/usr/bin/packagemaker
PKGROOT=/tmp/apcupsd-pkg

all-install: install-darwin install-$(USBDRV)
all-uninstall: uninstall-darwin

install-darwin:
	$(call DISTINST,Darwin Mac OS X)
	$(call MKDIR,/Library/StartupItems/apcupsd)
	$(call INSTDATA,744,apcupsd,/Library/StartupItems/apcupsd)
	$(call INSTDATA,444,StartupParameters.plist,/Library/StartupItems/apcupsd)
	-$(V)chown root:wheel $(DESTDIR)/Library/StartupItems/apcupsd
	-$(V)chown root:wheel $(DESTDIR)/Library/StartupItems/apcupsd/apcupsd
	-$(V)chown root:wheel $(DESTDIR)/Library/StartupItems/apcupsd/StartupParameters.plist
	$(call INSTDATA,755,apcupsd-uninstall,/sbin)
	@echo "================================================="
	@echo " "
	@echo "apcupsd script installation for MacOS X (Darwin) complete." 
	@echo " "
	@echo "You should now edit $(DESTDIR)/$(sysconfdir)/apcupsd.conf to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo " "
	@echo "    SystemStarter start \"APC UPS monitor\""
	@echo " "
	@echo "Thereafter when you reboot, it will be stopped and started"
	@echo "automatically."
	@echo " "
	@if [ -f $(DESTDIR)/$(sysconfdir)/apcupsd ] ; then \
		echo "WARNING: Old startup script $(DESTDIR)/$(sysconfdir)/apcupsd was renamed to"; \
		echo "         $(DESTDIR)/$(sysconfdir)/apcupsd.obsolete. Be sure to remove any";   \
		echo "         references to that script that you may have manually";    \
		echo "         added to the system init scripts. Apcupsd startup is";    \
		echo "         now managed via SystemStarter, making the old script";    \
		echo "         obsolete.";                                               \
		echo " ";                                                                \
		mv $(DESTDIR)/$(sysconfdir)/apcupsd $(DESTDIR)/$(sysconfdir)/apcupsd.obsolete;                 \
	fi
	@echo "================================================="

install-usb:
	$(call MKDIR,/System/Library/Extensions/ApcupsdDummy.kext/Contents)
	-$(V)chown root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext
	-$(V)chown root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents
	$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext
	$(V)chmod 755 $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents
	$(call INSTDATA,644,Info.plist,/System/Library/Extensions/ApcupsdDummy.kext/Contents)
	-$(V)chown root:wheel $(DESTDIR)/System/Library/Extensions/ApcupsdDummy.kext/Contents/Info.plist
	$(call UNINST,/System/Library/Extensions.mkext)
	$(call UNINST,/System/Library/Extensions.kextcache)
	@echo "================================================="
	@echo " "
	@echo "Driver installation complete." 
	@echo "You must REBOOT before running apcupsd."
	@echo " "
	@echo "================================================="

uninstall-darwin:
	$(call DISTUNINST,Darwin Mac OS X)
	-$(call UNINST,/Library/StartupItems/apcupsd)
	-$(call UNINST,/System/Library/Extensions/ApcupsdDummy.kext)
	-$(call UNINST,/System/Library/Extensions.mkext)
	-$(call UNINST,/System/Library/Extensions.kextcache)
	-$(call UNINST,/sbin/apcupsd-uninstall)
	@echo "================================================="
	@echo " "
	@echo "Please REBOOT to complete uninstall." 
	@echo " "
	@echo "================================================="

# Note removal of 'TokenDefinitions.plist' from package below is to prevent
# automatic relocation of apcagent.app at install time. This is a workaround
# for a PackageMaker bug.
apcupsd.pkg:
	rm -rf $(PKGROOT)
	mkdir $(PKGROOT)
	mkdir $(PKGROOT)/Apcupsd.Resources
	mkdir $(PKGROOT)/Apcupsd.Scripts
	mkdir $(PKGROOT)/Apcupsd.Root
	mkdir $(PKGROOT)/Apcupsd
	( cd $(topdir) && DESTDIR=$(PKGROOT)/Apcupsd.Root make install )
	cp $(topdir)/ReleaseNotes $(PKGROOT)/Apcupsd.Resources/ReadMe.txt
	cp $(topdir)/COPYING $(PKGROOT)/Apcupsd.Resources/License.txt
	cp Welcome.txt $(PKGROOT)/Apcupsd.Resources
	cp ApcupsdDaemon.preflight $(PKGROOT)/Apcupsd.Scripts/preflight
	cp ApcupsdDaemon.postflight $(PKGROOT)/Apcupsd.Scripts/postflight
	chown root:admin $(PKGROOT)/Apcupsd.Root/Library
	chmod 1775 $(PKGROOT)/Apcupsd.Root/Library
	chown root $(PKGROOT)/Apcupsd.Root/Applications
	chgrp admin $(PKGROOT)/Apcupsd.Root/Applications
	chmod 775 $(PKGROOT)/Apcupsd.Root/Applications
	$(PACKAGEMAKER) -v --no-recommend --no-relocate --root $(PKGROOT)/Apcupsd.Root \
	   --info ApcupsdDaemon.Info.plist --resources $(PKGROOT)/Apcupsd.Resources \
	   --scripts $(PKGROOT)/Apcupsd.Scripts --id com.apcupsd.apcupsd --root-volume-only \
           --target 10.4 --version $(VERSION) --out $(PKGROOT)/Apcupsd/Apcupsd-$(VERSION).pkg
	rm -f $(PKGROOT)/Apcupsd/Apcupsd-$(VERSION).pkg/Contents/Resources/TokenDefinitions.plist
	hdiutil create -ov -fs HFS+ -srcfolder $(PKGROOT)/Apcupsd -volname Apcupsd-$(VERSION) Apcupsd-$(VERSION).dmg
