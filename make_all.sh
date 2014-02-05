PATH=/usr/local/bin:/bin:/usr/bin:/usr/X11R6/bin:/sbin:/usr/bin
PATH=$PATH:/home/gpl/cybertan/brcm_toolchain/tools/brcm/hndtools-mipsel-linux/bin
PATH=$PATH:/home/gpl/cybertan/brcm_toolchain/tools/brcm/hndtools-mipsel-uclibc/bin
export PATH

cd release/src
make

cd ..
cd ..
cp release/image/code.bin Result
