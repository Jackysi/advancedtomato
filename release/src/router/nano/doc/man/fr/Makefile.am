mandir = @mandir@/fr

if USE_NANORC
man_MANS = nano.1 nanorc.5 rnano.1
if GROFF_HTML
BUILT_SOURCES = nano.1.html nanorc.5.html rnano.1.html
endif
else
man_MANS = nano.1 rnano.1
if GROFF_HTML
BUILT_SOURCES = nano.1.html rnano.1.html
endif
endif

nano_man_mans = nano.1 nanorc.5 rnano.1
if GROFF_HTML
htmlman_DATA = nano.1.html nanorc.5.html rnano.1.html
htmlmandir = $(datadir)/nano/man-html/fr

nano_built_sources = nano.1.html nanorc.5.html rnano.1.html

nano.1.html: nano.1
	groff -t -mandoc -Thtml < $? > $@
nanorc.5.html: nanorc.5
	groff -t -mandoc -Thtml < $? > $@
rnano.1.html: rnano.1
	groff -t -mandoc -Thtml < $? > $@

EXTRA_DIST = $(nano_man_mans) $(nano_built_sources)
else
EXTRA_DIST = $(nano_man_mans)
endif

