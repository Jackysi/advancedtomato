
include ../common.mak

all:

clean:

install:
	rm -rf $(INSTALLDIR)/www
	mkdir -p $(INSTALLDIR)/www

	cp *.gif *.png *.ico robots.txt $(INSTALLDIR)/www

# squish some of the files by trimming whitespace...

	for F in $(wildcard *.js *.jsx); do \
		sed '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^\/\/ --\+\s*/d; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

	for F in $(wildcard *.css); do \
		sed '/^\/\*\s*$$/,/\*\//! { s/\s\+/ /g; s/^\s\+//; s/\s\+$$//; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

# remove "debug.js" references, convert color.css -> nv().css, remove comments
# in between REMOVE-BEGIN and REMOVE-END, and compress whitespace

	for F in $(wildcard *.asp *.svg); do \
		sed	-e "/REMOVE-BEGIN/,/REMOVE-END/d" \
			-e "s,<script[^>]\+debug\.js[^>]\+></script>,," \
			-e "s,color\.css,<% nv('web_css'); %>\.css," \
			-e "s,<!-- / / / -->,," \
			-e "/^$$/d" \
			-e "/^<!--$$/,/^-->$$/! { s,^\s\+, , }" $$F > $(INSTALLDIR)/www/$$F; \
	done
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
# Only include the CIFS pages if CIFS is configured in.
ifneq ($(TCONFIG_CIFS),y)
	rm -f $(INSTALLDIR)/www/admin-cifs.asp
	sed -i $(INSTALLDIR)/www/tomato.js -e "/CIFS-BEGIN/,/CIFS-END/d"
endif

# Only include extra themes if USB Extras are configured in.
ifneq ($(TCONFIG_USB_EXTRAS),y)
	rm -f $(INSTALLDIR)/www/usbred.css
	rm -f $(INSTALLDIR)/www/usbblue.css
	rm -f $(INSTALLDIR)/www/tomatousb.png
	rm -f $(INSTALLDIR)/www/tomatousb_bg.png
	sed -i $(INSTALLDIR)/www/admin-access.asp -e "/THEMES-BEGIN/,/THEMES-END/d"
endif

# Only include the vpn pages in the navigation if OpenVPN is compiled in
ifneq ($(TCONFIG_OPENVPN),y)
	sed -i $(INSTALLDIR)/www/tomato.js -e "/VPN-BEGIN/,/VPN-END/d"
	rm -f $(INSTALLDIR)/www/vpn.js
	rm -f $(INSTALLDIR)/www/vpn-client.asp
	rm -f $(INSTALLDIR)/www/vpn-server.asp
endif

# install extra vpngui files and directories

ifeq ($(TCONFIG_OPENVPN),y)
	mkdir -p $(INSTALLDIR)/rom/openvpn
	cp openvpn-updown.sh $(INSTALLDIR)/rom/openvpn/updown.sh
	chmod 0755 $(INSTALLDIR)/rom/openvpn/updown.sh
endif

# make sure old and debugging crap is gone
	@rm -f $(INSTALLDIR)/www/debug.js
	@rm -f $(INSTALLDIR)/www/*-x.*
	@rm -f $(INSTALLDIR)/www/*-old.*
	@rm -f $(INSTALLDIR)/www/color.css

	chmod 0644 $(INSTALLDIR)/www/*
