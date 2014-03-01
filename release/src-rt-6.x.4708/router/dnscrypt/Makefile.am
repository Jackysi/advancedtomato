ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	README.markdown \
	README-PLUGINS.markdown \
	README-WINDOWS.markdown \
	TECHNOTES \
	THANKS \
	apparmor.profile.dnscrypt-proxy \
	autogen.sh \
	org.dnscrypt.osx.DNSCryptProxy.plist

SUBDIRS = \
	dist-build \
	man

if PLUGINS
SUBDIRS += \
	libltdl
endif

SUBDIRS += \
	src \
	test
