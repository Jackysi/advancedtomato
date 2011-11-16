
include ../common.mak

all:

clean:

install:
	rm -rf $(INSTALLDIR)/www
	mkdir -p $(INSTALLDIR)/www

	cp *.gif *.png *.ico *.html robots.txt $(INSTALLDIR)/www

# squish some of the files by trimming whitespace...

	for F in $(wildcard *.js *.jsx); do \
		sed '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^\/\/ --\+\s*/d; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

	for F in $(wildcard *.css); do \
		sed '/^\/\*\s*$$/,/\*\//! { s/\s\+/ /g; s/^\s\+//; s/\s\+$$//; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

# remove "debug.js" references, convert color.css, remove comments
# in between REMOVE-BEGIN and REMOVE-END, and compress whitespace

	for F in $(wildcard *.asp *.svg); do \
		sed	-e "/REMOVE-BEGIN/,/REMOVE-END/d" \
			-e "s,<script[^>]\+debug\.js[^>]\+></script>,," \
			-e "s,<link[^>]\+href='color\.css'>,<% css(); %>," \
			-e "s,color\.css,<% nv('web_css'); %>\.css," \
			-e "s,<!-- / / / -->,," \
			-e "s,\x0d,," \
			-e "s,^\s\+,," \
			-e "/^$$/d" \
			-e "/^<!--$$/,/^-->$$/! { s,^\s\+, , }" $$F > $(INSTALLDIR)/www/$$F; \
	done

# Only include Linux 2.6 options if building for Linux 2.6.
ifneq ($(CONFIG_LINUX26),y)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js); do \
		sed -i $$F -e "/LINUX26-BEGIN/,/LINUX26-END/d"; \
	done
endif

# Only include the CIFS pages if CIFS is configured in.
ifneq ($(TCONFIG_CIFS),y)
	rm -f $(INSTALLDIR)/www/admin-cifs.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/CIFS-BEGIN/,/CIFS-END/d"
	sed -i $(INSTALLDIR)/www/admin-bwm.asp -e "/CIFS-BEGIN/,/CIFS-END/d"
	sed -i $(INSTALLDIR)/www/nas-media.asp -e "/CIFS-BEGIN/,/CIFS-END/d"
endif

# Only include the JFFS pages if JFFS is configured in.
ifneq ($(TCONFIG_JFFS2),y)
	rm -f $(INSTALLDIR)/www/admin-jffs2.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/admin-bwm.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/admin-upgrade.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
	sed -i $(INSTALLDIR)/www/nas-media.asp -e "/JFFS2-BEGIN/,/JFFS2-END/d"
endif

# Only include the Zebra options if Zebra is configured in.
ifneq ($(TCONFIG_ZEBRA),y)
	sed -i $(INSTALLDIR)/www/advanced-routing.asp -e "/ZEBRA-BEGIN/,/ZEBRA-END/d"
endif

# Only include EMF options if EMF is configured in.
ifneq ($(TCONFIG_EMF),y)
	sed -i $(INSTALLDIR)/www/advanced-routing.asp -e "/EMF-BEGIN/,/EMF-END/d"
endif

# Only include extra themes if USB Extras are configured in.
ifneq ($(TCONFIG_USB_EXTRAS),y)
	rm -f $(INSTALLDIR)/www/usbred.css
	rm -f $(INSTALLDIR)/www/usbblue.css
	rm -f $(INSTALLDIR)/www/tomatousb.png
	rm -f $(INSTALLDIR)/www/tomatousb_bg.png
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/THEMES-BEGIN/,/THEMES-END/d"
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/EXTRAS-BEGIN/,/EXTRAS-END/d"
endif

# Only include NTFS settings if NTFS support is configured in.
ifneq ($(TCONFIG_NTFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/NTFS-BEGIN/,/NTFS-END/d"
endif
# Only include the FTP pages if FTP Server is configured in.
ifneq ($(TCONFIG_FTP),y)
	rm -f $(INSTALLDIR)/www/nas-ftp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/FTP-BEGIN/,/FTP-END/d"
endif
# Only include the Samba pages if Samba is configured in.
ifneq ($(TCONFIG_SAMBASRV),y)
	rm -f $(INSTALLDIR)/www/nas-samba.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/SAMBA-BEGIN/,/SAMBA-END/d"
endif
# Only include the Media Server pages if Media Server is configured in.
ifneq ($(TCONFIG_MEDIA_SERVER),y)
	rm -f $(INSTALLDIR)/www/nas-media.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/MEDIA-SRV-BEGIN/,/MEDIA-SRV-END/d"
endif

# Victek for RAF verion
# Only include Captive Portal menu option and pages when configured.
ifneq ($(TCONFIG_NOCAT),y)
	rm -f $(INSTALLDIR)/www/new-splashd.asp
	rm -f $(INSTALLDIR)/www/splash.html
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NOCAT-BEGIN/,/NOCAT-END/d"
endif

# Only include IPv6 options if IPv6 is configured in.
ifneq ($(TCONFIG_IPV6),y)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx); do \
		[ -f $(INSTALLDIR)/www/$$F ] && sed -i $$F \
		-e "/IPV6-BEGIN/,/IPV6-END/d" \
		|| true; \
	done
	rm -f $(INSTALLDIR)/www/basic-ipv6.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/IPV6-BEGIN/,/IPV6-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/IPV6-BEGIN/,/IPV6-END/d"
	sed -i $(INSTALLDIR)/www/admin-debug.asp -e "/IPV6-BEGIN/,/IPV6-END/d"
	sed -i $(INSTALLDIR)/www/advanced-dhcpdns.asp -e "/IPV6-BEGIN/,/IPV6-END/d"
endif

# Only include the USB and NAS pages if USB Support is configured in.
ifneq ($(TCONFIG_USB),y)
	rm -f $(INSTALLDIR)/www/nas-*.*
	sed -i $(INSTALLDIR)/www/tomato.js -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-buttons.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/USB-BEGIN/,/USB-END/d"
endif

# Only include CTF option if CTF module exists
##	test -d $(SRCBASE)/ctf/linux || 
	sed -i $(INSTALLDIR)/www/advanced-misc.asp -e "/CTF-BEGIN/,/CTF-END/d"

# Only include VLAN-GUI pages if VLAN-GUI is configured in.
ifneq ($(TCONFIG_VLAN),y)
	rm -f $(INSTALLDIR)/www/advanced-vlan.asp
	rm -f $(INSTALLDIR)/www/advanced-access.asp
	sed -i $(INSTALLDIR)/www/about.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/advanced-firewall.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/advanced-routing.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/forward-upnp.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/ VLAN-BEGIN/,/ VLAN-END/d"
	sed -i $(INSTALLDIR)/www/tomato.js -e "/ VLAN-BEGIN/,/ VLAN-END/d"
else
	sed -i $(INSTALLDIR)/www/advanced-routing.asp -e "/ NOVLAN-BEGIN/,/ NOVLAN-END/d"
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/ NOVLAN-BEGIN/,/ NOVLAN-END/d"
	sed -i $(INSTALLDIR)/www/status-devices.asp -e "/ NOVLAN-BEGIN/,/ NOVLAN-END/d"
	sed -i $(INSTALLDIR)/www/status-overview.asp -e "/ NOVLAN-BEGIN/,/ NOVLAN-END/d"
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp); do \
		sed -e "s,<div class='title'>Tomato</div>,<div class='title'>Tomato <small><i>(beta)</i></small></div>," $$F > $$F.tmp; \
		mv $$F.tmp $$F; \
	done
endif

# Only include the vpn pages if OpenVPN is compiled in
# Remove AES ciphers from the GUI if openssl doesn't have an AES directory
ifeq ($(TCONFIG_OPENVPN),y)
	test -d ../openssl/crypto/aes || sed -i $(INSTALLDIR)/www/vpn.js -e "/AES-BEGIN/,/AES-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/PPTP-BEGIN/,/PPTP-END/d"
else
	rm -f $(INSTALLDIR)/www/vpn-{client,server}.asp $(INSTALLDIR)/www/vpn.js
	sed -i $(INSTALLDIR)/www/tomato.js -e "/OPENVPN-BEGIN/,/OPENVPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/OPENVPN-BEGIN/,/OPENVPN-END/d"
ifneq ($(TCONFIG_PPTP),y)
	sed -i $(INSTALLDIR)/www/tomato.js -e "/VPN-BEGIN/,/VPN-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/VPN-BEGIN/,/VPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/VPN-BEGIN/,/VPN-END/d"
endif
endif

# Only include the snmp pages if SNMP is compiled in
ifneq ($(TCONFIG_SNMP),y)
	rm -f $(INSTALLDIR)/www/admin-snmp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/SNMP-BEGIN/,/SNMP-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/SNMP-BEGIN/,/SNMP-END/d"
endif

ifneq ($(TCONFIG_PPTP),y)
	rm -f $(INSTALLDIR)/www/vpn-pptp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/PPTP-BEGIN/,/PPTP-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/PPTP-BEGIN/,/PPTP-END/d"
endif

# clean up
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx *.html); do \
		[ -f $(INSTALLDIR)/www/$$F ] && sed -i $$F \
		-e "/LINUX26-BEGIN/d"	-e "/LINUX26-END/d" \
		-e "/USB-BEGIN/d"	-e "/USB-END/d" \
		-e "/EXTRAS-BEGIN/d"	-e "/EXTRAS-END/d" \
		-e "/NTFS-BEGIN/d"	-e "/NTFS-END/d" \
		-e "/SAMBA-BEGIN/d"	-e "/SAMBA-END/d" \
		-e "/FTP-BEGIN/d"	-e "/FTP-END/d" \
		-e "/MEDIA-SRV-BEGIN/d"	-e "/MEDIA-SRV-END/d" \
		-e "/THEMES-BEGIN/d"	-e "/THEMES-END/d" \
		-e "/JFFS2-BEGIN/d"	-e "/JFFS2-END/d" \
		-e "/CIFS-BEGIN/d"	-e "/CIFS-END/d" \
		-e "/ZEBRA-BEGIN/d"	-e "/ZEBRA-END/d" \
		-e "/EMF-BEGIN/d"	-e "/EMF-END/d" \
		-e "/VPN-BEGIN/d"	-e "/VPN-END/d" \
		-e "/AES-BEGIN/d"	-e "/AES-END/d" \
		-e "/IPV6-BEGIN/d"	-e "/IPV6-END/d" \
		-e "/CTF-BEGIN/d"	-e "/CTF-END/d" \
		-e "/NOCAT-BEGIN/d"	-e "/NOCAT_END/d"\
		-e "/SNMP-BEGIN/d"	-e "/SNMP_END/d"\
		-e "/ VLAN-BEGIN/d"	-e "/ VLAN-END/d" \
		-e "/ NOVLAN-BEGIN/d"	-e "/ NOVLAN-END/d" \
		|| true; \
	done

# make sure old and debugging crap is gone
	@rm -f $(INSTALLDIR)/www/debug.js
	@rm -f $(INSTALLDIR)/www/*-x.*
	@rm -f $(INSTALLDIR)/www/*-old.*
	@rm -f $(INSTALLDIR)/www/color.css

	chmod 0644 $(INSTALLDIR)/www/*

# remove C-style comments from java files. All "control" comments have been processed by now.
	for F in $(wildcard *.js *.jsx); do \
		[ -f $(INSTALLDIR)/www/$$F ] && $(TOP)/www/remcoms2.sh $(INSTALLDIR)/www/$$F c; \
	done
