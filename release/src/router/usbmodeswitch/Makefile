PROG        = usb_modeswitch
VERS        = 1.1.1
#CC          = gcc
CFLAGS      += -Wall -l usb
RM          = /bin/rm -f
OBJS        = usb_modeswitch.c
PREFIX      = $(DESTDIR)/usr
ETCDIR      = $(DESTDIR)/etc
UDEVDIR     = $(DESTDIR)/lib/udev
RULESDIR    = $(UDEVDIR)/rules.d
SBINDIR     = $(PREFIX)/sbin
MANDIR      = $(PREFIX)/share/man/man1


.PHONY:     clean

all:        $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(CFLAGS)

clean:
	$(RM) usb_modeswitch

install: all files-install rules-reload

files-install:
	install -d $(SBINDIR)
	install --mode=755 usb_modeswitch $(SBINDIR)/usb_modeswitch
	install --mode=755 usb_modeswitch.sh $(UDEVDIR)/usb_modeswitch
	install --mode=644 usb_modeswitch.conf $(ETCDIR)/usb_modeswitch.conf
	install -d $(ETCDIR)/usb_modeswitch.d
	install --mode=644 -t $(ETCDIR)/usb_modeswitch.d usb_modeswitch.d/*
	install --mode=644 usb_modeswitch.1 $(MANDIR)/usb_modeswitch.1
	mkdir -p $(RULESDIR)
	install --mode=644 40-usb_modeswitch.rules $(RULESDIR)/40-usb_modeswitch.rules

rules-reload:
	if [ `which udevadm 2>/dev/null` ]; then \
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
	fi

uninstall: files-uninstall rules-reload

files-uninstall:
	$(RM) $(SBINDIR)/usb_modeswitch
	$(RM) $(UDEVDIR)/usb_modeswitch
	$(RM) $(RULESDIR)/40-usb_modeswitch.rules
	$(RM) $(ETCDIR)/usb_modeswitch.conf
	$(RM) -R $(ETCDIR)/usb_modeswitch.d

.PHONY:    clean install uninstall

