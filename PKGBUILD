pkgname=cassave
pkgver=0.4
pkgrel=1
pkgdesc="Data to Audio Converter"
arch=('x86_64')
url='https://github.com/Toby-420/cassave'
license=('GPL-3.0-or-later')

depends=('libsndfile')
makedepends=('make' 'gcc')

tar czf cassave-0.4.tar.gz --transform "s,^,cassave-$pkgver/," \
    src include text Makefile README.MD COPYING

source=("cassave-$pkgver.tar.gz")
sha256sums=('SKIP')

build() {
    cd "$pkgname-$pkgver"
    make
}

package() {
    cd "$pkgname-$pkgver"

    install -Dm755 bin/cassave "$pkgdir/usr/bin/cassave"

    install -d "$pkgdir/usr/share/cassave"
    cp -r text/* "$pkgdir/usr/share/cassave/"

    install -Dm644 README.MD "$pkgdir/usr/share/doc/cassave/README.MD"
}
