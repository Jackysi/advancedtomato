#! /bin/sh

if [ -z "$NDK_ROOT" ]; then
    echo "You should probably set NDK_ROOT to the directory containing"
    echo "the Android NDK"
fi

if [ ! -f ./configure ]; then
	echo "Can't find ./configure. Wrong directory or haven't run autogen.sh?"
	exit 1
fi

export NDK_PLATFORM=${NDK_PLATFORM:-android-14}
export NDK_ROOT=${NDK_ROOT:-/usr/local/Cellar/android-ndk/9}
export TARGET_ARCH=arm
export TARGET="${TARGET_ARCH}-linux-androideabi"
export MAKE_TOOLCHAIN="${NDK_ROOT}/build/tools/make-standalone-toolchain.sh"

export PREFIX="$(pwd)/dnscrypt-proxy-android"
export TOOLCHAIN_DIR="$(pwd)/android-toolchain"
export PATH="${PATH}:${TOOLCHAIN_DIR}"
export SODIUM_ANDROID_PREFIX=${SODIUM_ANDROID_PREFIX:-/tmp/libsodium-android}
export CPPFLAGS="$CPPFLAGS -I${SODIUM_ANDROID_PREFIX}/include"
export CPPFLAGS="$CPPFLAGS -DUSE_ONLY_PORTABLE_IMPLEMENTATIONS=1"
export CFLAGS="-Os -mthumb"
export LDFLAGS="$LDFLAGS -L${SODIUM_ANDROID_PREFIX}/lib"

$MAKE_TOOLCHAIN --platform="$NDK_PLATFORM" --arch="$TARGET_ARCH" \
                --install-dir="$TOOLCHAIN_DIR"

./configure --host=arm-linux-androideabi \
            --with-sysroot="${TOOLCHAIN_DIR}/sysroot" \
            --disable-shared \
            --disable-pie \
            --prefix="$PREFIX" && \
make -j3 install && \
echo "dnscrypt-proxy has been installed into $PREFIX"
