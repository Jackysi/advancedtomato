#!/bin/bash
#
# build_tools.sh - Do a build of the MIPS toolchain & glibc
#
# Copyright 2001,2003,2004 Broadcom Corporation
#
#

usage ()
{
cat <<EOF
    Usage: $script [-d base_dir] [-k kernel_dir] [-i install_base]
    -d base_dir
              The base directory where the source will be extracted to and
              the release will be built.  If no -d option is provided,
              the default base_dir is the current pwd.

    -k kernel_dir
              The base directory where a copy of the kernel sources,
	      already configured; can be found. If no -k option is
	      provided, the default kernel_dir is
	      $base_dir/../release/src/linux/linux.

    -i install_base
              The base directory where the resulting binaries will be
	      installed and run from.  If no -i option is provided, the
              default install_base is /opt/brcm.
	      
    (Note: arguments specified must be absolute paths.)

EOF
    exit 1;
}

checkerr()
{
    if [ $1 -ne 0 ]; then
	shift; echo "Error: $*"
	exit 1
    fi
}

report()
{
    if [ $# -lt 2 ]; then
        echo "Internal script error ${1:-(no rc)}"
	exit 1
    fi
    rc=$1; shift
    if [ $rc -eq 0 ]; then
        echo "$* succeeded."
    else
	echo "$* failed, $rc" | tr 'a-z' 'A-Z'
	exit $rc
    fi
}

RUN_DIR=`pwd`
BUILD_DIR=`pwd`
LINUX=${BUILD_DIR}/../release/src/linux/linux
INSTALL_BASE=$BUILD_DIR/../tools/brcm

while getopts 'd:i:k:h' OPT
do
    case "$OPT" in
    d)
	BUILD_DIR=$OPTARG;
	;;
    i)
	INSTALL_BASE=$OPTARG;
	;;
    k)
	LINUX=$OPTARG;
	;;
    h)
	usage;
	;;
    :)
	echo "Missing required argument for option \"$OPTARG\".\n";
	usage;
	;;
    ?)
	echo "Unrecognized option - \"$OPTARG\".\n";
	usage;
	;;
    esac
done

if [[ $BUILD_DIR != /* ]] || [[ $INSTALL_BASE != /* ]] || [[ ${LINUX:-/} != /* ]]; then
    echo "Directory arguments must be absolute paths."
    exit 1
fi

if [ "$OPTIND" -le "$#" ]; then
    usage;
fi

# Figure out install directory
INSTALL_DIR=${INSTALL_BASE}/hndtools-mipsel-linux-3.2.3

# Figure out GNU directory
GNU=${BUILD_DIR}/gnu
if [ ! -d ${GNU} ]; then
    echo "Can't find gnu (sources) in ${BUILD_DIR}"
    GNU=${RUN_DIR}/gnu
    if [ ! -d ${GNU} ]; then
	echo "Nor here (in ${RUN_DIR})"
        exit 1
    fi
    echo "Using gnu in current directory (${RUN_DIR})"
fi    

echo "BUILD_DIR=$BUILD_DIR"
echo "LINUX_DIR=$LINUX"
echo "INSTALL_DIR=$INSTALL_DIR"
echo "GNU=$GNU"

# Enter build directory
cd "${BUILD_DIR}"
checkerr $? "Cannot cd to ${BUILD_DIR}"
mkdir obj
checkerr $? "Cannot create ${BUILD_DIR}/obj"

echo -n "Building gnutools: "
date

OBJ=${BUILD_DIR}/obj
cd ${OBJ}

###############
# Do binutils
echo -n "Doing binutils: "
date

mkdir ${OBJ}/binutils && cd ${OBJ}/binutils
checkerr $? "Cannot cd to ${OBJ}/binutils"

${GNU}/binutils/configure -v \
    --prefix=${INSTALL_DIR} --target=mipsel-linux --with-bcm4710a0 \
  > ${BUILD_DIR}/,binutils-config.log 2>&1
report $? "Configure of binutils"

gmake > ${BUILD_DIR}/,binutils-build.log 2>&1
report $? "Build of binutils"

gmake install > ${BUILD_DIR}/,binutils-install.log 2>&1
report $? "Install of binutils"

# Copy additional static libraries
cp libiberty/libiberty.a opcodes/libopcodes.a bfd/libbfd.a ${INSTALL_DIR}/lib
report $? "Static library copy"

# Make sure installed tools can now be used
export PATH=${INSTALL_DIR}/bin:$PATH

###########################
# Do bootstrap gcc (xgcc)
echo -n "Doing xgcc: "
date

mkdir ${OBJ}/xgcc && cd ${OBJ}/xgcc
checkerr $? "Cannot cd to ${OBJ}/xgcc"

${GNU}/gcc/configure -v \
    --prefix=${INSTALL_DIR} --target=mipsel-linux --with-bcm4710a0 \
    --with-newlib --disable-shared --disable-threads --enable-languages=c \
  > ${BUILD_DIR}/,xgcc-config.log 2>& 1
report $? "Configure of xgcc"

gmake > ${BUILD_DIR}/,xgcc-build.log 2>& 1
report $? "Make of xgcc"

gmake install > ${BUILD_DIR}/,xgcc-install.log 2>&1
report $? "Install of xgcc"

#######################################
# Get kernel headers (to build glibc)

TARGET=mipsel-linux
INCDIR=$INSTALL_DIR/$TARGET/include
CROSS=$INSTALL_DIR/bin/$TARGET-

echo -n "Coping linux includes: "
date

cd ${LINUX}
checkerr $? "Cannot cd to ${LINUX}"
if [ ! -f .config -o ! -f include/linux/version.h ]; then
	echo "LINUX TREE IS NOT CONFIGURED"
	exit 1
fi

# Make include directory in INSTALL_DIR and copy include files
mkdir -p $INCDIR
checkerr $? "Cannot create ${INCDIR}"
cd include && tar -cf - asm asm-mips linux | tar -xf - -C $INCDIR
report $? "Linux include directory copy"

#########################
# Do glibc (using xgcc)
mkdir ${OBJ}/glibc{,-install} && cd ${OBJ}/glibc
checkerr $? "Cannot cd to ${OBJ}/glibc"

echo -n "Doing glibc: "
date

CFLAGS="-O2 -g -finline-limit=10000" ${GNU}/glibc/configure \
    --build=i686-pc-linux-gnu --host=${TARGET} --prefix= \
    --with-headers=${INCDIR} --enable-add-ons \
  > ${BUILD_DIR}/,glibc-config.log 2>&1
report $? "Configure of glibc"

gmake > ${BUILD_DIR}/,glibc-build.log 2>&1
report $? "Build of glibc"

# Separate install dir gives more control over what goes in
gmake install_root=${OBJ}/glibc-install install \
  > ${BUILD_DIR}/,glibc-install.log 2>&1
report $? "Install of glibc"

# Now copy over include and lib, not binary
cd ${OBJ}/glibc-install/include && tar -cf - . | tar -xf - -C ${INCDIR}
report $? "Copy of glibc include"
cd ${OBJ}/glibc-install/lib && tar -cf - . | tar -xf - -C ${INSTALL_DIR}/${TARGET}/lib
report $? "Copy of glibc lib"

echo "Fixing up libc.so locations"
sed -e s%/lib/%"${INSTALL_DIR}/${TARGET}/lib/"%g \
	< "${INSTALL_DIR}/${TARGET}/lib/libc.so" \
	> "${INSTALL_DIR}/${TARGET}/lib/.new.libc.so"
report $? "Fixup of libc.so"

mv -f "${INSTALL_DIR}/${TARGET}/lib/.new.libc.so" \
	"${INSTALL_DIR}/${TARGET}/lib/libc.so"
report $? "Replace of libc.so"

#######################
# Now do the full gcc

mkdir ${OBJ}/gcc && cd ${OBJ}/gcc
checkerr $? "Cannot cd to ${OBJ}/gcc"

echo -n "Doing full gcc: "
date

${GNU}/gcc/configure -v \
    --target=${TARGET} --prefix=${INSTALL_DIR} --with-bcm4710a0 \
    --with-headers=${INSTALL_DIR}/${TARGET}/include --enable-languages=c,c++ \
  > ${BUILD_DIR}/,gcc-config.log 2>&1
report $? "Configure of gcc"

gmake > ${BUILD_DIR}/,gcc-build.log 2>&1
report $? "Build of gcc"

gmake install > ${BUILD_DIR}/,gcc-install.log 2>&1
report $? "Install of gcc"

echo -n "build_tools done: "
date
