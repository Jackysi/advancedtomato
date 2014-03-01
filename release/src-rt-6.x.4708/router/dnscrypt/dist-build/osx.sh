#! /bin/sh

export CFLAGS="-mmacosx-version-min=10.6 -arch x86_64 -arch i386"
export LDFLAGS="-mmacosx-version-min=10.6 -arch x86_64 -arch i386"

./configure --with-included-ltdl \
            --enable-plugins \
            --enable-plugins-root && \
make -j3
