#! /bin/sh

export XCODEDIR=$(xcode-select -p)
export BASEDIR="${XCODEDIR}/Platforms/iPhoneOS.platform/Developer"
export PATH="${BASEDIR}/usr/bin:$BASEDIR/usr/sbin:$PATH"
export SDK="${BASEDIR}/SDKs/iPhoneOS.sdk"
export IPHONEOS_VERSION_MIN="5.1.1"
export CFLAGS="-Oz -mthumb -arch armv7 -isysroot ${SDK} -miphoneos-version-min=${IPHONEOS_VERSION_MIN}"
export LDFLAGS="-mthumb -arch armv7 -isysroot ${SDK} -miphoneos-version-min=${IPHONEOS_VERSION_MIN}"
export PREFIX="$(pwd)/dnscrypt-proxy-ios"

export SODIUM_IOS_PREFIX="/tmp/libsodium-ios"
export CPPFLAGS="$CPPFLAGS -I${SODIUM_IOS_PREFIX}/include"
export CPPFLAGS="$CPPFLAGS -DUSE_ONLY_PORTABLE_IMPLEMENTATIONS=1"
export LDFLAGS="$LDFLAGS -L${SODIUM_IOS_PREFIX}/lib"

./configure --host=arm-apple-darwin10 \
            --disable-shared \
            --prefix="$PREFIX" && \
make -j3 install && \
sed 's#/usr/local/#/usr/#g' < org.dnscrypt.osx.DNSCryptProxy.plist > \
  "$PREFIX/org.dnscrypt.osx.DNSCryptProxy.plist" && \
cp README-iOS.markdown "$PREFIX/" && \
echo "dnscrypt-proxy has been installed into $PREFIX" && \
echo 'Now, using codesign(1) to sign dnscrypt-proxy'
