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

#TOMATO_EXPERIMENTAL=0

#export PARALLEL_BUILD :=
export PARALLEL_BUILD := -j`grep -c '^processor' /proc/cpuinfo`

#
#
#
SEP=`$(eval progress=$(shell echo $$(($(progress)+1))))` \
echo "\033[41;1m   $@ ${progress}$\/${totalSteps} \033[0m\033]2;$@ ${progress}$\/${totalSteps}\007"


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
obj-y += wwwAT
obj-y += bridge
obj-$(TCONFIG_DNSSEC) += nettle
obj-y += dnsmasq
obj-y += etc
# obj-y += vlan # use Busybox vconfig
obj-y += pppd
obj-y += rp-pppoe
obj-y += utils
obj-y += ntpc
obj-y += rstats
obj-y += cstats
obj-y += udpxy

ifneq ($(TCONFIG_TOR)$(TCONFIG_MEDIA_SERVER)$(TCONFIG_BBT),)
obj-y += zlib
endif
ifneq ($(TCONFIG_HTTPS)$(TCONFIG_BBT)$(TCONFIG_NGINX),)
obj-y += openssl
endif
ifneq ($(TCONFIG_TOR)$(TCONFIG_NFS)$(TCONFIG_BBT),)
obj-y += libevent
endif
ifneq ($(TCONFIG_MEDIA_SERVER)$(TCONFIG_NGINX),)
obj-y += sqlite
endif
ifneq ($(TCONFIG_NGINX)$(TCONFIG_NANO),)
obj-y += libncurses
endif

obj-$(TCONFIG_TOR) += tor
obj-$(TCONFIG_DNSCRYPT) += libsodium
obj-$(TCONFIG_DNSCRYPT) += dnscrypt
obj-$(TCONFIG_SNMP) += snmp
obj-$(TCONFIG_SDHC) += mmc

# !!TB - updated Broadcom Wireless driver
obj-y += et
obj-y += libbcmcrypto
obj-y += wlconf

obj-y += cyassl
obj-y += mssl
obj-y += mdu
obj-$(TCONFIG_RAID) += mdadm

obj-$(TCONFIG_NFS) += portmap
obj-$(TCONFIG_NFS) += e2fsprogs
obj-$(TCONFIG_NFS) += libnfsidmap
obj-$(TCONFIG_NFS) += nfs-utils

#Roadkill
obj-$(TCONFIG_NOCAT) += nocat

# !!TB
obj-$(TCONFIG_USB) += p910nd
obj-$(TCONFIG_USB) += comgt
obj-$(TCONFIG_USB) += uqmi

obj-$(TCONFIG_UPS) += apcupsd

ifeq ($(CONFIG_LINUX26),y)
obj-$(TCONFIG_USB) += sd-idle
else
obj-$(TCONFIG_USB) += scsi-idle
endif

obj-y += libusb10
#obj-y += libusb
obj-y += usbmodeswitch
obj-$(TCONFIG_FTP) += vsftpd

ifeq ($(CONFIG_LINUX26),y)
ifeq ($(TCONFIG_SAMBASRV),y)
NEED_EX_NLS = y
endif
ifeq ($(TCONFIG_USB_EXTRAS),y)
NEED_EX_USB = y
endif
ifeq ($(TCONFIG_MICROSD),y)
NEED_SD_MODULES = y
endif
endif

ifeq ($(TCONFIG_SAMBASRV),y)
ifeq ($(TCONFIG_SAMBA3),y)
NEED_SAMBA3 = y
else
NEED_SAMBA2 = y
endif
endif

ifeq ($(TCONFIG_IPV6),y)
export TCONFIG_IPV6 := y
else
TCONFIG_IPV6 :=
endif

ifeq ($(TCONFIG_IPSEC),y)
export TCONFIG_IPSEC := y
else
TCONFIG_IPSEC :=
endif

ifeq ($(TCONFIG_RAID),y)
export TCONFIG_RAID := y
else
TCONFIG_RAID :=
endif

obj-$(NEED_SAMBA2) += samba
obj-$(NEED_SAMBA3) += samba3

ifeq ($(CONFIG_BCMWL6),y)
ifeq ($(TCONFIG_UFSD),y)
obj-$(TCONFIG_NTFS) += ufsd
else
obj-$(TCONFIG_NTFS) += ntfs-3g
endif
else
obj-$(TCONFIG_NTFS) += ntfs-3g
endif

obj-$(TCONFIG_EBTABLES) += ebtables
#obj-$(TCONFIG_IPV6) += libdaemon
#obj-$(TCONFIG_IPV6) += radvd
obj-$(TCONFIG_IPV6) += dhcpv6

obj-$(TCONFIG_MEDIA_SERVER) += ffmpeg
#obj-$(TCONFIG_MEDIA_SERVER) += libiconv
obj-$(TCONFIG_MEDIA_SERVER) += libogg
obj-$(TCONFIG_MEDIA_SERVER) += flac
obj-$(TCONFIG_MEDIA_SERVER) += jpeg
obj-$(TCONFIG_MEDIA_SERVER) += libexif
obj-$(TCONFIG_MEDIA_SERVER) += libid3tag
obj-$(TCONFIG_MEDIA_SERVER) += libvorbis
obj-$(TCONFIG_MEDIA_SERVER) += minidlna
MEDIA_SERVER_STATIC=y
#MEDIA_SERVER_STATIC :=

#obj-y += libnfnetlink
obj-y += miniupnpd
#	obj-y += upnp
obj-$(CONFIG_LINUX26) += ipset


#
# configurable packages
#
obj-$(TCONFIG_L2TP) += xl2tpd
obj-$(TCONFIG_PPTP) += accel-pptp
obj-$(TCONFIG_PPTPD) += pptpd
obj-$(TCONFIG_SSH) += dropbear
obj-$(TCONFIG_ZEBRA) += zebra
#	obj-$(TCONFIG_IPP2P) += ipp2p
obj-$(TCONFIG_LZO) += lzo
obj-$(TCONFIG_OPENVPN) += openvpn
obj-$(TCONFIG_OPENVPN) += openvpn_plugin_auth_nvram
obj-$(TCONFIG_TINC) += tinc
obj-$(TCONFIG_EMF) += emf
obj-$(TCONFIG_EMF) += igs

# Tomato RAF
# additional modules for nginx
ifeq (obj-$(TCONFIG_NGINX),y)
ifndef ($(ADDITIONAL_MODULES))
ADDITIONAL_MODULES:=
else
ifeq ($(TCONFIG_IPV6),y)
ADDITIONAL_MODULES += --with-ipv6
endif
endif
endif
obj-$(TCONFIG_NGINX) += mysql
obj-$(TCONFIG_NGINX) += spawn-fcgi
obj-$(TCONFIG_NGINX) += pcre
obj-$(TCONFIG_NGINX) += libatomic_ops
obj-$(TCONFIG_NGINX) += libiconv
obj-$(TCONFIG_NGINX) += libxml2
obj-$(TCONFIG_NGINX) += libpng
obj-$(TCONFIG_NGINX) += jpeg
obj-$(TCONFIG_NGINX) += php
obj-$(TCONFIG_NGINX) += nginx

obj-$(TCONFIG_NANO) += nano

obj-$(TCONFIG_BBT) += libcurl
obj-$(TCONFIG_BBT) += transmission
obj-$(TCONFIG_BT) += btgui

obj-$(CONFIG_LINUX26) += hotplug2
obj-$(CONFIG_LINUX26) += udevtrigger

ifneq ($(TCONFIG_OPENVPN)$(TCONFIG_FTP_SSL)$(TCONFIG_BBT)$(TCONFIG_TOR)$(TCONFIG_TINC),)
    export FULL_OPENSSL := y
else
    FULL_OPENSSL :=
endif

obj-y += wanuptime

obj-clean := $(foreach obj, $(obj-y) $(obj-n) $(obj-), $(obj)-clean)
obj-install := $(foreach obj,$(obj-y),$(obj)-install)



#
# Basic rules
#

all: countSteps clean-build libc $(obj-y) kernel

countSteps:
	@totalSteps=0
	@progress=0
	$(foreach n, $(obj-y) $(obj-n) $(obj-), $(eval totalSteps=$(shell echo $$(($(totalSteps)+1)))))
	@echo ${totalSteps}

kernel: $(LINUXDIR)/.config
	@$(SEP)

	@if ! grep -q "CONFIG_EMBEDDED_RAMDISK=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) zImage CC=$(KERNELCC) $(PARALLEL_BUILD); \
	fi
	if grep -q "CONFIG_MODULES=y" $(LINUXDIR)/.config ; then \
	    $(MAKE) -C $(LINUXDIR) modules CC=$(KERNELCC) $(PARALLEL_BUILD); \
	fi
ifeq ($(CONFIG_LINUX26),y)
	$(MAKE) -C $(LINUXDIR)/arch/mips/brcm-boards/bcm947xx/compressed srctree=$(LINUXDIR) $(PARALLEL_BUILD)
endif


lzma-loader:
	$(MAKE) -C $(SRCBASE)/lzma-loader CROSS_COMPILE=$(CROSS_COMPILE) LD=$(LD) $(PARALLEL_BUILD)

lzma-loader-install: lzma-loader
	@$(SEP)


kmod: dummy
	$(MAKE) -C $(LINUXDIR) modules CC=$(KERNELCC) $(PARALLEL_BUILD)

testfind:
	cd $(TARGETDIR)/lib/modules/* && find -name "*.o" -exec mv -i {} . \; || true
	cd $(TARGETDIR)/lib/modules/* && find -type d -delete || true

install package: $(obj-install) $(LINUXDIR)/.config
	@$(SEP)

	install -d $(TARGETDIR)


# kernel modules
	$(MAKE) -C $(LINUXDIR) modules_install \
	INSTALL_MOD_STRIP="--strip-debug -x -R .comment -R .note -R .pdr -R .mdebug.abi32 -R .note.gnu.build-id -R .gnu.attributes -R .reginfo" \
	DEPMOD=/bin/true INSTALL_MOD_PATH=$(TARGETDIR)

ifneq ($(CONFIG_LINUX26),y)
	find $(TARGETDIR)/lib/modules -name wl.*o -exec $(STRIP) --strip-unneeded -x {} \;
	find $(TARGETDIR)/lib/modules -name et.*o -exec $(STRIP) --strip-unneeded -x {} \;
	find $(TARGETDIR)/lib/modules -name bcm57*.*o -exec $(STRIP) --strip-unneeded -x {} \;
	find $(TARGETDIR)/lib/modules -name ctf.*o -exec $(STRIP) --strip-unneeded -x {} \;
	find $(TARGETDIR)/lib/modules -name emf.*o -exec $(STRIP) --strip-unneeded -x {} \;
	find $(TARGETDIR)/lib/modules -name igs.*o -exec $(STRIP) --strip-unneeded -x {} \;
	find $(TARGETDIR)/lib/modules -name jffs*.*o -exec $(STRIP) --strip-unneeded -x {} \;

	find $(TARGETDIR)/lib/modules -name *.*o -exec $(STRIP) --strip-debug -x -R .mdebug.abi32 {} \;
endif

# nice and clean
ifneq ($(TCONFIG_USBAP),y)
	-cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv diag/* . && rm -rf diag
	-cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv et.4702/* . && rm -rf et.4702 || true
	-cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv usb/* . && rm -rf usb
endif
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
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv jfs/* . && rm -rf jfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv vfat/* . && rm -rf vfat || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv msdos/* . && rm -rf msdos || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv fuse/* . && rm -rf fuse || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv ntfs/* . && rm -rf ntfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv smbfs/* . && rm -rf smbfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv reiserfs/* . && rm -rf reiserfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv hfs/* . && rm -rf hfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv hfsplus/* . && rm -rf hfsplus || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv lockd/* . && rm -rf lockd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv nfsd/* . && rm -rf nfsd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv nfs/* . && rm -rf nfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv xfs/* . && rm -rf xfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv nls/* . && rm -rf nls || true
	cd $(TARGETDIR)/lib/modules/*/kernel/fs && mv exportfs/* . && rm -rf exportfs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/net && mv sunrpc/* . && rm -rf sunrpc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/net && mv auth_gss/* . && rm -rf auth_gss || true
	cd $(TARGETDIR)/lib/modules/*/kernel/sound/core && mv oss/* . && rm -rf oss || true
	cd $(TARGETDIR)/lib/modules/*/kernel/sound/core && mv seq/* . && rm -rf seq || true
	cd $(TARGETDIR)/lib/modules/*/kernel/sound && mv core/* . && rm -rf core || true
	cd $(TARGETDIR)/lib/modules/*/kernel/sound && mv usb/* . && rm -rf usb || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv hcd/* . && rm -rf hcd || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv host/* . && rm -rf host || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv storage/* . && rm -rf storage || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv serial/* . && rm -rf serial || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv core/* . && rm -rf core || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv class/* . && rm -rf class || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv misc/* . && rm -rf misc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/usb && mv usbip/* . && rm -rf usbip || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/mmc && mv core/* . && rm -rf core || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/mmc && mv card/* . && rm -rf card || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/mmc && mv host/* . && rm -rf host || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/hid && mv usbhid/* . && rm -rf usbhid || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/input && mv joystick/* . && rm -rf joystick || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/input && mv keyboard/* . && rm -rf keyboard || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/input && mv misc/* . && rm -rf misc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/input && mv mouse/* . && rm -rf mouse || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media/video && mv uvc/* . && rm -rf uvc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media/video && mv pwc/* . && rm -rf pwc || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media/video/gspca && mv gl860/* . && rm -rf gl860 || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media/video/gspca && mv m5602/* . && rm -rf m5602 || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media/video/gspca && mv stv06xx/* . && rm -rf stv06xx || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media/video && mv gspca/* . && rm -rf gspca || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/media && mv video/* . && rm -rf video || true

	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv bcm57xx/* . && rm -rf bcm57xx || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv emf/* . && rm -rf emf || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv igs/* . && rm -rf igs || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv ctf/* . && rm -rf ctf || true
	cd $(TARGETDIR)/lib/modules/*/kernel/drivers/net && mv usb/* . && rm -rf usb || true
	cd $(TARGETDIR)/lib/modules && rm -f */source || true

# misc
	for dir in $(wildcard $(patsubst %,$(INSTALLDIR)/%,$(obj-y))) ; do \
	    (cd $${dir} && tar cpf - .) | (cd $(TARGETDIR) && tar xpf -) \
	done

ifneq ($(TCONFIG_L7),y)
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
ifeq ($(TCONFIG_BBT),y)
	install $(LIBDIR)/librt-0.9.30.1.so $(TARGETDIR)/lib/librt.so.0
endif
ifeq ($(TCONFIG_NGINX),y)
	install $(LIBDIR)/libstdc++.so.6 $(TARGETDIR)/lib/libstdc++.so.6
	cd $(TARGETDIR)/lib && ln -sf libstdc++.so.6 libstdc++.so
	$(STRIP) $(TARGETDIR)/lib/libstdc++.so.6
endif
ifneq ($(TCONFIG_OPTIMIZE_SHARED_LIBS),y)
	install $(LIBDIR)/libresolv.so.0 $(TARGETDIR)/lib/
	$(STRIP) $(TARGETDIR)/lib/*.so.0
endif

	@cd $(TARGETDIR) && $(TOP)/others/rootprep.sh

ifeq ($(TCONFIG_TOR),y)
		@cd $(TARGETDIR) && $(TOP)/others/rootprep.sh ln_Tor
endif

	@echo ---

ifeq ($(TCONFIG_OPTIMIZE_SHARED_LIBS),y)
	@$(SRCBASE)/btools/libfoo.pl
else
	@$(SRCBASE)/btools/libfoo.pl --noopt
endif
	@chmod 0555 $(TARGETDIR)/lib/*.so*
	@chmod 0555 $(TARGETDIR)/usr/lib/*.so*

# !!TB - moved to run after libfoo.pl - to make sure shared libs include all symbols needed by extras
# separated/copied extra stuff
	@rm -rf $(PLATFORMDIR)/extras
	@mkdir $(PLATFORMDIR)/extras
	@mkdir $(PLATFORMDIR)/extras/ipsec
	@mkdir $(PLATFORMDIR)/extras/raid
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/ipv4/ip_gre.*o $(PLATFORMDIR)/extras/ || true
#	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/usr/lib/iptables/libipt_policy.*o $(PLATFORMDIR)/extras/ipsec/ || true

	$(if $(TCONFIG_OPENVPN),@cp -f,$(if $(TCONFIG_USB_EXTRAS),@cp -f,$(if $(TCONFIG_IPV6),@cp -f,@mv))) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/tun.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_EBTABLES),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/bridge/netfilter/ebt*.*o $(PLATFORMDIR)/extras/ || true

	$(if $(TCONFIG_RAID),@cp -f,@mv) $(TARGETDIR)/usr/sbin/mdadm $(PLATFORMDIR)/extras/raid/ || true
	$(if $(TCONFIG_RAID),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/md/*.ko $(PLATFORMDIR)/extras/raid/ || true
	$(if $(TCONFIG_RAID),@ls,@rm -rf) $(TARGETDIR)/lib/modules/*/kernel/drivers/md || true

	@cp $(TARGETDIR)/lib/modules/*/kernel/net/ipv4/netfilter/ip_set*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/ifb.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/sched/sch_red.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/ntfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/smbfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/reiserfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/jfs.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_NFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nfs.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_NFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nfsd.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_NFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/lockd.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_NFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/exportfs.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_NFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/sunrpc.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/auth_rpcgss.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/net/rpcsec_gss_krb5.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/fs/xfs.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/scsi/sr_mod.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/scanner.*o $(PLATFORMDIR)/extras/ || true

	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/usbserial.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/option.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/sierra.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/cdc-acm.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/mii.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/cdc_*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usbnet.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/huawei_ether.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/cdc_*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/usbnet.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/huawei_ether.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ftdi_sio.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/pl2303.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/class/cdc-wdm.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/qmi_wwan.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_SD_MODULES),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/mmc/*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_SD_MODULES),@ls,@rm -rf) $(TARGETDIR)/lib/modules/*/kernel/drivers/mmc || true

	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ch341.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/usbip*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/usbkbd.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/usbmouse.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/hid*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ipw.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/audio.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/ov51*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/pwc*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/emi*.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/cdc_subset.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/rndis_host.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/ipheth.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/cdc_subset.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/rndis_host.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb/ipheth.*o $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/net/usb || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/media/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/media || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/sound/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/sound || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/sound/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/sound || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/input/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/input || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/hid/* $(PLATFORMDIR)/extras/ || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/hid || true
	@cp -f $(TARGETDIR)/lib/modules/*/kernel/drivers/net/bcm57*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_CTF),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/ctf*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_PPTP),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/pptp.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_L2TP),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/net/pppol2tp.*o $(PLATFORMDIR)/extras/ || true
	@mv $(TARGETDIR)/lib/modules/*/kernel/drivers/net/ppp_deflate.*o  $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/crypto/*.ko $(PLATFORMDIR)/extras/ipsec/ || true
	$(if $(TCONFIG_IPSEC),@ls,@rm -rf) $(TARGETDIR)/lib/modules/*/kernel/crypto || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/xfrm/*.ko $(PLATFORMDIR)/extras/ipsec/ || true
	$(if $(TCONFIG_IPSEC),@ls,@rm -rf) $(TARGETDIR)/lib/modules/*/kernel/net/xfrm || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/key/*.ko $(PLATFORMDIR)/extras/ipsec/ || true
	$(if $(TCONFIG_IPSEC),@ls,@rm -rf) $(TARGETDIR)/lib/modules/*/kernel/net/key || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/ipv*/xfrm*.ko $(PLATFORMDIR)/extras/ipsec/ || true
#	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/ipv*/tunnel*.ko $(PLATFORMDIR)/extras/ipsec/ || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/ipv*/ah*.ko $(PLATFORMDIR)/extras/ipsec/ || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/ipv*/esp*.ko $(PLATFORMDIR)/extras/ipsec/ || true
	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/ipv*/ipcomp*.ko $(PLATFORMDIR)/extras/ipsec/ || true
#	$(if $(TCONFIG_IPSEC),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/netfilter/xt_policy.ko $(PLATFORMDIR)/extras/ipsec/ || true

	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_cp9*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_cp1251.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_euc-jp.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_sjis.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_gb2312.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_euc-kr.*o $(PLATFORMDIR)/extras/ || true
	$(if $(NEED_EX_NLS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_big5.*o $(PLATFORMDIR)/extras/ || true

	$(if $(TCONFIG_USB),@cp -f,$(if $(TCONFIG_SDHC),@cp -f,@mv)) $(TARGETDIR)/lib/modules/*/kernel/fs/nls_*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/usb/*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/scsi/*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/leds/*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,$(if $(TCONFIG_SDHC),@cp -f,@mv)) $(TARGETDIR)/lib/modules/*/kernel/fs/ext2.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/ext3.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,$(if $(TCONFIG_SDHC),@cp -f,@mv)) $(TARGETDIR)/lib/modules/*/kernel/fs/jbd.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/mbcache.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,$(if $(TCONFIG_SDHC),@cp -f,@mv)) $(TARGETDIR)/lib/modules/*/kernel/fs/fat.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,$(if $(TCONFIG_SDHC),@cp -f,@mv)) $(TARGETDIR)/lib/modules/*/kernel/fs/vfat.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/msdos.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/fuse.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_HFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/hfs.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_HFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/hfsplus.*o $(PLATFORMDIR)/extras/ || true

ifneq ($(TCONFIG_USB),y)
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/usb || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/scsi || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/leds || true
endif

	$(if $(TCONFIG_USB_EXTRAS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/connector/cn.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_USB_EXTRAS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/drivers/block/loop.*o $(PLATFORMDIR)/extras/ || true
ifneq ($(TCONFIG_USB_EXTRAS),y)
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/connector || true
	@rm -rf $(TARGETDIR)/lib/modules/*/kernel/drivers/block || true
endif
	$(if $(TCONFIG_CIFS),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/cifs.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_JFFS2),$(if $(TCONFIG_JFFSV1),@mv,@cp -f),@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/jffs2.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_JFFS2),$(if $(TCONFIG_JFFSV1),@mv,@cp -f),@mv) $(TARGETDIR)/lib/modules/*/kernel/lib/zlib_*.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_JFFS2),$(if $(TCONFIG_JFFSV1),@cp -f,@mv),@mv) $(TARGETDIR)/lib/modules/*/kernel/fs/jffs.*o $(PLATFORMDIR)/extras/ || true
	[ ! -f $(TARGETDIR)/lib/modules/*/kernel/lib/* ] && rm -rf $(TARGETDIR)/lib/modules/*/kernel/lib || true
	$(if $(TCONFIG_L7),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/ipv4/netfilter/ipt_layer7.*o $(PLATFORMDIR)/extras/ || true
	$(if $(TCONFIG_L7),@cp -f,@mv) $(TARGETDIR)/lib/modules/*/kernel/net/netfilter/xt_layer7.*o $(PLATFORMDIR)/extras/ || true

	@mkdir -p $(PLATFORMDIR)/extras/apps
	@mkdir -p $(PLATFORMDIR)/extras/lib

	@mv $(TARGETDIR)/usr/sbin/ttcp $(PLATFORMDIR)/extras/apps/ || true
	@mv $(TARGETDIR)/usr/sbin/mii-tool $(PLATFORMDIR)/extras/apps/ || true
	@cp -r $(TARGETDIR)/usr/sbin/robocfg $(PLATFORMDIR)/extras/apps/ || true

	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/usr/lib/libusb* $(PLATFORMDIR)/extras/lib/ || true
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/usr/sbin/usb_modeswitch $(PLATFORMDIR)/extras/apps/ || true
	@cp usbmodeswitch/usb_modeswitch.conf $(PLATFORMDIR)/extras/apps/usb_modeswitch.conf || true
	@cp usbmodeswitch/usb_modeswitch.setup $(PLATFORMDIR)/extras/apps/usb_modeswitch.setup || true
	@mkdir -p $(PLATFORMDIR)/extras/apps/usb_modeswitch.d
	@cp -f usbmodeswitch/data/usb_modeswitch.d/* $(PLATFORMDIR)/extras/apps/usb_modeswitch.d || true
ifneq ($(NEED_EX_USB),y)
	@rm -rf $(TARGETDIR)/rom/etc/usb_modeswitch.d || true
	@rm -f  $(TARGETDIR)/rom/etc/usb_modeswitch.conf || true
endif
	$(if $(NEED_EX_USB),@cp -f,@mv) $(TARGETDIR)/usr/sbin/chat $(PLATFORMDIR)/extras/apps/ || true

	@mkdir -p $(TARGETDIR)/rom/etc/l7-protocols
ifeq ($(TCONFIG_L7PAT),y)
	@cd layer7 && ./squish.sh
	cp layer7/squished/*.pat $(TARGETDIR)/rom/etc/l7-protocols
endif

ifeq ($(TCONFIG_PPTPD),y)
	@mkdir -p $(TARGETDIR)/rom/etc/vpn
	cp rc/vpn/* $(TARGETDIR)/rom/etc/vpn
	chmod +x $(TARGETDIR)/rom/etc/vpn/*
endif

	busybox/examples/depmod.pl -k $(LINUXDIR)/vmlinux -b $(TARGETDIR)/lib/modules/*/
	@mv $(TARGETDIR)/lib/modules/*/modules.dep $(TARGETDIR)/lib/modules/
	@echo ---

	@rm -f $(TARGETDIR)/lib/modules/*/build

	@$(MAKE) -C $(LINUXDIR)/scripts/squashfs mksquashfs-lzma
	@$(LINUXDIR)/scripts/squashfs/mksquashfs-lzma $(TARGETDIR) $(PLATFORMDIR)/target.image -all-root -noappend -no-duplicates | tee target.info

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
	rm -f .ipv6-y .ipv6-n
	rm -f .fullssl-y .fullssl-n
	make -C config clean

clean-build: dummy
	rm -rf $(TARGETDIR)
	rm -rf $(INSTALLDIR)
	rm -f $(PLATFORMDIR)/linux.trx $(PLATFORMDIR)/vmlinuz $(PLATFORMDIR)/target.image
	rm -rf $(PLATFORMDIR)/extras

distclean: clean
ifneq ($(INSIDE_MAK),1)
	$(MAKE) -C $(SRCBASE) $@ INSIDE_MAK=1
endif
#	-rm -f $(LIBDIR)/*.so.0  $(LIBDIR)/*.so

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
	@$(MAKE) shared-clean rc-clean nvram-clean httpd-clean prebuilt-clean libbcmcrypto-clean dhcpv6-clean

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

.ipv6-y .ipv6-n:
	@rm -f .ipv6-y .ipv6-n
	@$(MAKE) iptables-clean ebtables-clean pppd-clean zebra-clean dnsmasq-clean iproute2-clean
	@touch $@

.fullssl-y .fullssl-n:
	@rm -f .fullssl-y .fullssl-n
	@$(MAKE) openssl-clean vsftpd-clean mssl-clean mdu-clean httpd-clean
	@touch $@

dependconf: .ipv6-$(if $(TCONFIG_IPV6),y,n) .fullssl-$(if $(FULL_OPENSSL),y,n)

oldconfig oldconf: koldconf roldconf dependconf bboldconf


#
# overrides and extra dependencies
#

busybox: dummy
	@$(MAKE) -C busybox EXTRA_CFLAGS="-fPIC $(EXTRACFLAGS)" $(PARALLEL_BUILD)

busybox-install:
	rm -rf $(INSTALLDIR)/busybox
	$(MAKE) -C busybox install EXTRA_CFLAGS="-fPIC $(EXTRACFLAGS)" CONFIG_PREFIX=$(INSTALLDIR)/busybox

busybox-clean:
	-@$(MAKE) -C busybox distclean

busybox-config:
	$(MAKE) -C busybox menuconfig


httpd: shared nvram mssl
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
	-@$(MAKE) -C matrixssl/src clean

cyassl/stamp-h1:
	@cd cyassl && CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) \
	CFLAGS="-Os -Wall -fPIC $(EXTRACFLAGS) -ffunction-sections -fdata-sections \
	-DNO_MD4 -DNO_AES -DNO_ERROR_STRINGS -DNO_HC128 -DNO_RABBIT -DNO_PSK -DNO_DSA -DNO_DH -DNO_PWDBASED" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	PTHREAD_LIBS="-lpthread" \
	$(CONFIGURE)
	@touch cyassl/stamp-h1

cyassl: cyassl/stamp-h1
	@$(SEP)
	@$(MAKE) -C cyassl

cyassl-clean:
	-@$(MAKE) -C cyassl clean
	@rm -f cyassl/stamp-h1

cyassl-install:
	@true

ifeq ($(FULL_OPENSSL),y)
OPENSSL_CIPHERS:=enable-rc5
else
OPENSSL_CIPHERS:=no-dh no-idea no-rc2 no-rc5 no-engine no-aes192 no-cast
endif

openssl/stamp-h1:
	cd openssl && \
	./Configure linux-mipsel --openssldir=/etc --cross-compile-prefix=' ' \
	-ffunction-sections -fdata-sections -Wl,--gc-sections \
	shared $(OPENSSL_CIPHERS) \
	no-sha0 no-smime no-camellia no-krb5 no-rmd160 no-ripemd \
	no-seed no-capieng no-gms no-gmp no-rfc3779 \
	no-err no-hw no-jpake \
	no-zlib no-engines no-sse2 no-libunbound no-ssl-trace \
	no-dtls1 no-store no-psk no-md2 no-mdc2 no-ts

	@$(MAKE) -C openssl clean
	@touch openssl/stamp-h1

openssl: openssl/stamp-h1
	@$(SEP)
	@$(MAKE) -C openssl $(PARALLEL_BUILD)

openssl-clean:
	-@$(MAKE) -C openssl clean
	@rm -f openssl/stamp-h1

openssl-install: openssl
	install -D openssl/libcrypto.so.1.0.0 $(INSTALLDIR)/openssl/usr/lib/libcrypto.so.1.0.0
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libcrypto.so.1.0.0
	cd $(INSTALLDIR)/openssl/usr/lib && ln -sf libcrypto.so.1.0.0 libcrypto.so

	install -D openssl/apps/openssl $(INSTALLDIR)/openssl/usr/sbin/openssl
	$(STRIP) $(INSTALLDIR)/openssl/usr/sbin/openssl
	chmod 0500 $(INSTALLDIR)/openssl/usr/sbin/openssl

	install -D -m 0500 httpd/gencert.sh $(INSTALLDIR)/openssl/usr/sbin/gencert.sh

#	perl -e 'while (<>) { s/.SECS/time()-(24*60*60)/e; print; }' < httpd/gencert.sh > $(INSTALLDIR)/openssl/usr/sbin/gencert.sh
#	chmod 0500 $(INSTALLDIR)/openssl/usr/sbin/gencert.sh

#ifeq ($(FULL_OPENSSL),y)
	install -D openssl/libssl.so.1.0.0 $(INSTALLDIR)/openssl/usr/lib/libssl.so.1.0.0
	$(STRIP) $(INSTALLDIR)/openssl/usr/lib/libssl.so.1.0.0
	cd $(INSTALLDIR)/openssl/usr/lib && ln -sf libssl.so.1.0.0 libssl.so
#endif

mssl:	$(if $(FULL_OPENSSL),openssl,cyassl)

mdu:	shared mssl

rc: nvram shared

bridge/Makefile:
	cd bridge && CFLAGS="-Os -g $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix="" --with-linux-headers=$(LINUXDIR)/include

bridge: bridge/Makefile
	@$(SEP)
	@$(MAKE) -C bridge

bridge-clean:
	-@$(MAKE) -C bridge clean
	@rm -f bridge/Makefile

bridge-install:
	install -D bridge/brctl/brctl $(INSTALLDIR)/bridge/usr/sbin/brctl
	$(STRIP) $(INSTALLDIR)/bridge/usr/sbin/brctl

dnsmasq: $(if $(TCONFIG_DNSSEC),nettle,)
	@$(SEP)
	@$(MAKE) -C dnsmasq $(PARALLEL_BUILD) \
	COPTS="$(if $(TCONFIG_DNSSEC),-I$(TOP)/nettle/include -I$(TOP)/gmp -DHAVE_DNSSEC -DHAVE_DNSSEC_STATIC,) \
		$(if $(TCONFIG_IPV6),-DUSE_IPV6,-DNO_IPV6) \
		$(if $(CONFIG_LINUX26),-DUSE_IPSET,-DNO_IPSET) \
		-DNO_INOTIFY -DHAVE_BROKEN_RTC -DHAVE_TOMATO $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="$(if $(TCONFIG_DNSSEC),-L$(TOP)/nettle/lib -L$(TOP)/gmp/.libs,) -ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	$(if $(TCONFIG_DNSSEC),PKG_CONFIG_PATH="$(TOP)/nettle/lib/pkgconfig",)

dnsmasq-install:
	install -D dnsmasq/src/dnsmasq $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq
	$(STRIP) $(INSTALLDIR)/dnsmasq/usr/sbin/dnsmasq

nettle/stamp-h1: gmp
	@cd nettle && CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) \
	CPPFLAGS="-I$(TOP)/gmp" \
	CFLAGS="-O2 -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	LDFLAGS="-L$(TOP)/gmp/.libs -ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	$(CONFIGURE) prefix=$(TOP)/nettle --disable-documentation
	@touch nettle/stamp-h1

nettle: nettle/stamp-h1
	@$(SEP)
	@$(MAKE) -C nettle
	@$(MAKE) -C nettle install

nettle-clean:
	-@$(MAKE) -C nettle clean
	@rm -f nettle/stamp-h1
	@rm -rf nettle/include nettle/lib nettle/bin nettle/share

gmp/stamp-h1:
	@cd gmp && CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	$(CONFIGURE)
	@touch gmp/stamp-h1

gmp: gmp/stamp-h1
	@$(SEP)
	@$(MAKE) -C gmp

gmp-clean:
	@$(MAKE) -C gmp clean
	@rm -f gmp/stamp-h1

iptables:
ifeq ($(CONFIG_LINUX26),y)
	cp -f iptables/extensions/libipt_ipp2p_K26.c iptables/extensions/libipt_ipp2p.c
else
	cp -f iptables/extensions/libipt_ipp2p_K24.c iptables/extensions/libipt_ipp2p.c
endif
	@$(SEP)
	$(MAKE) -C iptables BINDIR=/usr/sbin LIBDIR=/usr/lib KERNEL_DIR=$(LINUXDIR) COPT_FLAGS="-Os $(EXTRACFLAGS) -U CONFIG_NVRAM_SIZE"

iptables-install:
	install -D iptables/iptables $(INSTALLDIR)/iptables/usr/sbin/iptables
	cd $(INSTALLDIR)/iptables/usr/sbin && \
		ln -sf iptables iptables-restore && \
		ln -sf iptables iptables-save

	install -d $(INSTALLDIR)/iptables/usr/lib/iptables
	install -D iptables/extensions/*.so $(INSTALLDIR)/iptables/usr/lib/iptables/

	install -D iptables/libiptc.so $(INSTALLDIR)/iptables/usr/lib/libiptc.so

	$(STRIP) $(INSTALLDIR)/iptables/usr/sbin/iptables
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/iptables/*.so
	$(STRIP) $(INSTALLDIR)/iptables/usr/lib/libiptc.so

ifeq ($(TCONFIG_IPV6),y)
	install iptables/ip6tables $(INSTALLDIR)/iptables/usr/sbin/ip6tables
	$(STRIP) $(INSTALLDIR)/iptables/usr/sbin/ip6tables
	cd $(INSTALLDIR)/iptables/usr/sbin && \
		ln -sf ip6tables ip6tables-restore && \
		ln -sf ip6tables ip6tables-save
endif

iptables-clean:
	-@$(MAKE) -C iptables KERNEL_DIR=$(LINUXDIR) clean

ppp:
	@$(SEP)
	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp $(if $(TCONFIG_IPV6),HAVE_INET6=y,) $(PARALLEL_BUILD)
#	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp DFLAGS="-DDEBUG -DDEBUGALL"

ppp-%:
	$(MAKE) -C ppp/pppoecd $* INSTALLDIR=$(INSTALLDIR)/ppp $(if $(TCONFIG_IPV6),HAVE_INET6=y,)

rp-pppoe/src/stamp-h1: rp-pppoe/src/Makefile.in
	cd rp-pppoe/src && CFLAGS="-g -O2 $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr --enable-plugin=$(TOP)/pppd \
	ac_cv_linux_kernel_pppoe=yes rpppoe_cv_pack_bitfields=rev
	@touch rp-pppoe/src/stamp-h1

rp-pppoe: pppd rp-pppoe/src/stamp-h1
	$(MAKE) -C rp-pppoe/src pppoe-relay rp-pppoe.so $(PARALLEL_BUILD)

rp-pppoe-clean:
	-@$(MAKE) -C rp-pppoe/src clean
	@rm -f rp-pppoe/src/pppoe-relay
	@rm -f rp-pppoe/src/stamp-h1

rp-pppoe-install: rp-pppoe
	install -D rp-pppoe/src/rp-pppoe.so $(INSTALLDIR)/rp-pppoe/usr/lib/pppd/rp-pppoe.so
	$(STRIP) $(INSTALLDIR)/rp-pppoe/usr/lib/pppd/*.so
#	install -D rp-pppoe/src/pppoe-relay $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe-relay
#	$(STRIP) $(INSTALLDIR)/rp-pppoe/usr/sbin/pppoe-relay


upnp: nvram shared iptables

libnfnetlink/stamp-h1:
	cd $(TOP)/libnfnetlink && CC=$(CC) STRIP='mipsel-uclibc-strip' \
		CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" \
		$(CONFIGURE) --prefix=/usr --enable-shared --enable-static
	touch libnfnetlink/stamp-h1

libnfnetlink: libnfnetlink/stamp-h1
	@$(SEP)
	$(MAKE) -C libnfnetlink

libnfnetlink-install:
	install -D libnfnetlink/src/.libs/libnfnetlink.so.0.2.0 $(INSTALLDIR)/libnfnetlink/usr/lib/libnfnetlink.so.0.2.0
	$(STRIP) -s $(INSTALLDIR)/libnfnetlink/usr/lib/libnfnetlink.so.0.2.0
	cd $(INSTALLDIR)/libnfnetlink/usr/lib/ && \
		ln -sf libnfnetlink.so.0.2.0 libnfnetlink.so.0 && \
		ln -sf libnfnetlink.so.0.2.0 libnfnetlink.so

miniupnpd-config:
#ifeq ($(TCONFIG_IPV6),y)
#	@cd miniupnpd && ./genconfig.sh --leasefile --ipv6
#else
	@cd miniupnpd && ./genconfig.sh --leasefile --vendorcfg
#endif

miniupnpd: iptables miniupnpd-config
	@$(SEP)
	$(MAKE) -C miniupnpd miniupnpd -f Makefile.linux CC=$(CC) \
		CFLAGS="-Wall -Os -D_GNU_SOURCE $(EXTRACFLAGS) -I$(TOP)/iptables/include" \
		LDFLAGS="-L$(TOP)/iptables -liptc" LIBS="" $(PARALLEL_BUILD)

miniupnpd-clean:
	-@$(MAKE) -C miniupnpd -f Makefile.linux clean
	@rm -f miniupnpd/config.h

miniupnpd-install:
	install -D miniupnpd/miniupnpd $(INSTALLDIR)/miniupnpd/usr/sbin/miniupnpd
	$(STRIP) $(INSTALLDIR)/miniupnpd/usr/sbin/miniupnpd

# !!TB
shared: busybox

vsftpd: $(if $(TCONFIG_FTP_SSL),openssl,)
	@$(SEP)
	$(MAKE) -C vsftpd $(PARALLEL_BUILD)

vsftpd-install: vsftpd
	install -D vsftpd/vsftpd $(INSTALLDIR)/vsftpd/usr/sbin/vsftpd
	$(STRIP) -s $(INSTALLDIR)/vsftpd/usr/sbin/vsftpd

ufsd: kernel_header kernel
	@$(MAKE) -C ufsd all

ufsd-install: ufsd
	@$(MAKE) -C ufsd install INSTALLDIR=$(INSTALLDIR)/ufsd

ntfs-3g/Makefile:
	cd ntfs-3g && \
	CC=$(CC) CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
		$(CONFIGURE) --enable-shared=no --enable-static=no \
		--disable-library --disable-ldconfig --disable-mount-helper --with-fuse=internal \
		--disable-ntfsprogs --disable-crypto --without-uuid \
		--disable-posix-acls --disable-nfconv --disable-dependency-tracking

ntfs-3g: ntfs-3g/Makefile
	@$(MAKE) -C ntfs-3g $(PARALLEL_BUILD)

ntfs-3g-clean:
	-@$(MAKE) -C ntfs-3g clean
	@rm -f ntfs-3g/Makefile

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
	install -D libusb10/libusb/.libs/libusb-1.0.so $(INSTALLDIR)/libusb10/usr/lib/libusb-1.0.so
	$(STRIP) $(INSTALLDIR)/libusb10/usr/lib/*.so
	cd $(INSTALLDIR)/libusb10/usr/lib && \
		ln -sf libusb-1.0.so libusb-1.0.so.0

libusb10-clean:
	-@$(MAKE) -C $@ clean
	@rm -rf libusb10/Makefile

libusb/Makefile: libusb/Makefile.in
	cd libusb && CFLAGS="-Wall -Os $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr \
		LIBUSB_1_0_CFLAGS="-I$(TOP)/libusb10/libusb" \
		LIBUSB_1_0_LIBS="-L$(TOP)/libusb10/libusb/.libs -lusb-1.0 -lpthread \
		-Wl,-R/lib:/usr/lib:/opt/usr/lib:/usr/local/share"

libusb: libusb10 libusb/Makefile
	$(MAKE) -C $@

libusb-install: libusb
	install -D libusb/libusb/.libs/libusb-0.1.so $(INSTALLDIR)/libusb/usr/lib/libusb-0.1.so
	$(STRIP) $(INSTALLDIR)/libusb/usr/lib/*.so

libusb-clean:
	-@$(MAKE) -C $@ clean
	@rm -rf libusb/Makefile

usbmodeswitch: libusb10
	$(MAKE) -C $@ CC=$(CC) CFLAGS="-Os $(EXTRACFLAGS) -DLIBUSB10 \
	    -Wl,-R/lib:/usr/lib:/opt/usr/lib:/usr/local/share -lpthread \
	    -I$(TOP)/libusb10/libusb -L$(TOP)/libusb10/libusb/.libs -lusb-1.0"

usbmodeswitchdb-install:
	@mkdir -p $(TARGETDIR)/rom/etc/usb_modeswitch.d
	# compress whitespace
	@for D in $(wildcard $(TOP)/usbmodeswitch/data/usb_modeswitch.d/*); do \
		F=`basename $$D`; \
		sed 's/###.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < $$D > $(TARGETDIR)/rom/etc/usb_modeswitch.d/$$F; \
	done

usbmodeswitch-install: usbmodeswitch usbmodeswitchdb-install
	install -D usbmodeswitch/usb_modeswitch $(INSTALLDIR)/usbmodeswitch/usr/sbin/usb_modeswitch
	$(STRIP) -s $(INSTALLDIR)/usbmodeswitch/usr/sbin/usb_modeswitch
	@mkdir -p $(TARGETDIR)/rom/etc
	@sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < $(TOP)/usbmodeswitch/usb_modeswitch.conf > $(TARGETDIR)/rom/etc/usb_modeswitch.conf

#libdaemon: libdaemon/Makefile
	#$(MAKE) -C $@

#libdaemon/Makefile: libdaemon/Makefile.in
#	cd libdaemon && $(CONFIGURE) ac_cv_func_setpgrp_void=yes \
#		--disable-dependency-tracking

#libdaemon-install: libdaemon
#	install -D libdaemon/libdaemon/.libs/libdaemon.so.0.5.0 $(INSTALLDIR)/libdaemon/usr/lib/libdaemon.so.0.5.0
#	$(STRIP) $(INSTALLDIR)/libdaemon/usr/lib/*.so.*
#	cd $(INSTALLDIR)/libdaemon/usr/lib && \
#		ln -sf libdaemon.so.0.5.0 libdaemon.so && \
#		ln -sf libdaemon.so.0.5.0 libdaemon.so.0

#libdaemon-clean:
#	-@$(MAKE) -C libdaemon distclean

#radvd/stamp-h1:
#	cd radvd && CFLAGS="-Os $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
#		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" \
#	$(CONFIGURE) --prefix="" --disable-dependency-tracking \
#	DAEMON_CFLAGS="-I$(TOP)/libdaemon" \
#	DAEMON_LIBS="-ldaemon -L$(TOP)/libdaemon/libdaemon/.libs"
#	@$(MAKE) -C radvd clean
#	@touch radvd/stamp-h1
##
#radvd: libdaemon radvd/stamp-h1

#radvd-clean:
#	-@$(MAKE) -C radvd distclean
#	@rm -f radvd/stamp-h1

#radvd-install: radvd
#	install -D radvd/radvd $(INSTALLDIR)/radvd/usr/sbin/radvd
#	$(STRIP) $(INSTALLDIR)/radvd/usr/sbin/radvd

dhcpv6/stamp-h1:
	@cd dhcpv6 && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -D_GNU_SOURCE -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	ac_cv_func_setpgrp_void=yes \
	$(CONFIGURE) --prefix= --with-localdbdir=/var
	@$(MAKE) -C dhcpv6 clean
	@touch dhcpv6/stamp-h1

dhcpv6: dhcpv6/stamp-h1
	@$(SEP)
	@$(MAKE) -C dhcpv6 dhcp6c

dhcpv6-install: dhcpv6
	install -D dhcpv6/dhcp6c $(INSTALLDIR)/dhcpv6/usr/sbin/dhcp6c
	$(STRIP) $(INSTALLDIR)/dhcpv6/usr/sbin/dhcp6c

dhcpv6-clean:
	-@$(MAKE) -C dhcpv6 clean
	@rm -f dhcpv6/Makefile dhcpv6/stamp-h1


p910nd:
samba:
samba3:

nvram: shared

prebuilt: shared libbcmcrypto

vlan:
	@$(SEP)
	@$(MAKE) -C vlan CROSS=$(CROSS_COMPILE)	# STRIPTOOL=$(STRIP)

vlan-install:
	$(MAKE) -C vlan CROSS=$(CROSS_COMPILE) INSTALLDIR=$(INSTALLDIR) install	# STRIPTOOL=$(STRIP)
	$(STRIP) $(INSTALLDIR)/vlan/usr/sbin/vconfig


pptp-client-install:
	install -D pptp-client/pptp $(INSTALLDIR)/pptp-client/usr/sbin/pptp
	$(STRIP) $(INSTALLDIR)/pptp-client/usr/sbin/pptp


accel-pptp: pppd accel-pptp/Makefile
	@$(MAKE) -C accel-pptp

accel-pptp/Makefile: accel-pptp/Makefile.in $(LINUXDIR)/include/linux/version.h
	cd accel-pptp && CFLAGS="-g -O2 $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr KDIR=$(LINUXDIR) PPPDIR=$(TOP)/pppd

accel-pptp-clean:
	-@$(MAKE) -C accel-pptp clean
	@rm -f accel-pptp/Makefile

accel-pptp-install: accel-pptp
	install -D accel-pptp/src/.libs/pptp.so $(INSTALLDIR)/accel-pptp/usr/lib/pppd/pptp.so
	$(STRIP) $(INSTALLDIR)/accel-pptp/usr/lib/pppd/pptp.so

pptpd/stamp-h1:
	cd pptpd && $(CONFIGURE) --prefix=$(INSTALLDIR)/pptpd --enable-bcrelay CC=mipsel-uclibc-gcc \
		STRIP=mipsel-uclibc-strip AR=mipsel-uclibc-ar LD=mipsel-uclibc-ld NM=mipsel-uclibc-nm RANLIB=mipsel-uclibc-ranlib
	touch pptpd/stamp-h1

pptpd: pptpd/stamp-h1

pptpd-install: pptpd
	@echo pptpd
	@install -D pptpd/pptpd $(INSTALLDIR)/pptpd/usr/sbin/pptpd
	@install -D pptpd/bcrelay $(INSTALLDIR)/pptpd/usr/sbin/bcrelay
	@install -D pptpd/pptpctrl $(INSTALLDIR)/pptpd/usr/sbin/pptpctrl
	@$(STRIP) $(INSTALLDIR)/pptpd/usr/sbin/pptpd
	@$(STRIP) $(INSTALLDIR)/pptpd/usr/sbin/bcrelay
	@$(STRIP) $(INSTALLDIR)/pptpd/usr/sbin/pptpctrl

pptpd-clean:
	-@$(MAKE) -C pptpd clean
	rm -rf pptpd/stamp-h1 pptpd/.deps

pppd/Makefile: pppd/linux/Makefile.top
	cd pppd && $(CONFIGURE) --prefix=/usr --sysconfdir=/tmp

pppd: pppd/Makefile
	@$(SEP)
	@$(MAKE) -C pppd MFLAGS='$(if $(TCONFIG_IPV6),HAVE_INET6=y,) EXTRACFLAGS="$(EXTRACFLAGS)"'

pppd-clean:
	-@$(MAKE) -C pppd clean
	@rm -f pppd/Makefile

pppd-install: pppd
	install -D pppd/pppd/pppd $(INSTALLDIR)/pppd/usr/sbin/pppd
	$(STRIP) $(INSTALLDIR)/pppd/usr/sbin/pppd
	install -D pppd/chat/chat $(INSTALLDIR)/pppd/usr/sbin/chat
	$(STRIP) $(INSTALLDIR)/pppd/usr/sbin/chat
ifeq ($(TCONFIG_L2TP),y)
	install -D pppd/pppd/plugins/pppol2tp/pppol2tp.so $(INSTALLDIR)/pppd/usr/lib/pppd/pppol2tp.so
	$(STRIP) $(INSTALLDIR)/pppd/usr/lib/pppd/*.so
endif

#	ipupdate-install:
#		install -D ipupdate/ez-ipupdate $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate
#		$(STRIP) $(INSTALLDIR)/ipupdate/usr/sbin/ez-ipupdate

zebra/stamp-h1:
	@cd zebra && rm -f config.cache && \
	CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	$(CONFIGURE) --sysconfdir=/etc \
	--enable-netlink $(if $(TCONFIG_IPV6),--enable-ipv6,--disable-ipv6) --disable-ripngd --disable-ospfd --disable-doc \
	--disable-ospf6d --disable-bgpd --disable-bgpd-announce
	@touch zebra/stamp-h1

zebra: zebra/stamp-h1
	@$(MAKE) -C zebra CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD)

zebra-clean:
	-@$(MAKE) -C zebra clean
	@rm -f zebra/stamp-h1

zebra-install: zebra
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

xl2tpd: pppd
	CFLAGS="-g $(EXTRACFLAGS)" $(MAKE) -C $@ PREFIX=/usr xl2tpd $(PARALLEL_BUILD)

xl2tpd-install: xl2tpd
	install -D xl2tpd/xl2tpd $(INSTALLDIR)/xl2tpd/usr/sbin/xl2tpd
	$(STRIP) $(INSTALLDIR)/xl2tpd/usr/sbin/xl2tpd


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


iproute2:
	@$(SEP)
	@$(MAKE) -C $@ KERNEL_INCLUDE=$(LINUXDIR)/include EXTRACFLAGS="$(EXTRACFLAGS) $(if $(TCONFIG_IPV6),-DUSE_IPV6,-DNO_IPV6)"

iproute2-install: iproute2
	install -D iproute2/tc/tc $(INSTALLDIR)/iproute2/usr/sbin/tc
	$(STRIP) $(INSTALLDIR)/iproute2/usr/sbin/tc
	install -D iproute2/ip/ip $(INSTALLDIR)/iproute2/usr/sbin/ip
	$(STRIP) $(INSTALLDIR)/iproute2/usr/sbin/ip


ntpc: nvram shared


dropbear: dropbear/config.h
	@$(SEP)
	@$(MAKE) -C dropbear PROGRAMS="dropbear dbclient dropbearkey scp" MULTI=1 $(PARALLEL_BUILD)

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
	-@$(MAKE) -C dropbear clean
	@rm -f dropbear/config.h

dropbear/config.h:
	cd dropbear && \
		CFLAGS="-Os -Wall $(EXTRACFLAGS) -DARGTYPE=3 -ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" \
		ac_cv_func_logout=no ac_cv_func_logwtmp=no \
		$(CONFIGURE) --disable-zlib --enable-syslog --disable-lastlog --disable-utmp \
		--disable-utmpx --disable-wtmp --disable-wtmpx --disable-pututline \
		--disable-pututxline --disable-loginfunc --disable-pam --enable-openpty --enable-bundled-libtom
	@$(MAKE) -C dropbear clean

# Media libraries

sqlite/stamp-h1:
	cd sqlite && \
	CC=$(CC) CFLAGS="-Os $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" \
		$(CONFIGURE) --prefix=/usr --enable-shared --enable-static \
		--disable-readline --disable-dynamic-extensions --enable-threadsafe
	touch sqlite/stamp-h1

sqlite: sqlite/stamp-h1
	@$(MAKE) -C sqlite all $(PARALLEL_BUILD)

sqlite-clean:
	-@$(MAKE) -C sqlite clean
	@rm -f sqlite/stamp-h1

sqlite-install: sqlite
	@$(SEP)
ifeq ($(TCONFIG_NGINX),y)
	install -D sqlite/.libs/libsqlite3.so.0.8.6 $(INSTALLDIR)/sqlite/usr/lib/libsqlite3.so.0.8.6
	$(STRIP) $(INSTALLDIR)/sqlite/usr/lib/libsqlite3.so.0.8.6
	cd $(INSTALLDIR)/sqlite/usr/lib/ && \
		ln -sf libsqlite3.so.0.8.6 libsqlite3.so.0 && \
		ln -sf libsqlite3.so.0.8.6 libsqlite3.so
endif

FFMPEG_FILTER_CONFIG= $(foreach c, $(2), --$(1)="$(c)")

FFMPEG_DECODERS:=aac ac3 atrac3 flac h264 jpegls mp3 mpeg1video mpeg2video mpeg4 mpeg4aac mpegvideo png wmav1 wmav2
FFMPEG_CONFIGURE_DECODERS:=$(call FFMPEG_FILTER_CONFIG,enable-decoder,$(FFMPEG_DECODERS))

FFMPEG_PARSERS:=aac ac3 h264 mpeg4video mpegaudio mpegvideo
FFMPEG_CONFIGURE_PARSERS:=$(call FFMPEG_FILTER_CONFIG,enable-parser,$(FFMPEG_PARSERS))

FFMPEG_PROTOCOLS:=file
FFMPEG_CONFIGURE_PROTOCOLS:=$(call FFMPEG_FILTER_CONFIG,enable-protocol,$(FFMPEG_PROTOCOLS))

FFMPEG_DISABLED_DEMUXERS:=amr apc ape ass bethsoftvid bfi c93 daud dnxhd dsicin dxa ffm gsm gxf idcin iff image2 image2pipe ingenient ipmovie lmlm4 mm mmf msnwc_tcp mtv mxf nsv nut oma pva rawvideo rl2 roq rpl segafilm shorten siff smacker sol str thp tiertexseq tta txd vmd voc wc3 wsaud wsvqa xa yuv4mpegpipe
FFMPEG_CONFIGURE_DEMUXERS:=$(call FFMPEG_FILTER_CONFIG,disable-demuxer,$(FFMPEG_DISABLED_DEMUXERS))

ffmpeg/stamp-h1: zlib
	cd ffmpeg && symver_asm_label=no symver_gnu_asm=no symver=no CC=$(CC) \
		./configure --enable-cross-compile --arch=mips --target_os=linux \
		--cross-prefix=$(CROSS_COMPILE) --enable-shared --enable-gpl --disable-doc \
		--enable-pthreads --enable-small --disable-encoders --disable-filters \
		--disable-muxers --disable-devices --disable-ffmpeg --disable-ffplay \
		--disable-ffserver --disable-ffprobe --disable-avdevice --disable-swscale \
		--disable-hwaccels --disable-network --disable-bsfs --disable-mpegaudio-hp \
		--enable-demuxers $(FFMPEG_CONFIGURE_DEMUXERS) \
		--disable-decoders $(FFMPEG_CONFIGURE_DECODERS) \
		--disable-parsers $(FFMPEG_CONFIGURE_PARSERS) \
		--disable-protocols $(FFMPEG_CONFIGURE_PROTOCOLS) \
		--extra-cflags="-Os $(EXTRACFLAGS) -ffunction-sections -fdata-sections -fPIC -I$(TOP)/zlib" \
		--extra-ldflags="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
		--extra-libs="-L$(TOP)/zlib -lz" \
		--enable-zlib --disable-debug --prefix=''
	touch ffmpeg/stamp-h1

ffmpeg: ffmpeg/stamp-h1 zlib
	@$(MAKE) -C ffmpeg all $(PARALLEL_BUILD)

ffmpeg-clean:
	-@$(MAKE) -C ffmpeg clean
	@rm -f ffmpeg/stamp-h1 ffmpeg/config.h ffmpeg/config.mak

ffmpeg-install: ffmpeg
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D ffmpeg/libavformat/libavformat.so.52 $(INSTALLDIR)/ffmpeg/usr/lib/libavformat.so.52
	install -D ffmpeg/libavcodec/libavcodec.so.52 $(INSTALLDIR)/ffmpeg/usr/lib/libavcodec.so.52
	install -D ffmpeg/libavutil/libavutil.so.50 $(INSTALLDIR)/ffmpeg/usr/lib/libavutil.so.50
	$(STRIP) $(INSTALLDIR)/ffmpeg/usr/lib/libavformat.so.52
	$(STRIP) $(INSTALLDIR)/ffmpeg/usr/lib/libavcodec.so.52
	$(STRIP) $(INSTALLDIR)/ffmpeg/usr/lib/libavutil.so.50
endif

libogg/stamp-h1:
	cd libogg && \
	CFLAGS="-Os $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	LDFLAGS="-fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --enable-shared --enable-static --prefix=''
	touch libogg/stamp-h1

libogg: libogg/stamp-h1
	@$(MAKE) -C libogg all $(PARALLEL_BUILD)

libogg-clean:
	-@$(MAKE) -C libogg clean
	@rm -f libogg/stamp-h1

libogg-install: libogg
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D libogg/src/.libs/libogg.so.0 $(INSTALLDIR)/libogg/usr/lib/libogg.so.0
	$(STRIP) $(INSTALLDIR)/libogg/usr/lib/libogg.so.0
endif

flac/stamp-h1: libogg
	cd flac && \
	CFLAGS="-Os $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	CPPFLAGS="-I$(TOP)/libogg/include -I$(LINUXDIR)/include" \
	LDFLAGS="-L$(TOP)/libogg/src/.libs -fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --enable-shared --enable-static --prefix='' --disable-rpath \
		--disable-doxygen-docs --disable-xmms-plugin --disable-cpplibs \
		--without-libiconv-prefix --disable-altivec --disable-3dnow --disable-sse
	touch flac/stamp-h1

flac: flac/stamp-h1 libogg
	@$(MAKE) -C flac/src/libFLAC all $(PARALLEL_BUILD)

flac-clean:
	-@$(MAKE) -C flac clean
	@rm -f flac/stamp-h1

flac-install: flac
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D flac/src/libFLAC/.libs/libFLAC.so.8 $(INSTALLDIR)/flac/usr/lib/libFLAC.so.8
	$(STRIP) $(INSTALLDIR)/flac/usr/lib/libFLAC.so.8
endif

jpeg/stamp-h1:
	cd jpeg && \
	CFLAGS="-Os $(EXTRACFLAGS) -fPIC" CC=$(CC) AR2="touch" $(CONFIGURE) --enable-shared --enable-static --prefix=''
	touch jpeg/stamp-h1

jpeg: jpeg/stamp-h1
	@$(MAKE) -C jpeg LIBTOOL="" O=o A=a CC=$(CC) AR2="touch" libjpeg.a libjpeg.so $(PARALLEL_BUILD)
	install -d $(TOP)/jpeg/staged/include
	install -d $(TOP)/jpeg/staged/lib
	install -d $(TOP)/jpeg/staged/bin
	install -d $(TOP)/jpeg/staged/man/man1
	@$(MAKE) -C jpeg LIBTOOL="" prefix=$(TOP)/jpeg/staged install
	install -D jpeg/libjpeg.so $(TOP)/jpeg/staged/lib/libjpeg.so
	rm -f $(TOP)/jpeg/staged/lib/libjpeg.la

jpeg-clean:
	-@$(MAKE) -C jpeg clean
	@rm -f jpeg/stamp-h1 jpeg/Makefile
	@rm -rf jpeg/staged

jpeg-install: jpeg
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D jpeg/libjpeg.so $(INSTALLDIR)/jpeg/usr/lib/libjpeg.so
	$(STRIP) $(INSTALLDIR)/jpeg/usr/lib/libjpeg.so
endif
ifeq ($(TCONFIG_NGINX),y)
	install -D jpeg/libjpeg.so $(INSTALLDIR)/jpeg/usr/lib/libjpeg.so
	$(STRIP) $(INSTALLDIR)/jpeg/usr/lib/libjpeg.so
endif

libexif/stamp-h1:
	cd libexif && CFLAGS="-Os -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	LDFLAGS="-fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --enable-shared --enable-static --prefix='' \
		--disable-docs --disable-rpath --disable-nls --without-libiconv-prefix --without-libintl-prefix
	touch libexif/stamp-h1

libexif: libexif/stamp-h1
	@$(MAKE) -C libexif all

libexif-clean:
	-@$(MAKE) -C libexif clean
	@rm -f libexif/stamp-h1

libexif-install: libexif
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D libexif/libexif/.libs/libexif.so.12 $(INSTALLDIR)/libexif/usr/lib/libexif.so.12
	$(STRIP) $(INSTALLDIR)/libexif/usr/lib/libexif.so.12
endif

zlib/stamp-h1:
	cd zlib && \
	CC=$(CC) RANLIB=$(RANLIB) LD=$(LD) CFLAGS="-Os -Wall $(EXTRACFLAGS) -fPIC" LDSHAREDLIBC="$(EXTRALDFLAGS)" \
	./configure --shared --prefix=/usr --libdir=/usr/lib
	touch zlib/stamp-h1

zlib: zlib/stamp-h1
	@$(MAKE) -C zlib all
	@$(MAKE) -C zlib DESTDIR=$(TOP)/zlib/staged install

zlib-clean:
	-@$(MAKE) -C zlib clean
	@rm -f zlib/stamp-h1 zlib/Makefile zlib/zconf.h zlib/zlib.pc
	@rm -rf zlib/staged

zlib-install: zlib
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -d $(INSTALLDIR)/zlib/usr/lib
	install -D zlib/libz.so.1 $(INSTALLDIR)/zlib/usr/lib/
	$(STRIP) $(INSTALLDIR)/zlib/usr/lib/libz.so.1
endif
ifeq ($(TCONFIG_BT),y)
	install -d $(INSTALLDIR)/zlib/usr/lib
	install -D zlib/libz.so.1 $(INSTALLDIR)/zlib/usr/lib/
	$(STRIP) $(INSTALLDIR)/zlib/usr/lib/libz.so.1
endif
ifeq ($(TCONFIG_TOR),y)
	install -d $(INSTALLDIR)/zlib/usr/lib
	install -D zlib/libz.so.1 $(INSTALLDIR)/zlib/usr/lib/
	$(STRIP) $(INSTALLDIR)/zlib/usr/lib/libz.so.1
endif
ifeq ($(TCONFIG_NGINX),y)
	install -d $(INSTALLDIR)/zlib/usr/lib
	install -D zlib/libz.so.1 $(INSTALLDIR)/zlib/usr/lib/
	$(STRIP) $(INSTALLDIR)/zlib/usr/lib/libz.so.1
endif

libid3tag/stamp-h1: zlib
	cd libid3tag && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" CPPFLAGS="-I$(TOP)/zlib" \
	LDFLAGS="-L$(TOP)/zlib -fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --enable-shared --enable-static --prefix='' \
		--disable-debugging --disable-profiling
	touch libid3tag/stamp-h1

libid3tag: libid3tag/stamp-h1 zlib
	@$(MAKE) -C libid3tag all $(PARALLEL_BUILD)

libid3tag-clean:
	-@$(MAKE) -C libid3tag clean
	@rm -f libid3tag/stamp-h1

libid3tag-install: libid3tag
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D libid3tag/.libs/libid3tag.so.0 $(INSTALLDIR)/libid3tag/usr/lib/libid3tag.so.0
	$(STRIP) $(INSTALLDIR)/libid3tag/usr/lib/libid3tag.so.0
endif

libvorbis/stamp-h1: libogg
	cd libvorbis && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	CPPFLAGS="-I$(TOP)/libogg/include" \
	LDFLAGS="-L$(TOP)/libogg/src/.libs -fPIC -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --enable-shared --enable-static --prefix='' --disable-oggtest \
		--with-ogg-includes="$(TOP)/libogg/include" \
		--with-ogg-libraries="$(TOP)/libogg/src/.libs"
	touch libvorbis/stamp-h1

libvorbis: libvorbis/stamp-h1
	@$(MAKE) -C libvorbis/lib all $(PARALLEL_BUILD)

libvorbis-clean:
	-@$(MAKE) -C libvorbis clean
	@rm -f libvorbis/stamp-h1

libvorbis-install: libvorbis
	@$(SEP)
ifneq ($(MEDIA_SERVER_STATIC),y)
	install -D libvorbis/lib/.libs/libvorbis.so.0 $(INSTALLDIR)/libvorbis/usr/lib/libvorbis.so.0
	$(STRIP) $(INSTALLDIR)/libvorbis/usr/lib/libvorbis.so.0
endif

minidlna: zlib sqlite ffmpeg libogg flac jpeg libexif libid3tag libvorbis
	@$(SEP)
	@$(MAKE) -C minidlna CC=$(CC) $(if $(MEDIA_SERVER_STATIC),STATIC=1,) minidlna $(PARALLEL_BUILD)

minidlna-clean:
	-@$(MAKE) -C minidlna clean
	@rm -f minidlna/stamp-h1

minidlna-install: minidlna
	@$(SEP)
	install -D minidlna/minidlna $(INSTALLDIR)/minidlna/usr/sbin/minidlna
	$(STRIP) $(INSTALLDIR)/minidlna/usr/sbin/minidlna

igmpproxy/src/Makefile: igmpproxy/src/Makefile.in
	cd igmpproxy && CFLAGS="-O2 -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --prefix=/usr

igmpproxy: igmpproxy/src/Makefile
	@$(SEP)
	@$(MAKE) -C igmpproxy/src $(PARALLEL_BUILD)

igmpproxy-install: igmpproxy
	install -D igmpproxy/src/igmpproxy $(INSTALLDIR)/igmpproxy/usr/sbin/igmpproxy
	$(STRIP) $(INSTALLDIR)/igmpproxy/usr/sbin/igmpproxy

igmpproxy-clean:
	-@$(MAKE) -C igmpproxy/src clean
	@rm -f igmpproxy/src/Makefile

udev:
	$(MAKE) -C $@ CROSS_COMPILE=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)" \
	    PROGRAMS=udevtrigger

udev-install: udev
	install -d $(INSTALLDIR)
	install -d $(TARGETDIR)
	$(MAKE) -C udev DESTDIR=$(INSTALLDIR) prefix=/udev install-udevtrigger

hotplug2:
	$(MAKE) -C $@ CROSS_COMPILE=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)"

hotplug2-install: hotplug2
	$(MAKE) -C hotplug2 install PREFIX=$(INSTALLDIR)/hotplug2 SUBDIRS=""
	$(MAKE) -C hotplug2/examples install PREFIX=$(INSTALLDIR)/hotplug2/rom KERNELVER=$(LINUX_KERNEL)

emf:
	$(MAKE) -C $(SRCBASE)/emf/emfconf CROSS=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)"

emf-install: emf
	$(MAKE) -C $(SRCBASE)/emf/emfconf INSTALLDIR=$(INSTALLDIR) install

igs:
	$(MAKE) -C $(SRCBASE)/emf/igsconf CROSS=$(CROSS_COMPILE) EXTRACFLAGS="$(EXTRACFLAGS)"

igs-install: igs
	$(MAKE) -C $(SRCBASE)/emf/igsconf INSTALLDIR=$(INSTALLDIR) install

wanuptime: nvram shared
	@$(SEP)
	@$(MAKE) -C wanuptime

wanuptime-clean:
	-@$(MAKE) -C wanuptime clean

wanuptime-install:
	install -D wanuptime/wanuptime $(INSTALLDIR)/wanuptime/usr/sbin/wanuptime
	$(STRIP) $(INSTALLDIR)/wanuptime/usr/sbin/wanuptime

ebtables: dummy
	$(MAKE) -C ebtables CC=$(CC) LD=$(LD) \
	CFLAGS="-Os $(EXTRACFLAGS) -DEBT_MIN_ALIGN=4 -Wall -Wunused" \
	LOCKFILE="/var/lock/ebtables" LOCKDIR="/var/lock" \
	BINDIR="/usr/sbin" LIBDIR="/usr/lib/ebtables" KERNEL_INCLUDES=$(LINUXDIR)/include $(if $(TCONFIG_IPV6),DO_IPV6=1,)

ebtables-install: ebtables
	install -D ebtables/ebtables $(INSTALLDIR)/ebtables/usr/sbin/ebtables

	@mkdir -p $(TARGETDIR)/rom/etc
	@sed 's/#.*//g;s/[ \t]\+/ /g;s/^[ \t]*//;s/[ \t]*$$//;/^$$/d' < $(TOP)/ebtables/ethertypes > $(TARGETDIR)/rom/etc/ethertypes
	chmod 0644 $(TARGETDIR)/rom/etc/ethertypes

	install -d $(INSTALLDIR)/ebtables/usr/lib
	install -d $(INSTALLDIR)/ebtables/usr/lib/ebtables
	install -D ebtables/*.so $(INSTALLDIR)/ebtables/usr/lib/
	install -D ebtables/extensions/*.so $(INSTALLDIR)/ebtables/usr/lib/ebtables/

	$(STRIP) $(INSTALLDIR)/ebtables/usr/sbin/ebtables
	$(STRIP) $(INSTALLDIR)/ebtables/usr/lib/ebtables/*.so
	$(STRIP) $(INSTALLDIR)/ebtables/usr/lib/libebt*.so

ebtables-clean:
	-@$(MAKE) -C ebtables clean

#Roadkill
spawn-fcgi/stamp-h1:
	cd spawn-fcgi && autoreconf && CC=$(CC) \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	ac_cv_func_malloc_0_nonnull=yes \
	$(CONFIGURE) --prefix=/usr
	touch spawn-fcgi/stamp-h1

spawn-fcgi: spawn-fcgi/stamp-h1
	@$(MAKE) -C spawn-fcgi $(PARALLEL_BUILD)

spawn-fcgi-clean:
	-@$(MAKE) -C spawn-fcgi clean
	rm -f spawn-fcgi/stamp-h1

spawn-fcgi-install: spawn-fcgi
	install -d $(INSTALLDIR)/spawn-fcgi/usr/bin
	install spawn-fcgi/src/spawn-fcgi $(INSTALLDIR)/spawn-fcgi/usr/bin/spawn-fcgi
	$(STRIP) -s $(INSTALLDIR)/spawn-fcgi/usr/bin/spawn-fcgi

#Roadkill
glib/stamp-h1:
	@$(SEP)
	@cd glib && $(CONFIGURE) --prefix=/usr glib_cv_prog_cc_ansi_proto=no glib_cv_has__inline=yes glib_cv_has__inline__=yes glib_cv_hasinline=yes \
		glib_cv_sane_realloc=yes glib_cv_va_copy=no glib_cv___va_copy=yes glib_cv_va_val_copy=yes glib_cv_rtldglobal_broken=no \
		glib_cv_uscore=no glib_cv_func_pthread_mutex_trylock_posix=yes glib_cv_func_pthread_cond_timedwait_posix=yes glib_cv_sizeof_gmutex=24 \
		glib_cv_byte_contents_gmutex="0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0" glib_cv_sys_pthread_getspecific_posix=yes \
		glib_cv_sys_pthread_mutex_trylock_posix=yes glib_cv_sys_pthread_cond_timedwait_posix=yes ac_cv_func_getpwuid_r=yes ac_cv_func_getpwuid_r_posix=yes
	@$(MAKE) -C glib
	touch glib/stamp-h1

glib: glib/stamp-h1
	@$(MAKE) -C glib $(PARALLEL_BUILD)

glib-clean:
	-@$(MAKE) -C glib clean
	rm -f glib/stamp-h1

glib-install: glib
	@$(MAKE) -C glib DESTDIR=$(INSTALLDIR)/glib install

nocat/stamp-h1: glib-install
	@$(SEP)
	@cd nocat && \
	NC_CONF_PATH="/" \
	$(CONFIGURE) --with-firewall=iptables --with-glib-prefix="$(INSTALLDIR)/glib/usr" --localstatedir=/var --sysconfdir=/etc
	@$(MAKE) -C nocat

	echo *** integrate glib to nocat installdir
	install -d $(INSTALLDIR)/nocat/usr/lib
	install -D glib/.libs/libglib-1.2.so.0.0.10 $(INSTALLDIR)/nocat/usr/lib/libglib-1.2.so.0.0.10
	cd $(INSTALLDIR)/nocat/usr/lib && ln -s libglib-1.2.so.0.0.10 libglib-1.2.so.0
	$(STRIP) $(INSTALLDIR)/nocat/usr/lib/libglib-1.2.so.0.0.10
	touch nocat/stamp-h1

nocat: nocat/stamp-h1
	@$(MAKE) -C nocat $(PARALLEL_BUILD)

nocat-clean:
	-@$(MAKE) -C nocat clean
	rm -f nocat/stamp-h1

nocat-install:
	install -D nocat/src/splashd $(INSTALLDIR)/nocat/usr/sbin/splashd
	$(STRIP) $(INSTALLDIR)/nocat/usr/sbin/splashd
	mkdir -p $(INSTALLDIR)/nocat/usr/libexec/nocat
	install -D nocat/libexec/iptables/* $(INSTALLDIR)/nocat/usr/libexec/nocat

#Tomato RAF
pcre/stamp-h1:
	$(SEP)
	cd pcre && autoreconf && \
	CC=$(CC) CXX=$(HOST)-g++ AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr --disable-dependency-tracking --enable-utf8 --enable-unicode-properties --disable-cpp
	@touch pcre/stamp-h1
	[ -d pcre/m4 ] || mkdir pcre/m4

pcre: pcre/stamp-h1

pcre-install: pcre
	@$(SEP)
	install -D pcre/.libs/libpcre.so.1 $(INSTALLDIR)/pcre/usr/lib/libpcre.so.1.2.7
	$(STRIP) -s $(INSTALLDIR)/pcre/usr/lib/libpcre.so.1.2.7
	cd $(INSTALLDIR)/pcre/usr/lib/ && \
		ln -sf libpcre.so.1.2.7 libpcre.so.1
#unused at the moment - shibby
#	install -D pcre/.libs/libpcreposix.so.0.0.4 $(INSTALLDIR)/pcre/usr/lib/libpcreposix.so.0.0.4
#	$(STRIP) -s $(INSTALLDIR)/pcre/usr/lib/libpcreposix.so.0.0.4
#	cd $(INSTALLDIR)/pcre/usr/lib/ && \
#		ln -sf libpcreposix.so.0.0.4 libpcreposix.so.0

pcre-clean:
	( if [ -f pcre/Makefile ]; then \
		$(MAKE) -C pcre clean; \
		rm -rf pcre/stamp-h1; \
	fi )

libxml2/stamp-h1:
	cd libxml2 && \
	CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr --without-python --enable-static --enable-shared
	touch $@

libxml2: libxml2/stamp-h1
	@$(MAKE) -C libxml2 all $(PARALLEL_BUILD)
	@$(MAKE) -C libxml2 DESTDIR=$(TOP)/libxml2/staged install

libxml2-install: libxml2
	@$(SEP)
	install -D libxml2/.libs/libxml2.so.2.9.1 $(INSTALLDIR)/libxml2/usr/lib/libxml2.so.2.9.1
	$(STRIP) $(INSTALLDIR)/libxml2/usr/lib/libxml2.so.2.9.1
	cd $(INSTALLDIR)/libxml2/usr/lib && \
		ln -sf libxml2.so.2.9.1 libxml2.so.2 && \
		ln -sf libxml2.so.2.9.1 libxml2.so

libxml2-clean:
	-@$(MAKE) -C libxml2 clean
	@rm -f libxml2/stamp-h1
	@rm -rf libxml2/staged

libpng/stamp-h1: zlib
	cd libpng && \
	CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) CFLAGS="-Os -Wall -I$(TOP)/zlib $(EXTRACFLAGS)" LDFLAGS="-L$(TOP)/zlib" \
	$(CONFIGURE) --prefix=/usr --enable-static --enable-shared
	touch $@

libpng: libpng/stamp-h1
	@$(MAKE) -C libpng all $(PARALLEL_BUILD)
	@$(MAKE) -C libpng DESTDIR=$(TOP)/libpng/staged install

libpng-install: libpng
	@$(SEP)
	install -D libpng/.libs/libpng.so.3.51.0 $(INSTALLDIR)/libpng/usr/lib/libpng.so.3.51.0
	$(STRIP) $(INSTALLDIR)/libpng/usr/lib/libpng.so.3.51.0
	cd $(INSTALLDIR)/libpng/usr/lib && \
		ln -sf libpng.so.3.51.0 libpng.so && \
		ln -sf libpng.so.3.51.0 libpng.so.3
	install -D libpng/.libs/libpng12.so.0.51.0 $(INSTALLDIR)/libpng/usr/lib/libpng12.so.0.51.0
	$(STRIP) $(INSTALLDIR)/libpng/usr/lib/libpng12.so.0.51.0
	cd $(INSTALLDIR)/libpng/usr/lib && \
		ln -sf libpng12.so.0.51.0 libpng12.so && \
		ln -sf libpng12.so.0.51.0 libpng12.so.0

libatomic_ops/stamp-h1:
	cd libatomic_ops && \
	./autogen.sh && \
	CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr --enable-static --enable-shared
	touch $@

libatomic_ops: libatomic_ops/stamp-h1
	@$(MAKE) -C libatomic_ops $(PARALLEL_BUILD)

libatomic_ops-install: libatomic_ops
	@$(SEP)
	install -D libatomic_ops/src/.libs/libatomic_ops.so.1.0.3 $(INSTALLDIR)/libatomic_ops/usr/lib/libatomic_ops.so.1.0.3
	$(STRIP) $(INSTALLDIR)/libatomic_ops/usr/lib/libatomic_ops.so.1.0.3

libatomic_ops-clean:
	-@$(MAKE) -C libatomic_ops clean
	@rm -f libatomic_ops/stamp-h1

php/stamp-h1: pcre zlib libiconv sqlite libxml2 libpng jpeg libcurl
	@$(SEP)
	cd php && CC=$(CC) CXX=$(ARCH)-g++ AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) \
	CFLAGS="-Os -Wall -I$(TOP)/zlib -I$(TOP)/libxml2/include/libxml -I$(TOP)/libxml2/include -I$(TOP)/pcre -I$(TOP)/libiconv/include \
		-I$(TOP)/libpng/staged/usr/include -I$(TOP)/libcurl/staged/include" \
	LDFLAGS="-L$(TOP)/pcre/.libs -L$(TOP)/sqlite/.libs -L$(TOP)/zlib -L$(TOP)/libxml2/.libs -L$(TOP)/libiconv/lib/.libs \
		-L$(TOP)/libpng/.libs -L$(TOP)/libcurl/staged/lib" \
	CPPFLAGS="-L$(TOP)/pcre/.libs -L$(TOP)/sqlite/.libs -L$(TOP)/zlib -L$(TOP)/libxml2/.libs -L$(TOP)/libiconv/lib/.libs -L$(TOP)/libpng/.libs" \
	LIBS="-L$(TOP)/pcre/.libs -L$(TOP)/sqlite/.libs -L$(TOP)/zlib -L$(TOP)/libxml2/.libs -L$(TOP)/libiconv/lib/.libs -L$(TOP)/libpng/.libs -lz \
		-lsqlite3 -ldl -lpthread -liconv -lxml2 -lstdc++ -lcurl" \
	PHP_FCGI_LIBXML_DIR="$(TOP)/libxml2/staged/usr" \
	ac_cv_func_memcmp_working=yes \
	cv_php_mbstring_stdarg=yes \
	$(CONFIGURE) --prefix=/usr \
	--enable-shared \
	--disable-static \
	--disable-rpath \
	--disable-debug \
	--without-pear \
	--with-config-file-path=/etc \
	--with-config-file-scan-dir=/etc/php5 \
	--disable-short-tags \
	--with-zlib \
	--with-zlib-dir="$(TOP)/zlib/staged/usr" \
	--disable-phar \
	--enable-cli \
	--enable-cgi \
	--disable-calendar \
	--enable-ctype \
	--with-curl="$(TOP)/libcurl/staged" \
	--enable-fileinfo \
	--without-gettext \
	--enable-dom \
	--enable-exif \
	--disable-ftp \
	--without-gmp \
	--with-gd \
	--with-png-dir="$(TOP)/libpng/staged/usr" \
	--with-jpeg-dir="$(TOP)/jpeg/staged" \
	--enable-hash \
	--with-iconv="$(TOP)/libiconv/staged/usr" \
	--with-iconv-dir="$(TOP)/libiconv/staged/usr" \
	--enable-json \
	--without-ldap \
	--enable-mbstring \
	--without-mcrypt \
	--without-openssl \
	--disable-pcntl \
	--without-mysqli \
	--with-mysql \
	--with-mysql-sock="/var/run/mysqld.sock" \
	--with-pdo-mysql \
	--without-pdo-pgsql \
	--with-pdo-sqlite \
	--enable-pdo \
	--without-pgsql \
	--enable-session \
	--disable-shmop \
	--enable-simplexml \
	--disable-soap \
	--disable-sockets \
	--with-sqlite3 \
	--disable-sysvmsg \
	--disable-sysvsem \
	--disable-sysvshm \
	--disable-tokenizer \
	--enable-xml \
	--enable-xmlreader \
	--enable-xmlwriter \
	--enable-zip \
	--disable-filter \
	--with-libxml-dir="$(TOP)/libxml2/staged/usr"
	@touch php/stamp-h1

php: php/stamp-h1
	@$(SEP)
	@$(MAKE) -C php

php-clean:
	-@$(MAKE) -C php clean
	-@rm php/stamp-h1

php-install: php
	@$(SEP)
	install -d $(INSTALLDIR)/php/usr/sbin
	install -D php/sapi/cli/.libs/php $(INSTALLDIR)/php/usr/sbin/php-cli && chmod 0755 $(INSTALLDIR)/php/usr/sbin/php-cli
	$(STRIP) $(INSTALLDIR)/php/usr/sbin/php-cli
	install -D php/sapi/cgi/.libs/php-cgi $(INSTALLDIR)/php/usr/sbin/php-cgi && chmod 0755 $(INSTALLDIR)/php/usr/sbin/php-cgi
	cd $(INSTALLDIR)/php/usr/sbin && ln -sf php-cgi php-fcgi
	$(STRIP) $(INSTALLDIR)/php/usr/sbin/php-cgi

nginx/stamp-h1: openssl zlib pcre libatomic_ops
	@$(SEP)
	cd $(TOP)/nginx && ./configure --crossbuild=Linux::$(ARCH) \
		--prefix=/usr \
		--sbin-path=/usr/sbin \
		--conf-path=/etc/nginx/nginx.conf \
		--error-log-path=/tmp/var/log/nginx/error.log \
		--http-log-path=/tmp/var/log/nginx/access.log \
		--pid-path=/tmp/var/run/nginx.pid \
		--lock-path=/tmp/var/run/nginx.lock.accept \
		--http-client-body-temp-path=/tmp/var/lib/nginx/client \
		--http-fastcgi-temp-path=/tmp/var/lib/nginx/fastcgi \
		--with-http_flv_module \
		--with-http_ssl_module \
		--with-http_gzip_static_module \
		--http-uwsgi-temp-path=/tmp/var/lib/nginx/uwsgi \
		--http-scgi-temp-path=/tmp/var/lib/nginx/scgi \
		--http-proxy-temp-path=/tmp/var/lib/nginx/proxy \
		--with-libatomic="$(TOP)/libatomic_ops" \
		--with-cc="$(CC)" \
		--with-ld-opt="$(LDFLAGS) -L $(TOP)/pcre/.libs -L $(TOP)/zlib -L $(TOP)/openssl -L $(TOP)/libatomic_ops/src" \
		--with-cc-opt="$(EXTRACFLAGS) $(CPPFLAGS) $(CFLAGS) -I $(TOP)/openssl/include -I $(TOP)/openssl/include/openssl -I $(TOP)/pcre -I $(TOP)/zlib -I $(TOP)/libatomic_ops/src/.libs" \
		$(ADDITIONAL_MODULES)

nginx: nginx/stamp-h1
	@$(SEP)
	@$(MAKE) -C nginx

nginx-clean:
	-@$(MAKE) -C nginx clean
	-@rm -f nginx/stamp-h1

nginx-install: nginx
	install -d $(INSTALLDIR)/nginx/usr/sbin
	install -D nginx/objs/nginx $(INSTALLDIR)/nginx/usr/sbin/nginx && chmod 755 $(INSTALLDIR)/nginx/usr/sbin/nginx
	$(STRIP) $(INSTALLDIR)/nginx/usr/sbin/nginx

libncurses/stamp-h1:
	cd libncurses && CC=$(CC) \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections -fPIC" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	LIBS="-lstdc++" \
	$(CONFIGURE) --prefix=/usr --with-shared --with-normal --disable-debug --without-ada --without-manpages --without-progs --without-tests
	touch libncurses/stamp-h1

libncurses: libncurses/stamp-h1
	@$(SEP)
	$(MAKE) -C libncurses $(PARALLEL_BUILD)
	$(MAKE) -C libncurses DESTDIR=$(TOP)/libncurses/staged install

libncurses-clean:
	-@$(MAKE) -C libncurses clean
	@rm -f libncurses/stamp-h1 libncurses/Makefile
	@rm -rf libncurses/staged

libncurses-install: libncurses
	install -d $(INSTALLDIR)/libncurses/usr/lib
	install -d $(INSTALLDIR)/libncurses/usr/share/terminfo
	install -d $(INSTALLDIR)/libncurses/usr/share/terminfo/l
	install -d $(INSTALLDIR)/libncurses/usr/share/terminfo/v
	install -d $(INSTALLDIR)/libncurses/usr/share/terminfo/x
	install -D -m 0644 libncurses/staged/usr/share/terminfo/l/linux $(INSTALLDIR)/libncurses/usr/share/terminfo/l
	install -D -m 0644 libncurses/staged/usr/share/terminfo/v/vt100 $(INSTALLDIR)/libncurses/usr/share/terminfo/v
	install -D -m 0644 libncurses/staged/usr/share/terminfo/v/vt220 $(INSTALLDIR)/libncurses/usr/share/terminfo/v
	install -D -m 0644 libncurses/staged/usr/share/terminfo/x/xterm $(INSTALLDIR)/libncurses/usr/share/terminfo/x
	cd $(INSTALLDIR)/libncurses/usr/lib && ln -sf ../share/terminfo terminfo
	install libncurses/lib/libncurses.so.6.0 $(INSTALLDIR)/libncurses/usr/lib/libncurses.so.6
	$(STRIP) $(INSTALLDIR)/libncurses/usr/lib/libncurses.so.6
	install libncurses/lib/libpanel.so.6.0 $(INSTALLDIR)/libncurses/usr/lib/libpanel.so.6
	$(STRIP) $(INSTALLDIR)/libncurses/usr/lib/libpanel.so.6
	install libncurses/lib/libform.so.6.0 $(INSTALLDIR)/libncurses/usr/lib/libform.so.6
	$(STRIP) $(INSTALLDIR)/libncurses/usr/lib/libform.so.6
	install libncurses/lib/libmenu.so.6.0 $(INSTALLDIR)/libncurses/usr/lib/libmenu.so.6
	$(STRIP) $(INSTALLDIR)/libncurses/usr/lib/libmenu.so.6
	cd $(INSTALLDIR)/libncurses/usr/lib/ && \
		ln -sf libncurses.so.6 libncurses.so && \
		ln -sf libpanel.so.6 libpanel.so && \
		ln -sf libform.so.6 libform.so && \
		ln -sf libmenu.so.6 libmenu.so

mysql/stamp-h1: openssl zlib libncurses
	@$(SEP)
	cd mysql && cp -f configure.mipsel configure && \
	CFLAGS="-O2 -Wall -fno-delete-null-pointer-checks -funit-at-a-time --param large-function-growth=800 \
		--param max-inline-insns-single=3000 -ffunction-sections -fdata-sections -I$(TOP)/zlib \
		-I$(TOP)/openssl/include -I$(TOP)/libncurses/include" \
	CPPFLAGS="-O2 -Wall -fno-delete-null-pointer-checks -funit-at-a-time --param large-function-growth=800 \
		--param max-inline-insns-single=3000 -ffunction-sections -fdata-sections -I$(TOP)/zlib \
		-I$(TOP)/openssl/include -I$(TOP)/libncurses/include" \
	LDFLAGS="-L$(TOP)/openssl -L$(TOP)/zlib/staged/usr/lib -L$(TOP)/libncurses/lib -fPIC -EL" \
	LIBS="-lcrypt -lz -lstdc++ -lssl -lcrypto -lncurses" \
	$(CONFIGURE) --prefix=/usr \
		--without-debug --without-docs --without-man --with-charset=utf8 --with-extra-charsets=ascii,latin1,gb2312,gbk \
		--enable-shared --disable-static \
		--without-mysqlmanager \
		--with-pthread \
		--with-ssl \
		--without-docs \
		--with-geometry \
		--with-low-memory \
		--enable-assembler \
		--with-zlib-dir="$(TOP)/zlib/staged/usr" \
		ac_cv_c_stack_direction=-1
	cd mysql && tar xvfz .host.tgz
	# Update time of config.status to avoid run 'config.status --recheck'
	touch mysql/config.status
	# Update time of Makefile to avoid regenerate it when do make
	sleep 5
	-find mysql -name Makefile -print | xargs -i touch {}
	touch mysql/stamp-h1

mysql: mysql/stamp-h1
	@$(SEP)
	@$(MAKE) -C mysql
	@$(MAKE) -C mysql DESTDIR=$(TOP)/mysql/staged install

mysql-clean:
	-@$(MAKE) -C mysql clean
	-@rm -f mysql/stamp-h1
	-@rm -rf mysql/staged mysql/host

mysql-install:
	install -d $(INSTALLDIR)/mysql/usr/bin
	install -d $(INSTALLDIR)/mysql/usr/lib
	install -d $(INSTALLDIR)/mysql/usr/libexec
	install -d $(INSTALLDIR)/mysql/usr/lib/mysql
	install -d $(INSTALLDIR)/mysql/usr/lib/mysql/plugin
	install -d $(INSTALLDIR)/mysql/usr/share
	install -d $(INSTALLDIR)/mysql/usr/share/mysql
	install -D -m 755 mysql/mycheck $(INSTALLDIR)/mysql/usr/bin
	install -D -m 755 mysql/staged/usr/bin/my_print_defaults $(INSTALLDIR)/mysql/usr/bin
	$(STRIP) $(INSTALLDIR)/mysql/usr/bin/my_print_defaults
	install -D -m 755 mysql/staged/usr/bin/myisamchk $(INSTALLDIR)/mysql/usr/bin
	$(STRIP) $(INSTALLDIR)/mysql/usr/bin/myisamchk
	install -D -m 755 mysql/staged/usr/bin/mysql $(INSTALLDIR)/mysql/usr/bin
	$(STRIP) $(INSTALLDIR)/mysql/usr/bin/mysql
	install -D -m 755 mysql/staged/usr/bin/mysql_install_db $(INSTALLDIR)/mysql/usr/bin
	install -D -m 755 mysql/staged/usr/bin/mysqladmin $(INSTALLDIR)/mysql/usr/bin
	$(STRIP) $(INSTALLDIR)/mysql/usr/bin/mysqladmin
	install -D -m 755 mysql/staged/usr/bin/mysqldump $(INSTALLDIR)/mysql/usr/bin
	$(STRIP) $(INSTALLDIR)/mysql/usr/bin/mysqldump
	install -D -m 755 mysql/staged/usr/libexec/mysqld $(INSTALLDIR)/mysql/usr/libexec
	$(STRIP) $(INSTALLDIR)/mysql/usr/libexec/mysqld
	cd $(INSTALLDIR)/mysql/usr/bin && ln -sf ../libexec/mysqld mysqld
#	install -D -m 755 mysql/staged/usr/bin/mysqld_safe $(INSTALLDIR)/mysql/usr/bin
	install -D -m 755 mysql/staged/usr/lib/mysql/libmysqlclient.so.16.0.0 $(INSTALLDIR)/mysql/usr/lib/mysql
	$(STRIP) $(INSTALLDIR)/mysql/usr/lib/mysql/libmysqlclient.so.16.0.0
	-@cd $(INSTALLDIR)/mysql/usr/lib/mysql && \
		ln -sf libmysqlclient.so.16.0.0 libmysqlclient.so.16 && \
		ln -sf libmysqlclient.so.16.0.0 libmysqlclient.so
	install -D -m 755 mysql/staged/usr/lib/mysql/libmysqlclient_r.so.16.0.0 $(INSTALLDIR)/mysql/usr/lib/mysql
	$(STRIP) $(INSTALLDIR)/mysql/usr/lib/mysql/libmysqlclient_r.so.16.0.0
	-@cd $(INSTALLDIR)/mysql/usr/lib/mysql && \
		ln -sf libmysqlclient_r.so.16.0.0 libmysqlclient_r.so.16 && \
		ln -sf libmysqlclient_r.so.16.0.0 libmysqlclient_r.so
#	-@cd $(INSTALLDIR)/mysql/usr/lib/mysql/plugin && cp -arfpu $(TOP)/mysql/staged/usr/lib/mysql/plugin/* . && \
#	rm -f *.la *.a && \
#	$(STRIP) *.so.*
	-@cd $(INSTALLDIR)/mysql/usr/share/mysql && \
	cp -arfpu $(TOP)/mysql/staged/usr/share/mysql/english . && \
	cp -arfpu $(TOP)/mysql/staged/usr/share/mysql/fill_help_tables.sql . && \
	cp -arfpu $(TOP)/mysql/staged/usr/share/mysql/mysql_system_tables.sql . && \
	cp -arfpu $(TOP)/mysql/staged/usr/share/mysql/mysql_system_tables_data.sql .

lzo/stamp-h1:
	cd lzo && \
	CFLAGS="-O3 -Wall -fPIC $(EXTRACFLAGS)" $(CONFIGURE) --enable-shared --enable-static
	touch lzo/stamp-h1

lzo: lzo/stamp-h1
	@$(MAKE) -C lzo $(PARALLEL_BUILD)

lzo-clean:
	-@$(MAKE) -C lzo clean
	@rm -f lzo/stamp-h1

lzo-install: lzo
	install -D lzo/src/.libs/liblzo2.so $(INSTALLDIR)/lzo/usr/lib/liblzo2.so.2.0.0
	$(STRIP) $(INSTALLDIR)/lzo/usr/lib/liblzo2.so.2.0.0
	cd $(INSTALLDIR)/lzo/usr/lib && \
		ln -sf liblzo2.so.2.0.0 liblzo2.so.2 && \
		ln -sf liblzo2.so.2.0.0 liblzo2.so

openvpn/.conf: openssl lzo
	cd openvpn && autoreconf && \
	CFLAGS="-O3 -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="-L$(TOP)/openssl -L$(TOP)/lzo/src/.libs -ffunction-sections -fdata-sections -Wl,--gc-sections" \
	CPPFLAGS="-I$(TOP)/lzo/include -I$(TOP)/openssl/include" \
	$(CONFIGURE) --prefix= \
	--with-crypto-library=openssl \
	--disable-debug --disable-plugin-auth-pam --disable-plugin-down-root --with-plugindir=/lib --enable-management --disable-small \
	--disable-selinux --disable-socks \
	OPENSSL_SSL_CFLAGS="-I$(TOP)/openssl/include" \
	OPENSSL_SSL_LIBS="-L$(TOP)/openssl -lssl" \
	ac_cv_lib_resolv_gethostbyname=no
	touch openvpn/.conf

openvpn: openvpn/.conf
	@$(MAKE) -C openvpn $(PARALLEL_BUILD)

openvpn-clean:
	-@$(MAKE) -C openvpn clean
	@rm -f openvpn/.conf

openvpn-install: openvpn
	install -D openvpn/src/openvpn/.libs/openvpn $(INSTALLDIR)/openvpn/usr/sbin/openvpn
	$(STRIP) -s $(INSTALLDIR)/openvpn/usr/sbin/openvpn
	chmod 0500 $(INSTALLDIR)/openvpn/usr/sbin/openvpn

openvpn_plugin_auth_nvram:nvram

#shibby
nano/stamp-h1: libncurses
	cd nano && CC=$(CC) STRIP='mipsel-uclibc-strip' \
		CFLAGS="-Os -Wall $(EXTRACFLAGS) -I$(TOP)/libncurses/staged/usr/include -ffunction-sections -fdata-sections" \
		CPPFLAGS="-Os -Wall $(EXTRACFLAGS) -I$(TOP)/libncurses/staged/usr/include -ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -L$(TOP)/libncurses/staged/usr/lib -fPIC" \
		ac_cv_lib_ncursesw_get_wch=no \
		$(CONFIGURE) --prefix=/usr --disable-nls --enable-tiny --without-libiconv-prefix --disable-utf8
	touch nano/stamp-h1

nano: nano/stamp-h1
	@$(MAKE) -C nano $(PARALLEL_BUILD)

nano-clean:
	-@$(MAKE) -C nano clean
	@rm -f nano/stamp-h1 nano/Makefile nano/src/Makefile

nano-install: nano
	install -d $(INSTALLDIR)/nano/usr/sbin
	install -D nano/src/nano $(INSTALLDIR)/nano/usr/sbin/nano
	$(STRIP) -s $(INSTALLDIR)/nano/usr/sbin/nano

ifeq ($(TCONFIG_AIO),y)
CACERT:=--with-ca-bundle=/rom/cacert.pem
else
CACERT:=
endif

libcurl/stamp-h1: zlib openssl
	cd libcurl && CC=$(CC) STRIP='mipsel-uclibc-strip' \
		CPPFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections -I$(TOP)/zlib" \
		LDFLAGS="-L$(TOP)/zlib -ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
		$(CONFIGURE) --prefix=$(TOP)/libcurl/staged --disable-thread --enable-shared --enable-static \
		--enable-ares --enable-cookies --enable-crypto-auth --enable-nonblocking \
		--enable-file --enable-ftp --enable-http --enable-ipv6 --enable-tftp \
		--disable-nls --disable-ares --disable-dict --disable-debug --disable-gopher \
		--disable-ldap --disable-manual --disable-telnet --disable-verbose \
		--with-random="/dev/urandom" --with-ssl="$(TOP)/openssl/ssl" \
		--without-gnutls --without-krb4 --without-libidn \
		--with-linux-headers=$(LINUXDIR)/include $(CACERT) \
		OPENSSL_CFLAGS="-I$(TOP)/openssl/include" \
		OPENSSL_LIBS="-L$(TOP)/openssl -lcrypto -lssl"
		touch libcurl/stamp-h1

libcurl: libcurl/stamp-h1
	@$(MAKE) -C libcurl $(PARALLEL_BUILD)
	@$(MAKE) -C libcurl install

libcurl-clean:
	-@$(MAKE) -C libcurl clean
	@rm -f libcurl/stamp-h1 libcurl/Makefile
	@rm -rf libcurl/staged

libcurl-install: libcurl
	install -d $(INSTALLDIR)/libcurl/usr/sbin
	install -D libcurl/src/.libs/curl $(INSTALLDIR)/libcurl/usr/sbin/curl
	$(STRIP) -s $(INSTALLDIR)/libcurl/usr/sbin/curl
	install -d $(INSTALLDIR)/libcurl/usr/lib
	install -D libcurl/lib/.libs/libcurl.so.4.4.0 $(INSTALLDIR)/libcurl/usr/lib/libcurl.so.4.4.0
	$(STRIP) -s $(INSTALLDIR)/libcurl/usr/lib/libcurl.so.4.4.0
	cd $(INSTALLDIR)/libcurl/usr/lib/ && \
		ln -sf libcurl.so.4.4.0 libcurl.so.4 && \
		ln -sf libcurl.so.4.4.0 libcurl.so

libevent/stamp-h1:
	cd libevent && CC=$(CC) \
		CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
		$(CONFIGURE)
		touch libevent/stamp-h1

libevent: libevent/stamp-h1
	@$(SEP)
	$(MAKE) -C libevent $(PARALLEL_BUILD)
	$(MAKE) -C libevent DESTDIR=$(TOP)/libevent/staged install

libevent-clean:
	-@$(MAKE) -C libevent clean
	@rm -f libevent/stamp-h1 libevent/Makefile
	@rm -rf libevent/staged

libevent-install: libevent
	install -d $(INSTALLDIR)/libevent/usr/lib
	install libevent/.libs/libevent-2.0.so.5.1.9 $(INSTALLDIR)/libevent/usr/lib/libevent-2.0.so.5
	$(STRIP) -s $(INSTALLDIR)/libevent/usr/lib/libevent-2.0.so.5

libiconv/stamp-h1:
	cd libiconv && CC=$(CC) \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	$(CONFIGURE) --prefix=/usr --disable-nls --enable-static --enable-shared
	touch libiconv/stamp-h1

libiconv: libiconv/stamp-h1
	@$(SEP)
	$(MAKE) -C libiconv $(PARALLEL_BUILD)
	$(MAKE) -C libiconv DESTDIR=$(TOP)/libiconv/staged install

libiconv-clean:
	-@$(MAKE) -C libiconv clean
	@rm -rf libiconv/stamp-h1 libiconv/Makefile
	@rm -rf libiconv/staged

libiconv-install: libiconv
	install -d $(INSTALLDIR)/libiconv/usr/lib
	install libiconv/lib/.libs/libiconv.so.2.6.0 $(INSTALLDIR)/libiconv/usr/lib/libiconv.so.2.6.0
	$(STRIP)-s $(INSTALLDIR)/libiconv/usr/lib/libiconv.so.2.6.0
	cd $(INSTALLDIR)/libiconv/usr/lib/ && \
		ln -sf libiconv.so.2.6.0 libiconv.so.2 && \
		ln -sf libiconv.so.2.6.0 libiconv.so

transmission/stamp-h1: openssl libcurl libevent zlib
	cd transmission && CC=$(CC) STRIP='mipsel-uclibc-strip' \
		autoreconf && \
		CFLAGS="-Os -Wall -fno-delete-null-pointer-checks -funit-at-a-time \
			--param large-function-growth=800 --param max-inline-insns-single=3000 \
			-ffunction-sections -fdata-sections" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC \
			-L$(TOP)/zlib -L$(TOP)/openssl -L$(TOP)/libcurl/lib/.libs -L$(TOP)/libevent/.libs" \
		HAVE_CXX=yes \
		$(CONFIGURE) --target=mipsel-linux --host=mipsel-linux --build=x86_64-linux-gnu --prefix=/usr --disable-nls \
			--enable-lightweight --enable-largefile --enable-utp --without-gtk \
			LIBCURL_CFLAGS="-I$(TOP)/libcurl/include" \
			LIBCURL_LDFLAGS="-L$(TOP)/libcurl/lib/.libs/" \
			LIBCURL_LIBS="-lcurl" \
			LIBEVENT_CFLAGS="-I$(TOP)/libevent/include" \
			LIBEVENT_LDFLAGS="-L$(TOP)/libevent/.libs" \
			LIBEVENT_LIBS="-levent" \
			OPENSSL_CFLAGS="-I$(TOP)/openssl/include" \
			OPENSSL_LDFLAGS="-L$(TOP)/openssl" \
			OPENSSL_LIBS="-lcrypto -lssl" \
			ZLIB_CFLAGS="-I$(TOP)/zlib" \
			ZLIB_LDFLAGS="-L$(TOP)/zlib" \
			ZLIB_LIBS="-lz"
		touch transmission/stamp-h1

transmission: transmission/stamp-h1
	@$(SEP)
	$(MAKE) -C transmission $(PARALLEL_BUILD)

transmission-clean:
	-@$(MAKE) -C transmission clean
	@rm -f transmission/stamp-h1 transmission/Makefile

transmission-install: transmission
	$(MAKE) -C transmission DESTDIR=$(INSTALLDIR)/transmission install-strip
	@rm -rf $(INSTALLDIR)/transmission/usr/share/man
	@rm -rf $(INSTALLDIR)/transmission/usr/bin/transmission-cli
	@rm -rf $(INSTALLDIR)/transmission/usr/bin/transmission-show
	@rm -rf $(INSTALLDIR)/transmission/usr/bin/transmission-edit
	@rm -rf $(INSTALLDIR)/transmission/usr/bin/transmission-create
	$(STRIP) -s $(INSTALLDIR)/transmission/usr/bin/transmission-daemon
ifeq ($(TCONFIG_TR_EXTRAS),y)
	$(STRIP) -s $(INSTALLDIR)/transmission/usr/bin/transmission-remote
else
	@rm -rf $(INSTALLDIR)/transmission/usr/bin/transmission-remote
endif

btgui:
	@$(SEP)

btgui-install: btgui
	install -d $(INSTALLDIR)/btgui/usr/bin
	install -D transmission/btcheck $(INSTALLDIR)/btgui/usr/bin/btcheck

libnfsidmap/stamp-h1:
	cd libnfsidmap && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	ac_cv_func_malloc_0_nonnull=yes \
	ac_cv_func_realloc_0_nonnull=yes \
	$(CONFIGURE) --prefix=/usr --enable-shared --enable-static
	touch libnfsidmap/stamp-h1

libnfsidmap: libnfsidmap/stamp-h1
	@$(SEP)
	$(MAKE) -C libnfsidmap $(PARALLEL_BUILD)

libnfsidmap-clean:
	-@$(MAKE) -C libnfsidmap clean
	@rm -f libnfsidmap/stamp-h1
	@rm -rf libnfsidmap/staged

libnfsidmap-install: libnfsidmap
	install -d $(TOP)/libnfsidmap/staged
	$(MAKE) -C libnfsidmap DESTDIR=$(TOP)/libnfsidmap/staged install

portmap/stamp-h1:
	cd portmap \
	CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	$(MAKE) -C portmap CC=$(CC) AR=$(AR) LD=$(LD) RANLIB=$(RANLIB) RPCUSER="nobody"
	touch portmap/stamp-h1

portmap: portmap/stamp-h1
	@$(SEP)
	$(MAKE) -C portmap $(PARALLEL_BUILD)

portmap-clean:
	-@$(MAKE) -C portmap clean
	@rm -f portmap/stamp-h1

portmap-install: portmap
	install -d $(INSTALLDIR)/portmap/usr/sbin
	install -D portmap/portmap $(INSTALLDIR)/portmap/usr/sbin/portmap
	$(STRIP) -s $(INSTALLDIR)/portmap/usr/sbin/portmap

e2fsprogs/stamp-h1:
	cd e2fsprogs && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=$(TOP)/e2fsprogs/staged --enable-shared --enable-static
	touch e2fsprogs/stamp-h1

e2fsprogs: e2fsprogs/stamp-h1
	@$(SEP)
	$(MAKE) -C e2fsprogs $(PARALLEL_BUILD)

e2fsprogs-clean:
	-@$(MAKE) -C e2fsprogs clean
	@rm -f e2fsprogs/stamp-h1
	@rm -f e2fsprogs/Makefile
	@rm -rf e2fsprogs/staged

nfs-utils/stamp-h1: libevent-install e2fsprogs portmap libnfsidmap-install
	cd nfs-utils && STRIP='mipsel-uclibc-strip' \
	CFLAGS="-Os -Wall -fno-delete-null-pointer-checks -funit-at-a-time \
	-I$(TOP)/libevent/staged/usr/local/include -I$(TOP)/e2fsprogs/lib \
	-I$(TOP)/libnfsidmap/staged/usr/include -ffunction-sections -fdata-sections" \
	LDFLAGS="-L$(TOP)/libevent/staged/usr/local/lib -L$(TOP)/e2fsprogs/lib \
	-L$(TOP)/libnfsidmap/staged/usr/lib -ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	ac_cv_func_malloc_0_nonnull=yes \
	ac_cv_func_realloc_0_nonnull=yes \
	$(CONFIGURE) --disable-gss --without-tcp-wrappers --enable-static --enable-shared \
	--enable-nfsv4 --enable-nfsv3
	touch nfs-utils/stamp-h1

nfs-utils: nfs-utils/stamp-h1
	@$(SEP)
	$(MAKE) -C nfs-utils $(PARALLEL_BUILD)

nfs-utils-clean:
	-@$(MAKE) -C nfs-utils clean
	@rm -f nfs-utils/stamp-h1

nfs-utils-install: nfs-utils
	install -d $(INSTALLDIR)/nfs-utils/usr/sbin
	install -D nfs-utils/utils/mount/mount.nfs $(INSTALLDIR)/nfs-utils/usr/sbin/mount.nfs
	install -D nfs-utils/utils/nfsd/nfsd $(INSTALLDIR)/nfs-utils/usr/sbin/nfsd
	install -D nfs-utils/utils/showmount/showmount $(INSTALLDIR)/nfs-utils/usr/sbin/showmount
	install -D nfs-utils/utils/exportfs/exportfs $(INSTALLDIR)/nfs-utils/usr/sbin/exportfs
	install -D nfs-utils/utils/statd/statd $(INSTALLDIR)/nfs-utils/usr/sbin/statd
	install -D nfs-utils/utils/mountd/mountd $(INSTALLDIR)/nfs-utils/usr/sbin/mountd
	install -D nfs-utils/nfs.rc $(INSTALLDIR)/nfs-utils/usr/sbin/nfs.rc
	$(STRIP) -s $(INSTALLDIR)/nfs-utils/usr/sbin/mount.nfs
	$(STRIP) -s $(INSTALLDIR)/nfs-utils/usr/sbin/nfsd
	$(STRIP) -s $(INSTALLDIR)/nfs-utils/usr/sbin/showmount
	$(STRIP) -s $(INSTALLDIR)/nfs-utils/usr/sbin/exportfs
	$(STRIP) -s $(INSTALLDIR)/nfs-utils/usr/sbin/statd
	$(STRIP) -s $(INSTALLDIR)/nfs-utils/usr/sbin/mountd

tinc/stamp-h1: openssl zlib lzo
	@cd tinc && autoreconf -fsi && \
	CC=$(CC) AR=$(AR) RANLIB=$(RANLIB) LD=$(LD) \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -fPIC -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -fPIC" \
	$(CONFIGURE) \
	--prefix=/usr \
	--sysconfdir=/etc \
	--localstatedir=/var \
	--with-zlib-include="$(TOP)/zlib" \
	--with-zlib-lib="$(TOP)/zlib" \
	--with-lzo-include="$(TOP)/lzo/include" \
	--with-lzo-lib="$(TOP)/lzo/src/.libs" \
	--with-openssl-include="$(TOP)/openssl/include" \
	--with-openssl-lib="$(TOP)/openssl" \
	--disable-curses \
	--disable-readline
	@touch tinc/stamp-h1

tinc: tinc/stamp-h1
	@$(SEP)
	@$(MAKE) -C tinc LIBS="-lcrypto $(TOP)/zlib/libz.a -llzo2 -lm" $(PARALLEL_BUILD)

tinc-clean:
	-@$(MAKE) -C tinc clean
	@rm -f tinc/stamp-h1

tinc-install:
	install -D tinc/src/tinc $(INSTALLDIR)/tinc/usr/sbin/tinc
	install -D tinc/src/tincd $(INSTALLDIR)/tinc/usr/sbin/tincd
	$(STRIP) $(INSTALLDIR)/tinc/usr/sbin/tinc
	$(STRIP) $(INSTALLDIR)/tinc/usr/sbin/tincd

sd-idle/stamp-h1:
	cd sd-idle \
	CFLAGS="-Os -Wall --host=mipsel-linux --target=mipsel-linux $(EXTRACFLAGS)" \
	$(MAKE) -C sd-idle
	chmod 0755 sd-idle/sd-idle
	touch sd-idle/stamp-h1

sd-idle-clean:
	-@$(MAKE) -C sd-idle clean
	@rm -f sd-idle/stamp-h1

sd-idle-install: sd-idle/stamp-h1
	install -d $(INSTALLDIR)/sd-idle/usr/bin
	install -D sd-idle/sd-idle $(INSTALLDIR)/sd-idle/usr/bin/sd-idle
	$(STRIP) -s $(INSTALLDIR)/sd-idle/usr/bin/sd-idle

snmp/stamp-h1:
	cd snmp && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	$(CONFIGURE) --prefix=/usr --with-endianness=little --enable-mini-agent --disable-debugging \
	--disable-privacy --without-opaque-special-types --with-persistent-directory=/tmp/snmp-persist \
	--with-default-snmp-version=3 --with-sys-contact=root --with-sys-location=Unknown --with-logfile=/dev/null \
	--with-out-transports=UDPIPv6,TCPIPv6,AAL5PVC,IPX,TCP,Unix --enable-shared=no --enable-static --with-gnu-ld \
	--enable-internal-md5 --with-copy-persistent-files=no --without-openssl -sysconfdir=/tmp \
	--with-mib-modules=mibII,host,mibII/ip,mibII/tcp,mibII/udp,mibII/icmp,mibII/var_route,mibII/kernel_linux,qos,ucd_snmp \
	--with-out-mib-modules=snmpv3mibs,agent_mips,agentx,notification,utilities,target --disable-ipv6 --with-defaults \
	--without-efence --without-rsaref --without-kmem-usage --without-rpm --without-dmalloc
	touch snmp/stamp-h1

snmp: snmp/stamp-h1
	@$(SEP)
	$(MAKE) -C snmp

snmp-clean:
	-@$(MAKE) -C snmp clean
	@rm -f snmp/stamp-h1

snmp-install: snmp
	install -D snmp/agent/snmpd $(INSTALLDIR)/snmp/usr/sbin/snmpd
	$(STRIP) $(INSTALLDIR)/snmp/usr/sbin/snmpd

apcupsd/stamp-h1:
	cd apcupsd && touch autoconf/variables.mak && CC=$(CC) STRIP='mipsel-uclibc-strip' \
	$(MAKE) configure && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS)" \
	LDFLAGS="-L$(TOOLCHAIN)/lib -ffunction-sections -fdata-sections" \
	$(CONFIGURE) --prefix=/usr --without-x --enable-usb --disable-pcnet --enable-cgi \
		--disable-lgd --enable-net --sysconfdir=/usr/local/apcupsd --bindir=/bin \
		--disable-dumb --disable-snmp --disable-net-snmp --with-cgi-bin=/www/apcupsd --with-serial-dev=
	touch apcupsd/stamp-h1

apcupsd: apcupsd/stamp-h1
	$(MAKE) -C apcupsd $(PARALLEL_BUILD)

apcupsd-clean:
	-@$(MAKE) -C apcupsd clean
	@rm -f apcupsd/stamp-h1
	@rm -f apcupsd/config*

apcupsd-install: apcupsd
	$(MAKE) -C apcupsd DESTDIR=$(INSTALLDIR)/apcupsd install
	@rm -rf $(INSTALLDIR)/apcupsd/sbin/apctest
	@rm -rf $(INSTALLDIR)/apcupsd/www/apcupsd/ups*.cgi
	$(STRIP) $(INSTALLDIR)/apcupsd/sbin/*
	$(STRIP) $(INSTALLDIR)/apcupsd/www/apcupsd/*

libsodium/stamp-h1:
	cd $(TOP)/libsodium && CC=$(CC) STRIP='mipsel-uclibc-strip' \
	$(CONFIGURE) --prefix=/usr --disable-ssp --enable-minimal
	touch libsodium/stamp-h1

libsodium: libsodium/stamp-h1
	$(MAKE) -C libsodium $(PARALLEL_BUILD)

libsodium-install: libsodium
	install -d $(INSTALLDIR)/libsodium/usr/lib
	install -D libsodium/src/libsodium/.libs/libsodium.so.18.2.0 $(INSTALLDIR)/libsodium/usr/lib/libsodium.so.18.2.0
	$(STRIP) -s $(INSTALLDIR)/libsodium/usr/lib/libsodium.so.18.2.0
	cd $(INSTALLDIR)/libsodium/usr/lib/ && \
		ln -sf libsodium.so.18.2.0 libsodium.so.18 && \
		ln -sf libsodium.so.18.2.0 libsodium.so

libsodium-clean:
	-$(MAKE) -C libsodium clean
	@rm -rf libsodium/stamp-h1

dnscrypt/stamp-h1: libsodium
	cd $(TOP)/dnscrypt && CC=$(CC) STRIP='mipsel-uclibc-strip' \
	autoreconf && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	CPPFLAGS="-I$(TOP)/libsodium/src/libsodium/include" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections -L$(TOP)/libsodium/src/libsodium/.libs" \
	$(CONFIGURE) --prefix=/usr --disable-ssp
	touch dnscrypt/stamp-h1

dnscrypt: dnscrypt/stamp-h1
	$(MAKE) -C dnscrypt $(PARALLEL_BUILD)

dnscrypt-install: dnscrypt
	install -D dnscrypt/src/proxy/.libs/dnscrypt-proxy $(INSTALLDIR)/dnscrypt/usr/sbin/dnscrypt-proxy
	install -D dnscrypt/src/hostip/.libs/hostip $(INSTALLDIR)/dnscrypt/usr/sbin/hostip
	$(STRIP) -s $(INSTALLDIR)/dnscrypt/usr/sbin/dnscrypt-proxy
	$(STRIP) -s $(INSTALLDIR)/dnscrypt/usr/sbin/hostip

dnscrypt-clean:
	-@$(MAKE) -C dnscrypt clean
	@rm -rf dnscrypt/stamp-h1 dnscrypt/src/dnscrypt-proxy/.deps dnscrypt/Makefile

tor/stamp-h1: openssl zlib libevent
	cd $(TOP)/tor && CC=$(CC) STRIP='mipsel-uclibc-strip' \
	autoreconf && \
	CFLAGS="-Os -Wall $(EXTRACFLAGS) -ffunction-sections -fdata-sections" \
	LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" \
	$(CONFIGURE) --prefix=/usr --with-libevent-dir=$(TOP)/libevent/staged/usr/local \
		--with-openssl-dir=$(TOP)/openssl --with-zlib-dir=$(TOP)/zlib \
		--disable-asciidoc --disable-tool-name-check --disable-unittests
	touch tor/stamp-h1

tor: tor/stamp-h1
	$(MAKE) -C tor $(PARALLEL_BUILD)

tor-install: tor
	install -D tor/src/or/tor $(INSTALLDIR)/tor/usr/sbin/tor
	$(STRIP) -s $(INSTALLDIR)/tor/usr/sbin/tor

tor-clean:
	-@$(MAKE) -C tor clean
	@rm -rf tor/stamp-h1 tor/Makefile

udpxy/stamp-h1:
	$(MAKE) -C udpxy CFLAGS="-Os -Wall --host=mipsel-linux --target=mipsel-linux $(EXTRACFLAGS)" \
		LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections"
	touch udpxy/stamp-h1

udpxy: udpxy/stamp-h1
	@$(SEP)

udpxy-clean:
	-@$(MAKE) -C udpxy clean
	@rm -f udpxy/stamp-h1

udpxy-install: udpxy
	@$(SEP)
	install -d $(INSTALLDIR)/udpxy/usr/bin
	install -D udpxy/udpxy $(INSTALLDIR)/udpxy/bin/udpxy
	$(STRIP) -s $(INSTALLDIR)/udpxy/bin/udpxy

mdadm:
	$(MAKE) -C mdadm $(PARALLEL_BUILD)

mdadm-install: mdadm
	install -D mdadm/mdadm $(INSTALLDIR)/mdadm/usr/sbin/mdadm
	$(STRIP) -s $(INSTALLDIR)/mdadm/usr/sbin/mdadm

mdadm-clean:
	-@$(MAKE) -C mdadm clean

ipset:
	$(MAKE) -C ipset binaries

ipset-install: ipset
	install -D ipset/ipset $(INSTALLDIR)/ipset/usr/sbin/ipset
	install -d $(INSTALLDIR)/ipset/usr/lib/
	install ipset/*.so $(INSTALLDIR)/ipset/usr/lib/
	$(STRIP) $(INSTALLDIR)/ipset/usr/lib/*.so
	$(STRIP) $(INSTALLDIR)/ipset/usr/sbin/ipset

ipset-clean:
	-@$(MAKE) -C ipset clean

libjson-c/stamp-h1:
	cd $(TOP)/libjson-c && CC=$(CC) STRIP=$(CROSS_COMPILE)strip \
	./autogen.sh && \
	$(CONFIGURE) --prefix=/usr
	touch libjson-c/stamp-h1

libjson-c: libjson-c/stamp-h1
	$(MAKE) -C libjson-c $(PARALLEL_BUILD)

libjson-c-clean:
	-@$(MAKE) -C libjson-c clean
	@rm -rf libjson-c/stamp-h1

uqmi: libjson-c
	$(MAKE) -C uqmi/libubox
	$(MAKE) -C uqmi SHARED=0 CC='$(CC) -static'

uqmi-install: uqmi
	install -D uqmi/uqmi $(INSTALLDIR)/uqmi/usr/sbin/uqmi
	$(STRIP) $(INSTALLDIR)/uqmi/usr/sbin/uqmi

uqmi-clean: libjson-c-clean
	-@$(MAKE) -C uqmi/libubox clean
	-@$(MAKE) -C uqmi clean

comgt:
	@$(MAKE) -C comgt CC=$(CC) CFLAGS="$(EXTRACFLAGS)" LDFLAGS="-ffunction-sections -fdata-sections -Wl,--gc-sections" comgt

comgt-install: comgt
	install -D comgt/comgt $(INSTALLDIR)/comgt/usr/sbin/comgt
	cd $(INSTALLDIR)/comgt/usr/sbin/ && \
		ln -sf comgt gcom
	$(STRIP) -s $(INSTALLDIR)/comgt/usr/sbin/comgt
	@mkdir -p $(TARGETDIR)/rom/etc/gcom
	@cp -f comgt/gcom/* $(TARGETDIR)/rom/etc/gcom/

comgt-clean:
	-@$(MAKE) -C comgt clean

#
# Generic rules
#

%:
	@[ ! -d $* ] || ( $(SEP); $(MAKE) -C $* )


%-clean:
	@-[ ! -d $* ] || $(MAKE) -C $* clean

%-distclean:
	@-[ ! -d $* ] || $(MAKE) -C $* distclean

%-install: %
	@[ ! -d $* ] || $(MAKE) -C $* install INSTALLDIR=$(INSTALLDIR)/$*

%-build:
	$(MAKE) $*-clean $*

$(obj-y) $(obj-n) $(obj-clean) $(obj-install): dummy

.PHONY: all clean distclean mrproper install package
.PHONY: conf mconf oldconf kconf kmconf config menuconfig oldconfig
.PHONY: dummy libnet libpcap
