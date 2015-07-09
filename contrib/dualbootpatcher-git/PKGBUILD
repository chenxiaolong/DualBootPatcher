# Maintainer: Andrew Gunnerson <andrewgunnerson@gmail.com>

pkgname=dualbootpatcher-git
pkgver=latest
pkgrel=1
pkgdesc="Dual Boot Patcher for Android ROMs"
arch=(i686 x86_64)
url="https://snapshots.noobdev.io/"
license=('GPL')
depends=(qt5-base)
makedepends=(android-ndk cmake gcc-multilib)
provides=(dualbootpatcher)
conflicts=(dualbootpatcher)
source=(git+https://github.com/chenxiaolong/DualBootPatcher.git)
sha512sums=('SKIP')

pkgver() {
    cd DualBootPatcher
    git describe --long | sed 's/\([^-]*-g\)/r\1/;s/-/./g'
}

prepare() {
    cd DualBootPatcher
    git submodule update --init --recursive
}

build() {
    cd DualBootPatcher
    mkdir -p build
    cd build
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DCMAKE_INSTALL_LIBDIR=lib
	make
}

package() {
    cd DualBootPatcher/build
	make DESTDIR="${pkgdir}/" install
}
