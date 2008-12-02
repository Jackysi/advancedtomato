include ../common.mak

CFLAGS = -O2 -Wall
CFLAGS += -I$(SRCBASE) -I$(SRCBASE)/include -I. -I$(TOP)/shared

OBJS = httpd.o cgi.o tomato.o version.o 
OBJS += misc.o dhcp.o upgrade.o traceping.o parser.o upnp.o ctnf.o
OBJS += nvram.o log.o webio.o wl.o devlist.o ddns.o config.o bwm.o
OBJS += blackhole.o

LIBS = -L../nvram -lnvram -L../shared -lshared

ifeq ($(TCONFIG_HTTPS),y)
#	LIBS += ../matrixssl/src/libmatrixsslstatic.a
#	OBJS += mssl.o
LIBS += -L../mssl -lmssl
endif

ifeq ($(TCONFIG_SAMBA),y)
OBJS += share.o
endif

all: httpd

httpd: $(OBJS)
	@echo " [httpd] CC $@"
	@$(CC) -o $@ $(OBJS) $(LIBS)

	$(SIZECHECK)
	$(CPTMP)


install:
	@echo " [httpd] Installing to $(INSTALLDIR)"
	@install -m 0500 -D httpd $(INSTALLDIR)/usr/sbin/httpd
	@$(STRIP) $(INSTALLDIR)/usr/sbin/httpd

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
