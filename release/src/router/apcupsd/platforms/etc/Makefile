topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-etc
all-uninstall: uninstall-etc

install-etc:
	$(call MKDIR,$(sysconfdir))
	$(call INSTNEW,644,apcupsd.conf,$(sysconfdir))
	$(call INSTORIG,744,changeme,$(sysconfdir))
	$(call INSTORIG,744,commfailure,$(sysconfdir))
	$(call INSTORIG,744,commok,$(sysconfdir))
	$(call INSTORIG,744,offbattery,$(sysconfdir))
	$(call INSTORIG,744,onbattery,$(sysconfdir))

uninstall-etc:

