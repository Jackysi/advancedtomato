PROG        = usb-modeswitch-data
VERS        = 20110714
RM          = /bin/rm -f
PREFIX      = $(DESTDIR)/usr
ETCDIR      = $(DESTDIR)/etc
RULESDIR    = $(DESTDIR)/lib/udev/rules.d


.PHONY:     clean

all:

clean:

install: files-install db-install rules-reload

install-packed: files-install db-install-packed rules-reload

files-install:
	install -d $(PREFIX)/share/usb_modeswitch
	install -d $(ETCDIR)/usb_modeswitch.d
	install -D --mode=644 40-usb_modeswitch.rules $(RULESDIR)/40-usb_modeswitch.rules

db-install:
	install --mode=644 -t $(PREFIX)/share/usb_modeswitch ./usb_modeswitch.d/*

db-install-packed:
	cd ./usb_modeswitch.d; tar -czf ../configPack.tar.gz *
	install --mode=644 -t $(PREFIX)/share/usb_modeswitch ./configPack.tar.gz
	rm -f ./configPack.tar.gz

rules-reload:
	if [ -f $(ETCDIR)/issue ]; then \
		UDEVADM=`which udevadm 2>/dev/null`; \
		if [ "x$$UDEVADM" != "x" ]; then \
			UDEVADM_VER=`$$UDEVADM -V 2>/dev/null`; \
			if [ -z $$UDEVADM_VER ]; then \
				UDEVADM_VER=`$$UDEVADM --version 2>/dev/null`; \
			fi; \
			if [ $$UDEVADM_VER -gt 127 ]; then \
				$$UDEVADM control --reload-rules; \
			else \
				$$UDEVADM control --reload_rules; \
			fi \
		elif [ `which udevcontrol 2>/dev/null` ]; then \
		`which udevcontrol` reload_rules; \
		fi \
	fi

uninstall: files-uninstall rules-reload

files-uninstall:
	$(RM) $(RULESDIR)/40-usb_modeswitch.rules
	$(RM) -R $(PREFIX)/share/usb_modeswitch

.PHONY:    clean install uninstall
