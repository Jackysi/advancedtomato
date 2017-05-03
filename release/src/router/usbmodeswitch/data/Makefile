PROG        = usb-modeswitch-data
VERS        = 20170205
RM          = /bin/rm -f
PREFIX      = $(DESTDIR)/usr
ETCDIR      = $(DESTDIR)/etc
RULESDIR    = $(DESTDIR)/lib/udev/rules.d


.PHONY:     clean

all: 40-usb_modeswitch.rules

clean:
	$(RM) 40-usb_modeswitch.rules

install: all files-install db-install

install-packed: files-install db-install-packed

files-install: 
	install -d $(PREFIX)/share/usb_modeswitch
	install -d $(ETCDIR)/usb_modeswitch.d
	install -D --mode=644 40-usb_modeswitch.rules $(RULESDIR)/40-usb_modeswitch.rules

40-usb_modeswitch.rules:
	./gen-rules.tcl

db-install: files-install
	install --mode=644 -t $(PREFIX)/share/usb_modeswitch ./usb_modeswitch.d/*

db-install-packed:
	@# Create a compressed tar without gzip timestamp, so tar.gz
	@# differs only if content is different
	cd ./usb_modeswitch.d; find * -print0 | LC_ALL=C sort -z | tar --no-recursion --null --files-from=- --mode=go=rX,u+rw,a-s -cf ../configPack.tar
	gzip -f9n ./configPack.tar
	install --mode=644 -t $(PREFIX)/share/usb_modeswitch ./configPack.tar.gz
	rm -f ./configPack.tar.gz

uninstall: files-uninstall

files-uninstall:
	$(RM) $(RULESDIR)/40-usb_modeswitch.rules
	$(RM) -R $(PREFIX)/share/usb_modeswitch

.PHONY:    clean install uninstall
