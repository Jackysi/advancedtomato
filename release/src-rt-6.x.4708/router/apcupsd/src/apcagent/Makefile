topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.m) $(wildcard *.c)
LIBS := -Wl,-framework -Wl,Cocoa -Wl,-framework -Wl,Growl $(LIBS)

all-targets: apcagent MainMenu.nib apcagent.app

apcagent: $(OBJS) $(APCLIBS)
	$(LINK)

apcagent.app: apcagent MainMenu.nib Info.plist PkgInfo growl.plist \
              $(topdir)/doc/images/commlost.png \
              $(topdir)/doc/images/charging.png \
              $(topdir)/doc/images/onbatt.png \
              $(topdir)/doc/images/online.png
	@$(ECHO) "  BUNDL" $@
	$(V)$(RMF) apcagent.app
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/MacOS
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/Resources
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/Frameworks
	$(V)$(MKINSTALLDIRS) apcagent.app/Contents/Resources/English.lproj
	$(V)$(INSTALL_PROGRAM) $(STRIP) -m 755 apcagent apcagent.app/Contents/MacOS
	$(V)$(SED) -e "s/__VERSION__/${VERSION}/" Info.plist > /tmp/Info.plist.out
	$(V)$(INSTALL_DATA) -m 644 /tmp/Info.plist.out apcagent.app/Contents/Info.plist
	$(V)$(INSTALL_DATA) -m 644 PkgInfo apcagent.app/Contents
	$(V)$(INSTALL_DATA) -m 644 MainMenu.nib apcagent.app/Contents/Resources/English.lproj
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/commlost.png apcagent.app/Contents/Resources
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/charging.png apcagent.app/Contents/Resources
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/onbatt.png apcagent.app/Contents/Resources
	$(V)$(INSTALL_DATA) -m 644 $(topdir)/doc/images/online.png apcagent.app/Contents/Resources
	$(V)cp -R /Developer/SDKs/MacOSX10.4u.sdk/System/Library/Frameworks/Growl.framework apcagent.app/Contents/Frameworks
	$(V)$(INSTALL_DATA) -m 644 growl.plist apcagent.app/Contents/Resources/Growl\ Registration\ Ticket.growlRegDict

apcagent.dmg: apcagent.app
	hdiutil create -ov -srcfolder apcagent.app apcagent.dmg

all-install: install-apcagent
all-uninstall: uninstall-apcagent

install-apcagent:
	$(call MKDIR,/Applications)
	$(call COPY,apcagent.app,Applications)

uninstall-apcagent:
	$(call UNINST,/Applications/apcagent.app)

# Include dependencies
-include $(DEPS)
