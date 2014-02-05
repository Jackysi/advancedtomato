topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-etc
all-uninstall: uninstall-etc

install-etc:
	$(call MKDIR,$(bindir))
	$(call INSTORIG,744,hostname,$(bindir))

uninstall-etc:

