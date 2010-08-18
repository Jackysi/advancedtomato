PROG        = usb-modeswitch-data
VERS        = 20100817
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
	install -d $(ETCDIR)/usb_modeswitch.d
	install -D --mode=644 40-usb_modeswitch.rules $(RULESDIR)/40-usb_modeswitch.rules

db-install:
	install --mode=644 -t $(ETCDIR)/usb_modeswitch.d ./usb_modeswitch.d/*

db-install-packed:
	cd ./usb_modeswitch.d; tar -czf ../configPack.tar.gz *
	install --mode=644 -t $(ETCDIR)/usb_modeswitch.d ./configPack.tar.gz
	rm -f ./configPack.tar.gz


rules-reload:
	if [ -f $(ETCDIR)/issue ]; then \
		if [ -n `which udevadm 2>/dev/null` ]; then \
			UDEVADM=`which udevadm`; \
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
	$(RM) -R $(ETCDIR)/usb_modeswitch.d

.PHONY:    clean install uninstall

