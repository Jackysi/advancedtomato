
include ../common.mak

all:

clean:

install:
	rm -rf $(INSTALLDIR)/www
	mkdir -p $(INSTALLDIR)/www

	cp *.ico *.html *.php robots.txt $(INSTALLDIR)/www

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
# But remove some K24 options if building for Linux 2.6.
else
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js); do \
		sed -i $$F -e "/LINUX24-BEGIN/,/LINUX24-END/d"; \
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

# Only include sd/mmc card support if MICROSD is configured in.
ifneq ($(TCONFIG_MICROSD),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/MICROSD-BEGIN/,/MICROSD-END/d"
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
	rm -f $(INSTALLDIR)/www/advanced-splashd.asp
	rm -f $(INSTALLDIR)/www/splash.html
	rm -f $(INSTALLDIR)/www/style.css
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NOCAT-BEGIN/,/NOCAT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/NOCAT-BEGIN/,/NOCAT-END/d"
endif

# Clean up NGinX files if not needed
ifneq ($(TCONFIG_NGINX),y)
	rm -f $(INSTALLDIR)/www/nginx.asp
	rm -f $(INSTALLDIR)/www/index.html
	rm -f $(INSTALLDIR)/www/index.php
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NGINX-BEGIN/,/NGINX-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/NGINX-BEGIN/,/NGINX-END/d"
endif

# Only include HFS/HFS+ option and pages when configured.
ifneq ($(TCONFIG_HFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/HFS-BEGIN/,/HFS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/HFS-BEGIN/,/HFS-END/d"
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
endif

# Only include the Transmission binary path select if Transmission binaries is configured in.
ifneq ($(TCONFIG_BBT),y)
	sed -i $(INSTALLDIR)/www/nas-bittorrent.asp -e "/BBT-BEGIN/,/BBT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/BBT-BEGIN/,/BBT-END/d"
endif

# Only include the Transmission pages if Transmission is configured in.
ifneq ($(TCONFIG_BT),y)
	rm -f $(INSTALLDIR)/www/nas-bittorrent.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/BT-BEGIN/,/BT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/BT-BEGIN/,/BT-END/d"
endif

# Only include the UPS pages if apcupsd is configured in.
ifneq ($(TCONFIG_UPS),y)
	rm -f $(INSTALLDIR)/www/nas-ups.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/UPS-BEGIN/,/UPS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/UPS-BEGIN/,/UPS-END/d"
endif

# Only include the TOR pages if tor project is configured in.
ifneq ($(TCONFIG_TOR),y)
	rm -f $(INSTALLDIR)/www/advanced-tor.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/TOR-BEGIN/,/TOR-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/TOR-BEGIN/,/TOR-END/d"
endif



# Only include the USB and NAS pages if USB Support is configured in.
ifneq ($(TCONFIG_USB),y)
	rm -f $(INSTALLDIR)/www/nas-*.*
	sed -i $(INSTALLDIR)/www/tomato.js -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-buttons.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/USB-BEGIN/,/USB-END/d"
endif

# Only include the USB and NAS pages if REMOVE_USBAPP is NOT configured in.
ifeq ($(TCONFIG_REMOVE_USBAPP),y)
	rm -f $(INSTALLDIR)/www/nas-*.*
	sed -i $(INSTALLDIR)/www/tomato.js -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-buttons.asp -e "/USB-BEGIN/,/USB-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/USB-BEGIN/,/USB-END/d"
endif

## Only include CTF option if CTF module exists
#	test -d $(SRCBASE)/ctf/linux || sed -i $(INSTALLDIR)/www/advanced-misc.asp -e "/CTF-BEGIN/,/CTF-END/d"

# Only include the CTF configuration if CTF is configured in.
ifneq ($(TCONFIG_CTF),y)
	sed -i $(INSTALLDIR)/www/advanced-misc.asp -e "/CTF-BEGIN/,/CTF-END/d"
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp); do \
		sed -e "s,<div class='title'>Tomato</div>,<div class='title'>Tomato <small><i>(beta)</i></small></div>," $$F > $$F.tmp; \
		mv $$F.tmp $$F; \
	done
endif

	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp); do \
		sed -e "s,<div class='version'>Version <% version(); %></div>,<div class='version'>Version <% version(); %> by shibby</div>," $$F > $$F.tmp; \
		mv $$F.tmp $$F; \
	done


# Only include the vpn pages if OpenVPN is compiled in
# Remove AES ciphers from the GUI if openssl doesn't have an AES directory
ifeq ($(TCONFIG_OPENVPN),y)
	test -d ../openssl/crypto/aes || sed -i $(INSTALLDIR)/www/vpn.js -e "/AES-BEGIN/,/AES-END/d"
	sed -i $(INSTALLDIR)/www/tomato.js -e "/ VPN-BEGIN/d" -e "/ VPN-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/ VPN-BEGIN/d" -e "/ VPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/ VPN-BEGIN/d" -e "/ VPN-END/d"
else
	rm -f $(INSTALLDIR)/www/vpn-server.asp
	rm -f $(INSTALLDIR)/www/vpn-client.asp
	rm -f $(INSTALLDIR)/www/js/vpn.js
	sed -i $(INSTALLDIR)/www/tomato.js -e "/ OPENVPN-BEGIN/,/ OPENVPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/ OPENVPN-BEGIN/,/ OPENVPN-END/d"
endif

# Only include the PPTPD pages if PPTPD is compiled in
ifeq ($(TCONFIG_PPTPD),y)
	sed -i $(INSTALLDIR)/www/tomato.js -e "/ VPN-BEGIN/d" -e "/ VPN-END/d"
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/ VPN-BEGIN/d" -e "/ VPN-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/ VPN-BEGIN/d" -e "/ VPN-END/d"
else
	rm -f $(INSTALLDIR)/www/vpn-pptp-server.asp
	rm -f $(INSTALLDIR)/www/vpn-pptp-online.asp
	rm -f $(INSTALLDIR)/www/vpn-pptp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/ PPTPD-BEGIN/,/ PPTPD-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/ PPTPD-BEGIN/,/ PPTPD-END/d"
endif

# Only include the nfs pages if NFS is compiled in
ifneq ($(TCONFIG_NFS),y)
	rm -f $(INSTALLDIR)/www/admin-nfs.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/NFS-BEGIN/,/NFS-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/NFS-BEGIN/,/NFS-END/d"
endif

# Only include the snmp pages if SNMP is compiled in
ifneq ($(TCONFIG_SNMP),y)
	rm -f $(INSTALLDIR)/www/admin-snmp.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/SNMP-BEGIN/,/SNMP-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/SNMP-BEGIN/,/SNMP-END/d"
endif

# Only include the mmc pages if SDHC is compiled in
ifneq ($(TCONFIG_SDHC),y)
	rm -f $(INSTALLDIR)/www/admin-sdhc.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/SDHC-BEGIN/,/SDHC-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/SDHC-BEGIN/,/SDHC-END/d"
endif

# Only include the dnscrypt option if is compiled in
ifeq ($(TCONFIG_DNSCRYPT),y)
	$(TOP)/www/dnscrypt-helper.sh $(INSTALLDIR)/../rom/rom/etc/dnscrypt-resolvers.csv $(INSTALLDIR)/www/basic-network.asp
else
	sed -i $(INSTALLDIR)/www/basic-network.asp -e "/DNSCRYPT-BEGIN/,/DNSCRYPT-END/d"
	sed -i $(INSTALLDIR)/www/about.asp -e "/DNSCRYPT-BEGIN/,/DNSCRYPT-END/d"
 endif

# clean up
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp *.js *.jsx *.html); do \
		[ -f $(INSTALLDIR)/www/$$F ] && sed -i $$F \
		-e "/LINUX26-BEGIN/d"	-e "/LINUX26-END/d" \
		-e "/LINUX24-BEGIN/d"	-e "/LINUX24-END/d" \
		-e "/USB-BEGIN/d"	-e "/USB-END/d" \
		-e "/EXTRAS-BEGIN/d"	-e "/EXTRAS-END/d" \
		-e "/NTFS-BEGIN/d"	-e "/NTFS-END/d" \
		-e "/SAMBA-BEGIN/d"	-e "/SAMBA-END/d" \
		-e "/FTP-BEGIN/d"	-e "/FTP-END/d" \
		-e "/MEDIA-SRV-BEGIN/d"	-e "/MEDIA-SRV-END/d" \
		-e "/JFFS2-BEGIN/d"	-e "/JFFS2-END/d" \
		-e "/CIFS-BEGIN/d"	-e "/CIFS-END/d" \
		-e "/ZEBRA-BEGIN/d"	-e "/ZEBRA-END/d" \
		-e "/EMF-BEGIN/d"	-e "/EMF-END/d" \
		-e "/OPENVPN-BEGIN/d"	-e "/OPENVPN-END/d" \
		-e "/AES-BEGIN/d"	-e "/AES-END/d" \
		-e "/PPTPD-BEGIN/d"	-e "/PPTPD-END/d"\
		-e "/VPN-BEGIN/,/VPN-END/d" \
		-e "/IPV6-BEGIN/d"	-e "/IPV6-END/d" \
		-e "/CTF-BEGIN/d"	-e "/CTF-END/d" \
		-e "/BBT-BEGIN/d"	-e "/BBT-END/d" \
		-e "/BT-BEGIN/d"	-e "/BT-END/d" \
		-e "/NFS-BEGIN/d"	-e "/NFS-END/d" \
		-e "/NOCAT-BEGIN/d"	-e "/NOCAT-END/d"\
        -e "/NGINX-BEGIN/d"	-e "/NGINX-END/d"\
		-e "/SNMP-BEGIN/d"	-e "/SNMP-END/d"\
		-e "/SDHC-BEGIN/d"	-e "/SDHC-END/d"\
		-e "/HFS-BEGIN/d"	-e "/HFS-END/d"\
		-e "/DNSCRYPT-BEGIN/d"	-e "/DNSCRYPT-END/d"\
		-e "/TOR-BEGIN/d"	-e "/TOR-END/d"\
		-e "/MICROSD-BEGIN/d"	-e "/MICROSD-END/d"\
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

	# Copy over the directories needed for the new AdvancedTomato stuff (img, css and js)
	mkdir -p $(INSTALLDIR)/www/img
	cp -r img/* $(INSTALLDIR)/www/img/.
	mkdir -p $(INSTALLDIR)/www/css
	cp -r css/* $(INSTALLDIR)/www/css/.
	mkdir -p $(INSTALLDIR)/www/js
	cp -r js/* $(INSTALLDIR)/www/js/.
