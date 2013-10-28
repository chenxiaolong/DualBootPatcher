#!/bin/bash

# Some stuff is copied from the Kivy build scripts

set -e

export ANDROIDSDK="/opt/android-sdk"
export ANDROIDNDK="/opt/android-ndk"
export ANDROIDNDKVER=r9
export ANDROIDAPI=18

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

  export CFLAGS="-DANDROID -mandroid -fomit-frame-pointer --sysroot ${NDKPLATFORM}"
  if [ "X${ARCH}" == "Xarmeabi-v7a" ]; then
    CFLAGS+=" -march=armv7-a -mfloat-abi=softfp -mfpu=vfp -mthumb"
  fi
  export CXXFLAGS="${CFLAGS}"

  export TOOLCHAIN_PREFIX=arm-linux-androideabi
  export TOOLCHAIN_VERSION=4.8

  export PATH="${ANDROIDNDK}:${ANDROIDSDK}/tools:${PATH}"
  export PATH="${ANDROIDNDK}/toolchains/${TOOLCHAIN_PREFIX}-${TOOLCHAIN_VERSION}/prebuilt/linux-x86_64/bin/:${PATH}"

  export CC="$TOOLCHAIN_PREFIX-gcc ${CFLAGS}"
  export CXX="$TOOLCHAIN_PREFIX-g++ ${CXXFLAGS}"
  export AR="$TOOLCHAIN_PREFIX-ar"
  export RANLIB="$TOOLCHAIN_PREFIX-ranlib"
  export LD="$TOOLCHAIN_PREFIX-ld"
  export STRIP="$TOOLCHAIN_PREFIX-strip --strip-unneeded"

  if which ccache >/dev/null; then
    export CC="ccache ${CC}"
    export CXX="ccache ${CXX}"
  fi
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

CURDIR="$(cd "$(dirname "${0}")" && cd ..  && pwd)"
cd "${CURDIR}"

cd androidbinaries
if [ ! -d python-for-android ]; then
  git clone https://github.com/kivy/python-for-android.git
  cd python-for-android
else
  cd python-for-android
  git pull
fi

CURDIR="$(pwd)"

export NDKPLATFORM="${ANDROIDNDK}/platforms/android-${ANDROIDAPI}/arch-arm"
export ARCH="armeabi"
#export ARCH="armeabi-v7a" # not tested yet.

BUILDDIR="${CURDIR}/build/"
RECIPES="${CURDIR}/recipes/"

rm -rf "${BUILDDIR}"
mkdir -p "${BUILDDIR}"

PYVER=2.7.2

pushd "${BUILDDIR}"
wget http://python.org/ftp/python/${PYVER}/Python-${PYVER}.tar.bz2

mkdir host
mkdir target

tar jxvf Python-${PYVER}.tar.bz2 -C host
tar jxvf Python-${PYVER}.tar.bz2 -C target

pushd "host/Python-${PYVER}"
cp "${RECIPES}/hostpython/Setup" Modules/Setup
./configure
make -j8
mv Parser/pgen "${BUILDDIR}/hostpgen"
mv python "${BUILDDIR}/hostpython"
popd

pushd "target/Python-${PYVER}"

patch -p1 -i ${RECIPES}/python/patches/Python-${PYVER}-xcompile.patch
patch -p1 -i ${RECIPES}/python/patches/disable-modules.patch
patch -p1 -i ${RECIPES}/python/patches/fix-locale.patch
patch -p1 -i ${RECIPES}/python/patches/fix-gethostbyaddr.patch
patch -p1 -i ${RECIPES}/python/patches/fix-setup-flags.patch
patch -p1 -i ${RECIPES}/python/patches/fix-filesystemdefaultencoding.patch
patch -p1 -i ${RECIPES}/python/patches/fix-termios.patch
patch -p1 -i ${RECIPES}/python/patches/custom-loader.patch
patch -p1 -i ${RECIPES}/python/patches/verbose-compilation.patch
patch -p1 -i ${RECIPES}/python/patches/fix-remove-corefoundation.patch
patch -p1 -i ${RECIPES}/python/patches/fix-dynamic-lookup.patch
patch -p1 -i ${RECIPES}/python/patches/fix-dlfcn.patch

cp "${RECIPES}/hostpython/Setup" Modules/Setup
cp "${BUILDDIR}/hostpython" .
cp "${BUILDDIR}/hostpgen" .

push_arm
./configure \
  --host=arm-eabi \
  --prefix="${BUILDDIR}/python-install" \
  --enable-shared \
  --disable-toolbox-glue \
  --disable-framework

make install \
  HOSTPYTHON=$(pwd)/hostpython \
  HOSTPGEN=$(pwd)/hostpgen \
  CROSS_COMPILE_TARGET=yes \
  INSTSONAME=libpython2.7.so || true

touch python.exe python

make install \
  HOSTPYTHON=$(pwd)/hostpython \
  HOSTPGEN=$(pwd)/hostpgen \
  CROSS_COMPILE_TARGET=yes \
  INSTSONAME=libpython2.7.so

pop_arm

popd

pushd python-install

find -type f -name '*.pyc' -delete
find -type f -name '*.pyo' -delete
find -type f -name '*.o' -delete
find -type f -name '*.a' -delete
rm -r lib/python2.7/test/
rm -r lib/python2.7/distutils/tests
rm -r lib/python2.7/lib2to3/tests
rm -r lib/python2.7/json/tests
rm -r lib/pkgconfig/
rm -r include
rm -r share

for i in $(LANG=C find -type f | xargs file | grep "not stripped" | cut -d: -f1); do
  chmod 755 ${i}
  ${ANDROIDNDK}/toolchains/${TOOLCHAIN_PREFIX}-${TOOLCHAIN_VERSION}/prebuilt/linux-x86_64/bin/${TOOLCHAIN_PREFIX}-strip ${i}
done

popd

tar Jcvf python-install-$(git rev-parse --short HEAD).tar.xz python-install/

popd