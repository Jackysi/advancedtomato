include ../common.mak

all:
	@echo "" >rom/etc/motd
	@echo "" >>rom/etc/motd
	@echo "Tomato v"`cat ../shared/tomato_version`"" >>rom/etc/motd

clean:
	rm -f rom/etc/motd

install:
	mkdir -p $(INSTALLDIR)
	( tar --exclude=.svn --exclude=*.bak -cpf - rom ) | ( cd $(INSTALLDIR) && tar -xpf - )

	# remove comments, compress whitespace
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/services > $(INSTALLDIR)/rom/etc/services
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/protocols > $(INSTALLDIR)/rom/etc/protocols
	
ifeq ($(TCONFIG_HTTPS),y)
	sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < rom/etc/openssl.cnf > $(INSTALLDIR)/rom/etc/openssl.cnf
else
	rm -f $(INSTALLDIR)/rom/etc/openssl.cnf
endif

ifneq ($(TCONFIG_DNSSEC),y)
	rm -f $(INSTALLDIR)/rom/etc/trust-anchors.conf
endif

ifneq ($(TCONFIG_DNSCRYPT),y)
	rm -f $(INSTALLDIR)/rom/etc/dnscrypt-resolvers.csv
else
	-wget https://raw.githubusercontent.com/jedisct1/dnscrypt-proxy/master/dnscrypt-resolvers.csv -O $(INSTALLDIR)/rom/etc/dnscrypt-resolvers.csv
endif

ifeq ($(TCONFIG_AIO),y)
#	Used by Curl
	cp -f others/cacert.pem $(INSTALLDIR)/rom/cacert.pem
endif
