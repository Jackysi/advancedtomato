include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS)
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared -I$(TOP)/mssl
LDFLAGS	=
LIBS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared -L$(TOP)/mssl -lmssl
#LIBS = -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared $(TOP)/mssl/libmssl.a $(TOP)/matrixssl/src/libmatrixsslstatic.a

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

ifeq ($(FULL_OPENSSL),y)
LIBS += -L$(TOP)/openssl -lssl -lcrypto
endif

OBJS = mdu.o md5.o

all: mdu
	
mdu: $(OBJS)
	@echo " [mdu] CC -o $@"
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

	$(SIZECHECK)
	$(CPTMP)

clean:
	rm -f mdu .*.depend
	rm -f *.o

install: all
	@echo " [mdu] Installing..."
	install -D mdu $(INSTALLDIR)/bin/mdu
	$(STRIP) $(INSTALLDIR)/bin/mdu
	chmod 0500 $(INSTALLDIR)/bin/mdu

%.o: %.c .%.depend
	@echo " [mdu] CC $@"
	@$(CC) $(CFLAGS) -c $<

.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)

