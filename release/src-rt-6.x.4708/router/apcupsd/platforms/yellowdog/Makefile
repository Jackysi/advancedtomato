topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-yellowdog
all-uninstall: uninstall-yellowdog

install-yellowdog:
	$(call DISTINST,Yellowdog)
   # unlink old rc script (if not doing a DESTDIR install)
	$(call CHKCFG,del,/etc/rc.d/init.d/apcupsd)
   # install new rc script
	$(call MKDIR,/etc/rc.d/init.d)
	$(call INSTDATA,744,apcupsd,/etc/rc.d/init.d)
   # link new rc script (if not doing a DESTDIR install)
	$(call CHKCFG,add,/etc/rc.d/init.d/apcupsd)
   # save old halt script
	$(call COPY,/etc/rc.d/init.d/halt,/etc/rc.d/init.d/halt.old)
   # insert apcupsd callout into halt script
	@echo "  PATCH" $(DESTDIR)/etc/rc.d/init.d/halt
	$(V)awk -f awkhaltprog $(DESTDIR)/etc/rc.d/init.d/halt.old >$(DESTDIR)/etc/rc.d/init.d/halt
	$(V)chmod 744 $(DESTDIR)/etc/rc.d/init.d/halt
	@echo "================================================="
	@echo " "
	@echo "apcupsd script installation for RedHat $(DISTVER) complete." 
	@echo " "
	@echo "You should now edit  /etc/apcupsd/apcupsd.conf  to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo " "
	@echo "/etc/rc.d/init.d/apcupsd start"
	@echo " "
	@echo "thereafter when you reboot, it will be stopped and started"
	@echo "automatically."
	@echo " "
	@echo "Please check that your halt script in:"
	@echo "    /etc/rc.d/init.d/halt"
	@echo "was properly updated (see installation section of manual)"
	@echo " "
	@echo "================================================="

uninstall-yellowdog:
	$(call DISTUNINST,Yellowdog)
   # unlink old rc script (if not doing a DESTDIR install)
	-$(call CHKCFG,del,/etc/rc.d/init.d/apcupsd)
	-$(call UNINST,/etc/rc.d/init.d/apcupsd)
	-$(call COPY,$(DESTDIR)/etc/rc.d/init.d/halt.old,/etc/rc.d/init.d/halt)
	-$(call UNINST,/etc/rc.d/init.d/halt.old)
