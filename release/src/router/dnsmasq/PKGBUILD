# Maintainer: Sung Pae <self@sungpae.com>
pkgname=dnsmasq-nerv
pkgver=
pkgrel=1
pkgdesc="Custom dnsmasq build"
arch=('x86_64')
url="https://github.com/guns/dnsmasq"
license=('GPL')
groups=('nerv')
backup=('etc/dnsmasq/dnsmasq.conf'
        'etc/dnsmasq/resolv.conf')
depends=('glibc' 'libidn')
install=dnsmasq.install
makedepends=('git')
provides=('dnsmasq')
conflicts=('dnsmasq')
replaces=('dnsmasq-guns')

envmake() {
    DESTDIR="$pkgdir/" PREFIX=/usr BINDIR=/usr/bin RCDIR= \
    make -e -j $(grep -c ^processor /proc/cpuinfo) "$@"
}

pkgver() {
    printf %s "$(git describe --long | tr - .)"
}

build() {
    cd "$startdir"
    envmake all-i18n
}

package() {
    cd "$startdir"
    envmake install
}
