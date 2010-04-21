PROG        = usb_modeswitch
VERS        = 1.1.2
CC          = gcc
CFLAGS      += -Wall -l usb
RM          = /bin/rm -f
OBJS        = usb_modeswitch.c
PREFIX      = $(DESTDIR)/usr
ETCDIR      = $(DESTDIR)/etc
UDEVDIR     = $(DESTDIR)/lib/udev
SBINDIR     = $(PREFIX)/sbin
MANDIR      = $(PREFIX)/share/man/man1

.PHONY:     clean

all:        $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(OBJS) $(CFLAGS)

clean:
	$(RM) usb_modeswitch

install: all
	install -d $(SBINDIR)
	install --mode=755 usb_modeswitch $(SBINDIR)/usb_modeswitch
	install --mode=755 usb_modeswitch.tcl $(UDEVDIR)/usb_modeswitch
	install --mode=644 usb_modeswitch.conf $(ETCDIR)/usb_modeswitch.conf
	install --mode=644 usb_modeswitch.1 $(MANDIR)/usb_modeswitch.1


uninstall:
	$(RM) $(SBINDIR)/usb_modeswitch
	$(RM) $(UDEVDIR)/usb_modeswitch
	$(RM) $(RULESDIR)/40-usb_modeswitch.rules
	$(RM) $(ETCDIR)/usb_modeswitch.conf
	$(RM) -R $(ETCDIR)/usb_modeswitch.d

.PHONY:    clean install uninstall

