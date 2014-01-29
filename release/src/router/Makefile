#
# Broadcom Linux Router Makefile
#
# Copyright 2005, Broadcom Corporation
# All Rights Reserved.
#
# THIS SOFTWARE IS OFFERED "AS IS", AND BROADCOM GRANTS NO WARRANTIES OF ANY
# KIND, EXPRESS OR IMPLIED, BY STATUTE, COMMUNICATION OR OTHERWISE. BROADCOM
# SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A SPECIFIC PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.
#
#

include common.mak


#
#
#
ifeq ($(TOMATO_DEV),jon)
SEP=echo -e "\033[41;1m   $@   \033[0m"
else
SEP=true
endif

#
# standard packages
#
obj-y += nvram
obj-y += shared
obj-y += prebuilt
obj-y += iptables
obj-y += rc
obj-y += iproute2
obj-y += rom
obj-y += others
obj-y += busybox
obj-y += httpd
obj-y += www
obj-y += bridge
obj-y += dnsmasq
obj-y += etc
obj-y += vlan
obj-y += ppp
obj-y += ttcp
obj-y += ntpc
obj-y += rstats
#	obj-y += ebtables
#	obj-y += libbcm
#	obj-y += libbcmcrypto
#	obj-y += nas
#	obj-y += wlconf

obj-y += mssl
obj-y += mdu

ifeq ($(TEST_MINIUPNPD),1)
obj-y += miniupnpd
else
obj-y += upnp
endif


#
# configurable packages
#
obj-$(TCONFIG_L2TP) += rp-l2tp
obj-$(TCONFIG_PPTP) += pptp-client
#	obj-$(TCONFIG_HEARTBEAT) += bpalogin
#	obj-$(TCONFIG_DDNS) += ipupdate
obj-$(TCONFIG_HTTPS) += openssl
obj-$(TCONFIG_HTTPS) += matrixssl
obj-$(TCONFIG_SSH) += dropbear
obj-$(TCONFIG_ZEBRA) += zebra
obj-$(TCONFIG_IPP2P) += ipp2p

ifeq ($(TCONFIG_L2TP),y)
NEED_PPPD = y
endif
ifeq ($(TCONFIG_PPTP),y)
NEED_PPPD = y
endif
obj-$(NEED_PPPD) += pppd


obj-clean := $(foreach obj,$(obj-y) $(obj-n),$(obj)-clean)
obj-install := $(foreach obj,$(obj-y),$(obj)-install)



#
# Basic rules
#

all: clean-build $(obj-y) kernel


kernel: $(LINUXDIR)/.config
	@$(SEP)

	@if ! grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) zImage ; \
	fi
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules ; \
	fi


kmod: dummy
	$(MAKE) -C $(LINUXDIR) modules

fooit:
	@../btools/libfoo.pl

install package: $(obj-install) $(LINUXDIR)/.config
	@$(SEP)

	install -d $(TARGETDIR)


# kernel modules
	$(MAKE) -C $(LINUXDIR) modules_install DEPMOD=/bin/true INSTALL_MOD_PATH=$(TARGETDIR)
	find $(TARGETDIR)/lib/modules -name *.o -exec mipsel-linux-strip --strip-unneeded {} \;

	-cd $(TARGETDIR)/lib/modules/2.4.20/kernel/drivers/net && mv diag/* . && rm -rf diag

# nice and clean
	-cd $(TARGETDIR)/lib/modules/2.4.20/kernel/drivers/net && mv et.4702/* . && rm -rf et.4702 || true
	cd $(TARGETDIR)/lib/modules/2.4.20/kernel/drivers/net && mv et/* . && rm -rf et
	cd $(TARGETDIR)/lib/modules/2.4.20/kernel/drivers/net && mv wl/* . && rm -rf wl
	cd $(TARGETDIR)/lib/modules/2.4.20/kernel/fs && mv cifs/* . && rm -rf cifs
	cd $(TARGETDIR)/lib/modules/2.4.20/kernel/fs && mv jffs2/* . && rm -rf jffs2
	cd $(TARGETDIR)/lib/modules/2.4.20/kernel/lib && mv zlib_inflate/* . && rm -rf zlib_inflate
	cd $(TARGETDIR)/lib/modules/2.4.20/kernel/lib && mv zlib_deflate/* . && rm -rf zlib_deflate
	rm -rf $(TARGETDIR)/lib/modules/2.4.20/pcmcia

# misc
	for dir in $(wildcard $(patsubst %,$(INSTALLDIR)/%,$(obj-y))) ; do \
	    (cd $${dir} && tar cpf - .) | (cd $(TARGETDIR) && tar xpf -) \
	done

ifneq ($(TCONFIG_JFFS2),y)
	rm -f $(TARGETDIR)/lib/modules/2.4.20/kernel/fs/jffs2.o
	rm -rf $(TARGETDIR)/lib/modules/2.4.20/kernel/lib/zlib_*
endif

ifneq ($(TCONFIG_CIFS),y)
	rm -f $(TARGETDIR)/lib/modules/2.4.20/kernel/fs/cifs.o
endif

ifneq ($(TCONFIG_L7),y)
	rm -f $(TARGETDIR)/lib/modules/2.4.20/kernel/net/ipv4/netfilter/ipt_layer7.o
	rm -f $(TARGETDIR)/usr/lib/iptables/libipt_layer7.so
endif

# separated extra stuff
	@rm -rf $(PLATFORMDIR)/extras
	@mkdir $(PLATFORMDIR)/extras
	@mv $(TARGETDIR)/lib/modules/2.4.20/kernel/net/ipv4/ip_gre.o $(PLATFORMDIR)/extras/
	@mv $(TARGETDIR)/lib/modules/2.4.20/kernel/drivers/net/tun.o $(PLATFORMDIR)/extras/

# uClibc
	install $(LIBDIR)/ld-uClibc.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libcrypt.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libc.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libdl.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libm.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libutil.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libnsl.so.0 $(TARGETDIR)/lib/

	@cd $(TARGETDIR) && $(TOP)/others/rootprep.sh

	@echo ---
	busybox/examples/depmod.pl -k $(LINUXDIR)/vmlinux -b $(TARGETDIR)/lib/modules/2.4.20/
	@echo ---

	@rm -f $(TARGETDIR)/lib/modules/2.4.20/build

	@../btools/libfoo.pl
	@chmod 0555 $(TARGETDIR)/lib/*.so*


	@mkdir -p $(TARGETDIR)/rom/etc/l7-protocols
ifeq ($(TCONFIG_L7PAT),y)
	@cd layer7 && ./squish.sh
	cp layer7/squished/*.pat $(TARGETDIR)/rom/etc/l7-protocols
endif

	@$(MAKE) -C $(SRCBASE)/linux/linux/scripts/squashfs mksquashfs-lzma
	@$(SRCBASE)/linux/linux/scripts/squashfs/mksquashfs-lzma $(TARGETDIR) $(PLATFORMDIR)/target.image

#	Package kernel and filesystem
#	if grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
#	    cp $(PLATFORMDIR)/target.image $(LINUXDIR)/arch/mips/ramdisk/$${CONFIG_EMBEDDED_RAMDISK_IMAGE} ; \
#	    $(MAKE) -C $(LINUXDIR) zImage ; \
#	else \
#	    cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz $(PLATFORMDIR)/ ; \
#	    trx -o $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image ; \
#	fi

# 	Pad self-booting Linux to a 64 KB boundary
#	cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/zImage $(PLATFORMDIR)/
#	dd conv=sync bs=64k < $(PLATFORMDIR)/zImage > $(PLATFORMDIR)/linux.bin
# 	Append filesystem to self-booting Linux
#	cat $(PLATFORMDIR)/target.image >> $(PLATFORMDIR)/linux.bin



#
# cleaners
#

clean: clean-build $(obj-clean)
	rm -rf layer7/squished

clean-build: dummy
	rm -rf $(TARGETDIR)
	rm -rf $(INSTALLDIR)
	rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image
	rm -rf $(PLATFORMDIR)/extras

distclean: clean
ifneq ($(INSIDE_MAK),1)
	$(MAKE) -C .. $@ INSIDE_MAK=1
endif

#
# configuration
#

CONFIG_IN := config/config.in

config/conf config/mconf:
	@$(MAKE) -C config

rconf: config/conf
	@config/conf $(CONFIG_IN)

rmconf: config/mconf
	@config/mconf $(CONFIG_IN)

roldconf: config/conf
	@config/conf -o $(CONFIG_IN)

kconf:
	@$(MAKE) -C $(LINUXDIR) config

kmconf:
	@$(MAKE) -C $(LINUXDIR) menuconfig

koldconf:
	@$(MAKE) -C $(LINUXDIR) oldconfig

config conf: rconf kconf

menuconfig mconf: rmconf kmconf

oldconfig oldconf: roldconf koldconf


#
# overrides and extra dependencies
#

busybox: dummy
	$(MAKE) -C busybox CFLAGS="-Os -fno-delete-null-pointer-checks"
# V=1

busybox-install:
	rm -rf $(INSTALLDIR)/busybox
	$(MAKE) -C busybox install CONFIG_PREFIX=$(INSTALLDIR)/busybox CFLAGS="-Os"

busybox-test: dummy
	$(MAKE) -C busybox objsizes CFLAGS="-Os"

# 1.2:
#	busybox: busybox/include/bb_config.h
#		@$(MAKE) -C busybox CROSS_CFLAGS="-mips2"
#	busybox/include/bb_config.h:
#		@$(MAKE) -C busybox oldconfig
#		@$(MAKE) -C busybox clean
#	busybox-install:
#		rm -rf $(INSTALLDIR)/busybox
#		$(MAKE) -C busybox install PREFIX=$(INSTALLDIR)/busybox
	
httpd: matrixssl shared nvram mssl
	@$(SEP)
	@$(MAKE) -C httpd


matrixssl:
	@$(SEP)
	@$(MAKE) -C matrixssl/src

matrixssl-install:
	@true

matrixssl-clean:
	$(MAKE) -C matrixssl/src clean


openssl-install:
	install -D openssl/libcrypto.so $(INSTALLDIR)/openssl/usr/lib/libcrypto.so
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libcrypto.so

	install -D -m 500 openssl/apps/openssl $(INSTALLDIR)/openssl/usr/sbin/openssl
	$(STRIP) $(INSTALLDIR)/openssl/usr/sbin/openssl

	install -D -m 0500 httpd/gencert.sh $(INSTALLDIR)/openssl/usr/sbin/gencert.sh

#	perl -e 'while (<>) { s/.SECS/time()-(24*60*60)/e; print; }' < httpd/gencert.sh > $(INSTALLDIR)/openssl/usr/sbin/gencert.sh
#	chmod 0500 $(INSTALLDIR)/openssl/usr/sbin/gencert.sh

#	install -D openssl/libssl.so $(INSTALLDIR)/openssl/usr/lib/libssl.so
#	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libssl.so

mssl:	matrixssl

mdu:	shared mssl

rc: nvram shared


bridge:
	@$(SEP)
	@$(MAKE) -C bridge brctl/brctl

bridge-install:
	install -D bridge/brctl/brctl $(INSTALLDIR)/bridge/usr/sbin/brctl
	$(STRIP) $(INSTALLDIR)/bridge/usr/sbin/brctl


dnsmasq-install:
	install -D dnsmasq/src/dnsmasq $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq
	$(STRIP) $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq

iptables:
	@$(SEP)
	$(MAKE) -C iptables BINDIR=/usr/sbin LIBDIR=/usr/lib KERNEL_DIR=$(LINUXDIR)

iptables-install:
	install -D iptables/iptables $(INSTALLDIR)/iptables/usr/sbin/iptables
	cd $(INSTALLDIR)/iptables/usr/sbin && \
		ln -sf iptables iptables-restore

	install -d $(INSTALLDIR)/iptables/usr/lib/iptables
	install -D iptables/extensions/*.so $(INSTALLDIR)/iptables/usr/lib/iptables/

	install -D iptables/libiptc.so $(INSTALLDIR)/iptables/usr/lib/libiptc.so

	$(STRIP) $(INSTALLDIR)/iptables/usr/sbin/iptables
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/iptables/*.so
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/libiptc.so

iptables-clean:
	-$(MAKE) -C iptables KERNEL_DIR=$(LINUXDIR) clean

ppp:
	@$(SEP)
	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp 
#	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp DFLAGS="-DDEBUG -DDEBUGALL"

ppp-%:
	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp



upnp: nvram shared iptables

#	miniupnpd-install:
#		install -D miniupnpd/miniupnpd $(INSTALLDIR)/miniupnpd/usr/sbin/miniupnpd
#		$(STRIP) $(INSTALLDIR)/miniupnpd/usr/sbin/miniupnpd

nvram: shared

vlan:
	@$(SEP)
	@$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP)

vlan-install:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP) INSTALLDIR=$(INSTALLDIR) install


pptp-client-install:
	install -D pptp-client/pptp $(INSTALLDIR)/pptp-client/usr/sbin/pptp
	$(STRIP) $(INSTALLDIR)/pptp-client/usr/sbin/pptp


pppd:
	@$(SEP)
	@$(MAKE) -C pppd/pppd

pppd-clean:
	$(MAKE) -C pppd/pppd clean

pppd-install:
	install -D pppd/pppd/pppd $(INSTALLDIR)/pppd/usr/sbin/pppd
	$(STRIP) $(INSTALLDIR)/pppd/usr/sbin/pppd

#	ipupdate-install:
#		install -D ipupdate/ez-ipupdate $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate
#		$(STRIP) $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate


zebra:
	@$(SEP)
	@$(MAKE) -C zebra CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)

zebra-install:
	install -D zebra/zebra/zebra $(INSTALLDIR)/zebra/usr/sbin/zebra
	install -D zebra/ripd/ripd $(INSTALLDIR)/zebra/usr/sbin/ripd
	install -D zebra/lib/libzebra.so $(INSTALLDIR)/zebra/usr/lib/libzebra.so
	$(STRIP) $(INSTALLDIR)/zebra/usr/sbin/zebra
	$(STRIP) $(INSTALLDIR)/zebra/usr/sbin/ripd
	$(STRIP) $(INSTALLDIR)/zebra/usr/lib/libzebra.so


rp-l2tp-install:
	install -d $(INSTALLDIR)/rp-l2tp/usr/lib/l2tp
	install rp-l2tp/handlers/*.so $(INSTALLDIR)/rp-l2tp/usr/lib/l2tp
	$(STRIP) $(INSTALLDIR)/rp-l2tp/usr/lib/l2tp/*.so
	install -D rp-l2tp/handlers/l2tp-control $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tp-control
	$(STRIP) $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tp-control
	install -D rp-l2tp/l2tpd $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tpd
	$(STRIP) $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tpd


bpalogin-install:
	install -D bpalogin/bpalogin $(INSTALLDIR)/bpalogin/usr/sbin/bpalogin
	$(STRIP) $(INSTALLDIR)/bpalogin/usr/sbin/bpalogin


#	libnet:
#		@$(SEP)
#		@-mkdir -p libnet/lib
#		@$(MAKE) -C libnet CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)

#	libpcap:
#		@$(SEP)
#		@$(MAKE) -C libpcap CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)

libbcm:
	@[ ! -f libbcm/Makefile ] || $(MAKE) -C libbcm

libbcm-install:
	install -D libbcm/libbcm.so $(INSTALLDIR)/libbcm/usr/lib/libbcm.so
	$(STRIP) $(INSTALLDIR)/libbcm/usr/lib/libbcm.so


iproute2-install:
	install -D iproute2/tc/tc $(INSTALLDIR)/iproute2/usr/sbin/tc
	$(STRIP) $(INSTALLDIR)/iproute2/usr/sbin/tc
	install -D iproute2/ip/ip $(INSTALLDIR)/iproute2/usr/sbin/ip
	$(STRIP) $(INSTALLDIR)/iproute2/usr/sbin/ip


ntpc: nvram shared


dropbear: dropbear/config.h
	@$(SEP)
	@make -C dropbear PROGRAMS="dropbear dbclient dropbearkey scp" MULTI=1

dropbear-install:
	install -D dropbear/dropbearmulti $(INSTALLDIR)/dropbear/usr/bin/dropbearmulti
	$(STRIP) $(INSTALLDIR)/dropbear/usr/bin/dropbearmulti
	cd $(INSTALLDIR)/dropbear/usr/bin && \
	ln -sf dropbearmulti dropbear && \
	ln -sf dropbearmulti dropbearconvert && \
	ln -sf dropbearmulti dropbearkey && \
	ln -sf dropbearmulti dbclient && \
	ln -sf dropbearmulti ssh && \
	ln -sf dropbearmulti scp

dropbear-clean:
	@make -C dropbear clean

dropbear/config.h:
	cd dropbear && CFLAGS="-O2 -Wall" ac_cv_func_logout=no ac_cv_func_logwtmp=no ./configure --host=mipsel-linux --disable-zlib --enable-syslog --disable-lastlog --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx --disable-pututline --disable-pututxline --disable-loginfunc --disable-largefile
	make -C dropbear clean


ebtables: dummy
	@make -C ebtables

ebtables-install: dummy
	install -D ebtables/ebtables $(INSTALLDIR)/ebtables/usr/sbin/ebtables

	mkdir -p $(INSTALLDIR)/ebtables/usr/lib/
	cp ebtables/*.so $(INSTALLDIR)/ebtables/usr/lib/
	cp ebtables/extensions/*.so $(INSTALLDIR)/ebtables/usr/lib/
	
ebtables-clean:
	make -C ebtables clean




#
# Generic rules
#

%:
	@[ ! -d $* ] || ( $(SEP); $(MAKE) -C $* )


%-clean:
	@-[ ! -d $* ] || $(MAKE) -C $* clean

%-install:
	@[ ! -d $* ] || $(MAKE) -C $* install INSTALLDIR=$(INSTALLDIR)/$*

%-build:
	$(MAKE) $*-clean $*

$(obj-y) $(obj-n) $(obj-clean) $(obj-install): dummy

.PHONY: all clean distclean mrproper install package
.PHONY: conf mconf oldconf kconf kmconf config menuconfig oldconfig
.PHONY: dummy libnet libpcap
