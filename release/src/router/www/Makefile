
include ../common.mak

all:

clean:

install:
	rm -rf $(INSTALLDIR)/www
	mkdir -p $(INSTALLDIR)/www

	cp *.gif *.png *.ico robots.txt $(INSTALLDIR)/www

# squish some of the files by trimming whitespace...

	for F in *.{js,jsx}; do \
		sed '/^\/\*\s*$$/,/\*\//! { s/^\s\+//; s/\s\+$$//; /^\/\/ --\+\s*/d; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

	for F in *.css; do \
		sed '/^\/\*\s*$$/,/\*\//! { s/\s\+/ /g; s/^\s\+//; s/\s\+$$//; /^$$/d }' < $$F > $(INSTALLDIR)/www/$$F; \
	done

# remove "debug.js" references, convert color.css -> nv().css, remove comments
# in between REMOVE-BEGIN and REMOVE-END, and compress whitespace

	for F in *.{asp,svg}; do \
		sed	-e "/REMOVE-BEGIN/,/REMOVE-END/d" \
			-e "s,<script[^>]\+debug\.js[^>]\+></script>,," \
			-e "s,color\.css,<% nv('web_css'); %>\.css," \
			-e "s,<!-- / / / -->,," \
			-e "/^$$/d" \
			-e "/^<!--$$/,/^-->$$/! { s,^\s\+, , }" $$F > $(INSTALLDIR)/www/$$F; \
	done


	
# make sure old and debugging crap is gone
	@rm -f $(INSTALLDIR)/debug.js
	@rm -f $(INSTALLDIR)/www/*-x.*
	@rm -f $(INSTALLDIR)/www/*-old.*
	@rm -f $(INSTALLDIR)/www/color.css

	chmod 0644 $(INSTALLDIR)/www/*
