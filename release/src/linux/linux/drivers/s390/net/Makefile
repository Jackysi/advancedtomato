#
# S/390 network devices
#

O_TARGET := s390-net.o

list-multi := ctc.o
export-objs := iucv.o fsm.o

ctc-objs := ctcmain.o ctctty.o

obj-$(CONFIG_IUCV) += iucv.o fsm.o
obj-$(CONFIG_CTC) += ctc.o fsm.o
obj-$(CONFIG_IUCV) += netiucv.o

include $(TOPDIR)/Rules.make

ctc.o: $(ctc-objs)
	$(LD) -r -o $@ $(ctc-objs)

