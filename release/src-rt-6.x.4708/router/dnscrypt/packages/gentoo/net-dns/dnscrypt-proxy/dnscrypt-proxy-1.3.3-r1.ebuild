EAPI=5

inherit autotools-utils

DESCRIPTION="A tool for securing communications between a client and a DNS resolver"
HOMEPAGE="http://dnscrypt.org"
SRC_URI="http://download.dnscrypt.org/dnscrypt-proxy/${P}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="amd64 i386"

RDEPEND="
	>=dev-libs/libsodium-0.4.2"
IUSE="-plugins"

AUTOTOOLS_IN_SOURCE_BUILD=1

DOCS=(AUTHORS COPYING INSTALL NEWS README README.markdown TECHNOTES THANKS)

PATCHES=(
	"${FILESDIR}/0001-Handle-disable-plugins-correctly-in-configure.ac.patch"
)
AUTOTOOLS_AUTORECONF=1

pkg_setup() {
	enewgroup dnscrypt
	enewuser dnscrypt -1 -1 /var/empty dnscrypt
}

src_configure() {
	local myeconfargs=(
		$(use_enable plugins)
	)
	autotools-utils_src_configure
}

src_install() {
	autotools-utils_src_install

	newinitd "${FILESDIR}/dnscrypt-proxy_1_2_0.initd" dnscrypt-proxy || die "newinitd failed"
	newconfd "${FILESDIR}/dnscrypt-proxy_1_2_0.confd" dnscrypt-proxy || die "newconfd failed"
}
