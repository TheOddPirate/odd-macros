# Maintainer: Odd Østlie <theoddpirate@gmail.com>
pkgname=odd-macros
pkgver=1.0.0
pkgrel=1
pkgdesc="A small daemon that registers global shortcuts from your config file and executes selected macro on trigger for Wayland KDE"
arch=('x86_64')
url="https://github.com/TheOddPirate/odd-macros"
license=('LGPL-2.1-or-later')
depends=(
    'qt6-base'
    'kglobalaccel'
    'kconfig'
    'libxkbcommon'
)
makedepends=(
    'cmake'
    'extra-cmake-modules'
    'pkgconf'
)
optdepends=(
    'input: Group needed to write to /dev/uinput without root permissions'
)
source=("git+${url}.git")
sha256sums=('SKIP')

build() {
    cmake -B build -S "$pkgname" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}

post_install() {
    echo ""
    echo "=========================================================================="
    echo "  IMPORTANT SETUP FOR AIO-DAEMON:"
    echo "  1. Your user must be in the 'input' group to access /dev/uinput:"
    echo "     sudo usermod -aG input \$USER"
    echo "     (Log out and log back in for changes to take effect)"
    echo ""
    echo "  2. To start the daemon automatically for your user:"
    echo "     systemctl --user enable --now odd-macros-daemon.service"
    echo "=========================================================================="
    echo ""
}

post_upgrade() {
    post_install
}


post_remove() {
    echo ""
    echo "=========================================================================="
    echo "  REMINDER FOR ODD-MACROS:"
    echo "  If the user service was active, stop and disable it by running:"
    echo "     systemctl --user disable --now odd-macros-daemon.service"
    echo "=========================================================================="
    echo ""
}