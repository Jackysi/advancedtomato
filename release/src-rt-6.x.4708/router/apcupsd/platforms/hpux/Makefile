topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-install: install-unknown
all-uninstall: uninstall-unknown

install-unknown:
	$(call DISTINST,Unknown)
	@echo "You have to manually install apcupsd boot script and"
	@echo "halt script for clean emergency shutdown."
	@echo "Please contribute your distribution install to apcupsd team."
	@echo "I'm sorry: you are on your own."

uninstall-unknown:
	$(call DISTUNINST,Unknown)
	@echo "You have to manually uninstall apcupsd boot script and"
	@echo "halt script."
	@echo "Please contribute your distribution install to apcupsd team."
	@echo "I'm sorry: you are on your own."
