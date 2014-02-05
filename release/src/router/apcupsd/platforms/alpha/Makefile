topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-alpha
all-uninstall: uninstall-alpha

install-alpha:
	$(call DISTINST,alpha)
	$(call MKDIR,/sbin/init.d)
	$(call INSTDATA,744,apcupsd,/sbin/init.d/apcupsd)
	$(call MKDIR,/sbin/rc2.d)
	$(call SYMLINK,../init.d/apcupsd,/sbin/rc2.d/K56apcupsd)
	$(call SYMLINK,../init.d/apcupsd,/sbin/rc2.d/S56apcupsd)
	$(call MKDIR,/sbin/rc3.d)
	$(call SYMLINK,../init.d/apcupsd,/sbin/rc3.d/K56apcupsd)
	$(call SYMLINK,../init.d/apcupsd,/sbin/rc3.d/S56apcupsd)
   # save old halt script
	$(call COPY,/sbin/rc0,/sbin/rc0.old)
   # insert apcupsd callout into halt script
	@echo "  AWK " $(DESTDIR)/sbin/rc0
	$(V)awk -f awkhaltprog $(DESTDIR)/sbin/rc0.old >$(DESTDIR)/sbin/rc0
	$(V)chmod 744 $(DESTDIR)/sbin/rc0
	@echo "================================================="
	@echo " "
	@echo "apcupsd script installation for the Alpha $(DISTVER) complete." 
	@echo " "
	@echo "You should now edit  /etc/apcupsd/apcupsd.conf  to correspond"
	@echo "to your setup then start the apcupsd daemon with:"
	@echo " "
	@echo "/sbin/init.d/apcupsd start"
	@echo " "
	@echo "thereafter when you reboot, it will be stopped and started"
	@echo "automatically."
	@echo " "
	@echo "Please check that your halt script in:"
	@echo "    /sbin/rc0"
	@echo "was properly updated (see installation section of manual)"
	@echo " "
	@echo "================================================="

uninstall-alpha:
	$(call DISTUNINST,alpha)
	-$(call UNINST,/sbin/rc2.d/K56apcupsd)
	-$(call UNINST,/sbin/rc2.d/S56apcupsd)
	-$(call UNINST,/sbin/rc3.d/K56apcupsd)
	-$(call UNINST,/sbin/rc3.d/S56apcupsd)
	-$(call UNINST,/sbin/init.d/apcupsd)
	$(call COPY,/sbin/rc0,/sbin/rc0.old)
	$(V)grep -v "###apcupsd###" $(DESTDIR)/sbin/rc0.old >$(DESTDIR)/sbin/rc0
	$(V)chmod 744 $(DESTDIR)/sbin/rc0
