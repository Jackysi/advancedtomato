ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	DNSCRYPT-V2-PROTOCOL.txt \
	README-iOS.markdown \
	README-PLUGINS.markdown \
	README-WINDOWS.markdown \
	README.markdown \
	THANKS \
	apparmor.profile.dnscrypt-proxy \
	autogen.sh \
	dnscrypt-proxy.service \
	dnscrypt-proxy.socket \
	org.dnscrypt.osx.DNSCryptProxy.plist \
	resolvers-check.sh

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

dist_pkgdata_DATA = \
	dnscrypt-resolvers.csv \
	minisign.pub

dist_sysconf_DATA = \
	dnscrypt-proxy.conf
