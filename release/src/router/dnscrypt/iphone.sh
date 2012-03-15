#! /bin/sh

export XCODEDIR="/Applications/Xcode.app/Contents/Developer"
export BASEDIR="${XCODEDIR}/Platforms/iPhoneOS.platform/Developer"
export PATH="${BASEDIR}/usr/bin:$BASEDIR/usr/sbin:$PATH"
export SDK="${BASEDIR}/SDKs/iPhoneOS5.0.sdk"
export CFLAGS="-pthread -mthumb -arch armv6 -isysroot ${SDK}"
export LDFLAGS="-pthread -mthumb -arch armv6 -isysroot ${SDK}"

./configure --host=arm-apple-darwin10 && make -j2
