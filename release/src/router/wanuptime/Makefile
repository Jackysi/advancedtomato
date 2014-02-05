include ../common.mak

CFLAGS = -Os -Wall $(EXTRACFLAGS)
CFLAGS += -I$(SRCBASE) -I$(SRCBASE)/include -I. -I$(TOP)/shared

LIBS = -L$(TOP)/nvram -lnvram -L$(TOP)/shared -lshared

EXE = wanuptime
OBJS = wanuptime.o

default: $(EXE)

$(EXE): $(OBJS)
	@$(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c
	@$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(EXE) $(OBJS)
