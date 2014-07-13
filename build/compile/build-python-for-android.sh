#!/bin/bash

# Thanks to the authors of the following pages for build commands and patches!
# - https://github.com/rave-engine/python3-android
# - http://bugs.python.org/review/20305/
# - http://datko.net/2013/05/10/cross-compiling-python-3-3-1-for-beaglebone-arm-angstrom/
# - https://github.com/kivy/python-for-android

set -e

. env.sh

PYVER=3.4.1

CURDIR="$(cd "$(dirname "${0}")" && pwd)"
BUILDDIR="${CURDIR}/build/"
mkdir -p "${BUILDDIR}"

pushd "${BUILDDIR}"

if [ ! -f Python-${PYVER}.tar.xz ]; then
    wget http://python.org/ftp/python/${PYVER}/Python-${PYVER}.tar.xz
fi

rm -rf "${PYDIR}"
tar Jxvf Python-${PYVER}.tar.xz

pushd Python-${PYVER}

PYDIR="$(pwd)"

./configure
make -j8 python Parser/pgen
mv python hostpython
mv Parser/pgen Parser/hostpgen
make distclean

patch -p1 -i ${CURDIR}/crosscompile.patch
patch -p1 -i ${CURDIR}/fix-locale.patch
patch -p1 -i ${CURDIR}/fix-gethostbyaddr.patch
patch -p1 -i ${CURDIR}/fix-filesystemdefaultencoding.patch
patch -p1 -i ${CURDIR}/fix-termios.patch
patch -p1 -i ${CURDIR}/fix-dlfcn.patch
patch -p1 -i ${CURDIR}/fix-subprocess.patch
patch -p1 -i ${CURDIR}/fix-module-linking.patch
patch -p1 -i ${CURDIR}/0001-Compile-with-pie.patch
patch -p1 -i ${CURDIR}/0002-Remove-log2.patch

cat >config.site <<EOF
ac_cv_file__dev_ptmx=no
ac_cv_file__dev_ptc=no
EOF

setup_toolchain
export CFLAGS="-DANDROID -mandroid -fomit-frame-pointer --sysroot ${NDKPLATFORM}/sysroot -I${NDKPLATFORM}/sysroot/usr/include -fstack-protector-all -D_FORTIFY_SOURCE=2"
export CXXFLAGS="${CFLAGS}"
export LDFLAGS="-L${NDKPLATFORM}/sysroot/usr/lib -Wl,-z,noexecstack -Wl,-z,now -Wl,-z,relro"

rm -rf "${BUILDDIR}/python-install"

export PYTHON_FOR_BUILD=" \\
    PYTHONHOME=$(pwd) \\
    _PYTHON_PROJECT_BASE=$(pwd) \\
    _PYTHON_HOST_PLATFORM=linux-arm \\
    PYTHONPATH=${PYDIR}/Lib:${PYDIR}/Lib/plat-linux:${PYDIR}/build/lib.linux-arm-3.4 \\
    ${PYDIR}/hostpython
"

CONFIG_SITE=config.site \
./configure \
    CROSS_COMPILE_TARGET=yes \
    --host=${TOOLCHAIN_PREFIX} \
    --build=${HOST_TOOLCHAIN_PREFIX} \
    --prefix="${BUILDDIR}/python-install" \
    --disable-shared \
    --disable-ipv6 \
    --with-ensurepip=no

sed -i '/HAVE_LOG2/s/^.*$/#define HAVE_LOG2 0/g' pyconfig.h

make install \
  HOSTPGEN=./Parser/hostpgen

popd

pushd python-install

find -type d -name '__pycache__' | xargs rm -rf
find -type f -name '*.a' -delete
find -type f -name '*.o' -delete
rm -r lib/python3.4/test/
rm -r lib/python3.4/ctypes/test/
rm -r lib/python3.4/distutils/tests/
rm -r lib/python3.4/idlelib/idle_test/
rm -r lib/python3.4/lib2to3/tests/
rm -r lib/python3.4/sqlite3/test/
rm -r lib/python3.4/tkinter/test/
rm -r lib/python3.4/unittest/test/
rm -r lib/pkgconfig/
rm -r include/
rm -r share/
rm bin/2to3*
rm bin/idle3*
rm bin/pydoc3*
rm bin/python3*-config
rm bin/pyvenv*

for i in $(LANG=C find -type f | xargs file | grep "not stripped" | cut -d: -f1); do
    chmod 755 ${i}
    ${STRIP} ${i}
done

popd

tar Jcvf python-install-${PYVER}.tar.xz python-install/

popd
