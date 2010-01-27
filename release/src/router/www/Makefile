
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

# remove "debug.js" references, convert color.css, remove comments
# in between REMOVE-BEGIN and REMOVE-END, and compress whitespace

	for F in $(wildcard *.asp *.svg); do \
		sed	-e "/REMOVE-BEGIN/,/REMOVE-END/d" \
			-e "s,<script[^>]\+debug\.js[^>]\+></script>,," \
			-e "s,<link[^>]\+href='color\.css'>,<% css(); %>," \
			-e "s,color\.css,<% nv('web_css'); %>\.css," \
			-e "s,<!-- / / / -->,," \
			-e "/^$$/d" \
			-e "/^<!--$$/,/^-->$$/! { s,^\s\+, , }" $$F > $(INSTALLDIR)/www/$$F; \
	done

# Only include the vpn pages if OpenVPN is compiled in
# Remove AES ciphers from the GUI if openssl doesn't have an AES directory

ifeq ($(TCONFIG_OPENVPN),y)
	test -d ../openssl/crypto/aes || sed -i $(INSTALLDIR)/www/vpn.js -e "/AES-BEGIN/,/AES-END/d"
else
	rm -f $(INSTALLDIR)/www/vpn*
	sed -i $(INSTALLDIR)/www/tomato.js -e "/VPN-BEGIN/,/VPN-END/d"
endif

ifeq ($(TOMATO_EXPERIMENTAL),1)
	cd $(INSTALLDIR)/www && \
	for F in $(wildcard *.asp); do \
		sed -e "s,<div class='title'>Tomato</div>,<div class='title'>Tomato <small><i>(beta)</i></small></div>," $$F > $$F.tmp; \
		mv $$F.tmp $$F; \
	done
endif



# make sure old and debugging crap is gone
	@rm -f $(INSTALLDIR)/www/debug.js
	@rm -f $(INSTALLDIR)/www/*-x.*
	@rm -f $(INSTALLDIR)/www/*-old.*
	@rm -f $(INSTALLDIR)/www/color.css

	chmod 0644 $(INSTALLDIR)/www/*
