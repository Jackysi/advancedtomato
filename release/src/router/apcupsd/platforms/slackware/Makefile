topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-slackware
all-uninstall: uninstall-slackware

install-slackware:
	$(call DISTINST,Slackware)
	$(call MKDIR,/etc/rc.d)
	$(call INSTPROG,744,apcupsd,/etc/rc.d/rc.apcupsd)
	$(call MKDIR,$(LOCKDIR)/subsys)
	$(VV)-rcfile=/etc/rc.d/rc.6; \
	grep -q $(sysconfdir)/apccontrol $$rcfile; \
	if [ $$? -ne 0 ]; then \
      echo "  PATCH $(DESTDIR)$$rcfile"; \
      cp -p $$rcfile $(DESTDIR)$$rcfile.`date +%Y%m%d%M%S`; \
      patch -s $(DESTDIR)$$rcfile rc6.patch; \
      if [ $$? -ne 0 ]; then \
         echo "Patches could not be properly applied.  Please check"; \
         echo "$(DESTDIR)$$rcfile and edit it manually."; \
      fi; \
   fi
	@( echo ''; \
	echo "The apcupsd boot script has been installed to"; \
	echo "/etc/rc.d/rc.apcupsd and the halt script /etc/rc.d/rc.6 has"; \
	echo "been patched.  You must still call /etc/rc.d/rc.apcupsd"; \
	echo "during the boot process after you have tested your"; \
	echo "installation.  One way to do this is from the"; \
	echo "/etc/rc.d/rc.local file."; \
	echo "See the distributions/slackware/README file."; \
	echo ) | /usr/bin/fmt

uninstall-slackware:
	$(call DISTUNINST,Slackware)
	-$(call UNINST,/etc/rc.d/rc.apcupsd)
	$(VV)-rcfile=/etc/rc.d/rc.6; \
	grep -q $(sysconfdir)/apccontrol $$rcfile; \
	if [ $$? -eq 0 ]; then \
      echo "  PATCH $(DESTDIR)$$rcfile"; \
      cp -p $(DESTDIR)$$rcfile $(DESTDIR)$$rcfile.`date +%Y%m%d%M%S`; \
      patch -s -R $(DESTDIR)$$rcfile rc6.patch; \
      if [ $$? -ne 0 ]; then \
         echo "Patches could not be properly removed.  Please check"; \
         echo "$(DESTDIR)$$rcfile and edit it manually."; \
      fi; \
   fi;
