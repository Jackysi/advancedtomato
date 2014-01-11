EAPI=5

inherit autotools-utils

DESCRIPTION="a new easy-to-use high-speed software library for network communication, encryption, decryption, signatures, etc."
HOMEPAGE="http://download.libsodium.org/libsodium/releases/"
SRC_URI="http://download.libsodium.org/libsodium/releases/${P}.tar.gz"

LICENSE="BSD"
SLOT="0"
KEYWORDS="amd64 i386"

DOCS=(AUTHORS ChangeLog LICENSE README README.markdown THANKS)
