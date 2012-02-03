topdir:=../..

SUBDIRS =
include $(topdir)/autoconf/targets.mak

all-targets: manual.html manual.pdf

# Force pdf to regenerate whenever HTML changes
# HTML rule has all the dependency stuff
manual.pdf: manual.html

# Include dependencies
-include $(DEPDIR)/manual.P
