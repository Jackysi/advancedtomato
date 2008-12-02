To build the uClibc from the provided sources:
 o cd uClibc
 o [make menuconfig] or [make config] to change configuration
   (or [make oldconfig] to rebuild existing configuration)
 o make
 o make install
Note: building uClibc requires an installed GNU toolchain.


To make the GNU toolchain
 o cd to ../release/src/router
 o make menuconfig
   save defaults and return
 o Get back to the tools-src directory and run 
   build_tools.sh
 o Set up a symbolic link in the /opt/brcm directory
   ln -s hndtools-mipsel-uclibc-0.9.19 hndtools-mipsel-uclibc
   ln -s hndtools-mipsel-linux-3.2.3 hndtools-mipsel-linux
   Add these two directories to your path


Now to build the image
 o cd ../release/src
 o make
