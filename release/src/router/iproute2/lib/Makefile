
UTILOBJ=utils.o rt_names.o ll_types.o ll_proto.o ll_addr.o inet_proto.o

NLOBJ=ll_map.o libnetlink.o

all: libnetlink.a libutil.a

libnetlink.a: $(NLOBJ)
	ar rcs $@ $(NLOBJ)

libutil.a: $(UTILOBJ) $(ADDLIB)
	ar rcs $@ $(UTILOBJ) $(ADDLIB)

clean:
	rm -f $(NLOBJ) $(UTILOBJ) $(ADDLIB) libnetlink.a libutil.a

