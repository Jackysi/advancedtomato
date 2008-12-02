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
# $Id: Makefile,v 1.65.26.5 2006/07/20 01:22:42 honor Exp $
#

include .config
ifneq ($(wildcard ../cy_conf.mak),)
  include ../cy_conf.mak
endif

#
# Paths
#

# Source bases
export PLATFORM LINUXDIR LIBDIR USRLIBDIR
export TOP := $(shell pwd)
export SRCBASE := $(shell (cd $(TOP)/.. && pwd -P))

#
# Cross-compile environment variables
#

# Build platform
export BUILD := i386-pc-linux-gnu
export HOSTCC := gcc

# uClibc wrapper
ifeq ($(CONFIG_UCLIBC),y)
export PLATFORM := $(PLATFORM)-uclibc
endif

ifeq ($(PLATFORM),mipsel)
export CROSS_COMPILE := mipsel-linux-
export CONFIGURE := ./configure mipsel-linux --build=$(BUILD)
export TOOLCHAIN := $(shell cd $(dir $(shell which $(CROSS_COMPILE)gcc))/../mipsel-linux && pwd -P)
endif

ifeq ($(PLATFORM),mipsel-uclibc)
export CROSS_COMPILE := mipsel-uclibc-
export CONFIGURE := ./configure mipsel-linux --build=$(BUILD)
export TOOLCHAIN := $(shell cd $(dir $(shell which $(CROSS_COMPILE)gcc))/.. && pwd -P)
endif


ifeq ($(CONFIG_BCMWPA2),y)
export CFLAGS += -DBCMWPA2 
endif

export CC := $(CROSS_COMPILE)gcc
export AR := $(CROSS_COMPILE)ar
export AS := $(CROSS_COMPILE)as
export LD := $(CROSS_COMPILE)ld
export NM := $(CROSS_COMPILE)nm
export RANLIB := $(CROSS_COMPILE)ranlib
export STRIP := $(CROSS_COMPILE)strip
export SIZE := $(CROSS_COMPILE)size

#
# Install and target directories
#

export PLATFORMDIR := $(TOP)/$(PLATFORM)
export INSTALLDIR := $(PLATFORMDIR)/install
export TARGETDIR := $(PLATFORMDIR)/target

# always build libbcmcrypto
obj-y += libbcmcrypto 

#
# Configuration
#
CONFIG_IPTABLES=y

obj-$(CONFIG_NVRAM) += nvram
obj-$(CONFIG_SHARED) += shared
obj-$(CONFIG_LIBBCM) += libbcm
obj-$(CONFIG_HTTPD) += httpd
obj-$(CONFIG_RC) += rc
obj-$(CONFIG_WWW) += www
obj-$(CONFIG_GLIBC) += lib
obj-$(CONFIG_UCLIBC) += lib
obj-$(CONFIG_WLCONF) += wlconf
obj-$(CONFIG_BRIDGE) += bridge
obj-$(CONFIG_BUSYBOX) += busybox
obj-$(CONFIG_DNSMASQ) += dnsmasq
obj-$(CONFIG_IPTABLES) += iptables
obj-$(CONFIG_LIBIPT) += iptables
obj-$(CONFIG_NAS) += nas
obj-$(CONFIG_SES) += ses
obj-$(CONFIG_EZC) += ezc
obj-$(CONFIG_NETCONF) += netconf
obj-$(CONFIG_NTP) += ntpclient
obj-$(CONFIG_PPP) += ppp
obj-$(CONFIG_UDHCPD) += udhcpd
obj-$(CONFIG_UPNP) += upnp
obj-$(CONFIG_UTILS) += utils
obj-$(CONFIG_ETC) += etc
obj-$(CONFIG_VLAN) += vlan

obj-$(CONFIG_OPENSSL) += openssl
#obj-$(CONFIG_LIBNET) += net
#obj-$(CONFIG_LIBPCAP) += pcap

##################################################################
CONFIG_OTHERS=y

ifeq ($(HSIAB_SUPPORT),1)
CONFIG_OPENSSL=y
CONFIG_HTTPD_R=y
CONFIG_HSIAB=y
CONFIG_SSLCLIENT=y
obj-$(CONFIG_HTTPD_R) += httpd_r
obj-$(CONFIG_HSIAB) += hsiab
obj-$(CONFIG_SSLCLIENT) += sslclient
endif
ifeq ($(HTTPS_SUPPORT),1)
CONFIG_OPENSSL=y
endif
ifeq ($(SNMP_SUPPORT),1)
CONFIG_OPENSSL=y
endif
ifeq ($(MDNS_SUPPORT),1)
CONFIG_MDNS=y
obj-$(CONFIG_MDNS) += mDNS
endif
ifeq ($(EMI_TEST),1)
CONFIG_EMITEST=y
CONFIG_WLTEST=y
obj-$(CONFIG_EMITEST) += emi
obj-$(CONFIG_WLTEST) += wltest
endif
ifeq ($(SNMP_SUPPORT),1)
obj-$(CONFIG_SNMP) += snmp
endif
ifeq ($(L2TP_SUPPORT),1)
obj-$(CONFIG_L2TP) += rp-l2tp
endif
ifeq ($(HEARTBEAT_SUPPORT),1)
obj-$(CONFIG_HEARTBEAT) += bpalogin
endif
ifeq ($(PARENTAL_CONTROL_SUPPORT),1)
 ifeq ($(LOCALE),EUROPE)
 obj-$(CONFIG_PARENTAL_CONTROL) += artemis_old
 else
 obj-$(CONFIG_PARENTAL_CONTROL) += artemis
 endif
endif

ifeq ($(WAKE_ON_LAN_SUPPORT),1)
obj-$(CONFIG_WAKE_ON_LAN) += wol
endif

ifeq ($(TINYLOGIN_SUPPORT),1)
obj-$(CONFIG_TINYLOGIN) += tinylogin
endif

ifeq ($(SAMBA_SUPPORT),1)
obj-$(CONFIG_SAMBA) += samba
endif

ifeq ($(MULTICAST_SUPPORT),1)
obj-$(CONFIG_MULTICAST) += igmp-proxy
endif

ifeq ($(VERIZON_LAN_SUPPORT),1)
obj-$(CONFIG_VERIZON_LAN) += dhcprelay
endif

obj-$(CONFIG_TFTPD) += tftpd
obj-$(CONFIG_CRON) += cron
obj-$(CONFIG_PPTP) += pptp-client
obj-$(CONFIG_PPPD) += pppd
#obj-$(CONFIG_ZEBRA) += zebra
obj-$(CONFIG_DDNS) += ipupdate
obj-$(CONFIG_OPENSSL) += openssl
obj-$(CONFIG_OTHERS) += others
obj-$(CONFIG_DIAG) += traceroute

ifeq ($(RXP_SUPPORT),1)
obj-$(CONFIG_RXP) += rxp
endif

ifeq ($(PEANUTHULL_SUPPORT),1)
obj-$(CONFIG_PEANUTHULL) += peanuthull 
endif

ifeq ($(EOU_SUPPORT),1)
obj-$(CONFIG_EOU) += eou
endif

ifeq ($(HW_QOS_SUPPORT),1)
obj-$(CONFIG_HW_QOS) += iproute2
endif

ifeq ($(PPPOE_SERVER_SUPPORT),1)
obj-$(CONFIG_PPPOE_SERVER) += rp-pppoe
endif
ifeq ($(PPPOE_RELAY_SUPPORT),1)
obj-$(CONFIG_PPPOE_RELAY) += rp-pppoe
endif
ifeq ($(UTELNETD_SUPPORT),1)
obj-$(CONFIG_UTELNETD) += utelnetd
endif


obj-$(CONFIG_ZEBRA) += zebra

obj-clean := $(foreach obj,$(obj-y) $(obj-n),$(obj)-clean)
obj-install := $(foreach obj,$(obj-y),$(obj)-install)

#
# Basic rules
#

all: clean_target $(obj-y) $(LINUXDIR)/.config
	# Also build kernel
	# first make dep
	$(MAKE) -C $(LINUXDIR) dep
	if ! grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) zImage ; \
	fi
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules ; \
	fi

clean: $(obj-clean)
	rm -rf $(TARGETDIR)
	rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image
	if grep -q "MULTILANG_SUPPORT=1" ../cy_conf.mak ; then \
		rm -f $(PLATFORMDIR)/*lang.trx ; \
		rm -f $(PLATFORMDIR)/*lang.image ; \
		rm -f $(PLATFORMDIR)/lcode.* ; \
	fi

clean_target: 
	rm -rf $(TARGETDIR)
	rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image

distclean mrproper: clean
	rm -f .config $(LINUXDIR)/.config

optimize-lib:
	#../../tools/optimize_lib.sh netconf/ libnetconf.so libnetconf.a libnetconf_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libnetconf.so
	find $(TARGETDIR) -name ".svn" | xargs rm -rf
ifeq ($(CONFIG_OPENSSL),y)
	../../tools/optimize_lib.sh openssl/ libcrypto.so libcrypto.a libcrypto_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libcrypto.so
	../../tools/optimize_lib.sh openssl/ libssl.so libssl.a libssl_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libssl.so
endif
ifeq ($(ZEBRA_SUPPORT),1)
	../../tools/optimize_lib.sh zebra/lib/ libzebra.so libzebra.a libzebra_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libzebra.so
endif
	../../tools/optimize_lib.sh shared/ libshared.so libshared.a libshared_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libshared.so
	../../tools/optimize_lib.sh lib/ libm.so libm.a libm_min.so $(TARGETDIR) $(TARGETDIR)/lib/libm.so.0
	../../tools/optimize_lib.sh busybox/ libbb.so libbb.a libbb_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libbb.so
	../../tools/optimize_lib.sh iptables/ libiptables.so libiptables.a libiptables_min.so $(TARGETDIR) $(TARGETDIR)/usr/lib/libiptables.so

install package: $(filter-out lib-install,$(obj-install)) $(LINUXDIR)/.config
	# Install binaries into target directory
	install -d $(TARGETDIR)
	for dir in $(wildcard $(patsubst %,$(INSTALLDIR)/%,$(obj-y))) ; do \
	    (cd $${dir} && tar cpf - .) | (cd $(TARGETDIR) && tar xpf -) \
	done
	# optimize the crypto library by removing unneeded symbols
	[ ! -d libbcmcrypto ] || $(MAKE) -C libbcmcrypto optimize
	# Install (and possibly optimize) C library
	$(MAKE) lib-install
	# optimize the others library by removing unneeded symbols
	$(MAKE) optimize-lib
	# Install modules into filesystem
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules_install DEPMOD=/bin/true INSTALL_MOD_PATH=$(TARGETDIR) ; \
	fi
	find $(TARGETDIR) -name "wl_*.o"  | xargs rm -rf
	# Prepare filesystem
	cd $(TARGETDIR) && $(TOP)/misc/rootprep.sh
	cd $(TARGETDIR)  &&  find . -iname "CVS" | xargs rm -rf
	# Make sure mkcramfs-2.0 is used
	if grep -q "CONFIG_SQUASHFS=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR)/scripts/squashfs ; \
	    $(LINUXDIR)/scripts/squashfs/mksquashfs $(TARGETDIR) $(PLATFORMDIR)/target.image -info ; \
	    if grep -q "MULTILANG_SUPPORT=1" ../cy_conf.mak ; then \
	            rm -f $(PLATFORMDIR)/*lang.* ; \
	            rm -f $(PLATFORMDIR)/lcode.trx ; \
		    $(LINUXDIR)/scripts/squashfs/mksquashfs www/$(WEB_PAGE) $(PLATFORMDIR)/default_lang.image -info ; \
	    fi \
	elif grep -q "CONFIG_CRAMFS=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR)/scripts/cramfs mkcramfs ; \
	    if grep -q "CONFIG_LZMA_FS_INFLATE=y" $(LINUXDIR)/.config ; then \
	        $(LINUXDIR)/scripts/cramfs/mkcramfs -l $(TARGETDIR) $(PLATFORMDIR)/target.image ; \
		    if grep -q "MULTILANG_SUPPORT=1" ../cy_conf.mak ; then \
		            rm -f $(PLATFORMDIR)/*lang.* ; \
		            rm -f $(PLATFORMDIR)/lcode.trx ; \
			    $(LINUXDIR)/scripts/cramfs/mkcramfs -l www/$(WEB_PAGE) $(PLATFORMDIR)/default_lang.image ; \
		    fi \
	    else \
	        $(LINUXDIR)/scripts/cramfs/mkcramfs $(TARGETDIR) $(PLATFORMDIR)/target.image ; \
		    if grep -q "MULTILANG_SUPPORT=1" ../cy_conf.mak ; then \
		            rm -f $(PLATFORMDIR)/*lang.* ; \
		            rm -f $(PLATFORMDIR)/lcode.trx ; \
			    $(LINUXDIR)/scripts/cramfs/mkcramfs www/$(WEB_PAGE) $(PLATFORMDIR)/default_lang.image ; \
		    fi \
	    fi \
	fi
	# Package kernel and filesystem
	if grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    cp $(PLATFORMDIR)/target.image $(LINUXDIR)/arch/mips/ramdisk/$${CONFIG_EMBEDDED_RAMDISK_IMAGE} ; \
	    $(MAKE) -C $(LINUXDIR) zImage ; \
	else \
	    cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/vmlinuz $(PLATFORMDIR)/ ; \
	    trx -o $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image ; \
	    if grep -q "MULTILANG_SUPPORT=1" ../cy_conf.mak ; then \
		    trx -o $(PLATFORMDIR)/lcode.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image ; \
		    trx -o $(PLATFORMDIR)/default_lang.trx $(PLATFORMDIR)/default_lang.image ; \
	    fi \
	fi

	# Pad self-booting Linux to a 64 KB boundary
	cp $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed/zImage $(PLATFORMDIR)/
	dd conv=sync bs=64k < $(PLATFORMDIR)/zImage > $(PLATFORMDIR)/linux.bin
	# Append filesystem to self-booting Linux
	cat $(PLATFORMDIR)/target.image >> $(PLATFORMDIR)/linux.bin

#
# Configuration rules
#

conf mconf:
	$(MAKE) -C config
	@./config/$@ ./config/Config
	# Also configure kernel
	$(MAKE) k$@

oldconf: .config
	$(MAKE) -C config
	@./config/conf -o ./config/Config
	# Also configure kernel
	$(MAKE) k$@

kconf:
	$(MAKE) -C $(LINUXDIR) config

kmconf:
	$(MAKE) -C $(LINUXDIR) menuconfig

koldconf: $(LINUXDIR)/.config
	$(MAKE) -C $(LINUXDIR) oldconfig

# Convenience
config: conf

menuconfig: mconf

oldconfig: oldconf

# Default configurations
.config:
	cp config/defconfig $@
	$(MAKE) oldconfig

$(LINUXDIR)/.config:
	cp $(LINUXDIR)/arch/mips/defconfig-bcm947xx $@

#
# Overrides
#

OLDBBCFG=$(shell basename $$(readlink busybox/Config.h))
CURBBCFG=$(CONFIG_BUSYBOX_CONFIG).h

busybox/Config.h: dummy
ifneq ($(OLDBBCFG),$(CURBBCFG))
	cd busybox && rm -f Config.h && ln -sf configs/$(CURBBCFG) Config.h && touch Config.h
endif

busybox: busybox/Config.h
	$(MAKE) -C busybox STRIPTOOL=$(STRIP)

busybox-install:
	$(MAKE) -C busybox STRIPTOOL=$(STRIP) PREFIX=$(INSTALLDIR)/busybox install

httpd: nvram shared openssl hsiab
	$(MAKE) -C httpd

rc: netconf nvram shared openssl hsiab libnet
ifneq ($(CONFIG_BUSYBOX),)
rc: busybox/Config.h
endif

www www-%:
	$(MAKE) -C www/$(WEB_PAGE) $* INSTALLDIR=$(INSTALLDIR)/www

bridge:
	$(MAKE) -C bridge brctl/brctl

bridge-install:
	install -D bridge/brctl/brctl $(INSTALLDIR)/bridge/usr/sbin/brctl
	$(STRIP) $(INSTALLDIR)/bridge/usr/sbin/brctl

dnsmasq-install:
	install -D dnsmasq/dnsmasq $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq
	$(STRIP) $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq

iptables:
	$(MAKE) -C iptables BINDIR=/usr/sbin LIBDIR=/usr/lib KERNEL_DIR=$(LINUXDIR)

iptables-install:
ifeq ($(CONFIG_IPTABLES),y)
	install -d $(INSTALLDIR)/iptables/usr/lib/iptables
	install iptables/extensions/*.so $(INSTALLDIR)/iptables/usr/lib/iptables
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/iptables/*.so
	install -D iptables/iptables $(INSTALLDIR)/iptables/usr/sbin/iptables
	$(STRIP) $(INSTALLDIR)/iptables/usr/sbin/iptables
	install -D iptables/iptables-restore $(INSTALLDIR)/iptables/usr/sbin/iptables-restore
	$(STRIP) $(INSTALLDIR)/iptables/usr/sbin/iptables-restore
	install -D iptables/libiptables.so $(INSTALLDIR)/iptables/usr/lib/libiptables.so
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/libiptables.so
else
	# So that generic rule does not take precedence
	@true
endif

iptables-clean:
	-$(MAKE) -C iptables KERNEL_DIR=$(LINUXDIR) clean


netconf: iptables

ntpclient-install:
	install -D ntpclient/ntpclient $(INSTALLDIR)/ntpclient/usr/sbin/ntpclient
	$(STRIP) $(INSTALLDIR)/ntpclient/usr/sbin/ntpclient

ppp ppp-%:
	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp

udhcpd: shared nvram

udhcpd-install:
	install -D udhcpd/udhcpd $(INSTALLDIR)/udhcpd/usr/sbin/udhcpd
	install -D udhcpd/dumpleases $(INSTALLDIR)/udhcpd/usr/sbin/dumpleases
	$(STRIP) $(INSTALLDIR)/udhcpd/usr/sbin/udhcpd
	$(STRIP) $(INSTALLDIR)/udhcpd/usr/sbin/dumpleases
	cd $(INSTALLDIR)/udhcpd/usr/sbin && ln -sf udhcpd udhcpc

upnp: netconf nvram shared

wlconf: nvram shared

vlan:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP)

vlan-install:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) STRIPTOOL=$(STRIP) INSTALLDIR=$(INSTALLDIR) install

vlan-clean:
	$(MAKE) -C vlan clean

nas-install:
	cd $(INSTALLDIR)/nas/usr/sbin && ln -sf nas nas4not

libbcm:
	[ ! -f libbcm/Makefile ] || $(MAKE) -C libbcm

libbcm-install:
	install -D libbcm/libbcm.so $(INSTALLDIR)/libbcm/usr/lib/libbcm.so
	$(STRIP) $(INSTALLDIR)/libbcm/usr/lib/libbcm.so

ses:
	[ ! -f ses/Makefile ] || $(MAKE) -C ses

ses-install:
	install -D ses/ses/ses $(INSTALLDIR)/ses/usr/sbin/ses
	$(STRIP) $(INSTALLDIR)/ses/usr/sbin/ses

########################################################################
# Below is added by CT

tftpd-install:
	[ ! -d tftpd ] || install -D tftpd/tftpd $(INSTALLDIR)/tftpd/usr/sbin/tftpd
	$(STRIP) $(INSTALLDIR)/tftpd/usr/sbin/tftpd

cron-install:
	#install -d $(INSTALLDIR)/cron/etc/cron.d
	install -D cron/cron $(INSTALLDIR)/cron/usr/sbin/cron
	$(STRIP) $(INSTALLDIR)/cron/usr/sbin/cron

pptp-client-install:
	install -D pptp-client/pptp $(INSTALLDIR)/pptp-client/usr/sbin/pptp
	$(STRIP) $(INSTALLDIR)/pptp-client/usr/sbin/pptp

pppd:
	$(MAKE) -C pppd/pppd

pppd-clean:
	$(MAKE) -C pppd/pppd clean

pppd-install:
	install -D pppd/pppd/pppd $(INSTALLDIR)/pppd/usr/sbin/pppd
	$(STRIP) $(INSTALLDIR)/pppd/usr/sbin/pppd

ipupdate-install:
	install -D ipupdate/ez-ipupdate $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate
	$(STRIP) $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate

zebra:
	$(MAKE) -C zebra CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)

zebra-install:
	install -D zebra/zebra/zebra $(INSTALLDIR)/zebra/usr/sbin/zebra
	install -D zebra/ripd/ripd $(INSTALLDIR)/zebra/usr/sbin/ripd
	install -D zebra/lib/libzebra.so $(INSTALLDIR)/zebra/usr/lib/libzebra.so
	$(STRIP) $(INSTALLDIR)/zebra/usr/sbin/zebra
	$(STRIP) $(INSTALLDIR)/zebra/usr/sbin/ripd
	$(STRIP) $(INSTALLDIR)/zebra/usr/lib/libzebra.so

openssl-install:
	install -D openssl/libcrypto.so $(INSTALLDIR)/openssl/usr/lib/libcrypto.so
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libcrypto.so
	install -D openssl/libssl.so $(INSTALLDIR)/openssl/usr/lib/libssl.so
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libssl.so
	install -D openssl/apps/openssl $(INSTALLDIR)/openssl/usr/sbin/openssl
	$(STRIP) $(INSTALLDIR)/openssl/usr/sbin/openssl

ifeq ($(PEANUTHULL_SUPPORT),1)
peanuthull-install:
	install -D peanuthull/peanuthull $(INSTALLDIR)/peanuthull/usr/sbin/peanuthull
	$(STRIP) $(INSTALLDIR)/peanuthull/usr/sbin/peanuthull
endif

ifeq ($(HSIAB_SUPPORT),1)
httpd_r-install:
	install -D httpd_r/httpd_r $(INSTALLDIR)/httpd_r/usr/sbin/httpd_r
	$(STRIP) $(INSTALLDIR)/httpd_r/usr/sbin/httpd_r

hsiab-install:
	install -D hsiab/libhsiab.so $(INSTALLDIR)/hsiab/usr/lib/libhsiab.so
	install -D hsiab/test/test_register $(INSTALLDIR)/hsiab/usr/sbin/test_register
	install -D hsiab/test/test_hsiab $(INSTALLDIR)/hsiab/usr/sbin/test_hsiab
	$(STRIP) $(INSTALLDIR)/hsiab/usr/lib/libhsiab.so
	$(STRIP) $(INSTALLDIR)/hsiab/usr/sbin/test_register
	$(STRIP) $(INSTALLDIR)/hsiab/usr/sbin/test_hsiab

sslclient: openssl
	$(MAKE) -C sslclient

sslclient-install:
	install -D sslclient/sslclient $(INSTALLDIR)/sslclient/usr/sbin/sslclient
	install -D sslclient/root.pem $(INSTALLDIR)/sslclient/etc/root.pem
	install -D sslclient/client.pem $(INSTALLDIR)/sslclient/etc/client.pem
	$(STRIP) $(INSTALLDIR)/sslclient/usr/sbin/sslclient
endif

ifeq ($(MDNS_SUPPORT),1)
mDNS:
	$(MAKE) -C mDNS/mDNSPosix os=linux

mDNS-clean:
	$(MAKE) -C mDNS/mDNSPosix clean

mDNS-install:
	install -D mDNS/mDNSPosix/build/mDNSResponderPosix $(INSTALLDIR)/mDNS/usr/sbin/mDNSResponderPosix
	$(STRIP) $(INSTALLDIR)/mDNS/usr/sbin/mDNSResponderPosix
endif

ifeq ($(EMI_TEST),1)
emi-install:
	install -d $(INSTALLDIR)/emi/usr/sbin/
	cp -f emi/*_* $(INSTALLDIR)/emi/usr/sbin/

wltest-install:
	install -D wltest/wltest $(INSTALLDIR)/wltest/usr/sbin/wltest
	$(STRIP) $(INSTALLDIR)/wltest/usr/sbin/wltest
endif

ifeq ($(L2TP_SUPPORT),1)
rp-l2tp-install:
	install -d $(INSTALLDIR)/rp-l2tp/usr/lib/l2tp
	install rp-l2tp/handlers/*.so $(INSTALLDIR)/rp-l2tp/usr/lib/l2tp
	$(STRIP) $(INSTALLDIR)/rp-l2tp/usr/lib/l2tp/*.so
	install -D rp-l2tp/handlers/l2tp-control $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tp-control
	$(STRIP) $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tp-control
	install -D rp-l2tp/l2tpd $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tpd
	$(STRIP) $(INSTALLDIR)/rp-l2tp/usr/sbin/l2tpd
endif

ifeq ($(HEARTBEAT_SUPPORT),1)
bpalogin-install:
	install -D bpalogin/bpalogin $(INSTALLDIR)/bpalogin/usr/sbin/bpalogin
	$(STRIP) $(INSTALLDIR)/bpalogin/usr/sbin/bpalogin
endif

ifeq ($(MULTICAST_SUPPORT),1)
igmp-proxy-install:
	[ ! -d igmp-proxy ] || install -D igmp-proxy/igmprt $(INSTALLDIR)/igmp-proxy/usr/sbin/igmprt
	$(STRIP) $(INSTALLDIR)/igmp-proxy/usr/sbin/igmprt
endif

ifeq ($(VERIZON_LAN_SUPPORT),1)
dhcprelay-install:
	install -D dhcprelay/dhcrelay $(INSTALLDIR)/dhcprelay/usr/sbin/dhcrelay
	$(STRIP) $(INSTALLDIR)/dhcprelay/usr/sbin/dhcrelay
endif

ifeq ($(SNMP_SUPPORT),1)
snm/libp: openssl
	$(MAKE) -C snmp CC=$(CC) SHLIB_LD_CMD="$(AR) cr"
snmp-install:
	install -D snmp/agent/snmpd $(INSTALLDIR)/snmp/usr/sbin/snmpd
	$(STRIP) $(INSTALLDIR)/snmp/usr/sbin/snmpd
	install -D snmp/snmplib/libsnmp.so $(INSTALLDIR)/snmp/usr/lib/libsnmp.so
	$(STRIP) $(INSTALLDIR)/snmp/usr/lib/libsnmp.so
	install -D snmp/apps/snmptrap $(INSTALLDIR)/snmp/usr/sbin/snmptrap
	$(STRIP) $(INSTALLDIR)/snmp/usr/sbin/snmptrap
	install -D snmp/gen_snmpd_conf/gen_snmpd_conf $(INSTALLDIR)/snmp/usr/sbin/gen_snmpd_conf
	$(STRIP) $(INSTALLDIR)/snmp/usr/sbin/gen_snmpd_conf
	install -D snmp/agent/snmpd.conf $(INSTALLDIR)/snmp/usr/local/share/snmp/snmpd.conf
endif

ifeq ($(PARENTAL_CONTROL_SUPPORT),1)
artemis: nvram
	[ ! -d artemis ] || $(MAKE) -C artemis deps
	[ ! -d artemis ] || $(MAKE) -C artemis CC=$(CC) AR=$(AR)

artemis-install:
	[ ! -d artemis ] || install -D artemis/LINUX/artemis $(INSTALLDIR)/artemis/usr/sbin/artemis
	[ ! -d artemis ] || install -D artemis/LINUX/artmain $(INSTALLDIR)/artemis/usr/sbin/artmain
	$(STRIP) $(INSTALLDIR)/artemis/usr/sbin/artemis
	$(STRIP) $(INSTALLDIR)/artemis/usr/sbin/artmain
artemis_old: nvram
	[ ! -d artemis_old ] || $(MAKE) -C artemis_old deps
	[ ! -d artemis_old ] || $(MAKE) -C artemis_old CC=$(CC) AR=$(AR)

artemis_old-install:
	[ ! -d artemis_old ] || install -D artemis_old/LINUX/artemis $(INSTALLDIR)/artemis_old/usr/sbin/artemis
	[ ! -d artemis_old ] || install -D artemis_old/LINUX/artmain $(INSTALLDIR)/artemis_old/usr/sbin/artmain
	$(STRIP) $(INSTALLDIR)/artemis_old/usr/sbin/artemis
	$(STRIP) $(INSTALLDIR)/artemis_old/usr/sbin/artmain
endif

ifeq ($(EOU_SUPPORT),1)
eou: libnet libpcap
	[ ! -d eou ] || $(MAKE) -C eou

eou-install:
	[ ! -d eou ] || install -D eou/eou $(INSTALLDIR)/eou/usr/sbin/eou
	$(STRIP) $(INSTALLDIR)/eou/usr/sbin/eou

libnet:
	-mkdir -p libnet/lib
	$(MAKE) -C libnet CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)

libpcap:
	$(MAKE) -C libpcap CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)
endif

ifeq ($(WAKE_ON_LAN_SUPPORT),1)
wol: 
	[ ! -d wol ] || $(MAKE) -C wol
wol-install:
	[ ! -d wol ] || install -D wol/ether-wake $(INSTALLDIR)/wol/usr/sbin/ether-wake
	$(STRIP) $(INSTALLDIR)/wol/usr/sbin/ether-wake
endif

ifeq ($(SAMBA_SUPPORT),1)
samba:
	[ ! -d samba ] || $(MAKE) -C samba CC=$(CC)
samba-install:
	[ ! -d samba ] || install -D samba/bin/smbclient $(INSTALLDIR)/samba/usr/sbin/smbclient
	[ ! -d samba ] || install -D samba/bin/nmblookup $(INSTALLDIR)/samba/usr/sbin/nmblookup 
	$(STRIP) $(INSTALLDIR)/samba/usr/sbin/smbclient
	$(STRIP) $(INSTALLDIR)/samba/usr/sbin/nmblookup
endif

ifeq ($(TINYLOGIN_SUPPORT),1)
tinylogin:
	[ ! -d tinylogin ] || $(MAKE) -C tinylogin
tinylogin-install:
	[ ! -d tinylogin ] || $(MAKE) -C tinylogin PREFIX=$(INSTALLDIR)/tinylogin install
endif

traceroute:
	$(MAKE) -C traceroute CC=$(CC)
traceroute-install:
	install -D traceroute/traceroute $(INSTALLDIR)/traceroute/usr/sbin/traceroute
	$(STRIP) $(INSTALLDIR)/traceroute/usr/sbin/traceroute

ifeq ($(HW_QOS_SUPPORT),1)
iproute2-install:
	install -D iproute2/tc/tc $(INSTALLDIR)/iproute2/usr/sbin/tc
	$(STRIP) $(INSTALLDIR)/iproute2/usr/sbin/tc
	#install -D iproute2/ip/ip $(INSTALLDIR)/iproute2/usr/sbin/ip
	#$(STRIP) $(INSTALLDIR)/iproute2/usr/sbin/ip
	#install -D iproute2/tc/prio.sh $(INSTALLDIR)/iproute2/usr/sbin/prio.sh
	#install -D iproute2/tc/cbq.sh $(INSTALLDIR)/iproute2/usr/sbin/cbq.sh
	#install -D iproute2/tc/sfq.sh $(INSTALLDIR)/iproute2/usr/sbin/sfq.sh
	#install -D iproute2/tc/vlan1.sh $(INSTALLDIR)/iproute2/usr/sbin/vlan1.sh
endif

ifeq ($(PPPOE_SERVER_SUPPORT),1)
rp-pppoe:
	$(MAKE) -C rp-pppoe/src/ pppoe
	$(MAKE) -C rp-pppoe/src/ pppoe-server
rp-pppoe-install:
	install -D rp-pppoe/src/pppoe-server $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe-server
	install -D rp-pppoe/src/pppoe $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe
	$(STRIP) $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe-server
	$(STRIP) $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe
endif

ifeq ($(PPPOE_RELAY_SUPPORT),1)
rp-pppoe:
	$(MAKE) -C rp-pppoe/src/ pppoe-relay
rp-pppoe-install:
	install -D rp-pppoe/src/pppoe-relay $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe-relay
	$(STRIP) $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe-relay
endif
ifeq ($(RXP_SUPPORT),1)
rxp:
	$(MAKE) -C rxp CC=$(CC) AR=$(AR) RANLIB=$(RANLIB)
	$(MAKE) -C rxp/example xmlparser1 CC=$(CC)
rxp-install: 
	install -D rxp/rxp $(INSTALLDIR)/rxp/usr/sbin/rxp
	install -D rxp/example/xmlparser1 $(INSTALLDIR)/rxp/usr/sbin/xmlparser1
	$(STRIP) $(INSTALLDIR)/rxp/usr/sbin/rxp
	$(STRIP) $(INSTALLDIR)/rxp/usr/sbin/xmlparser1

endif

#
# Generic rules
#

%:
	[ ! -d $* ] || $(MAKE) -C $*

%-clean:
	[ ! -d $* ] || $(MAKE) -C $* clean

%-install:
	[ ! -d $* ] || $(MAKE) -C $* install INSTALLDIR=$(INSTALLDIR)/$*

$(obj-y) $(obj-n) $(obj-clean) $(obj-install): dummy

.PHONY: all clean distclean mrproper install package
.PHONY: conf mconf oldconf kconf kmconf config menuconfig oldconfig
.PHONY: dummy libnet libpcap
