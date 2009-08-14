
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
# Only include NTFS settings if NTFS support is configured in.
ifneq ($(TCONFIG_NTFS),y)
	sed -i $(INSTALLDIR)/www/nas-usb.asp -e "/NTFS-BEGIN/,/NTFS-END/d"
endif

# make sure old and debugging crap is gone
	@rm -f $(INSTALLDIR)/www/debug.js
	@rm -f $(INSTALLDIR)/www/*-x.*
	@rm -f $(INSTALLDIR)/www/*-old.*
	@rm -f $(INSTALLDIR)/www/color.css

	chmod 0644 $(INSTALLDIR)/www/*
