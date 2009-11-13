include ../common.mak

CFLAGS = -O2 -Wall $(EXTRACFLAGS)
CFLAGS += -I$(SRCBASE) -I$(SRCBASE)/include -I. -I$(TOP)/shared

OBJS = httpd.o cgi.o tomato.o version.o 
OBJS += misc.o dhcp.o upgrade.o traceping.o parser.o upnp.o ctnf.o
OBJS += nvram.o log.o webio.o wl.o devlist.o ddns.o config.o bwm.o
OBJS += blackhole.o

LIBS = -L../nvram -lnvram -L../shared -lshared
LIBS += -L../mssl -lmssl

all: httpd

httpd: $(OBJS)
	@echo " [httpd] CC $@"
	@$(CC) -o $@ $(OBJS) $(LIBS)

	$(SIZECHECK)
	$(CPTMP)


install:
	@echo " [httpd] Installing to $(INSTALLDIR)"
	@install -D httpd $(INSTALLDIR)/usr/sbin/httpd
	@$(STRIP) $(INSTALLDIR)/usr/sbin/httpd
	@chmod 0500 $(INSTALLDIR)/usr/sbin/httpd

clean:
	rm -f httpd *.o .*.depend

size: httpd
	mipsel-uclibc-nm --print-size --size-sort httpd

%.o: %.c .%.depend
	@echo " [httpd] CC $@"
	@$(CC) $(CFLAGS) -o $@ -c $<

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
