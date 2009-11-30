include ../common.mak

CFLAGS	= -O2 -Wall $(EXTRACFLAGS)
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared
LDFLAGS	=

all: libmssl.so libmssl.a

libmssl.so: mssl.o
	$(CC) -shared -o $@ $^ ../matrixssl/src/libmatrixsslstatic.a -L../shared -lshared -lpthread
	$(STRIP) $@
	$(CPTMP)

libmssl.a: mssl.o
	rm -f $@
	$(AR) cruv $@ $^

clean:
	rm -f .*.depend *.o *.so *.a

install: all
	@echo " [mssl] Installing..."
	install -D libmssl.so $(INSTALLDIR)/usr/lib/libmssl.so
	$(STRIP) $(INSTALLDIR)/usr/lib/libmssl.so

%.o: %.c .%.depend
	@echo " [mssl] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
