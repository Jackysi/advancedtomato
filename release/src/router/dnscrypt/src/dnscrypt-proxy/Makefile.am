
sbin_PROGRAMS = \
	dnscrypt-proxy

dnscrypt_proxy_SOURCES = \
	app.c \
	app.h \
	cert.c \
	cert.h \
	cert_p.h \
	dnscrypt.c \
	dnscrypt.h \
	dnscrypt_client.c \
	dnscrypt_client.h \
	dnscrypt_proxy.h \
	edns.c \
	edns.h \
	logger.c \
	logger.h \
	options.c \
	options.h \
	pid_file.c \
	pid_file.h \
	pathnames.h \
	probes_dnscrypt_proxy.d \
	probes_no_dtrace.h \
	safe_rw.c \
	safe_rw.h \
	salsa20_random.c \
	salsa20_random.h \
	stack_trace.c \
	stack_trace.h \
	tcp_request.c \
	tcp_request.h \
	tcp_request_p.h \
	udp_request.c \
	udp_request.h \
	udp_request_p.h \
	utils.c \
	utils.h \
	uv_alloc.c \
	uv_alloc.h

AM_CFLAGS = @CWFLAGS@

AM_CPPFLAGS = \
	-I../libuv/include \
	-I../@NACL_PATH@/include/local

dnscrypt_proxy_LDADD = \
	../libuv/uv.a \
	../@NACL_PATH@/lib/local/libnacl.a

dnscrypt_proxy_DEPENDENCIES = \
	../libuv/.done

BUILT_SOURCES = \
	../libnacl/.done \
	probes.h

../libnacl/.done:
	cd ../libnacl && $(MAKE) $(AM_MAKEFLAGS)

../libuv/.done:
	cd ../libuv && $(MAKE) $(AM_MAKEFLAGS)

probes.h: @PROBES_SOURCE@
	cat @PROBES_SOURCE@ > $@

probes_dnscrypt_proxy.h: probes_dnscrypt_proxy.d
	@DTRACE@ -o $@ -h -s probes_dnscrypt_proxy.d

CLEANFILES = \
	probes.h \
	probes_dnscrypt_proxy.h
