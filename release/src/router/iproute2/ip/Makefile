IPOBJ=ip.o ipaddress.o iproute.o iprule.o \
    rtm_map.o iptunnel.o ipneigh.o iplink.o ipmaddr.o \
    ipmonitor.o ipmroute.o

RTMONOBJ=rtmon.o

RTACCTOBJ=rtacct.o

ALLOBJ=$(IPOBJ) $(ADDOBJ) $(RTOBJ)

all: ip rtmon rtacct

ip: $(IPOBJ) $(LIBNETLINK) $(LIBUTIL)

rtmon: $(RTMONOBJ) $(LIBNETLINK)

rtacct: $(RTACCTOBJ)

clean:
	rm -f $(IPOBJ) $(RTMONOBJ) $(RTACCTOBJ) ip rtmon rtacct
