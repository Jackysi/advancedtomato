#! /bin/sh

export XCODEDIR="/Applications/Xcode5-DP.app/Contents/Developer"
export BASEDIR="${XCODEDIR}/Platforms/iPhoneOS.platform/Developer"
export PATH="${BASEDIR}/usr/bin:$BASEDIR/usr/sbin:$PATH"
export SDK="${BASEDIR}/SDKs/iPhoneOS7.0.sdk"
export CFLAGS="-Oz -mthumb -arch armv7 -isysroot ${SDK}"
export LDFLAGS="-mthumb -arch armv7 -isysroot ${SDK}"
export PREFIX="$(pwd)/dnscrypt-proxy-ios"

export SODIUM_IOS_PREFIX="/tmp/libsodium-ios"
export CPPFLAGS="$CPPFLAGS -I${SODIUM_IOS_PREFIX}/include"
export CPPFLAGS="$CPPFLAGS -DUSE_ONLY_PORTABLE_IMPLEMENTATIONS=1"
export LDFLAGS="$LDFLAGS -L${SODIUM_IOS_PREFIX}/lib"

./configure --host=arm-apple-darwin10 \
            --disable-shared \
            --prefix="$PREFIX" && \
make -j3 install && \
echo "dnscrypt-proxy has been installed into $PREFIX" && \
echo 'Now, using codesign(1) to sign dnscrypt-proxy'
