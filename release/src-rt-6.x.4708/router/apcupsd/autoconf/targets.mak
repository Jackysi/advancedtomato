# Pull in autoconf variables
include $(topdir)/autoconf/variables.mak

# Now that we have autoconf vars, overwrite $(topdir) with absolute path 
# version instead of relative version we inherited. The easy way to do this 
# would be to use $(abspath $(topdir)), but abspath is a gmake-3.81 feature.
# So we let autoconf figure it out for us.
topdir := $(abstopdir)

# Older (pre-3.79) gmake does not have $(CURDIR)
ifeq ($(CURDIR),)
  CURDIR := $(shell pwd)
endif

# By default we do pretty-printing only
V := @
VV := @
NPD := --no-print-directory

# Check verbose flag
ifeq ($(strip $(VERBOSE)),1)
  V :=
  NPD :=
endif
ifeq ($(strip $(VERBOSE)),2)
  V :=
  VV :=
  NPD :=
endif

# Relative path to this dir from $(topdir)
RELDIR := $(patsubst /%,%,$(subst $(topdir),,$(CURDIR)))
ifneq ($(strip $(RELDIR)),)
  RELDIR := $(RELDIR)/
endif

# Strip extensions
STRIPEXT = $(foreach file,$(1),$(basename $(file)))

# Convert a list of sources to a list of objects in OBJDIR
SRC2OBJ = $(foreach obj,$(call STRIPEXT,$(1)),$(dir $(obj))$(OBJDIR)/$(notdir $(obj)).o)

# All objects, derived from all sources
OBJS = $(call SRC2OBJ,$(SRCS))

# Dependency files, derived from all sources
DEPS = $(foreach dep,$(call STRIPEXT,$(SRCS)),$(DEPDIR)/$(dep).P)

# Default target: Build all subdirs, then reinvoke make to build local 
# targets. This is a little gross, but necessary to force make to build 
# subdirs first when running in parallel via 'make -jN'. Hopefully I will 
# discover a cleaner way to solve this someday.
.PHONY: all
all: all-subdirs
	$(VV)+$(MAKE) $(NPD) all-targets

# 'all-targets' is supplied by lower level Makefile. It represents
# all targets to be built at that level. We list it here with a do-nothing
# action in order to suppress the "Nothing to do for all-targets" message
# when all targets are up to date.
.PHONY: all-targets
all-targets:
	@#

# standard install target: Same logic as 'all'.
.PHONY: install
install: all-subdirs
	$(VV)+$(MAKE) $(NPD) all-targets
	$(VV)+$(MAKE) $(NPD) all-install

# 'all-install' is extended by lower-level Makefile to perform any
# required install actions.
.PHONY: all-install
all-install:
	@#

# no-op targets for use by lower-level Makefiles when a particular
# component is not being installed.
.PHONY: install- uninstall-
install-:
uninstall-:

# standard uninstall target: Depends on subdirs to force recursion,
# then reinvokes make to uninstall local targets. Same logic as 'install'.
.PHONY: uninstall
uninstall: all-subdirs
	$(VV)+$(MAKE) $(NPD) all-uninstall

# 'all-uninstall' is extended by lower-level Makefiles to perform 
# any required uninstall actions.
.PHONY: all-uninstall
all-uninstall:
	@#

# Typical clean target: Remove all objects and dependency files.
.PHONY: clean
clean:
	$(V)find . -depth \
	  \( -name $(OBJDIR) -o -name $(DEPDIR) -o -name \*.a \) \
          -exec $(ECHO) "  CLEAN" \{\} \; -exec $(RMF) \{\} \;

# Template rule to build a subdirectory
.PHONY: %_DIR
%_DIR:
	@$(ECHO) "       " $(RELDIR)$*
	$(VV)+$(MAKE) -C $* $(NPD) $(MAKECMDGOALS)

# Collective all-subdirs target depends on subdir rule
.PHONY: all-subdirs
all-subdirs: $(foreach subdir,$(SUBDIRS),$(subdir)_DIR)

# Echo with no newline
# Pipline here is silly, but should be more portable 
# than 'echo -n' or 'echo ...\c'. Cannot use autoconf
# to figure this out since 'make install' may be run
# with root's shell when ./configure was run with user's 
# shell. Could also use 'printf' but not certain how
# universal that is.
define ECHO_N
	$(ECHO) $(1) | tr -d '\n'
endef

# How to build dependencies
MAKEDEPEND = $(CC) -M $(CPPFLAGS) $< > $(df).d
ifeq ($(strip $(NODEPS)),)
  define DEPENDS
	if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi; \
	  $(MAKEDEPEND); \
	  $(call ECHO_N,$(OBJDIR)/) > $(df).P; \
	  $(SED) -e 's/#.*//' -e '/^$$/ d' < $(df).d >> $(df).P; \
	  $(SED) -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	         -e '/^$$/ d' -e 's/$$/ :/' < $(df).d >> $(df).P; \
	  $(RMF) $(df).d
  endef
else
  DEPENDS :=
endif

# Rule to build *.o from *.c and generate dependencies for it
$(OBJDIR)/%.o: %.c
	@$(ECHO) "  CXX  " $(RELDIR)$<
	$(VV)if test ! -d $(OBJDIR); then mkdir -p $(OBJDIR); fi
	$(V)$(CXX) $(CXXFLAGS) -c -o $@ $<
	$(VV)$(DEPENDS)

# Rule to build *.o from *.cpp and generate dependencies for it
$(OBJDIR)/%.o: %.cpp
	@$(ECHO) "  CXX  " $(RELDIR)$<
	$(VV)if test ! -d $(OBJDIR); then mkdir -p $(OBJDIR); fi
	$(V)$(CXX) $(CXXFLAGS) -c -o $@ $<
	$(VV)$(DEPENDS)

# Rule to build *.o from *.m and generate dependencies for it
$(OBJDIR)/%.o: %.m
	@$(ECHO) "  OBJC " $(RELDIR)$<
	$(VV)if test ! -d $(OBJDIR); then mkdir -p $(OBJDIR); fi
	$(V)$(OBJC) $(OBJCFLAGS) -c -o $@ $<
	$(VV)$(DEPENDS)

# Rule to link an executable
define LINK
	@$(ECHO) "  LD   " $(RELDIR)$@
	$(V)$(LD) $(LDFLAGS) $^ -o $@ $(LIBS)
endef

# Rule to generate an archive (library)
MAKELIB=$(call ARCHIVE,$@,$(OBJS))
define ARCHIVE
	@$(ECHO) "  AR   " $(RELDIR)$(1)
	$(VV)$(RMF) $(1)
	$(V)$(AR) rc $(1) $(2)
	$(V)$(RANLIB) $(1)
endef

# How to generate a *.nib from a *.xib
%.nib: %.xib
	@$(ECHO) "  NIB  " $(RELDIR)$<
	$(VV)if test ! -d $(OBJDIR); then mkdir -p $(OBJDIR); fi
	$(V)$(NIB) $(NIBFLAGS) --compile $@ $<

# Rule to create a directory during install
define MKDIR
   $(if $(wildcard $(DESTDIR)$(1)),, \
      @$(ECHO) "  MKDIR" $(DESTDIR)$(1))
   $(if $(wildcard $(DESTDIR)$(1)),, \
     $(V)$(MKINSTALLDIRS) $(DESTDIR)$(1))
endef

# Install a program file, given mode, src, and dest
define INSTPROG
   @$(ECHO) "  COPY " $(2) =\> $(DESTDIR)$(3)
#   $(V)$(INSTALL_PROGRAM) $(STRIP) -m $(1) $(2) $(DESTDIR)$(3)
   $(V)$(INSTALL_PROGRAM) -m $(1) $(2) $(DESTDIR)$(3)
endef

# Install a data file, given mode, src, and dest
define INSTDATA
   @$(ECHO) "  COPY " $(2) =\> $(DESTDIR)$(3)
   $(V)$(INSTALL_DATA) -m $(1) $(2) $(DESTDIR)$(3)
endef

# Install a data file, given mode, src, and dest.
# Existing dest file is preserved; new file is named *.new if dest exists.
define INSTNEW
   @$(ECHO) "  COPY " $(notdir $(2)) =\> $(DESTDIR)$(3)/$(notdir $(2))$(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))),.new,)
   $(V)$(INSTALL_DATA) -m $(1) $(2) $(DESTDIR)$(3)/$(notdir $(2))$(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))),.new,)
endef

# Install a data file, given mode, src, and dest.
# Existing dest file is renamed to *.orig if it exists.
define INSTORIG
   $(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))), \
      @$(ECHO) "  MV   " $(DESTDIR)$(3)/$(notdir $(2)) =\> \
         $(DESTDIR)$(3)/$(notdir $(2)).orig,)
   $(if $(wildcard $(DESTDIR)$(3)/$(notdir $(2))), \
      $(V)$(MV) $(DESTDIR)$(3)/$(notdir $(2)) $(DESTDIR)$(3)/$(notdir $(2)).orig,)
   @$(ECHO) "  COPY " $(notdir $(2)) =\> $(DESTDIR)$(3)/$(notdir $(2))
   $(V)$(INSTALL_SCRIPT) -m $(1) $(2) $(DESTDIR)$(3)
endef

# Make a symlink
define SYMLINK
   @$(ECHO) "  LN   " $(DESTDIR)/$(2) -\> $(1)
   $(V)$(LN) -sf $(1) $(DESTDIR)/$(2)
endef

# Copy a file
define COPY
   @$(ECHO) "  CP   " $(1) =\> $(DESTDIR)/$(2)
   $(V)$(CP) -fR $(1) $(DESTDIR)/$(2)
endef

# Uninstall a file
define UNINST
   @$(ECHO) "  RM   " $(DESTDIR)$(1)
   $(V)$(RMF) $(DESTDIR)$(1)
endef

# Announce distro install
define DISTINST
   @$(ECHO) "  ------------------------------------------------------------"
   @$(ECHO) "  $(1) distribution installation"
   @$(ECHO) "  ------------------------------------------------------------"
endef

# Announce distro uninstall
define DISTUNINST
   @$(ECHO) "  ------------------------------------------------------------"
   @$(ECHO) "  $(1) distribution uninstall"
   @$(ECHO) "  ------------------------------------------------------------"
endef

# If DESTDIR is set, we do no chkconfig processing
ifeq ($(DESTDIR),)
define CHKCFG
    $(if $(wildcard $(2)),@$(ECHO) "  CKCFG" $(1):$(2))
    $(if $(wildcard $(2)),$(V)$(CHKCONFIG) --$(1) apcupsd)
endef
endif

# How to massage dependency list from rst2html
ifeq ($(strip $(NODEPS)),)
  define RSTDEPENDS
	  $(ECHO) $@: $< \\ > $(df).P;                             \
	  $(SED) -e '$$q' -e 's/^.*$$/& \\/' < $(df).d >> $(df).P; \
	  $(ECHO) $<: >> $(df).P;                                  \
	  $(SED) -e 's/^.*$$/&:/' < $(df).d >> $(df).P;            \
	  $(RMF) $(df).d
  endef
else
  RSTDEPENDS :=
endif

# Build *.html from *.rst and generate dependencies for it
%.html: %.rst
	@$(ECHO) "  HTML " $<
ifneq ($(strip $(RST2HTML)),)
	$(VV)if test ! -d $(DEPDIR); then mkdir -p $(DEPDIR); fi;
	$(V)$(RST2HTML) $(RST2HTMLOPTS) $< $@
	$(VV)$(RSTDEPENDS)
else
	@$(ECHO) "--> Not building HTML due to missing rst2html"
endif

# Build *.pdf from *.rst
%.pdf: %.rst
	@$(ECHO) "  PDF  " $<
ifneq ($(strip $(RST2PDF)),)
	$(V)$(RST2PDF) $(RST2PDFOPTS) -o $@ $<
else
	@$(ECHO) "--> Not building PDF due to missing rst2pdf"
endif

# Format a manpage into plain text
define MANIFY
	@$(ECHO) "  MAN  " $(1) -\> $(2)
	$(V)man ./$(1) | col -b > $(2)
endef
