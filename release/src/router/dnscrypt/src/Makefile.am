
SUBDIRS = \
	ext \
	libevent-modified

if USE_BUILTIN_LIBSODIUM
SUBDIRS += \
	libsodium
endif

SUBDIRS += \
	proxy \
	hostip

if PLUGINS
SUBDIRS += \
	include \
	plugins
endif
