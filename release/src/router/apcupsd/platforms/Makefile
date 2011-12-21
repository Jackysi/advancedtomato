.NOTPARALLEL:

topdir:=..

SUBDIRS = etc $(DISTDIR)
include $(topdir)/autoconf/targets.mak

all-install: install-platform
all-uninstall: uninstall-platform

install-platform:
	$(call MKDIR,$(sysconfdir))
	$(if $(wildcard ./$(DISTNAME)/apccontrol), \
      $(call INSTORIG,744,$(DISTNAME)/apccontrol,$(sysconfdir)), \
      $(call INSTORIG,744,apccontrol,$(sysconfdir)))
	$(if $(HALPOLICYDIR),$(call MKDIR,$(HALPOLICYDIR)))
	$(if $(HALPOLICYDIR),$(call INSTDATA,644,80-apcupsd-ups-policy.fdi,$(HALPOLICYDIR)))

uninstall-platform:
	$(call UNINST,$(sysconfdir)/apccontrol)
	$(if $(HALPOLICYDIR),$(call UNINST,$(HALPOLICYDIR)/80-apcupsd-ups-policy.fdi))

