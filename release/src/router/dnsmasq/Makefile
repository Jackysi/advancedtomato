ifneq ($(wildcard $(SRCBASE)/cy_conf.mak),)
  include $(SRCBASE)/cy_conf.mak
endif

OBJS = cache.o rfc1035.o dhcp.o util.o option.o forward.o network.o dnsmasq.o
BINDIR = /usr/local/sbin
MANDIR = /usr/local/man




all : dnsmasq

clean :
	rm -f *~ $(OBJS) dnsmasq

.c.o: dnsmasq.h
	$(CC) $(CFLAGS) -O2 -Wall -W -c $*.c 


dnsmasq : $(OBJS) dnsmasq.h config.h
	$(CC) -o $@  $(OBJS) $(LIBS)

install : dnsmasq
	install -d $(BINDIR) -d $(MANDIR)
	install -m 644 dnsmasq.8 $(MANDIR)/man8 
	install -m 755 dnsmasq $(BINDIR)




