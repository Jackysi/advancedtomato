topdir:=../..
include $(topdir)/autoconf/targets.mak

common_srcs    := upsfetch.c cgilib.c
multimon_srcs  := multimon.c
upsstats_srcs  := upsstats.c
upsfstats_srcs := upsfstats.c
upsimage_srcs  := upsimage.c

common_obj     := $(call SRC2OBJ,$(common_srcs))
multimon_obj   := $(call SRC2OBJ,$(multimon_srcs))
upsstats_obj   := $(call SRC2OBJ,$(upsstats_srcs))
upsfstats_obj  := $(call SRC2OBJ,$(upsfstats_srcs))
upsimage_obj   := $(call SRC2OBJ,$(upsimage_srcs))

SRCS = $(common_srcs) $(multimon_srcs) $(upsstats_srcs) \
       $(upsfstats_srcs) $(upsimage_srcs)

all-targets: multimon.cgi upsstats.cgi upsfstats.cgi upsimage.cgi

multimon.cgi: $(common_obj) $(multimon_obj) $(APCLIBS)
	$(LINK)

upsstats.cgi: $(common_obj) $(upsstats_obj) $(APCLIBS)
	$(LINK)

upsfstats.cgi: $(common_obj) $(upsfstats_obj) $(APCLIBS)
	$(LINK)

upsimage.cgi: $(common_obj) $(upsimage_obj) $(APCLIBS)
	$(LINK) $(X_LIBS) $(LIBGD)

all-install: install-cgi
all-uninstall: uninstall-cgi

install-cgi:
	$(call MKDIR,$(cgibin))
	$(call INSTPROG,755,multimon.cgi,$(cgibin))
	$(call INSTPROG,755,upsstats.cgi,$(cgibin))
	$(call INSTPROG,755,upsfstats.cgi,$(cgibin))
	$(call INSTPROG,755,upsimage.cgi,$(cgibin))
	$(call MKDIR,$(sysconfdir))
	$(call INSTNEW,644,apcupsd.css,$(sysconfdir))
	$(call INSTNEW,644,$(topdir)/platforms/etc/hosts.conf,$(sysconfdir))
	$(call INSTNEW,644,$(topdir)/platforms/etc/multimon.conf,$(sysconfdir))

uninstall-cgi:
	$(call UNINST,$(cgibin)/multimon.cgi)
	$(call UNINST,$(cgibin)/upsstats.cgi)
	$(call UNINST,$(cgibin)/upsfstats.cgi)
	$(call UNINST,$(cgibin)/upsimage.cgi)
	$(call UNINST,$(sysconfdir)/apcupsd.css)
	$(call UNINST,$(sysconfdir)/apcupsd.css.new)
	$(call UNINST,$(sysconfdir)/hosts.conf)
	$(call UNINST,$(sysconfdir)/hosts.conf.new)
	$(call UNINST,$(sysconfdir)/multimon.conf)
	$(call UNINST,$(sysconfdir)/multimon.conf.new)

# Include dependencies
-include $(DEPS)
