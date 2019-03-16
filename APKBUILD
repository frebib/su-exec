# Contributor: Natanael Copa <ncopa@alpinelinux.org>
# Maintainer: frebib <su-exec@frebib.net>
pkgname=su-exec
pkgver=0.4
pkgrel=0
pkgdesc="switch user and group id, setgroups and exec"
url="https://github.com/frebib/su-exec"
arch="all"
license="MIT"
subpackages="$pkgname-doc"
makedepends="vim"
options="!check"

prepare() {
    make clean
}
build() {
	make
}
package() {
    make PREFIX=/usr INSTALL_DIR=/usr/sbin DESTDIR="$pkgdir" install
}
