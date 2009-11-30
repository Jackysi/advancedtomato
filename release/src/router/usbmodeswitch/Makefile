PROG        = usb_modeswitch
VERS        = 1.0.4
STRIP	    = strip
CC          = gcc
CCFLAGS     = -l usb
RM          = /bin/rm -f
OBJS        = usb_modeswitch.c
PREFIX	    = $(DESTDIR)/usr
ETCDIR	    = $(DESTDIR)/etc
SBINDIR	    = $(PREFIX)/sbin

.PHONY:     clean
all:        $(PROG)
$(PROG):    $(OBJS)
	      $(CC) $(CCFLAGS) -o $(PROG) $(OBJS)

clean:
	      $(RM) ./usb_modeswitch

install: all
	mkdir -p $(SBINDIR)
	install ./usb_modeswitch $(SBINDIR)
	mkdir -p $(ETCDIR)
	install --mode=644 ./usb_modeswitch.conf $(ETCDIR)

uninstall: clean
	$(RM) /usr/sbin/usb_modeswitch
	$(RM) /etc/usb_modeswitch.conf

.PHONY:     clean install uninstall

integrated:
	make -f Makefile.integrated

integrated_install:
	make -f Makefile.integrated install

