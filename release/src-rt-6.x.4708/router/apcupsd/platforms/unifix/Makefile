topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-unifix
all-uninstall: uninstall-unifix

install-unifix:
	$(call DISTINST,Unifix)
	@echo "You have to manually install apcupsd boot script and"
	@echo "halt script for clean emergency shutdown."
	@echo "I'm sorry: you are on your own."

uninstall-unifix:
	$(call DISTUNINST,Unifix)
	@echo "You have to manually uninstall apcupsd boot script and"
	@echo "halt script."
	@echo "I'm sorry: you are on your own."
