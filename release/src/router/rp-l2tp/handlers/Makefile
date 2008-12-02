# $Id: Makefile,v 1.1.48.1 2005/08/08 12:05:25 honor Exp $
# Makefile for LNS handlers
# LIC: GPL

prefix=/usr
exec_prefix=${prefix}
mandir=${prefix}/man
docdir=/usr/doc/rp-pppoe-$(VERSION)
install=/usr/bin/install -c
install_dir=/usr/bin/install -c -d
sbindir=${exec_prefix}/sbin

HANDLERS=sync-pppd.so cmd.so

OBJS=pty.o sync-pppd.o dstring.o
SRCS=$(OBJS:.o=.c)
CFLAGS=-g -O2 -I.. -I../libevent -I../../libevent -Wall

all: $(HANDLERS) l2tp-control

l2tp-control: cmd-control.o
	$(CC) -o l2tp-control cmd-control.o

cmd-control.o: cmd-control.c
	$(CC) -c -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

sync-pppd.so: pty.o sync-pppd.o
	$(CC) -shared -o $@ $^

cmd.so: cmd.o dstring.o
	$(CC) -shared -o $@ $^

clean:
	rm -f *.so *.o *~
	rm -f l2tp-control

install: all
	-mkdir -p $(RPM_INSTALL_ROOT)$(sbindir)
	-mkdir -p $(RPM_INSTALL_ROOT)$(prefix)/lib/l2tp/plugins
	$(install) -m 755 -s l2tp-control $(RPM_INSTALL_ROOT)$(sbindir)
	$(install) -m 755 $(HANDLERS) $(RPM_INSTALL_ROOT)$(prefix)/lib/l2tp/plugins
distclean: clean
