include ../common.mak

CFLAGS	= -Os -Wall $(EXTRACFLAGS)
CFLAGS	+= -I$(SRCBASE)/include -I$(TOP)/openvpn/include
LDFLAGS = -L$(TOP)/nvram${BCMEX} -lnvram
OBJS = openvpn_plugin_auth_nvram.o

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

all: openvpn_plugin_auth_nvram.so

openvpn_plugin_auth_nvram.so: $(OBJS)
	@echo " [] CC -o $@"
	@$(CC) -shared $(LDFLAGS) -o $@ $(OBJS) $(LDFLAGS)

	$(SIZECHECK)
	$(CPTMP)

install: all
	install -D openvpn_plugin_auth_nvram.so $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so
	$(STRIP) $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so
	chmod 0500 $(INSTALLDIR)/lib/openvpn_plugin_auth_nvram.so

clean:
	rm -f openvpn_plugin_auth_nvram.so .*.depend *.o
	
%.o: %.c .%.depend
	@echo " [ntpc] CC $@"
	@$(CC) $(CFLAGS) -c $<
	
.depend: $(OBJS:%.o=%.c)
	@$(CC) $(CFLAGS) -M $^ > .depend

.%.depend: %.c
	@$(CC) $(CFLAGS) -M $< > $@

-include $(OBJS:%.o=.%.depend)
