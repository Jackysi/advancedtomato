# =============================================================
#
# linux/arch/mips/galileo/compressed/Makefile
#
# By RidgeRun Inc.
#
# Description:
#   Create a system containing a copy of the compressed vmlinux kernel.
#   The system knows how to decompress the contained kernel and then
#   jump to it resulting in a linux kernel boot.
#
#   The system comes in three forms:
#
#       1. - ramsys -
#          to be loaded into ram then run. When run
#          it decompresses the kernel housed in its internal
#          data structures and then jumps to the image which
#          results in a linux kernel boot.
#
#       2. - flashsys -
#          to be loaded into ram so that it can be
#          burned into the onboard flash. Then the board jumpers
#          can be switched so that the next power cycle caused
#          the system in flash to run which then proceeds as
#          described by #1 above.
#          Note: burner.srec is the utility that will allow
#          the user to get this image into flash.
#
#       3. - flashsys2 -
#          to be loaded into ram so that it can be
#          burned into the onboard flash. Then on each power
#          cycle when the standard PMON prompt is presented
#          the user can type `call 0xbf000000` to invoke
#          the system in flash which then proceeds as
#          described by #1 above.
#          Note: burner.srec is the utility that will allow
#          the user to get this image into flash.
#
#       4. - burner.srec -
#          related to #2 and #3 above.
#
# =============================================================

USE_STANDARD_AS_RULE := true

all: ramsys.srec \
     flashsys.srec \
     flashsys2.srec \
     burner.srec

SYSTEM = $(TOPDIR)/vmlinux

CFLAGS_2 = -DCONSOLE_SERIAL -DDELIMITERLINES -DGALILEO_PORT \
           -DANSIESC -DELF_IMAGE -DELF_IMAGE -DDOWNLOAD_PROTO_TFTP \
           -DEVB64120A -D__MIPSEB__ -DINCLUDE_EEPRO100 \
           -DINCLUDE_GETH0 -DNOPRINTK -DPROM -DCOMPRESSEDVMLINUX

sbdreset_evb64120A.o:
	$(CC) $(CFLAGS) $(CFLAGS_2) -c sbdreset_evb64120A.S -o $*.o
memory.o:
	$(CC) $(CFLAGS) $(CFLAGS_2) -c memory.c -o $*.o
pci.o:
	$(CC) $(CFLAGS) $(CFLAGS_2) -c pci.c -o $*.o
pci_etherboot.o:
	$(CC) $(CFLAGS) $(CFLAGS_2) -c pci_etherboot.c -o $*.o
load.o:
	$(CC) $(CFLAGS) $(CFLAGS_2) -c load.c -o $*.o
flashdrv.o:
	$(CC) $(CFLAGS) $(CFLAGS_2) -c flashdrv.c -o $*.o

gz2asm: gz2asm.c
	g++ -o gz2asm gz2asm.c

doit: doit.c
	gcc -o doit doit.c

piggy.gz: $(SYSTEM)
	rm -f piggy piggy.gz
	$(OBJCOPY) -S -O binary $(SYSTEM) piggy
	gzip -f -9 < piggy > piggy.gz

piggy.S: doit fixit piggy.gz
	./doit < piggy.gz > piggy.S; . ./fixit piggy.S

OBJECTS_ramsys    = head.o misc.o piggy.o ../serialGT.o
OBJECTS_flashsys  = sbdreset_evb64120A.o evb64120A_Setup.o pci_etherboot.o memory.o pci.o head.o misc.o piggy.o ../serialGT.o
OBJECTS_flashsys2 = xfer.o head.o misc.o piggy.o ../serialGT.o
OBJECTS_burner    = burner.o load.o flashdrv.o

ramsys.srec : $(OBJECTS_ramsys) ld.script.gal
	@# Note: this image is intended to run out of ram. No flash involved.
	$(LD) -T ld.script.gal -o ramsys $(OBJECTS_ramsys)
	$(NM) ramsys | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | sort > System_ramsys.map
	$(OBJCOPY) -O srec ramsys ramsys.srec
	cp ramsys.srec $(TOPDIR)/.

flashsys.srec : $(OBJECTS_flashsys) ld.sys.big.Flash
	@# Note1: Use the burn utility to get this image into flash.
	@# Note2: This image is intended to run out of flash as invoked
	@# directly at powerup when EVB64120A jumpers are configured to
	@# bypass the onboard eprom.
	@# Assumes that 0xBFC00000 is the bootup run address (normal MIPS).
	@# And assumes that EVB64120A jumber J11 is added to the board and jumber
	@# J20 is moved from the 2&3 position to the 1&2 position instead. Without
	@# the jumper settings the system will execute at address 0xBFC00000,
	@# as normal, yet that address will map to the onboard eeprom instead
	@# of the onboard flash.
	@#
	$(LD) -T ld.sys.big.Flash -o flashsys $(OBJECTS_flashsys)
	$(NM) flashsys | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | sort > System_flashsys.map
	@#
	@# Next, Create the image that we want to place in the flash part.
	$(OBJCOPY)  -S -g --strip-unneeded \
	      --adjust-section-vma=.text+0x3f820000 \
	      --adjust-section-vma=.rodata+0x3f820000 \
	      --adjust-section-vma=.reginfo+0x3f820000 \
	      --adjust-section-vma=.data+0x3f820000  \
	      --remove-section=.bss \
	      --remove-section=.scommon \
	      --remove-section=.note \
	      --remove-section=.comment \
	      flashsys flashsys.temp
	@#
	@# Next, change the addresses so that when we download to
	@# to the board's ram it will land starting at address 0xA0300000
	@# because this is where we have choosen to have the image temporarily sit
	@# while we subsequently burn it (using some method not revealed here) into
	@# the board's flash. After the burn the system can be setup (via jumpers)
	@# to boot this image directory from the flash part.
	$(OBJCOPY) -O srec --adjust-vma=0xe0700000 flashsys.temp flashsys.srec
	cp flashsys.srec $(TOPDIR)/.

flashsys2.srec : $(OBJECTS_flashsys2) ld.sys.big.Flash2
	@# Note1: Use the burn utility to get this image into flash.
	@# Note2: This image is intended to be run out of flash as invoked
	@# manually from the standard PMON running in eprom. This means that
	@# the image will be set to run from location 0xBF000000 which is the
	@# location the flash is mapped to when the board jumpers are set to
	@# the standard location such that the board boots out of onboard
	@# eprom. From the PMON prompt the user can type `call 0xbf000000`
	@# to transfer control to the image we are constructing here.
	@#
	$(LD) -T ld.sys.big.Flash2 -o flashsys2 $(OBJECTS_flashsys2)
	$(NM) flashsys2 | grep -v '\(compiled\)\|\(\.o$$\)\|\( a \)' | sort > System_flashsys2.map
	@#
	@# Next, Create the image that we want to place in the flash part.
	$(OBJCOPY)  -S -g --strip-unneeded \
	      --adjust-section-vma=.text+0x3ec20000 \
	      --adjust-section-vma=.rodata+0x3ec20000 \
	      --adjust-section-vma=.reginfo+0x3ec20000 \
	      --adjust-section-vma=.data+0x3ec20000  \
	      --remove-section=.bss \
	      --remove-section=.scommon \
	      --remove-section=.note \
	      --remove-section=.comment \
	      flashsys2 flashsys2.temp
	@#
	@# Next, change the addresses so that when we download to
	@# to the board's ram it will land starting at address 0xA0300000
	@# because this is where we have choosen to have the image temporarily sit
	@# while we subsequently burn it (using some method not revealed here) into
	@# the board's flash. After the burn a user will then be able to type
	@# `call 0xbf000000` at the PMON prompt (following a power cycle) to invoke
	@# the linux kernel.
	$(OBJCOPY) -O srec --adjust-vma=0xe1300000 flashsys2.temp flashsys2.srec
	cp flashsys2.srec $(TOPDIR)/.

burner.srec : $(OBJECTS_burner) ld.sys.big.burner
	@# This utility can be used to burn the flashsys.srec or flashsys2.srec
	@# into the EVB64120A's on board flash part (1Meg minimum).
	$(LD) -T ld.sys.big.burner -o burner $(OBJECTS_burner)
	$(NM) burner | grep -v '\(compiled\)\|\(\.o$$\)\|\( [aU] \)\|\(\.\.ng$$\)\|\(LASH[RL]DI\)' | sort > System_burner.map
	$(OBJCOPY) -O srec burner burner.srec
	cp burner.srec $(TOPDIR)/.

clean:
	rm -f doit piggy.S piggy.gz piggy burner *.o ramsys* flashsys* System*.map *.srec
