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

TOMATO_EXPERIMENTAL=0

#
#
#
SEP=echo "\033[41;1m   $@   \033[0m"

#
# standard packages
#
obj-y += lzma-loader
obj-y += nvram
obj-y += shared
obj-y += prebuilt
obj-y += igmpproxy
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
# obj-y += vlan # use Busybox vconfig
obj-y += ppp
obj-y += ttcp
obj-y += ntpc
obj-y += rstats

# !!TB - updated Broadcom Wireless driver
obj-y += et
obj-y += libbcmcrypto
obj-y += wlconf

obj-y += mssl
obj-y += mdu

# !!TB
obj-$(TCONFIG_USB) += p910nd
obj-$(TCONFIG_USB) += scsi-idle
obj-$(TCONFIG_USB) += libusb10
obj-$(TCONFIG_USB) += libusb
obj-$(TCONFIG_USB) += usbmodeswitch
#obj-y += sendmail
obj-$(TCONFIG_FTP) += vsftpd
ifeq ($(CONFIG_LINUX26),y)
ifeq ($(TCONFIG_SAMBASRV),y)
NEED_EX_NLS = y
endif
endif
ifeq ($(TCONFIG_SAMBASRV),y)
ifeq ($(TCONFIG_SAMBA3),y)
NEED_SAMBA3 = y
else
NEED_SAMBA2 = y
endif
endif
obj-$(NEED_SAMBA2) += samba
obj-$(NEED_SAMBA3) += samba3
obj-$(TCONFIG_NTFS) += ntfs-3g

obj-y += miniupnpd
#	obj-y += upnp


#
# configurable packages
#
obj-$(TCONFIG_L2TP) += rp-l2tp
obj-$(TCONFIG_PPTP) += pptp-client
obj-$(TCONFIG_HTTPS) += openssl
obj-$(TCONFIG_HTTPS) += matrixssl
obj-$(TCONFIG_SSH) += dropbear
obj-$(TCONFIG_ZEBRA) += zebra
#	obj-$(TCONFIG_IPP2P) += ipp2p
obj-$(TCONFIG_LZO) += lzo
obj-$(TCONFIG_OPENVPN) += openvpn

ifeq ($(TCONFIG_L2TP),y)
NEED_PPPD = y
endif
ifeq ($(TCONFIG_PPTP),y)
NEED_PPPD = y
endif
obj-$(NEED_PPPD) += pppd

obj-$(CONFIG_LINUX26) += hotplug2
obj-$(CONFIG_LINUX26) += udev
obj-$(__CONFIG_BCMWL5) += emf
obj-$(__CONFIG_BCMWL5) += igs

obj-clean := $(foreach obj, $(obj-y) $(obj-n) $(obj-), $(obj)-clean)
obj-install := $(foreach obj,$(obj-y),$(obj)-install)



#
# Basic rules
#

all: clean-build libc $(obj-y) kernel


kernel: $(LINUXDIR)/.config
	@$(SEP)

	@if ! grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) zImage CC=$(KERNELCC); \
	fi
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules CC=$(KERNELCC); \
	fi
ifeq ($(CONFIG_LINUX26),y)
	$(MAKE) -C $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed srctree=$(LINUXDIR)
endif


lzma-loader:
	$(MAKE) -C $(SRCBASE)/lzma-loader CROSS_COMPILE=$(CROSS_COMPILE) LD=$(LD)

lzma-loader-install: lzma-loader
	@$(SEP)


kmod: dummy
	$(MAKE) -C $(LINUXDIR) modules

testfind:
	cd $(TARGETDIR)/lib/modules/* && find -name "*.o" -exec mv -i {} . \; || true
	cd $(TARGETDIR)/lib/modules/* && find -type d -delete || true

install package: $(obj-install) $(LINUXDIR)/.config
	@$(SEP)

	install -d $(TARGETDIR)


# kernel modules
	$(MAKE) -C $(LINUXDIR) modules_install DEPMOD=/bin/true INSTALL_MOD_PATH=$(TARGETDIR)
##!!TB	find $(TARGETDIR)/lib/modules -name *.o -exec mipsel-linux-strip --strip-unneeded {} \;
	find $(TARGETDIR)/lib/modules -name *.o -exec $(STRIP) --strip-debug {} \;
	find $(TARGETDIR)/lib/modules -name *.ko -exec $(STRIP) --strip-debug {} \;

	-cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv diag/* . && rm -rf diag

# nice and clean
	-cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv et.4702/* . && rm -rf et.4702 || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv et/* . && rm -rf et
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv wl/* . && rm -rf wl
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv cifs/* . && rm -rf cifs
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv jffs2/* . && rm -rf jffs2 || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv jffs/* . && rm -rf jffs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/lib && mv zlib_inflate/* . && rm -rf zlib_inflate || true
	cd $(TARGETDIR)/lib/modules/*/kernel/lib && mv zlib_deflate/* . && rm -rf zlib_deflate || true
	cd $(TARGETDIR)/lib/modules/*/kernel/lib && mv lzo/* . && rm -rf lzo || true
	rm -rf $(TARGETDIR)/lib/modules/*/pcmcia

##!!TB
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv ext2/* . && rm -rf ext2 || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv ext3/* . && rm -rf ext3 || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv jbd/* . && rm -rf jbd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv fat/* . && rm -rf fat || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv vfat/* . && rm -rf vfat || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv msdos/* . && rm -rf msdos || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv fuse/* . && rm -rf fuse || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv ntfs/* . && rm -rf ntfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv smbfs/* . && rm -rf smbfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv reiserfs/* . && rm -rf reiserfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv hfsplus/* . && rm -rf hfsplus || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv lockd/* . && rm -rf lockd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv nfsd/* . && rm -rf nfsd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv nfs/* . && rm -rf nfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv xfs/* . && rm -rf xfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv nls/* . && rm -rf nls || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv exportfs/* . && rm -rf exportfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/net && mv sunrpc/* . && rm -rf sunrpc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/net && mv auth_gss/* . && rm -rf auth_gss || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv hcd/* . && rm -rf hcd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv host/* . && rm -rf host || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv storage/* . && rm -rf storage || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv serial/* . && rm -rf serial || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv core/* . && rm -rf core || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv class/* . && rm -rf class || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv misc/* . && rm -rf misc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media && mv video/* . && rm -rf video || true

	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv bcm57xx/* . && rm -rf bcm57xx || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv emf/* . && rm -rf emf || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv igs/* . && rm -rf igs || true
	cd $(TARGETDIR)/lib/modules && rm -f */source || true

# misc
	for dir in $(wildcard $(patsubst %,$(INSTALLDIR)/%,$(obj-y))) ; do \
	    (cd $${dir} && tar cpf - .) | (cd $(TARGETDIR) && tar xpf -) \
	done

ifneq ($(TCONFIG_JFFS2),y)
	rm -f $(TARGETDIR)/lib/modules/*/kernel/fs/jffs2.*o || true
	rm -f $(TARGETDIR)/lib/modules/*/kernel/fs/jffs.*o || true
ifneq ($(CONFIG_LINUX26),y)
	rm -rf $(TARGETDIR)/lib/modules/*/kernel/lib/zlib_* || true
endif
endif

ifneq ($(TCONFIG_CIFS),y)
	rm -f $(TARGETDIR)/lib/modules/*/kernel/fs/cifs.*o
endif

ifneq ($(TCONFIG_L7),y)
	rm -f $(TARGETDIR)/lib/modules/*/kernel/net/ipv4/netfilter/ipt_layer7.*o
	rm -f $(TARGETDIR)/usr/lib/iptables/libipt_layer7.so
endif

# uClibc
	install $(LIBDIR)/ld-uClibc.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libcrypt.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libpthread.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libgcc_s.so.1 $(TARGETDIR)/lib/
	$(STRIP) $(TARGETDIR)/lib/libgcc_s.so.1
	install $(LIBDIR)/libc.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libdl.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libm.so.0 $(TARGETDIR)/lib/
	install $(LIBDIR)/libnsl.so.0 $(TARGETDIR)/lib/
ifeq ($(TCONFIG_SSH),y)
	install $(LIBDIR)/libutil.so.0 $(TARGETDIR)/lib/
endif

	@cd $(TARGETDIR) && $(TOP)/others/rootprep.sh

	@echo ---

	@../btools/libfoo.pl
	@chmod 0555 $(TARGETDIR)/lib/*.so*
	@chmod 0555 $(TARGETDIR)/usr/lib/*.so*

# !!TB - moved to run after libfoo.pl - to make sure shared libs include all symbols needed by extras
# separated extra stuff
	@rm -rf $(PLATFORMDIR)/extras
	@mkdir $(PLATFORMDIR)/extras
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/ipv4/ip_gre.*o $(PLATFORMDIR)/extras/ || true
ifneq ($(TCONFIG_OPENVPN),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/tun.*o $(PLATFORMDIR)/extras/ || true
endif

#!!TB
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/ntfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/smbfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/reiserfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/hfsplus.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nfsd.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/lockd.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/exportfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/sunrpc.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/auth_rpcgss.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/rpcsec_gss_krb5.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/xfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/scanner.*o $(PLATFORMDIR)/extras/ || true
ifneq ($(CONFIG_LINUX26),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/usbserial.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/option.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/sierra.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/*acm.*o $(PLATFORMDIR)/extras/ || true
endif
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ftdi_sio.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/pl2303.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ipw.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/audio.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ov51*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/pwc*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/emi*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/media/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/media || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/sound/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/sound || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/sound/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/sound || true

	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/pptp.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/pppol2tp.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/ppp_mppe*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/ppp_deflate.*o  $(PLATFORMDIR)/extras/ || true
	@mkdir -p $(PLATFORMDIR)/extras/crypto
	@mv $(TARGETDIR)/lib/modules/*/kernel/crypto/* $(PLATFORMDIR)/extras/crypto/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/crypto || true
ifneq ($(CONFIG_LINUX26),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/lib/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/lib || true
endif
ifneq ($(TCONFIG_USB),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_*.*o $(PLATFORMDIR)/extras/ || true
endif
ifneq ($(NEED_EX_NLS),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_cp9*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_cp1251.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_euc-jp.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_sjis.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_gb2312.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_euc-kr.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/nls_big5.*o $(PLATFORMDIR)/extras/ || true
endif
ifneq ($(TCONFIG_USB),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/*.*o $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/usb || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/scsi/*.*o $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/scsi || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/ext2.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/ext3.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/jbd.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/fat.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/vfat.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/msdos.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/fuse.*o $(PLATFORMDIR)/extras/ || true
endif
ifneq ($(TCONFIG_USB_EXTRAS),y)
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/block/loop.*o $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/block || true
endif

ifneq ($(CONFIG_LINUX26),y)
	@mkdir -p $(PLATFORMDIR)/extras/lib
	@mv $(TARGETDIR)/usr/lib/libusb* $(PLATFORMDIR)/extras/lib/ || true
	@mkdir -p $(PLATFORMDIR)/extras/apps
	@mv $(TARGETDIR)/usr/sbin/usb_modeswitch $(PLATFORMDIR)/extras/apps/ || true
	@cp usbmodeswitch/usb_modeswitch.conf $(PLATFORMDIR)/extras/apps/usb_modeswitch.conf || true
endif

	@mkdir -p $(TARGETDIR)/rom/etc/l7-protocols
ifeq ($(TCONFIG_L7PAT),y)
	@cd layer7 && ./squish.sh
	cp layer7/squished/*.pat $(TARGETDIR)/rom/etc/l7-protocols
endif

	busybox/examples/depmod.pl -k $(LINUXDIR)/vmlinux -b $(TARGETDIR)/lib/modules/*/
	@mv $(TARGETDIR)/lib/modules/*/modules.dep $(TARGETDIR)/lib/modules/
	@echo ---

	@rm -f $(TARGETDIR)/lib/modules/*/build

	@$(MAKE) -C $(LINUXDIR)/scripts/squashfs mksquashfs-lzma
	@$(LINUXDIR)/scripts/squashfs/mksquashfs-lzma $(TARGETDIR) $(PLATFORMDIR)/target.image -all-root -noappend -nopad | tee target.info

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


libc:	$(LIBDIR)/ld-uClibc.so.0
#	$(MAKE) -C ../../../tools-src/uClibc all
#	$(MAKE) -C ../../../tools-src/uClibc install


#
# cleaners
#

clean: clean-build $(obj-clean)
	rm -rf layer7/squished
	make -C config clean

clean-build: dummy
	rm -rf $(TARGETDIR)
	rm -rf $(INSTALLDIR)
	rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image
	rm -rf $(PLATFORMDIR)/extras

distclean: clean
ifneq ($(INSIDE_MAK),1)
	$(MAKE) -C .. $@ INSIDE_MAK=1
endif
	-rm -f $(LIBDIR)/*.so.0  $(LIBDIR)/*.so

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
	@$(MAKE) -C $(LINUXDIR) include/linux/version.h

bboldconf:
	@$(MAKE) -C busybox oldconfig

config conf: rconf kconf

menuconfig mconf: rmconf kmconf

oldconfig oldconf: koldconf roldconf bboldconf


#
# overrides and extra dependencies
#

busybox: dummy
	@$(MAKE) -C busybox CFLAGS_busybox="$(EXTRACFLAGS)"

# V=1

busybox-install:
	rm -rf $(INSTALLDIR)/busybox
	$(MAKE) -C busybox install CONFIG_PREFIX=$(INSTALLDIR)/busybox

busybox-clean:
	$(MAKE) -C busybox distclean

busybox-config:
	$(MAKE) -C busybox menuconfig


httpd: matrixssl shared nvram mssl
	@$(SEP)
	@$(MAKE) -C httpd

www-install:
	@$(MAKE) -C www install INSTALLDIR=$(INSTALLDIR)/www TOMATO_EXPERIMENTAL=$(TOMATO_EXPERIMENTAL)

matrixssl:
	@$(SEP)
	@$(MAKE) -C matrixssl/src

matrixssl-install:
	@true

matrixssl-clean:
	$(MAKE) -C matrixssl/src clean

openssl-config:
ifeq ($(TCONFIG_OPENVPN),y)
	cd openssl && \
	CC=$(CC) && ./Configure linux-mipsel
else
	cd openssl && \
	CC=$(CC) && ./Configure linux-mipsel no-aes no-idea no-md2 no-mdc2 no-rc5
endif
	@$(MAKE) -C openssl clean

openssl: openssl-config

openssl-install:
	install -D openssl/libcrypto.so $(INSTALLDIR)/openssl/usr/lib/libcrypto.so
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libcrypto.so

	install -D openssl/apps/openssl $(INSTALLDIR)/openssl/usr/sbin/openssl
	$(STRIP) $(INSTALLDIR)/openssl/usr/sbin/openssl
	chmod 0500 $(INSTALLDIR)/openssl/usr/sbin/openssl

	install -D -m 0500 httpd/gencert.sh $(INSTALLDIR)/openssl/usr/sbin/gencert.sh

#	perl -e 'while (<>) { s/.SECS/time()-(24*60*60)/e; print; }' < httpd/gencert.sh > $(INSTALLDIR)/openssl/usr/sbin/gencert.sh
#	chmod 0500 $(INSTALLDIR)/openssl/usr/sbin/gencert.sh

ifeq ($(TCONFIG_FTP_SSL),y)
# !!TB
	install -D openssl/libssl.so $(INSTALLDIR)/openssl/usr/lib/libssl.so
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libssl.so
endif

ifeq ($(TCONFIG_OPENVPN),y)
	install -D openssl/libssl.so $(INSTALLDIR)/openssl/usr/lib/libssl.so
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libssl.so
endif

mssl:	matrixssl

mdu:	shared mssl

rc: nvram shared

bridge/Makefile:
	cd bridge && CFLAGS="-Os -g $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix="" --with-linux-headers=$(LINUXDIR)/include

bridge: bridge/Makefile
	@$(SEP)
	@$(MAKE) -C bridge

bridge-clean:
	@$(MAKE) -C bridge clean || true
	@rm -f bridge/Makefile

bridge-install:
	install -D bridge/brctl/brctl $(INSTALLDIR)/bridge/usr/sbin/brctl
	$(STRIP) $(INSTALLDIR)/bridge/usr/sbin/brctl

dnsmasq:
	@$(SEP)
	@$(MAKE) -C dnsmasq COPTS="-DNO_IPV6 -DNO_SCRIPT $(EXTRACFLAGS)"

dnsmasq-install:
	install -D dnsmasq/src/dnsmasq $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq
	$(STRIP) $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq

iptables:
	@$(SEP)
	$(MAKE) -C iptables BINDIR=/usr/sbin LIBDIR=/usr/lib KERNEL_DIR=$(LINUXDIR) COPT_FLAGS="-Os $(EXTRACFLAGS)"

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

miniupnpd: iptables
	@$(SEP)
	$(MAKE) -C miniupnpd -f Makefile.tomato

miniupnpd-clean:
	-$(MAKE) -C miniupnpd -f Makefile.tomato clean

miniupnpd-install:
	install -D miniupnpd/miniupnpd $(INSTALLDIR)/miniupnpd/usr/sbin/miniupnpd
	$(STRIP) $(INSTALLDIR)/miniupnpd/usr/sbin/miniupnpd

# !!TB
shared: busybox

vsftpd: shared openssl
	@$(SEP)
	$(MAKE) -C vsftpd

vsftpd-install:
	install -D vsftpd/vsftpd $(INSTALLDIR)/vsftpd/usr/sbin/vsftpd
	$(STRIP) -s $(INSTALLDIR)/vsftpd/usr/sbin/vsftpd

ntfs-3g/Makefile:
	cd ntfs-3g && \
	CC=$(CC) CFLAGS="-Os $(EXTRACFLAGS)" $(CONFIGURE) --enable-shared=no --enable-static=no \
		--disable-library --disable-ldconfig --disable-mount-helper --with-fuse=internal \
		--disable-posix-acls --disable-nfconv --disable-dependency-tracking

ntfs-3g: ntfs-3g/Makefile
	@$(MAKE) -C ntfs-3g CFLAGS="-Os -Wall $(EXTRACFLAGS)"

ntfs-3g-clean:
	@$(MAKE) -C ntfs-3g clean || true

ntfs-3g-install: ntfs-3g
	install -D ntfs-3g/src/ntfs-3g $(INSTALLDIR)/ntfs-3g/bin/ntfs-3g
	$(STRIP) -s $(INSTALLDIR)/ntfs-3g/bin/ntfs-3g
	install -d $(INSTALLDIR)/ntfs-3g/sbin && cd $(INSTALLDIR)/ntfs-3g/sbin && \
		ln -sf ../bin/ntfs-3g mount.ntfs-3g && \
		ln -sf ../bin/ntfs-3g mount.ntfs

libusb10/Makefile: libusb10/Makefile.in
	cd libusb10 && CFLAGS="-Os -Wall $(EXTRACFLAGS)" LIBS="-lpthread" \
	$(CONFIGURE) --prefix=/usr ac_cv_lib_rt_clock_gettime=no

libusb10: libusb10/Makefile
	$(MAKE) -C $@

libusb10-install: libusb10
	install -D libusb10/libusb/.libs/libusb-1.0.so.0.0.0 $(INSTALLDIR)/libusb10/usr/lib/libusb-1.0.so.0
	$(STRIP) $(INSTALLDIR)/libusb10/usr/lib/*.so.*
	cd $(INSTALLDIR)/libusb10/usr/lib && \
		ln -sf libusb-1.0.so.0 libusb-1.0.so.0.0.0 && \
		ln -sf libusb-1.0.so.0 libusb-1.0.so

libusb/Makefile: libusb/Makefile.in
	cd libusb && CFLAGS="-Wall -Os $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr \
		LIBUSB_1_0_CFLAGS="-I$(TOP)/libusb10/libusb" \
		LIBUSB_1_0_LIBS="-L$(TOP)/libusb10/libusb/.libs -lusb-1.0 -lpthread \
		-Wl,-R/lib:/usr/lib:/opt/usr/lib:/usr/local/share"

libusb: libusb10 libusb/Makefile
	$(MAKE) -C $@

libusb-install: libusb
	install -D libusb/libusb/.libs/libusb-0.1.so.4.4.4 $(INSTALLDIR)/libusb/usr/lib/libusb-0.1.so.4
	$(STRIP) $(INSTALLDIR)/libusb/usr/lib/*.so.*
	cd $(INSTALLDIR)/libusb/usr/lib && \
		ln -sf libusb-0.1.so.4 libusb-0.1.so.4.4.4 && \
		ln -sf libusb-0.1.so.4 libusb.so

usbmodeswitch: libusb
	$(MAKE) -C $@ CC=$(CC) CCFLAGS="-Wall -Os $(EXTRACFLAGS) \
	    -Wl,-R/lib:/usr/lib:/opt/usr/lib:/usr/local/share \
	    -I$(TOP)/libusb10/libusb -L$(TOP)/libusb10/libusb/.libs -lusb-1.0 \
	    -I$(TOP)/libusb/libusb -L$(TOP)/libusb/libusb/.libs -lusb -lpthread"

usbmodeswitch-install:
	install -D usbmodeswitch/usb_modeswitch $(INSTALLDIR)/usbmodeswitch/usr/sbin/usb_modeswitch
	$(STRIP) -s $(INSTALLDIR)/usbmodeswitch/usr/sbin/usb_modeswitch

p910nd:
sendmail:
samba:
samba3:

nvram: shared

prebuilt: shared

vlan:
	@$(SEP)
	@$(MAKE) -C vlan CROSS=$(CROSS_COMPILE)	# STRIPTOOL=$(STRIP)

vlan-install:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) INSTALLDIR=$(INSTALLDIR) install	# STRIPTOOL=$(STRIP)
	$(STRIP) $(INSTALLDIR)/vlan/usr/sbin/vconfig


pptp-client-install:
	install -D pptp-client/pptp $(INSTALLDIR)/pptp-client/usr/sbin/pptp
	$(STRIP) $(INSTALLDIR)/pptp-client/usr/sbin/pptp


pppd/Makefile: pppd/linux/Makefile.top
	cd pppd && $(CONFIGURE) --prefix=/usr --sysconfdir=/tmp

pppd: pppd/Makefile
	@$(SEP)
	@$(MAKE) -C pppd

pppd-clean:
	$(MAKE) -C pppd clean

pppd-install:
	install -D pppd/pppd/pppd $(INSTALLDIR)/pppd/usr/sbin/pppd
	$(STRIP) $(INSTALLDIR)/pppd/usr/sbin/pppd
	install -D pppd/pppd/plugins/pppol2tp/pppol2tp.so $(INSTALLDIR)/pppd/usr/lib/pppd/pppol2tp.so
	$(STRIP) $(INSTALLDIR)/pppd/usr/lib/pppd/*.so

#	ipupdate-install:
#		install -D ipupdate/ez-ipupdate $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate
#		$(STRIP) $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate


zebra:
	@$(SEP)
	cd zebra && rm -f config.cache && CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) CFLAGS="-Os -Wall $(EXTRACFLAGS)" $(CONFIGURE) \
		--sysconfdir=/etc \
		--enable-netlink --disable-ipv6 --disable-ripngd --disable-ospfd --disable-doc \
		--disable-ospf6d --disable-bgpd --disable-bgpd-announce
	@$(MAKE) -C zebra CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD)

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
	cd dropbear && CFLAGS="-O2 -Wall $(EXTRACFLAGS)" ac_cv_func_logout=no ac_cv_func_logwtmp=no $(CONFIGURE) --disable-zlib --enable-syslog --disable-lastlog --disable-utmp --disable-utmpx --disable-wtmp --disable-wtmpx --disable-pututline --disable-pututxline --disable-loginfunc --disable-largefile
	make -C dropbear clean

igmpproxy/src/Makefile: igmpproxy/src/Makefile.in
	cd igmpproxy && CFLAGS="-O2 -Wall $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr

igmpproxy: igmpproxy/src/Makefile
	@$(SEP)
	@$(MAKE) -C igmpproxy/src

igmpproxy-install:
	install -D igmpproxy/src/igmpproxy $(INSTALLDIR)/igmpproxy/usr/sbin/igmpproxy
	$(STRIP) $(INSTALLDIR)/igmpproxy/usr/sbin/igmpproxy

igmpproxy-clean:
	$(MAKE) -C igmpproxy/src clean
	rm -f igmpproxy/src/Makefile

udev:
	$(MAKE) -C $@ CROSS_COMPILE=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)" \
	    PROGRAMS=udevtrigger

udev-install:
	install -d $(INSTALLDIR)
	install -d $(TARGETDIR)
	$(MAKE) -C udev DESTDIR=$(INSTALLDIR) prefix=/udev install-udevtrigger

hotplug2:
	$(MAKE) -C $@ CROSS_COMPILE=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRA_CFLAGS)"

hotplug2-install:
	$(MAKE) -C hotplug2 install PREFIX=$(INSTALLDIR)/hotplug2 SUBDIRS=""
	$(MAKE) -C hotplug2/examples install PREFIX=$(INSTALLDIR)/hotplug2/rom KERNELVER=$(LINUX_KERNEL)

emf:
	$(MAKE) -C $(SRCBASE)/emf/emfconf CROSS=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)"

emf-install:
	$(MAKE) -C $(SRCBASE)/emf/emfconf INSTALLDIR=$(INSTALLDIR) install

igs:
	$(MAKE) -C $(SRCBASE)/emf/igsconf CROSS=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)"

igs-install:
	$(MAKE) -C $(SRCBASE)/emf/igsconf INSTALLDIR=$(INSTALLDIR) install


ebtables: dummy
	@make -C ebtables

ebtables-install: dummy
	install -D ebtables/ebtables $(INSTALLDIR)/ebtables/usr/sbin/ebtables

	mkdir -p $(INSTALLDIR)/ebtables/usr/lib/
	cp ebtables/*.so $(INSTALLDIR)/ebtables/usr/lib/
	cp ebtables/extensions/*.so $(INSTALLDIR)/ebtables/usr/lib/

ebtables-clean:
	make -C ebtables clean


lzo-install:
	install -D lzo/src/.libs/liblzo2.so $(INSTALLDIR)/lzo/usr/lib/liblzo2.so.2
	$(STRIP) $(INSTALLDIR)/lzo/usr/lib/liblzo2.so.2
	cd $(INSTALLDIR)/lzo/usr/lib && ln -sf liblzo2.so.2 liblzo2.so


openvpn-install:
	install -D openvpn/openvpn $(INSTALLDIR)/openvpn/usr/sbin/openvpn
	$(STRIP) $(INSTALLDIR)/openvpn/usr/sbin/openvpn
	chmod 0500 $(INSTALLDIR)/openvpn/usr/sbin/openvpn


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
