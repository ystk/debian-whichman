# Copyright 1999-2003 Gentoo Technologies, Inc.
# Distributed under the terms of the GNU General Public License v2
# $Header: /home/cvsroot/gentoo-x86/net-misc/wget/wget-1.9-r2.ebuild,v 1.4 2003/12/05 02:13:04 seemant Exp $


IUSE=""

DESCRIPTION="fault tolerant search utilities"
HOMEPAGE="http://main.linuxfocus.org/~guido/"
SRC_URI="http://main.linuxfocus.org/~guido/${P}.tar.gz"

SLOT="0"
LICENSE="GPL-2"
KEYWORDS="x86 sparc alpha amd64 ia64"

DEPEND="virtual/glibc
	sys-devel/gcc"


src_unpack() {
	unpack ${P}.tar.gz
}

src_compile() {
	emake || die
}

src_install() {
	#make prefix=${D}/usr install || die
	make DESTDIR=${D} install || die
	dodoc README
}
