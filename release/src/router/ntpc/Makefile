include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS)
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared

OBJS = ntpc.o

all: ntpc

ntpc: $(OBJS)
	@echo " [ntpc] CC -o $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS) -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared

	$(SIZECHECK)
	$(CPTMP)

install: all
	install -D ntpc $(INSTALLDIR)/bin/ntpc
	$(STRIP) $(INSTALLDIR)/bin/ntpc
	chmod 0500 $(INSTALLDIR)/bin/ntpc

	cd $(INSTALLDIR)/bin && ln -sf ntpc ntpsync
	cd $(INSTALLDIR)/bin && ln -sf ntpc ntpstep

clean:
	rm -f ntpc .*.depend *.o
	
%.o: %.c .%.depend
	@echo " [ntpc] CC $@"
	@$(CC) $(CFLAGS) -c $<
	
.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
