include ../common.mak

srcdir=source

SMBCFLAGS = $(EXTRACFLAGS) -Os

ifneq ($(CONFIG_LINUX26),y)
SMBCFLAGS += -DNO_LOG
endif

all: .conf apps

apps:
	$(MAKE) -C $(srcdir) all

.conf:
	cd $(srcdir) && \
	ac_cv_lib_resolv_dn_expand=no \
	ac_cv_lib_resolv__dn_expand=no \
	ac_cv_lib_resolv___dn_expand=no \
	SMB_BUILD_CC_NEGATIVE_ENUM_VALUES=yes \
	libreplace_cv_READDIR_GETDIRENTRIES=no \
	libreplace_cv_READDIR_GETDENTS=no \
	linux_getgrouplist_ok=no \
	samba_cv_have_longlong=yes \
	samba_cv_HAVE_INO64_T=yes \
	samba_cv_HAVE_OFF64_T=yes \
	samba_cv_HAVE_SHARED_MMAP=yes \
	samba_cv_HAVE_STRUCT_FLOCK64=yes \
	samba_cv_SIZEOF_OFF_T=8 \
	samba_cv_HAVE_CREAT64=yes \
	samba_cv_HAVE_FSEEKO64=yes \
	samba_cv_HAVE_FSTAT64=yes \
	samba_cv_HAVE_FTELLO64=yes \
	samba_cv_HAVE_FTRUNCATE64=yes \
	samba_cv_HAVE_LSEEK64=yes \
	samba_cv_HAVE_LSTAT64=yes \
	samba_cv_HAVE_OPEN64=yes \
	samba_cv_HAVE_PREAD64=yes \
	samba_cv_HAVE_PWRITE64=yes \
	samba_cv_HAVE_READDIR64=yes \
	samba_cv_HAVE_STAT64=yes \
	samba_cv_HAVE_SIGSET=yes \
	samba_cv_HAVE_MMAP=no \
	samba_cv_HAVE_GETDIRENTRIES=yes \
	samba_cv_HAVE_GETMNTENT=yes \
	samba_cv_HAVE_SETMNTENT=yes \
	samba_cv_HAVE_ENDMNTENT=yes \
	samba_cv_HAVE_READDIR64=no \
	samba_cv_HAVE_FTRUNCATE_EXTEND=yes \
	samba_cv_REPLACE_READDIR=no \
	samba_cv_HAVE_TIMEGM=no \
	samba_cv_HAVE_BROKEN_LINUX_SENDFILE=no \
	samba_cv_HAVE_SENDFILE=yes \
	samba_cv_HAVE_WRFILE_KEYTAB=yes \
	samba_cv_HAVE_KERNEL_OPLOCKS_LINUX=yes \
	samba_cv_HAVE_IFACE_IFCONF=yes \
	samba_cv_HAVE_SETRESGID=yes \
	samba_cv_HAVE_SETRESUID=yes \
	samba_cv_USE_SETRESGID=yes \
	samba_cv_USE_SETRESUID=yes \
	samba_cv_HAVE_GETTIMEOFDAY_TZ=yes \
	samba_cv_REALPATH_TAKES_NULL=yes \
	CPPFLAGS="-DNDEBUG -DSHMEM_SIZE=524288 -Dfcntl=fcntl64 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE=1 -D_LARGEFILE64_SOURCE=1 -D_LARGE_FILES=1" \
	CFLAGS="$(SMBCFLAGS)" CC=$(CC) LD=$(LD) AR=$(AR) RANLIB=$(RANLIB) \
	$(CONFIGURE) --host=mipsel-linux \
		--prefix=/usr \
		--bindir=/usr/bin \
		--sbindir=/usr/sbin \
		--libdir=/etc \
		--localstatedir=/var \
		--with-configdir=/etc \
		--with-rootsbindir=/usr/sbin \
		--with-syslog \
		--enable-shared \
		--disable-static \
		--disable-cups \
		--disable-iprint \
		--disable-pie \
		--disable-fam \
		--disable-xmltest \
		--disable-dmalloc \
		--disable-krb5developer \
		--disable-developer \
		--disable-debug \
		--without-ldap \
		--without-cifsmount \
		--without-sendfile-support \
		--without-quotas \
		--without-sys-quotas
	touch .conf

clean:
	$(MAKE) -C $(srcdir) distclean
	@rm -f .conf

distclean: clean
	@find $(srcdir) -name config.h | xargs rm -f
	@find $(srcdir) -name Makefile | xargs rm -f
	@find $(srcdir) -name config.status | xargs rm -f
	@find $(srcdir) -name config.cache | xargs rm -f

install: all
	@install -d $(INSTALLDIR)/usr/bin/
	@install -d $(INSTALLDIR)/usr/sbin/
	@install -d $(INSTALLDIR)/usr/lib/
	@install -D $(srcdir)/bin/smbd $(INSTALLDIR)/usr/sbin/smbd
	@install -D $(srcdir)/bin/nmbd $(INSTALLDIR)/usr/sbin/nmbd
	@install -D $(srcdir)/bin/smbpasswd $(INSTALLDIR)/usr/bin/smbpasswd
	@install -D $(srcdir)/bin/libbigballofmud.so $(INSTALLDIR)/usr/lib/libbigballofmud.so

	$(STRIP) -s $(INSTALLDIR)/usr/sbin/smbd
	$(STRIP) -s $(INSTALLDIR)/usr/sbin/nmbd
	$(STRIP) -s $(INSTALLDIR)/usr/bin/smbpasswd
	# do not strip shared library, it will be optimized by libfoo.pl
	# $(STRIP) -s $(INSTALLDIR)/usr/lib/libbigballofmud.so
