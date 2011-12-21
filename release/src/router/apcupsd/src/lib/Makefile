topdir:=../..
include $(topdir)/autoconf/targets.mak

SRCS = apcconfig.c apcerror.c apcevents.c apcexec.c \
       apcfile.c apclibnis.c apclock.c apclog.c apcsignal.c \
       apcstatus.c asys.c newups.c md5.c statmgr.cpp gethostname.c \
       amutex.cpp astring.cpp autil.cpp atimer.cpp athread.cpp \
       $(LIBEXTRAOBJ:.o=.c) libsupc++fix.cpp

all-targets: libapc.a

libapc.a: $(OBJS)
	$(MAKELIB)

# Include dependencies
-include $(DEPS)
