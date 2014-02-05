topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-engarde
all-uninstall: uninstall-engarde

install-engarde:
	$(call DISTINST,engarde)
   # unlink old rc script (if not doing a DESTDIR install)
	$(call CHKCFG,del,/etc/init.d/apcupsd)
   # install new rc script
	$(call MKDIR,/etc/init.d)
	$(call INSTDATA,744,apcupsd,/etc/init.d)
   # link new rc script (if not doing a DESTDIR install)
	$(call CHKCFG,add,/etc/init.d/apcupsd)
   # save old halt script
	$(call COPY,/etc/init.d/halt,/etc/init.d/halt.old)
   # insert apcupsd callout into halt script
	@echo "  AWK  " $(DESTDIR)/etc/init.d/halt
	$(V)awk -f awkhaltprog $(DESTDIR)/etc/init.d/halt.old >$(DESTDIR)/etc/init.d/halt
	$(V)chmod 744 $(DESTDIR)/etc/init.d/halt
	@echo "================================================="
	@echo " "
	@echo "apcupsd script installation for Engarde Secure Linux complete." 
	@echo " "
	@echo "You should now edit  /etc/apcupsd/apcupsd.conf  to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo " "
	@echo "/etc/init.d/apcupsd start"
	@echo " "
	@echo "thereafter when you reboot, it will be stopped and started"
	@echo "automatically."
	@echo " "
	@echo "Please check that your halt script in:"
	@echo "    /etc/init.d/halt"
	@echo "was properly updated (see installation section of manual)"
	@echo " "
	@echo "================================================="

uninstall-engarde:
	$(call DISTUNINST,engarde)
   # unlink old rc script (if not doing a DESTDIR install)
	-$(call CHKCFG,del,/etc/init.d/apcupsd)
	-$(call UNINST,/etc/init.d/apcupsd)
	-$(call COPY,$(DESTDIR)/etc/init.d/halt.old,/etc/init.d/halt)
	-$(call UNINST,/etc/init.d/halt.old)

