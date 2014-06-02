#!/bin/bash

# Thanks to the authors of the following pages for build commands and patches!
# - https://github.com/rave-engine/python3-android
# - http://bugs.python.org/review/20305/
# - http://datko.net/2013/05/10/cross-compiling-python-3-3-1-for-beaglebone-arm-angstrom/
# - https://github.com/kivy/python-for-android

set -e

export HOST_TOOLCHAIN_PREFIX="x86_64-unknown-linux-gnu"
export ANDROIDSDK="/opt/android-sdk"
export ANDROIDNDK="/opt/android-ndk"
export ANDROIDNDKVER=r9
export ANDROIDAPI=18

PYVER=3.4.0

if [ ! -d gcc ]; then
    ${ANDROIDNDK}/build/tools/make-standalone-toolchain.sh \
        --verbose \
        --platform=android-18 \
        --install-dir=gcc \
        --ndk-dir="${ANDROIDNDK}" \
        --system=linux-x86_64
fi

export NDKPLATFORM="$(pwd)/gcc"

push_arm() {
    export OLD_PATH=$PATH
    export OLD_CFLAGS=$CFLAGS
    export OLD_CXXFLAGS=$CXXFLAGS
    export OLD_LDFLAGS=$LDFLAGS
    export OLD_CC=$CC
    export OLD_CXX=$CXX
    export OLD_AR=$AR
    export OLD_RANLIB=$RANLIB
    export OLD_STRIP=$STRIP
    export OLD_MAKE=$MAKE
    export OLD_LD=$LD

    export CFLAGS="-DANDROID -mandroid -fomit-frame-pointer --sysroot ${NDKPLATFORM}/sysroot -I${NDKPLATFORM}/sysroot/usr/include"
    export CXXFLAGS="${CFLAGS}"
    export LDFLAGS="-L${NDKPLATFORM}/sysroot/usr/lib"

    export TOOLCHAIN_PREFIX=arm-linux-androideabi
    export TOOLCHAIN_VERSION=4.8

    export PATH="${NDKPLATFORM}/bin:${ANDROIDSDK}/tools:${PATH}"

    export CC="${TOOLCHAIN_PREFIX}-gcc"
    export CXX="${TOOLCHAIN_PREFIX}-g++"
    export CPP="${TOOLCHAIN_PREFIX}-cpp"
    export AR="${TOOLCHAIN_PREFIX}-ar"
    export AS="${TOOLCHAIN_PREFIX}-as"
    export LD="${TOOLCHAIN_PREFIX}-ld"
    export OBJCOPY="${TOOLCHAIN_PREFIX}-objcopy"
    export OBJDUMP="${TOOLCHAIN_PREFIX}-objdump"
    export RANLIB="${TOOLCHAIN_PREFIX}-ranlib"
    export STRIP="${TOOLCHAIN_PREFIX}-strip"
}

pop_arm() {
    export PATH=$OLD_PATH
    export CFLAGS=$OLD_CFLAGS
    export CXXFLAGS=$OLD_CXXFLAGS
    export LDFLAGS=$OLD_LDFLAGS
    export CC=$OLD_CC
    export CXX=$OLD_CXX
    export AR=$OLD_AR
    export LD=$OLD_LD
    export RANLIB=$OLD_RANLIB
    export STRIP=$OLD_STRIP
    export MAKE=$OLD_MAKE
}

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

cat >config.site <<EOF
ac_cv_file__dev_ptmx=no
ac_cv_file__dev_ptc=no
EOF

push_arm

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

make install \
  HOSTPGEN=./Parser/hostpgen

pop_arm

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
    ${NDKPLATFORM}/bin/${TOOLCHAIN_PREFIX}-strip ${i}
done

popd

tar Jcvf python-install-${PYVER}.tar.xz python-install/

popd
