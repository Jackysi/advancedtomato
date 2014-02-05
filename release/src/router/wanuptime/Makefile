include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(SRCBASE) -I$(SRCBASE)/include -I. -I$(TOP)/shared

LIBS = -L$(TOP)/nvram${BCMEX} -lnvram -L$(TOP)/shared -lshared

ifeq ($(TCONFIG_BCMARM),y)
LIBS += -lgcc_s
endif

EXE = wanuptime
OBJS = wanuptime.o

default: $(EXE)

$(EXE): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(EXE) $(OBJS)
