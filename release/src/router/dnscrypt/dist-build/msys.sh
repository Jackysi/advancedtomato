#! /bin/sh

export CFLAGS="-Os -march=pentium2 -mtune=nocona"
export PREFIX="$(pwd)/dnscrypt-proxy-win32"

./configure --prefix="$PREFIX" --exec-prefix="$PREFIX" \
  --sbindir="${PREFIX}/bin" \
  --enable-plugins \
  --with-included-ltdl && \
make install-strip

rm -fr "${PREFIX}/share"
rm -fr "${PREFIX}/lib/pkgconfig"
mv "${PREFIX}/lib/dnscrypt-proxy" "${PREFIX}/plugins"
rm -fr "${PREFIX}/lib"
cp /usr/local/lib/libldns-1.dll "${PREFIX}/bin"
cp /usr/local/lib/libsodium-4.dll "${PREFIX}/bin"

if false; then
  upx --best --ultra-brute "${PREFIX}/dnscrypt-proxy.exe" &
  upx --best --ultra-brute "${PREFIX}/hostip.exe"
  wait
fi
