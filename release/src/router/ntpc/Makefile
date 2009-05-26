include ../common.mak

CFLAGS	= -Os -Wall #-mips32
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/shared

OBJS = ntpc.o

all: ntpc

ntpc: $(OBJS)
	@echo " [ntpc] CC -o $@"
	@$(CC) $(LDFLAGS) -o $@ $(OBJS) -L../nvram -lnvram -L../shared -lshared

	$(SIZECHECK)
	$(CPTMP)

install: all
	install -D -m 0500 ntpc $(INSTALLDIR)/bin/ntpc
	$(STRIP) $(INSTALLDIR)/bin/ntpc

	cd $(INSTALLDIR)/bin && ln -sf ntpc ntpsync && ln -sf ntpc ntpstep

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
