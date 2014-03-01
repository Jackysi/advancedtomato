EAPI="3"

inherit eutils flag-o-matic

DESCRIPTION="A tool for securing communications between a client and a DNS resolver"
HOMEPAGE="http://dnscrypt.org"
SRC_URI="http://download.dnscrypt.org/dnscrypt-proxy/dnscrypt-proxy-${PV}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="amd64 i386"

RDEPEND="
	>=dev-libs/libsodium-0.4.2"
IUSE="-plugins"

pkg_setup() {
	enewgroup dnscrypt
	enewuser dnscrypt -1 -1 /var/empty dnscrypt
}

src_configure() {
	econf $(use_enable plugins)
}

src_install() {
	emake DESTDIR="${D}" install || die "emake install failed"

	newinitd "${FILESDIR}/dnscrypt-proxy_1_2_0.initd" dnscrypt-proxy || die "newinitd failed"
	newconfd "${FILESDIR}/dnscrypt-proxy_1_2_0.confd" dnscrypt-proxy || die "newconfd failed"

	dodoc {AUTHORS,COPYING,INSTALL,NEWS,README,README.markdown,TECHNOTES,THANKS} || die "dodoc failed"
}
