topdir:=..

SUBDIRS = lib drivers $(CGIDIR) $(USBHIDDIR) $(GAPCMON) $(APCAGENT)
include $(topdir)/autoconf/targets.mak

# Force cgi/ and apcagent/ to build after lib/
# Necessary for parallel make since they link against libapc.a
cgi_DIR: lib_DIR
apcagent_DIR: lib_DIR

common_srcs     := options.c device.c reports.c action.c
apcupsd_srcs    := apcupsd.c apcnis.c
apcaccess_srcs  := apcaccess.c
apctest_srcs    := apctest.c
smtp_srcs       := smtp.c

common_obj      := $(call SRC2OBJ,$(common_srcs))
apcupsd_obj     := $(call SRC2OBJ,$(apcupsd_srcs))
apcaccess_obj   := $(call SRC2OBJ,$(apcaccess_srcs))
apctest_obj     := $(call SRC2OBJ,$(apctest_srcs))
smtp_obj        := $(call SRC2OBJ,$(smtp_srcs))

SRCS = $(common_srcs) $(apcupsd_srcs) $(apcaccess_srcs) \
       $(apctest_srcs) $(smtp_srcs) 

all-targets: apcupsd apcaccess smtp
#apctest

apcupsd: $(common_obj) $(apcupsd_obj) $(APCDRVLIBS) $(APCLIBS)
	$(LINK) $(DRVLIBS)

apctest: $(common_obj) $(apctest_obj) $(APCDRVLIBS) $(APCLIBS)
	$(LINK) $(DRVLIBS)

apcaccess: $(apcaccess_obj) $(APCLIBS)
	$(LINK)

smtp: $(smtp_obj) $(APCLIBS)
	$(LINK)

all-install: install-base
all-uninstall: uninstall-base

install-base:
	$(call MKDIR,$(sbindir))
	$(call INSTPROG,700,apcupsd,$(sbindir)/apcupsd)
#	$(call INSTPROG,700,apctest,$(sbindir)/apctest)
	$(call INSTPROG,755,apcaccess,$(sbindir)/apcaccess)
	$(call INSTPROG,755,smtp,$(sbindir)/smtp)

uninstall-base:
	$(call UNINST,$(sbindir)/apcupsd)
#	$(call UNINST,$(sbindir)/apctest)
	$(call UNINST,$(sbindir)/apcaccess)
	$(call UNINST,$(sbindir)/smtp)

# Include dependencies
-include $(DEPS)
