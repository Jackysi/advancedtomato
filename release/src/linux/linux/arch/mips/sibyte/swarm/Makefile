USE_STANDARD_AS_RULE := true

all: sbswarm.a

OBJS-y                   = setup.o cmdline.o rtc_xicor1241.o

OBJS-$(CONFIG_L3DEMO)   += procl3switch.o l3procbootstrap.o l3proc.o
OBJS-$(CONFIG_REMOTE_DEBUG) += dbg_io.o

#XMITTER=1

ifdef XMITTER
l3proc.bin: xmitter
	ln xmitter l3proc.bin
else
l3proc.bin: l3proc
	ln l3proc l3proc.bin
endif

l3proc.o: l3proc.bin
	mips-linux-ld -Tl3proc.lds -bbinary -o l3proc.o l3proc.bin

sbswarm.a: $(OBJS-y)
	$(AR) rcs sbswarm.a $^
	rm -f l3proc.o l3proc.bin

include $(TOPDIR)/Rules.make

