topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = $(wildcard *.c)

# This bit of hackery is because gapcmon has its own CFLAGS and LIBS.
# Plus it needs to be built with a C compiler while normally all apcupsd *.c
# files are built with a C++ compiler. At some point we'll rename all *.c
# files in the rest of the tree to *.cpp and some of this can then go away.
CXX = $(CC)
CPPFLAGS += $(GAPCMON_CFLAGS) -DICON_DIR=\"$(datadir)\"
CXXFLAGS = $(CFLAGS)
LIBS := $(GAPCMON_LIBS) $(X_LIBS) -lX11 -lXext $(LIBS)

all-targets: gapcmon

gapcmon: $(OBJS)
	$(LINK)

all-install: install-gapcmon
all-uninstall: uninstall-gapcmon

install-gapcmon:
	$(call MKDIR,$(bindir))
	$(call MKDIR,$(datadir)/pixmaps)
	$(call MKDIR,$(datadir)/applications)
	$(call INSTPROG,755,gapcmon,$(bindir)/gapcmon)
	$(call INSTDATA,755,gapcmon.desktop,$(datadir)/applications/gapcmon.desktop)
	$(call INSTDATA,744,apcupsd.png,$(datadir)/pixmaps/apcupsd.png)
	$(call INSTDATA,744,online.png,$(datadir)/pixmaps/online.png)
	$(call INSTDATA,744,onbatt.png,$(datadir)/pixmaps/onbatt.png)
	$(call INSTDATA,744,charging.png,$(datadir)/pixmaps/charging.png)
	$(call INSTDATA,744,unplugged.png,$(datadir)/pixmaps/unplugged.png)
	$(call INSTDATA,744,gapc_prefs.png,$(datadir)/pixmaps/gapc_prefs.png)

uninstall-gapcmon:
	$(call UNINST,$(datadir)/pixmaps/apcupsd.png)
	$(call UNINST,$(datadir)/pixmaps/unplugged.png)
	$(call UNINST,$(datadir)/pixmaps/online.png)
	$(call UNINST,$(datadir)/pixmaps/onbatt.png)
	$(call UNINST,$(datadir)/pixmaps/charging.png)
	$(call UNINST,$(datadir)/applications/gapc_prefs.png)
	$(call UNINST,$(datadir)/applications/gapcmon.desktop)
	$(call UNINST,$(bindir)/gapcmon)

# Include dependencies
-include $(DEPS)
