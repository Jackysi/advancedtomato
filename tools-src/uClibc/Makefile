# Makefile for uClibc
#
# Copyright (C) 2000 by Lineo, inc.
# Copyright (C) 2000-2002 Erik Andersen <andersen@uclibc.org>
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Library General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option) any
# later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU Library General Public License for more
# details.
#
# You should have received a copy of the GNU Library General Public License
# along with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# Makefile for uClibc
#
# Derived in part from the Linux-8086 C library, the GNU C Library, and several
# other sundry sources.  Files within this library are copyright by their
# respective copyright holders.


#--------------------------------------------------------------
# You shouldn't need to mess with anything beyond this point...
#--------------------------------------------------------------
noconfig_targets := menuconfig config oldconfig randconfig \
	defconfig allyesconfig allnoconfig clean distclean \
	release tags TAGS
TOPDIR=./
include Rules.mak

DIRS = extra ldso libc libcrypt libresolv libnsl libutil libm libpthread

ifeq ($(strip $(HAVE_DOT_CONFIG)),y)

all: headers subdirs shared utils finished

# In this section, we need .config
-include .config.cmd

.PHONY: $(SHARED_TARGET)
shared: $(SHARED_TARGET)
ifeq ($(strip $(HAVE_SHARED)),y)
	@$(MAKE) -C libc shared
	@$(MAKE) -C ldso shared
	@$(MAKE) -C libcrypt shared
	@$(MAKE) -C libresolv shared
	@$(MAKE) -C libnsl shared
	@$(MAKE) -C libutil shared
	@$(MAKE) -C libm shared
	@$(MAKE) -C libpthread shared
else
ifeq ($(SHARED_TARGET),)
	@echo
	@echo Not building shared libraries...
	@echo
endif
endif

ifneq ($(SHARED_TARGET),)

lib/main.o: $(ROOTDIR)/lib/libc/main.c
	$(CC) $(CFLAGS) $(ARCH_CFLAGS) -c -o $@ $(ROOTDIR)/lib/libc/main.c

bogus $(SHARED_TARGET): lib/libc.a lib/main.o Makefile
	make -C $(ROOTDIR) relink
	$(CC) -nostartfiles -o $(SHARED_TARGET) $(ARCH_CFLAGS) -Wl,-elf2flt -nostdlib		\
		-Wl,-shared-lib-id,${LIBID}				\
		lib/main.o \
		-Wl,--whole-archive,lib/libc.a,-lgcc,--no-whole-archive
	$(OBJCOPY) -L _GLOBAL_OFFSET_TABLE_ -L main -L __main -L _start \
		-L __uClibc_main -L __uClibc_start_main -L lib_main \
		-L _exit_dummy_ref		\
		-L __do_global_dtors -L __do_global_ctors		\
		-L __CTOR_LIST__ -L __DTOR_LIST__			\
		-L _current_shared_library_a5_offset_			\
		$(SHARED_TARGET).gdb
	ln -sf $(SHARED_TARGET).gdb .
endif

finished: shared
	@echo
	@echo Finally finished compiling...
	@echo

#
# Target for uClinux distro
#
.PHONY: romfs
romfs:
	@if [ "$(CONFIG_BINFMT_SHARED_FLAT)" = "y" ]; then \
		[ -e $(ROMFSDIR)/lib ] || mkdir -p $(ROMFSDIR)/lib; \
		$(ROMFSINST) $(SHARED_TARGET) /lib/lib$(LIBID).so; \
	fi
ifeq ($(strip $(HAVE_SHARED)),y)
	install -d $(ROMFSDIR)/lib
	install -m 644 lib/lib*-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
		$(ROMFSDIR)/lib
	cp -fa lib/*.so.* $(ROMFSDIR)/lib/.
	@if [ -x lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so ] ; then \
	    set -x -e; \
	    install -m 755 lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
	    		$(ROMFSDIR)/lib; \
		$(ROMFSINST) -s \
			/lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
			/lib/ld-linux.so.2; \
	fi;
endif

include/bits/uClibc_config.h: .config
	@if [ ! -x ./extra/config/conf ] ; then \
	    make -C extra/config conf; \
	fi;
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf -o extra/Configs/Config.$(TARGET_ARCH)

headers: include/bits/uClibc_config.h
	rm -f include/asm;
	@if [ "$(TARGET_ARCH)" = "powerpc" ];then \
	    ln -fs $(KERNEL_SOURCE)/include/asm-ppc include/asm; \
	elif [ "$(TARGET_ARCH)" = "mips" ];then \
	    ln -fs $(KERNEL_SOURCE)/include/asm-mips include/asm; \
	elif [ "$(TARGET_ARCH)" = "mipsel" ];then \
	    ln -fs $(KERNEL_SOURCE)/include/asm-mips include/asm; \
	    cd $(shell pwd)/libc/sysdeps/linux; \
	    ln -fns mips mipsel; \
	    cd $(shell pwd)/ldso/ldso; \
	    ln -fns mips mipsel; \
	    cd $(shell pwd)/libpthread/linuxthreads/sysdeps; \
	    ln -fns mips mipsel; \
	elif [ "$(TARGET_ARCH)" = "cris" ];then \
		ln -fs $(KERNEL_SOURCE)/include/asm-cris include/asm; \
	else \
	    if [ "$(UCLIBC_HAS_MMU)" != "y" ]; then \
	    	if [ -d $(KERNEL_SOURCE)/include/asm-$(TARGET_ARCH)nommu ] ; then \
		    ln -fs $(KERNEL_SOURCE)/include/asm-$(TARGET_ARCH)nommu include/asm;\
		else \
		    ln -fs $(KERNEL_SOURCE)/include/asm-$(TARGET_ARCH) include/asm; \
		fi; \
	    else \
		ln -fs $(KERNEL_SOURCE)/include/asm-$(TARGET_ARCH) include/asm; \
	    fi; \
	fi;
	@if [ ! -f include/asm/unistd.h ] ; then \
	    set -e; \
	    echo " "; \
	    echo "The path '$(KERNEL_SOURCE)/include/asm' doesn't exist."; \
	    echo "I bet you didn't set KERNEL_SOURCE, TARGET_ARCH or UCLIBC_HAS_MMU"; \
	    echo "correctly when you configured uClibc.  Please fix these settings."; \
	    echo " "; \
	    false; \
	fi;
	rm -f include/linux include/scsi
	ln -fs $(KERNEL_SOURCE)/include/linux include/linux
	ln -fs $(KERNEL_SOURCE)/include/scsi include/scsi
	@cd include/bits; \
	set -e; \
	for i in `ls ../../libc/sysdeps/linux/common/bits/*.h` ; do \
		ln -fs $$i .; \
	done; \
	if [ -d ../../libc/sysdeps/linux/$(TARGET_ARCH)/bits ] ; then \
		for i in `ls ../../libc/sysdeps/linux/$(TARGET_ARCH)/bits/*.h` ; do \
			ln -fs $$i .; \
		done; \
	fi
	@cd include/sys; \
	set -e; \
	for i in `ls ../../libc/sysdeps/linux/common/sys/*.h` ; do \
		ln -fs $$i .; \
	done; \
	if [ -d ../../libc/sysdeps/linux/$(TARGET_ARCH)/sys ] ; then \
		for i in `ls ../../libc/sysdeps/linux/$(TARGET_ARCH)/sys/*.h` ; do \
			ln -fs $$i .; \
		done; \
	fi
	@cd $(TOPDIR); \
	set -x -e; \
	rm -f include/bits/sysnum.h; \
	TOPDIR=. CC="$(CC)" /bin/sh extra/scripts/gen_bits_syscall_h.sh > include/bits/sysnum.h
	$(MAKE) -C libc/sysdeps/linux/$(TARGET_ARCH) headers

subdirs: $(patsubst %, _dir_%, $(DIRS))

$(patsubst %, _dir_%, $(DIRS)) : dummy
	$(MAKE) -C $(patsubst _dir_%, %, $@)

tags:
	ctags -R

install: install_dev install_runtime install_toolchain install_utils finished2


# Installs header files and development library links.
install_dev:
	install -d $(PREFIX)$(DEVEL_PREFIX)/lib
	install -d $(PREFIX)$(DEVEL_PREFIX)/usr/lib
	install -d $(PREFIX)$(DEVEL_PREFIX)/include
	-install -m 644 lib/*.[ao] $(PREFIX)$(DEVEL_PREFIX)/lib/
	tar -chf - include | tar -xf - -C $(PREFIX)$(DEVEL_PREFIX);
	-@for i in `find  $(PREFIX)$(DEVEL_PREFIX) -type d` ; do \
	    chmod -f 755 $$i; chmod -f 644 $$i/*.h; \
	done;
	-find $(PREFIX)$(DEVEL_PREFIX) -name CVS | xargs rm -rf;
	-chown -R `id | sed 's/^uid=\([0-9]*\).*gid=\([0-9]*\).*$$/\1.\2/'` $(PREFIX)$(DEVEL_PREFIX)
ifeq ($(strip $(HAVE_SHARED)),y)
	-install -m 644 lib/*.so $(PREFIX)$(DEVEL_PREFIX)/lib/
	-find lib/ -type l -name '*.so' -exec cp -a {} $(PREFIX)$(DEVEL_PREFIX)/lib ';'
	# If we build shared libraries then the static libs are PIC...
	# Make _pic.a symlinks to make mklibs.py and similar tools happy.
	for i in `find lib/  -type f -name '*.a' | sed -e 's/lib\///'` ; do \
		ln -sf $$i $(PREFIX)$(DEVEL_PREFIX)/lib/`echo $$i | sed -e 's/\.a$$/_pic.a/'`; \
	done
endif
	# Install precursors for later optimization
	install -d $(PREFIX)/$(DEVEL_PREFIX)/lib/optinfo
	install -m 644 libc/libc.a libc/misc/internals/interp.o $(PREFIX)$(DEVEL_PREFIX)/lib/optinfo/

# Installs run-time libraries and helper apps onto the host system
# allowing cross development.  If you want to deploy to a target 
# system, use the "install_target" target instead... 
install_runtime:
ifeq ($(strip $(HAVE_SHARED)),y)
	install -d $(PREFIX)$(DEVEL_PREFIX)/lib
	install -d $(PREFIX)$(DEVEL_PREFIX)/bin
	install -m 644 lib/lib*-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
		$(PREFIX)$(DEVEL_PREFIX)/lib
	cp -a lib/*.so.* $(PREFIX)$(DEVEL_PREFIX)/lib
	@if [ -x lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so ] ; then \
	    set -x -e; \
	    install -m 755 lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
	    		$(PREFIX)$(DEVEL_PREFIX)/lib; \
	fi;
	#@if [ -x lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so ] ; then \
	#    install -d $(PREFIX)$(SHARED_LIB_LOADER_PATH); \
	#    ln -sf $(PREFIX)$(DEVEL_PREFIX)/lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
	#		$(PREFIX)$(SHARED_LIB_LOADER_PATH)/$(UCLIBC_LDSO); \
	#fi;
endif

install_toolchain:
	install -d $(PREFIX)$(DEVEL_PREFIX)/lib
	install -d $(PREFIX)$(DEVEL_PREFIX)/bin
	install -d $(PREFIX)$(DEVEL_TOOL_PREFIX)/bin
	install -d $(PREFIX)$(SYSTEM_DEVEL_PREFIX)/bin
	$(MAKE) -C extra/gcc-uClibc install

ifeq ($(strip $(HAVE_SHARED)),y)
utils: $(TOPDIR)ldso/util/ldd
	$(MAKE) -C ldso utils
else
utils: dummy
endif

install_utils: utils
ifeq ($(strip $(HAVE_SHARED)),y)
	install -d $(PREFIX)$(DEVEL_TOOL_PREFIX)/bin;
	install -m 755 ldso/util/ldd \
		$(PREFIX)$(SYSTEM_DEVEL_PREFIX)/bin/$(TARGET_ARCH)-uclibc-ldd
	ln -fs $(SYSTEM_DEVEL_PREFIX)/bin/$(TARGET_ARCH)-uclibc-ldd \
		$(PREFIX)$(DEVEL_TOOL_PREFIX)/bin/ldd
	# For now, don't bother with readelf since surely the host
	# system has binutils, or we couldn't have gotten this far...
	#install -m 755 ldso/util/readelf \
	#	$(PREFIX)$(SYSTEM_DEVEL_PREFIX)/bin/$(TARGET_ARCH)-uclibc-readelf
	#ln -fs $(SYSTEM_DEVEL_PREFIX)/bin/$(TARGET_ARCH)-uclibc-readelf \
	#	$(PREFIX)$(DEVEL_TOOL_PREFIX)/bin/readelf
	@if [ -x ldso/util/ldconfig ] ; then \
	    set -x -e; \
	    install -d $(PREFIX)$(DEVEL_PREFIX)/etc; \
	    install -m 755 ldso/util/ldconfig \
		    $(PREFIX)$(SYSTEM_DEVEL_PREFIX)/bin/$(TARGET_ARCH)-uclibc-ldconfig; \
	    ln -fs $(SYSTEM_DEVEL_PREFIX)/bin/$(TARGET_ARCH)-uclibc-ldconfig \
		    $(PREFIX)$(DEVEL_TOOL_PREFIX)/bin/ldconfig; \
	fi;
endif

# Installs run-time libraries and helper apps in preparation for
# deploying onto a target system, but installed below wherever
# $PREFIX is set to, allowing you to package up the result for
# deployment onto your target system.
install_target:
ifeq ($(strip $(HAVE_SHARED)),y)
	install -d $(PREFIX)$(TARGET_PREFIX)/lib
	install -d $(PREFIX)$(TARGET_PREFIX)/usr/bin
	install -m 644 lib/lib*-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
		$(PREFIX)$(TARGET_PREFIX)/lib
	cp -a lib/*.so.* $(PREFIX)$(TARGET_PREFIX)/lib
	@if [ -x lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so ] ; then \
	    set -x -e; \
	    install -m 755 lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
	    		$(PREFIX)$(TARGET_PREFIX)/lib; \
	fi;
	#@if [ -x lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so ] ; then \
	#    install -d $(PREFIX)$(SHARED_LIB_LOADER_PATH); \
	#    ln -sf $(PREFIX)$(TARGET_PREFIX)/lib/ld-uClibc-$(MAJOR_VERSION).$(MINOR_VERSION).$(SUBLEVEL).so \
	#    		$(PREFIX)$(SHARED_LIB_LOADER_PATH)/$(UCLIBC_LDSO); \
	#fi;
endif

install_target_utils:
ifeq ($(strip $(HAVE_SHARED)),y)
	@$(MAKE) -C ldso/util ldd.target readelf.target #ldconfig.target
	install -d $(PREFIX)$(TARGET_PREFIX)/usr/bin;
	install -m 755 ldso/util/ldd.target $(PREFIX)$(TARGET_PREFIX)/usr/bin/ldd
	install -m 755 ldso/util/readelf.target $(PREFIX)$(TARGET_PREFIX)/usr/bin/readelf
	@if [ -x ldso/util/ldconfig.target ] ; then \
	    set -x -e; \
	    install -d $(PREFIX)$(TARGET_PREFIX)/etc; \
	    install -d $(PREFIX)$(TARGET_PREFIX)/sbin; \
	    install -m 755 ldso/util/ldconfig.target $(PREFIX)$(TARGET_PREFIX)/sbin/ldconfig; \
	fi;
endif
ifeq ($(strip $(UCLIBC_HAS_LOCALE)),y)
	@$(MAKE) -C libc/misc/wchar iconv.target
	install -d $(PREFIX)$(TARGET_PREFIX)/usr/bin;
	install -m 755 libc/misc/wchar/iconv.target $(PREFIX)$(TARGET_PREFIX)/usr/bin/iconv
endif

finished2:
	@echo
	@echo Finished installing...
	@echo

else # ifeq ($(strip $(HAVE_DOT_CONFIG)),y)

all: menuconfig

# configuration
# ---------------------------------------------------------------------------

extra/config/conf:
	make -C extra/config conf
	-@if [ ! -f .config ] ; then \
		cp extra/Configs/Config.$(TARGET_ARCH).default .config; \
	fi

extra/config/mconf:
	make -C extra/config ncurses conf mconf
	-@if [ ! -f .config ] ; then \
		cp extra/Configs/Config.$(TARGET_ARCH).default .config; \
	fi

menuconfig: extra/config/mconf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/mconf extra/Configs/Config.$(TARGET_ARCH)

config: extra/config/conf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf extra/Configs/Config.$(TARGET_ARCH)

oldconfig: extra/config/conf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf -o extra/Configs/Config.$(TARGET_ARCH)

randconfig: extra/config/conf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf -r extra/Configs/Config.$(TARGET_ARCH)

allyesconfig: extra/config/conf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf -y extra/Configs/Config.$(TARGET_ARCH)

allnoconfig: extra/config/conf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf -n extra/Configs/Config.$(TARGET_ARCH)

defconfig: extra/config/conf
	rm -rf include/bits
	mkdir -p include/bits
	@./extra/config/conf -d extra/Configs/Config.$(TARGET_ARCH)


clean:
	- find . \( -name \*.o -o -name \*.a -o -name \*.so -o -name core -o -name .\#\* \) -exec rm -f {} \;
	@rm -rf tmp lib include/bits libc/tmp _install
	$(MAKE) -C test clean
	$(MAKE) -C ldso clean
	$(MAKE) -C libc/misc/internals clean
	$(MAKE) -C libc/misc/wchar clean
	$(MAKE) -C libc/unistd clean
	$(MAKE) -C libc/sysdeps/linux/common clean
	$(MAKE) -C extra/gcc-uClibc clean
	$(MAKE) -C extra/locale clean
	@set -e; \
	for i in `(cd $(TOPDIR)/libc/sysdeps/linux/common/sys; ls *.h)` ; do \
		rm -f include/sys/$$i; \
	done; \
	if [ -d libc/sysdeps/linux/$(TARGET_ARCH)/sys ] ; then \
		for i in `(cd libc/sysdeps/linux/$(TARGET_ARCH)/sys; ls *.h)` ; do \
			rm -f include/sys/$$i; \
		done; \
	fi;
	@rm -f include/linux include/scsi include/asm
	@if [ -d libc/sysdeps/linux/$(TARGET_ARCH) ]; then		\
	    $(MAKE) -C libc/sysdeps/linux/$(TARGET_ARCH) clean;		\
	fi;
	@if [ "$(TARGET_ARCH)" = "mipsel" ]; then \
	    $(MAKE) -C libc/sysdeps/linux/mips clean; \
	    rm -f ldso/ldso/mipsel; \
	    rm -f libc/sysdeps/linux/mipsel; \
	    rm -f libpthread/linuxthreads/sysdeps/mipsel; \
	fi;

distclean: clean
	rm -f .config .config.old .config.cmd
	$(MAKE) -C extra clean

release: distclean
	cd ..;					\
	rm -rf uClibc-$(VERSION);		\
	cp -a uClibc uClibc-$(VERSION);		\
	find uClibc-$(VERSION)/ -type f		\
	    -name .\#* -exec rm -rf {} \; ;	\
	find uClibc-$(VERSION)/ -type d		\
	    -name CVS  -exec rm -rf {} \; ;	\
						\
	tar -cvzf uClibc-$(VERSION).tar.gz uClibc-$(VERSION)/;

endif # ifeq ($(strip $(HAVE_DOT_CONFIG)),y)

.PHONY: dummy subdirs release distclean clean config oldconfig menuconfig


